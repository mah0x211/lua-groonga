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
 *  schema_tbl.c
 *  lua-groonga
 *
 *  Created by Masatoshi Teruya on 2015/02/15.
 *
 */

#include "lgroonga.h"

#define MODULE_MT   GROONGA_TABLE_MT


static int name_lua( lua_State *L )
{
    lgrn_tbl_t *t = luaL_checkudata( L, 1, MODULE_MT );
    lgrn_tblname_t tname;
    
    if( lgrn_get_tblname( &tname, t->ctx, t->tbl ) ){
        lua_pushlstring( L, tname.name, (size_t)tname.len );
    }
    // temporary table
    else {
        lua_pushnil( L );
    }
    
    return 1;
}


static int path_lua( lua_State *L )
{
    lgrn_tbl_t *t = luaL_checkudata( L, 1, MODULE_MT );
    const char *path = grn_obj_path( t->ctx, t->tbl );
    
    if( path ){
        lua_pushstring( L, path );
    }
    else {
        lua_pushnil( L );
    }
    
    return 1;
}


static int type_lua( lua_State *L )
{
    lgrn_tbl_t *t = luaL_checkudata( L, 1, MODULE_MT );
    
    switch( t->tbl->header.flags & GRN_OBJ_TABLE_TYPE_MASK ){
        case GRN_OBJ_TABLE_HASH_KEY:
            lua_pushstring( L, "TABLE_HASH_KEY" );
        break;
        case GRN_OBJ_TABLE_PAT_KEY:
            lua_pushstring( L, "TABLE_PAT_KEY" );
        break;
        case GRN_OBJ_TABLE_DAT_KEY:
            lua_pushstring( L, "TABLE_DAT_KEY" );
        break;
        case GRN_OBJ_TABLE_NO_KEY:
            lua_pushstring( L, "TABLE_NO_KEY" );
        break;
    }
    
    return 1;
}


static int domain_lua( lua_State *L )
{
    lgrn_tbl_t *t = luaL_checkudata( L, 1, MODULE_MT );
    grn_obj *obj = grn_ctx_at( t->ctx, t->tbl->header.domain );
    lgrn_tblname_t tname;
    
    if( obj && lgrn_get_tblname( &tname, t->ctx, obj ) ){
        lua_pushlstring( L, tname.name, (size_t)tname.len );
    }
    else {
        lua_pushnil( L );
    }

    return 1;
}


static int persistent_lua( lua_State *L )
{
    lgrn_tbl_t *t = luaL_checkudata( L, 1, MODULE_MT );
    
    lua_pushboolean( L, t->tbl->header.flags & GRN_OBJ_PERSISTENT );

    return 1;
}


static int tostring_lua( lua_State *L )
{
    return lgrn_tostring( L, MODULE_MT );
}


static int gc_lua( lua_State *L )
{
    lgrn_tbl_t *s = lua_touserdata( L, 1 );
    
    if( s->tbl ){
        grn_obj_unlink( s->ctx, s->tbl );
    }
    lstate_unref( L, s->ref_g );
    
    return 0;
}


LUALIB_API int luaopen_groonga_table( lua_State *L )
{
    struct luaL_Reg mmethods[] = {
        { "__gc", gc_lua },
        { "__tostring", tostring_lua },
        { NULL, NULL }
    };
    struct luaL_Reg methods[] = {
        { "name", name_lua },
        { "path", path_lua },
        { "type", type_lua },
        { "domain", domain_lua },
        { "persistent", persistent_lua },
        { NULL, NULL }
    };
    
    lgrn_register_mt( L, MODULE_MT, mmethods, methods );
    
    return 0;
}

