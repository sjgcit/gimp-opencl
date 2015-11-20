
/*
 * Kernel Average 3x3 grid
 *
 * $Id: blur_3x3_w.cl,v 1.1 2014/11/05 10:04:43 sjg Exp $
 *
 */

// menuentry Blur ( 3x3 average )

// byline

// kernel filter_b2b bufin bufout params


__kernel void filter_b2b( __global uchar4* bufin,
                            __global uchar4* bufout,
                            __constant uf_t *cons
                          )
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
    int m = 0 ;
    int n = 0 ;
    
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
        
        for( m = 0 ; m < 3 ; m++ )
        {
            for( n = 0 ; n < 3 ; n++ )
            {
                a += cols[m][n] ;
            }
        }
        
        a /= 9.0f ;
        
        a.w = cols[1][1].w ;
        
        bufout[ k ] = convert_uchar4_sat( a ) ;
        
        k++ ;
    };
    
    // do the last column separately to simply main loop logic
    // as the main loop is done more often

    cols[0] = cols[1] ;
    cols[1] = cols[2] ;
    // cols[2] is the same
    
    a = 0.0f ;
    
    for( m = 0 ; m < 3 ; m++ )
    {
        for( n = 0 ; n < 3 ; n++ )
        {
            a += cols[m][n] ;
        }
    }
    
    a /= 9.0f ;
    
    a.w = cols[1][1].w ;
    
    bufout[ k ] = convert_uchar4_sat( a ) ;

}



