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


#define lstate_setmetatable(L,tname) do { \
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

#define lstate_int2tbl(L,k,v) do{ \
    lua_pushstring(L,k); \
    lua_pushinteger(L,v); \
    lua_rawset(L,-3); \
}while(0)

#define lstate_int2arr(L,i,v) do{ \
    lua_pushinteger(L,v); \
    lua_rawseti(L,-2,i); \
}while(0)


static inline int lstate_targerror( lua_State *L, int arg, const char *k, 
                                    int expected, int type )
{
    char msg[255];
    
    snprintf( 
        msg, 255, "%s = %s expected, got %s", 
        k, lua_typename( L, expected ), lua_typename( L, type ) 
    );
    
    return luaL_argerror( L, arg, msg );
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

// MARK: constants
// metatable names
#define GROONGA_MT          "groonga"
#define GROONGA_TABLE_MT    "groonga.table"


// MARK: structures
typedef struct {
    grn_ctx ctx;
    uint8_t removed;
    uint64_t retain;
} lgrn_t;


typedef struct {
    lgrn_t *g;
    grn_obj *tbl;
    uint8_t removed;
    int ref_g;
} lgrn_tbl_t;


// MARK: prototypes
LUALIB_API int luaopen_groonga( lua_State *L );
LUALIB_API int luaopen_groonga_table( lua_State *L );


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
    do {
        lstate_fn2tbl( L, ptr->name, ptr->func );
        ptr++;
    } while( ptr->name );
    
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
        do {
            lstate_fn2tbl( L, ptr->name, ptr->func );
            ptr++;
        } while( ptr->name );
        
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



// MARK: database

static inline lgrn_t *lgrn_retain( lgrn_t *g )
{
    g->retain++;
    return g;
}


static inline uint64_t lgrn_release( lgrn_t *g )
{
    if( g->retain ){
        g->retain--;
    }
    
    return g->retain;
}


static inline grn_ctx *lgrn_get_ctx( lgrn_t *g )
{
    return &g->ctx;
}


static inline grn_obj *lgrn_get_db( lgrn_t *g )
{
    if( !g->removed ){
        return grn_ctx_db( &g->ctx );
    }
    
    return NULL;
}



// MARK: iterator
typedef struct {
    grn_ctx *ctx;
    grn_table_cursor *cur;
} lgrn_iter_t;


static inline grn_rc lgrn_iter_init( lgrn_iter_t *it, grn_ctx *ctx, 
                                     grn_obj *tbl, const void *min, 
                                     unsigned int min_size, const void *max, 
                                     unsigned int max_size, int offset, 
                                     int limit, int flags )
{
    it->cur = grn_table_cursor_open( ctx, tbl, min, min_size, max, max_size, 
                                     offset, limit, flags );
    if( it->cur ){
        it->ctx = ctx;
        return GRN_SUCCESS;
    }
    
    return ctx->rc;
}

static inline grn_rc lgrn_iter_dispose( lgrn_iter_t *it )
{
    return grn_table_cursor_close( it->ctx, it->cur );
}



// MARK: table management
typedef struct {
    int len;
    char name[GRN_TABLE_MAX_KEY_SIZE];
} lgrn_tblname_t;


static inline int lgrn_get_tblname( lgrn_tblname_t *tname, grn_ctx *ctx, 
                                    grn_obj *tbl )
{
    tname->len = grn_obj_name( ctx, tbl, tname->name, GRN_TABLE_MAX_KEY_SIZE );
    return tname->len;
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

// init table lookup iterator.
#define lgrn_tbl_iter_init( it, ctx ) \
    lgrn_iter_init( it, ctx, grn_ctx_db( ctx ), NULL, 0, NULL, 0, 0, -1, 0 )

// lookup a next registered table of database
static inline grn_rc lgrn_tbl_iter_next( lgrn_iter_t *it, grn_obj **tbl )
{
    grn_ctx *ctx = it->ctx;
    grn_table_cursor *cur = it->cur;
    grn_obj *obj = NULL;
    grn_id id;
    
    while( ( id = grn_table_cursor_next( ctx, cur ) ) != GRN_ID_NIL )
    {
        if( ( obj = grn_ctx_at( ctx, id ) ) )
        {
            // return table object
            if( lgrn_obj_istbl( obj ) ){
                *tbl = obj;
                return GRN_SUCCESS;
            }
            grn_obj_unlink( ctx, obj );
        }
        else if( ctx->rc != GRN_SUCCESS ){
            return ctx->rc;
        }
    }
    
    return GRN_END_OF_DATA;
}


#endif
