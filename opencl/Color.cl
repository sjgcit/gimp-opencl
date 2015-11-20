

/*
 * Get the color of image, removing luminance information
 *
 * $Id: Color.cl,v 1.1 2014/11/05 10:04:46 sjg Exp $
 *
 * (c) S. Geary, Oct 2014
 */

// menuentry Value Equalizer

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

    rgb2 = ( rgb * 255.0f / v ) ;
    
    rgb2.w = a ;
    
    rgb = rgb2 ;
    
    bufout[i] = convert_uchar4_sat( rgb ) ;
}

