local groonga = require('groonga');
local path = './db/testdb';
local g = groonga.new( path );
local ref;

-- remove testdb
if g then
    g:remove();
end

-- create temporary db
ifNil( groonga.new( nil, true ) );

-- not exists
ifNotNil( groonga.new( path ) );
-- create db
g = ifNil( groonga.new( path, true ) );
-- open db
ref = ifNil( groonga.new( path ) );
ifNotEqual( g, ref );

-- remove db
g:remove();
ifTrue( ref:remove() );
ifNotNil( groonga.new( path ) );

