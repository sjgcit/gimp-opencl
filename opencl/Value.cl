

/*
 * Get the Value ( max(rgb) ) of image, removing color
 *
 * $Id: Value.cl,v 1.1 2014/11/05 10:05:02 sjg Exp $
 *
 * (c) S. Geary, Oct 2014
 */

// menentry Value

// kernel filter_b2b bufin bufout params

__kernel void filter_b2b( __global uchar4* bufin, __global uchar4* bufout, __constant uf_t *cons )
{
    int i = get_global_id(0) ;
    
    float4 rgb = convert_float4( bufin[i] ) ;
    float4 rgb2 ;
    
    float v ;
    float a ;
    
    a = rgb.w ;
    
    v = max( rgb.z, max( rgb.x, rgb.y ) ) ;

    rgb.x = v ;
    rgb.y = v ;
    rgb.z = v ;
    
    rgb.w = a ;
    
    bufout[i] = convert_uchar4_sat( rgb ) ;
}

