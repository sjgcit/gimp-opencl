
/*
 * rgb2lab.cl
 *
 * (c) Stephen Geary, Aug 2014
 *
 * $Id: RGB_to_LAB.cl,v 1.1 2014/11/05 10:04:57 sjg Exp $
 */

// menuentry RGB to LAB

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

float Ffor(float t)
{
//    return ((t > 0.008856)? pow(t, (1.0/3.0)) : (7.787*t + 16.0/116.0));
    return ((t > 0.008856)? pow(t, (1.0/3.0)) : (7.787*t + 0.16/1.16));
}

float4 RGBtoLAB(float4 RGB)
{
    const float4 D65 = (float4)(0.9505, 1.0, 1.0890, 0.0);

    const float c1  = 0.16f ;
    const float c2  = 1.16f ;
    const float c21 = c1 / c2 ;

    float4 XYZ = RGBtoXYZ(RGB);

    float4 LAB;

    LAB.x = c2 *  Ffor(XYZ.y/D65.y) - c1 ;
    LAB.y = 5.0f * (Ffor(XYZ.x/D65.x) - Ffor(XYZ.y/D65.y));
    LAB.z = 2.0f * (Ffor(XYZ.y/D65.y) - Ffor(XYZ.z/D65.z));
    LAB.w = XYZ.w;
    
    // LAB A and B values can be negative
    
    LAB.y += 0.5f ;
    LAB.z += 0.5f ;

    return LAB;
}


__kernel void filter_b2b( __global uchar4* bufin, __global uchar4* bufout, __constant uf_t *cons )
{
    int i = get_global_id(0) ;

    float4 rgba = convert_float4( bufin[i] ) ;
    
    rgba = rgba / 255.0f ;
    
    rgba = RGBtoLAB( rgba ) ;

    rgba = rgba * 255.0f ;
    
    rgba = rgba + 0.5f ;
    
    bufout[i] = convert_uchar4_sat( rgba ) ;
}

