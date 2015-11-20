
/*
 * expr.h
 *
 * $Id: expr.h,v 1.1 2014/11/05 04:19:20 sjg Exp $
 *
 * (c) Stephen Geary, Oct 2014
 *
 * An expression evaluator
 *
 * Designed for positive integer expressions.
 * in use in the OpenCL plug-in.
 */


#ifndef __EXPR_H
#  define __EXPR_H

extern int width ;

extern int height ;

extern int channels ;

extern guchar *eval_expression( guchar *expr, int *resultp ) ;

#endif /* __EXPR_H */

