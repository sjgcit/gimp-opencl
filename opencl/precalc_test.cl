

// menuentry Test ( precalc kernel )

// byline

// buffer stats rw global (2*h*c)+16

// kernel precalc_rgba bufin stats params

// bypixel

// kernel filter_rgba bufin bufout stats params

#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable


__kernel void precalc_rgba( __global uchar4* bufin,
                            __global uchar4* bufout,
                            __constant uf_t *cons
                            )
{
    // calculate the maximum and minimum values
    
    // not even remotely efficient, just for testing !
    
    int i = get_global_id(0) ;
    
    int k ;
    
    uchar4 c ;
    
    int w = cons[0].i ;
    int h = cons[1].i ;
    
    k = w * i ;
    
    c = bufin[k] ;
    
    uchar4 maxval = c ;
    uchar4 minval = c ;
    
    // if we're the first count for threads completed.
    
    /* Note the "volatile" keyword.
     * Without the "volatile" the compiler might optimize
     * out the *ip as a constant reference that won't
     * change duting the busy wait loop.
     */
    
    __global volatile int *ip = (__global int *)( bufout ) ;
    
    int offset = sizeof(int) ;
    
    if( i == 0 )
    {
        atomic_xchg( ip, 0 ) ;
    }
    
    // barrier( CLK_GLOBAL_MEM_FENCE ) ;
    
    int j ;
    
    for( j = 1 ; j < w ; j++ )
    {
        c = bufin[k+j] ;
    
        minval = min( minval, c ) ;
        maxval = max( maxval, c ) ;
    };
    
    k = 2*i ;
    
    bufout[k+offset]   = minval ;
    bufout[k+offset+1] = maxval ;
    
    // printf( "min, max = ( %v4hhx ) , ( %v4hhx )\n", minval, maxval ) ;
    
    // mark the thread as finished
    
    atomic_inc( ip ) ;
    
    if( i == 0 )
    {
        // busy wait until *ip == h
        
        while( *ip != h )
        {
        };
        
        // should be finished
        // can final pass
        
        minval = bufout[offset] ;
        maxval = bufout[offset+1] ;
    
        for( j = 2 ; j < 2*h ; j+=2 )
        {
            minval = min( minval, bufout[offset+j] ) ;
            maxval = max( maxval, bufout[offset+j+1] ) ;
        };
        
        bufout[0] = minval ;
        bufout[1] = maxval ;
        
        printf( "OPENCL :: precalc_rgba is finished\n" ) ;
    }
}
    

__kernel void filter_rgba( __global uchar4* bufin,
                            __global uchar4* bufout,
                            __global uchar4* precalc,
                            __constant uf_t *cons
                           )
{
    int i = get_global_id(0) ;
    
    float4 b = convert_float4( bufin[i] ) ;
    
    float4 mn = convert_float4( precalc[0] ) ;
    float4 mx = convert_float4( precalc[1] ) ;
    
    mx -= mn ;
    
    mx = ( mx > 0.5f ? mx : 255.0f ) ;
    
    mx /= 255.0f ;
    
    float4 c ;
    
    c = ( b - mn )/mx ;
    
    c.w = b.w ;
    
    bufout[i] = convert_uchar4( c ) ;
}


