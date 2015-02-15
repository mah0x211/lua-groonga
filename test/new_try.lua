local groonga = require('groonga');
local path = './db/testdb';

-- remove testdb
groonga.removeDb( path );

-- not exists
ifNotNil( groonga.new( path ) );
-- create db
ifNil( groonga.new( path, true ) );
-- open db
ifNil( groonga.new( path ) );
-- create temporary db
ifNil( groonga.new( nil, true ) );

-- remove db
ifNotTrue( groonga.removeDb( path ) );

