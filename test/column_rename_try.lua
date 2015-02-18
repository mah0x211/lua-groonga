local groonga = require('groonga');
local path = './db/testdb';
local g = groonga.new( path );
local name = 'testColumn';
local t,c;

if g then
    g:remove();
end

g = ifNil( groonga.new( path, true ) );
t = ifNil( g:tableCreate({ name = 'test' }) );
c = ifNil( t:columnCreate({
    name = name,
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
}) );
-- rename
ifNotTrue( c:rename( name .. 'change' ) );
ifNotEqual( c:name(), name .. 'change' );

g:remove();
