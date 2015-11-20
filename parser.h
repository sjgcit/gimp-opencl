
/*
 * Routines for scanning a source file's comments
 *
 * $Id: parser.h,v 1.2 2014/11/05 07:48:03 sjg Exp $
 *
 * (c) Stephen Geary, Oct 2014
 */

#ifndef __SJGPARSER_H

#define __SJGPARSER_H

#define parser_starts_with( p1, p2 )    parser_starts_with_real( (guchar *)(p1), (guchar *)(p2) )

extern guchar *parser_ptrtoindex( void *handlep, int idx ) ;

extern int parser_starts_with_real( guchar *big, guchar *small ) ;

extern void *parser_loadfile( guchar *fname ) ;

extern void parser_closefile( void *handlep ) ;

extern int parser_build_indexarray( void *handlep, guchar *prefix ) ;

extern int *parser_get_indexarray( void *handlep ) ;

extern int parser_get_filelen( void *handle ) ;

extern guchar *parser_get_filecopy( void *handle ) ;

extern int *parser_get_indexarray( void *handle ) ;

extern int parser_get_indexlen( void *handle ) ;

extern int parser_readfp( guchar *ptr ) ;

#endif /* __SJGPARSER_H */


