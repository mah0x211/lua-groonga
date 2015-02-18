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
 *  column.c
 *  lua-groonga
 *
 *  Created by Masatoshi Teruya on 2015/02/17.
 *
 */

#include "lgroonga.h"

#define MODULE_MT   GROONGA_COLUMN_MT

#define LGRN_ENOCOLUMN  "column has been removed"


// MARK: column API

static int name_lua( lua_State *L )
{
    lgrn_col_t *c = luaL_checkudata( L, 1, MODULE_MT );
    lgrn_objname_t oname;
    
    if( !c->col ){
        lua_pushnil( L );
        lua_pushstring( L, LGRN_ENOCOLUMN );
        return 2;
    }
    
    oname.len = grn_column_name( lgrn_get_ctx( c->t->g ), c->col, oname.name, 
                                 GRN_TABLE_MAX_KEY_SIZE );
    lua_pushlstring( L, oname.name, (size_t)oname.len );
    
    return 1;
}


static int path_lua( lua_State *L )
{
    lgrn_col_t *c = luaL_checkudata( L, 1, MODULE_MT );
    const char *path = NULL;
    
    if( !c->col ){
        lua_pushnil( L );
        lua_pushstring( L, LGRN_ENOCOLUMN );
        return 2;
    }
    else if( ( path = grn_obj_path( lgrn_get_ctx( c->t->g ), c->col ) ) ){
        lua_pushstring( L, path );
    }
    else {
        lua_pushnil( L );
    }
    
    return 1;
}


static int type_lua( lua_State *L )
{
    lgrn_col_t *c = luaL_checkudata( L, 1, MODULE_MT );
    
    if( !c->col ){
        lua_pushnil( L );
        lua_pushstring( L, LGRN_ENOCOLUMN );
        return 2;
    }
    else
    {
        size_t len = 0;
        const char *name = lgrn_i2n_column( L, 
            c->col->header.flags & GRN_OBJ_COLUMN_TYPE_MASK, &len 
        );
        
        lua_pushlstring( L, name, len );
        
        return 1;
    }
}


static int val_type_lua( lua_State *L )
{
    lgrn_col_t *c = luaL_checkudata( L, 1, MODULE_MT );
    
    if( !c->col ){
        lua_pushnil( L );
        lua_pushstring( L, LGRN_ENOCOLUMN );
        return 2;
    }
    else
    {
        grn_ctx *ctx = lgrn_get_ctx( c->t->g );
        grn_id id = grn_obj_get_range( ctx, c->col );
        
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
}


static int compress_lua( lua_State *L )
{
    lgrn_col_t *c = luaL_checkudata( L, 1, MODULE_MT );
    grn_id id = 0;
    
    if( !c->col ){
        lua_pushnil( L );
        lua_pushstring( L, LGRN_ENOCOLUMN );
        return 2;
    }
    else if( ( id = c->col->header.flags & GRN_OBJ_COMPRESS_MASK ) ){
        size_t len = 0;
        const char *name = lgrn_i2n_compress( L, id, &len );
        lua_pushlstring( L, name, len );
    }
    else {
        lua_pushnil( L );
    }
    
    return 1;
}


static int persistent_lua( lua_State *L )
{
    lgrn_col_t *c = luaL_checkudata( L, 1, MODULE_MT );
    
    if( !c->col ){
        lua_pushnil( L );
        lua_pushstring( L, LGRN_ENOCOLUMN );
        return 2;
    }
    
    lua_pushboolean( L, c->col->header.flags & GRN_OBJ_PERSISTENT );

    return 1;
}


static int with_weight_lua( lua_State *L )
{
    lgrn_col_t *c = luaL_checkudata( L, 1, MODULE_MT );
    
    if( !c->col ){
        lua_pushnil( L );
        lua_pushstring( L, LGRN_ENOCOLUMN );
        return 2;
    }
    else
    {
        grn_obj_flags flags = c->col->header.flags;
        
        switch( c->col->header.flags & GRN_OBJ_COLUMN_TYPE_MASK ){
            case GRN_OBJ_COLUMN_VECTOR:
            case GRN_OBJ_COLUMN_INDEX:
                lua_pushboolean( L, flags & GRN_OBJ_WITH_WEIGHT );
            break;
            
            default:
                lua_pushboolean( L, 0 );
        }
        
        return 1;
    }
}


static int with_section_lua( lua_State *L )
{
    lgrn_col_t *c = luaL_checkudata( L, 1, MODULE_MT );
    
    if( !c->col ){
        lua_pushnil( L );
        lua_pushstring( L, LGRN_ENOCOLUMN );
        return 2;
    }
    else
    {
        grn_obj_flags flags = c->col->header.flags;
        
        switch( c->col->header.flags & GRN_OBJ_COLUMN_TYPE_MASK ){
            case GRN_OBJ_COLUMN_INDEX:
                lua_pushboolean( L, flags & GRN_OBJ_WITH_SECTION );
            break;
            
            default:
                lua_pushboolean( L, 0 );
        }
        
        return 1;
    }
}


static int with_position_lua( lua_State *L )
{
    lgrn_col_t *c = luaL_checkudata( L, 1, MODULE_MT );
    
    if( !c->col ){
        lua_pushnil( L );
        lua_pushstring( L, LGRN_ENOCOLUMN );
        return 2;
    }
    else
    {
        grn_obj_flags flags = c->col->header.flags;
        
        switch( c->col->header.flags & GRN_OBJ_COLUMN_TYPE_MASK ){
            case GRN_OBJ_COLUMN_INDEX:
                lua_pushboolean( L, flags & GRN_OBJ_WITH_POSITION );
            break;
            
            default:
                lua_pushboolean( L, 0 );
        }
        
        return 1;
    }
}


static int tostring_lua( lua_State *L )
{
    return lgrn_tostring( L, MODULE_MT );
}


static int gc_lua( lua_State *L )
{
    lgrn_col_t *c = lua_touserdata( L, 1 );
    
    if( c->col )
    {
        if( c->removed ){
            grn_obj_remove( lgrn_get_ctx( c->t->g ), c->col );
        }
        else {
            grn_obj_unlink( lgrn_get_ctx( c->t->g ), c->col );
        }
    }
    // release reference
    lstate_unref( L, c->ref_t );
    
    return 0;
}


LUALIB_API int luaopen_groonga_column( lua_State *L )
{
    struct luaL_Reg mmethods[] = {
        { "__gc", gc_lua },
        { "__tostring", tostring_lua },
        { NULL, NULL }
    };
    struct luaL_Reg methods[] = {
        { "name", name_lua },
        { "path", path_lua },
        { "type", type_lua },
        { "valType", val_type_lua },
        { "compress", compress_lua },
        { "persistent", persistent_lua },
        { "withWeight", with_weight_lua },
        { "withSection", with_section_lua },
        { "withPosition", with_position_lua },
        { NULL, NULL }
    };
    
    lgrn_register_mt( L, MODULE_MT, mmethods, methods );
    
    return 0;
}

