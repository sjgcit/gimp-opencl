
/*
 * sjg_messages.h
 *
 * $Id: sjg_messages.h,v 1.2 2015/11/18 23:55:48 sjg Exp $
 *
 * (c) Stephen Geary, Nov 2015
 *
 * String constants in the sjgopencl plug-in to facilitate
 * easier language conversions.
 */

#ifndef __SJG_MESSAGES_H
#  define __SJG_MESSAGES_H


#define SJG_MESSAGE_001 "%s not an Open CL file extension"
#define SJG_MESSAGE_002 "Newer OpenCL source %s found so deleting old script file."
#define SJG_MESSAGE_003 "%s does not exist."

#define SJG_MESSAGE_004 ";\n; Autogenerated by the %s plugin\n;\n\n"
#define SJG_MESSAGE_005 "    \"Auto generated by sjg-opencl to trigger %s.\"\n"
#define SJG_MESSAGE_006 "    \"Auto generated by sjg-opencl.\"\n"
#define SJG_MESSAGE_007 "    \"2015-Nov-18\"\n"
#define SJG_MESSAGE_008 "%s found"
#define SJG_MESSAGE_009 "%s :: quit called\n"

#define SJG_MESSAGE_010 "Run mode"
#define SJG_MESSAGE_011 "Input image"
#define SJG_MESSAGE_012 "Input drawable"
#define SJG_MESSAGE_013 "File name"
#define SJG_MESSAGE_014 "Size of array"
#define SJG_MESSAGE_015 "Array of float parameters"

#define SJG_MESSAGE_016 "Open CL Testbed"
#define SJG_MESSAGE_017 "Test  plug-in for Open CL development"
#define SJG_MESSAGE_018 "Copyright "
#define SJG_MESSAGE_019 "2015"
#define SJG_MESSAGE_020 "_Open CL Testbed"
#define SJG_MESSAGE_021 "<Image>/Filters/Generic"

#define SJG_MESSAGE_022 "Error checking "
#define SJG_MESSAGE_023 "clmem ref is NULL"
#define SJG_MESSAGE_024 "cl_mem %p is valid with ref count %d"
#define SJG_MESSAGE_025 "cl_mem %p is valid with map count %d"
#define SJG_MESSAGE_026 "cl_mem %p is valid with size %zd"
#define SJG_MESSAGE_027 "cl_mem %p is valid with host ptr %p"
#define SJG_MESSAGE_028 "Unrecognized event status."
#define SJG_MESSAGE_029 "Event into check failed."
#define SJG_MESSAGE_030 "Event not completed : status = %d"
#define SJG_MESSAGE_031 "event has command type was %x"

#define SJG_MESSAGE_032 "This plugin requires an RGBA or Single Channel image to operate."
#define SJG_MESSAGE_033 "This plugin requires an RGB image with an Alpha channel."
#define SJG_MESSAGE_034 "trying to add alpha channel ..."
#define SJG_MESSAGE_035 "Could not add required alpha channel to layer."
#define SJG_MESSAGE_036 "Alpha channel added."
#define SJG_MESSAGE_037 "Drawable is not a layer - cannot add alpha channel."
#define SJG_MESSAGE_038 "OpenCL Plugin only works on RGB, RGBA or single channel images."
#define SJG_MESSAGE_039 "try file name [%s]\n"
#define SJG_MESSAGE_040 "%s cannot be read."
#define SJG_MESSAGE_041 "Kernel source file cannot be accessed."

#define SJG_MESSAGE_042 "No comments found with embedded script instructions."
#define SJG_MESSAGE_043 "Tried but failed to set CPU_MAX_COMPUTE_UNITS = 1"
#define SJG_MESSAGE_044 "Set CPU_MAX_COMPUTE_UNITS = 1"
#define SJG_MESSAGE_045 "Could not find GPU or CPU."
#define SJG_MESSAGE_046 "OpenCL device type is %d."
#define SJG_MESSAGE_047 "Could not create program from sources"
#define SJG_MESSAGE_048 "Build Log for program : %s"
#define SJG_MESSAGE_049 "OpenCL compiler options : %s"

#define SJG_MESSAGE_050 "onecu was used"
#define SJG_MESSAGE_051 "Cannot reference a different layer %s for writing."
#define SJG_MESSAGE_052 "Cannot reference a mask %s for writing."
#define SJG_MESSAGE_053 "Expected layer index number offset for buffer %s."
#define SJG_MESSAGE_054 "Layer index offset is outside available range for buffer %s."
#define SJG_MESSAGE_055 "Layer %s size different to base layer."
#define SJG_MESSAGE_056 "Could not fetch drawable for layer %s."
#define SJG_MESSAGE_057 "Could not allocate memory for layer %s."
#define SJG_MESSAGE_058 "Cannot fetch mask %s as source is not a layer."
#define SJG_MESSAGE_059 "Mask %s could not be found."

#define SJG_MESSAGE_060 "Could not fetch drawable for mask %s."
#define SJG_MESSAGE_061 "Could not allocate memory for mask %s."
#define SJG_MESSAGE_062 "Expected valid expression."
#define SJG_MESSAGE_063 "Kernel named %s requested"
#define SJG_MESSAGE_064 "Kernel %s could not be created."
#define SJG_MESSAGE_065 "Kernel %s created as cl ref %p"
#define SJG_MESSAGE_066 "Named buffer [ %s ] not declared."
#define SJG_MESSAGE_067 "Buffer named %s requested"
#define SJG_MESSAGE_068 "Kernel %s had no buffers specified\n"
#define SJG_MESSAGE_069 "Reusing kernel argument for %s"

#define SJG_MESSAGE_070 "Local memory argument of size %zd named %s"
#define SJG_MESSAGE_071 "Created device buffer OK for %s"
#define SJG_MESSAGE_072 "argument %s is global buffer of size %zd"
#define SJG_MESSAGE_073 "Error setting up argument %d for kernel %s"
#define SJG_MESSAGE_074 "About to enqueue kernel %s ( cl ref %p )"
#define SJG_MESSAGE_075 "global_size[0] = %zd"
#define SJG_MESSAGE_076 "Kernel %s failed to enqueue."
#define SJG_MESSAGE_077 "Waiting on kernel %s ..."
#define SJG_MESSAGE_078 "cl_event checked.  About to wait ..."
#define SJG_MESSAGE_079 "Kernel %s completed."

#define SJG_MESSAGE_080 "ERROR :: Corruption of buffer_list."
#define SJG_MESSAGE_081 "About to fetch buffer %s ( ptr = %p , size = %zd ) from clmemref %p"
#define SJG_MESSAGE_082 "Waiting for reading of buffer %s from device..."
#define SJG_MESSAGE_083 "Open CL completed"
#define SJG_MESSAGE_084 "Trying to release kernel %s..."
#define SJG_MESSAGE_085 "process() completed."

#define SJG_MESSAGE_086 "%d elements in floatarray passed to plugin."
#define SJG_MESSAGE_087 "OpenCL being processed..."


#endif /* __SJG_MESSAGES_H */

 
