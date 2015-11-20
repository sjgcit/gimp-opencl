
/*
 * lab2rgb.cl
 *
 * (c) Stephen Geary, Aug 2014
 *
 * $Id: XYZ_to_RGB.cl,v 1.1 2014/11/05 10:05:04 sjg Exp $
 */

// menuentry XYZ to RGB

// kernel filter_b2b bufin bufout params

float4 XYZtoRGB(float4 XYZ)
{
    float4 val;

    val.x =  XYZ.x*3.2404542 - XYZ.y*1.5371385 - XYZ.z*0.4985341 ;
    val.y = -XYZ.x*0.9692660 + XYZ.y*1.8760108 + XYZ.z*0.0415560 ;
    val.z =  XYZ.x*0.0556434 - XYZ.y*0.2040259 + XYZ.z*1.0572252 ;

    val.x = (val.x<=0.0031308)? ( 12.92*val.x ) : ( ( 1.055 * pow( val.x, (1.0/2.4) ) ) - 0.055 ) ;
    val.y = (val.y<=0.0031308)? ( 12.92*val.y ) : ( ( 1.055 * pow( val.y, (1.0/2.4) ) ) - 0.055 ) ;
    val.z = (val.z<=0.0031308)? ( 12.92*val.z ) : ( ( 1.055 * pow( val.z, (1.0/2.4) ) ) - 0.055 ) ;

    val.w = XYZ.w;

    return val;
}


__kernel void filter_b2b( __global uchar4* bufin, __global uchar4* bufout, __constant uf_t *cons )
{
    int i = get_global_id(0) ;

    float4 rgba = convert_float4( bufin[i] ) ;
    
    rgba = rgba / 255.0f ;
    
    rgba = XYZtoRGB( rgba ) ;

    rgba = rgba * 255.0f ;
    
    rgba = rgba + 0.5f ;
    
    bufout[i] = convert_uchar4_sat( rgba ) ;
}

