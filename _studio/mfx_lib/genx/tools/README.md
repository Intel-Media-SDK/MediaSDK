# embed_isa helper tool

embed_isa is a simple application which generates c-array from the binary kernel ISA file. Usage:
```sh
embed_isa <kernel_file>.isa
```
On the output you will get 2 files:
1. <kernel_file>_isa.h header file of the format similar to:
```sh
#ifndef __<kernel_file>__
#define __<kernel_file>__
extern const unsigned char <kernel_file>[<size>];
#endif
```
2. <kernel_file>_isa.cpp source file with the kernel c-style array in the format similar to:
```sh
#include "<kernel_file>_isa.h"

const unsigned char <kernel_file>[<size>] = {
... // c-style array data
};
```
