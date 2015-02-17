local groonga = require('groonga');
local unpack = table.unpack or unpack;
local path = './db/testdb';
local g = groonga.new( path );
local tbls = {};
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
    tbls[name] = true;
    nelts = nelts + 1;
end

-- getting all column
for name, col in t:columns() do
    if tbls[name] then
        tbls[name] = nil;
        nelts = nelts - 1;
    end
end
ifNotEqual( nelts, 0 );

g:remove();
