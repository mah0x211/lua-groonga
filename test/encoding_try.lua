local groonga = require('groonga');
-- get default encoding
local default = ifNil( groonga.encoding() );

-- set default encoding
for _, enc in ipairs({
  'default',
  'none',
  'euc_jp',
  'utf8',
  'sjis',
  'latin1',
  'koi8r'
}) do
    ifNotEqual( 
        groonga.encoding( enc ), 
        enc == 'default' and default or enc
    );
end