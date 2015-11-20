
// menuentry Test ( pixel from global ID etc. )

// kernel paint_ids bufin bufout params

__kernel void paint_ids( __global uchar4* bufin, __global uchar4* bufout, __constant int *width )
{
    uint i = get_global_id(0) ;
    uint j = get_local_id(0) ;
    uint k = get_group_id(0) ;
    
    uint r,g,b ;
    
    r = i % 256 ;
    
    g = j % 256 ;
    
    b = k % 256 ;
    
    // set pixel values based on global_id and compute_unit_id
    
    bufout[i] = convert_uchar4_sat( (uint4)( 0, g, b, 255 ) ) ;
    
    bufout[i].w = bufin[i].w ;
}

