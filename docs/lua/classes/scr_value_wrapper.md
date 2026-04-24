# Class: scr_value_wrapper

Class for wrapping arguments and return values of GTA script functions, used by add_script_function_hook.

## Functions (10)

### `get_count()`

Get the total number of elements in the wrapper.

- **Returns:**
  - `int`: The number of elements in the wrapper.

**Example Usage:**
```lua
count = scr_value_wrapper:get_count()
```

---

### `get_int()`

Get the int value currently contained by the wrapper.

- **Parameters:**
  - `index` (int): The index to access.

- **Returns:**
  - `int`: The current value.

**Example Usage:**
```lua
value = scr_value_wrapper:get_int(0)
```

---

### `set_int()`

Set the int value contained by the wrapper.

- **Parameters:**
  - `index` (int): The index to access.
  - `value` (int): The new value.

**Example Usage:**
```lua
scr_value_wrapper:set_int(0, 42)
```

---

### `get_uns()`

Get the unsigned int value currently contained by the wrapper.

- **Parameters:**
  - `index` (int): The index to access.

- **Returns:**
  - `int`: The current unsigned value.

**Example Usage:**
```lua
value = scr_value_wrapper:get_uns(0)
```

---

### `set_uns()`

Set the unsigned int value contained by the wrapper.

- **Parameters:**
  - `index` (int): The index to access.
  - `value` (int): The new unsigned value.

**Example Usage:**
```lua
scr_value_wrapper:set_uns(0, 42)
```

---

### `get_float()`

Get the float value currently contained by the wrapper.

- **Parameters:**
  - `index` (int): The index to access.

- **Returns:**
  - `float`: The current value.

**Example Usage:**
```lua
value = scr_value_wrapper:get_float(0)
```

---

### `set_float()`

Set the float value contained by the wrapper.

- **Parameters:**
  - `index` (int): The index to access.
  - `value` (float): The new value.

**Example Usage:**
```lua
scr_value_wrapper:set_float(0, 3.14)
```

---

### `get_string()`

Get the string value currently contained by the wrapper.

- **Parameters:**
  - `index` (int): The index to access.

- **Returns:**
  - `string`: The current value.

**Example Usage:**
```lua
value = scr_value_wrapper:get_string(0)
```

---

### `set_string()`

Set the string value contained by the wrapper.

- **Parameters:**
  - `index` (int): The index to access.
  - `value` (string): The new value.

**Example Usage:**
```lua
scr_value_wrapper:set_string(0, "hello")
```

---

### `get_reference()`

Get the reference value currently contained by the wrapper.

- **Parameters:**
  - `index` (int): The index to access.

- **Returns:**
  - `class`: The current value.

**Example Usage:**
```lua
value = scr_value_wrapper:get_reference(0)
```