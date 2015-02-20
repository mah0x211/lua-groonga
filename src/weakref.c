/*
 *  Copyright 2015 Masatoshi Teruya. All rights reserved.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a 
 *  copy of this software and associated documentation files (the "Software"), 
 *  to deal in the Software without restriction, including without limitation 
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 *  and/or sell copies of the Software, and to permit persons to whom the 
 *  Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL 
 *  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 *  DEALINGS IN THE SOFTWARE.
 *
 *  weakref.c
 *  lua-groonga
 *
 *  Created by Masatoshi Teruya on 2015/02/18.
 *
 */

#include "lgroonga.h"


#define MODULE_MT   "groonga.weak_reference"

static int REF_WEAK_DB;
static int REF_WEAK_TABLE;
static int REF_WEAK_COLUMN;


static int getref( lua_State *L, int ref, const char *name, size_t len )
{
    // push dummy value
    lua_pushnil( L );
    
    // get field value
    lstate_pushref( L, ref );
    lua_pushlstring( L, name, len );
    lua_rawget( L, -2 );
    
    // found
    if( lua_type( L, -1 ) == LUA_TUSERDATA ){
        lua_replace( L, -3 );
        lua_pop( L, 1 );
        return 1;
    }
    
    // not found
    lua_pop( L, 2 );
    
    return 0;
}


static void setref( lua_State *L, int ref, const char *name, size_t len,
                    int idx )
{
    // convert to unsigned index
    if( idx < 0 ){
        idx = lua_gettop( L ) + idx + 1;
    }
    
    lstate_pushref( L, ref );
    lua_pushlstring( L, name, len );
    lua_pushvalue( L, idx );
    lua_rawset( L, -3 );
    lua_pop( L, 1 );
}


// db reference
int lgrn_refget_db( lua_State *L, const char *name, size_t len )
{
    return getref( L, REF_WEAK_DB, name, len );
}

void lgrn_refset_db( lua_State *L, const char *name, size_t len, int idx )
{
    setref( L, REF_WEAK_DB, name, len, idx );
}

// table reference
int lgrn_refget_tbl( lua_State *L, const char *name, size_t len )
{
    return getref( L, REF_WEAK_TABLE, name, len );
}

void lgrn_refset_tbl( lua_State *L, const char *name, size_t len, int idx )
{
    setref( L, REF_WEAK_TABLE, name, len, idx );
}


// column reference
int lgrn_refget_col( lua_State *L, const char *name, size_t len )
{
    return getref( L, REF_WEAK_COLUMN, name, len );
}

void lgrn_refset_col( lua_State *L, const char *name, size_t len, int idx )
{
    setref( L, REF_WEAK_COLUMN, name, len, idx );
}


void lgrn_weakref_init( lua_State *L )
{
    luaL_newmetatable( L, MODULE_MT );
    // metamethods
    lstate_str2tbl( L, "__mode", "v" );
    lua_pop( L, 1 );
    
    // create weak reference table for DB
    lua_newtable( L );
    lstate_setmetatable( L, MODULE_MT );
    REF_WEAK_DB = lstate_ref( L );
    
    // create weak reference table for tables
    lua_newtable( L );
    lstate_setmetatable( L, MODULE_MT );
    REF_WEAK_TABLE = lstate_ref( L );
    
    // create weak reference table for columns
    lua_newtable( L );
    lstate_setmetatable( L, MODULE_MT );
    REF_WEAK_COLUMN = lstate_ref( L );
}


