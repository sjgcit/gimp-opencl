/* #######################################################################
    Copyright 2011 Oscar Amoros Huguet, Cristian Garcia Marin

    This file is part of SimpleOpenCL

    SimpleOpenCL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3.

    SimpleOpenCL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with SimpleOpenCL. If not, see <http://www.gnu.org/licenses/>.

   ####################################################################### 

   SimpleOpenCL Version 0.010_27_02_2013 
   
   SJG OpenCL plugin private version : $Id: simpleCL.c,v 1.6 2014/11/10 12:25:12 sjg Exp $

*/

#ifdef __cplusplus
extern "C" {
#endif

#include "simpleCL.h"


#if ( !defined(DEBUGME) ) || ( !defined(DEBUG_SIMPLECL) )

#  define SJGDEBUGHEAD(l,f)
#  define SJGDEBUG()
#  define SJGDEBUGMSG(msg)
#  define SJGDEBUGF(...)
#  define SJGDEBUGF_REAL(lineno, func, ...)

#else

#  define SJGDEBUGHEAD(lineno,func)    fprintf(stderr,"SJG :: %d :: %s :: ", (lineno), (func) )

#  define SJGDEBUG()        { SJGDEBUGHEAD(__LINE__,__func__) ; fputc((int)'\n',stderr) ; fflush( stderr ) ; }

#  define SJGDEBUGMSG(msg)  { SJGDEBUGHEAD(__LINE__,__func__) ; fprintf( stderr,"%s\n", (msg) ) ; fflush( stderr ) ; }

#  define SJGDEBUGF_REAL(lineno,func , ...)  { \
                                SJGDEBUGHEAD((lineno),(func)) ; \
                                fprintf( stderr, __VA_ARGS__ ) ; \
                                fprintf( stderr, "\n" ) ; \
                                fflush( stderr ) ; \
                            }

#  define SJGDEBUGF(...)    SJGDEBUGF_REAL(__LINE__, __func__, __VA_ARGS__)

#endif



#define safe_free(ptr)  if( (ptr) != NULL ) \
                        { \
                            SJGDEBUGF( "freeing pointer :: %p", (ptr) ) ; \
                            free( (ptr) ) ; \
                            (ptr) = NULL ; \
                        }

#define CHECKPTR(ptr)   if( (ptr) == NULL ){ SJGDEBUG() ; goto err_exit ; }


/***************************************************************
 */

#define sclCasePrintf(__sym)   \
                                case CL_ ## __sym : \
                                    fprintf( stderr, "CL_" #__sym ) ; \
                                    break

void sclPrintErrorFlags( cl_int flag )
{
	switch (flag)
	{
		sclCasePrintf( DEVICE_NOT_FOUND ) ;
		sclCasePrintf( DEVICE_NOT_AVAILABLE) ;
		sclCasePrintf( COMPILER_NOT_AVAILABLE ) ;
		sclCasePrintf( PROFILING_INFO_NOT_AVAILABLE ) ;
		sclCasePrintf( MEM_COPY_OVERLAP ) ;
		sclCasePrintf( IMAGE_FORMAT_MISMATCH ) ;
		sclCasePrintf( IMAGE_FORMAT_NOT_SUPPORTED ) ;
		sclCasePrintf( INVALID_COMMAND_QUEUE ) ;
		sclCasePrintf( INVALID_CONTEXT ) ;
		sclCasePrintf( INVALID_MEM_OBJECT ) ;
		sclCasePrintf( INVALID_VALUE ) ;
		sclCasePrintf( INVALID_EVENT_WAIT_LIST ) ;
		sclCasePrintf( MEM_OBJECT_ALLOCATION_FAILURE ) ;
		sclCasePrintf( OUT_OF_HOST_MEMORY ) ;

		sclCasePrintf( INVALID_PROGRAM_EXECUTABLE ) ;
		sclCasePrintf( INVALID_KERNEL ) ;
		sclCasePrintf( INVALID_KERNEL_ARGS ) ;
		sclCasePrintf( INVALID_WORK_DIMENSION ) ;
#ifndef __APPLE__ 
		sclCasePrintf( INVALID_GLOBAL_WORK_SIZE ) ;
#endif
		sclCasePrintf( INVALID_WORK_GROUP_SIZE ) ;
		sclCasePrintf( INVALID_WORK_ITEM_SIZE ) ;
		sclCasePrintf( INVALID_GLOBAL_OFFSET ) ;
		sclCasePrintf( OUT_OF_RESOURCES ) ;

		sclCasePrintf( INVALID_PROGRAM ) ;
		sclCasePrintf( INVALID_KERNEL_NAME ) ;
		sclCasePrintf( INVALID_KERNEL_DEFINITION ) ;
		sclCasePrintf( INVALID_BUFFER_SIZE ) ;
		sclCasePrintf( BUILD_PROGRAM_FAILURE ) ;
		sclCasePrintf( INVALID_ARG_INDEX ) ;
		sclCasePrintf( INVALID_ARG_VALUE ) ;
		sclCasePrintf( MAP_FAILURE ) ;
		sclCasePrintf( MISALIGNED_SUB_BUFFER_OFFSET ) ;
		sclCasePrintf( EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST ) ;
		sclCasePrintf( INVALID_DEVICE_TYPE ) ;
		sclCasePrintf( INVALID_PLATFORM ) ;
		sclCasePrintf( INVALID_DEVICE ) ;
		sclCasePrintf( INVALID_QUEUE_PROPERTIES ) ;
		sclCasePrintf( INVALID_HOST_PTR ) ;
		sclCasePrintf( INVALID_IMAGE_FORMAT_DESCRIPTOR ) ;
		sclCasePrintf( INVALID_IMAGE_SIZE ) ;
		sclCasePrintf( INVALID_SAMPLER ) ;
		sclCasePrintf( INVALID_BINARY ) ;
		sclCasePrintf( INVALID_BUILD_OPTIONS ) ;
		sclCasePrintf( INVALID_ARG_SIZE ) ;
		sclCasePrintf( INVALID_EVENT ) ;
		sclCasePrintf( INVALID_OPERATION ) ;
		sclCasePrintf( INVALID_GL_OBJECT ) ;
		sclCasePrintf( INVALID_MIP_LEVEL ) ;
		sclCasePrintf( INVALID_PROPERTY ) ;
		default:
			fprintf( stderr, "\nUnknown error code: %d\n",flag);
	}
}

/***************************************************************
 */

/* SJG : modified to remove warning and tidy code
 */
char* _sclLoadProgramSource( const char *filename )
{ 
	struct stat statbuf;
	FILE *fh = NULL ;
	char *source = NULL ;
	size_t bytesread = 0 ;
	
	fh = fopen( filename, "r" );

	if( fh == NULL )
	{
		return NULL ;
    }
	
	stat( filename, &statbuf );

	source = (char *)malloc( statbuf.st_size + 1 );
	
	if( source == NULL )
	{
	    fclose(fh) ;
	
	    return NULL ;
	}
	
	// add a terminating nul
	
    source[ statbuf.st_size ] = 0 ;

	if( ( bytesread = fread( source, statbuf.st_size, 1, fh ) ) != 1 )
	{
	    // assume an error of some kind
	
	    free( source ) ;
	    
	    source = NULL ;
	}
    
	fclose( fh );
	
	return source; 
}
 
/***************************************************************
 */

cl_program _sclCreateProgram( char* program_source, cl_context context )
{
	cl_program program;
	cl_int err;
	
	/*
	SJGDEBUGF( " program_source = [ %p ]\n", program_source ) ;
	SJGDEBUGF( "&program_source = [ %p ]\n", &program_source ) ;
	SJGDEBUGF( " context        = [ %p ]\n", (char *)context ) ;
	*/
	
	program = clCreateProgramWithSource( context, 1, (const char**)&program_source, NULL, &err ) ;
	
	SJGDEBUG() ;
	
	CLDEBUG( err ) ;
	
	SJGDEBUG() ;
	
	return program;
}

/***************************************************************
 */

cl_int _sclBuildProgram( cl_program program, cl_device_id devices, char *options )
{
	cl_int err;
	char build_c[4096];
	
	err = clBuildProgram( program, 0, NULL, options, NULL, NULL ) ;
	
	CLDEBUG( err ) ;
	
   	if ( err != CL_SUCCESS )
   	{
		err = clGetProgramBuildInfo( program, devices, CL_PROGRAM_BUILD_LOG, 4096, build_c, NULL ) ;
		
		if( err == CL_SUCCESS )
		{
		    printf( "Build Log for program:\n%s\n", build_c ) ;
		}
	}
	
	return err ;
}

/***************************************************************
 */

cl_kernel _sclCreateKernel( sclSoft software )
{
	cl_kernel kernel ;
	cl_int err ;

	kernel = clCreateKernel( software.program, software.kernelName, &err ) ;
	
	CLDEBUG( err ) ;
	
	return kernel;
}

/***************************************************************
 */

cl_event sclLaunchKernel( sclHard hardware, sclSoft software, size_t *global_work_size, size_t *local_work_size)
{
	cl_event myEvent ;
	cl_int err ;

	err = clEnqueueNDRangeKernel( hardware.queue, software.kernel, 2, NULL, global_work_size, local_work_size, 0, NULL, &myEvent ) ;
	
	CLDEBUG( err ) ;

	sclFinish( hardware ) ;
	
	return myEvent ;
}

/***************************************************************
 */

cl_event sclEnqueueKernel( sclHard hardware, sclSoft software, size_t *global_work_size, size_t *local_work_size)
{
	cl_event myEvent ;
	cl_int err ;

	err = clEnqueueNDRangeKernel( hardware.queue, software.kernel, 2, NULL, global_work_size, local_work_size, 0, NULL, &myEvent ) ;
	
	CLDEBUG( err ) ;

	return myEvent ;
}

/***************************************************************
 */

void sclReleaseClSoft( sclSoft soft )
{
    if( soft.kernel != NULL )
    {
	    clReleaseKernel( soft.kernel ) ;
	}
	
	if( soft.program != NULL )
	{
	    clReleaseProgram( soft.program ) ;
	}
}

/***************************************************************
 */

void sclReleaseClHard( sclHard hardware )
{
    if( hardware.queue != NULL )
    {
    	clReleaseCommandQueue( hardware.queue ) ;
	}
	
	if( hardware.context != NULL )
	{
	    clReleaseContext( hardware.context ) ;
	}
}

/***************************************************************
 */

void sclRetainClHard( sclHard hardware )
{
	clRetainCommandQueue( hardware.queue ) ;
	
	clRetainContext( hardware.context ) ;
}

/***************************************************************
 */

void sclReleaseAllHardware ( sclHard* hardList, int listlen )
{
	int i ;

	for( i = 0 ; i < listlen ; ++i )
	{
		sclReleaseClHard( hardList[i] ) ;
	}
}

/***************************************************************
 */

void sclRetainAllHardware ( sclHard* hardList, int listlen )
{
	int i ;

	for( i = 0 ; i < listlen ; ++i )
	{
		sclRetainClHard( hardList[i] ) ;
	}
}

/***************************************************************
 */

void sclReleaseMemObject( cl_mem object )
{
	cl_int err ;

	err = clReleaseMemObject( object ) ;
	
	CLDEBUG( err ) ;
}

/***************************************************************
 */

/* SJG
 *
 * Reduce number of tmp buffer created to just one ( from three )
 * 
 */

void sclPrintDeviceNamePlatforms( sclHard* hardList, int listlen )
{
	int i ;
	cl_char tmpbuf[ sizeof(cl_char)*1024 ] ;

	for( i = 0 ; i < listlen ; ++i )
	{
		clGetPlatformInfo( hardList[i].platform, CL_PLATFORM_NAME, sizeof(tmpbuf), tmpbuf, NULL ) ;
		
	    fprintf( stderr, "\n  Device %d\n  Platform name : %s\n  Vendor : ", hardList[i].devNum, tmpbuf ) ;
	
		clGetPlatformInfo( hardList[i].platform, CL_PLATFORM_VENDOR, sizeof(tmpbuf), tmpbuf, NULL ) ;
		
		fprintf( stderr, "%s\n  Device name : \n", tmpbuf ) ;

		clGetDeviceInfo( hardList[i].device, CL_DEVICE_NAME, sizeof(tmpbuf), tmpbuf, NULL ) ;
		
		fprintf( stderr, "%s\n", tmpbuf );	
	}
}

/***************************************************************
 */

void sclPrintHardwareStatus( sclHard hardware )
{
	cl_int err ;
	char platform[100] ;
	cl_bool deviceAV ;

	err = clGetPlatformInfo( hardware.platform,
			CL_PLATFORM_NAME,
			sizeof( platform ),
			platform,
			NULL ) ;
	
	CLDEBUG( err ) ;
	
	if( err == CL_SUCCESS )
	{
	    fprintf( stdout, "\nPlatform object alive" ) ;
	}
	
	err = clGetDeviceInfo( hardware.device,
			CL_DEVICE_AVAILABLE,
			sizeof(cl_bool),
			(void*)(&deviceAV),
			NULL ) ;
			
	if( err == CL_SUCCESS && deviceAV )
	{
		fprintf( stdout, "\nDevice object alive and device available." ) ;
	}
	else if( err == CL_SUCCESS )
	{
		fprintf( stdout, "\nDevice object alive and device NOT available.") ;
	}
	else
	{
		fprintf( stdout, "\nDevice object not alive.") ;
	} 

}

/***************************************************************
 */

void _sclCreateQueues( sclHard* hardList, int listlen )
{
	int i ;
	cl_int err ;

	for( i = 0 ; i < listlen ; ++i )
	{
		hardList[i].queue = clCreateCommandQueue( hardList[i].context, hardList[i].device,
							 0 /* CL_QUEUE_PROFILING_ENABLE */, &err ) ;
							 
		if ( err != CL_SUCCESS )
		{
			fprintf( stderr, "\nError creating command queue %d", i ) ;
		}
	}
}

/***************************************************************
 */

void _sclSmartCreateContexts( sclHard* hardList, int listlen )
{
	cl_device_id deviceList[16];
	cl_context context;
	char var_queries1[1024];
	char var_queries2[1024];
	cl_int err;
	ptsclHard groups[10][20];
	int i, j, groupSet = 0;
	int groupSizes[10];
	int nGroups = 0;


	for( i = 0 ; i < listlen ; ++i )
	{
	    /* Group generation
	     */
	
		clGetPlatformInfo( hardList[i].platform, CL_PLATFORM_NAME, sizeof(var_queries1), var_queries1, NULL ) ;

		if( nGroups == 0 )
		{
			groups[0][0] = &( hardList[0] ) ;
			
			nGroups++ ;
			
			groupSizes[0] = 1 ;
		}
		else
		{
			groupSet = 0 ;
			
			for( j = 0 ; j < nGroups ; ++j )
			{
				clGetPlatformInfo( groups[j][0]->platform, CL_PLATFORM_NAME, sizeof(var_queries2), var_queries2, NULL ) ;
				
				if(    ( strcmp( var_queries1, var_queries2 ) == 0 )
				    && ( hardList[i].deviceType == groups[j][0]->deviceType )
				    && ( hardList[i].maxPointerSize == groups[j][0]->maxPointerSize )
				  )
				{
					groups[j][ groupSizes[j] ] = &( hardList[i] ) ;
					
					groupSizes[j]++ ;
					groupSet = 1 ;
				}
			}
			
			if( !groupSet )
			{
				groups[nGroups][0] = &( hardList[i] ) ;
				groupSizes[nGroups] = 1 ;
				nGroups++ ;
			}
		}
	}


	for( i = 0 ; i < nGroups ; ++i )
	{
	    /* Context generation */
	
		// fprintf( stdout, "\nGroup %d with %d devices", i+1, groupSizes[i] ) ;
		
		for( j = 0 ; j < groupSizes[i] ; ++j )
		{
			deviceList[j] = groups[i][j]->device ;
		}
		
		context = clCreateContext( 0, groupSizes[i], deviceList, NULL, NULL, &err ) ;
		
		if( err != CL_SUCCESS )
		{
			fprintf( stderr, "\nError creating context on device %d", i );
		}
        
		for( j = 0 ; j < groupSizes[i] ; ++j )
		{
			groups[i][j]->context = context ;
		}
	}
}

/***************************************************************
 */

int _sclGetMaxComputeUnits( cl_device_id device )
{
	cl_uint nCompU;

	clGetDeviceInfo( device, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(nCompU), (void *)&nCompU, NULL ) ;

	return (int)nCompU ;
}

/***************************************************************
 */

/* SJG
 *
 * Original definition was :
 *      unsigned long int _sclGetMaxMemAllocSize( cl_device_id device )
 *
 * But there is no certainty of the bit size of unsigned long int.
 *
 * The actual value is a cl_ulong and we simply use that type
 * which will resolve to some integer type.
 */
cl_ulong _sclGetMaxMemAllocSize( cl_device_id device )
{
	cl_ulong mem ;

 	clGetDeviceInfo( device, CL_DEVICE_MAX_MEM_ALLOC_SIZE, sizeof(mem), (void *)&mem, NULL ) ;

	return mem ;
}

/***************************************************************
 */

/* SJG
 *
 * Bug fix.
 *
 * Original version compares returned data as if they were strings.
 * They're not strings, but values of s the type cl_device_type
 */

int _sclGetDeviceType( cl_device_id device )
{
	int out = 2 ;

	cl_device_type type ;

 	clGetDeviceInfo( device, CL_DEVICE_TYPE, sizeof(type), &type, NULL ) ;
	
	if ( ( type & CL_DEVICE_TYPE_GPU ) == CL_DEVICE_TYPE_GPU )
	{
		out = 0;
	}
	else
	{
	    if( ( type & CL_DEVICE_TYPE_CPU ) == CL_DEVICE_TYPE_CPU )
	    {
		    out = 1;
	    }
    }
    
	return out;
}

/***************************************************************
 */

/* SJG
 *
 * Poorly named as it just returns the device with the most
 * computer units, which may or may not be faster on any
 * given task.
 */

sclHard sclGetFastestDevice( sclHard *hardList, int listlen )
{
	int i ;
	int maxCpUnits = 0 ;
	int device = 0 ;

	for( i = 0 ; i < listlen ; ++i )
	{
		// fprintf( stdout, "\nDevice %d Compute Units %d", i, hardList[i].nComputeUnits ) ;
		
		if ( maxCpUnits < hardList[i].nComputeUnits )
		{
			device = i ;
			maxCpUnits = hardList[i].nComputeUnits ;
		}
	}

	return hardList[ device ] ;
}

/***************************************************************
 */

sclHard *sclGetAllHardware( int *found )
{
	int i ;
	int j ;
	cl_uint nPlatforms=0 ;
	cl_uint nDevices=0 ;
	char *platformName = NULL ;
	sclHard *hardList = NULL ;
	
	*found = 0 ;

	cl_platform_id *GPUplatforms = NULL ;
	cl_platform_id *platforms = NULL ;
	cl_int err;
	cl_device_id *devices = NULL ;
	
	platforms = (cl_platform_id *)malloc( sizeof(cl_platform_id) * 8 ) ;
	CHECKPTR( platforms ) ;
	
	GPUplatforms = (cl_platform_id *)malloc( sizeof(cl_platform_id) * 8 ) ;
	CHECKPTR( GPUplatforms ) ;
	
	platformName = (char *)malloc( sizeof(char) * 30 ) ;
	CHECKPTR( platformName ) ;
	
	devices = (cl_device_id *)malloc( sizeof(cl_device_id) * 16 ) ;
	CHECKPTR( devices ) ;
	
	hardList = (sclHard*)malloc( 16*sizeof(sclHard) ) ;
	CHECKPTR( hardList ) ;

	err = clGetPlatformIDs( 8, platforms, &nPlatforms ) ;
	
	if ( nPlatforms == 0 )
	{
		fprintf( stderr, "\nNo OpenCL plantforms found.\n") ;
	}
	else
	{
		for( i = 0 ; i < (int)nPlatforms ; ++i )
		{
			err = clGetDeviceIDs( platforms[i], CL_DEVICE_TYPE_ALL, 16, devices, &nDevices ) ;
			
			if( nDevices == 0 )
			{
				fprintf( stderr, "\nNo OpenCL enabled device found." ) ;

				if( err != CL_SUCCESS )
				{
					printf( "\nError clGetDeviceIDs" ) ;
					sclPrintErrorFlags( err ) ;
				}
			}
			else
			{
				for( j = 0 ; j < (int)nDevices ; ++j )
				{
					hardList[ *found ].platform       = platforms[ i ] ;
					hardList[ *found ].device         = devices[ j ] ;
					hardList[ *found ].nComputeUnits  = _sclGetMaxComputeUnits( hardList[ *found ].device ) ;
					hardList[ *found ].maxPointerSize = _sclGetMaxMemAllocSize( hardList[ *found ].device ) ;				
					hardList[ *found ].deviceType     = _sclGetDeviceType( hardList[ *found ].device ) ;
					hardList[ *found ].devNum         = *found ;
					
					(*found)++ ;
				}
			}
		}

		_sclSmartCreateContexts( hardList, *found ) ;
		_sclCreateQueues( hardList, *found ) ;
	}

#ifdef DEBUG
	sclPrintDeviceNamePlatforms( hardList, *found ) ;
#endif

	sclRetainAllHardware( hardList, *found ) ;
	
	return hardList ;

    /* if something went wrong ...
     */
     
err_exit:

    safe_free( platforms ) ;
    safe_free( GPUplatforms ) ;
    safe_free( platformName ) ;
    safe_free( devices ) ;
    safe_free( hardList ) ;
    
    *found = 0 ;
    
    return NULL ;
}

/***************************************************************
 */

/* SJG
 *
 * sclGetCPUHardware() and sclGetGPUHardware() were identical
 * part from variable names and the specific constants
 * CL_DEVICE_TYPE_CPU and CL_DEVICE_TYPE_GPU
 *
 * They have been changed to use one new main function
 * which is sclGetSpecificHardware()
 *
 * Value set in found parameter is 1 for true for 0 for false ( not found ).
 */

sclHard sclGetCPUHardware( int nDevice, int* found )
{
    SJGDEBUG() ;

    return sclGetSpecificHardware( nDevice, CL_DEVICE_TYPE_CPU, found ) ;
}


sclHard sclGetGPUHardware( int nDevice, int* found )
{
    SJGDEBUG() ;

    return sclGetSpecificHardware( nDevice, CL_DEVICE_TYPE_GPU, found ) ;
}


sclHard sclGetSpecificHardware( int nDevice, int type, int *found )
{
	int i = 0 ;
	int nTotalDevs = 0 ;
	int nPUplatforms = 0 ;
	cl_platform_id *PUplatforms = NULL ;
	sclHard hardware ;
	cl_int err ;
	cl_uint nPlatforms = 0 ;
	cl_uint nDevices = 0 ;
	cl_platform_id *platforms = NULL ;
	cl_device_id *devices = NULL ;
	char *platformName = NULL ;

	*found = 0 ;
	
	if( ( type != CL_DEVICE_TYPE_CPU ) && ( type != CL_DEVICE_TYPE_GPU ) )
	{
	    SJGDEBUG() ;
	
	    return hardware ;
	}
	
	platforms = (cl_platform_id *)malloc( sizeof(cl_platform_id) * 8 ) ;
	CHECKPTR( platforms ) ;
	
	PUplatforms = (cl_platform_id *)malloc( sizeof(cl_platform_id) * 8 ) ;
	CHECKPTR( PUplatforms ) ;
	
	platformName = (char *)malloc( sizeof(char) * 30 ) ;
	CHECKPTR( platformName ) ;
	
	devices = (cl_device_id *)malloc( sizeof(cl_device_id) * 8 ) ;
	CHECKPTR( devices ) ;
	
    SJGDEBUG() ;

	err = clGetPlatformIDs( 8, platforms, &nPlatforms ) ;
	
    SJGDEBUG() ;

	/* printf("\n Number of platforms found: %d \n",nPlatforms) ;
	 */

	if( nPlatforms == 0 )
	{
        SJGDEBUG() ;

		printf("\nNo OpenCL plantforms found.\n") ;
		
		*found = 0 ;
	}
	else if( nPlatforms == 1 )
	{
        SJGDEBUG() ;

		hardware.platform = platforms[0] ;
		
		err = clGetDeviceIDs( hardware.platform, type, 8, devices, &nDevices ) ;
		
		// CLDEBUG( err ) ;
		
		if( nDevices == 0 )
		{
            SJGDEBUG() ;

			printf("\nNo OpenCL enabled GPU found.\n") ;
			
			*found = 0 ;
		}
		else
		{
		    *found = 1 ;
		}
	}
	else
	{
        SJGDEBUG() ;

		for ( i = 0; i < (int)nPlatforms; ++i )
		{
            SJGDEBUG() ;

			err = clGetDeviceIDs( platforms[i], type, 8, devices + nTotalDevs, &nDevices ) ;
			
            SJGDEBUG() ;

			// CLDEBUG( err ) ;
			
			nTotalDevs += (int)nDevices ;
			
			if( nDevices > 0 )
			{
                SJGDEBUG() ;

				PUplatforms[nPUplatforms] = platforms[i] ;
				nPUplatforms++ ; 
			}  
		}
		
		if( nPUplatforms == 0 )
		{
			// printf( "\nNo OpenCL enabled GPU found.\n" ) ;
			
            SJGDEBUG() ;

			*found = 0 ;
		}
		else if( nPUplatforms == 1 )
		{
            SJGDEBUG() ;

			hardware.platform = PUplatforms[0] ;
			hardware.device = devices[nDevice] ;
			
			*found = 1 ;
		}
		else
		{
            SJGDEBUG() ;

			err = clGetPlatformInfo( PUplatforms[0], CL_PLATFORM_VENDOR, (size_t)30, (void *)platformName, NULL) ;
		    
            SJGDEBUG() ;

			CLDEBUG( err ) ;
			
            SJGDEBUG() ;

			// printf( "\nMore than one OpenCL platform with enabled GPU's.\nUsing: %s\n", platformName );
			
			hardware.platform = PUplatforms[0] ;
			hardware.device = devices[nDevice] ;
			
			*found = 1 ;
		}

	}

    SJGDEBUG() ;

	
	if( *found == 1 )
	{
        SJGDEBUG() ;

		/* Create context
		 */
		hardware.device = devices[nDevice] ;
		
		// hardware.context = clCreateContext( 0, 1, &hardware.device, NULL, NULL, &err ) ;
		
		hardware.context = clCreateContext( NULL, nDevices, devices, NULL, NULL, &err ) ;
		
        SJGDEBUG() ;

		if( err != CL_SUCCESS )
		{
            SJGDEBUG() ;

			fprintf( stderr, "\nError 3" ) ;
			sclPrintErrorFlags( err ) ;
		}

        SJGDEBUG() ;

		/* Create command queue
		 */
		hardware.queue = clCreateCommandQueue( hardware.context, hardware.device, 0 /* CL_QUEUE_PROFILING_ENABLE */, &err ) ;
		
        SJGDEBUG() ;

		if ( err != CL_SUCCESS )
		{
            SJGDEBUG() ;

			fprintf( stderr, "\nError 3.1" ) ;
			sclPrintErrorFlags( err ) ;
		}
	}

err_exit:

    safe_free( platforms ) ;
    safe_free( PUplatforms ) ;
    safe_free( platformName ) ;
    safe_free( devices ) ;
    
	SJGDEBUG() ;
	
    return hardware ;
}

/***************************************************************
 */

/* SJG
 *
 * PROBLEM :: This code assumes no errors and has no way to
 * report an error to the caller.
 *
 * Added code to ensure that software.kernel is set to NULL
 * as a default and the user can check it is not on return.
 *
 * Original function split to allow a function that loads
 * directly from an already loaded source file in memory.
 *
 * Added compiler options parameter.
 */

sclSoft sclGetCLSoftware( char* path, char* name, sclHard hardware, char *options )
{
	sclSoft software ;
	
	// SJG
	software.kernel = NULL ;
	
	/* Load program source
	 */
	char *source = NULL ;
	
	source = _sclLoadProgramSource( path ) ;
	
	if( source == NULL )
	{
	    return software ;
	}
	
	sclGetCLSoftwareFromMemory( source, name, hardware, &software, options ) ;
	
	return software ;
}

/***************************************************************
 *
 * Split from original sclGetSoftware() function to allow
 * an entry that loads from a preloaded file in memory.
 */


void sclGetCLSoftwareFromMemory( char *source, char *name, sclHard hardware, sclSoft *softp, char *options )
{
    if( softp == NULL )
        return ;

    if( source == NULL )
        return ;
    
    cl_int err = CL_SUCCESS ;

	SJGDEBUG() ;
	
	/* SJG :: this version is safer than the original
	 * as it ensures a buffer overrun cannot happen
	 */
	if( name != NULL )
	{
	    strncpy( softp->kernelName, name, SCL_MAX_KERNEL_NAME_SIZE ) ;
	}
	else
	{
	    softp->kernelName[0] = 0 ;
	}
	
	SJGDEBUG() ;
	
	softp->kernelName[SCL_MAX_KERNEL_NAME_SIZE - 1] = 0 ;
	
	SJGDEBUG() ;
	
	/* Create program objects from source
	 */
	softp->program = _sclCreateProgram( source, hardware.context ) ;
	
	SJGDEBUG() ;
	
	if( softp->program != NULL )
	{
    	SJGDEBUG() ;
    	
	    /* Build the program (compile it)
       	 */
       	err = _sclBuildProgram( softp->program, hardware.device, options ) ;
       	
       	if( err == CL_SUCCESS )
       	{
        	SJGDEBUG() ;
	
           	/* Create the kernel object
	         */
	        softp->kernel = _sclCreateKernel( *softp ) ;
	    }
	}
	
	SJGDEBUG() ;
}

/***************************************************************
 */

cl_mem sclMalloc( sclHard hardware, cl_int mode, size_t size )
{
	cl_mem buffer = NULL ;
	cl_int err ;
	
	buffer = clCreateBuffer( hardware.context, mode, size, NULL, &err ) ;
	
	CLDEBUG(err) ;
		
	return buffer;
}	

/***************************************************************
 */

cl_mem sclMallocWrite( sclHard hardware, cl_int mode, size_t size, void* hostPointer )
{
	cl_mem buffer = NULL ;
	cl_int err ;
	
    buffer = clCreateBuffer( hardware.context, mode, size, NULL, &err ) ;
    
    CHECKPTR( buffer ) ;
    
    CLDEBUG( err ) ;
    
    if( err == CL_SUCCESS )
    {
    	err = clEnqueueWriteBuffer( hardware.queue, buffer, CL_TRUE, 0, size, hostPointer, 0, NULL, NULL ) ;
	}
	
	CLDEBUG( err ) ;
	
err_exit:

	return buffer ;
}

/***************************************************************
 */

void sclWrite( sclHard hardware, size_t size, cl_mem buffer, void* hostPointer )
{
	cl_int err;

	err = clEnqueueWriteBuffer( hardware.queue, buffer, CL_TRUE, 0, size, hostPointer, 0, NULL, NULL );
	
	CLDEBUG( err ) ;
}

/***************************************************************
 */

void sclRead( sclHard hardware, size_t size, cl_mem buffer, void *hostPointer )
{
	cl_int err ;

	err = clEnqueueReadBuffer( hardware.queue, buffer, CL_TRUE, 0, size, hostPointer, 0, NULL, NULL ) ;
	
	CLDEBUG( err ) ;
}

/***************************************************************
 */

cl_int sclFinish( sclHard hardware )
{
	cl_int err;

	err = clFinish( hardware.queue ) ;
	
	CLDEBUG( err ) ;

	return err;
}

/***************************************************************
 */

cl_ulong sclGetEventTime( sclHard hardware, cl_event event )
{
	cl_ulong elapsedTime, startTime, endTime ;

	sclFinish( hardware ) ;

	clGetEventProfilingInfo( event, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &startTime, NULL) ;
	clGetEventProfilingInfo( event, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &endTime, NULL) ;

	elapsedTime = endTime-startTime ;

	return elapsedTime ;
}

/***************************************************************
 */

void sclSetKernelArg( sclSoft software, int argnum, size_t typeSize, void *argument )
{
	cl_int err ;

	err = clSetKernelArg( software.kernel, argnum, typeSize, argument ) ;
	
	if ( err != CL_SUCCESS )
	{
		fprintf( stderr, "\nError clSetKernelArg number %d\n", argnum ) ;
		sclPrintErrorFlags( err ) ;
	}
}

/***************************************************************
 */

void _sclWriteArgOnAFile( int argnum, void* arg, size_t size, const char* diff )
{
	FILE *out = NULL ;
	char filename[150] ;

	snprintf( filename, 150, "../data/arg%d%s", argnum, diff ) ;

	out = fopen( filename, "w+" ) ;
	
	if( out != NULL )
	{
	    fwrite( arg, 1, size, out ) ;

	    fclose(out) ;
	}
}

/***************************************************************
 */

inline void _sclVSetKernelArgs( sclSoft software, const char *sizesValues, va_list argList )
{
	const char *p = NULL ;
	int argCount = 0 ;
	void* argument = NULL ;
	size_t actual_size ;
	
	for( p = sizesValues; *p != '\0'; p++ )
	{
		if( *p == '%' )
		{
			switch( *++p )
			{
				case 'a' :
					actual_size = va_arg( argList, size_t ) ;
					argument = va_arg( argList, void* ) ;
					sclSetKernelArg( software, argCount, actual_size, argument ) ;
					argCount++ ;
					break ;

				case 'v' :
					argument = va_arg( argList, void* ) ;
					sclSetKernelArg( software, argCount, sizeof(cl_mem) , argument ) ;
					argCount++ ;
					break ;

				case 'N' :
					actual_size = va_arg( argList, size_t ) ;
					sclSetKernelArg( software, argCount, actual_size, NULL ) ;
					argCount++;
					break;
				default:
					break;

			}
		}
	}
}

/***************************************************************
 */

void sclSetKernelArgs( sclSoft software, const char *sizesValues, ... )
{
	va_list argList ;

	va_start( argList, sizesValues ) ;

	_sclVSetKernelArgs( software, sizesValues, argList ) ;

	va_end( argList ) ;
}

/***************************************************************
 */

cl_event sclSetArgsLaunchKernel(    sclHard hardware,
                                    sclSoft software,
                                    size_t *global_work_size,
                                    size_t *local_work_size,
				                    const char *sizesValues,
				                    ... )
{
	va_list argList ;
	cl_event event ;

	va_start( argList, sizesValues ) ;
	
	_sclVSetKernelArgs( software, sizesValues, argList ) ;
	
	va_end( argList ) ;

	event = sclLaunchKernel( hardware, software, global_work_size, local_work_size ) ;

	return event ;
}

/***************************************************************
 */

cl_event sclSetArgsEnqueueKernel(   sclHard hardware,
                                    sclSoft software,
                                    size_t *global_work_size,
                                    size_t *local_work_size,
				                    const char *sizesValues,
				                    ... )
{
	va_list argList ;
	cl_event event ;

	va_start( argList, sizesValues ) ;
	
	_sclVSetKernelArgs( software, sizesValues, argList ) ;
	
	va_end( argList ) ;

	event = sclEnqueueKernel( hardware, software, global_work_size, local_work_size ) ;

	return event ;
}

/***************************************************************
 */

cl_event sclManageArgsLaunchKernel( sclHard hardware,
                                    sclSoft software,
                                    size_t *global_work_size,
                                    size_t *local_work_size,
				                    const char* sizesValues,
				                    ... )
{
	va_list argList ;
	cl_event event ;
	const char *p ;
	int argCount = 0, outArgCount = 0, inArgCount = 0, i ;
	void* argument ;
	size_t actual_size ;
	cl_mem outBuffs[30] ;
	cl_mem inBuffs[30] ;
	size_t sizesOut[30] ;
	// typedef unsigned char* puchar ;
	// puchar outArgs[30] ;
	char *outArgs[30] ;

	va_start( argList, sizesValues ) ;

	for( p = sizesValues; *p != '\0'; p++ )
	{
		if( *p == '%' )
		{
			switch( *++p )
			{
				case 'a': /* Single value non pointer argument */
					actual_size = va_arg( argList, size_t ) ;
					argument = va_arg( argList, void* ) ;
					sclSetKernelArg( software, argCount, actual_size, argument ) ;
					argCount++ ;
					break ;

				case 'v': /* Buffer or image object void* argument */
					argument = va_arg( argList, void* ) ;
					sclSetKernelArg( software, argCount, sizeof(cl_mem) , argument ) ;
					argCount++ ;
					break ;

				case 'N': /* Local memory object using NULL argument */
					actual_size = va_arg( argList, size_t ) ;
					sclSetKernelArg( software, argCount, actual_size, NULL ) ;
					argCount++ ;
					break ;
				
				case 'w': /* */
					sizesOut[ outArgCount ] = va_arg( argList, size_t ) ;
					outArgs[ outArgCount ] = (char *)va_arg( argList, void* ) ;
					outBuffs[ outArgCount ] = sclMalloc(    hardware,
					                                        CL_MEM_WRITE_ONLY,
                                                            sizesOut[ outArgCount ]
                                                       ) ;
					sclSetKernelArg( software, argCount, sizeof(cl_mem), &outBuffs[ outArgCount ] ) ;
					argCount++ ;
					outArgCount++ ;
					break ;
					
				case 'r': /* */
					actual_size = va_arg( argList, size_t ) ;
					argument = va_arg( argList, void* ) ;
					inBuffs[ inArgCount ] = sclMallocWrite( hardware,
					                                        CL_MEM_READ_ONLY,
					                                        actual_size,
					                                        argument
					                                      ) ;
					sclSetKernelArg( software, argCount, sizeof(cl_mem), &inBuffs[ inArgCount ] ) ;
					inArgCount++ ;
					argCount++ ;
					break ;
					
				case 'R': /* */
					sizesOut[ outArgCount ] = va_arg( argList, size_t ) ;
					outArgs[ outArgCount ] = (char *)va_arg( argList, void* ) ;
					outBuffs[ outArgCount ] = sclMallocWrite(   hardware,
					                                            CL_MEM_READ_WRITE, 
										                        sizesOut[ outArgCount ],
										                        outArgs[ outArgCount ]
										                    ) ;
					sclSetKernelArg( software, argCount, sizeof(cl_mem), &outBuffs[ outArgCount ] ) ;
					argCount++ ;
					outArgCount++ ;
					break ;
					
				case 'g':
					actual_size = va_arg( argList, size_t ) ;
					inBuffs[ inArgCount ] = sclMalloc( hardware, CL_MEM_READ_WRITE, actual_size ) ;
					sclSetKernelArg( software, argCount, sizeof(cl_mem), &inBuffs[ inArgCount ] ) ;
					inArgCount++ ;
					argCount++ ;
					break ;
					
				default:
					break ;
			}
		}
	}
	
	va_end( argList ) ;

	event = sclLaunchKernel( hardware, software, global_work_size, local_work_size ) ;
	
	for( i = 0; i < outArgCount; i++ )
	{
		sclRead( hardware, sizesOut[i], outBuffs[i], outArgs[i] ) ;
	}

	sclFinish( hardware ) ;
	
	for( i = 0; i < outArgCount; i++ )
	{
		sclReleaseMemObject( outBuffs[i] ) ;
	}

	for( i = 0; i < inArgCount; i++ )
	{
		sclReleaseMemObject( inBuffs[i] ) ;
	}

	return event ;
}

/***************************************************************
 */

#ifdef __cplusplus
}
#endif


