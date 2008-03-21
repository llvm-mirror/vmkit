##############################################################################
#
# Micro-VM Allocator
# by Charles Clément, corrected by Gaël Thomas.
#
##############################################################################


I Memory layout
  1 Exponantial area
  2 Linear area
  3 Mmap area
II Structure
  1 Referencing memory chunks
  2 Harsh tables
III Internal memory management

I Memory layout
#--------------

Based on the size of the objects to allocate, the allocator chooses between
three distincts area.
i) The exponantial area for objects less than max_exp bytes
ii) The linear area for x in max_exp < x < max_lin bytes
iii) The mmap area for LIN bytes < x

Max_exp and max_lin are actually defined to 32 and 8192 bytes.

I.1 Exponantial area:
Objects stored in that area are allocated 2^n bytes, the smallest power of two
in which the object can fit.

-------------------------------------
| Size to allocate | reserved space |
-------------------------------------
 1                  2
 2                  2
 3                  4
 4                  4
 {5,6,7,8}          8
 {9-16}             16
 {17-32}            32

-> Size of allocated chunk in this area grows exponantially.
With max_exp = 32 bytes

I.2 Linear area:
In the linear area, the size needed to be allocated is rounded to the next
max_exp multiple. This is to reduce memory loss, as otherwise, with the
previous technique allocating 260 bytes would induce to occupy a 512 bytes
area.

   ------------------------------------
  |  32  |   64   |  96  |  128  |  ...
   ------------------------------------

max_exp*1    *2      *3      *4

-> Size of allocated chunk in this area grows linearly.

I.3 Mmap area:
For objects larger than 2^13 (8192) bytes, a number of physical page is
associated, thus a multiplier of 4096 bytes.


II Descriptors
#-------------

II.1 Hash tables:

In order to keep track of allocated memory adresses, we need to store the
adresses that we allocated. This is a requirement for the garbage collector, as
the compatibility with the C ABI implies that a memory reference is
indistinguishable from a value. Hence, every page descriptor is inserted in a
hash table when allocated.
This has a limitation, as it is not possible to make the difference between a
value in the heap that would point to an actual allocated space in memory if
translated into a pointer.


II.2 Referencing memory chunks:
Here is the representation of an adress in memory :

          ----------------------------------------------------------
Pointer: |    Table Entry     |  Page Descriptor  |       Index     |
          ----------------------------------------------------------
             |     __                    |                   |
             |    |  |                   |                   |
             |    |__|                   |                   |
             |    |  |    GCHashSet     \|/                  |
             |    |__|       __ __ __ __ __ __               |
              --->|  |----->|__|__|__|__|__|__|              |
                  |__|                    |                  |
                  |  |                    |    GCPage       \|/
                  |__|                    |      __ __ __ __ __ __
           GCHash                          ---->|__|__|__|__|__|__| Headers


Memory is separated in spaces. Each space contains memory chunk of the same
size. When the allocator needs to allocate n bytes, it choose a memory space
in an exponential, linear or mmaped space and round n to the size of this
space. This technique is used to construct a performant hashtable of all
allocated memory chunks, which is used to identify pointer during a collection
(the garbage collector is only conservative).

The first bits of a memory chunk pointer reference an entry in GCHash, and
the next bits reference an entry in GCHashSet. Each entry in GCHashSet is a
GCPage structure, which describes a set of contiguous pages (a memory space).

The GCPage Class holds a field (GCChunkNode*)_headers containing the list to
all headers of free chunks of the space. It also holds the size of the space
in _chunk_nbb. This size is the rounded size (exp or linear) of all memory
chunks managed in this space.

_Note_:
This is not the case for space allocated in the mmap area, whose page
descriptor holds directly the header itself.

A header is described by the class GCChunkNode. It holds the size _nbb_mark of
memory chunks in the last 29 bits (the 3 last ones are used by the garbage
collector to set the *color* of the chunk). GCChunkNode are stored in a
circular double linked list.


III Internal memory management
#-----------------------------

The allocator has to allocate its memmory structures itself. It is responsible
to allocate the memory for the micro-vm and the structures needed to manage
and access the allocated areas.
