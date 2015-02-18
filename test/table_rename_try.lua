local groonga = require('groonga');
local path = './db/testdb';
local g = groonga.new( path );
local name = 'testColumn';
local t;

if g then
    g:remove();
end

g = ifNil( groonga.new( path, true ) );
t = ifNil( g:tableCreate({ name = 'test' }) );
-- rename
ifNotTrue( t:rename( name .. 'change' ) );
ifNotEqual( t:name(), name .. 'change' );

g:remove();
