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

#define LGRN_ENOTABLE  "table has been removed"


// MARK: column API

static int column_lua( lua_State *L )
{
    lgrn_tbl_t *t = luaL_checkudata( L, 1, MODULE_MT );
    
    if( !t->tbl ){
        lua_pushnil( L );
        lua_pushstring( L, LGRN_ENOTABLE );
        return 2;
    }
    else
    {
        size_t len = 0;
        const char *name = luaL_checklstring( L, 2, &len );
        grn_ctx *ctx = lgrn_get_ctx( t->g );
        lgrn_col_t *c = NULL;
        grn_obj *col = ( len > GRN_TABLE_MAX_KEY_SIZE ) ? NULL :
                        grn_obj_column( ctx, t->tbl, name, len );
        
        if( !col ){
            lua_pushnil( L );
            return 1;
        }
        // alloc lgrn_col_t
        else if( ( c = lua_newuserdata( L, sizeof( lgrn_col_t ) ) ) ){
            lstate_setmetatable( L, GROONGA_COLUMN_MT );
            // retain references
            lgrn_col_init( c, t->g, col, lstate_refat( L, 1 ) );
            
            return 1;
        }
        
        // nomem error
        grn_obj_unlink( ctx, col );
        lua_pushnil( L );
        lua_pushstring( L, strerror( errno ) );
        
        return 2;
    }
}


static int columns_next_lua( lua_State *L )
{
    lgrn_tbl_t *t = luaL_checkudata( L, lua_upvalueindex( 1 ), MODULE_MT );
    lgrn_col_iter_t *it = lua_touserdata( L, lua_upvalueindex( 2 ) );
    int ref = lua_tointeger( L, lua_upvalueindex( 3 ) );
    int rv = 0;
    
    if( !t->tbl ){
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
         
        while( ( rc = lgrn_col_iter_next( it, &col ) ) == GRN_SUCCESS )
        {
            // push column name
            oname.len = grn_column_name( it->ctx, col, oname.name, 
                                         GRN_TABLE_MAX_KEY_SIZE );
            lua_pushlstring( L, oname.name, (size_t)oname.len );
            
            // create table metatable
            if( ( c = lua_newuserdata( L, sizeof( lgrn_col_t ) ) ) ){
                lstate_setmetatable( L, GROONGA_COLUMN_MT );
                // push tbl pointer
                lstate_pushref( L, ref );
                // retain references
                lgrn_col_init( c, t->g, col, lstate_ref( L ) );
                
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
    
    lgrn_col_iter_dispose( it );
    lstate_unref( L, ref );
    
    return rv;
}


static int columns_lua( lua_State *L )
{
    lgrn_tbl_t *t = luaL_checkudata( L, 1, MODULE_MT );
    grn_ctx *ctx = lgrn_get_ctx( t->g );
    lgrn_col_iter_t *it = lua_newuserdata( L, sizeof( lgrn_col_iter_t ) );
    
    if( !it ){
        lua_pushnil( L );
        lua_pushstring( L, strerror( errno ) );
        return 2;
    }
    else if( lgrn_col_iter_init( it, ctx, t->tbl ) != GRN_SUCCESS ){
        lua_pushnil( L );
        lua_pushstring( L, ctx->errbuf );
        return 2;
    }
    
    // upvalues: t, it, ref of t
    lua_pushinteger( L, lstate_refat( L, 1 ) );
    lua_pushcclosure( L, columns_next_lua, 3 );
    
    return 1;

}

static int column_create_lua( lua_State *L )
{
    lgrn_tbl_t *t = luaL_checkudata( L, 1, MODULE_MT );
    
    if( !t->tbl ){
        lua_pushnil( L );
        lua_pushstring( L, LGRN_ENOTABLE );
        return 2;
    }
    else
    {
        grn_ctx *ctx = lgrn_get_ctx( t->g );
        size_t len = 0;
        const char *name = NULL;
        const char *path = NULL;
        grn_obj *vtype = NULL;
        grn_obj_flags flags = 0;
        lgrn_col_t *c = NULL;
        grn_obj *col = NULL;
        
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
                if( id == -1 || !( vtype = grn_ctx_at( ctx, id ) ) ){
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
            if( len > UINT_MAX ){
                lua_pushnil( L );
                lua_pushfstring( 
                    L, "column name length must be less than %d", UINT_MAX 
                );
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
                                           (unsigned int)len, path, 
                                           flags, vtype ) ) )
            {
                lstate_setmetatable( L, GROONGA_COLUMN_MT );
                // retain references
                lgrn_col_init( c, t->g, col, lstate_refat( L, 1 ) );
                
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
}


// MARK: table API
static int name_lua( lua_State *L )
{
    lgrn_tbl_t *t = luaL_checkudata( L, 1, MODULE_MT );
    lgrn_objname_t oname;
    
    if( !t->tbl ){
        lua_pushnil( L );
        lua_pushstring( L, LGRN_ENOTABLE );
        return 2;
    }
    else if( lgrn_get_objname( &oname, lgrn_get_ctx( t->g ), t->tbl ) ){
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
    
    if( !t->tbl ){
        lua_pushnil( L );
        lua_pushstring( L, LGRN_ENOTABLE );
        return 2;
    }
    else if( ( path = grn_obj_path( lgrn_get_ctx( t->g ), t->tbl ) ) ){
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
    
    if( !t->tbl ){
        lua_pushnil( L );
        lua_pushstring( L, LGRN_ENOTABLE );
        return 2;
    }
    else
    {
        size_t len = 0;
        const char *name = lgrn_i2n_table( 
            L, t->tbl->header.flags & GRN_OBJ_TABLE_TYPE_MASK, &len 
        );
        
        lua_pushlstring( L, name, len );
        
        return 1;
    }
}


static int key_type_lua( lua_State *L )
{
    lgrn_tbl_t *t = luaL_checkudata( L, 1, MODULE_MT );
    
    if( !t->tbl ){
        lua_pushnil( L );
        lua_pushstring( L, LGRN_ENOTABLE );
        return 2;
    }
    else if( t->tbl->header.domain == GRN_ID_NIL ){
        lua_pushnil( L );
    }
    else {
        size_t len = 0;
        const char *name = lgrn_i2n_data( L, t->tbl->header.domain, &len );
        lua_pushlstring( L, name, len );
    }
    
    return 1;
}


static int val_type_lua( lua_State *L )
{
    lgrn_tbl_t *t = luaL_checkudata( L, 1, MODULE_MT );
    grn_id id = 0;
    
    if( !t->tbl ){
        lua_pushnil( L );
        lua_pushstring( L, LGRN_ENOTABLE );
        return 2;
    }
    
    id = grn_obj_get_range( lgrn_get_ctx( t->g ), t->tbl );
    if( id != GRN_ID_NIL ){
        size_t len = 0;
        const char *name = lgrn_i2n_data( L, id, &len );
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
    
    if( !t->tbl ){
        lua_pushnil( L );
        lua_pushstring( L, LGRN_ENOTABLE );
        return 2;
    }
    
    lua_pushboolean( L, t->tbl->header.flags & GRN_OBJ_PERSISTENT );

    return 1;
}


static int with_sis_lua( lua_State *L )
{
    lgrn_tbl_t *t = luaL_checkudata( L, 1, MODULE_MT );
    
    if( !t->tbl ){
        lua_pushnil( L );
        lua_pushstring( L, LGRN_ENOTABLE );
        return 2;
    }
    
    lua_pushboolean( L, t->tbl->header.flags & GRN_OBJ_KEY_WITH_SIS );
    
    return 1;
}


static int normalize_lua( lua_State *L )
{
    lgrn_tbl_t *t = luaL_checkudata( L, 1, MODULE_MT );
    
    if( !t->tbl ){
        lua_pushnil( L );
        lua_pushstring( L, LGRN_ENOTABLE );
        return 2;
    }
    
    lua_pushboolean( L, t->tbl->header.flags & GRN_OBJ_KEY_NORMALIZE );
    
    return 1;
}


static int remove_lua( lua_State *L )
{
    lgrn_tbl_t *t = luaL_checkudata( L, 1, MODULE_MT );
    
    t->removed = 1;
    // force remove
    if( t->tbl && lua_isboolean( L, 2 ) && lua_toboolean( L, 2 ) ){
        grn_obj_remove( lgrn_get_ctx( t->g ), t->tbl );
        t->tbl = NULL;
    }
    
    return 0;
}


static int tostring_lua( lua_State *L )
{
    return lgrn_tostring( L, MODULE_MT );
}


static int gc_lua( lua_State *L )
{
    lgrn_tbl_t *t = lua_touserdata( L, 1 );
    
    if( t->tbl )
    {
        if( t->removed ){
            grn_obj_remove( lgrn_get_ctx( t->g ), t->tbl );
        }
        else {
            grn_obj_unlink( lgrn_get_ctx( t->g ), t->tbl );
        }
    }
    // release reference
    lstate_unref( L, t->ref_g );
    lgrn_release( t->g );
    
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
    
    return 0;
}

