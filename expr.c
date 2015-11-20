
/*
 * expr.c
 *
 * $Id: expr.c,v 1.8 2014/11/05 04:19:17 sjg Exp $
 *
 * (c) Stephen Geary, Oct 2014
 *
 * An expression evaluator
 *
 * Designed for positive integer expressions.
 * in use in the OpenCL plug-in.
 */

#include <stdio.h>

#include <stdlib.h>

#include <glib.h>

#include "sjgutils.h"

#include "expr.h"


#define MAXDIGITS 10

// need to make forward reference so must declare.


static guchar *eval_simple_expression( guchar *expr, int *resultp )
{
    guchar *retp = NULL ;
    
    if( expr == NULL )
        return NULL ;
    
    if( resultp == NULL )
        return NULL ;
    
    /* Evaluate simple expressions used in simple
     * code statements.
     *
     * Expression of these forms are allowed :
     *
     *      <a positive integer>
     *      w ( image width )
     *      h ( image height )
     *      n ( number of pixels )
     *      c ( number of channels )
     *      f ( size of float in bytes )
     *      i ( size of int in bytes )
     *      k ( shorthand for 1024 )
     *      m ( shorthand for 1024*1024 )
     *      d ( largest of w and h )
     *      s ( smallest of w and h )
     */
    
    int res1 = 0 ;
    int i  = 0 ;
    
    guchar *p = expr ;
    
    skipspace(p) ;
    
    retp = p + 1 ;
    
    switch( *p )
    {
        case 'w' :
            *resultp = width ;
            break ;
        case 'h' :
            *resultp = height ;
            break ;
        case 'n' :
            *resultp = width*height ;
            break ;
        case 'd' :
            *resultp = ( width > height ? width : height ) ;
            break ;
        case 's' :
            *resultp = ( width < height ? width : height ) ;
            break ;
        case 'k' :
            *resultp = 1024 ;
            break ;
        case 'm' :
            *resultp = 1048576 ;
            break ;
        case 'f' :
            *resultp = sizeof( float ) ;
            break ;
        case 'i' :
            *resultp = sizeof( int ) ;
            break ;
        case 'c' :
            *resultp = channels ;
            break ;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            res1 = (int)((*p)-'0') ;
            p++ ;
            i = 1 ;
            while( isdigit(*p) && ( i < MAXDIGITS ) )
            {
                res1 *= 10 ;
                res1 += (int)((*p)-'0') ;
                i++ ;
                p++ ;
            };
            if( i <= MAXDIGITS )
            {
                retp = p ;
                *resultp = res1 ;
            }
            else
            {
                retp = NULL ;
            }
            break ;
        case '(':
            {
                // expect a sub-expression in brackets
                
                guchar *p2 ;
                int res2 = 0 ;
                
                p2 = p+1 ;
                
                p2 = eval_expression( p2, &res2 ) ;
                
                if( p2 != NULL )
                {
                    skipspace(p2) ;
                    
                    if( *p2 == ')' )
                    {
                        p = p2 + 1 ;
                        retp = p ;
                        *resultp = res2 ;
                    }
                }
            }
            break ;
        default:
            retp = NULL ;
            break ;
    };
    
    return retp ;
}


#define BIOP_NONE 0
#define BIOP_ADD 1
#define BIOP_SUB 2
#define BIOP_MUL 3
#define BIOP_DIV 4
#define BIOP_MOD 5

/* precedence is :
 *
 * First :
 *          BIOP_MOD
 *          BIOP_DIV
 *          BIOP_MUL
 *          BIOP_ADD
 *          BIOP_SUB
 * Last
 */

static int precedence[5] = { BIOP_MOD, BIOP_DIV, BIOP_MUL, BIOP_SUB, BIOP_ADD } ;

#ifdef DEBUG_EXPR
static char *opname[5] = { "BIOP_ADD", "BIOP_SUB", "BIOP_MUL", "BIOP_DIV", "BIOP_MOD" } ;
#endif


/* use alloca() to track a list of expression elements
 * linked by operators.  This list is scanned for
 * precedence order and the list reduced in this way
 * util a single value remains
 */

struct exprstack_s ;

struct exprstack_s {
        struct exprstack_s *next ;
        int value ;
        } ;

typedef struct exprstack_s exprstack_t ;

/* Note stack reverses order of operands !
 * that matters for MOD and DIV which are not commuative !
 *
 * Note we use alloca() as each sub-expression is evaluated
 * on a local stack.
 */

#define PUSH(v)     { \
                        exprstack_t *_p = NULL ; \
                        _p = (exprstack_t *)alloca( sizeof(exprstack_t) ) ; \
                        _p->next = stack ; \
                        stack = _p ; \
                        _p->value = (v) ; \
                    }


#ifdef DEBUG_EXPR
static void expr_dump_stack( exprstack_t *s )
{
    printf( "STACK ::\n" ) ;

    if( s != NULL )
    {
        printf( "\tstack :: %d\n", s->value ) ;
        s = s->next ;
    }

    while( s != NULL )
    {
        printf( "\tstack :: %s\n", opname[s->value - 1] ) ;
        
        s = s->next ;
    
        printf( "\tstack :: %d\n", s->value ) ;
    
        s = s->next ;
    };
    
    printf( "STACK END\n\n" ) ;
}
#else
#  define expr_dump_stack(_s)
#endif



guchar *eval_expression( guchar *expr, int *resultp )
{
    if( ( expr == NULL ) || ( resultp == NULL ) )
    {
        return NULL ;
    }
    
    guchar *retp = NULL ;
    
    // int res = 0 ;
    
    int res1 = 0 ;
    int res2 = 0 ;
    
    int biop = BIOP_NONE ;
    
    /* evaluate expressions of the form :
     *
     * <simple expression>
     * <expression> <binaryop> <expression>
     * '(' <expression> ')'
     */
    
    guchar *p = expr ;
    guchar *p2 = NULL ;
    
    skipspace(p) ;
    
    if( iseol(*p) )
        return NULL ;
    
    if( ( p2 = eval_simple_expression(p,&res2) ) != NULL )
    {
        // a simple expression
        
        retp = p2 ;
        res1 = res2 ;
    }
    else
    {
        return NULL ;
    }
    
    // start the stack
    
    exprstack_t *stack = NULL ;
    
    PUSH( res1 ) ;
    
    // check for more to this expression
    
    p = retp ;
    
    skipspace(p) ;
    
    while( isnoteol(*p) && ( *p != ')' ) )
    {
        // could be more
        
        // must be a binary operator
        
        biop = BIOP_NONE ;
        
        switch(*p)
        {
            case '+' :
                biop = BIOP_ADD ;
                break ;
            case '-' :
                biop = BIOP_SUB ;
                break ;
            case '*' :
                biop = BIOP_MUL ;
                break ;
            case '/' :
                biop = BIOP_DIV ;
                break ;
            case '%' :
                biop = BIOP_MOD ;
                break ;
            default :
                // an error
                return NULL ;
                break ;
        }
        
        p++ ;
        
        skipspace(p) ;
        
        if( iseol(*p) )
        {
            // an error
            
            return NULL ;
        }
        
        // what follows must be an expression
        
        p2 = eval_simple_expression( p, &res2 ) ;
        
        if( p2 == NULL )
        {
            return NULL ;
        }
        
        // we got here so we need to stack the op and new value
        
        PUSH( biop )
        
        PUSH( res2 ) ;

        p = p2 ;
        
        skipspace(p) ;
    };
    
    retp = p ;
    
    /* now we evaluate the stack in precendce order
     */
    
    exprstack_t *sp = stack->next ;
    exprstack_t *prev = stack ;
    
    int i = 0 ;
    
    while( ( sp != NULL ) && ( i < sizeof(precedence) ) )
    {
        expr_dump_stack(stack) ;
    
        while( sp != NULL )
        {
            if( sp->value == precedence[i] )
            {
                /* found a BIOP matching our current precedence type
                 *
                 * reduce the list by performing the operation
                 */
                
                int v1 = prev->value ;
                int v2 = sp->next->value ;
                
                switch( sp->value )
                {
                    case BIOP_ADD :
                        v1 += v2 ;
                        break ;
                    case BIOP_SUB :
                        v1 = v2 - v1 ;
                        break ;
                    case BIOP_MUL :
                        v1 *= v2 ;
                        break ;
                    case BIOP_DIV :
                        v1 = v2 / v1 ;
                        break ;
                    case BIOP_MOD :
                        v1 = v2 % v1 ;
                        break ;
                    default:
                        retp = NULL ;
                        break ;
                }
                
                sp = (sp->next)->next ;
                prev->next = sp ;
                prev->value = v1 ;
            }
            else
            {
                prev = sp->next ;
                sp = (sp->next)->next ;
            }
        };
        
        // next loop
        
        sp = stack->next ;
        prev = stack ;
        
        i++ ;
    };
    
    *resultp = stack->value ;
    
    return retp ;
}


