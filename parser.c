
/*
 * Routines for scanning a source file's comments
 *
 * $Id: parser.c,v 1.2 2014/11/05 07:48:03 sjg Exp $
 *
 * (c) Stephen Geary, Oct 2014
 */

#include <string.h>

#include <stdio.h>

#include <ctype.h>

#include <glib.h>

#include "sjgutils.h"

#include "debugme.h"

#include "parser.h"


/****************************************************************
 */


struct parser_handle_s {
          guchar     *filecopy ;
          gsize      filelen ;
          int       *indexarray ;
          int        indexlen ;
          } ;
          
typedef struct parser_handle_s parser_handle_t ;


/****************************************************************
 */

/* return the pointer to the start of the line index given
 */

guchar *parser_ptrtoindex( void *handlep, int idx )
{
    if( handlep == NULL )
    {
        return NULL;
    }
    
    parser_handle_t *p = (parser_handle_t *)handlep ;
    
    if( ( p->filecopy == NULL ) || ( p->indexarray == NULL ) || ( p->indexlen < idx ) )
    {
        return NULL ;
    }
    
    return ( (p->filecopy) + ((p->indexarray)[idx]) ) ;
}

/****************************************************************
 */

/* returns >= 0 if "big" starts with "small"
 * Value returned is number of characters in big used to make match.
 * negative return is NO match
 *
 * uses BOTH nuls and newlines as termination characters
 * as the parser has to deal with BOTH possibilies.
 *
 * Character comparisons are case independent as this
 * is designed for keywords.
 *
 * a space in "small" is treated as ANY whitespace in "big"
 */

int parser_starts_with_real( guchar *big, guchar *small )
{
    if( ( big == NULL ) || ( small == NULL ) )
    {
        SJGDEBUG() ;
    
        return -1 ;
    }

    int retv = -1 ;

    guchar *b = big ;
    guchar *s = small ;
    
    while( isnoteol(*s) && isnoteol(*b) )
    {
        if( isblank(*s) )
        {
            // check for any white space matching in "big"
            // meaning any space or tab character
            
            if( ! isblank(*b) )
            {
                break ;
            }
            
            b++ ;
            
            while( isnoteol(*b) && isblank(*b) )
            {
                b++ ;
            };
            
            if( iseol(*b) )
            {
                break ;
            }
            
            b-- ;
        }
        else
        {
            if( toupper(*b) != toupper(*s) )
            {
                break ;
            }
        }
    
        b++ ;
        s++ ;
    };
    
    // match ONLY if *s = a nul or newline
    
    if( iseol(*s) )
    {
        // SJGDEBUGF( "b   = [%p]", b ) ;
        // SJGDEBUGF( "big = [%p]", big ) ;
    
        retv = (int)( (guchar *)b - big ) ;
        
        // SJGDEBUGF( "retv = [ %d ]", retv ) ;
    }
    
    return retv ;
}


/****************************************************************
 */

int parser_get_filelen( void *handle )
{
    if( handle == NULL )
    {
        return -1 ;
    }
    
    return ((parser_handle_t *)handle)->filelen ;
}

/****************************************************************
 */

guchar *parser_get_filecopy( void *handle )
{
    if( handle == NULL )
    {
        return NULL;
    }
    
    return ((parser_handle_t *)handle)->filecopy ;
}

/****************************************************************
 */

int *parser_get_indexarray( void *handle )
{
    if( handle == NULL )
    {
        return NULL ;
    }
    
    return ((parser_handle_t *)handle)->indexarray ;
}

/****************************************************************
 */

int parser_get_indexlen( void *handle )
{
    if( handle == NULL )
    {
        return -1 ;
    }
    
    return ((parser_handle_t *)handle)->indexlen ;
}

/****************************************************************
 */

/* open a file and return a handle to it's in-memory copy
 *
 * return NULL for any error.
 */

void *parser_loadfile( guchar *fname )
{
    if( fname == NULL )
    {
        return NULL ;
    }
    
    guchar *filecopy = NULL ;
    
    gsize filelen = 0 ;
    
    if( g_file_get_contents( (char *)fname, (gchar **)(&filecopy), &filelen, NULL ) == FALSE )
    {
        return NULL ;
    }
    
    parser_handle_t *handle = NULL ;
    
    handle = (parser_handle_t *)g_malloc_dbg( sizeof(parser_handle_t) ) ;
    
    if( handle == NULL )
    {
        g_free( filecopy ) ;
        
        return NULL ;
    }
    
    handle->filecopy    = filecopy ;
    
    handle->filelen     = filelen ;
    
    handle->indexarray  = NULL ;
    
    handle->indexlen = 0 ;
    
    return (void *)handle ;
}

/****************************************************************
 */

/* close an in-memory copy of a source file
 */

void parser_closefile( void *handle )
{
    if( handle == NULL )
    {
        return ;
    }
    
    parser_handle_t *hp = (parser_handle_t *)handle ;

    g_free_safe( hp->filecopy ) ;
    
    g_free_safe( hp->indexarray ) ;
    
    g_free_safe( hp ) ;
}


/****************************************************************
 */

/* return the number of lines found
 *
 * The index array can be accessed by using parser_get_indexarray(...)
 *
 * It contains a list of offsets in the in memory copy to
 * the start of each line.
 */

#define PARSER_DEFAULT_PREFIX "// "

#define PARSER_MAX_PREFIX_LEN 64

int parser_build_indexarray( void *handlep, guchar *useprefix )
{
    if( handlep == NULL )
    {
        // SJGDEBUG() ;
    
        return -1 ;
    }
    
    parser_handle_t *hp = (parser_handle_t *)handlep ;

    int retv = 0 ;
    
    guchar prefix[PARSER_MAX_PREFIX_LEN] ;
    
    if( useprefix != NULL )
    {
        int j = 0 ;
        
        while( ( j < PARSER_MAX_PREFIX_LEN-1 ) && isnoteol( useprefix[j] ) )
        {
            prefix[j] = toupper( useprefix[j] ) ;
        
            j++ ;
        };
        
        prefix[j] = 0 ;
    }
    else
    {
        strcpy( (char *)prefix, PARSER_DEFAULT_PREFIX ) ;
    }
    
    guchar *p = hp->filecopy ;
    
    gsize filelen = hp->filelen ;
    
    // SJGDEBUGF( "filelen = [%d]", (int)filelen ) ;
    
    int i = 0 ;

    int j = 0 ;
    
    while ( i < filelen )
    {
        j = parser_starts_with( p+i, prefix ) ;
        
        // SJGDEBUGF( "prefix check returned j = [%d]", j ) ;
        
        if( j >= 0 )
        {
            // prefix found
            
            retv++ ;
            
            i += j ;
        }
    
        // scan until EOL or EOF
        
        while( ( i < filelen ) && isnoteol( p[i] ) )
        {
            i++ ;
        };
        
        // This inc pushes us onto the next char after '\n' or past EOF
        
        i++ ;
    };
    
    // now have a count of indicies
    
    // SJGDEBUGF( "retv = [ %d ]", retv ) ;
    
    if( hp->indexarray != NULL )
    {
        // already have an index array
        
        // free it
        
        g_free_safe( hp->indexarray ) ;
        
        hp->indexlen = 0 ;
    }
    
    hp->indexarray = (int *)g_malloc_dbg( retv * sizeof(int) ) ;
    
    if( hp->indexarray == NULL )
    {
        return -1 ;
    }
    
    // now scan again to record the found indicies
    
    p = hp->filecopy ;
    
    retv = 0 ;
    
    i = 0 ;
    
    while ( i < filelen )
    {
        j = parser_starts_with( p+i, prefix ) ;
        
        if( j >= 0 )
        {
            // prefix found
            
            (hp->indexarray)[retv] = i ;
            
            retv++ ;
            
            i += j ;
        }
    
        // scan until EOL or EOF
        
        while( ( i < filelen ) && isnoteol( p[i] ) )
        {
            i++ ;
        };
        
        // This inc pushes us onto the next char after '\n' or past EOF
        
        i++ ;
    };
    
    // SJGDEBUGF( "retv = [ %d ]", retv ) ;
    
    hp->indexlen = retv ;
    
    return retv ;
}


/****************************************************************
 */

/* read a floating point number from string pointed to
 * returning the number of characters read or a negative
 * value on error or no number found.
 */

int parser_readfp( guchar *ptr )
{
    int retv = -1 ;
    
    guchar *p = ptr ;
    
    if( *p == '-' )
    {
        retv = 1 ;
        
        p++ ;
    }
    
    if( ! isdigit(*p) )
    {
        return -1 ;
    }
    
    int dots = 0 ;
    
    while( isnoteol(*p) && ( dots < 2 ) && ( isdigit(*p) || ( *p == '.' ) ) )
    {
        if( *p == '.' )
        {
            dots++ ;
        }
        
        p++ ;
        
        retv++ ;
    };
    
    if( ( dots > 1 ) || isnoteol(*p) )
    {
        return -1 ;
    }
    
    retv = (int)( p - ptr ) ;
    
    return retv ;
}

/****************************************************************
 */




