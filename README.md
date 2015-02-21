# lua-groonga

groonga bindings for lua.

**NOTE:**  
this module is currently under heavy development.

---

## Dependencies

- groonga: https://github.com/groonga/groonga


## Module functions

### ver = groonga.version()

returns a groonga version string.

```lua
local groonga = require('groonga');

print( groonga.version() );
```

**Returns**

1. `ver:string`: version string.

### enc = groonga.encoding( [enc:string] )

change default encoding to specified encoding and returns a default encoding name.

```lua
local groonga = require('groonga');

print( groonga.encoding() );
```

**Parameters**

- `enc:string`: encoding string.

---

please refer to the groonga documentation for more details.  
http://groonga.org/ja/docs/reference/api/grn_encoding.html

---

**Returns**

1. `enc:string`: default encoding string.


### db, err = groonga.new( path:string [, create:boolean] )

open (or create) a database and returns a database object.

```lua
local groonga = require('groonga');
local db, err = groonga.new('./mydb', true );
```

**Parameters**

- `path:string`: path string of database.
- `create:boolean`: create new database if not found.

**Returns**

1. `db:userdata`: database object, or a `nil` on failure.
2. `err:string`: error string. 


## Database object

### ok, err = db:path()

returns an absolute path of database file.

```lua
local path, err = db:path();
```

**Returns**

1. `path:string`: an absolute path of database file, or a `nil` on failure.
2. `err:string`: error string. 


### ok, err = db:remove()

```lua
local ok, err = db:remove();
```

dispose database object and remove database object associated files.

**Returns**

1. `ok:boolean`: true on success, or false on failure.
2. `err:string`: error string. 


### ok, err = db:touch()

update a database modification time to current time.

```lua
local ok, err = db:touch();
```

**Returns**

1. `ok:boolean`: true on success, or false on failure.
2. `err:string`: error string. 


### ok, err = db:table( name:string )

returns a table object.

```lua
local tbl, err = db:table('mytable');
```

**Parameters**

- `name:string`: table name.

**Returns**

1. `tbl:userdata`: table object, or a `nil` on failure.
2. `err:string`: error string. 


### iter, err = db:tables( [withObj:boolean] )

returns a iterator function for table lookup.

```lua
local withObj = true;
local iter, err = db:tables( withObj );

for name, tbl in iter do
    print( name, tbl );
end

-- use with for-in loop directly
for name in db:tables() do
    print( name );
end
```

**Parameters**

- `withObj:boolean`: iterator function returns table name and table object if specified to `true`.

**Returns**

1. `iter:function`: iterator function, or a `nil` on failure.
2. `err:string`: error string.


### tbl, err = db:tableCreate( [attr:table] )

create new table to database and returns an table object. if argument is a `nil`, that table is a temporary table.

```lua
-- creating temporary table
local tmp, err = db:tableCreate();

-- creating persistent table
local tbl, err = db:tableCreate({
    name = 'mytbl',
    path = '/path/to/table/file',
    -- 'PAT_KEY' | 'DAT_KEY' | 'HASH_KEY' | 'NO_KEY'
    type = 'HASH_KEY'
    keyType = '',
    valType = '',
    persistent = true,
    withSIS = true,
    normalize = true
});
```

**Parameters**

- `attr:table`: table attributes.
  - `name:string`: table name
  - `path:string`: path of table file, or a `nil`.
  - `type:string`: `table type`.
  - `keyType:string`: `data type` for key.
  - `valType:string`: `data type` for value.
  - `persistent:boolean`: set `true` to create a persistent table.
  - `withSIS:boolean`: set `true` to ...
  - `normalize:boolean`: set `true` to ...

---

please refer to the groonga documentation for more details.  
http://groonga.org/ja/docs/reference/api/grn_table.html

---

**Returns**

1. `tbl:userdata`: table object, or a `nil` on failure.
2. `err:string`: error string.

## Type Constants

### Table Types

- `'PAT_KEY'`: patricia trie.
- `'DAT_KEY'`: double array trie.
- `'HASH_KEY'`: hash map.
- `'NO_KEY'`: plain array.


### Data Types

- `'OBJECT'`
- `'BOOL'`
- `'INT8'`
- `'UINT8'`
- `'INT16'`
- `'UINT16'`
- `'INT32'`
- `'UINT32'`
- `'INT64'`
- `'UINT64'`
- `'FLOAT'`
- `'TIME'`
- `'SHORT_TEXT'`
- `'TEXT'`
- `'LONG_TEXT'`
- `'TOKYO_GEO_POINT'`
- `'WGS84_GEO_POINT'`


## Table object

N/A

## Column object

N/A

