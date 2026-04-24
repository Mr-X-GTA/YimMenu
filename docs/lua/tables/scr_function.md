# Table: scr_function

Table for calling and hooking GTA script functions. Calls must be made in the fiber pool.

## Functions (5)

### `call_script_function(script_name, function_name, pattern, return_type_string, args_)`

Calls a script function with the given arguments. Returns the return value as the given type.
**Example Usage:**
```lua
local value = scr_function.call_script_function("freemode", "wear_sunglasses_at_night", "69 42 06 66", "bool", {
   { "int", 69 },
   { "float", 4.20 },
   { "int", 666 }
})
```

- **Parameters:**
  - `script_name` (string): Name of the script.
  - `function_name` (string): Name of the function. This parameter needs to be unique.
  - `pattern` (string): Pattern to scan for within the script.
  - `return_type_string` (string): Return type of the function. Supported types are **"int"**, **"bool"**, **"const char\*/string"**, **"ptr/pointer/*"**, **"float"**, and **"vector3"**. Anything different will be rejected.
  - `args_` (table): Arguments to pass to the function. Supported types are the same as return types.

**Example Usage:**
```lua
scr_function.call_script_function(script_name, function_name, pattern, return_type_string, args_)
```

### `call_script_function(script_name, instruction_pointer, return_type_string, args_)`

Calls a script function directly using the function position with the given arguments. Returns the return value as the given type.
**Example Usage:**
```lua
local value = scr_function.call_script_function("freemode", 0xE792, "string", {
   { "int", 191 }
})
```

- **Parameters:**
  - `script_name` (string): Name of the script.
  - `instruction_pointer` (integer): Position of the function within the script.
  - `return_type_string` (string): Return type of the function. Supported types are **"int"**, **"bool"**, **"const char\*/string"**, **"ptr/pointer/*"**, **"float"**, and **"vector3"**. Anything different will be rejected.
  - `args_` (table): Arguments to pass to the function. Supported types are the same as return types.

**Example Usage:**
```lua
scr_function.call_script_function(script_name, instruction_pointer, return_type_string, args_)
```

### `add_script_function_hook(script_name, hook_name, pattern, hook_func)`

Hooks a script function. If the callback returns `false`, the original function is skipped, and values in `rets` are pushed to the stack. If `true`, the original function executes normally.

- **Parameters:**
  - `script_name` (string): Name of the script.
  - `hook_name` (string): Name of the hook. This parameter needs to be unique.
  - `pattern` (string): Pattern to scan for within the script.
  - `hook_func` (string): The callback function. It receives args and rets, which can be read or set via `get/set_int/uns/float/string/reference` methods. Return value determines whether to skip or execute the original function.

**Example Usage:**
```lua
scr_function.add_script_function_hook("some_script", "my_hook", "2D 00 ? ? 00 61", function(args, rets)
  local val = args:get_int(0)
  if val == 1 then
    rets:set_int(0, 100)
    return false
  end
  return true
end)
```

### `add_script_function_hook(script_name, hook_name, instruction_pointer, hook_func)`

Hooks a script function directly using the function position. If the callback returns `false`, the original function is skipped, and values in `rets` are pushed to the stack. If `true`, the original function executes normally.

- **Parameters:**
  - `script_name` (string): Name of the script.
  - `hook_name` (string): Name of the hook. This parameter needs to be unique.
  - `instruction_pointer` (int): Position of the function within the script.
  - `hook_func` (string): The callback function. It receives args and rets, which can be read or set via `get/set_int/uns/float/string/reference` methods. Return value determines whether to skip or execute the original function.

**Example Usage:**
```lua
scr_function.add_script_function_hook("some_script", "my_hook", 0x10BE, function(args, rets)
  local val = args:get_int(0)
  if val == 1 then
    rets:set_int(0, 100)
    return false
  end
  return true
end)
```

### `remove_script_function_hook(script_name, hook_name)`

Removes an existing script function hook.

- **Parameters:**
  - `script_name` (string): Name of the script associated with the hook.
  - `hook_name` (string): The unique name provided during add_script_function_hook.

**Example Usage:**
```lua
scr_function.remove_script_function_hook("some_script", "my_hook")
```