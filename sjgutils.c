
/*
 * sjgutils.c
 *
 * $Id: sjgutils.c,v 1.2 2014/11/05 07:48:03 sjg Exp $
 *
 * (c) Stephen Geary, Sep 2014
 *
 * Utility code
 */

#include <glib.h>


#include "sjgutils.h"

#include "debugme.h"


#ifdef G_OS_UNIX

#  include <sys/types.h>
#  include <sys/stat.h>
#  include <unistd.h>
#  include <time.h>

#endif


#ifdef G_OS_WIN32

#  include <windows.h>

#endif


/****************************************************************************************
 */

/* Sleep for a given number of milliseconds
 * Useful if you want to busywait()
 */

void sleep_for_ms( unsigned int milliseconds )
{
#ifdef G_OS_UNIX
    struct timespec t ;
    
    t.tv_sec = 0 ;
    
    t.tv_nsec = 1000000 * milliseconds ;

    nanosleep( &t, NULL ) ;
#endif /* G_OS_UNIX */

#ifdef G_OS_WIN32
    Sleep( milliseconds ) ;
#endif /* G_OS_WIN32 */
}

/****************************************************************************************
 */

/* Annoyingly glib has decrepcated g_strncasecmp() which is a useful
 * function in a code parser.
 */

int sjg_strncasecmp_real( guchar *p1, guchar *p2, guint n )
{
    int retv = 0 ;
    
    int j = 0 ;
    
    guchar c1 ;
    guchar c2 ;
    
    while( ( j < n ) && isnoteol(p1[j]) && isnoteol(p2[j]) )
    {
        c1 = tolower(p1[j]) ;
        c2 = tolower(p2[j]) ;
        
        if( c1 > c2 )
        {
            return 1 ;
        }
        else if( c1 < c2 )
        {
            return -1 ;
        }
        
        j++ ;
    };
    
    if( j == n )
    {
        return 0 ;
    }
    
    return retv ;
}

/****************************************************************************************
 */


/* This operation has no convenient call in glib or the stanard C libraries
 * and even getting the file write times is platform specific
 */

int compare_file_write_times_real( char *path1, char *path2 )
{
    int retval = FCWT_ERROR ;
    
    if( ( path1 == NULL ) || ( path2 == NULL ) )
    {
        goto err_ret ;
    }
    
    SJGDEBUGF( "Comparing file times for :\n\t\t%s\nand\t\t%s", path1, path2 ) ;

#ifdef G_OS_UNIX

    /**************
     *
     * UNIX version
     */

    struct stat sb1, sb2 ;
    
    if( stat( path1, &sb1 ) != 0 )
    {
        goto err_ret ;
    }
    
    if( stat( path2, &sb2 ) != 0 )
    {
        goto err_ret ;
    }
    
    time_t t1, t2 ;
    
    t1 = sb1.st_mtime ;
    t2 = sb2.st_mtime ;
    
    SJGDEBUGF( "t1 = %ld   t2 = %ld", t1, t2 ) ;
    
    if( t1 < t2 )
    {
        return FCWT_OLDER ;
    }
    
    if( t1 == t2 )
    {
        return FCWT_SAME ;
    }
    
    if( t1 > t2 )
    {
        return FCWT_NEWER ;
    }

#endif /* G_OS_UNIX */


#ifdef G_OS_WIN32

    /**********************
     *
     * MS Windows version
     */
    
    HANDLE h1, h2 ;
    FILETIME ft1, ft2 ;
    
    h1 = CreateFile( path1, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL ) ;
    
    if( h1 == INVALID_HANDLE_VALUE )
    {
        goto err_ret ;
    }
    
    h2 = CreateFile( path2, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL ) ;
    
    if( h2 == INVALID_HANDLE_VALUE )
    {
        CloseHandle( h1 ) ;
    
        goto err_ret ;
    }
    
    if( GetFileTime( h!, NULL, NULL, &ft1 ) )
    {
        if( GetFileTime( h2, NULL, NULL, &ft2 ) )
        {
            // now have file time, so simply compare them
            
            int t ;
            
            t = CompareFileTimes( &ft1, &ft2 ) ;
            
            if( t == -1 )
            {
                retval = FCWT_OLDER ;
            }
            
            if( t == 0 )
            {
                retval = FCWT_SAME ;
            }
            
            if( t == 1 )
            {
                retval = FCWT_NEWER ;
            }
        }
    }
    
    CloseHandle( h2 ) ;
    CloseHandle( h1 ) ;
    
    return retval ;
    
#endif /* G_OS_WIN32 */


err_ret:

    SJGDEBUGF( "Returning error." ) ;

    return retval ;
}





