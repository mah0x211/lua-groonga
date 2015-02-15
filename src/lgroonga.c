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
 *  lgroonga.c
 *  lua-groonga
 *
 *  Created by Masatoshi Teruya on 2015/02/13.
 *
 */

#include "lgroonga.h"

#define MODULE_MT   GROONGA_MT

#define LGRN_ENODB  "database has been removed"

// MARK: table API
static int table_lua( lua_State *L )
{
    lgrn_t *g = luaL_checkudata( L, 1, MODULE_MT );
    
    if( !lgrn_get_db( g ) ){
        lua_pushnil( L );
        lua_pushstring( L, LGRN_ENODB );
        return 2;
    }
    else
    {
        int rv = 1;
        grn_ctx *ctx = lgrn_get_ctx( g );
        size_t len = 0;
        const char *name = luaL_checklstring( L, 2, &len );
        grn_obj *obj = NULL;
        lgrn_tbl_t *t = NULL;
        
        if( len < GRN_TABLE_MAX_KEY_SIZE && 
            ( obj = grn_ctx_get( ctx, name, len ) ) )
        {
            if( lgrn_obj_istbl( obj ) )
            {
                // create table metatable
                if( ( t = lua_newuserdata( L, sizeof( lgrn_tbl_t ) ) ) ){
                    lstate_setmetatable( L, GROONGA_TABLE_MT );
                    // retain references
                    t->ref_g = lstate_refat( L, 1 );
                    t->g = lgrn_retain( g );
                    t->tbl = obj;
                    return 1;
                }
                
                // nomem error
                lua_pushnil( L );
                lua_pushstring( L, strerror( errno ) );
                rv = 2;
            }
            grn_obj_unlink( ctx, obj );
        }
        
        // not found
        if( rv == 1 ){
            lua_pushnil( L );
        }
        
        return rv;
    }
}


static int tables_next_lua( lua_State *L )
{
    lgrn_t *g = luaL_checkudata( L, lua_upvalueindex( 1 ), MODULE_MT );
    lgrn_iter_t *it = (lgrn_iter_t*)lua_touserdata( L, lua_upvalueindex( 2 ) );
    int ref = lua_tointeger( L, lua_upvalueindex( 3 ) );
    
    if( lgrn_get_db( g ) )
    {
        lgrn_tbl_t *t = NULL;
        grn_obj *tbl = NULL;
        lgrn_tblname_t tname;
        
        while( lgrn_tbl_iter_next( it, &tbl ) == GRN_SUCCESS )
        {
            // create table metatable
            t = lua_newuserdata( L, sizeof( lgrn_tbl_t ) );
            if( t )
            {
                lstate_setmetatable( L, GROONGA_TABLE_MT );
                // retain references
                lstate_pushref( L, ref );
                t->ref_g = lstate_ref( L );
                t->g = lgrn_retain( g );
                t->tbl = tbl;
                
                // get table name
                if( lgrn_get_tblname( &tname, it->ctx, tbl ) ){
                    lua_pushlstring( L, tname.name, (size_t)tname.len );
                }
                // temporary table has no name
                else {
                    lua_pushnil( L );
                }
            }
            // nomem error
            else {
                lstate_unref( L, ref );
                lua_pushnil( L );
                lua_pushstring( L, strerror( errno ) );
            }
            return 2;
        }
    }
    
    lgrn_iter_dispose( it );
    lstate_unref( L, ref );
    
    return 0;
}


static int tables_lua( lua_State *L )
{
    lgrn_t *g = luaL_checkudata( L, 1, MODULE_MT );
    lgrn_iter_t *it = lua_newuserdata( L, sizeof( lgrn_iter_t ) );
    
    if( !it || lgrn_tbl_iter_init( it, lgrn_get_ctx( g ) ) != GRN_SUCCESS ){
        lua_pushnil( L );
    }
    else {
        lua_pushinteger( L, lstate_refat( L, 1 ) );
        lua_pushcclosure( L, tables_next_lua, 3 );
    }
    
    return 1;
}


// MARK: database API

static int path_lua( lua_State *L )
{
    lgrn_t *g = luaL_checkudata( L, 1, MODULE_MT );
    grn_obj *db = lgrn_get_db( g );
    
    if( !db ){
        lua_pushnil( L );
        lua_pushstring( L, LGRN_ENODB );
        return 2;
    }
    
    lua_pushstring( L, grn_obj_path( lgrn_get_ctx( g ), db ) );
    
    return 1;
}


static int touch_lua( lua_State *L )
{
    lgrn_t *g = luaL_checkudata( L, 1, MODULE_MT );
    grn_obj *db = lgrn_get_db( g );
    
    if( !db ){
        lua_pushboolean( L, 0 );
        lua_pushstring( L, LGRN_ENODB );
        return 2;
    }
    
    grn_db_touch( lgrn_get_ctx( g ), db );
    lua_pushboolean( L, 1 );
    
    return 1;
}


static int remove_lua( lua_State *L )
{
    lgrn_t *g = luaL_checkudata( L, 1, MODULE_MT );
    grn_obj *db = lgrn_get_db( g );
    
    g->removed = 1;
    // can be remove immediately if reference counter is 0
    if( db && !g->retain ){
        grn_obj_remove( lgrn_get_ctx( g ), db );
    }
    
    return 0;
}


static int gc_lua( lua_State *L )
{
    lgrn_t *g = (lgrn_t*)lua_touserdata( L, 1 );
    grn_obj *db = grn_ctx_db( &g->ctx );
    
    if( db )
    {
        // remove db
        if( g->removed ){
            grn_obj_remove( &g->ctx, db );
        }
        // close db
        else {
            grn_obj_unlink( &g->ctx, db );
        }
    }
    
    // free
    grn_ctx_fin( &g->ctx );
    
    return 0;
}


static int tostring_lua( lua_State *L )
{
    return lgrn_tostring( L, MODULE_MT );
}


static int new_lua( lua_State *L )
{
    lgrn_t *g = NULL;
    const char *path = luaL_optstring( L, 1, NULL );
    
    // alloc context
    if( ( g = lua_newuserdata( L, sizeof( lgrn_t ) ) ) )
    {
        grn_ctx_init( &g->ctx, 0 );
        // open
        if( grn_db_open( &g->ctx, path ) ||
            // create database if path does not exists
            ( lua_type( L, 2 ) == LUA_TBOOLEAN && lua_toboolean( L, 2 ) && 
              grn_db_create( &g->ctx, path, NULL ) ) ){
            lstate_setmetatable( L, MODULE_MT );
            g->removed = 0;
            g->retain = 0;
            return 1;
        }
        // got error
        lua_pushnil( L );
        lua_pushstring( L, g->ctx.errbuf );
        grn_ctx_fin( &g->ctx );
    }
    // nomem error
    else {
        lua_pushnil( L );
        lua_pushstring( L, strerror( errno ) );
    }
    
    return 2;
}


static int encoding_lua( lua_State *L )
{
    size_t len = 0;
    const char *enc = luaL_optlstring( L, 1, NULL, &len );
    
    // set default encoding
    if( len ){
        grn_set_default_encoding( grn_encoding_parse( enc ) );
    }
    
    lua_pushstring( L, grn_encoding_to_string( grn_get_default_encoding() ) );
    
    return 1;
}


static int version_lua( lua_State *L )
{
    lua_pushstring( L, grn_get_version() );
    return 1;
}


LUALIB_API int luaopen_groonga( lua_State *L )
{
    struct luaL_Reg mmethods[] = {
        { "__gc", gc_lua },
        { "__tostring", tostring_lua },
        { NULL, NULL }
    };
    struct luaL_Reg methods[] = {
        { "remove", remove_lua },
        { "touch", touch_lua },
        { "path", path_lua },
        { "table", table_lua },
        { "tables", tables_lua },
        { NULL, NULL }
    };
    struct luaL_Reg funcs[] = {
        { "version", version_lua },
        { "encoding", encoding_lua },
        { "new", new_lua },
        { NULL, NULL }
    };
    
    // failed to initialize groonga global variables
    // ???: should i construct error message?
    if( grn_init() != GRN_SUCCESS ){
        return lua_error( L );
    }
    
    // create metatable
    lgrn_register_mt( L, MODULE_MT, mmethods, methods );
    // create module table
    lgrn_register_fn( L, funcs );
    // register related module
    luaopen_groonga_table( L );
    
    // constants
    lstate_int2tbl( L, "PERSISTENT", GRN_OBJ_PERSISTENT );
    // table flags
    lstate_int2tbl( L, "TABLE_NO_KEY", GRN_OBJ_TABLE_NO_KEY );
    lstate_int2tbl( L, "TABLE_HASH_KEY", GRN_OBJ_TABLE_HASH_KEY );
    lstate_int2tbl( L, "TABLE_PAT_KEY", GRN_OBJ_TABLE_PAT_KEY );
    lstate_int2tbl( L, "TABLE_DAT_KEY", GRN_OBJ_TABLE_DAT_KEY );
    // key flags
    lstate_int2tbl( L, "KEY_NORMALIZE", GRN_OBJ_KEY_NORMALIZE );
    lstate_int2tbl( L, "KEY_WITH_SIS", GRN_OBJ_KEY_WITH_SIS );
    // column flags
    lstate_int2tbl( L, "COLUMN_SCALAR", GRN_OBJ_COLUMN_SCALAR );
    lstate_int2tbl( L, "COLUMN_VECTOR", GRN_OBJ_COLUMN_VECTOR );
    lstate_int2tbl( L, "COLUMN_INDEX", GRN_OBJ_COLUMN_INDEX );
    // compress flags
    lstate_int2tbl( L, "COMPRESS_NONE", GRN_OBJ_COMPRESS_NONE );
    lstate_int2tbl( L, "COMPRESS_ZLIB", GRN_OBJ_COMPRESS_ZLIB );
    lstate_int2tbl( L, "COMPRESS_LZ4", GRN_OBJ_COMPRESS_LZ4 );
    // with flags
    lstate_int2tbl( L, "WITH_SECTION", GRN_OBJ_WITH_SECTION );
    lstate_int2tbl( L, "WITH_WEIGHT", GRN_OBJ_WITH_WEIGHT );
    lstate_int2tbl( L, "WITH_POSITION", GRN_OBJ_WITH_POSITION );
    // data type
    lstate_int2tbl( L, "T_VOID", GRN_DB_VOID );
    lstate_int2tbl( L, "T_DB", GRN_DB_DB );
    lstate_int2tbl( L, "T_OBJECT", GRN_DB_OBJECT );
    lstate_int2tbl( L, "T_BOOL", GRN_DB_BOOL );
    lstate_int2tbl( L, "T_INT8", GRN_DB_INT8 );
    lstate_int2tbl( L, "T_UINT8", GRN_DB_UINT8 );
    lstate_int2tbl( L, "T_INT16", GRN_DB_INT16 );
    lstate_int2tbl( L, "T_UINT16", GRN_DB_UINT16 );
    lstate_int2tbl( L, "T_INT32", GRN_DB_INT32 );
    lstate_int2tbl( L, "T_UINT32", GRN_DB_UINT32 );
    lstate_int2tbl( L, "T_INT64", GRN_DB_INT64 );
    lstate_int2tbl( L, "T_UINT64", GRN_DB_UINT64 );
    lstate_int2tbl( L, "T_FLOAT", GRN_DB_FLOAT );
    lstate_int2tbl( L, "T_TIME", GRN_DB_TIME );
    lstate_int2tbl( L, "T_SHORT_TEXT", GRN_DB_SHORT_TEXT );
    lstate_int2tbl( L, "T_TEXT", GRN_DB_TEXT );
    lstate_int2tbl( L, "T_LONG_TEXT", GRN_DB_LONG_TEXT );
    lstate_int2tbl( L, "T_TOKYO_GEO", GRN_DB_TOKYO_GEO_POINT );
    lstate_int2tbl( L, "T_WGS84_GEO", GRN_DB_WGS84_GEO_POINT );

    return 1;
}

