
<!---
Readme.md for sjgopencl2 GIMP plug-in

$Id: Readme.md,v 1.2 2015/11/19 19:17:26 sjg Exp $

(c) Stephen Geary, Nov 2015
-->

Sjg-OpenCL2 plug-in for GIMP 2.8
===============================


What Is It ?
------------

The **sjg-opencl2** plug-in allows users to define filters using OpenCL source files.

Filters can use multi-stage kernels with named buffers.

Users can write their own filters ( in OpenCL source code ) and it will be executed
by this plug-in.

The plug-in also supports a number of features allowing the OpenCL file to contain
directives that control parameters required by the file.  Script-fu ( Schema ) files
will automaticaly be generated for the filters which slot the required menu entry into
GIMP's UI and also provide a basic UI for the adjustment of parameters.


What is it *not* ?
------------------

It is *not* designed to be optimal for any particular operation.  Because it is intended
as a generic plug-in able to run a wide variety of third party OpenCL filters it is
simply impossible to optimize the plug-in much.

Also note that GIMP's plug-in architecture is somewhat cumbersome for OpenCL purposes so 
again optimization is not ideal for any specific purpose.

A key point to note is that at this point in time the plug-in does not have any way to
facilitate the use of local memory easily.  You can still use local memory in your
filters, but be aware that at present you will be limited in using it effectively.  In a
practical sense OpenCL's memory model ( and the models used by GPUs ) is really ghastly
from a developer's point of view.  We really need the hardware to cache memory accesses
and not to force developers to do this explicitly.  Something to hope for in the future.

Strictly speaking OpenCL and GPU hardware has not matured enough at this time to be easy
going for anyone.  Optimization of OpenCL is almost exclusively a task for the coder and
the reliability of OpenCL compilers is, to be kind, abysmal.  It is not at all uncommon
to find yourself with perfectly legitimate OpenCL that seems to compile but does not
execute correctly.  This is not a plug-in error, but a driver ( compiler ) error.  Until
OpenCL matures this will continue.

I'm an experienced developer who started back on 8-bit CPUs with 1Kb of memory and in
some ways OpenCL coding reminds me of those days ( over thirty years ago ).  It is very
like writing assembly language in that optimization is entirely down to the developer.
The compiler will give *no* help.

Experience testing the plug-in suggests it is still possible to write fast OpenCL filters
using this plug-in.  It is equally possible to write code that runs *slower* than an
equivalent single CPU core version of the same code.

So the plug-in allows you to use OpenCL, but it does not make your OpenCL good or bad.


Building the plug-in
====================

The repository should contain a *Makefile* for the project.  It is simply C code and
requires both OpenCL and GIMP developer files ( i.e. include files and the associated
libraries and gimptool ) and the common Linux tool chain.

The code has been built on two Linux systems, one based on the AMD A8 APU and one on
a humble Intel Baytrail ( Celeron ) using the Beignet open source Intel OpenCL driver.
It both builds and executes on these systems.  Linux Mint 14 and 17.2 were used.

The Makefile will try and install the plug-in to the GIMP 2.8 user directory.

OpenCL source files should be placed in the same user directory, but inside a folder
called *opencl*.


Using the plug-in
==================

Filters can be installed in any legitimate menu location that GIMP's runtime will allow.
Using filters is trivial from menu entries.

Writing OpenCL is outside the scope of this article, beyond the conventions your code
should follow for use with the plug-in.  So what are the conventions ?


Kernels :
=========

To write a kernel you must be aware that the kernel functions all conform to this
format :

```C
__kernel void <kernel-name> ( <buffer-parameter-list> )
```

So something like these definitions :

```C


__kernel void first(    __global uchar4* bufin,
                        __global uchar4* bufout
                   )

__kernel void precalc_rgba( __global uchar4* bufin,
                            __global uchar4* bufout,
                            __constant uf_t *cons
                            )

__kernel void filter_rgba( __global uchar4* bufin,
                            __global uchar4* bufout,
                            __global uchar4* precalc,
                            __constant uf_t *cons
                           )

```

Buffers :
=========

At this time the plug-in passes byte buffers ( uchar4 is an array of four bytes ) to
kernels.  This is the entire selected region, with the exception of precalc buffers
and the constants buffer.

You can pass an arbitrary number of buffers to a kernel.  To inform the plug-in that
you will do this you must define them in a kernal definition comment.

```C
// buffer temp rw global w*h*c

// kernel first bufin temp
```

What are we doing here ?

Well the first comment is a directive to tell the plug-in we require a buffer named
"temp" to be created for reading and writing ( by kernels ) with a size of w*h*c bytes.

The use of "global" will make sense to anyone writing OpenCL.

The plug-in can evaluate simple expressions so that we can define constants and buffer
sizes in useful ways.  It understands, for example, that w = width, h = height and 
c = number of channels needed.

The full list of defined values teh expression evaluator recognizes are :

     *  <a positive integer>
     *  w ( image width )
     *  h ( image height )
     *  n ( number of pixels )
     *  c ( number of channels )
     *  f ( size of float in bytes )
     *  i ( size of int in bytes )
     *  k ( shorthand for 1024 )
     *  m ( shorthand for 1024*1024 )
     *  d ( largest of w and h )
     *  s ( smallest of w and h )


These should be enough to construct expressions that are image independent and yet flexible
enough to define useful constants and buffer sizes ( like a size of w*h*c ).


The second line simply tells the plug-in to look for a kernel which recieves three buffers,
named "first", "bufin" and "temp".  The plug-in uses "bufin" and "bufout" as reserved names
for the initial input buffer ( automatically filled by the plug-in from the image ) and
the final output buffer ( automatically written back to the image ).  Apart from those names
you can name a buffer anything.

Kernels are executed in the order they are defined in these comments.

For those unfamiliar with OpenCL you might need to define an intermediate buffer if you were
to use multiple kernels, one after the other.  You might also simply want a suitable temp
store for a single kernel.


Menu entries and Script files
=============================

We need a way to tell GIMP where to place the filter in it's menu and what to call it.

All OpenCL filters will be inserted under <Image>/Filters/Open CL.  Unfortunately the
GIMP developers prevent the use of submenus, so this does lead to potentially very large
OpenCL menus.  Future versions may allow more flexibility, but at present that's it.

A menu entry definition is easy.  Typically at the start of the OpenCL code you would
add a comment :

```C
// menuentry Whatever you like
```

The plug-in will use that when writing the automatically generated Script-fu file that
registers each OpenCL filter and provides them with a UI and activation method.  You
can, in principle, alter that automatically generated script file, and as long as the
name is not changed ( or the OpenCL file not altered ), it will be left alone.

When GIMP starts and the plug-in is initialized it scans all files in the "opencl"
directory in your GIMP folder.  New files or files with dates later than the corresponding
script file will trigger the generation of a new script file.  On the one hand this is
useful in allowing you to just focus on writing OpenCL and not managing script files, but
it also means that if you customize those scripts they are vulnerable if you alter
the OpenCL file.  Remember this.


Paramters and Constants :
=========================

We need mechanisms to boh define and pass paramters and constants to OpenCL.

Here's an example from a filter :

```C
// PARAM weight 0.0

// kernel filter_b2b bufin bufout params

__kernel void filter_b2b( __global uchar4* bufin, __global uchar4* bufout, __constant uf_t *cons )
{
...
```

This defines a parameter to be received via the UI ( or a script-fu command ) named
"weight".  The default value is 0.0, and you can choose whatever default you want.

Paramters are *always* floats.

To access that parameter we pass the "params" buffer to the kernel.  This is a reserved
buffer for passing constants used by all the kernels.  You do not need to define that
buffer, but you can also not use it ( as earlier examples show ).

To access the parameters in the OpenCL code the plug-in adds a couple of defines to
the code passed to the OpenCL compiler.  Here's how to use those :

```C
float weight = PARAMF(0) ;
```


What could be simpler ?  Zero is the index of the first parameter.  PARAMF() is a
macro ( OpenCL supports macros ), which hides the messy details of accessing the
constants array.  Currently the prepended code is this :


```C
union uf_u {
  int   	i ;
  float f ;
	} ;

typedef union uf_u uf_t ;

#define PARAMF(idx) ( cons[(idx)+4].f )
#define PARAMI(idx) ( cons[(idx)+4].i )

#define WIDTH (cons[0].i)
#define HEIGHT (cons[1].i)

```


First note the special defines for WIDTH and HEIGHT.  If your kernel needs to know
these values then you *must* pass the "params" buffer to your kernel.

Also note the use of the "uf_t" union type.  If you're familiar with OpenCL ( or C )
you should know what is going on.  If not you need to learn it !

A union is a special type that can store one of several types of data in a single
structure.  To reference the different interpretations you use named fields.

In our "uf_t" union we have integer and float types.



