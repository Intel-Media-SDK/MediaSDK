// Copyright (c) 2020 Intel Corporation
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WIDTH 16

void make_copyright(FILE* f)
{
    fprintf(f,
"// Copyright (c) 2020 Intel Corporation\n"
"//\n"
"// Permission is hereby granted, free of charge, to any person obtaining a copy\n"
"// of this software and associated documentation files (the \"Software\"), to deal\n"
"// in the Software without restriction, including without limitation the rights\n"
"// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\n"
"// copies of the Software, and to permit persons to whom the Software is\n"
"// furnished to do so, subject to the following conditions:\n"
"//\n"
"// The above copyright notice and this permission notice shall be included in all\n"
"// copies or substantial portions of the Software.\n"
"//\n"
"// THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n"
"// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n"
"// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n"
"// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n"
"// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n"
"// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE\n"
"// SOFTWARE.\n"
);
}

/* first argument is file.isa to incorporate in C code */
int main(int argc, char** argv)
{
    FILE* f;
    long int size;
    long int size_width;
    int width = WIDTH;
    unsigned char* buf = NULL;
    char* basename;
    char* fname, *sbase, *ebase, *bufname;
    int len,i;
    size_t result;

    if(argc != 2)
    {
        printf("Usage: %s somefile.isa", argv[0]);
        exit(-1);
    }

    if((f=fopen(argv[1],"rb")) != NULL)
    {
        fseek(f, 0, SEEK_END);
        size = ftell(f);
        rewind(f);
        buf = (unsigned char*)malloc(size);
        result = fread(buf,1,size,f);
        fclose(f);   
    }

    if( buf )
    {
        //find basename
        fname = argv[1];
        len = strlen(fname);
        ebase = fname+len-1;
        while(ebase >= fname && *ebase != '.') ebase--;
        sbase = ebase;
        while(sbase >= fname && *sbase != '/' && *sbase != '\\') sbase--;
        sbase++;

        len = (int)(ebase-sbase);
        if(len <= 0)
        {
            free(buf);
            exit(-1);
        }

        basename = (char*)malloc(len+9);
        bufname = (char*)malloc(len+1);
        strncpy(bufname, sbase, len);
        bufname[len] = 0;

        strncpy(basename, sbase, len);
        strncpy(basename+len, "_isa.cpp", 8);
        basename[len+8] = 0;

        //cpp file
        if((f=fopen(basename,"wt")) != NULL)
        {
             make_copyright(f);
            //fprintf(f,"#ifndef __%s__\n#define __%s__\n",bufname,bufname);
            fprintf(f,"#include \"%s_isa.h\"\n\n", bufname);
            fprintf(f,"const unsigned char %s[%lu] = {\n",bufname, size);
            for(i=0; i<size; i++)
            {                
                fprintf(f,"0x%02x", buf[i]);
                if(i != size-1) fprintf(f,",");
                if(i == (width-1)){
                    width += WIDTH;
                    fprintf(f,"\n");
                }
            }
            fprintf(f,"\n};\n");
            //fprintf(f,"#endif\n");
            fclose(f);
        }

        //header file
        strncpy(basename, sbase, len);
        strncpy(basename+len, "_isa.h", 6);
        basename[len+6] = 0;
        if((f=fopen(basename,"wt")) != NULL)
        {
             make_copyright(f);
            //fprintf(f,"#ifndef __%s__\n#define __%s__\n",bufname,bufname);
            fprintf(f,"#ifndef __%s__\n#define __%s__\n",bufname, bufname);
            fprintf(f,"extern const unsigned char %s[%lu];\n", bufname, size);
            fprintf(f,"#endif\n");
            //fprintf(f,"#endif\n");
            fclose(f);
        }

        free(bufname);
        free(basename);
        free(buf);
    }

    return 0;
}