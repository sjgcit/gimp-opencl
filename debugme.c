
/*
 * debugme.c
 *
 * $Id: debugme.c,v 1.1 2014/11/05 04:19:15 sjg Exp $
 *
 * (c) Stephen Geary, Sep 2014
 *
 * Debugging code for use with glib.h
 */

#ifdef DEBUGME

/* This code won't compile unless DEBUGME is defined !
 *
 * The functions are only needed if DEBUGME is defined
 * as they are only referred to in macros defined in
 * debugme.h which are empty unless DEBUGME is defined.
 */

#include <stdio.h>
#include <time.h>

#include <glib.h>

#include "sjgutils.h"

#include "debugme.h"



/* We redirect stderr, but if we don't restore it the plugin will
 * crash spectacularly after the close fout ( which is what we redirect
 * stderr to ).  So we must save the old stream and restore it.
 */


static FILE *savedstderr = NULL ;
static FILE *savedstdout = NULL ;

static FILE *debuglog = NULL ;

void debug_init_log_real( guchar *name )
{
    if( name == NULL )
    {
        return ;
    }

    debuglog = fopen( (char *)name, "w" ) ;
    
    if( debuglog != NULL )
    {
        savedstderr = stderr ;
        savedstdout = stdout ;
    
        stderr = debuglog ;
        stdout = debuglog ;
    }

    time_t starttime ;
    
    starttime = time(NULL) ;
    
    SJGDEBUGF( "Log started at %s", asctime(localtime(&starttime)) ) ;
}


void debug_finish_log_real()
{
    time_t starttime ;
    
    starttime = time(NULL) ;
    
    SJGDEBUGF( "Log finished at %s", asctime(localtime(&starttime)) ) ;

    if( ( debuglog != NULL ) && ( debuglog != savedstderr ) )
    {
        fflush( debuglog ) ;
        
        fclose( debuglog ) ;
        
        stderr = savedstderr ;
        stdout = savedstdout ;
    }
}


/********************************************************************
 *
 * Master list of all allocations
 */

static memheader_t *memmarkerlist = NULL ;
  


/********************************************************************
 *
 * Used to check all the guard markers we are tracking for error
 */

void check_memmarkers_real( gint line )
{
    memheader_t *mp = NULL ;
    
    mp = memmarkerlist ;
    
    while( mp != NULL )
    {
        if( invalid_memmarker( mp->marker ) )
        {
            SJGDEBUGF( "At line %d detected currupted pre marker in %p block allocated at line %d.", line, mp->userptr, mp->allocline ) ;
        }
        
        if( invalid_memmarker( ((guchar *)(mp->userptr)) + mp->usersize ) )
        {
            SJGDEBUGF( "At line %d detected currupted post marker in %p block allocated at line %d.", line, mp->userptr, mp->allocline ) ;
        }
        
        mp = mp->next ;
    };
}

/********************************************************************
 *
 * Used to release memory
 *
 * Designed to also free memory not allocated through this routine
 */


void g_free_dbg_real( gpointer ptr, int line, const char *func )
{
    gchar *p ;
    
    p = (gchar *)ptr - sizeof(memheader_t) ;
  
    SJGDEBUGF_REAL( line, func, "Freeing memory at user ptr %p", (void *)ptr ) ;
    
    // was this pointer allocated in our list of marked memory
    
    memheader_t *mp = memmarkerlist ;
    
    while( mp != NULL )
    {
        if( mp->userptr == (gchar *)ptr )
        {
            break ;
        }
        
        mp = mp->next ;
    };
    
    if( ( mp == NULL ) || ( mp->userptr != (char *)ptr ) )
    {
        // not in our list - just free and return
        
        SJGDEBUGF_REAL( line, func, "Freeing memory not explicitly allocated at %p.", ptr ) ;
        
        g_free( ptr ) ;
        
        return ;
    }
    
    // got here then mp is the required memheader_t pointer
    
    // check the marker bytes
    
    if( invalid_memmarker( mp->marker ) )
    {
        SJGDEBUGF_REAL( line, func, "Memory block %p at line %d detected currupted pre marker %hhx %hhx %hhx %hhx.",
                     ptr, line, mp->marker[0] ,mp->marker[1], mp->marker[2], mp->marker[3] ) ;
        
        SJGDEBUGF_REAL( line, func, "\t\tMarker should be %hhx %hhx %hhx %hhx", MARKER0, MARKER1, MARKER2, MARKER3 ) ;
    }

    guchar *mb ;
    
    mb = ((guchar *)ptr) + mp->usersize ;
    
    if( invalid_memmarker( mb ) )
    {
        SJGDEBUGF_REAL( line, func, "Memory block %p at line %d detected currupted pre marker %hhx %hhx %hhx %hhx.",
                     ptr, line, mb[0] ,mb[1], mb[2], mb[3] ) ;
        
        SJGDEBUGF_REAL( line, func, "\t\tMarker should be %hhx %hhx %hhx %hhx", MARKER0, MARKER1, MARKER2, MARKER3 ) ;
    }
    
    // remove this memheader from the list ;
    
    if( mp->prev != NULL )
    {
        mp->prev->next = mp->next ;
    }
    
    if( mp->next != NULL )
    {
        mp->next->prev = mp->prev ;
    }
    
    if( memmarkerlist == mp )
    {
        memmarkerlist = mp->next ;
    }

    // now actually free the memory

    g_free( (gpointer)p ) ;
}

/********************************************************************
 *
 *
 */

gpointer g_try_malloc_dbg( gsize nb, gint line, const char *func )
{
    gchar *p = NULL ;
    
    // alocate enough space for a post and pre marker int

    p = (char *)g_try_malloc( nb + 4 + sizeof(memheader_t) ) ;

    SJGDEBUGF_REAL( line, func, "Allocated memory at %p", (void *)(p+sizeof(memheader_t)) ) ;
    
    memheader_t *mp = (memheader_t *)p ;

    mp->prev = NULL ;
    
    mp->next = memmarkerlist ;
    memmarkerlist = mp ;
    
    if( mp->next != NULL )
    {
        mp->next->prev = mp ;
    }

    mp->userptr  = p + sizeof(memheader_t) ;
    mp->usersize = nb ;
    
    mp->allocline = line ;
    
    mp->marker[0] = MARKER0 ;
    mp->marker[1] = MARKER1 ;
    mp->marker[2] = MARKER2 ;
    mp->marker[3] = MARKER3 ;
    
    gchar *mb = (gchar *)( p + sizeof(memheader_t) + nb ) ;
    
    mb[0] = MARKER0 ;
    mb[1] = MARKER1 ;
    mb[2] = MARKER2 ;
    mb[3] = MARKER3 ;
    
    return (gpointer)( p + sizeof(memheader_t) ) ;
}


#endif /* DEBUGME */

