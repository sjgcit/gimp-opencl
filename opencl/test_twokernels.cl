
// menuentry Test ( two kernels )

// bypixel

// buffer temp rw global w*h*c

// kernel first bufin temp

// kernel second temp bufout




__kernel void first(    __global uchar4* bufin,
                        __global uchar4* bufout
                   )
{
    int i = get_global_id(0) ;
    
    uchar4 c = bufin[i] ;
    uchar4 d ;
    
    d.x = 255 - c.x ;
    d.y = 255 - c.y ;
    d.z = 255 - c.z ;
    d.w = c.w ;
    
    bufout[i] = d ;
}



__kernel void second(   __global uchar4* bufin,
                        __global uchar4* bufout
                    )
{
    int i = get_global_id(0) ;
    
    uchar4 c = bufin[i] ;
    uchar4 d ;
    
    uchar v ;
    
    v = max( c.x, max( c.y, c.z ) ) ;
    
    d.x = v ;
    d.y = v ;
    d.z = v ;
    d.w = c.w ;
    
    bufout[i] = d ;
}



