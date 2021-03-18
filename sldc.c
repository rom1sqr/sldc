#include <stdio.h>
#include <unistd.h>
#include "sldc.h"

int main(int argc, char** argv)
{
    if (argc == 2)
    {
        guint32 ver; guchar* result; gsize outlen = 0; ssize_t r; guchar indata[65536];
        switch (argv[1][1])
        {
            case 'h':
                fprintf(stderr, "%s\n", argv[0]);
                fprintf(stderr, "-h\thelp\n");
                fprintf(stderr, "-v\tversion\n");
                fprintf(stderr, "-c\tcompress\n");
                fprintf(stderr, "-d\tdecompress\n");
                break;
            case 'v':
                ver = sldc_version;
                printf("%d.%d.%d\n", (ver>>16)&0xFF, (ver>>8)&0xFF, (ver>>0)&0xFF);
                break;
            case 'c':
                r = read(0, indata, 65535);
                if (r > 0)
                {
                    result = sldc_compress(indata, r, &outlen);
                    for (int i = 0; i < outlen; i++)
                    printf("%c", result[i]);
                }
                break;
            case 'd':
                r = read(0, indata, 65535);
                if (r > 0)
                {
                    result = sldc_decompress(indata, r, &outlen);
                    for (int i = 0; i < outlen; i++)
                    printf("%c", result[i]);
                }
                break;
            default: break;
        }
    }

    return 0;
}

