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
 *  lgroonga.h
 *  lua-groonga
 *
 *  Created by Masatoshi Teruya on 2015/02/13.
 *
 */

#ifndef lgroonga_h
#define lgroonga_h

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <math.h>
#include <lua.h>
#include <lauxlib.h>
#include <groonga/groonga.h>

// MARK: helper macros
#define pdealloc(p)     free((void*)(p))

#define LUANUM_ISDBL(val)   ((lua_Number)((lua_Integer)val) != val)

#define LUANUM_ISUINT(val)  (!signbit( val ) && !LUANUM_ISDBL( val ))


#define lstate_setmetatable(L,tname) do{ \
    luaL_getmetatable( L, tname ); \
    lua_setmetatable( L, -2 ); \
}while(0)

#define lstate_ref(L) \
    (luaL_ref( L, LUA_REGISTRYINDEX ))

#define lstate_refat(L,idx) \
    (lua_pushvalue(L,idx),luaL_ref( L, LUA_REGISTRYINDEX ))

#define lstate_unref(L,ref) \
    luaL_unref( L, LUA_REGISTRYINDEX, (ref) )

#define lstate_pushref(L,ref) \
    lua_rawgeti( L, LUA_REGISTRYINDEX, ref )

#define lstate_fn2tbl(L,k,v) do{ \
    lua_pushstring(L,k); \
    lua_pushcfunction(L,v); \
    lua_rawset(L,-3); \
}while(0)

#define lstate_str2tbl(L,k,v) do{ \
    lua_pushstring(L,k); \
    lua_pushstring(L,v); \
    lua_rawset(L,-3); \
}while(0)

#define lstate_int2tbl(L,k,v) do{ \
    lua_pushstring(L,k); \
    lua_pushinteger(L,v); \
    lua_rawset(L,-3); \
}while(0)

#define lstate_int2arr(L,i,v) do{ \
    lua_pushinteger(L,v); \
    lua_rawseti(L,-2,i); \
}while(0)


#define lstate_str2arr(L,i,v) do{ \
    lua_pushinteger(L,i); \
    lua_pushstring(L,v); \
    lua_rawset(L,-3); \
}while(0)


#define lstate_argerror( L, arg, ... ) do{ \
    char _msg[255]; \
    snprintf( _msg, 255, __VA_ARGS__ ); \
    return luaL_argerror( L, arg, _msg ); \
}while(0)


static inline int lstate_targerror( lua_State *L, int arg, const char *k, 
                                    int expected, int type )
{
    lstate_argerror( 
        L, arg, "%s = %s expected, got %s", 
        k, lua_typename( L, expected ), lua_typename( L, type ) 
    );
}


static inline int lstate_tchecktype( lua_State *L, const char *k, int t, 
                                     int except_nil )
{ 
    const int argc = lua_gettop( L );
    int type = 0;
    
    lua_pushstring( L, k );
    lua_gettable( L, -2 );
    type = lua_type( L, -1 );
    if( type != t )
    {
        lua_pop( L, 1 );
        if( except_nil && type == LUA_TNIL ){
            return LUA_TNIL;
        }
        else {
            return lstate_targerror( L, argc, k, t, type );
        }
    }
    
    return t;
}


static inline const char *lstate_tchecklstring( lua_State *L, const char *k, 
                                                size_t *len )
{ 
    const char *v = NULL;
    
    lstate_tchecktype( L, k, LUA_TSTRING, 0 );
    v = lua_tolstring( L, -1, len );
    lua_pop( L, 1 );
    
    return v;
}


static inline const char *lstate_tcheckstring( lua_State *L, const char *k )
{ 
    size_t len = 0;
    return lstate_tchecklstring( L, k, &len );
}


static inline const char *lstate_toptlstring( lua_State *L, const char *k, 
                                              const char *defval, size_t *len )
{
    if( lstate_tchecktype( L, k, LUA_TSTRING, 1 ) == LUA_TSTRING ){
        const char *v = lua_tolstring( L, -1, len );
        lua_pop( L, 1 );
        return v;
    }
    
    return defval;
}


static inline const char *lstate_toptstring( lua_State *L, const char *k, 
                                             const char *defval )
{
    size_t len = 0;
    return lstate_toptlstring( L, k, defval, &len );
}


static inline lua_Integer lstate_toptinteger( lua_State *L, const char *k,
                                              lua_Integer defval )
{
    if( lstate_tchecktype( L, k, LUA_TNUMBER, 1 ) == LUA_TNUMBER ){
        lua_Integer v = lua_tointeger( L, -1 );
        lua_pop( L, 1 );
        return v;
    }
    
    return defval;
}


static inline int lstate_toptboolean( lua_State *L, const char *k, int defval )
{
    if( lstate_tchecktype( L, k, LUA_TBOOLEAN, 1 ) == LUA_TBOOLEAN ){
        int v = lua_toboolean( L, -1 );
        lua_pop( L, 1 );
        return v;
    }
    
    return defval;
}


// MARK: constants
// metatable names
#define GROONGA_MT          "groonga"
#define GROONGA_TABLE_MT    "groonga.table"
#define GROONGA_COLUMN_MT   "groonga.column"


// MARK: prototypes
LUALIB_API int luaopen_groonga( lua_State *L );
LUALIB_API int luaopen_groonga_table( lua_State *L );
LUALIB_API int luaopen_groonga_column( lua_State *L );


// constants conversion
void lgrn_constants_init( lua_State *L );
// get constants value by name
int lgrn_n2i_data( lua_State *L, const char *name );
int lgrn_n2i_table( lua_State *L, const char *name );
int lgrn_n2i_column( lua_State *L, const char *name );
int lgrn_n2i_compress( lua_State *L, const char *name );
// get name by constants value
const char *lgrn_i2n_data( lua_State *L, int id, size_t *len );
const char *lgrn_i2n_table( lua_State *L, int id, size_t *len );
const char *lgrn_i2n_column( lua_State *L, int id, size_t *len );
const char *lgrn_i2n_compress( lua_State *L, int id, size_t *len );


// MARK: helper API
// common metamethods
#define lgrn_tostring(L,tname) ({ \
    lua_pushfstring( L, tname ": %p", lua_touserdata( L, 1 ) ); \
    1; \
})


// metanames
// module definition register
static inline int lgrn_register_fn( lua_State *L, struct luaL_Reg method[] )
{
    struct luaL_Reg *ptr = method;
    
    // methods
    lua_newtable( L );
    while( ptr->name ){
        lstate_fn2tbl( L, ptr->name, ptr->func );
        ptr++;
    }
    
    return 1;
}


static inline int lgrn_register_mt( lua_State *L, const char *tname, 
                                    struct luaL_Reg mmethod[], 
                                    struct luaL_Reg method[] )
{
    // create table __metatable
    if( luaL_newmetatable( L, tname ) )
    {
        struct luaL_Reg *ptr = mmethod;
        
        // metamethods
        while( ptr->name ){
            lstate_fn2tbl( L, ptr->name, ptr->func );
            ptr++;
        }
        
        // methods
        if( method ){
            lua_pushstring( L, "__index" );
            lgrn_register_fn( L, method );
            lua_rawset( L, -3 );
        }
        lua_pop( L, 1 );
        
        return 1;
    }
    
    return 0;
}


// MARK: object
typedef struct {
    int len;
    char name[GRN_TABLE_MAX_KEY_SIZE];
} lgrn_objname_t;

// get a non null-terminated name string
static inline int lgrn_get_objname( lgrn_objname_t *oname, grn_ctx *ctx, 
                                    grn_obj *obj )
{
    oname->len = grn_obj_name( ctx, obj, oname->name, GRN_TABLE_MAX_KEY_SIZE );
    return oname->len;
}


static inline int lgrn_obj_ispersistent( grn_obj *obj )
{
    return obj->header.flags & GRN_OBJ_PERSISTENT;
}


static inline int lgrn_obj_istbl( grn_obj *obj )
{
    switch( obj->header.type ){
        case GRN_TABLE_HASH_KEY:
        case GRN_TABLE_PAT_KEY:
        case GRN_TABLE_DAT_KEY:
        case GRN_TABLE_NO_KEY:
            return 1;
        
        default:
            return 0;
    }
}


// MARK: database

#define LGRN_ENODB  "database has been removed"

typedef struct {
    grn_ctx ctx;
    uint8_t removed;
} lgrn_t;


static inline grn_ctx *lgrn_get_ctx( lgrn_t *g )
{
    return &g->ctx;
}


static inline grn_obj *lgrn_get_db( lgrn_t *g )
{
    return grn_ctx_db( &g->ctx );
}


// MARK: table

#define LGRN_ENOTABLE  "table has been removed"

typedef struct {
    lgrn_t *g;
    grn_obj *tbl;
    uint8_t removed;
    int ref_g;
} lgrn_tbl_t;


// initialize lgrn_tbl_t
static inline void lgrn_tbl_init( lgrn_tbl_t *t, lgrn_t *g, grn_obj *tbl, 
                                  int ref )
{
    t->ref_g = ref;
    t->g = g;
    t->tbl = tbl;
    t->removed = 0;
}


// MARK: column management

#define LGRN_ENOCOLUMN  "column has been removed"

typedef struct {
    lgrn_tbl_t *t;
    grn_obj *col;
    uint8_t removed;
    int ref_t;
} lgrn_col_t;


// initialize lgrn_col_t
static inline void lgrn_col_init( lgrn_col_t *c, lgrn_tbl_t *t, grn_obj *col, 
                                  int ref )
{
    c->ref_t = ref;
    c->t = t;
    c->col = col;
    c->removed = 0;
}


#endif
