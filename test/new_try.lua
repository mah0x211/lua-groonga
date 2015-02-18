local groonga = require('groonga');
local path = './db/testdb';
local g = groonga.new( path );

-- remove testdb
if g then
    g:remove();
end

-- not exists
ifNotNil( groonga.new( path ) );
-- create db
ifNil( groonga.new( path, true ) );
-- open db
ifNil( groonga.new( path ) );
-- create temporary db
ifNil( groonga.new( nil, true ) );

-- remove db
g = groonga.new( path );
if g then
    g:remove();
end

