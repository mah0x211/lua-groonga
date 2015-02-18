local groonga = require('groonga');
local unpack = table.unpack or unpack;
local path = './db/testdb';
local g = groonga.new( path );
local tbls = {};
local nelts = 0;
local t,c;

if g then
    g:remove();
end
g = ifNil( groonga.new( path, true ) );
t = ifNil( g:tableCreate({ name = 'test' }) );

for _, spec in ipairs({
    {
        name = 'testColumn',
        path = "./db/col",
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
    },
}) do
    c = ifNil( t:columnCreate( spec ) );
    -- verify db
    ifNotEqual( g, c:db() );
    -- verify table
    ifNotEqual( t, c:table() );
    -- verify spec
    for k, v in pairs( spec ) do
        ifNotEqual( c[k]( c ), v );
    end
end


g:remove();
