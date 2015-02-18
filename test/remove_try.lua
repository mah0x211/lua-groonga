local groonga = require('groonga');
local path = './db/testdb';
local g = groonga.new( path );
local tbl = {
    name = 'testTable'
};
local col = {
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
};
local t,c;

if g then
    g:remove();
end

-- g -> t -> c
g = ifNil( groonga.new( path, true ) );
t = ifNil( g:tableCreate( tbl ) );
c = ifNil( t:columnCreate( col ) );
-- remove db
ifNotTrue( g:remove() );
ifNotNil( g:path() );
-- remove table
ifTrue( t:remove() );
ifNotNil( t:name() );
-- remove column
ifTrue( c:remove() );
ifNotNil( c:name() );

-- g -> c -> t
g = ifNil( groonga.new( path, true ) );
t = ifNil( g:tableCreate( tbl ) );
c = ifNil( t:columnCreate( col ) );
-- remove db
ifNotTrue( g:remove() );
ifNotNil( g:path() );
-- remove column
ifTrue( c:remove() );
ifNotNil( c:name() );
-- remove table
ifTrue( t:remove() );
ifNotNil( t:name() );


-- t -> g -> c
g = ifNil( groonga.new( path, true ) );
t = ifNil( g:tableCreate( tbl ) );
c = ifNil( t:columnCreate( col ) );
-- remove table
ifNotTrue( t:remove() );
ifNotNil( t:name() );
-- remove db
ifNotTrue( g:remove() );
ifNotNil( g:path() );
-- remove column
ifTrue( c:remove() );
ifNotNil( c:name() );

-- t -> c -> g
g = ifNil( groonga.new( path, true ) );
t = ifNil( g:tableCreate( tbl ) );
c = ifNil( t:columnCreate( col ) );
-- remove table
ifNotTrue( t:remove() );
ifNotNil( t:name() );
-- remove column
ifTrue( c:remove() );
ifNotNil( c:name() );
-- remove db
ifNotTrue( g:remove() );
ifNotNil( g:path() );


-- c -> t -> g
g = ifNil( groonga.new( path, true ) );
t = ifNil( g:tableCreate( tbl ) );
c = ifNil( t:columnCreate( col ) );
-- remove column
ifNotTrue( c:remove() );
ifNotNil( c:name() );
-- remove table
ifNotTrue( t:remove() );
ifNotNil( t:name() );
-- remove db
ifNotTrue( g:remove() );
ifNotNil( g:path() );

-- c -> g -> t
g = ifNil( groonga.new( path, true ) );
t = ifNil( g:tableCreate( tbl ) );
c = ifNil( t:columnCreate( col ) );
-- remove column
ifNotTrue( c:remove() );
ifNotNil( c:name() );
-- remove db
ifNotTrue( g:remove() );
ifNotNil( g:path() );
-- remove table
ifTrue( t:remove() );
ifNotNil( t:name() );

g:remove();
