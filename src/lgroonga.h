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
#include <lua.h>
#include <lauxlib.h>
#include <groonga/groonga.h>

// MARK: helper macros
#define pdealloc(p)     free((void*)(p))

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


// MARK: constants
// metatable names
#define GROONGA_MT          "groonga"
#define GROONGA_TABLE_MT    "groonga.table"


// MARK: structures
typedef struct {
    grn_ctx ctx;
} lgrn_t;


typedef struct {
    grn_ctx *ctx;
    grn_obj *tbl;
    int ref_g;
} lgrn_tbl_t;


typedef struct {
    int len;
    char name[GRN_TABLE_MAX_KEY_SIZE];
} lgrn_tblname_t;


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


#endif
