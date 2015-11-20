
/*
 * lab2rgb.cl
 *
 * (c) Stephen Geary, Aug 2014
 *
 * $Id: LAB_to_RGB.cl,v 1.1 2014/11/05 10:04:53 sjg Exp $
 */

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


float4 LABtoRGB(float4 LAB)
{
    const float4 D65 = (float4)( 0.9505, 1.0, 1.0890, 0.0 ) ;
    
    const float c1  = 0.16f ;
    const float c2  = 1.16f ;
    const float c21 = c1 / c2 ;

    float fy = ( LAB.x + c1 ) / c2 ;
    float fx = fy + ( ( LAB.y - 0.5f ) / 5.0f ) ;
    float fz = fy - ( ( LAB.z - 0.5f ) / 2.0f ) ;

    float delta = 6.0/29.0 ;

    float4 XYZ = (float4)(
                            (fx > delta)? D65.x * (fx*fx*fx) : (fx - c1/c2)*3*(delta*delta)*D65.x,
                            (fy > delta)? D65.y * (fy*fy*fy) : (fy - c1/c2)*3*(delta*delta)*D65.y,
                            (fz > delta)? D65.z * (fz*fz*fz) : (fz - c1/c2)*3*(delta*delta)*D65.z,
                             LAB.w
                           );

    return XYZtoRGB(XYZ);
}


__kernel void filter_b2b( __global uchar4* bufin, __global uchar4* bufout, __constant uf_t *cons )
{
    int i = get_global_id(0) ;

    float4 rgba = convert_float4( bufin[i] ) ;
    
    rgba = rgba / 255.0f ;
    
    rgba = LABtoRGB( rgba ) ;

    rgba = rgba * 255.0f ;
    
    rgba = rgba + 0.5f ;
    
    bufout[i] = convert_uchar4_sat( rgba ) ;
}

