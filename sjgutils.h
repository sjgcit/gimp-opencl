
/*
 * sjgutils.h
 *
 * $Id: sjgutils.h,v 1.3 2014/11/05 14:46:49 sjg Exp $
 *
 * (c) Stephen Geary, Sep 2014
 *
 * Utility code
 */


#ifndef __SJGUTILS_H
#define __SJGUTILS_H


#include <ctype.h>

#include <stdint.h>

#define FCWT_ERROR -2

#define FCWT_OLDER -1
#define FCWT_SAME   0
#define FCWT_NEWER  1


#if ( ! defined(TRUE) ) || ( ! defined(FALSE) )
#  ifdef TRUE
#    undef TRUE
#  endif
#  ifdef FALSE
#    undef FALSE
#  endif
#  define TRUE 1
#  define FALSE 0
#endif


#define FLAGSET(v,f)    ( ( (v) & (f) ) == (f) )


extern void sleep_for_ms( unsigned int milliseconds ) ;


#define compare_file_write_times(p1,p2)     compare_file_write_times_real((char *)(p1),(char *)(p2))

extern int compare_file_write_times_real( char *path1, char *path2 ) ;


#ifndef MAXPATHLEN
#  define MAXPATHLEN 1024
#endif


#define g_file_exists(fname)    g_file_test((gchar *)(fname), G_FILE_TEST_EXISTS )


// macro for checking characters

#define iseol(c)        ( ( (c) == 0 ) || ( (c) == '\n' ) )

#define isnoteol(c)     ( ( (c) != 0 ) && ( (c) != '\n' ) )


#define isliteral(c)    ( isalnum((int)(c)) || ( (c) == '_' ) )


#define skipspace(p)    if( (p) != NULL ) \
                        { \
                            while( isnoteol(*(p)) && isblank(*(p)) ) \
                            { \
                                (p)++ ; \
                            }; \
                        }

#define sjg_strncasecmp(p1,p2,n)  sjg_strncasecmp_real( (guchar *)(p1), (guchar *)(p2), (guint)(n) )

extern int sjg_strncasecmp_real( guchar *p1, guchar *p2, guint n ) ;

/* Singly linked list helper macros
 */

#define slist_add(root,node) \
    if( (node) != NULL ) \
    { \
        (node)->next = (root) ; \
        (root) = (node) ; \
    }


#define slist_addlast(root,node,tmpptr) \
    if( (root) == NULL ) \
    { \
        (root) = (node) ; \
    } \
    else if( (node) != NULL ) \
    { \
        (tmpptr) = (root) ; \
        while( (tmpptr)->next != NULL ) \
        { \
            (tmpptr) = (tmpptr)->next ; \
        } ; \
        (tmpptr)->next = (node) ; \
    } \
    (node)->next = NULL ;


#define slist_remove(root,node,tmpptr) \
    if( ( (root) != NULL ) && ( (node) != NULL ) ) \
    { \
        if( (root) == (node) ) \
        { \
            (root) = (node)->next ; \
        } \
        else \
        { \
            (tmpptr) = (root) ; \
            while( (tmpptr)->next != NULL ) \
            { \
                if( (tmpptr)->next == (node) ) \
                { \
                    break ; \
                } \
                (tmpptr) = (tmpptr)->next ; \
            }; \
            if( (tmpptr)->next == (node) ) \
            { \
                (tmpptr)->next = (node)->next ; \
            } \
        } \
    }


/* doubly linked list helper macros
 *
 * Requires a node type that has a 'next' and 'prev'
 * fields are the pointers to the next and previous nodes.
 */

#define dlist_add(root,node) \
    if( (node) != NULL ) \
    { \
        (node)->next = (root) ; \
        (node)->prev = NULL ; \
        (root)->prev = (node) ; \
        (root) = (node) ; \
    }

#define dlist_remove(root,node,tmpptr) \
    if( ( (root) != NULL ) && ( (node) != NULL ) ) \
    { \
        if( (root) == (node) ) \
        { \
            (root) = (node)->next ; \
            if( (node)->next != NULL ) \
            { \
                (node)->next->prev = NULL ; \
            } \
        } \
        else \
        { \
            (node)->prev->next = (node)->next ; \
            if( (node)->next != NULL ) \
            { \
                (node)->next->prev = (node)->prev ; \
            } \
        } \
    }


/* Safe alligned version of alloca() ( insofar as alloca() is safe )
 *
 * Assumes alignment is a power of two
 *
 * May be useful to ensure tmp stores for pointers are aligned correctly.
 */

#define aligned_alloca(need,alignment) \
        (void *)( (uintptr_t)( alloca( (need)+(alignment) ) ) & ~((uintptr_t)(alignment)-(uintptr_t)1) )



#endif /* __SJGUTILS_H */



