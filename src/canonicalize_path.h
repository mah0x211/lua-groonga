/*
 *  Copyright 2015 Masatoshi Teruya. All rights reserved.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *  THE SOFTWARE.
 *
 *
 *  canonicalize_path.h
 *
 *  Created by Masatoshi Teruya on 2015/02/20.
 *
 */

#ifndef CANONICALIZE_PATH_H
#define CANONICALIZE_PATH_H

#include <unistd.h>
#include <stdlib.h>
#include <string.h>

static char *canonicalize_path( const char *pathname, int *relative )
{
    size_t len = strlen( pathname );
    char *rpath = NULL;
    char *ptr = NULL;
    char *prev = NULL;
    char c = 0;
    
    // prepend slash if pathname is relative-path
    if( ( *relative = pathname[0] != '/' ) )
    {
        len += 1;
        if( !( rpath = malloc( len + 2 ) ) ){
            return NULL;
        }
        *rpath = '/';
        memcpy( rpath + 1, pathname, len );
        rpath[len] = 0;
        *relative = 1;
    }
    else if( !( rpath = strndup( pathname, len ) ) ){
        return NULL;
    }
    
    ptr = rpath;
    
    // preprocessing to reduce conditional branch in while loop
    if( *ptr == '.' && ( !c || c == '/' ) )
    {
        // pathname: ./bar
        if( ptr[1] == '/' ){
            // pathname: ./bar -> /bar
            memcpy( rpath, ptr + 1, len );
            prev = rpath;
            ptr = prev;
        }
        // pathname: ../bar
        else if( ptr[1] == '.' && ptr[2] == '/' ){
            // pathname: ../bar -> /bar
            memcpy( rpath, ptr + 2, len - 1 );
            prev = rpath;
            ptr = prev;
        }
        c = *ptr;
        ptr++;
    }
    
    while( *ptr )
    {
        if( *ptr == '/' )
        {
            prev = ptr;
            // skip multiple slashes
            if( ptr[1] == '/' )
            {
                while( *ptr == '/' ){
                    ptr++;
                }
                if( *ptr ){
                    memcpy( prev + 1, ptr, strlen( ptr ) + 1 );
                }
                else {
                    prev[1] = 0;
                }
                ptr = prev;
            }
        }
        else if( *ptr == '.' && ( !c || c == '/' ) )
        {
            // pathname: ./bar
            if( ptr[1] == '/' ){
                // pathname: /foo/./bar -> /foo/bar
                memcpy( prev + 1, ptr + 2, strlen( ptr + 2 ) + 1 );
                ptr = prev;
            }
            // pathname: ../bar
            else if( ptr[1] == '.' && ptr[2] == '/' )
            {
                // pathname: /foo/../bar -> /bar
                while( prev != rpath )
                {
                    prev--;
                    if( *prev == '/' ){
                        break;
                    }
                }
                memcpy( prev + 1, ptr + 3, strlen( ptr + 3 ) + 1 );
                ptr = prev;
            }
        }
        c = *ptr;
        ptr++;
    }
    
    return rpath;
}


#endif
