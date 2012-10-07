txmlparser
==========

A tiny light-weight XML parser in "C" for embedded devices.

- Portable and cross-compilable for any platform or OS.

- Modified to make it work inside Linux Kernel also.

- Extremely low memory footprint and usage.

- Unlike other standard XML libraries, It makes uses of internal pointers from XML tree 
  to access the tag names, text and other attributes directly, thereby avoiding any 
  redundant/intermediate memory allocations for storing these values.