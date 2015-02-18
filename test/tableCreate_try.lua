local groonga = require('groonga');
local unpack = table.unpack or unpack;
local path = './db/testdb';
local g = ifNil( groonga.new( path, true ) );
local tbls = {};
local nelts = 0;
local t;

g:remove();
g = groonga.new( path, true );

for _, spec in ipairs({
    -- temporary table
    {},
    -- persistent table
    {
        name = 'test',
        path = './db/tbl',
        keyType = 'UINT32',
        valType = 'INT32',
        -- 'PAT_KEY' | 'DAT_KEY' | 'HASH_KEY' | 'NO_KEY'
        type = 'HASH_KEY',
        -- 'PERSISTENT'
        persistent = true,
        -- 'KEY_WITH_SIS'
        withSIS = false,
        -- 'KEY_NORMALIZE'
        normalize = false,
    },
    {
        name = 'test',
        keyType = 'SHORT_TEXT',
        type = 'PAT_KEY'
    },
    {
        name = 'test',
        type = 'DAT_KEY'
    },
    {
        name = 'test',
        type = 'NO_KEY'
    }
}) do
    t = ifNil( g:tableCreate( spec ) );
    -- verify db
    ifNotEqual( g, t:db() );
    -- verify spec
    for k, v in pairs( spec ) do
        ifNotEqual( t[k]( t ), v );
    end
    -- force remove
    t:remove( true );
end

g:remove();
