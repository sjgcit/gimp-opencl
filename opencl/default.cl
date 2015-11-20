
// menuentry Test ( half invert )

// kernel filter_b2b bufin bufout params


__kernel void filter_b2b( __global uchar4* bufin, __global uchar4* bufout, __constant uf_t *cons )
{
    int w = WIDTH ;
    int h = HEIGHT ;
    
    int i = get_global_id(0) ;
    
    uchar4 b = bufin[i] ;
    
    int hh = h * w ;
    
    hh = hh >> 1 ;
    
    uchar a = b.w ;
    
    b = ( i < hh ) ?  ~b : b ;
    
    b.w = a ;
    
    bufout[i] = b ;
}

