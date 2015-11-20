
/*
 * Gimp plugin for OpenCL support
 *
 * (c) Stephen Geary, Dec 2013
 *
 * $Id: sjgopencl.c,v 1.44 2015/11/18 23:55:00 sjg Exp $
 *
 * A major rewrite from generiffilter.c v 1.221 to add support
 * for more scripted control of multiple kernel processes.
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <glib.h>
#include <glib/gstdio.h>

#include <gtk-2.0/gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif


#include "sjg_messages.h"

#include "simpleCL.h"


#include "sjgutils.h"

#include "debugme.h"

#include "parser.h"

#include "expr.h"

/***********************************************************************************
 */


#define PLUG_IN_NAME "sjg-opencl"

#define PLUG_IN_AUTHOR "Stephen Geary"


// Union used in passing parameters to kernel

union uf_u {
    int i ;
    float f ;
    } ;

typedef union uf_u uf_t ;

/***********************************************************************************
 */

#define CHECKPTR(ptr)   if( (ptr) == NULL ){ SJGDEBUG() ; goto err_exit ; }

/***********************************************************************************
 */

// convenience macro for filling file paths in gimp_directory subdirs

#define make_gimp_filename( buffer, subdir, fname ) \
    if( (buffer) != NULL ) \
    { \
        snprintf( (char *)((buffer)), MAXPATHLEN, "%s%c%s%c%s%c", \
                    gimp_directory(), \
                    G_DIR_SEPARATOR, \
                    (subdir), \
                    G_DIR_SEPARATOR, \
                    (fname), \
                    0 \
            ) ; \
    }


/***********************************************************************************
 */


int width    = 0 ;
int height   = 0 ;
int channels = 0 ;


/***********************************************************************************
 */

/* return rest of line with matching key in buffer
 * 
 * return FALSE if not found
 */

#define check_for_firstentry(h,p,b,l) check_for_firstentry_real((h),(guchar *)(p),(b),(l))

static int check_for_firstentry_real( void *handlep, guchar *prefix, guchar *buffer, int buflen )
{
    // check for FIRST entry in the parser file matching "<prefix> <something>EOL"
    
    int retv = FALSE ;
    
    if( ( handlep == NULL ) && ( prefix != NULL ) )
    {
        SJGDEBUG() ;
    
        return FALSE ;
    }
    
    if( ( buffer != NULL ) && ( buflen > 0 ) )
    {
        buffer[0] = 0 ;
    }

    int *idxp = NULL ;
    
    int idxcount = 0 ;
    
    idxp = parser_get_indexarray( handlep ) ;
    
    if( idxp == NULL )
    {
        SJGDEBUG() ;
        
        return FALSE ;
    }
    
    idxcount = parser_get_indexlen( handlep ) ;
    
    if( idxcount == 0 )
    {
        SJGDEBUG() ;
        
        return FALSE ;
    }
    
    // have a useful index array
    
    guchar *filecopy ;
    
    filecopy = parser_get_filecopy( handlep ) ;
    
    guchar *p ;
    
    int j = 0 ;
    int i = 0 ;
    
    while( j < idxcount )
    {
        p = filecopy + idxp[j] ;
    
        i = parser_starts_with( p, prefix ) ;
        
        if( i > 0 )
        {
            // found a match to the prefix
        
            retv = TRUE ;
        
            p += i ;
            
            i = 0 ;
            
            while( ( i < buflen-1 ) && isnoteol(p[i]) )
            {
                buffer[i] = p[i] ;
                
                i++ ;
            };
            
            // terminating nul
            
            buffer[i] = 0 ;
            
            // as we have a first match we can stop looking
            
            break ;
        }
        
        j++ ;
    };
    
    return retv ;
}

/***********************************************************************************
 */

// #define PARAMCOUNTMAX 64



/* scans a file for parameters names
 *
 * The file was loaded via parser_loadfile() and the
 * handlep we receive is used for referencing it.
 *
 * Also the indexarray was built previously.
 *
 * returns a count and set the array pointer to an 
 * allocated memory array of strings ( or NULL if count was zero )
 */


static int check_for_parameters( void *handlep, guchar ***paramnames, int *maxlenp )
{
    int count = 0 ;
    
    *paramnames = NULL ;
    
    if( handlep == NULL )
    {
        return 0 ;
    }

    // have the file in memory.  Now scan it for parameters.
    
    int *idxp = NULL ;
    
    int idxcount = 0 ;
    
    idxp = parser_get_indexarray( handlep ) ;
    
    if( idxp == NULL )
    {
        SJGDEBUG() ;
        
        return 0 ;
    }
    
    idxcount = parser_get_indexlen( handlep ) ;
    
    if( idxcount == 0 )
    {
        // cannot be any PARAM entries as their are no entries
    
        return 0 ;
    }
    
    SJGDEBUGF( "idxcount = [%d]", idxcount ) ;
    
    // have a list of candidate lines in the file
    //
    // now find the ones that match our needs
    
    /* Alloca() temporary char pointer arrays
     *
     * Cannot need more than idxcount entries
     */
    
    /*
    guchar *found[PARAMCOUNTMAX] ;
    guchar *values[PARAMCOUNTMAX] ;
     */

    /* The following replaces the above and assumes that
     * alloca() allocates an area aligned for pointers
     */
    guchar **found  = (guchar **)aligned_alloca( idxcount*sizeof(guchar *), sizeof(guchar *) ) ;
    guchar **values = (guchar **)aligned_alloca( idxcount*sizeof(guchar *), sizeof(guchar *) ) ;

    guchar *filecopy = NULL ;
    
    filecopy = parser_get_filecopy( handlep ) ;

    int i = 0 ;
    
    int j = 0 ;
    
    guchar *p = NULL ;
    
    count = 0 ;
    
    int maxlen = 0 ;
    int len = 0 ;
    
    while( i < idxcount )
    {
        p = filecopy + idxp[i] ;
        
        j = parser_starts_with( p, (guchar *)"// param " ) ;
        
        if( j >= 0 )
        {
            p += j ;
            
            if( isnoteol(*p) && isliteral(*p) )
            {
                found[count] = p ;
                
                len = 1 ;
                
                while( isnoteol(*p) && ! isblank(*p) )
                {
                    p++ ;
                    
                    len++ ;
                };
                
                if( isblank(*p) )
                {
                    // the expected space is here
                    
                    if( len > maxlen )
                    {
                        maxlen = len ;
                    }
                    
                    p++ ;
                    
                    while( isblank(*p) )
                    {
                        p++ ;
                    };
                    
                    len = parser_readfp( p ) ;
                    
                    if( len > 0 )
                    {
                        // did find a decimal floating point number
                        
                        if( len > maxlen )
                        {
                            maxlen = len ;
                        }
                        
                        values[count] = p ;
                        
                        // SJGDEBUGF( "found[%d]  = [%p]", count, found[count] ) ;
                        // SJGDEBUGF( "values[%d] = [%p]", count, values[count] ) ;
                        
                        p += j ;
                        
                        count++ ;
                    }
                }
            }
        }
        
        i++ ;
    };
    
    // leave room for a terminating nul
    
    maxlen++ ;
    
    // store maxlen
    
    *maxlenp = maxlen ;
    
    // now allocate an array ( if needed ) for the
    // parameters names array
    // note that we return BOTH the array of pointers
    // AND the names in a single allocated memory block
    //
    // The pointers are arranged as an array at the start
    // of the block and the names follow ( as another array )
    // with the default values they relate to paired.
    
    if( count == 0 )
    {
        *paramnames = NULL ;
        
        return 0 ;
    }

    int w = maxlen + 1 ;
        
    *paramnames = (guchar **)g_malloc_dbg( count*( ( 2*w ) + sizeof(gchar *) ) ) ;
    
    if( *paramnames == NULL )
    {
        // cannot build parameter list
        
        return 0 ;
    }
    
    // copy the param names into the file
    
    guchar *nameblock = ( (guchar *)(*paramnames) ) + ( count * sizeof(gchar *) ) ;
    
    guchar **pnames = *paramnames ;
    
    guchar *s = NULL ;
    guchar *d = NULL ;
    
    i = 0 ;
    
    while( i < count )
    {
        // SJGDEBUGF( "found[i] = [%p]", found[i] ) ;
        
        // store the pointer to the name
        pnames[i] = nameblock + ( 2 * i * w ) ;
        
        // SJGDEBUGF( "*paramnames[i] = [%p]", pnames[i] ) ;
        
        s = found[i] ;
        d = pnames[i] ;
        
        while( isnoteol(*s) && ! isblank(*s) )
        {
            *d = *s ;
            d++ ;
            s++ ;
        }
        
        *d = 0 ;
        
        // store the default value it relates to.
        
        s = values[i] ;
        d = pnames[i] + w ;
        
        while( isnoteol(*s) )
        {
            *d = *s ;
            d++ ;
            s++ ;
        }
        
        *d = 0 ;
        
        SJGDEBUGF( "copied to *paramnames[i] = [%s]", pnames[i] ) ;
        
        i++ ;
        
        // SJGDEBUG() ;
    };

    // SJGDEBUG() ;

    return count ;

}



/***********************************************************************************
 */


#define MENUENTRY_MAXLEN 64


static void init()
{
    
    // SJGDEBUGF( "%s :: init called", PLUG_IN_NAME ) ;
    
    // scan the opencl directory in gimp_directory()
    // to see if we have matching "auto_opencl_*.py"
    // file for them.
    
    GDir *dirh = NULL ;
    
    guchar *oclpath = NULL ;
    
    guchar fname[MAXPATHLEN+1] ;
    
    fname[MAXPATHLEN] = 0 ;
    
    make_gimp_filename( fname, "opencl.log", "sjgopencl_init.log" ) ;

    debug_init_log( fname ) ;

    oclpath = (guchar *)g_build_filename( gimp_directory(), "opencl", NULL ) ;
    
    if( oclpath == NULL )
    {
        goto errexit ;
    }
    
    dirh = g_dir_open( (gchar *)oclpath, 0, NULL ) ;
    
    if( dirh != NULL )
    {
        guchar *dname = NULL ;
        
        guchar dnamefull[MAXPATHLEN+1] ;
        
        int len = -1 ;
        
        int scriptdirlen = - 1 ;
        
        make_gimp_filename( fname, "scripts", "sjgopencl_" ) ;
                
        scriptdirlen = strlen( (char *)fname ) ;
        
        guchar menuentry[MENUENTRY_MAXLEN] ;
        
        menuentry[0] = 0 ;
        
        guchar **paramnames = NULL ;
        
        int paramcount = 0 ;
        
        int maxlen = 0 ;
        
        int k = 0 ;
        
        
        while( ( dname = (guchar *)g_dir_read_name( dirh ) ) != NULL )
        {
            // SJGDEBUGF( "ocl dir list :: %s", dname ) ;
            
            /* last three chars must be ".cl" so check this
             * and if not ignore the entry
             */
            
            len = strlen( (char *)dname ) ;
            
            if(    ( len < 4 )
                || ( dname[len-3] != '.' )
                || ( (char)toupper((int)dname[len-2]) != 'C' )
                || ( (char)toupper((int)dname[len-1]) != 'L' )
              )
            {
                SJGDEBUGF( SJG_MESSAGE_001, dname ) ;
                
                continue ;
            }
            
            // check to see if there is a corresponding script
            // file for it.
            
            // construct fname using omitting the "cl" at the end
            
            // Now done when fname root is constructed as it doesn't change
            // memcpy( fname+scriptdirlen, "sjgopencl_", 10 ) ;

            memcpy( fname+scriptdirlen, dname, len-2 ) ;
            
            fname[scriptdirlen+len-2] = 's' ;
            fname[scriptdirlen+len-1] = 'c' ;
            fname[scriptdirlen+len  ] = 'm' ;
            fname[scriptdirlen+len+1] = 0 ;
            
            /*
             * check if a file exists and is more recent than the
             * opencl source file ( so we can force an update of
             * script if need be ).
             */
            
            make_gimp_filename( dnamefull, "opencl", dname ) ;
    
            // if( g_access( (gchar *)fname, F_OK ) == 0 )
            if( g_file_exists( fname ) == TRUE )
            {
                // check existing file modification times
                
                if( compare_file_write_times( dnamefull , fname ) == FCWT_NEWER )
                {
                    // OpenCL file time is newer so we need
                    // to delete the old script file and
                    // generate a new one.
                    
                    SJGDEBUGF( SJG_MESSAGE_002, dname ) ;
                    
                    g_remove( (gchar *)fname ) ;
                }
            }
            
            // if no script file exists then create one
            
            // if( g_access( (gchar *)fname, F_OK ) != 0 )
            if( g_file_exists( fname ) == FALSE )
            {
                SJGDEBUGF( SJG_MESSAGE_003, fname ) ;
                
                // create the script file
                
                FILE *newscript = NULL ;
                
                newscript = fopen( (char *)fname, "w" ) ;
                
                if( newscript != NULL )
                {
                    /* check for Script-Fu parameters in the OpenCL file
                     *
                     * These are denote using commented lines ( "// " )
                     */
                    
                    void *handlep = NULL ;
                    
                    handlep = parser_loadfile( dnamefull ) ;
                    
                    if( handlep == NULL )
                    {
                        // could not load in parser - skip to next file
                    
                        SJGDEBUG() ;
                        
                        continue ;
                    }
                    
                    // build the arrayindex of all lines with
                    // the require comment signature at the start
                    
                    k = parser_build_indexarray( handlep, (guchar *)"// " ) ;
                    
                    if( k <= 0 )
                    {
                        SJGDEBUG() ;
                        
                        paramcount = 0 ;
                        
                        menuentry[0] = 0 ;
                    }
                    else
                    {
                        check_for_firstentry( handlep, "// menuentry ", menuentry, MENUENTRY_MAXLEN ) ;
                        
                        paramcount = check_for_parameters( handlep, &paramnames, &maxlen ) ;
                    }
                    
                    parser_closefile( handlep ) ;
                
                    // write to the file
                    
                    fprintf( newscript, SJG_MESSAGE_004, PLUG_IN_NAME ) ;
                    
                    fprintf( newscript, "(define (sjgopencl-%.*s image drawable", len-3, dname ) ;
                    
                    for( k = 0 ; k < paramcount ; k++ )
                    {
                        fprintf( newscript, " p%d", k ) ;
                    }
                    
                    fprintf( newscript, ")\n\n    (let* (\n" ) ;
                    
                    fprintf( newscript, "            (parray (cons-array %d 'double))\n", paramcount ) ;
                    
                    fprintf( newscript, "        )\n\n" ) ;
                    
                    for( k = 0 ; k < paramcount ; k++ )
                    {
                        fprintf( newscript, "        (aset parray %d p%d)\n", k, k ) ;
                    }
                    
                    fprintf( newscript, "\n        (%s 1 image drawable \"%.*s.cl\" %d parray)\n", PLUG_IN_NAME, len-3, dname, paramcount ) ;
                    
                    fprintf( newscript, "    )\n\n" ) ;
                    
                    fprintf( newscript, ")\n\n" ) ;
                    
                    // if we found a menu entry in the source file then
                    // it will leave a non empty string in 'menuentry'
                    
                    if( menuentry[0] == 0 )
                    {
                        // without a menuentry in the source file we try
                        // building one from the name of the file.
                    
                        // The menu entry is a modified version of dname.
                        // We remove the extension and replace underscores
                        // wirh spaces.
                        // Lastly we make the entiure string lower case except
                        // for the first character which is made upper case.
                        
                        int j = 0 ;
                        
                        while( j < len-3 )
                        {
                            if( dname[j] == '_' )
                            {
                                menuentry[j] = ' ' ;
                            }
                            else
                            {
                                menuentry[j] = dname[j] ;
                            }
                            
                            j++ ;
                        };
                        
                        menuentry[len-3] = 0 ;
                        
                        // check for the "_w" "_W" and "_h" "_H" at the end of
                        // filename before the extension.
                        // if found omit them from the menu entry
                        
                        if( dname[len-5] == '_' )
                        {
                            if(   ( dname[len-4] == 'w' )
                                || ( dname[len-4] == 'W' )
                                || ( dname[len-4] == 'h' )
                                || ( dname[len-4] == 'H' )
                              )
                            {
                                menuentry[len-5] = 0 ;
                            }
                        }
                        
                        menuentry[0] = (char)toupper( (int)menuentry[0] ) ;
                        
                        // SJGDEBUGF( "menuentry = [%s]", menuentry ) ;
                    }
                    
                    fprintf( newscript, "(script-fu-register    \"sjgopencl-%.*s\"\n", len-3, dname ) ;
                    fprintf( newscript, "    \"<Image>/Filters/Open CL/%s\"\n", menuentry ) ;
                    fprintf( newscript, SJG_MESSAGE_005, dname ) ;
                    fprintf( newscript, "    \"\"\n" ) ;
                    fprintf( newscript, SJG_MESSAGE_006 ) ;
                    fprintf( newscript, SJG_MESSAGE_007 ) ;
                    fprintf( newscript, "    \"\"\n" ) ;
                    fprintf( newscript, "    SF-IMAGE \"Image\" 0\n" ) ;
                    fprintf( newscript, "    SF-DRAWABLE \"Drawable\" 0\n" ) ;
                    
                    // SJGDEBUG() ;
                    
                    // SJGDEBUGF( "paramnames = %p", paramnames ) ;
                    
                    guchar *pp = NULL ;
                    
                    for( k = 0 ; k < paramcount ; k++ )
                    {
                        // SJGDEBUGF( "paramnames[k] = %p", paramnames[k] ) ;
                        
                        pp = paramnames[k] ;
                        
                        fprintf( newscript, "    SF-VALUE \"%s\" \"%s\"\n", pp, (pp+maxlen+1) ) ;
                    }
                    
                    fprintf( newscript, ")\n\n" ) ;
                    
                    // tidy up
                
                    fflush( newscript ) ;
                    fclose( newscript ) ;
                    
                    if( paramnames != NULL )
                    {
                        g_free_safe( paramnames ) ;
                        
                        paramnames = NULL ;
                    }
                }
            }
            else
            {
                SJGDEBUGF( SJG_MESSAGE_008, fname ) ;
            }
            
        };
    
        // close the directory
    
        g_dir_close( dirh ) ;
    }
    
errexit:

    g_free_safe( oclpath ) ;
    
    // SJGDEBUGMSG( "Finished Init." ) ;
    
    debug_finish_log() ;
}

/***********************************************************************************
 */


static void quit()
{
    SJGDEBUGF( SJG_MESSAGE_009, PLUG_IN_NAME ) ;
}


/***********************************************************************************
 */

static void query()
{
    static GimpParamDef args[] = {
        {
            GIMP_PDB_INT32,
            "run-mode",
            SJG_MESSAGE_010
        },
        {
            GIMP_PDB_IMAGE,
            "image",
            SJG_MESSAGE_011
        },
        {
            GIMP_PDB_DRAWABLE,
            "drawable",
            SJG_MESSAGE_012
        },
        {
            GIMP_PDB_STRING,
            "filename",
            SJG_MESSAGE_013
        },
        {
            GIMP_PDB_INT32,
            "arraycount",
            SJG_MESSAGE_014
        },
        {
            GIMP_PDB_FLOATARRAY,
            "array",
            SJG_MESSAGE_015
        }
    };

    gimp_install_procedure(
        PLUG_IN_NAME,
        SJG_MESSAGE_016,
        SJG_MESSAGE_017,
        PLUG_IN_AUTHOR,
        SJG_MESSAGE_018 PLUG_IN_AUTHOR,
        SJG_MESSAGE_019,
        SJG_MESSAGE_020,
        "RGB*",
        GIMP_PLUGIN,
        G_N_ELEMENTS (args), 0,
        args, NULL
        ) ;

    gimp_plugin_menu_register(PLUG_IN_NAME, SJG_MESSAGE_021 ) ;
}


/***********************************************************************************
 */


static gint32 drawableid = -1 ;

static GimpDrawable *drawable = NULL ;


static char *prepend_buffer = "\n" \
                              "union uf_u {\n" \
                              "  int   \t" \
                              "i ;\n" \
                              "  float f ;\n" \
                              "\t} ;\n" \
                              "\n" \
                              "typedef union uf_u uf_t ;\n\n" \
                              "#define PARAMF(idx) ( cons[(idx)+4].f )\n" \
                              "#define PARAMI(idx) ( cons[(idx)+4].i )\n\n" \
                              "#define WIDTH (cons[0].i)\n" \
                              "#define HEIGHT (cons[1].i)\n\n" ;

#ifdef DEBUGME
#  define DEFAULT_COMPILER_OPTIONS  "-g -cl-opt-disable"
#else
#  define DEFAULT_COMPILER_OPTIONS  "-cl-single-precision-constant " \
                                    "-cl-mad-enable " \
                                    "-cl-no-signed-zeros " \
                                    "-cl-fast-relaxed-math"
#endif


struct oclbuffer_s ;

struct oclbuffer_s {
        struct oclbuffer_s  *next ;
        guint                flags ;
        char                *hostptr ;
        cl_mem               clmemref ;
        size_t               buffersize ;
        guchar               name[128] ;
        } ;

typedef struct oclbuffer_s oclbuffer_t ;


#define OCL_FLAG_NONE   1
#define OCL_FLAG_R      2
#define OCL_FLAG_W      4
#define OCL_FLAG_GLOBAL 8
#define OCL_FLAG_LOCAL  16
#define OCL_FLAG_CONST  32
#define OCL_FLAG_LAYER  64
#define OCL_FLAG_MASK   128

#define OCL_FLAG_READ   OCL_FLAG_R
#define OCL_FLAG_WRITE  OCL_FLAG_W

#define OCL_FLAG_RW     ( OCL_FLAG_READ | OCL_FLAG_WRITE )


static inline void init_oclbuffer( oclbuffer_t *ocp )
{
    ocp->next       = NULL ;
    ocp->flags      = OCL_FLAG_NONE ;
    ocp->hostptr    = NULL ;
    ocp->clmemref   = NULL ;
    ocp->buffersize = 0 ;
    ocp->name[0]    = 0 ;
}

#define add_new_oclbuffer( newflags, ptr, size, newname ) \
    { \
    ocp = (oclbuffer_t *)alloca( sizeof(oclbuffer_t) ) ; \
    CHECKPTR(ocp) ; \
    \
    ocp->clmemref   = NULL ; \
    ocp->flags      = (newflags) ; \
    ocp->hostptr    = (char *)((ptr)) ; \
    ocp->buffersize = (size) ; \
    strncpy( (char *)(ocp->name), (char *)(newname), sizeof(ocp->name) ) ; \
    \
    slist_add( buffer_list, ocp ) ; \
    }

static inline void dump_oclbuffer_list( oclbuffer_t *root )
{
    oclbuffer_t *this ;
    
    int i = 0 ;
    
    this = root ;
    
    while( this != NULL )
    {
        SJGDEBUGF( "Buffer %d :: name = [ %s ] :: size = %zd :: ptr = %p", i, this->name, this->buffersize, this->hostptr ) ;
    
        this = this->next ;
        
        i++ ;
    };
}



/* maintain a list of kernels
 */

struct oclkernel_s ;

struct oclkernel_s {
    struct oclkernel_s    *next ;
    cl_kernel              kernel ;
    char                   name[sizeof(sclSoft)] ;
    } ;

typedef struct oclkernel_s oclkernel_t ;

/* A list of arguments to the current kernel
 */

struct oclarg_s ;

struct oclarg_s {
    struct oclarg_s     *next ;
    oclbuffer_t         *buffer ;
    } ;

typedef struct oclarg_s     oclarg_t ;




#define get_firstentry(_tag) check_for_firstentry( handlep, (_tag), tmpbuf, MAXPATHLEN )


/* Some OpenCL shorthand macros and functions
 */


#define clgetinfo( item, infoid, fn, type, msgfmt ) \
    { \
        type tmpthing ; \
        \
        cl_int tmperr ; \
        \
        tmperr = clGet ## fn ## Info( (item), (infoid), sizeof(tmpthing), (void *)&tmpthing, NULL ) ; \
        \
        if( tmperr == CL_SUCCESS ) \
        { \
            SJGDEBUGF( (msgfmt), (item), (tmpthing) ) ; \
        } \
        else \
        { \
            SJGDEBUGF( SJG_MESSAGE_022 #type " %p", (item) ) ; \
            \
            CLDEBUG(tmperr) ; \
        } \
    }



static inline void check_clmemref( cl_mem clm )
{
    if( clm == NULL )
    {
        SJGDEBUGMSG( SJG_MESSAGE_023 ) ;
    
        return ;
    }
    
    clgetinfo( clm, CL_MEM_REFERENCE_COUNT, MemObject, cl_uint, SJG_MESSAGE_024 ) ;
    clgetinfo( clm, CL_MEM_MAP_COUNT, MemObject, cl_uint, SJG_MESSAGE_025 ) ;
    clgetinfo( clm, CL_MEM_SIZE, MemObject, size_t, SJG_MESSAGE_026 ) ;
    clgetinfo( clm, CL_MEM_HOST_PTR, MemObject, void *, SJG_MESSAGE_027 ) ;
}


static inline void check_clqueue( cl_command_queue q )
{
    clgetinfo( q, CL_QUEUE_REFERENCE_COUNT, CommandQueue, cl_uint, "cl_command_queue %p has reference count %d" ) ;
    
    clgetinfo( q, CL_QUEUE_PROPERTIES, CommandQueue, cl_command_queue_properties, "cl_command_queue %p has properties %x" ) ;
}


#define cl_flush_and_finish(q)   cl_flush_and_finish_real((q),__LINE__,__func__)


#define CL_FLUSH(q) cl_flush_real((q),__LINE__,__func__)


static inline void cl_flush_real( cl_command_queue q, int lineno, const char *func )
{
    cl_int cle = CL_SUCCESS ;
    
    SJGDEBUG() ;
    
    SJGDEBUGHEAD(lineno,func) ;
    
    if( q != NULL )
    {
        cle = clFlush( q ) ;
        
        SJGDEBUG() ;
        
        CLDEBUG(cle) ;
    }
}

static cl_int cl_flush_and_finish_real( cl_command_queue q, int lineno, const char *func )
{
    cl_int cle = CL_SUCCESS ;
    
    SJGDEBUG() ;
    
    SJGDEBUGHEAD(lineno,func) ;
    
    if( q != NULL )
    {
        cle = clFlush( q ) ;
        
        SJGDEBUG() ;
        
        CLDEBUG(cle) ;
        
        if( cle == CL_SUCCESS )
        {
            cle = clFinish( q ) ;
            
            SJGDEBUG() ;
            
            CLDEBUG(cle) ;
        }
    }
    
    fprintf(stderr, "\n" ) ;
    fflush(stderr) ;
    
    return cle ;
}


static inline void check_clevent( cl_event ev )
{
    cl_int eventinfo ;
    cl_int cle ;
    
    cle = clGetEventInfo( ev, CL_EVENT_COMMAND_EXECUTION_STATUS, sizeof(eventinfo), &eventinfo, NULL ) ;
    
    if( cle == CL_SUCCESS )
    {
        switch( eventinfo )
        {
            case CL_QUEUED :
                SJGDEBUGMSG( "CL_QUEUED" ) ;
                break ;
            case CL_SUBMITTED :
                SJGDEBUGMSG( "CL_SUBMITTED" ) ;
                break ;
            case CL_RUNNING :
                SJGDEBUGMSG( "CL_RUNNING" ) ;
                break ;
            case CL_COMPLETE :
                SJGDEBUGMSG( "CL_COMPLETE" ) ;
                break ;
            default:
                SJGDEBUGMSG( SJG_MESSAGE_028 ) ;
                break ;
        };
    }
    else
    {
        SJGDEBUGMSG( SJG_MESSAGE_029 ) ;
    }
}


#define CL_RELEASE_EVENT(ev)    cl_release_event_real((ev), __LINE__, __func__ )

static inline cl_int cl_release_event_real( cl_event ev, int ln, const char *fn )
{
    SJGDEBUGHEAD( ln, fn ) ;
    fprintf( stderr, "\n" ) ;
    fflush(stderr) ;
    
    cl_int cle ;
    
    cl_int status ;
    
    cle = clGetEventInfo( ev, CL_EVENT_COMMAND_EXECUTION_STATUS, sizeof(status), (void *)&status, NULL ) ;
    
    if( cle == CL_SUCCESS )
    {
        if( ( status == CL_COMPLETE ) || ( status < 0 ) )
        {
            cle = clReleaseEvent(ev) ;
        }
        else
        {
            SJGDEBUGF( SJG_MESSAGE_030, status ) ;
        }
    }
    
    CLDEBUG(cle) ;
    
    return cle ;
}


#define CL_WAIT_FOR_EVENTS( num, events )   cl_wait_for_events_real( (num), (events), __LINE__,__func__ )

static inline void cl_wait_for_events_real( int n, cl_event *evs, unsigned int ln, const char *fn )
{
    cl_int cle ;
    
    SJGDEBUGHEAD( ln, fn ) ;
    fprintf( stderr, "\n" ) ;
    fflush(stderr) ;
    
    if( ( *evs == NULL ) || ( n != 1 ) )
        return ;

#ifdef DEBUGME
    cl_command_type type ;
    
    cle = clGetEventInfo( *evs, CL_EVENT_COMMAND_TYPE, sizeof(type), (void *)&type, NULL ) ;
    
    CLDEBUG( cle ) ;
    
    if( cle != CL_SUCCESS )
        return ;
    
    SJGDEBUGF( SJG_MESSAGE_031, type ) ;
#endif
    
    cle = clWaitForEvents( n, evs ) ;
    
    CLDEBUG( cle ) ;
    
    cle = CL_RELEASE_EVENT( *evs ) ;
    
    *evs = NULL ;
}


/*************************************************
 *
 * Code needed to check and drawable type and
 * possibly add an alpha channel to an RGB image
 */

static inline gboolean validate_drawable_type( gint32 id )
{
    if( ! gimp_drawable_is_rgb( id ) )
    {
        if( ! gimp_drawable_is_gray( id ) )
        {
            g_message( SJG_MESSAGE_032 ) ;
            
            return FALSE ;
        }
    }
    else
    {
        if( ! gimp_drawable_has_alpha( id ) )
        {
            g_message( SJG_MESSAGE_033 ) ;
            
            /* add an Alpha channel to the layer
             */
            
            if( gimp_item_is_layer( id ) )
            {
                g_message( SJG_MESSAGE_034 ) ;
            
                if( ! gimp_layer_add_alpha( id ) )
                {
                    g_message( SJG_MESSAGE_035 ) ;
                    
                    return FALSE ;
                }
                
                g_message( SJG_MESSAGE_036 ) ;
            }
            else
            {
                g_message( SJG_MESSAGE_037 ) ;
                
                return FALSE ;
            }
        }
    }
    
    return TRUE ;
}


/* Get the number of channels in a drawable
 * and return -1 for not 1 or 4 !
 */

static inline int get_channel_count( gint32 id )
{
    int count = -1 ;

    gint32 dtype ;
    
    dtype = gimp_drawable_type( id ) ;
    
    if( dtype == GIMP_RGBA_IMAGE )
    {
        count = 4 ;
    }
    else if( dtype == GIMP_GRAY_IMAGE )
    {
        count = 1 ;
    }
    else
    {
        SJGDEBUGF( SJG_MESSAGE_038 ) ;
        
        count = -1 ;
    }
    
    return count ;
}


/*
 * process() expects to receive a filename and the
 * parameters for the kernel in an int array.
 * The array values params[0] = w, params[1] = h
 * and params[2] = number of additional parameters
 * and param[3] is reserved for the local memory size
 * so it can be passed to the kernel as a constant.
 * are given.
 */
 
static void process( char *filename, int nparams, uf_t *params )
{
    GimpPixelRgn prin ;
    GimpPixelRgn prout ;
    
    guchar *bufin  = NULL ;
    guchar *bufout = NULL ;
    
    
    void *handlep = NULL ;
    
    int found = FALSE ;
    
    guchar tmpbuf[MAXPATHLEN] ;
    
    int i = 0 ;
    guchar *p = NULL ;
    
    char compiler_options[MAXPATHLEN] ;
    
    sclHard hardware ;
    
    sclSoft software ;
    
    software.kernel = NULL ;
    software.program = NULL ;
    software.kernelName[0] = 0 ;
    
    cl_int cle = CL_SUCCESS ;
    
    cl_event completed = NULL ;

    int hardwarefound   = 0 ;
    
    const char *oclsource[2] ;
    
    int tmpi = 0 ;
    
    /* Need a memory buffer list to track buffers each
     * kernel uses, releases and creates.
     */
    
    oclbuffer_t *buffer_list = NULL ;
    
    oclbuffer_t *ocp = NULL ;
    
    /* need a kernel list to track created kernels
     */
    
    oclkernel_t *kernel_list = NULL ;
    oclkernel_t *tmpkernel = NULL ;
    
    /* variables for argument list manipulation
     */
    
    oclarg_t *oclarg_list = NULL ;
    
    oclarg_t *newarg = NULL ;
    oclarg_t *tmparg = NULL ;
            
    /* we need to load the source file to parse it for various
     * flags and other items
     */
    
    int indexed_count = 0 ;
    
    handlep = parser_loadfile( (guchar *)filename ) ;
    
    if( handlep == NULL )
    {
        // file name may be missing math so build a new path and try again
    
        make_gimp_filename( tmpbuf, "opencl", filename ) ;
        
        SJGDEBUGF( SJG_MESSAGE_039, tmpbuf ) ;

        handlep = parser_loadfile( tmpbuf ) ;
        
        if( handlep == NULL )
        {
            g_message( SJG_MESSAGE_040, tmpbuf ) ;
            
            SJGDEBUGMSG( SJG_MESSAGE_041 ) ;
            
            goto err_exit ;
        }
    }
    
    /* Note that we don't close this until we return from this function.
     *
     * This lets us reuse the parser index again if we need more info
     */
    
    indexed_count = parser_build_indexarray( handlep, (guchar *)"// " ) ;

    if( indexed_count == 0 )
    {
        SJGDEBUGMSG( SJG_MESSAGE_042 ) ;
    
        g_message( SJG_MESSAGE_042 ) ;
    
        goto err_exit ;
    }
    
    if( get_firstentry( "// usegdb" ) )
    {
        if( g_setenv( "CPU_MAX_COMPUTE_UNITS", "1", TRUE ) == FALSE )
        {
            SJGDEBUGMSG( SJG_MESSAGE_043 ) ;
        }
        else
        {
            SJGDEBUGMSG( SJG_MESSAGE_044 ) ;
        }
    }
    
    /* Grab pixel region
     */
    
    // selected region bounding box
    
    int x1 = 0 ;
    int y1 = 0 ;
    int x2 = 0 ;
    int y2 = 0 ;
    
    gimp_drawable_mask_bounds( drawableid, &x1, &y1, &x2, &y2 ) ;
    
    width  = x2 - x1 ;
    height = y2 - y1 ;
    
    // SJGDEBUGF( "w = %d\n", width ) ;
    // SJGDEBUGF( "h = %d\n", height ) ;
    
    drawable = gimp_drawable_get( drawableid ) ;
    
    gimp_pixel_rgn_init( &prin , drawable, x1, y1, width, height, FALSE, FALSE ) ;
    gimp_pixel_rgn_init( &prout, drawable, x1, y1, width, height, TRUE, TRUE ) ;
    
    channels = get_channel_count( drawableid ) ;
    
    if( ( channels != 1 ) && ( channels != 4 ) )
    {
        goto err_exit ;
    }
    
    int worksize = 0 ;

    worksize = width * height * channels ;
    
    bufin  = (guchar *)g_malloc_dbg( worksize ) ;
    
    CHECKPTR( bufin ) ;
    
    bufout  = (guchar *)g_malloc_dbg( worksize ) ;
    
    CHECKPTR( bufout ) ;
    
    // SJGDEBUG() ;

    gimp_pixel_rgn_get_rect( &prin, bufin, x1, y1, width, height ) ;
    
    // SJGDEBUG() ;
    
    /**************************************************
     *
     * OpenCL stuff
     */
    
    // SJGDEBUGMSG("Starting OpenCL stuff ...") ;
    
    // add the initial input and output buffers to the list
    
    add_new_oclbuffer( OCL_FLAG_R | OCL_FLAG_GLOBAL , bufin , worksize, "bufin" ) ;
    add_new_oclbuffer( OCL_FLAG_W | OCL_FLAG_GLOBAL , bufout, worksize, "bufout" ) ;
    add_new_oclbuffer( OCL_FLAG_R | OCL_FLAG_CONST  , params, nparams*sizeof(uf_t), "params" ) ;
    
    /* set some flags that cannot be changed from kernel to kernel
     */
    
    int usecpu = FALSE ;
    
    usecpu = get_firstentry( "// usecpu" ) ;
    
#ifdef ONLYUSECPU
    usecpu = TRUE ;
#endif
    
    if( usecpu == TRUE )
    {
        // we use the CPU not the GPU
        
        // SJGDEBUGMSG( "Using C-PU..." ) ;

        SJGEXEC( hardware = sclGetCPUHardware( 0, &hardwarefound ) ; )
    }
    else
    {
        // we use the GPU
        
        // SJGDEBUGMSG( "Using G-PU..." ) ;

        SJGEXEC( hardware = sclGetGPUHardware( 0, &hardwarefound ) ; )
    }
    
    SJGDEBUGF( "hardwarefound = %d", hardwarefound ) ;
    
    /* clearly no hardware means we give up
     */
    
    if( hardwarefound == 0 )
    {
        g_message( SJG_MESSAGE_045 ) ;
        
        SJGDEBUGMSG( SJG_MESSAGE_045 ) ;
        
        goto err_exit ;
    }
    else
    {
        SJGDEBUGF( SJG_MESSAGE_046, hardware.deviceType ) ;
    }
    
    /* Build an in-memory source file with some prepended standard source
     */
    
    char *filecopy = NULL ;
    
    filecopy = (char *)parser_get_filecopy( handlep ) ;
    
    /*
     * Fill the oclsource buffer
     */
    
    oclsource[0] = (char *)prepend_buffer ;
    oclsource[1] = (char *)filecopy ;
    
    SJGDEBUGF( "oclsource starts :\n====================\n%s\n" \
                    "%s\n=================\noclsource ends.\n\n", oclsource[0], oclsource[1] ) ;
    
    SJGEXEC( software.program = clCreateProgramWithSource( hardware.context, 2, oclsource, NULL, &cle ) ; )
    
    if( software.program == CL_SUCCESS )
    {
        // cannot create program
        
        SJGDEBUGMSG( SJG_MESSAGE_047 ) ;
        
        g_message( SJG_MESSAGE_047 ) ;
        
        goto err_exit ;
    }

    SJGDEBUG() ;
    
    cle = cl_flush_and_finish( hardware.queue ) ;
    
    SJGDEBUG() ;
    
    /* build the program
     *
     * Need compiler options before we do this.
     */

    if( get_firstentry("// options ") )
    {
        // get compiler options
        
        /* This line looks trivial, but it's in case the options
         * supplied by the user were empty.  In that case the strncpy()
         * might do nothing, so make sure there is at least a terminating
         * nul char at the beginning, just in case.
         */
        
        compiler_options[0] = 0 ;
        
        strncpy( compiler_options, (char *)tmpbuf, MAXPATHLEN ) ;
    }
    else
    {
        strncpy( compiler_options, DEFAULT_COMPILER_OPTIONS, MAXPATHLEN ) ;
    }
    
	SJGEXEC( cle = clBuildProgram( software.program, 0, NULL, compiler_options, NULL, NULL ) ; )
	
   	if ( cle != CL_SUCCESS )
   	{
    	char *build_log = NULL ;
    	size_t log_size = 0 ;
	    
	    /* get the lenght of the buld log
	     */
	    
	    SJGEXEC(
		cle = clGetProgramBuildInfo(    software.program,
		                                hardware.device,
		                                CL_PROGRAM_BUILD_LOG,
		                                0,
		                                NULL,
		                                &log_size
		                            ) ;
		)
		
		if( cle == CL_SUCCESS )
		{
		    /* if that worked create space for the log
		     * and then actually fetch the log
		     */
		
		    build_log = (char *)g_malloc_dbg( log_size + 1 ) ;
		    
		    if( build_log == NULL )
		    {
		        SJGEXEC(
    		    cle = clGetProgramBuildInfo(    software.program,
    		                                    hardware.device,
    		                                    CL_PROGRAM_BUILD_LOG,
    		                                    log_size,
    		                                    (void *)build_log,
    		                                    NULL
    		                                ) ;
		        )
		        
    		    if( cle == CL_SUCCESS )
    		    {
    		        SJGDEBUGF( SJG_MESSAGE_048, build_log ) ;
    		        fprintf( stderr, "\n%s\n\n", build_log ) ;
    		        fflush( stderr ) ;
    		    }
    		    
    		    g_free_safe( build_log ) ;
    		}
		}
		
		/* last of all if we had an error in building the
		 * program then we must exit as we have no viable
		 * way to proceed.
		 */
		
		goto err_exit ;
	}
    
    /* just in case make sure the string terminates
     */
    
    compiler_options[MAXPATHLEN-1] = 0 ;
    
    SJGDEBUGF( SJG_MESSAGE_049, compiler_options ) ;
    
    /* scan through each line of the indexed lines in the source
     * and process as a script
     */
    
    size_t global_size[2] ;
    
    global_size[0] = width * height ;
    global_size[1] = 1 ;
    
    size_t local_size[2] ;
    
    local_size[0]  = CL_DEVICE_MAX_WORK_GROUP_SIZE ;
    local_size[1]  = 1 ;
    
    /* we process each pixel as a four vector of chars
     * hence the division by 4 if it's an rgba or b2b
     * image
     */
    
    int *idxarray = NULL ;
    
    idxarray = parser_get_indexarray( handlep ) ;
    
    if( idxarray == NULL )
    {
        /* make sure the upcoming loop won't do anything
         */
    
        indexed_count = 0 ;
    }
    
    // initialize some control variables
    
    int kernelcount = 0 ;
    
    strncpy( compiler_options, DEFAULT_COMPILER_OPTIONS, MAXPATHLEN-1 ) ;
    
    int line = 0 ;
    
    for( line = 0 ; line < indexed_count ; line++ )
    {
        /* check each line for a script directive
         */
        
        p = (guchar *)filecopy + idxarray[line] ;
        
        if( ( i = parser_starts_with( p,"// bypixel") ) > 0 )
        {
            SJGDEBUG() ;
        
            global_size[0] = width*height ;
            
            continue ;
        }
        
        if( ( i = parser_starts_with( p,"// byline") ) > 0 )
        {
            SJGDEBUG() ;
        
            global_size[0] = height ;
            
            continue ;
        }
        
        if( ( i = parser_starts_with( p,"// bycolumn") ) > 0 )
        {
            SJGDEBUG() ;
        
            global_size[0] = width ;
            
            continue ;
        }
        
        if( ( i = parser_starts_with( p,"// onecu") ) > 0 )
        {
            /* the "onecu" option is intended to force the compiler
             * to specify a size for the work group that matches the
             * maximum for the device in order to force the use of only
             * one compute unit.
             *
             * It's doubtful this works and is really an experimental
             * "feature" which may break things !
             */

            SJGDEBUGMSG( SJG_MESSAGE_050 ) ;
            
            local_size[0]  = CL_DEVICE_MAX_WORK_GROUP_SIZE ;
            local_size[1]  = 1 ;
            
            continue ;
        }
        
        if( ( i = parser_starts_with( p,"// buffer ") ) > 0 )
        {
            /* create and add a new buffer to our list
             */
            
            guchar name[sizeof(ocp->name)] ;
            
            guchar *q = p+i ;
            
            /* Format is 'buffer <name> <r|w|rw> <local|global|mask|layer> <size-expression|index-offset>'
             *
             * You can access masks and layers for read only.  If you use these you specify the
             * index offset from the current layer to indicate what you want.  So the command
             * 'buffer layerbelow r layer -1' means the layer below this one.
             * If you try and access a layer than does not exist then it is reported as an error.
             *
             * You can only access the current layer's mask ( at this time ) so no index is needed.
             */
            
            /* get the name
             */
            
            int j = 0 ;
            
            while( isliteral(*q) && ( j < sizeof(ocp->name) ) )
            {
                name[j] = *q ;
                
                j++ ;
                q++ ;
            };
            
            name[j] = 0 ;
            
            skipspace(q) ;
            
            guint f = 0 ;
            
            if( ( *q == 0 ) || ( *q == '\n' ) )
            {
                SJGDEBUG() ;
                
                goto err_exit ;
            }
            
            if( ( *q == 'r' ) || ( *q == 'R' ) )
            {
                f |= OCL_FLAG_R ;
                
                q++ ;
            }
            
            if( ( *q == 'w' ) || ( *q == 'W' ) )
            {
                f |= OCL_FLAG_W ;
            
                q++ ;
            }
            
            skipspace(q) ;
            
            if( sjg_strncasecmp(q,"local",5) == 0 )
            {
                f |= OCL_FLAG_LOCAL ;
                
                q += 5 ;
                
                skipspace(q) ;
            }
            else if( sjg_strncasecmp(q,"global",6) == 0 )
            {
                f |= OCL_FLAG_GLOBAL ;
                
                q += 6 ;
                
                skipspace(q) ;
            }
            else if( sjg_strncasecmp(q,"layer",5) == 0 )
            {
                if( FLAGSET( f, OCL_FLAG_W ) )
                {
                    SJGDEBUGF( SJG_MESSAGE_051, name ) ;
                    g_message( SJG_MESSAGE_051, name ) ;
                    
                    goto err_exit ;
                }
                
                f |= OCL_FLAG_LAYER ;
            }
            else if( sjg_strncasecmp(q,"mask",4) == 0 )
            {
                if( FLAGSET( f, OCL_FLAG_W ) )
                {
                    SJGDEBUGF( SJG_MESSAGE_052, name ) ;
                    g_message( SJG_MESSAGE_052, name ) ;
                    
                    goto err_exit ;
                }
                
                f |= OCL_FLAG_MASK ;
            }
            
            skipspace(q) ;
            
            /* Get the buffer size ( or the layer index )
             */
            
            int size = 0 ;
            
            if( FLAGSET( f, OCL_FLAG_LAYER ) )
            {
                int layeroffset = 0 ;
                
                if( eval_expression( q, &layeroffset ) == NULL )
                {
                    SJGDEBUGF( SJG_MESSAGE_053, name ) ;
                    g_message( SJG_MESSAGE_053, name ) ;
                    
                    goto err_exit ;
                }
                
                gint32 layerid = 0 ;
                
                gint layerpos = 0 ;
                
                gint32 imageid = -1 ;
                
                imageid = gimp_item_get_image( drawableid ) ;
                
                layerpos = gimp_image_get_item_position( imageid, drawableid ) ;
                
                layerpos += layeroffset ;
                
                gint numlayers = 0 ;
                
                gint *layerids = NULL ;
                
                layerids = gimp_image_get_layers( imageid, &numlayers ) ;
                
                if( ( layerpos < 0 ) || ( layerpos >= numlayers ) )
                {
                    SJGDEBUGF( SJG_MESSAGE_054, name ) ;
                    g_message( SJG_MESSAGE_054, name ) ;
                    
                    g_free( layerids ) ;
                    
                    goto err_exit ;
                }
                
                layerid = layerids[layerpos] ;
                
                g_free( layerids ) ;
                
                gint lx1 = 0 ;
                gint lx2 = 0 ;
                gint ly1 = 0 ;
                gint ly2 = 0 ;
                gint lw = 0 ;
                gint lh = 0 ;
                
                gimp_drawable_mask_bounds( layerid, &lx1, &ly1, &lx2, &ly2 ) ;
                
                lw = lx2 - lx1 ;
                lh = ly2 - ly1 ;
                
                if( ( lw != width ) || ( lh != height ) )
                {
                    SJGDEBUGF( SJG_MESSAGE_055, name ) ;
                    g_message( SJG_MESSAGE_055, name ) ;
                    
                    /* This could be considered an error.
                     *
                     * Ideally need a way to pass the dimensions of the
                     * bounding box to the OpenCL code
                     *
                     * but at present we don't have one.
                     *
                     * A TODO.
                     */
                }
                
                /* We need an RGBA or greyscale image type and may have
                 * to convert an RGB image
                 */
                
                if( validate_drawable_type( layerid ) == FALSE )
                {
                    goto err_exit ;
                }
                
                
                int lc = 0 ;
                
                lc = get_channel_count( layerid ) ;
                
                if( ( lc != 1 ) && ( lc != 4 ) )
                {
                    goto err_exit ;
                }
                
                GimpPixelRgn prlayer ;
                
                guchar *layerbuf  = NULL ;
                
                GimpDrawable *layerdrawable = NULL ;
                
                layerdrawable = gimp_drawable_get( layerid ) ;
                
                if( layerdrawable == NULL )
                {
                    SJGDEBUGF( SJG_MESSAGE_056, name ) ;
                    g_message( SJG_MESSAGE_056, name ) ;
                    
                    goto err_exit ;
                }
                
                
                
                size = lw * lh * lc ;
                
                layerbuf = (guchar *)g_malloc_dbg( lw * lh * lc ) ;
                
                if( layerbuf == NULL )
                {
                    SJGDEBUGF( SJG_MESSAGE_057, name ) ;
                    g_message( SJG_MESSAGE_057, name ) ;
                    
                    goto err_exit ;
                }
                
                gimp_pixel_rgn_init( &prlayer , layerdrawable, lx1, ly1, lw, lh, FALSE, FALSE ) ;
                
                gimp_pixel_rgn_get_rect( &prlayer, layerbuf, lx1, ly1, lw, lh ) ;
                
                add_new_oclbuffer( f , layerbuf , size, name ) ;
            }
            else if( FLAGSET( f, OCL_FLAG_MASK ) )
            {
                if( gimp_item_is_layer( drawableid ) == FALSE )
                {
                    SJGDEBUGF( SJG_MESSAGE_058, name ) ;
                    g_message( SJG_MESSAGE_058, name ) ;
                    
                    goto err_exit ;
                }
            
                size = width * height ;
                
                /* try and fetch the mask
                 */
                
                gint32 maskid = 0 ;
                
                maskid = gimp_layer_get_mask( drawableid ) ;
                
                if( maskid == -1 )
                {
                    SJGDEBUGF( SJG_MESSAGE_059, name ) ;
                    g_message( SJG_MESSAGE_059, name ) ;
                    
                    goto err_exit ;
                }
                
                GimpPixelRgn prmask ;
                
                guchar *maskbuf  = NULL ;
                
                GimpDrawable *maskdrawable = NULL ;
                
                maskdrawable = gimp_drawable_get( maskid ) ;
                
                if( maskdrawable == NULL )
                {
                    SJGDEBUGF( SJG_MESSAGE_060, name ) ;
                    g_message( SJG_MESSAGE_060, name ) ;
                    
                    goto err_exit ;
                }
                
                maskbuf = (guchar *)g_malloc_dbg( width * height ) ;
                
                if( maskbuf == NULL )
                {
                    SJGDEBUGF( SJG_MESSAGE_061, name ) ;
                    g_message( SJG_MESSAGE_061, name ) ;
                    
                    goto err_exit ;
                }
                
                gimp_pixel_rgn_init( &prmask , maskdrawable, x1, y1, width, height, FALSE, FALSE ) ;
                
                gimp_pixel_rgn_get_rect( &prmask, maskbuf, x1, y1, width, height ) ;
                
                add_new_oclbuffer( f , maskbuf , size, name ) ;
            }
            else
            {
                if( eval_expression( q, &size ) == NULL )
                {
                    SJGDEBUGMSG( SJG_MESSAGE_062 ) ;
                    
                    goto err_exit ;
                }
                
                /* create a new buffer_list entry for this buffer
                 *
                 * Note that NONE of these buffers are mapped from
                 * ones in the host system.  They ONLY exist as
                 * intermediate buffers on the OpenCL device.
                 */
                
                add_new_oclbuffer( f, NULL, size, name ) ;
            }
            
            continue ;
        }
        /*****************************************
         *
         * End of parsing 'buffer' command
         *
         *****************************************
         */
        
        if( ( i = parser_starts_with( p,"// kernel ") ) > 0 )
        {
            /* invoke a kernel
             *
             * format is 'kernel <kernel-name> <input-buffer> [<additional buffers>]'
             *
             * If your image is RGB then the kernal-name is left unchanged,
             * but for single channel images, the code automatically seeks
             * a kernel with "_sc" appended.
             *
             * For example "kernel wibble bufin bufout params" will call
             * the kernel function named "wibble" for RGBA images and
             * will call "wibble_sc" for single channel images.
             */
            
            guchar *q = p+i ;
            
            /* get the kernel name
             */
            
            guchar name[sizeof(software.kernelName)] ;
            
            int j = 0 ;
            
            while( isliteral(*q) && ( j < sizeof(name) ) )
            {
                name[j] = *q ;
                
                j++ ;
                q++ ;
            };
            
            /* is this a single channel image ?
             */
            
            if( channels == 1 )
            {
                /* append "_sc" to the kernel name
                 */
                
                name[j++] = '_' ;
                name[j++] = 's' ;
                name[j++] = 'c' ;
            }
            
            name[j] = 0 ;
            
            /* Try and create the kernel
             */
            
            SJGDEBUGF( SJG_MESSAGE_063, name ) ;
            
            SJGEXEC( software.kernel = clCreateKernel( software.program, (char *)name, &cle ) ; )
            
            if( cle != CL_SUCCESS )
            {
                /* an error
                 */
                
                SJGDEBUGF( SJG_MESSAGE_064, name ) ;
                
                g_message( SJG_MESSAGE_064, name ) ;
                
                CLDEBUG( cle ) ;
                                
                goto err_exit ;
            }
            
            kernelcount++ ;
            
            strncpy( software.kernelName, (char *)name, sizeof(software.kernelName) ) ;
            
            tmpkernel = (oclkernel_t*)alloca( sizeof(oclkernel_t) ) ;
            
            strncpy( tmpkernel->name, (char *)name, sizeof(software.kernelName) ) ;
            tmpkernel->kernel = software.kernel ;
            tmpkernel->next = kernel_list ;
            
            kernel_list = tmpkernel ;
            
            SJGDEBUGF( SJG_MESSAGE_065, software.kernelName, (char *)software.kernel ) ;
            
            /* For each buffer name
             * Lookp up each name in the buffer list
             * if found set the argument for the kernel
             * if not found it's an error so leave
             */
            
            guchar buffname[ sizeof(ocp->name) ] ;
            
            int buffercount = 0 ;
            
            buffname[0] = 0 ;
            buffname[sizeof(buffname)-1] = 0 ;
            
            /* NOTE :
             *
             * We EXPECT oclarg_list to be empty when we enter this loop.
             *
             * This requires that of it was previously used it should have
             * been emptied immediately after use
             */
            
            while( isnoteol(*q) )
            {
                skipspace(q) ;
                
                j = 0 ;
                
                while( isliteral(*q) && ( j < sizeof(buffname) ) )
                {
                    buffname[j] = *q ;
                    
                    j++ ;
                    q++ ;
                };
                
                buffname[j] = 0 ;
                
                skipspace(q) ;
                
                /* lookup the named buffer
                 */
                
                ocp = buffer_list ;
                
                found = FALSE ;
                
                while( ocp != NULL )
                {
                    if( ocp->name != NULL )
                    {
                        if( strcmp( (char *)ocp->name, (char *)buffname ) == 0 )
                        {
                            /* found the buffer
                             */
                             
                            found = TRUE ;
                            
                            break ;
                        }
                    }
                
                    ocp = ocp->next ;
                };
            
                if( !found )
                {
                    SJGDEBUGF( SJG_MESSAGE_066, buffname ) ;
                    
                    g_message( SJG_MESSAGE_066, buffname ) ;
                    
                    goto err_exit ;
                }
                
                SJGDEBUGF( SJG_MESSAGE_067, buffname ) ;
                
                /* found the buffer name and have ocp pointing at
                 * the relevent buffer entry
                 */
                
                newarg = (oclarg_t *)g_malloc_dbg( sizeof(oclarg_t) ) ;
                
                if( newarg == NULL )
                {
                    /* ran out of memory !
                     *
                     * even trying to report this could cause more problems.
                     */
                    
                    goto err_exit ;
                }
                
                buffercount++ ;
                
                newarg->buffer = ocp ;
                
                /* this list must be added to the end of the list
                 */
                
                slist_addlast( oclarg_list, newarg, tmparg ) ;
            };
            
            if( buffercount == 0 )
            {
                /* Must have at least one buffer
                 */
                
                SJGDEBUGF( SJG_MESSAGE_068, software.kernelName ) ;
                
                g_message( SJG_MESSAGE_068, software.kernelName ) ;
                
                goto err_exit ;
            }
            
            /* set the kernel arguments
             */
            
            buffercount = -1 ;
            
            tmparg = oclarg_list ;
            
            while( tmparg != NULL )
            {
                ocp = tmparg->buffer ;
                
                tmparg = tmparg->next ;
                
                buffercount++ ;
                
                if( ocp->clmemref != NULL )
                {
                    /* just need to set the argument.
                     */
                    
                    SJGEXEC(
                    cle = clSetKernelArg( software.kernel, buffercount, sizeof(cl_mem), (void *)&(ocp->clmemref) ) ;
                    )
                    
                    CLDEBUG( cle ) ;
                
                    SJGDEBUGF( SJG_MESSAGE_069, ocp->name ) ;
                }
                else if( ( ocp->flags & OCL_FLAG_LOCAL ) == OCL_FLAG_LOCAL )
                {
                    /* A local memory buffer
                     */
                    
                    SJGEXEC(
                    cle = clSetKernelArg( software.kernel, buffercount, ocp->buffersize, NULL ) ;
                    )
                    
                    CLDEBUG( cle ) ;
                
                    SJGDEBUGF( SJG_MESSAGE_070, ocp->buffersize, buffname ) ;
                }
                else if(    FLAGSET( ocp->flags, OCL_FLAG_GLOBAL )
                         || FLAGSET( ocp->flags, OCL_FLAG_CONST )
                         || FLAGSET( ocp->flags, OCL_FLAG_MASK )
                         || FLAGSET( ocp->flags, OCL_FLAG_LAYER )
                       )
                {
                    /* A global or constant buffer of some type
                     */
                    
                    /* if we got here the memory either needs to be
                     * transferred from the host or it is a new buffer
                     * resident only on the OpenCL device.
                     */
                    
                    cl_mem newclmemref = NULL ;
                    cl_mem_flags mflags = 0 ;
                    
                    if( FLAGSET( ocp->flags, OCL_FLAG_READ ) )
                    {
                        if( FLAGSET( ocp->flags, OCL_FLAG_WRITE ) )
                        {
                            mflags = CL_MEM_READ_WRITE ;
                            
                            SJGDEBUG() ;
                        }
                        else
                        {
                            mflags = CL_MEM_READ_ONLY ;
                            
                            SJGDEBUG() ;
                        }
                    }
                    else if( FLAGSET( ocp->flags, OCL_FLAG_WRITE ) )
                    {
                        mflags = CL_MEM_WRITE_ONLY ;
                        
                        SJGDEBUG() ;
                    }
                    
                    
                    if( ( ocp->hostptr != NULL ) && FLAGSET( mflags, CL_MEM_READ_ONLY ) )
                    {
                        /* if hostptr is not NULL and marked for reading we have a real buffer we want to transfer
                         */
                        
                        mflags |= CL_MEM_COPY_HOST_PTR ;
                        
                        SJGDEBUG() ;
                    }
                    
                    cle = CL_SUCCESS ;
                    
                    if( FLAGSET( mflags, CL_MEM_COPY_HOST_PTR ) )
                    {
                       // SJGDEBUG() ;
                        
                        /* The commented out code works, but the second code may
                         * be the better for efficiency, as the commented out code
                         * blocks until the transfer is complete.
                         */
                        
                        // newclmemref = clCreateBuffer( hardware.context, mflags, ocp->buffersize, ocp->hostptr, &cle ) ;
                        
                        SJGEXEC(
                        newclmemref = clCreateBuffer(   hardware.context,
                                                        ( mflags & ~CL_MEM_COPY_HOST_PTR ),
                                                        ocp->buffersize,
                                                        NULL,
                                                        &cle
                                                    ) ;
                        )
                        
                        if( cle == CL_SUCCESS )
                        {
                            // SJGDEBUG() ;
                            
                            SJGEXEC(
                            cle = clEnqueueWriteBuffer( hardware.queue,
                                                        newclmemref,
                                                        CL_TRUE,
                                                        0,
                                                        ocp->buffersize,
                                                        (void *)ocp->hostptr,
                                                        0, NULL, NULL
                                                      ) ;
                            )
                        }
                    }
                    else
                    {
                        // SJGDEBUG() ;
                    
                        SJGEXEC(
                        newclmemref = clCreateBuffer( hardware.context, mflags, ocp->buffersize, NULL, &cle ) ;
                        )
                    }
                    
                    CLDEBUG(cle) ;
                    
                    if( cle == CL_SUCCESS )
                    {
                        /* OK to keep going as we have a new clmemref
                         */
                        
                        SJGDEBUGF( SJG_MESSAGE_071, ocp->name ) ;
                        
                        ocp->clmemref = newclmemref ;
                        
                        SJGEXEC(
                        cle = clSetKernelArg( software.kernel, buffercount, sizeof(cl_mem), (void *)&newclmemref ) ;
                        )
                        
                        CLDEBUG(cle) ;
                        
                        if( cle == CL_SUCCESS )
                        {
                            SJGDEBUGF( SJG_MESSAGE_072, ocp->name, ocp->buffersize ) ;
                        }
                    }
                }
                
                /* check for an error
                 */
                
                if( cle != CL_SUCCESS )
                {
                    /* some kind of error
                     */
                    
                    SJGDEBUGF( SJG_MESSAGE_073, buffercount, software.kernelName ) ;
                    
                    g_message( SJG_MESSAGE_073, buffercount, software.kernelName ) ;
                    
                    CLDEBUG( cle ) ;
                
                    goto err_exit ;
                }
            };
            
            /* make sure any pending transfers or kernels are completed !
             *
             * if we don't do this we may end up with kernels running on
             * inconsistenmt buffers ( or in theory even running kernels
             * at the same time that need to be sequential ! ).
             */
            
            // cle = cl_flush_and_finish( hardware.queue ) ;
            
            CL_FLUSH( hardware.queue ) ;
            
            CLDEBUG(cle) ;
            
            /* Enqueue the kernel ( at last ! )
             */
            
            SJGDEBUGF( SJG_MESSAGE_074, software.kernelName, (char *)software.kernel ) ;
            
            SJGDEBUG() ;
            
            check_memmarkers() ;
            
            
enqueue_kernel:
            
            cle = CL_SUCCESS ;
            
            SJGDEBUGF( SJG_MESSAGE_075, global_size[0] ) ;
            SJGDEBUGF( SJG_MESSAGE_075, global_size[1] ) ;
            
            SJGEXEC(
            CL_WAIT_FOR_EVENTS( 1, &completed ) ;
            )
            
#ifdef DEBUGME
            tmparg = oclarg_list ;
            while( tmparg != NULL )
            {
                check_clmemref( tmparg->buffer->clmemref ) ;
            
                tmparg = tmparg->next ;
            };
#endif

            SJGEXEC(
            cle = clEnqueueNDRangeKernel(   hardware.queue,
                                            software.kernel,
                                            2,
                                            NULL,
                                            global_size,
                                            NULL,
                                            0, NULL, &completed
                                         ) ;
            )

            SJGDEBUG() ;
            
            if( cle != CL_SUCCESS )
            {
                SJGDEBUGF( SJG_MESSAGE_076, software.kernelName ) ;
                g_message( SJG_MESSAGE_076, software.kernelName ) ;
                
                CLDEBUG( cle ) ;
                
                goto err_exit ;
            }
            
            SJGDEBUGF( SJG_MESSAGE_077, software.kernelName ) ;
            
            CL_FLUSH( hardware.queue ) ;
            
#ifdef DEBUGME
            check_clevent( completed ) ;
            
            SJGDEBUGF( SJG_MESSAGE_078 ) ;
            
            check_clqueue( hardware.queue ) ;
#endif
            
            CL_WAIT_FOR_EVENTS( 1, &completed ) ;
            
            SJGDEBUGF( SJG_MESSAGE_079, software.kernelName ) ;
            
            /* Tidy up
             */
            
#ifdef DEBUGME
            check_clqueue( hardware.queue ) ;
#endif
            
            /* Defer all kernel releases until we're completely finished
             */
            
            software.kernel = NULL ;
            
            /* empty the argument list
             */
            
            while( oclarg_list != NULL )
            {
                tmparg = oclarg_list ;
                
                g_free_safe( oclarg_list ) ;
                
                oclarg_list = tmparg->next ;
            };
            
            continue ;
        }
        /*******************************************
         *
         * end of kernel directive processing
         */
    };
    /*******************************************
     *
     * End of for loop parsing comment lines
     */
    
    /* If we get here then all the kernels ran without error
     * and we can simply update the output buffer on the
     * system from the device resident copy.
     *
     * Obviously it's up to the user to remember to write
     * a kernel that writes to the output buffer.  We have
     * no way to enforce that.
     */
    
#ifdef DEBUGME
    dump_oclbuffer_list( buffer_list ) ;
#endif
    
    /* find the output buffer ( named "bufout" )
     * in the buffer list
     */
    
    ocp = buffer_list ;
    
    found = FALSE ;
    
    while( ocp != NULL )
    {
        if( ocp->name != NULL )
        {
            if( strcmp( (char *)ocp->name, "bufout" ) == 0 )
            {
                found = TRUE ;
                
                break ;
            }
        }
        
        ocp = ocp->next ;
    };
    
    if( ! found )
    {
        /* this would be a catastrophic and strange error
         * as it should be impossible at this point in the
         * code, but just in case ...
         *
         * Note that in theory an APU could corrupt memory
         * although we'd hope that driver writer would not
         * let this happen.
         */
        
        SJGDEBUGMSG( SJG_MESSAGE_080 ) ;
        
        goto err_exit ;
    }
    
    SJGDEBUGF( SJG_MESSAGE_081, ocp->name, ocp->hostptr, ocp->buffersize, ocp->clmemref ) ;
    
    CL_WAIT_FOR_EVENTS( 1, &completed ) ;
    
    SJGEXEC(
    cle = clEnqueueReadBuffer(  hardware.queue,
                                ocp->clmemref,
                                CL_FALSE,
                                0,
                                ocp->buffersize,
                                ocp->hostptr,
                                0, NULL, &completed
                             ) ;
    )
    
    // cle = cl_flush_and_finish( hardware.queue ) ;
    
    SJGDEBUG() ;
    
    CL_FLUSH( hardware.queue ) ;
    
    SJGDEBUGF( SJG_MESSAGE_082, ocp->name ) ;
    
    CL_WAIT_FOR_EVENTS( 1, &completed ) ;
    
    SJGDEBUG() ;
    
    check_memmarkers() ;
    
    g_message( SJG_MESSAGE_083 ) ;
    
    SJGDEBUGMSG( SJG_MESSAGE_083 ) ;
    
    /* end of OpenCL stuff
     *
     ***************************************************/
    
    gimp_pixel_rgn_set_rect( &prout, bufout, x1, y1, width, height ) ;
    
    gimp_drawable_flush( drawable ) ;
    
    gimp_drawable_merge_shadow( drawableid, TRUE ) ;
    
    gimp_drawable_update( drawableid, x1, y1, width, height ) ;

    /* end of pixel region stuff
     *
     ***************************************************/

err_exit:

    // tidy up
    
    SJGDEBUG() ;
    
    if( handlep != NULL )
    {
        parser_closefile( handlep ) ;
    }
    
    SJGDEBUG() ;
    
    /* release any kernels
     */
    
    SJGDEBUG() ;
    
    while( kernel_list != NULL )
    {
        tmpkernel = kernel_list->next ;
        
        SJGDEBUGF( SJG_MESSAGE_084, kernel_list->name ) ;
        
        SJGEXEC( cle = clReleaseKernel( kernel_list->kernel ) ; )
        
        CLDEBUG(cle) ;
        
        kernel_list = tmpkernel ;
    };
    
    SJGDEBUG() ;
    
    /* Note :
     *
     * buffer_list is built using alloca() for nodes
     * so there is no need to release the memory for the nodes.
     *
     * however we do have to ensure any cl_mem objects are released
     * and that we free any host buffers allocated.
     *
     * Note that this will also free the host memory used by
     * any buffers, including bufin and bufout
     *
     * NOTE this also frees the parameter buffer !
     */
    
    while( buffer_list != NULL )
    {
        if( buffer_list->clmemref != NULL )
        {
            SJGEXEC( cle = clReleaseMemObject( buffer_list->clmemref ) ; )
            
            CLDEBUG(cle) ;
        }
        
        if( buffer_list->hostptr != NULL )
        {
            g_free_safe( buffer_list->hostptr ) ;
        }
        
        buffer_list = buffer_list->next ;
    };
    
    /* in case we got here due to an incomplete parse
     * of a kernel argument there might be allocated
     * memory in the argument list
     */
    
    SJGDEBUG() ;
    
    newarg = oclarg_list ;
    
    while( newarg != NULL )
    {
        tmparg = newarg->next ;
        
        g_free_safe( newarg ) ;
        
        newarg = tmparg ;
    };
    
    oclarg_list = NULL ;
    
    /* release any software and hardware objects
     */
    
    SJGDEBUG() ;
    
    if( hardwarefound != 0 )
    {
        if( software.program != NULL )
        {
            SJGEXEC( cle = clReleaseProgram( software.program ) ; )
            
            CLDEBUG(cle) ;
        }
        
    	SJGEXEC( cle = clReleaseCommandQueue( hardware.queue ) ; )
	    
	    CLDEBUG(cle) ;
	    
    	SJGEXEC( cle = clReleaseContext( hardware.context ) ; )
    	
    	CLDEBUG(cle) ;
    }
    
    SJGDEBUGMSG( SJG_MESSAGE_085 ) ;
}

/***********************************************************************************
 */


static void run(
    const gchar      *name,
    gint              nparams,
    const GimpParam  *param,
    gint             *nreturn_vals,
    GimpParam       **return_vals
    )
{
    static GimpParam  values[1] ;
    
    GimpPDBStatusType status = GIMP_PDB_SUCCESS ;
    GimpRunMode       run_mode ;
    
    GtkWidget *dialog = NULL ;
    
    // FILE *fout = NULL ;
    
    guchar fname[MAXPATHLEN+1] ;
    
    make_gimp_filename( fname, "opencl.log", "sjgopencl.log" ) ;

    debug_init_log( fname ) ;
    
    /* Setting mandatory output values
     */
    *nreturn_vals = 1 ;
    *return_vals  = values ;

    values[0].type          = GIMP_PDB_STATUS ;
    values[0].data.d_status = status ;

    char *filename = NULL ;
    
    int filename_from_chooser = FALSE ;
    
    /*  Get the specified drawable  */
    
    
    
    drawableid = param[2].data.d_drawable ;
    
    SJGDEBUGF( "nparams = [%d]", nparams ) ;
    
    // Dump the params
    
#ifdef DEBUGME
#  define SJGDEBUG_ARGTYPE(idx,typ) \
                { \
                    if( param[i].type == GIMP_PDB_##typ ) \
                    { \
                        SJGDEBUGF( "  param %d was " #typ , (idx) ) ; \
                    } \
                }

    {
        int i = 0 ;
        
        while( i < nparams )
        {
            SJGDEBUG_ARGTYPE( i, INT32 ) ;
            SJGDEBUG_ARGTYPE( i, INT16 ) ;
            SJGDEBUG_ARGTYPE( i, INT8 ) ;
            SJGDEBUG_ARGTYPE( i, FLOAT ) ;
            SJGDEBUG_ARGTYPE( i, STRING ) ;
            SJGDEBUG_ARGTYPE( i, INT32ARRAY ) ;
            SJGDEBUG_ARGTYPE( i, INT16ARRAY ) ;
            SJGDEBUG_ARGTYPE( i, INT8ARRAY ) ;
            SJGDEBUG_ARGTYPE( i, FLOATARRAY ) ;
            SJGDEBUG_ARGTYPE( i, STRINGARRAY ) ;
            SJGDEBUG_ARGTYPE( i, COLOR ) ;
            SJGDEBUG_ARGTYPE( i, ITEM ) ;
            SJGDEBUG_ARGTYPE( i, DISPLAY ) ;
            SJGDEBUG_ARGTYPE( i, IMAGE ) ;
            SJGDEBUG_ARGTYPE( i, LAYER ) ;
            SJGDEBUG_ARGTYPE( i, CHANNEL ) ;
            SJGDEBUG_ARGTYPE( i, DRAWABLE ) ;
            SJGDEBUG_ARGTYPE( i, SELECTION ) ;
            SJGDEBUG_ARGTYPE( i, COLORARRAY ) ;
            SJGDEBUG_ARGTYPE( i, VECTORS ) ;
            SJGDEBUG_ARGTYPE( i, PARASITE ) ;
            SJGDEBUG_ARGTYPE( i, STATUS ) ;
            SJGDEBUG_ARGTYPE( i, END ) ;

            i++ ;
        };
    }

#endif
        
    if( validate_drawable_type( drawableid ) == FALSE )
    {
        goto err_exit ;
    }
    
    /* Getting run_mode - we won't display a dialog if 
     * we are in NONINTERACTIVE mode
     */
    run_mode = param[0].data.d_int32 ;

    if( run_mode == GIMP_RUN_NONINTERACTIVE )
    {
        /* filename is in parameter
         */
        
        SJGDEBUGMSG( "GIMP_RUN_NONINTERACTIVE detected" ) ;
        
        if( nparams == 6 )
        {
            filename = param[3].data.d_string ;
            
            filename_from_chooser = FALSE ;
            
            SJGDEBUGF( "param[3].data.d_string = [%s]", filename ) ;
        }
        else
        {
            SJGDEBUGF( "nparams = [%d] was too small", nparams ) ;
        }
    }
    else
    {
        SJGDEBUGMSG( "GIMP_RUN_NONINTERACTIVE not detected" ) ;
        
        gimp_ui_init( "GenericFilter", FALSE ) ;
        
        dialog = gtk_file_chooser_dialog_new( "Open File",
                                              NULL,
                                              GTK_FILE_CHOOSER_ACTION_OPEN,
                                              GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                              GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                              NULL ) ;
        
        if( dialog != NULL )
        {
            gtk_file_chooser_set_current_folder( (GtkFileChooser *)dialog, gimp_directory() ) ;
            
            GtkFileFilter *filefilter = NULL ;
            
            filefilter = gtk_file_filter_new() ;
            gtk_file_filter_add_pattern( filefilter, "*.cl" ) ;
            
            gtk_file_chooser_add_filter( (GtkFileChooser *)dialog, filefilter ) ;
            
            if( gtk_dialog_run( GTK_DIALOG(dialog) ) == GTK_RESPONSE_ACCEPT )
            {
                char *dialogfname = NULL ;
            
                dialogfname = gtk_file_chooser_get_filename( GTK_FILE_CHOOSER(dialog) ) ;

                if( dialogfname != NULL )
                {
                    int fnlen = 0 ;
                    
                    fnlen = strlen( dialogfname ) ;
                    fnlen++ ;
                    
                    if( fnlen < MAXPATHLEN )
                    {
                        filename = (char *)g_malloc_dbg( fnlen ) ;
                        
                        CHECKPTR( filename ) ;
                        
                        memcpy( filename, dialogfname, fnlen ) ;
                        
                        filename_from_chooser = TRUE ;
                    }
                }
            }
        }
        else
        {
            fprintf( stderr, "SJG :: dialog not created\n" ) ;
        }
    }
    
    CHECKPTR( filename ) ;

    SJGDEBUG() ;
    
    // all in order so process the drawable
    
    // first construct a parameter array ( if needed )
    
    int x1 = 0 ;
    int y1 = 0 ;
    int x2 = 0 ;
    int y2 = 0 ;
    
    int w, h ;
    
    gimp_drawable_mask_bounds( drawableid, &x1, &y1, &x2, &y2 ) ;
    
    w = x2 - x1 ;
    h = y2 - y1 ;
    
    uf_t *p = NULL ;
    int np = 0 ;
    
    // Build the constants data passing parameters to the OpenCL
    
    /* Parameter array always includes stats appended to end
     * of parameter list as they are commonly used in image
     * processing algorithms
     *
     * If there are no parameters the extra values are not appended.
     */
     
     /* Standard Parameters are :
      *
      * index 0        : width in pixels
      * index 1        : height in pixels
      * index 2        : number of parameters ( N )
      * index 3        : local memory size for kernel
      * index 4 to 3+N : paramters listed in kernel ( from UI )
      * index 4+N      : sum of parameters after index 2
      * index 5+N      : square of deviation of parameters after index 2
      * index 6+N      : standard deviation of parameters after index 2
      */
    
    gdouble sumx  = 0.0 ;
    gdouble sumxx = 0.0 ;
    
    np = 4 ;
    
    if( ( param[4].type == GIMP_PDB_INT32 ) && ( param[5].type == GIMP_PDB_FLOATARRAY ) )
    {
        // have an array count
        
        int ii = 0 ;
        
        gint nn = param[4].data.d_int32 ;
        
        p = (uf_t *)g_malloc_dbg( ( nn + 7 ) * sizeof(uf_t) ) ;
        
        CHECKPTR( p ) ;
        
        SJGDEBUGF( SJG_MESSAGE_086, nn ) ;
        
        gdouble *gda = (gdouble *)( param[5].data.d_floatarray ) ;
         
        for( ii = 0 ; ii < nn ; ii++ )
        {
            SJGDEBUGF( "Parameter [%d] = %g", ii, gda[ii] ) ;
            
            // NOTE : gimp uses gdouble as a float value
            // OpenCL will be getting a single precision float
            // We must convert them.
            
            p[np++].f = (float)( gda[ii] ) ;
            
            sumx  += gda[ii] ;
            sumxx += (gda[ii])*(gda[ii]) ;
        }
        
        p[np++].f = (float)sumx ;
        
        // square of standard deviation of parameters
        
        p[np].f = (float)( ( sumxx - ( sumx*sumx/(gdouble)nn ) ) / (gdouble)nn ) ;
        
        // standard deviation - don't use dec and inc ops here to avoid
        // potential issues
        
        p[np+1].f = sqrtf( p[np].f ) ;
        
        np++ ;
    }
    else
    {
        p = (uf_t *)g_malloc_dbg( 4 * sizeof(uf_t) ) ;
        
        CHECKPTR( p ) ;
    }
    
    p[0].i = w ;
    p[1].i = h ;
    p[2].i = np ;
    p[3].i = 0 ;
    
    check_memmarkers() ;
    
    // process the drawable
    
    gimp_progress_init( SJG_MESSAGE_087 ) ;
    
    gimp_progress_pulse() ;
    
    process( filename, np, p ) ;
    
    SJGDEBUG() ;
    
    gimp_progress_end() ;
    
    // tidy up
    
    /* The parameter buffer becomes a buffer managed by the process()
     * function and that function will ALSO free the memory allocated
     * as part of it's normal routine.
     */
    // g_free_safe( p ) ;
    
    SJGDEBUG() ;

    if( filename_from_chooser != 0 )
    {
        g_free_safe( filename ) ;
    }

    SJGDEBUG() ;


err_exit:
        
    if( dialog != NULL )
    {
        SJGDEBUG() ;
        
        gtk_widget_destroy( dialog ) ;

        SJGDEBUG() ;
    }


    SJGDEBUG() ;

    debug_finish_log() ;


    SJGDEBUG() ;
    
    // SJGDEBUG() ;
    
    gimp_displays_flush() ;
    
    SJGDEBUG() ;
}


/***********************************************************************************
 */



GimpPlugInInfo PLUG_IN_INFO = {
    init,
    quit,
    query,
    run
};


MAIN() ;

