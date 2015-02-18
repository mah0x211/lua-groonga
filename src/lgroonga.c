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

#define ITERATOR_MT "groonga.table.iterator"


// helper macrocs
#define CHECK_RET_NIL       lua_pushnil( L )
#define CHECK_RET_FALSE     lua_pushboolean( L, 0 )

#define IS_REMOVED( g ) ((g)->removed)

#define CHECK_EXISTS_EX( L, g, CHECK_RET ) do{ \
    if( IS_REMOVED( g ) ){ \
        CHECK_RET; \
        lua_pushstring( L, LGRN_ENODB ); \
        return 2; \
    } \
}while(0)

#define CHECK_EXISTS( L, g ) \
    CHECK_EXISTS_EX( L, g, CHECK_RET_NIL )

// MARK: table iterator
typedef struct {
    grn_ctx *ctx;
    grn_table_cursor *cur;
} tbl_iter_t;


// init table lookup iterator
static grn_rc tbl_iter_init( tbl_iter_t *it, grn_ctx *ctx )
{
    if( ( it->cur = grn_table_cursor_open( ctx, grn_ctx_db( ctx ), NULL, 0, 
                                           NULL, 0, 0, -1, 0 ) ) ){
        it->ctx = ctx;
        return GRN_SUCCESS;
    }
    
    return ctx->rc;
}


static grn_rc tbl_iter_dispose( tbl_iter_t *it )
{
    if( it->cur ){
        grn_rc rc = grn_table_cursor_close( it->ctx, it->cur );
        it->cur = NULL;
        return rc;
    }
    
    return GRN_SUCCESS;
}


// lookup a next registered table of database
static grn_rc tbl_iter_next( tbl_iter_t *it, grn_obj **tbl )
{
    grn_table_cursor *cur = it->cur;
    grn_obj *obj = NULL;
    grn_id id;
    
    while( ( id = grn_table_cursor_next( it->ctx, cur ) ) != GRN_ID_NIL )
    {
        if( ( obj = grn_ctx_at( it->ctx, id ) ) )
        {
            // return table object
            if( lgrn_obj_istbl( obj ) ){
                *tbl = obj;
                return GRN_SUCCESS;
            }
            grn_obj_unlink( it->ctx, obj );
        }
        else if( it->ctx->rc != GRN_SUCCESS ){
            return it->ctx->rc;
        }
    }
    
    return GRN_END_OF_DATA;
}


static int tbl_iter_gc( lua_State *L )
{
    tbl_iter_t *it = lua_touserdata( L, 1 );

    tbl_iter_dispose( it );
    
    return 0;
}


static void tbl_iter_init_mt( lua_State *L )
{
    struct luaL_Reg mmethods[] = {
        { "__gc", tbl_iter_gc },
        { NULL, NULL }
    };
    
    lgrn_register_mt( L, ITERATOR_MT, mmethods, NULL );
}


// MARK: table API

static int table_lua( lua_State *L )
{
    lgrn_t *g = luaL_checkudata( L, 1, MODULE_MT );
    grn_ctx *ctx = NULL;
    size_t len = 0;
    const char *name = NULL;
    grn_obj *tbl = NULL;
    lgrn_tbl_t *t = NULL;
    
    CHECK_EXISTS( L, g );
    name = luaL_checklstring( L, 2, &len );
    ctx = lgrn_get_ctx( g );
    
    if( len > GRN_TABLE_MAX_KEY_SIZE && 
        !( tbl = grn_ctx_get( ctx, name, (int)len ) ) ){
        lua_pushnil( L );
        return 1;
    }
    else if( !lgrn_obj_istbl( tbl ) ){
        grn_obj_unlink( ctx, tbl );
        lua_pushnil( L );
        return 1;
    }
    // create table metatable
    else if( ( t = lua_newuserdata( L, sizeof( lgrn_tbl_t ) ) ) ){
        lstate_setmetatable( L, GROONGA_TABLE_MT );
        // retain references
        lgrn_tbl_init( t, g, tbl, lstate_refat( L, 1 ) );
        return 1;
    }

    // nomem error
    grn_obj_unlink( ctx, tbl );
    lua_pushnil( L );
    lua_pushstring( L, strerror( errno ) );
    
    return 2;
}


static int tables_next_lua( lua_State *L )
{
    lgrn_t *g = luaL_checkudata( L, lua_upvalueindex( 1 ), MODULE_MT );
    int with_obj = lua_toboolean( L, lua_upvalueindex( 2 ) );
    tbl_iter_t *it = lua_touserdata( L, lua_upvalueindex( 3 ) );
    int rv = 0;
    
    if( IS_REMOVED( g ) ){
        lua_pushnil( L );
        lua_pushstring( L, LGRN_ENODB );
        rv = 2;
    }
    else
    {
        lgrn_tbl_t *t = NULL;
        grn_obj *tbl = NULL;
        lgrn_objname_t oname;
        
        while( tbl_iter_next( it, &tbl ) == GRN_SUCCESS )
        {
            // get table name
            lgrn_get_objname( &oname, it->ctx, tbl );
            lua_pushlstring( L, oname.name, (size_t)oname.len );
            
            // return name
            if( !with_obj ){
                return 1;
            }
            // create table metatable
            else if( ( t = lua_newuserdata( L, sizeof( lgrn_tbl_t ) ) ) ){
                lstate_setmetatable( L, GROONGA_TABLE_MT );
                lgrn_tbl_init( 
                    t, g, tbl, lstate_refat( L, lua_upvalueindex( 1 ) ) 
                );
                
                return 2;
            }
            
            // nomem error
            grn_obj_unlink( it->ctx, tbl );
            lua_pushnil( L );
            lua_pushstring( L, strerror( errno ) );
            rv = 2;
        }
    }
    
    tbl_iter_dispose( it );
    
    return rv;
}


static int tables_lua( lua_State *L )
{
    lgrn_t *g = luaL_checkudata( L, 1, MODULE_MT );
    grn_ctx *ctx = lgrn_get_ctx( g );
    int with_obj = 0;
    tbl_iter_t *it = NULL;
    
    CHECK_EXISTS( L, g );
    ctx = lgrn_get_ctx( g );
        
    // check argument
    if( lua_type( L, 2 ) == LUA_TBOOLEAN ){
        with_obj = lua_toboolean( L, 2 );
    }
    // remove unused stack items
    lua_settop( L, 1 );
    lua_pushboolean( L, with_obj );
    
    // nomem
    if( !( it = lua_newuserdata( L, sizeof( tbl_iter_t ) ) ) ){
        lua_pushnil( L );
        lua_pushstring( L, strerror( errno ) );
        return 2;
    }
    // groonga error
    else if( tbl_iter_init( it, ctx ) != GRN_SUCCESS ){
        lua_pushnil( L );
        lua_pushstring( L, ctx->errbuf );
        return 2;
    }
    
    // set metatable
    lstate_setmetatable( L, ITERATOR_MT );
    // upvalues: g, with_obj, it
    lua_pushcclosure( L, tables_next_lua, 3 );
    
    return 1;
}


static int table_create_lua( lua_State *L )
{
    lgrn_t *g = luaL_checkudata( L, 1, MODULE_MT );
    grn_ctx *ctx = NULL;
    size_t len = 0;
    const char *name = NULL;
    const char *path = NULL;
    grn_obj *ktype = NULL;
    grn_obj *vtype = NULL;
    grn_obj_flags flags = 0;
    grn_obj *tbl = NULL;
    lgrn_tbl_t *t = NULL;
    
    CHECK_EXISTS( L, g );
    ctx = lgrn_get_ctx( g );
    
    // check arguments
    if( lua_gettop( L ) > 1 )
    {
        int id;
        
        lua_settop( L, 2 );
        luaL_checktype( L, 2, LUA_TTABLE );
        
        // path
        path = lstate_toptstring( L, "path", NULL );
        
        // type
        name = lstate_toptstring( L, "type", NULL );
        if( name )
        {
            id = lgrn_n2i_table( L, name );
            if( id == -1 ){
                return luaL_argerror( L, 2, "invalid type value" );
            }
            flags |= id;
        }
        
        // keyType
        name = lstate_toptstring( L, "keyType", NULL );
        if( name )
        {
            id = lgrn_n2i_data( L, name );
            if( id == -1 || !( ktype = grn_ctx_at( ctx, (grn_id)id ) ) ){
                return luaL_argerror( L, 2, "invalid keyType value" );
            }
        }
        
        // valType
        name = lstate_toptstring( L, "valType", NULL );
        if( name )
        {
            id = lgrn_n2i_data( L, name );
            if( id == -1 || !( vtype = grn_ctx_at( ctx, (grn_id)id ) ) ){
                return luaL_argerror( L, 2, "invalid valType value" );
            }
        }
        
        // name
        name = lstate_toptlstring( L, "name", NULL, &len );
        if( len > GRN_TABLE_MAX_KEY_SIZE ){
            lua_pushnil( L );
            lua_pushfstring( L, "table name length must be less than %u", 
                             GRN_TABLE_MAX_KEY_SIZE );
            return 2;
        }
        // set persistent flag automatically if name is not null
        else if( len || lstate_toptboolean( L, "persistent", 0 ) ){
            flags |= GRN_OBJ_PERSISTENT;
        }
        
        // withSIS flag
        if( lstate_toptboolean( L, "withSIS", 0 ) ){
            flags |= GRN_OBJ_KEY_WITH_SIS;
        }
        
        // normalize flag
        if( lstate_toptboolean( L, "normalize", 0 ) ){
            flags |= GRN_OBJ_KEY_NORMALIZE;
        }
    }

    // create table metatable
    if( ( t = lua_newuserdata( L, sizeof( lgrn_tbl_t ) ) ) )
    {
        // create table
        if( ( tbl = grn_table_create( ctx, name, (unsigned int)len, path, 
                                      flags, ktype, vtype ) ) ){
            lstate_setmetatable( L, GROONGA_TABLE_MT );
            // init fields
            lgrn_tbl_init( t, g, tbl, lstate_refat( L, 1 ) );
            
            return 1;
        }
        
        // got error
        lua_pushnil( L );
        lua_pushstring( L, ctx->errbuf );
    }
    // nomem error
    else {
        lua_pushnil( L );
        lua_pushstring( L, strerror( errno ) );
    }

    return 2;
}


// MARK: database API

static int path_lua( lua_State *L )
{
    lgrn_t *g = luaL_checkudata( L, 1, MODULE_MT );
    
    CHECK_EXISTS_EX( L, g, CHECK_RET_FALSE );
    lua_pushstring( L, grn_obj_path( lgrn_get_ctx( g ), lgrn_get_db( g ) ) );
    
    return 1;
}


static int touch_lua( lua_State *L )
{
    lgrn_t *g = luaL_checkudata( L, 1, MODULE_MT );
    
    CHECK_EXISTS_EX( L, g, CHECK_RET_FALSE );
    grn_db_touch( lgrn_get_ctx( g ), lgrn_get_db( g ) );
    lua_pushboolean( L, 1 );
    
    return 1;
}


static int remove_lua( lua_State *L )
{
    lgrn_t *g = luaL_checkudata( L, 1, MODULE_MT );
    
    if( !g->removed )
    {
        grn_obj *db = lgrn_get_db( g );
        
        g->removed = 1;
        // can be remove immediately if reference counter is 0
        if( db && !g->retain ){
            grn_obj_remove( lgrn_get_ctx( g ), db );
        }
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
            ( lua_isboolean( L, 2 ) && lua_toboolean( L, 2 ) && 
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


// MARK: globals

// finalizing groonga library
static int finalize_lua( lua_State *L )
{
    #pragma unused( L )
    
    grn_fin();
    return 0;
}


// initializing groonga library
static int global_init( lua_State *L )
{
    struct luaL_Reg mmethods[] = {
        { "__gc", finalize_lua },
        { NULL, NULL }
    };
    char *finalizer = NULL;
    
    // create metatable
    lgrn_register_mt( L, "groonga.finalizer", mmethods, NULL );
    
    // failed to initialize groonga global variables
    // ???: should i construct error message?
    if( grn_init() != GRN_SUCCESS ){
        lua_pushfstring( L, "failed to grn_init()" );
        return lua_error( L );
    }
    else if( !( finalizer = lua_newuserdata( L, sizeof( char ) ) ) ){
        lua_pushfstring( L, "failed to allocate finalizer" );
        return lua_error( L );
    }
    lstate_setmetatable( L, "groonga.finalizer" );
    lstate_ref( L );
    
    return 0;
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
        { "tableCreate", table_create_lua },
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
    
    // initialize groonga global variables
    global_init( L );
    tbl_iter_init_mt( L );
    // create metatable
    lgrn_register_mt( L, MODULE_MT, mmethods, methods );
    // register related module
    luaopen_groonga_constants( L );
    luaopen_groonga_table( L );
    luaopen_groonga_column( L );
    
    // create module table
    lgrn_register_fn( L, funcs );

    return 1;
}

