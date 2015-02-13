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
    lgroonga_t *g = luaL_checkudata( L, 1, MODULE_MT );
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
    lgroonga_t *g = luaL_checkudata( L, 1, MODULE_MT );
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
    lgroonga_t *g = luaL_checkudata( L, 1, MODULE_MT );
    
    close_groonga( g );
    
    return 0;
}


static int open_lua( lua_State *L )
{
    lgroonga_t *g = luaL_checkudata( L, 1, MODULE_MT );
    const char *path = luaL_checkstring( L, 2 );
    
    // close current db
    close_groonga( g );
    if( grn_db_open( &g->ctx, path ) ){
        return 0;
    }
    
    // got error
    lua_pushstring( L, g->ctx.errbuf );
    
    return 1;
}


static int create_lua( lua_State *L )
{
    lgroonga_t *g = luaL_checkudata( L, 1, MODULE_MT );
    const char *path = luaL_checkstring( L, 2 );
    
    // close current db
    close_groonga( g );
    if( grn_db_create( &g->ctx, path, NULL ) ){
        return 0;
    }
    
    // got error
    lua_pushstring( L, g->ctx.errbuf );
    
    return 1;
}


static int gc_lua( lua_State *L )
{
    lgroonga_t *g = (lgroonga_t*)lua_touserdata( L, 1 );
    
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


static int new_lua( lua_State *L )
{
    lgroonga_t *g = NULL;
    
    // alloc context
    if( ( g = lua_newuserdata( L, sizeof( lgroonga_t ) ) ) ){
        grn_ctx_init( &g->ctx, 0 );
        lstate_setmetatable( L, MODULE_MT );
        return 1;
    }
    
    // got error
    lua_pushnil( L );
    lua_pushstring( L, strerror( errno ) );
    
    return 2;
}


LUALIB_API int luaopen_groonga( lua_State *L )
{
    struct luaL_Reg mmethods[] = {
        { "__gc", gc_lua },
        { "__tostring", tostring_lua },
        { NULL, NULL }
    };
    struct luaL_Reg methods[] = {
        { "create", create_lua },
        { "open", open_lua },
        { "close", close_lua },
        { "touch", touch_lua },
        { "path", path_lua },
        { NULL, NULL }
    };
    struct luaL_Reg funcs[] = {
        { "new", new_lua },
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

