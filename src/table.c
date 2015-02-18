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
 *  table.c
 *  lua-groonga
 *
 *  Created by Masatoshi Teruya on 2015/02/15.
 *
 */

#include "lgroonga.h"

#define MODULE_MT   GROONGA_TABLE_MT

// helper macrocs
#define CHECK_RET_NIL       lua_pushnil( L )
#define CHECK_RET_FALSE     lua_pushboolean( L, 0 )

#define IS_REMOVED( t ) \
    ((t)->removed || (t)->g->removed)

#define CHECK_EXISTS_EX( L, t, CHECK_RET ) do{ \
    if( (t)->g->removed ){ \
        CHECK_RET; \
        lua_pushstring( L, LGRN_ENODB ); \
        return 2; \
    } \
    else if( (t)->removed ){ \
        CHECK_RET; \
        lua_pushstring( L, LGRN_ENOTABLE ); \
        return 2; \
    } \
}while(0)

#define CHECK_EXISTS( L, t ) \
    CHECK_EXISTS_EX( L, t, CHECK_RET_NIL )



// MARK: column lookup iterator

#define ITERATOR_MT "groonga.column.iterator"

typedef struct {
    grn_ctx *ctx;
    grn_hash_cursor *cur;
    grn_hash *cols;
    int ncols;
} col_iter_t;


// init iterator
static grn_rc col_iter_init( col_iter_t *it, grn_ctx *ctx, grn_obj *tbl )
{
    // create container
    grn_hash *cols = grn_hash_create( ctx, NULL, sizeof( grn_id ), 0, 
                                      GRN_OBJ_TABLE_HASH_KEY );
    
    if( cols )
    {
        it->ncols = grn_table_columns( ctx, tbl, NULL, 0, (grn_obj*)cols );
        it->cur = grn_hash_cursor_open( ctx, cols, NULL, 0, NULL, 0, 0, -1, 0 );
        if( it->cur ){
            it->ctx = ctx;
            it->cols = cols;
            return GRN_SUCCESS;
        }
        
        grn_hash_close( ctx, cols );
    }
    
    return ctx->rc;
}


static inline grn_rc col_iter_dispose( col_iter_t *it )
{
    if( it->cur ){
        grn_hash_cursor_close( it->ctx, it->cur );
        it->cur = NULL;
        return grn_hash_close( it->ctx, it->cols );
    }
    
    return GRN_SUCCESS;
}


static int col_iter_gc( lua_State *L )
{
    col_iter_t *it = lua_touserdata( L, 1 );

    col_iter_dispose( it );
    
    return 0;
}


static void col_iter_init_mt( lua_State *L )
{
    struct luaL_Reg mmethods[] = {
        { "__gc", col_iter_gc },
        { NULL, NULL }
    };
    
    lgrn_register_mt( L, ITERATOR_MT, mmethods, NULL );
}


// lookup a next column of table
static inline grn_rc col_iter_next( col_iter_t *it, grn_obj **col )
{
    grn_ctx *ctx = it->ctx;
    grn_hash_cursor *cur = it->cur;
    grn_obj *obj = NULL;
    grn_id *id = NULL;
    int rv = 0 ;
    
    while( grn_hash_cursor_next( ctx, cur ) != GRN_ID_NIL )
    {
        //
        // ctx: grn_ctx*                    | context
        // cur: grn_hash_cursor*            | cursor
        // key: void** or NULL              | key
        // ksz: unsigned int* or NULL       | key size
        // val: void** or NULL              | value
        // ret: int or GRN_INVALID_ARGUMENT | value size
        //
        // int grn_hash_cursor_get_key_value( grn_ctx *ctx,
        //                                    grn_hash_cursor *cur,
        //                                    void **key, unsigned int *ksz,
        //                                    void **val );
        //
        rv = grn_hash_cursor_get_key_value( ctx, cur, (void**)&id, NULL, NULL );
        if( rv == GRN_INVALID_ARGUMENT ){
            return GRN_INVALID_ARGUMENT;
        }
        else if( ( obj = grn_ctx_at(ctx, *id ) ) ){
            *col = obj;
            return GRN_SUCCESS;
        }
        else if( ctx->rc != GRN_SUCCESS ){
            return ctx->rc;
        }
        grn_obj_unlink( ctx, obj );
    }
    
    return GRN_END_OF_DATA;
}


// MARK: column API

static int column_lua( lua_State *L )
{
    lgrn_tbl_t *t = luaL_checkudata( L, 1, MODULE_MT );
    size_t len = 0;
    const char *name = NULL;
    grn_ctx *ctx = NULL;
    lgrn_col_t *c = NULL;
    grn_obj *col = NULL;
    
    CHECK_EXISTS( L, t );
    name = luaL_checklstring( L, 2, &len );
    ctx = lgrn_get_ctx( t->g );
    
    if( len > GRN_TABLE_MAX_KEY_SIZE ||
        !( col = grn_obj_column( ctx, t->tbl, name, (unsigned int)len ) ) ){
        lua_pushnil( L );
        return 1;
    }
    // lookup from weak reference
    else if( lgrn_refget_col( L, name, len ) ){
        grn_obj_unlink( ctx, col );
        return 1;
    }
    // alloc lgrn_col_t
    else if( ( c = lua_newuserdata( L, sizeof( lgrn_col_t ) ) ) ){
        lstate_setmetatable( L, GROONGA_COLUMN_MT );
        // retain references
        lgrn_col_init( c, t, col, lstate_refat( L, 1 ) );
        
        return 1;
    }
    
    // nomem error
    grn_obj_unlink( ctx, col );
    lua_pushnil( L );
    lua_pushstring( L, strerror( errno ) );
    
    return 2;
}


static int columns_next_lua( lua_State *L )
{
    lgrn_tbl_t *t = luaL_checkudata( L, lua_upvalueindex( 1 ), MODULE_MT );
    int with_obj = lua_toboolean( L, lua_upvalueindex( 2 ) );
    col_iter_t *it = lua_touserdata( L, lua_upvalueindex( 3 ) );
    int rv = 0;
    
    if( IS_REMOVED( t ) ){
        lua_pushnil( L );
        lua_pushstring( L, LGRN_ENOTABLE );
        rv = 2;
    }
    else
    {
        lgrn_col_t *c = NULL;
        grn_obj *col = NULL;
        lgrn_objname_t oname;
        int rc = 0;
         
        while( ( rc = col_iter_next( it, &col ) ) == GRN_SUCCESS )
        {
            // push column name
            oname.len = grn_column_name( it->ctx, col, oname.name, 
                                         GRN_TABLE_MAX_KEY_SIZE );
            lua_pushlstring( L, oname.name, (size_t)oname.len );
            
            // return name
            if( !with_obj ){
                return 1;
            }
            // lookup from weak reference
            else if( lgrn_refget_col( L, oname.name, (size_t)oname.len ) ){
                grn_obj_unlink( it->ctx, col );
                return 2;
            }
            // create table metatable
            else if( ( c = lua_newuserdata( L, sizeof( lgrn_col_t ) ) ) ){
                lstate_setmetatable( L, GROONGA_COLUMN_MT );
                // save reference
                lgrn_refset_col( L, oname.name, (size_t)oname.len, -1 );
                // init fields
                lgrn_col_init( 
                    c, t, col, lstate_refat( L, lua_upvalueindex( 1 ) )
                );
                
                return 2;
            }
            
            // nomem error
            grn_obj_unlink( it->ctx, col );
            lua_pushnil( L );
            lua_pushstring( L, strerror( errno ) );
            rv = 2;
        }
        
        if( rc != GRN_SUCCESS && rc != GRN_END_OF_DATA ){
            lua_pushnil( L );
            lua_pushstring( L, strerror( errno ) );
            rv = 2;
        }
    }
    
    col_iter_dispose( it );
    
    return rv;
}


static int columns_lua( lua_State *L )
{
    lgrn_tbl_t *t = luaL_checkudata( L, 1, MODULE_MT );
    grn_ctx *ctx = NULL;
    int with_obj = 0;
    col_iter_t *it = NULL;
    
    CHECK_EXISTS( L, t );
    ctx = lgrn_get_ctx( t->g );
    
    // check argument
    if( lua_type( L, 2 ) == LUA_TBOOLEAN ){
        with_obj = lua_toboolean( L, 2 );
    }
    // remove unused stack items
    lua_settop( L, 1 );
    lua_pushboolean( L, with_obj );
    
    // nomem
    if( !( it = lua_newuserdata( L, sizeof( col_iter_t ) ) ) ){
        lua_pushnil( L );
        lua_pushstring( L, strerror( errno ) );
        return 2;
    }
    else if( col_iter_init( it, ctx, t->tbl ) != GRN_SUCCESS ){
        lua_pushnil( L );
        lua_pushstring( L, ctx->errbuf );
        return 2;
    }
    
    // set metatable
    lstate_setmetatable( L, ITERATOR_MT );
    // upvalues: t, with_obj, it
    lua_pushcclosure( L, columns_next_lua, 3 );
    
    return 1;
}


static int column_create_lua( lua_State *L )
{
    lgrn_tbl_t *t = luaL_checkudata( L, 1, MODULE_MT );
    grn_ctx *ctx = NULL;
    size_t len = 0;
    const char *name = NULL;
    const char *path = NULL;
    grn_obj *vtype = NULL;
    grn_obj_flags flags = 0;
    lgrn_col_t *c = NULL;
    grn_obj *col = NULL;
    
    CHECK_EXISTS( L, t );
    ctx = lgrn_get_ctx( t->g );
    
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
            id = lgrn_n2i_column( L, name );
            if( id == -1 ){
                return luaL_argerror( L, 2, "invalid type value" );
            }
            flags |= id;
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
        
        // compress
        name = lstate_toptstring( L, "compress", NULL );
        if( name )
        {
            id = lgrn_n2i_compress( L, name );
            if( id == -1 ){
                return luaL_argerror( L, 2, "invalid compress value" );
            }
            flags |= id;
        }
        
        // name
        name = lstate_toptlstring( L, "name", NULL, &len );
        if( len > GRN_TABLE_MAX_KEY_SIZE ){
            lua_pushnil( L );
            lua_pushfstring( L, "column name length must be less than %u",
                             GRN_TABLE_MAX_KEY_SIZE );
            return 2;
        }
        
        // persistent flag
        if( lstate_toptboolean( L, "persistent", 0 ) ){
            flags |= GRN_OBJ_PERSISTENT;
        }
        // check path option
        else if( path ){
            lua_pushnil( L );
            lua_pushfstring( L, "should be enabled persistent option if " \
                                "path option specified" );
            return 2;
        }
        
        // withWeight flag
        if( lstate_toptboolean( L, "withWeight", 0 ) )
        {
            if( flags & GRN_OBJ_COLUMN_INDEX || 
                flags & GRN_OBJ_COLUMN_VECTOR ){
                flags |= GRN_OBJ_WITH_WEIGHT;
            }
            else {
                lua_pushnil( L );
                lua_pushfstring( L, "withWeight option should be used " \
                                    "to the INDEX or VECTOR column" );
                return 2;
            }
        }
        
        // withSection flag
        if( lstate_toptboolean( L, "withSection", 0 ) )
        {
            if( flags & GRN_OBJ_COLUMN_INDEX ){
                flags |= GRN_OBJ_WITH_SECTION;
            }
            else {
                lua_pushnil( L );
                lua_pushfstring( L, "withSection option should be used " \
                                    "to the INDEX column" );
                return 2;
            }
        }

        // withPosition flag
        if( lstate_toptboolean( L, "withPosition", 0 ) )
        {
            if( flags & GRN_OBJ_COLUMN_INDEX ){
                flags |= GRN_OBJ_WITH_POSITION;
            }
            else {
                lua_pushnil( L );
                lua_pushfstring( L, "withPosition option should be used " \
                                    "to the INDEX column" );
                return 2;
            }
        }
    }

    // create column metatable
    if( ( c = lua_newuserdata( L, sizeof( lgrn_col_t ) ) ) )
    {
        // create table
        if( ( col = grn_column_create( ctx, t->tbl, name, 
                                       (unsigned int)len, path, flags,
                                       vtype ) ) ){
            lstate_setmetatable( L, GROONGA_COLUMN_MT );
            // save reference
            lgrn_refset_col( L, name, len, -1 );
            // init fields
            lgrn_col_init( c, t, col, lstate_refat( L, 1 ) );
            
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


// MARK: table API
static int name_lua( lua_State *L )
{
    lgrn_tbl_t *t = luaL_checkudata( L, 1, MODULE_MT );
    lgrn_objname_t oname;
    
    CHECK_EXISTS( L, t );
    if( lgrn_get_objname( &oname, lgrn_get_ctx( t->g ), t->tbl ) ){
        lua_pushlstring( L, oname.name, (size_t)oname.len );
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
    const char *path = NULL;
    
    CHECK_EXISTS( L, t );
    if( ( path = grn_obj_path( lgrn_get_ctx( t->g ), t->tbl ) ) ){
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
    size_t len = 0;
    const char *name = NULL;
    
    CHECK_EXISTS( L, t );
    name = lgrn_i2n_table( L, t->tbl->header.flags & GRN_OBJ_TABLE_TYPE_MASK, 
                           &len );
    lua_pushlstring( L, name, len );
    
    return 1;
}


static int key_type_lua( lua_State *L )
{
    lgrn_tbl_t *t = luaL_checkudata( L, 1, MODULE_MT );
    
    CHECK_EXISTS( L, t );
    if( t->tbl->header.domain == GRN_ID_NIL ){
        lua_pushnil( L );
    }
    else {
        size_t len = 0;
        const char *name = lgrn_i2n_data( L, (int)t->tbl->header.domain, &len );
        lua_pushlstring( L, name, len );
    }
    
    return 1;
}


static int val_type_lua( lua_State *L )
{
    lgrn_tbl_t *t = luaL_checkudata( L, 1, MODULE_MT );
    grn_id id = 0;
    
    CHECK_EXISTS( L, t );
    id = grn_obj_get_range( lgrn_get_ctx( t->g ), t->tbl );
    if( id != GRN_ID_NIL ){
        size_t len = 0;
        const char *name = lgrn_i2n_data( L, (int)id, &len );
        lua_pushlstring( L, name, len );
    }
    else {
        lua_pushnil( L );
    }
    
    return 1;
}


static int persistent_lua( lua_State *L )
{
    lgrn_tbl_t *t = luaL_checkudata( L, 1, MODULE_MT );
    
    CHECK_EXISTS_EX( L, t, CHECK_RET_FALSE );
    lua_pushboolean( L, lgrn_obj_ispersistent( t->tbl ) );

    return 1;
}


static int with_sis_lua( lua_State *L )
{
    lgrn_tbl_t *t = luaL_checkudata( L, 1, MODULE_MT );
    
    CHECK_EXISTS_EX( L, t, CHECK_RET_FALSE );
    lua_pushboolean( L, t->tbl->header.flags & GRN_OBJ_KEY_WITH_SIS );
    
    return 1;
}


static int normalize_lua( lua_State *L )
{
    lgrn_tbl_t *t = luaL_checkudata( L, 1, MODULE_MT );
    
    CHECK_EXISTS_EX( L, t, CHECK_RET_FALSE );
    lua_pushboolean( L, t->tbl->header.flags & GRN_OBJ_KEY_NORMALIZE );
    
    return 1;
}


static int db_lua( lua_State *L )
{
    lgrn_tbl_t *t = luaL_checkudata( L, 1, MODULE_MT );
    
    CHECK_EXISTS( L, t );
    // push an associated database
    lstate_pushref( L, t->ref_g );
    
    return 1;
}


static int remove_lua( lua_State *L )
{
    lgrn_tbl_t *t = luaL_checkudata( L, 1, MODULE_MT );
    
    CHECK_EXISTS_EX( L, t, CHECK_RET_FALSE );
    grn_obj_remove( lgrn_get_ctx( t->g ), t->tbl );
    t->removed = 1;
    t->tbl = NULL;
    lua_pushboolean( L, 1 );
    
    return 1;
}


static int tostring_lua( lua_State *L )
{
    return lgrn_tostring( L, MODULE_MT );
}


static int gc_lua( lua_State *L )
{
    lgrn_tbl_t *t = lua_touserdata( L, 1 );
    
    if( t->tbl && !t->g->removed ){
        grn_obj_unlink( lgrn_get_ctx( t->g ), t->tbl );
    }
    // release reference
    lstate_unref( L, t->ref_g );

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
        { "remove", remove_lua },
        { "db", db_lua },
        { "name", name_lua },
        { "path", path_lua },
        { "type", type_lua },
        { "keyType", key_type_lua },
        { "valType", val_type_lua },
        { "persistent", persistent_lua },
        { "withSIS", with_sis_lua },
        { "normalize", normalize_lua },
        { "column", column_lua },
        { "columns", columns_lua },
        { "columnCreate", column_create_lua },
        { NULL, NULL }
    };
    
    lgrn_register_mt( L, MODULE_MT, mmethods, methods );
    col_iter_init_mt( L );
    
    return 0;
}

