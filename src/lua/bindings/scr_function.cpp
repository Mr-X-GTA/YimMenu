#include "scr_function.hpp"

#include "gta_util.hpp"
#include "memory.hpp"
#include "memory/pattern.hpp"
#include "pointers.hpp"
#include "util/scripts.hpp"
#include "services/script_function_hook/script_function_hook_service.hpp"

namespace lua::scr_function
{
	template<typename Arg>
	void push_arg(uint64_t* stack, uint32_t& stack_pointer, Arg&& value)
	{
		*reinterpret_cast<std::remove_cv_t<std::remove_reference_t<Arg>>*>(reinterpret_cast<uint64_t*>(stack) + (stack_pointer++)) = std::forward<Arg>(value);
	}

	// Lua API: Table
	// Name: scr_function
	// Table for calling and hooking GTA script functions. Calls must be made in the fiber pool.

	// Lua API: function
	// Table: scr_function
	// Name: call_script_function
	// Param: script_name: string: Name of the script.
	// Param: function_name: string: Name of the function. This parameter needs to be unique.
	// Param: pattern: string: Pattern to scan for within the script.
	// Param: return_type_string: string: Return type of the function. Supported types are **"int"**, **"bool"**, **"const char\*/string"**, **"ptr/pointer/*"**, **"float"**, and **"vector3"**. Anything different will be rejected.
	// Param: args_: table: Arguments to pass to the function. Supported types are the same as return types.
	// Calls a script function with the given arguments. Returns the return value as the given type.
	// **Example Usage:**
	// ```lua
	// local value = scr_function.call_script_function("freemode", "wear_sunglasses_at_night", "69 42 06 66", "bool", {
	//   { "int", 69 },
	//   { "float", 4.20 },
	//   { "int", 666 }
	// })
	// ```

	static sol::object call_script_function_by_signature(const std::string& script_name, const std::string& function_name, const std::string& pattern, const std::string& return_type_string, sol::table args_, sol::this_state state_)
	{
		std::vector<lua::memory::type_info_t> param_types;
		std::vector<sol::object> actual_args;
		static std::unordered_map<std::string, int32_t> pattern_results;
		for (const auto& [k, v_] : args_)
		{
			if (v_.is<sol::table>())
			{
				auto v = v_.as<sol::table>();
				param_types.push_back(lua::memory::get_type_info_from_string(v[1].get<const char*>()));
				actual_args.push_back(v[2].get<sol::object>());
			}
		}

		const auto return_type = lua::memory::get_type_info_from_string(return_type_string);

		auto thread  = big::gta_util::find_script_thread(rage::joaat(script_name));
		auto program = big::gta_util::find_script_program(rage::joaat(script_name));

		if (!thread || !program)
		{
			LOG(FATAL) << "Failed to find " << script_name << " for " << function_name;
			return sol::lua_nil;
		}

		int32_t instruction_pointer;

		if (pattern_results.contains(function_name))
		{
			instruction_pointer = pattern_results[function_name];
		}
		else
		{
			const ::memory::pattern pattern_scan(pattern);

			auto location = big::scripts::get_code_location_by_pattern(program, pattern_scan);

			if (!location)
			{
				LOG(FATAL) << "Failed to find pattern " << function_name << " in script " << script_name;
				return sol::lua_nil;
			}
			else
			{
				LOG(VERBOSE) << "Found pattern for " << function_name << " at " << HEX_TO_UPPER(location.value()) << " in " << script_name;
			}

			pattern_results[function_name] = instruction_pointer = location.value();
		}

		auto tls_ctx                       = rage::tlsContext::get();
		auto stack                         = (uint64_t*)thread->m_stack;
		auto og_thread                     = tls_ctx->m_script_thread;
		tls_ctx->m_script_thread           = thread;
		tls_ctx->m_is_script_thread_active = true;
		rage::scrThreadContext ctx         = thread->m_context;
		auto top_stack                     = ctx.m_stack_pointer; // This will be the top item in the stack after the args and return address are cleaned off

		for (size_t i = 0; i < param_types.size(); i++)
		{
			switch (param_types[i])
			{
			case lua::memory::type_info_t::boolean_:
			{
				const auto val = actual_args[i].as<std::optional<bool>>();
				if (val)
				{
					push_arg(stack, ctx.m_stack_pointer, *val);
				}
				break;
			}
			case lua::memory::type_info_t::string_:
			{
				const auto val = actual_args[i].as<std::optional<const char*>>();
				if (val)
				{
					push_arg(stack, ctx.m_stack_pointer, *val);
				}
				break;
			}
			case lua::memory::type_info_t::integer_:
			{
				const auto val = actual_args[i].as<std::optional<int>>();
				if (val)
				{
					push_arg(stack, ctx.m_stack_pointer, *val);
				}
				break;
			}
			case lua::memory::type_info_t::ptr_:
			{
				const auto val = actual_args[i].as<std::optional<lua::memory::pointer>>();
				if (val)
				{
					push_arg(stack, ctx.m_stack_pointer, (*val).get_address());
				}
				break;
			}
			case lua::memory::type_info_t::float_:
			{
				const auto val = actual_args[i].as<std::optional<float>>();
				if (val)
				{
					push_arg(stack, ctx.m_stack_pointer, *val);
				}
				break;
			}
			case lua::memory::type_info_t::vector3_:
			{
				const auto val = actual_args[i].as<std::optional<Vector3>>();
				if (val)
				{
					push_arg(stack, ctx.m_stack_pointer, val.value().x);
					push_arg(stack, ctx.m_stack_pointer, val.value().y);
					push_arg(stack, ctx.m_stack_pointer, val.value().z);
				}
				break;
			}
			default: break;
			}
		}

		stack[ctx.m_stack_pointer++] = 0;
		ctx.m_instruction_pointer    = instruction_pointer;
		ctx.m_state                  = rage::eThreadState::running;

		big::g_pointers->m_gta.m_script_vm(stack, big::g_pointers->m_gta.m_script_globals, program, &ctx);

		tls_ctx->m_script_thread           = og_thread;
		tls_ctx->m_is_script_thread_active = og_thread != nullptr;

		if (return_type == lua::memory::type_info_t::boolean_)
		{
			return sol::make_object(state_, (bool)*reinterpret_cast<BOOL*>(stack + top_stack));
		}
		else if (return_type == lua::memory::type_info_t::string_)
		{
			return sol::make_object(state_, *reinterpret_cast<const char**>(stack + top_stack));
		}
		else if (return_type == lua::memory::type_info_t::integer_)
		{
			return sol::make_object(state_, *reinterpret_cast<int*>(stack + top_stack));
		}
		else if (return_type == lua::memory::type_info_t::ptr_)
		{
			return sol::make_object(state_, *reinterpret_cast<uint64_t*>(stack + top_stack));
		}
		else if (return_type == lua::memory::type_info_t::float_)
		{
			return sol::make_object(state_, *reinterpret_cast<float*>(stack + top_stack));
		}
		else if (return_type == lua::memory::type_info_t::vector3_)
		{
			return sol::make_object(state_, *reinterpret_cast<Vector3*>(stack + top_stack));
		}
		else
		{
			LOG(FATAL) << "Unimplemented return type " << return_type_string;
			return sol::lua_nil;
		}
	}

	// Lua API: function
	// Table: scr_function
	// Name: call_script_function
	// Param: script_name: string: Name of the script.
	// Param: instruction_pointer: integer: Position of the function within the script.
	// Param: return_type_string: string: Return type of the function. Supported types are **"int"**, **"bool"**, **"const char\*/string"**, **"ptr/pointer/*"**, **"float"**, and **"vector3"**. Anything different will be rejected.
	// Param: args_: table: Arguments to pass to the function. Supported types are the same as return types.
	// Calls a script function directly using the function position with the given arguments. Returns the return value as the given type.
	// **Example Usage:**
	// ```lua
	// local value = scr_function.call_script_function("freemode", 0xE792, "string", {
	//   { "int", 191 }
	// })
	// ```

	static sol::object call_script_function_by_instruction_pointer(const std::string& script_name, const int instruction_pointer, const std::string& return_type_string, sol::table args_, sol::this_state state_)
	{
		std::vector<lua::memory::type_info_t> param_types;
		std::vector<sol::object> actual_args;
		for (const auto& [k, v_] : args_)
		{
			if (v_.is<sol::table>())
			{
				auto v = v_.as<sol::table>();
				param_types.push_back(lua::memory::get_type_info_from_string(v[1].get<const char*>()));
				actual_args.push_back(v[2].get<sol::object>());
			}
		}

		const auto return_type = lua::memory::get_type_info_from_string(return_type_string);

		auto thread  = big::gta_util::find_script_thread(rage::joaat(script_name));
		auto program = big::gta_util::find_script_program(rage::joaat(script_name));

		if (!thread || !program || !instruction_pointer)
		{
			LOG(FATAL) << "Failed to run " << script_name << " script function at " << HEX_TO_UPPER(instruction_pointer);
			return sol::lua_nil;
		}

		auto tls_ctx                       = rage::tlsContext::get();
		auto stack                         = (uint64_t*)thread->m_stack;
		auto og_thread                     = tls_ctx->m_script_thread;
		tls_ctx->m_script_thread           = thread;
		tls_ctx->m_is_script_thread_active = true;
		rage::scrThreadContext ctx         = thread->m_context;
		auto top_stack                     = ctx.m_stack_pointer; // This will be the top item in the stack after the args and return address are cleaned off

		for (size_t i = 0; i < param_types.size(); i++)
		{
			switch (param_types[i])
			{
			case lua::memory::type_info_t::boolean_:
			{
				const auto val = actual_args[i].as<std::optional<bool>>();
				if (val)
				{
					push_arg(stack, ctx.m_stack_pointer, *val);
				}
				break;
			}
			case lua::memory::type_info_t::string_:
			{
				const auto val = actual_args[i].as<std::optional<const char*>>();
				if (val)
				{
					push_arg(stack, ctx.m_stack_pointer, *val);
				}
				break;
			}
			case lua::memory::type_info_t::integer_:
			{
				const auto val = actual_args[i].as<std::optional<int>>();
				if (val)
				{
					push_arg(stack, ctx.m_stack_pointer, *val);
				}
				break;
			}
			case lua::memory::type_info_t::ptr_:
			{
				const auto val = actual_args[i].as<std::optional<lua::memory::pointer>>();
				if (val)
				{
					push_arg(stack, ctx.m_stack_pointer, (*val).get_address());
				}
				break;
			}
			case lua::memory::type_info_t::float_:
			{
				const auto val = actual_args[i].as<std::optional<float>>();
				if (val)
				{
					push_arg(stack, ctx.m_stack_pointer, *val);
				}
				break;
			}
			case lua::memory::type_info_t::vector3_:
			{
				const auto val = actual_args[i].as<std::optional<Vector3>>();
				if (val)
				{
					push_arg(stack, ctx.m_stack_pointer, val.value().x);
					push_arg(stack, ctx.m_stack_pointer, val.value().y);
					push_arg(stack, ctx.m_stack_pointer, val.value().z);
				}
				break;
			}
			default: break;
			}
		}

		stack[ctx.m_stack_pointer++] = 0;
		ctx.m_instruction_pointer    = instruction_pointer;
		ctx.m_state                  = rage::eThreadState::running;

		big::g_pointers->m_gta.m_script_vm(stack, big::g_pointers->m_gta.m_script_globals, program, &ctx);

		tls_ctx->m_script_thread           = og_thread;
		tls_ctx->m_is_script_thread_active = og_thread != nullptr;

		if (return_type == lua::memory::type_info_t::boolean_)
		{
			return sol::make_object(state_, (bool)*reinterpret_cast<BOOL*>(stack + top_stack));
		}
		else if (return_type == lua::memory::type_info_t::string_)
		{
			return sol::make_object(state_, *reinterpret_cast<const char**>(stack + top_stack));
		}
		else if (return_type == lua::memory::type_info_t::integer_)
		{
			return sol::make_object(state_, *reinterpret_cast<int*>(stack + top_stack));
		}
		else if (return_type == lua::memory::type_info_t::ptr_)
		{
			return sol::make_object(state_, *reinterpret_cast<uint64_t*>(stack + top_stack));
		}
		else if (return_type == lua::memory::type_info_t::float_)
		{
			return sol::make_object(state_, *reinterpret_cast<float*>(stack + top_stack));
		}
		else if (return_type == lua::memory::type_info_t::vector3_)
		{
			return sol::make_object(state_, *reinterpret_cast<Vector3*>(stack + top_stack));
		}
		else
		{
			LOG(FATAL) << "Unimplemented return type " << return_type_string;
			return sol::lua_nil;
		}
	}

    // Lua API: Class
	// Name: scr_value_wrapper
	// Class for wrapping arguments and return values of GTA script functions, used by add_script_function_hook.
	class scr_value_wrapper_t
	{
		rage::scrValue* m_data;
		std::uint32_t m_count;

	public:
		scr_value_wrapper_t(rage::scrValue* data, std::uint32_t count) :
		    m_data(data),
		    m_count(count)
		{
		}

		// Lua API: Function
		// Class: scr_value_wrapper
		// Name: get_count
		// Returns: int: The number of elements in the wrapper.
		// Get the total number of elements in the wrapper.
		std::uint32_t get_count()
		{
			return m_count;
		}

		// Lua API: Function
		// Class: scr_value_wrapper
		// Name: get_int
		// Param: index: int: The index to access.
		// Returns: int: The current value.
		// Get the int value currently contained by the wrapper.
		std::int32_t get_int(std::uint32_t index)
		{
			return index < m_count ? m_data[index].Int : 0;
		}

		// Lua API: Function
		// Class: scr_value_wrapper
		// Name: set_int
		// Param: index: int: The index to access.
		// Param: value: int: The new value.
		// Set the int value contained by the wrapper.
		void set_int(std::uint32_t index, std::int32_t value)
		{
			if (index < m_count)
				m_data[index].Int = value;
		}

		// Lua API: Function
		// Class: scr_value_wrapper
		// Name: get_uns
		// Param: index: int: The index to access.
		// Returns: int: The current unsigned value.
		// Get the unsigned int value currently contained by the wrapper.
		std::uint32_t get_uns(std::uint32_t index)
		{
			return index < m_count ? m_data[index].Uns : 0;
		}

		// Lua API: Function
		// Class: scr_value_wrapper
		// Name: set_uns
		// Param: index: int: The index to access.
		// Param: value: int: The new unsigned value.
		// Set the unsigned int value contained by the wrapper.
		void set_uns(std::uint32_t index, std::uint32_t value)
		{
			if (index < m_count)
				m_data[index].Uns = value;
		}

		// Lua API: Function
		// Class: scr_value_wrapper
		// Name: get_float
		// Param: index: int: The index to access.
		// Returns: float: The current value.
		// Get the float value currently contained by the wrapper.
		float get_float(std::uint32_t index)
		{
			return index < m_count ? m_data[index].Float : 0.0f;
		}

		// Lua API: Function
		// Class: scr_value_wrapper
		// Name: set_float
		// Param: index: int: The index to access.
		// Param: value: float: The new value.
		// Set the float value contained by the wrapper.
		void set_float(std::uint32_t index, float value)
		{
			if (index < m_count)
				m_data[index].Float = value;
		}

		// Lua API: Function
		// Class: scr_value_wrapper
		// Name: get_string
		// Param: index: int: The index to access.
		// Returns: string: The current value.
		// Get the string value currently contained by the wrapper.
		const char* get_string(std::uint32_t index)
		{
			return index < m_count ? m_data[index].String : nullptr;
		}

		// Lua API: Function
		// Class: scr_value_wrapper
		// Name: set_string
		// Param: index: int: The index to access.
		// Param: value: string: The new value.
		// Set the string value contained by the wrapper.
		void set_string(std::uint32_t index, const char* value)
		{
			if (index < m_count)
				m_data[index].String = value;
		}

		// Lua API: Function
		// Class: scr_value_wrapper
		// Name: get_reference
		// Param: index: int: The index to access.
		// Returns: class: The current value.
		// Get the reference value currently contained by the wrapper.
		scr_value_wrapper_t get_reference(std::uint32_t index)
		{
			return index < m_count ? scr_value_wrapper_t(m_data[index].Reference, m_count) : scr_value_wrapper_t(nullptr, 0);
		}
	};

	// Lua API: function
	// Table: scr_function
	// Name: add_script_function_hook
	// Param: script_name: string: Name of the script.
	// Param: hook_name: string: Name of the hook. This parameter needs to be unique.
	// Param: pattern: string: Pattern to scan for within the script.
	// Param: hook_func: function: The callback function. It receives args and rets, which can be read or set via `get/set_int/float/string` methods. Return value determines whether to skip or execute the original function.
	// Hooks a script function. If the callback returns `false`, the original function is skipped, and values in `rets` are pushed to the stack. If `true`, the original function executes normally.
	// **Example Usage:**
	// ```lua
	// scr_function.add_script_function_hook("some_script", "my_hook", "2D 00 ? ? 00 61", function(args, rets)
	//   local val = args:get_int(0)
	//   if val == 1 then
	//     rets:set_int(0, 100)
	//     return false
	//   end
	//   return true
	// end)
	// ```
	static void add_script_function_hook_by_signature(const std::string& script_name, const std::string& hook_name, const std::string& pattern, sol::protected_function hook_func)
	{
		big::g_script_function_hook_service->add_hook(rage::joaat(script_name), hook_name, pattern, [hook_func](rage::scrValue* args, const std::uint32_t argCount, rage::scrValue* rets, const std::uint32_t retCount) -> bool {
			scr_value_wrapper_t lua_args(args, argCount);
			scr_value_wrapper_t lua_rets(rets, retCount);

			auto result = hook_func(lua_args, lua_rets);
			if (!result.valid())
				return true; // don't skip the original if lua fails

			return result.get<bool>();
		});
	}

	// Lua API: function
	// Table: scr_function
	// Name: add_script_function_hook
	// Param: script_name: string: Name of the script.
	// Param: hook_name: string: Name of the hook. This parameter needs to be unique.
	// Param: instruction_pointer: integer: Position of the function within the script.
	// Param: hook_func: function: The callback function. It receives args and rets, which can be read or set via `get/set_int/float/string` methods. Return value determines whether to skip or execute the original function.
	// Hooks a script function directly using the function position. If the callback returns `false`, the original function is skipped, and values in `rets` are pushed to the stack. If `true`, the original function executes normally.
	// **Example Usage:**
	// ```lua
	// scr_function.add_script_function_hook("some_script", "my_hook", 0x10BE, function(args, rets)
	//   local val = args:get_int(0)
	//   if val == 1 then
	//     rets:set_int(0, 100)
	//     return false
	//   end
	//   return true
	// end)
	// ```
	static void add_script_function_hook_by_instruction_pointer(const std::string& script_name, const std::string& hook_name, const std::uint32_t instruction_pointer, sol::protected_function hook_func)
	{
		big::g_script_function_hook_service->add_hook(rage::joaat(script_name), hook_name, instruction_pointer, [hook_func](rage::scrValue* args, const std::uint32_t argCount, rage::scrValue* rets, const std::uint32_t retCount) -> bool {
			scr_value_wrapper_t lua_args(args, argCount);
			scr_value_wrapper_t lua_rets(rets, retCount);

			auto result = hook_func(lua_args, lua_rets);
			if (!result.valid())
				return true; // don't skip the original if lua fails

			return result.get<bool>();
		});
	}

    // Lua API: function
	// Table: scr_function
	// Name: remove_script_function_hook
	// Param: script_name: string: Name of the script associated with the hook.
	// Param: hook_name: string: The unique name provided during add_script_function_hook.
	// Removes an existing script function hook.
	// **Example Usage:**
	// ```lua
	// scr_function.remove_script_function_hook("some_script", "my_hook")
	// ```
    static void remove_script_function_hook(const std::string& script_name, const std::string& hook_name)
	{
        big::g_script_function_hook_service->remove_hook(rage::joaat(script_name), hook_name);
	}

	void bind(sol::state& state)
	{
		auto ut = state.new_usertype<scr_value_wrapper_t>("scr_value_wrapper");

        ut["get_count"]     = &scr_value_wrapper_t::get_count;
		ut["get_int"]       = &scr_value_wrapper_t::get_int;
		ut["set_int"]       = &scr_value_wrapper_t::set_int;
		ut["get_uns"]       = &scr_value_wrapper_t::get_uns;
		ut["set_uns"]       = &scr_value_wrapper_t::set_uns;
		ut["get_float"]     = &scr_value_wrapper_t::get_float;
		ut["set_float"]     = &scr_value_wrapper_t::set_float;
		ut["get_string"]    = &scr_value_wrapper_t::get_string;
		ut["set_string"]    = &scr_value_wrapper_t::set_string;
		ut["get_reference"] = &scr_value_wrapper_t::get_reference;

		auto ns = state["scr_function"].get_or_create<sol::table>();

		ns["call_script_function"]        = sol::overload(call_script_function_by_signature, call_script_function_by_instruction_pointer);
		ns["add_script_function_hook"]    = sol::overload(add_script_function_hook_by_signature, add_script_function_hook_by_instruction_pointer);
		ns["remove_script_function_hook"] = remove_script_function_hook;
	}
}