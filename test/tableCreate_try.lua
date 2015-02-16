local INSPECT_OPT = { depth = 0 };
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
        keyType = groonga.UINT32,
        valType = groonga.INT32,
        flags = {
            groonga.TABLE_HASH_KEY
        }
    },
    {
        name = 'test',
        keyType = groonga.SHORT_TEXT,
        flags = {
            groonga.TABLE_PAT_KEY
        }
    },
    {
        name = 'test',
        flags = {
            groonga.TABLE_DAT_KEY
        }
    },
    {
        name = 'test',
        flags = {
            groonga.TABLE_NO_KEY
        }
    }
}) do
    t = ifNil( g:tableCreate( spec ) );
    -- verify spec
    for k, v in pairs( spec ) do
        if k == 'flags' and spec.name then
            v[#v+1] = groonga.PERSISTENT;
        end
        ifNotEqual( t[k]( t ), v );
    end
    -- force remove
    t:remove( true );
end

g:remove();
