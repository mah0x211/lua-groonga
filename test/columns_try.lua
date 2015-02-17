local groonga = require('groonga');
local path = './db/testdb';
local g = groonga.new( path );
local cols = {};
local nelts = 0;
local t, name;

if g then
    g:remove();
end
g = ifNil( groonga.new( path, true ) );
t = ifNil( g:tableCreate({ name = 'test' }) );

for i = 1, 10 do
    name = 'test' .. i;
    ifNil( t:columnCreate({
        name = name,
        -- 'SCALAR' | 'VECTOR' | 'INDEX'
        type ='SCALAR',
        valType = 'UINT32',
        -- 'PERSISTENT'
        persistent = true,
        -- 'ZLIB' | 'LZ4'
        compress = 'ZLIB',
        -- 'WITH_WEIGHT'
        withWeight = false,
        -- 'WITH_SECTION'
        withSection = false,
        -- 'WITH_POSITION'
        withPosition = false
    }) );
    cols[name] = true;
    nelts = nelts + 1;
end


-- getting all column name
for name, col in t:columns() do
    ifNil( cols[name] );
    ifNotNil( col );
end

-- getting all column name with column obj
for name, col in t:columns(true) do
    ifNil( cols[name] );
    ifNil( col );
    if cols[name] then
        cols[name] = nil;
        nelts = nelts - 1;
    end
end
ifNotEqual( nelts, 0 );

g:remove();
