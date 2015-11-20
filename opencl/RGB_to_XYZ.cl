
/*
 * rgb2lab.cl
 *
 * (c) Stephen Geary, Aug 2014
 *
 * $Id: RGB_to_XYZ.cl,v 1.1 2014/11/05 10:04:58 sjg Exp $
 */

// menuentry RGB to XYZ

// kernel filter_b2b bufin bufout params

float4 RGBtoXYZ(float4 RGB)
{
    float r = ( RGB.x > 0.04045 ) ? pow( (RGB.x + 0.055)/1.055, 2.4 ) : ( RGB.x/12.92 ) ;
    float g = ( RGB.y > 0.04045 ) ? pow( (RGB.y + 0.055)/1.055, 2.4 ) : ( RGB.y/12.92 ) ;
    float b = ( RGB.z > 0.04045 ) ? pow( (RGB.z + 0.055)/1.055, 2.4 ) : ( RGB.z/12.92 ) ;

    return (float4)( ( r*0.4124564 + g*0.3575761 + b*0.1804375 ),
                       ( r*0.2126759 + g*0.7151522 + b*0.0721750 ),
                       ( r*0.0193339 + g*0.1191920 + b*0.9503041 ),
                       RGB.w
                    );
}

__kernel void filter_b2b( __global uchar4* bufin, __global uchar4* bufout, __constant uf_t *cons )
{
    int i = get_global_id(0) ;

    float4 rgba = convert_float4( bufin[i] ) ;
    
    rgba = rgba / 255.0f ;
    
    rgba = RGBtoXYZ( rgba ) ;
    
    rgba = rgba * 255.0f ;
    
    rgba = rgba + 0.5f ;
    
    bufout[i] = convert_uchar4_sat( rgba ) ;
}

