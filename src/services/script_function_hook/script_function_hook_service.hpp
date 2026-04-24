#pragma once

namespace rage
{
	union scrValue;
	class scrThreadContext;
	class scrProgram;
}

namespace big
{
	class script_function_hook_service
	{
	public:
		using hook_func = std::function<bool(rage::scrValue* args, const std::uint32_t argCount, rage::scrValue* rets, const std::uint32_t retCount)>;

		script_function_hook_service();
		~script_function_hook_service();

		void process_hooks(std::uint8_t*& ip, std::uint8_t*& base, rage::scrValue*& sp, rage::scrValue* fp, rage::scrThreadContext* ctx, std::uint8_t** code);
		void resolve_hooks(rage::scrProgram* program);
		void add_hook(rage::joaat_t script, const std::string& name, const std::string& pattern, const hook_func& hook_func);
		void add_hook(rage::joaat_t script, const std::string& name, std::uint32_t ip, const hook_func& hook_func);
		void remove_hook(rage::joaat_t script, const std::string& name);

	private:
		struct function_hook
		{
			rage::joaat_t m_script;
			std::string m_name;
			std::string m_pattern;
			std::uint32_t m_start_ip;
			std::uint32_t m_end_ip;
			std::uint32_t m_arg_count;
			std::uint32_t m_ret_count;
			hook_func m_hook_func;
			bool m_resolved;
			bool m_active;
		};

		static void process_hook(function_hook& hook, std::uint8_t*& ip, std::uint8_t*& base, rage::scrValue*& sp, rage::scrValue* fp, std::uint8_t** code);
		static void resolve_hook(function_hook& hook, rage::scrProgram* program);

		std::vector<function_hook> m_hooks;
	};

	inline script_function_hook_service* g_script_function_hook_service{};
}