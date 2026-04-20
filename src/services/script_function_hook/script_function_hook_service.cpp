#include "script_function_hook_service.hpp"
#include "gta_util.hpp"
#include "util/scripts.hpp"
#include "hooking/call_hook.hpp"

namespace
{
	extern "C" std::uintptr_t g_script_vm_on_enter_end_jump_offset{};
	big::call_hook* g_script_vm_on_enter_end_hook{};

	extern "C" void script_vm_on_enter_end_handler();
	extern "C" void process_script_function_hooks(std::uint8_t*& ip, std::uint8_t*& base, rage::scrValue*& sp, rage::scrValue* fp, rage::scrThreadContext* ctx, std::uint8_t** code)
	{
		big::g_script_function_hook_service->process_hooks(ip, base, sp, fp, ctx, code);
	}
}

namespace big
{
	script_function_hook_service::script_function_hook_service()
	{
		g_script_vm_on_enter_end_jump_offset = memory::handle(g_pointers->m_gta.m_script_vm_on_enter_end).add(1).rip().as<std::uintptr_t>();

		g_script_vm_on_enter_end_hook = new call_hook(g_pointers->m_gta.m_script_vm_on_enter_end, script_vm_on_enter_end_handler);
		g_script_vm_on_enter_end_hook->enable();

		g_script_function_hook_service = this;
	}

	script_function_hook_service::~script_function_hook_service()
	{
		g_script_vm_on_enter_end_jump_offset = 0;

		g_script_vm_on_enter_end_hook->disable();
		delete g_script_vm_on_enter_end_hook;
		g_script_vm_on_enter_end_hook = nullptr;

		g_script_function_hook_service = nullptr;
	}

	void script_function_hook_service::process_hook(const function_hook& hook, std::uint8_t*& ip, std::uint8_t*& base, rage::scrValue*& sp, rage::scrValue* fp, std::uint8_t** code)
	{
		rage::scrValue ret_val[256]{};
		if (!hook.m_hook_func(fp, ret_val))
		{
			// push return value onto stack
			for (std::uint32_t i = 0; i < hook.m_ret_count; i++)
				(++sp)->Any = ret_val[i].Any;

			// jump to LEAVE
			std::int64_t offset = ip - base + (hook.m_end_ip - hook.m_start_ip);
			ip = &code[offset >> 14][offset & 0x3FFF] - 1;
			base = &ip[-offset];
		}
	}

	void script_function_hook_service::resolve_hook(function_hook& hook, rage::scrProgram* program)
	{
		auto location = scripts::get_code_location_by_pattern(program, memory::pattern(hook.m_pattern));
		if (!location)
		{
			LOG(FATAL) << "Failed to find pattern " << hook.m_name << " in script " << program->m_name;
			return;
		}

		LOG(VERBOSE) << "Found pattern " << hook.m_name << " at " << HEX_TO_UPPER(*location) << " in script " << program->m_name;

        std::uint32_t post_enter = *location + scripts::get_insn_size(program, *location);
		std::uint32_t pos = post_enter;

		while (pos < program->m_code_size)
		{
			std::uint8_t* op = program->get_code_address(pos);
			std::uint32_t size = scripts::get_insn_size(program, pos);

			if (*op == static_cast<uint8_t>(scrOpcode::LEAVE))
			{
				std::uint32_t next = pos + size;
				std::uint8_t* next_op = program->get_code_address(next);

				// If next op is end of code or ENTER, this is the last LEAVE of the function
				if (next >= program->m_code_size || *next_op == static_cast<uint8_t>(scrOpcode::ENTER))
				{
					hook.m_start_ip = post_enter;
					hook.m_end_ip = pos;
					hook.m_ret_count = op[2];
					hook.m_resolved = true;
					break;
				}
			}

			pos += size;
		}
	}

	void script_function_hook_service::process_hooks(std::uint8_t*& ip, std::uint8_t*& base, rage::scrValue*& sp, rage::scrValue* fp, rage::scrThreadContext* ctx, std::uint8_t** code)
	{
		for (const auto& hook : m_hooks)
		{
			if (!hook.m_resolved || hook.m_script != ctx->m_script_hash || hook.m_start_ip != static_cast<std::uint32_t>(ip - base))
				continue;

			process_hook(hook, ip, base, sp, fp, code);
		}
	}

	void script_function_hook_service::resolve_hooks(rage::scrProgram* program)
	{
		for (auto& hook : m_hooks)
		{
			if (hook.m_resolved || !program || hook.m_script != program->m_name_hash)
				continue;

			resolve_hook(hook, program);
		}
	}

	void script_function_hook_service::add_hook(rage::joaat_t script, const std::string& name, const std::string& pattern, const hook_func& hook_func)
	{
		// prevent duplicates
		for (const auto& hook : m_hooks)
		{
			if (hook.m_script == script && hook.m_name == name)
				return;
		}

		function_hook hook{};
		hook.m_script = script;
		hook.m_name = name;
		hook.m_pattern = pattern;
		hook.m_hook_func = hook_func;

		// if the program is already available, set the data now, otherwise we will try in init_native_tables
		if (auto program = gta_util::find_script_program(script))
			resolve_hook(hook, program);

		m_hooks.push_back(hook);
	}

    void script_function_hook_service::remove_hook(rage::joaat_t script, const std::string& name)
	{
		m_hooks.erase(std::remove_if(m_hooks.begin(), m_hooks.end(), [&](const function_hook& hook) {
            return hook.m_script == script && hook.m_name == name;
        }), m_hooks.end());
	}
}