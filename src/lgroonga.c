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

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <lua.h>
#include <lauxlib.h>
#include <groonga/groonga.h>

// helper macros
#define pdealloc(p)     free((void*)(p))

#define lstate_setmetatable(L,tname) do { \
    luaL_getmetatable( L, tname ); \
    lua_setmetatable( L, -2 ); \
}while(0)

#define lstate_ref(L) \
    (luaL_ref( L, LUA_REGISTRYINDEX ))

#define lstate_unref(L,ref) \
    luaL_unref( L, LUA_REGISTRYINDEX, (ref) )

#define lstate_fn2tbl(L,k,v) do{ \
    lua_pushstring(L,k); \
    lua_pushcfunction(L,v); \
    lua_rawset(L,-3); \
}while(0)


#define MODULE_MT   "groonga"


typedef struct {
    grn_ctx ctx;
    const char *path;
    grn_obj *db;
} lgroonga_t;


static int path_lua( lua_State *L )
{
    lgroonga_t *g = luaL_checkudata( L, 1, MODULE_MT );

    lua_pushstring( L, g->path );
    return 1;
}


static int gc_lua( lua_State *L )
{
    lgroonga_t *g = (lgroonga_t*)lua_touserdata( L, 1 );
    
    // close db
    grn_obj_close( &g->ctx, g->db );
    // free
    grn_ctx_fin( &g->ctx );
    pdealloc( g->path );
    
    return 0;
}


static int tostring_lua( lua_State *L )
{
    lua_pushfstring( L, "<" MODULE_MT " %p>", lua_touserdata( L, 1 ) );
    return 1;
}


// create or open database
#define create_groonga( g ) grn_db_create( &(g)->ctx, (g)->path, NULL )
#define open_groonga( g )   grn_db_open( &(g)->ctx, (g)->path )

// allocate lua object
#define alloc_groonga( L, fn ) do { \
    lgroonga_t *g = NULL; \
    /* alloc userdata */ \
    if( !( g = lua_newuserdata( L, sizeof( lgroonga_t ) ) ) ){ \
        lua_pushnil( L ); \
        lua_pushstring( L, strerror( errno ) ); \
        return 2; \
    } \
    /* init context and open or create database and resolve path */ \
    g->path = luaL_checkstring( L, 1 ); \
    grn_ctx_init( &g->ctx, 0 ); \
    if( !( g->db = fn( g ) ) || \
        !( g->path = realpath( g->path, NULL ) ) ){ \
        /* got error */ \
        lua_pushnil( L ); \
        lua_pushstring( L, g->ctx.errbuf ); \
        grn_ctx_fin( &g->ctx ); \
        return 2; \
    } \
    lstate_setmetatable( L, MODULE_MT ); \
}while(0)


static int open_lua( lua_State *L )
{
    alloc_groonga( L, open_groonga );
    return 1;
}


static int new_lua( lua_State *L )
{
    alloc_groonga( L, create_groonga );
    return 1;
}


LUALIB_API int luaopen_groonga( lua_State *L )
{
    struct luaL_Reg mmethod[] = {
        { "__gc", gc_lua },
        { "__tostring", tostring_lua },
        { NULL, NULL }
    };
    struct luaL_Reg methods[] = {
        { "path", path_lua },
        { NULL, NULL }
    };
    struct luaL_Reg *ptr = mmethod;
    
    // failed to initialize groonga global variables
    // ???: should i construct error message?
    if( grn_init() != GRN_SUCCESS ){
        return lua_error( L );
    }
    
    // create metatable
    luaL_newmetatable( L, MODULE_MT );
    // add metamethods
    while( ptr->name ){
        lstate_fn2tbl( L, ptr->name, ptr->func );
        ptr++;
    }
    // add methods
    ptr = methods;
    lua_pushstring( L, "__index" );
    lua_newtable( L );
    while( ptr->name ){
        lstate_fn2tbl( L, ptr->name, ptr->func );
        ptr++;
    }
    lua_rawset( L, -3 );
    lua_pop( L, 1 );
    
    // add allocator
    lua_newtable( L );
    lstate_fn2tbl( L, "new", new_lua );
    lstate_fn2tbl( L, "open", open_lua );

    return 1;
}

