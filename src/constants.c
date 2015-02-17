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
 *  constants.c
 *  lua-groonga
 *
 *  Created by Masatoshi Teruya on 2015/02/17.
 *
 */

#include "lgroonga.h"

// MARK: globals
static int REF_N2I_DATA;
static int REF_N2I_TABLE;
static int REF_N2I_COLUMN;
static int REF_N2I_COMPRESS;

static lua_Integer name2id( lua_State *L, int ref, const char *name )
{
    lua_Integer val = -1;
    
    lstate_pushref( L, ref );
    lua_pushstring( L, name );
    lua_rawget( L, -2 );
    if( lua_type( L, -1 ) == LUA_TNUMBER ){
        val = lua_tointeger( L, -1 );
    }
    lua_pop( L, 2 );
    
    return val;
}

int lgrn_n2i_data( lua_State *L, const char *name )
{
    return name2id( L, REF_N2I_DATA, name );
}

int lgrn_n2i_table( lua_State *L, const char *name )
{
    return name2id( L, REF_N2I_TABLE, name );
}

int lgrn_n2i_column( lua_State *L, const char *name )
{
    return name2id( L, REF_N2I_COLUMN, name );
}

int lgrn_n2i_compress( lua_State *L, const char *name )
{
    return name2id( L, REF_N2I_COMPRESS, name );
}


static int REF_I2N_DATA;
static int REF_I2N_TABLE;
static int REF_I2N_COLUMN;
static int REF_I2N_COMPRESS;

static const char *id2name( lua_State *L, int ref, int id, size_t *len )
{
    const char *val = NULL;
    
    lstate_pushref( L, ref );
    lua_rawgeti( L, -1, id );
    if( lua_type( L, -1 ) == LUA_TSTRING ){
        val = lua_tolstring( L, -1, len );
    }
    lua_pop( L, 2 );
    
    return val;
}

const char *lgrn_i2n_data( lua_State *L, int id, size_t *len )
{
    return id2name( L, REF_I2N_DATA, id, len );
}

const char *lgrn_i2n_table( lua_State *L, int id, size_t *len )
{
    return id2name( L, REF_I2N_TABLE, id, len );
}

const char *lgrn_i2n_column( lua_State *L, int id, size_t *len )
{
    return id2name( L, REF_I2N_COLUMN, id, len );
}

const char *lgrn_i2n_compress( lua_State *L, int id, size_t *len )
{
    return id2name( L, REF_I2N_COMPRESS, id >> 4, len );
}


LUALIB_API int luaopen_groonga_constants( lua_State *L )
{
    // data type
    lua_newtable( L );
    lstate_int2tbl( L, "VOID", GRN_DB_VOID );
    lstate_int2tbl( L, "DB", GRN_DB_DB );
    lstate_int2tbl( L, "OBJECT", GRN_DB_OBJECT );
    lstate_int2tbl( L, "BOOL", GRN_DB_BOOL );
    lstate_int2tbl( L, "INT8", GRN_DB_INT8 );
    lstate_int2tbl( L, "UINT8", GRN_DB_UINT8 );
    lstate_int2tbl( L, "INT16", GRN_DB_INT16 );
    lstate_int2tbl( L, "UINT16", GRN_DB_UINT16 );
    lstate_int2tbl( L, "INT32", GRN_DB_INT32 );
    lstate_int2tbl( L, "UINT32", GRN_DB_UINT32 );
    lstate_int2tbl( L, "INT64", GRN_DB_INT64 );
    lstate_int2tbl( L, "UINT64", GRN_DB_UINT64 );
    lstate_int2tbl( L, "FLOAT", GRN_DB_FLOAT );
    lstate_int2tbl( L, "TIME", GRN_DB_TIME );
    lstate_int2tbl( L, "SHORT_TEXT", GRN_DB_SHORT_TEXT );
    lstate_int2tbl( L, "TEXT", GRN_DB_TEXT );
    lstate_int2tbl( L, "LONG_TEXT", GRN_DB_LONG_TEXT );
    lstate_int2tbl( L, "TOKYO_GEO_POINT", GRN_DB_TOKYO_GEO_POINT );
    lstate_int2tbl( L, "WGS84_GEO_POINT", GRN_DB_WGS84_GEO_POINT );
    REF_N2I_DATA = lstate_ref( L );
    
    lua_newtable( L );
    lstate_str2arr( L, GRN_DB_VOID, "VOID" );
    lstate_str2arr( L, GRN_DB_DB, "DB" );
    lstate_str2arr( L, GRN_DB_OBJECT, "OBJECT" );
    lstate_str2arr( L, GRN_DB_BOOL, "BOOL" );
    lstate_str2arr( L, GRN_DB_INT8, "INT8" );
    lstate_str2arr( L, GRN_DB_UINT8, "UINT8" );
    lstate_str2arr( L, GRN_DB_INT16, "INT16" );
    lstate_str2arr( L, GRN_DB_UINT16, "UINT16" );
    lstate_str2arr( L, GRN_DB_INT32, "INT32" );
    lstate_str2arr( L, GRN_DB_UINT32, "UINT32" );
    lstate_str2arr( L, GRN_DB_INT64, "INT64" );
    lstate_str2arr( L, GRN_DB_UINT64, "UINT64" );
    lstate_str2arr( L, GRN_DB_FLOAT, "FLOAT" );
    lstate_str2arr( L, GRN_DB_TIME, "TIME" );
    lstate_str2arr( L, GRN_DB_SHORT_TEXT, "SHORT_TEXT" );
    lstate_str2arr( L, GRN_DB_TEXT, "TEXT" );
    lstate_str2arr( L, GRN_DB_LONG_TEXT, "LONG_TEXT" );
    lstate_str2arr( L, GRN_DB_TOKYO_GEO_POINT, "TOKYO_GEO_POINT" );
    lstate_str2arr( L, GRN_DB_WGS84_GEO_POINT, "WGS84_GEO_POINT" );
    REF_I2N_DATA = lstate_ref( L );

    
    // table type
    lua_newtable( L );
    lstate_int2tbl( L, "NO_KEY", GRN_OBJ_TABLE_NO_KEY );
    lstate_int2tbl( L, "HASH_KEY", GRN_OBJ_TABLE_HASH_KEY );
    lstate_int2tbl( L, "PAT_KEY", GRN_OBJ_TABLE_PAT_KEY );
    lstate_int2tbl( L, "DAT_KEY", GRN_OBJ_TABLE_DAT_KEY );
    REF_N2I_TABLE = lstate_ref( L );
    
    lua_newtable( L );
    lstate_str2arr( L, GRN_OBJ_TABLE_NO_KEY, "NO_KEY" );
    lstate_str2arr( L, GRN_OBJ_TABLE_HASH_KEY, "HASH_KEY" );
    lstate_str2arr( L, GRN_OBJ_TABLE_PAT_KEY, "PAT_KEY" );
    lstate_str2arr( L, GRN_OBJ_TABLE_DAT_KEY, "DAT_KEY" );
    REF_I2N_TABLE = lstate_ref( L );
    
    
    // column type
    lua_newtable( L );
    lstate_int2tbl( L, "SCALAR", GRN_OBJ_COLUMN_SCALAR );
    lstate_int2tbl( L, "VECTOR", GRN_OBJ_COLUMN_VECTOR );
    lstate_int2tbl( L, "INDEX", GRN_OBJ_COLUMN_INDEX );
    REF_N2I_COLUMN = lstate_ref( L );
    
    lua_newtable( L );
    lstate_str2arr( L, GRN_OBJ_COLUMN_SCALAR, "SCALAR" );
    lstate_str2arr( L, GRN_OBJ_COLUMN_VECTOR, "VECTOR" );
    lstate_str2arr( L, GRN_OBJ_COLUMN_INDEX, "INDEX" );
    REF_I2N_COLUMN = lstate_ref( L );
    
    
    // compress flags
    lua_newtable( L );
    lstate_int2tbl( L, "ZLIB", GRN_OBJ_COMPRESS_ZLIB );
    lstate_int2tbl( L, "LZ4", GRN_OBJ_COMPRESS_LZ4 );
    REF_N2I_COMPRESS = lstate_ref( L );

    lua_newtable( L );
    lstate_str2arr( L, GRN_OBJ_COMPRESS_ZLIB >> 4, "ZLIB" );
    lstate_str2arr( L, GRN_OBJ_COMPRESS_LZ4 >> 4, "LZ4" );
    REF_I2N_COMPRESS = lstate_ref( L );

    return 0;
}


