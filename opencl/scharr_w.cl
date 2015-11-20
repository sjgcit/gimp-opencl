
/*
 * Kernel Scharr 3x3 edge filter
 *
 * $Id: scharr_w.cl,v 1.1 2014/11/05 10:04:59 sjg Exp $
 *
 */

// menuentry Edge ( Scharr )

// byline

// kernel filter bufin bufout params

#define calc_g() \
    gy  = -( 3.0f*cols[2][0] + ( 10.0f*cols[2][1] ) + 3.0f*cols[2][2] ) ; \
    gy +=  ( 3.0f*cols[0][0] + ( 10.0f*cols[0][1] ) + 3.0f*cols[0][2] ) ; \
    \
    gx  =  3.0f * ( cols[0][0] - cols[0][2] ) ; \
    gx += 10.0f * ( cols[1][0] - cols[1][2] ) ; \
    gx +=  3.0f * ( cols[2][0] - cols[2][2] ) ; \
    \
    g = sqrt( ( gx*gx ) + ( gy*gy ) ) ; \
    \
    g /= 32.0f ;

/******************************************************************
 */

__kernel void filter_sc( __global uchar* bufin, __global uchar* bufout, __constant uf_t *cons )
{
    int w = WIDTH ;
    int h = HEIGHT ;
    
    int i = get_global_id(0) ;
    
    // process line by line
    
    int k = w*i ;
    
    float a ;
    
    // create a small private window for averaging
    
    float *cols[3] ;
    
    float c0[3], c1[3], c2[3] ;
    
    bool isstart = ( i == 0 ) ;
    bool isend   = ( i == h-1 ) ;
    
    // we will shift the window forward immediately on
    // entering the loop so we our colums forward of
    // their "natural" positions
    
    cols[0] = c0 ;
    cols[1] = c1 ;
    cols[2] = c2 ;
    
    c1[1] = convert_float( bufin[k] ) ;
    c1[0] = ( isstart ? c1[1] : convert_float( bufin[k-w] ) ) ;
    c1[2] = ( isend   ? c1[1] : convert_float( bufin[k+w] ) ) ;
    
    c2[0] = c1[0] ;
    c2[1] = c1[1] ;
    c2[2] = c1[2] ;
    
    int j = 0 ;
    
    float gx ;
    float gy ;
    
    float g ;
    
    float *tmp ;
    
    j = k + w - 1 ;

    while( k < j )
    {
        // shift window forward
        
        tmp = cols[0] ;
        
        cols[0] = cols[1] ;
        cols[1] = cols[2] ;
        
        cols[2] = tmp ;
        
        tmp[1] = convert_float( bufin[k+1] ) ;
        tmp[0] = ( isstart ? tmp[1] : convert_float( bufin[k+1-w] ) ) ;
        tmp[2] = ( isend   ? tmp[1] : convert_float( bufin[k+1+w] ) ) ;
        
        a = 0.0f ;
        
        calc_g() ;
        
        a = g ;
        
        bufout[ k ] = convert_uchar_sat( a ) ;
        
        k++ ;
    };
    
    // do the last column separately to simply main loop logic
    // as the main loop is done more often

    cols[0] = cols[1] ;
    cols[1] = cols[2] ;
    // cols[2] is the same
    
    a = 0.0f ;
    
    calc_g() ;
    
    a = g ;
    
    bufout[ k ] = convert_uchar_sat( a ) ;

}


/******************************************************************
 */


__kernel void filter( __global uchar4* bufin, __global uchar4* bufout, __constant uf_t *cons )
{
    int w = WIDTH ;
    int h = HEIGHT ;
    
    int i = get_global_id(0) ;
    
    // process line by line
    
    int k = w*i ;
    
    float4 a ;
    
    // create a small private window for averaging
    
    float4 *cols[3] ;
    
    float4 c0[3], c1[3], c2[3] ;
    
    bool isstart = ( i == 0 ) ;
    bool isend   = ( i == h-1 ) ;
    
    // we will shift the window forward immediately on
    // entering the loop so we our colums forward of
    // their "natural" positions
    
    cols[0] = c0 ;
    cols[1] = c1 ;
    cols[2] = c2 ;
    
    c1[1] = convert_float4( bufin[k] ) ;
    c1[0] = ( isstart ? c1[1] : convert_float4( bufin[k-w] ) ) ;
    c1[2] = ( isend   ? c1[1] : convert_float4( bufin[k+w] ) ) ;
    
    c2[0] = c1[0] ;
    c2[1] = c1[1] ;
    c2[2] = c1[2] ;
    
    int j = 0 ;
    
    float4 gx ;
    float4 gy ;
    
    float4 g ;
    
    float4 *tmp ;
    
    j = k + w - 1 ;

    while( k < j )
    {
        // shift window forward
        
        tmp = cols[0] ;
        
        cols[0] = cols[1] ;
        cols[1] = cols[2] ;
        
        cols[2] = tmp ;
        
        tmp[1] = convert_float4( bufin[k+1] ) ;
        tmp[0] = ( isstart ? tmp[1] : convert_float4( bufin[k+1-w] ) ) ;
        tmp[2] = ( isend   ? tmp[1] : convert_float4( bufin[k+1+w] ) ) ;
        
        a = 0.0f ;
        
        calc_g() ;
        
        a = g ;
        
        a.w = 255.0f ;
        
        bufout[ k ] = convert_uchar4_sat( a ) ;
        
        k++ ;
    };
    
    // do the last column separately to simply main loop logic
    // as the main loop is done more often

    cols[0] = cols[1] ;
    cols[1] = cols[2] ;
    // cols[2] is the same
    
    a = 0.0f ;
    
    calc_g() ;
    
    a = g ;
    
    a.w = 255.0f ;
    
    bufout[ k ] = convert_uchar4_sat( a ) ;

}



