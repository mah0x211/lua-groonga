local groonga = require('groonga');
local path = './db/testdb';
local g = groonga.new( path );
local tbls = {};
local nelts = 0;
local name;

-- cleanup
if g then
    g:remove();
end
g = ifNil( groonga.new( path, true ) );

-- create table
for i = 1, 10 do
    name = 'test'.. i;
    ifNil( g:tableCreate({ name = name }) );
    tbls[name] = true;
    nelts = nelts + 1;
end

-- getting all table
for tbl, name in g:tables() do
    if tbls[name] then
        tbls[name] = nil;
        nelts = nelts - 1;
    end
end
ifNotEqual( nelts, 0 );

g:remove();
