/***********************************************************************************

Copyright (C) 2018 Intel Corporation.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
- Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
- Neither the name of Intel Corporation nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY INTEL CORPORATION "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL INTEL CORPORATION BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

***********************************************************************************/

#include "cttmetrics_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

char CARD_N[16] = {0};
char CARD[32] = {0};

int path_gen(char* pdst, size_t sz, const char delim, const char* a, const char* b, const char* c)
{
    size_t total_len = 2 + strlen(a) + strlen(b) + strlen(c);

    if(total_len >= sz)
        return -1;

    while(*a) *pdst++ = *a++;
    *pdst++ = delim;
    while(*b) *pdst++ = *b++;
    *pdst++ = delim;
    while(*c) *pdst++ = *c++;
    *pdst++ = 0;

    return 0;
}

// get the string value of property and converts it to integer
static long int get_id_from_file(const char* d_name, const char* property)
{
    long int id = 0;
    char str[16] = {0};
    char fname[300] = {0};

    if ((NULL == d_name) || (NULL == property))
        return -1;

    if(path_gen(fname, sizeof(fname)/sizeof(fname[0]), '/', DRM_DIR, d_name, property))
        return -1;

    FILE* file = fopen(fname, "r");
    if (file)
    {
        if (fgets(str, sizeof(str), file))
        {
            id = strtol(str, NULL, 16);
        }
        fclose(file);
    }

    return id;
}

/*
    check that system has at least one Intel grraphics adapter
    support only first adapter
*/
cttStatus discover_path_to_gpu()
{
    cttStatus status = CTT_ERR_DRIVER_NOT_FOUND;
    long int class_id = 0, vendor_id = 0;
    struct stat buffer;
    char DRM_CARD[30] = {0};
    int i;
    for (i=0; i<100 ; i++)
    {
        snprintf(CARD_N, sizeof(CARD_N), "%d", i);
        snprintf(CARD, sizeof(CARD), "card%s", CARD_N);

        path_gen(DRM_CARD, sizeof(DRM_CARD)/sizeof(DRM_CARD[0]), '/', DRM_DIR, CARD, "");
        if(lstat(DRM_CARD, &buffer))
            break;

        // device class id
        class_id = get_id_from_file(CARD, "device/class");

        if (PCI_DISPLAY_CONTROLLER_CLASS == (class_id >> 16))
        {
            // device vendor id
            vendor_id = get_id_from_file(CARD, "device/vendor");
            if (vendor_id == INTEL_VENDOR_ID)
            {
                status = CTT_ERR_NONE;
                break;
            }
        }
    }

    return status;
}

int read_freq(int fd)
{
    size_t nread;
    char freq[16]; /* Intel i915 frequencies are in MHZ, so we expect to read 4 digits */

    if (-1 == fd) goto error;

    /* jump to the very beginning of the file */
    if (-1 == lseek(fd, 0, SEEK_SET)) goto error;

    /* read the new frequency value */
    nread = read(fd, freq, sizeof(freq));
    if (nread >= sizeof(freq)) goto error;

    return atoi(freq);

error:
    return 0;
}
