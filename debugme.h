
/*
 * debugme.h
 *
 * $Id: debugme.h,v 1.5 2015/11/18 19:53:09 sjg Exp $
 *
 * (c) Stephen Geary, Sep 2014
 *
 * Debugging code for use with glib.h
 */

#ifndef __DEBUGME_H
#define __DEBUGME_H

#include <stdio.h>

#include <glib.h>

/*******************************************************************
 *
 * Debugging report lines
 *
 *******************************************************************
 */


#ifndef DEBUGME
#  define debug_init_log( fname )
#  define debug_finish_log()
#else /* DEBUGME */

extern void debug_init_log_real( guchar *fname ) ;
extern void debug_finish_log_real() ;

#  define debug_init_log( fname )   debug_init_log_real( (fname) )
#  define debug_finish_log()        debug_finish_log_real()
#endif /* DEBUGME */


#ifndef DEBUGME

#  define SJGDEBUGHEAD(l,f)
#  define SJGDEBUG()
#  define SJGDEBUGMSG(msg)
#  define SJGDEBUGF(...)
#  define SJGDEBUGF_REAL(lineno, func, ...)

#else

#  define SJGDEBUGHEAD(lineno,func)    fprintf(stderr,"SJG :: %d :: %s :: ", (lineno), (func) )

#  define SJGDEBUG()        SJGDEBUGMSG("")

#  define SJGDEBUGMSG(msg)  { SJGDEBUGHEAD(__LINE__,__func__) ; fprintf( stderr,"%s\n", (msg) ) ; fflush( stderr ) ; }

#  define SJGDEBUGF_REAL(lineno,func , ...)  { \
                                SJGDEBUGHEAD((lineno),(func)) ; \
                                fprintf( stderr, __VA_ARGS__ ) ; \
                                fprintf( stderr, "\n" ) ; \
                                fflush( stderr ) ; \
                            }

#  define SJGDEBUGF(...)    SJGDEBUGF_REAL(__LINE__, __func__, __VA_ARGS__)

#endif


/* An aid to debugging code
 */

#ifdef DEBUGME
#  define SJGEXEC( stmt )   { SJGDEBUGF( "Executing :\n%s\n", #stmt ) ; stmt }
#else
#  define SJGEXEC( stmt )   stmt
#endif


/*******************************************************************
 *
 * Memory allocation support code
 *
 *******************************************************************
 */

#ifdef DEBUGME

  /*
   * The DEBUGME code includes support for monitoring
   * for allocated memory blocks that have guard bytes
   * which should not change unless the code is buggy
   * and exceeds the boundaries of the allocation.
   *
   * The four guard bytes at the start are in the header
   * which is prepended onto the area alloacted to the user.
   *
   * The four guard bytes at the end are appended to the
   * the user area.
   */

  struct memheader_s ;
  
  struct memheader_dummy_s {
        struct  memheader_s *next ;
        struct  memheader_s *prev ;
        gpointer userptr ;
        gsize    usersize ;
        gint     allocline ;
        guchar   marker[4] ;
  } ;

  struct memheader_s {
        struct  memheader_s *next ;
        struct  memheader_s *prev ;
        gpointer userptr ;
        gsize    usersize ;
        gint     allocline ;
        guchar   padding[ 128-sizeof(struct memheader_dummy_s) ] ;
        guchar   marker[4] ;
    } ;
  
  typedef struct memheader_s memheader_t ;
  
#  define MARKER0 0xff
#  define MARKER1 0x88
#  define MARKER2 MARKER0
#  define MARKER3 MARKER1


#  define invalid_memmarker( mb ) \
    (    ( (mb)[0] != MARKER0 ) \
      || ( (mb)[1] != MARKER1 ) \
      || ( (mb)[2] != MARKER2 ) \
      || ( (mb)[3] != MARKER3 ) \
    )

#  define  check_memmarkers()   check_memmarkers_real( __LINE__ )

  extern void check_memmarkers_real( gint line ) ;


#  define g_free_dbg(ptr)   g_free_dbg_real( (ptr), __LINE__, __func__ )

  extern void g_free_dbg_real( gpointer ptr, int line, const char *func ) ;

#  define g_malloc_dbg(sz)  g_try_malloc_dbg( (sz), __LINE__, __func__ )

  extern gpointer g_try_malloc_dbg( gsize nb, gint line, const char *func ) ;

#else

#  define g_free_dbg(ptr)   g_free((ptr))

#  define  check_memmarkers()

#  define g_malloc_dbg(sz)  g_try_malloc((sz))

#endif /* DEBUGME */

#define g_free_safe(ptr)    \
                            if( (ptr) != NULL ) \
                            { \
                                g_free_dbg( (ptr) ) ; \
                                \
                                (ptr) = NULL ; \
                            }

#endif /* __DEBUGME_H */

