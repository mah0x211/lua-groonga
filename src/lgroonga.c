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

// MARK: table API
static int table_lua( lua_State *L )
{
    lgrn_t *g = luaL_checkudata( L, 1, MODULE_MT );
    grn_ctx *ctx = &g->ctx;
    size_t len = 0;
    const char *name = luaL_checklstring( L, 2, &len );
    grn_obj *obj = NULL;
    lgrn_tbl_t *t = NULL;
    int rv = 1;
    
    if( len < GRN_TABLE_MAX_KEY_SIZE && 
        ( obj = grn_ctx_get( &g->ctx, name, len ) ) )
    {
        if( lgroonga_obj_istbl( obj ) )
        {
            // create table metatable
            if( ( t = lua_newuserdata( L, sizeof( lgrn_tbl_t ) ) ) ){
                lstate_setmetatable( L, GROONGA_TABLE_MT );
                // retain references
                t->tbl = obj;
                t->ctx = ctx;
                t->ref_g = lstate_refat( L, 1 );
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


static int tables_lua( lua_State *L )
{
    lgrn_t *g = luaL_checkudata( L, 1, MODULE_MT );
    grn_ctx *ctx = &g->ctx;
    grn_table_cursor *cur = grn_table_cursor_open( ctx, grn_ctx_db( ctx ), NULL,
                                                   0, NULL, 0, 0, -1, 0 );
    
    if( cur )
    {
        lgrn_tbl_t *t = NULL;
        grn_obj *obj = NULL;
        lgrn_tblname_t tname;
        int noname = 0;
        grn_id id;
        
        lua_newtable( L );
        while( ( id = grn_table_cursor_next( ctx, cur ) ) != GRN_ID_NIL )
        {
            if( ( obj = grn_ctx_at( ctx, id ) ) )
            {
                // check object type
                if( lgroonga_obj_istbl( obj ) )
                {
                    // get table name
                    if( lgroonga_get_tblname( &tname, ctx, obj ) ){
                        lua_pushlstring( L, tname.name, (size_t)tname.len );
                    }
                    // temporary table has no name
                    else {
                        lua_pushinteger( L, ++noname );
                    }
                    
                    // create table metatable
                    t = lua_newuserdata( L, sizeof( lgrn_tbl_t ) );
                    // nomem error
                    if( !t ){
                        lua_pushnil( L );
                        lua_pushstring( L, strerror( errno ) );
                        return 2;
                    }
                    lstate_setmetatable( L, GROONGA_TABLE_MT );
                    lua_rawset( L, -3 );
                    // retain references
                    t->tbl = obj;
                    t->ctx = ctx;
                    t->ref_g = lstate_refat( L, 1 );
                }
                else {
                    grn_obj_unlink( ctx, obj );
                }
            }
        }
        grn_table_cursor_close(ctx, cur);
    }
    else {
        lua_pushnil( L );
    }
    
    return 1;
}


// MARK: database API

// close db
#define close_groonga( g ) do { \
    grn_obj *db = grn_ctx_db( &(g)->ctx ); \
    if( db ){ \
        grn_obj_unlink( &(g)->ctx, db ); \
    } \
}while(0)


static int path_lua( lua_State *L )
{
    lgrn_t *g = luaL_checkudata( L, 1, MODULE_MT );
    grn_obj *db = grn_ctx_db( &g->ctx );
    
    if( db ){
        lua_pushstring( L, grn_obj_path( &g->ctx, db ) );
    }
    else {
        lua_pushnil( L );
    }
    
    return 1;
}


static int touch_lua( lua_State *L )
{
    lgrn_t *g = luaL_checkudata( L, 1, MODULE_MT );
    grn_obj *db = grn_ctx_db( &g->ctx );
    
    if( db ){
        grn_db_touch( &g->ctx, db );
        lua_pushboolean( L, 1 );
    }
    else {
        lua_pushboolean( L, 0 );
    }
    
    return 1;
}


static int close_lua( lua_State *L )
{
    lgrn_t *g = luaL_checkudata( L, 1, MODULE_MT );
    
    close_groonga( g );
    
    return 0;
}


static int gc_lua( lua_State *L )
{
    lgrn_t *g = (lgrn_t*)lua_touserdata( L, 1 );
    
    // close db
    close_groonga( g );
    // free
    grn_ctx_fin( &g->ctx );
    
    return 0;
}


static int tostring_lua( lua_State *L )
{
    return lgroonga_tostring( L, MODULE_MT );
}


static int remove_db_lua( lua_State *L )
{
    size_t len = 0;
    const char *path = luaL_checklstring( L, 1, &len );
    grn_obj *db = NULL;
    grn_ctx ctx;
    int rv = 2;
    
    if( !len ){
        lua_pushboolean( L, 0 );
        return 1;
    }
    
    grn_ctx_init( &ctx, 0 );
    if( !( db = grn_db_open( &ctx, path ) ) ){
        lua_pushboolean( L, 0 );
        lua_pushstring( L, ctx.errbuf );
    }
    else if( grn_obj_remove( &ctx, db ) != GRN_SUCCESS ){
        lua_pushboolean( L, 0 );
        lua_pushstring( L, ctx.errbuf );
        grn_obj_unlink( &ctx, db );
    }
    else {
        lua_pushboolean( L, 1 );
        rv = 1;
    }
    grn_ctx_fin( &ctx );
    
    return rv;
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
        { "close", close_lua },
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
        { "removeDb", remove_db_lua },
        { NULL, NULL }
    };
    
    // failed to initialize groonga global variables
    // ???: should i construct error message?
    if( grn_init() != GRN_SUCCESS ){
        return lua_error( L );
    }
    
    // create metatable
    lgroonga_register_mt( L, MODULE_MT, mmethods, methods );
    // create module table
    lgroonga_register_fn( L, funcs );
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

