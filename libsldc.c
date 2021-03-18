/* https://www.ecma-international.org/wp-content/uploads/ECMA-321_1st_edition_june_2001.pdf */

#include "sldc.h"

#define VERSION(major,minor,release) (((major)<<16)|((minor)<<8)|(release))

#define HBUFFER_SIZE           1024

#define CTRLSYMB_FLUSH         0x1FF0 /*0b1111111110000*/
#define CTRLSYMB_SCHEME_1      0x1FF1 /*0b1111111110001*/
#define CTRLSYMB_SCHEME_2      0x1FF2 /*0b1111111110010*/
#define CTRLSYMB_FILE_MARK     0x1FF3 /*0b1111111110011*/
#define CTRLSYMB_END_OF_RECORD 0x1FF4 /*0b1111111110100*/
#define CTRLSYMB_RESET_1       0x1FF5 /*0b1111111110101*/
#define CTRLSYMB_RESET_2       0x1FF6 /*0b1111111110110*/
#define CTRLSYMB_END_MARKER    0x1FFF /*0b1111111111111*/

#define REALLOC(buffer,current,limit) \
    if (current >= limit) { limit <<= 1; buffer = realloc(buffer, limit); }

const guint32 sldc_version = VERSION(1,0,0);

guint16 bin2int(guchar* buf, goffset off, guint8 sz)
{
    guint16 v = 0;
    for (guint8 i = 0; i < sz; i++)
        v |= buf[off + i] << (sz - 1 - i);
    return v;
}

guchar* sldc_compress(guchar* buffer, gsize len, gsize* outlen)
{
    /* if buffer is NULL, there is absolutely nothing to compress */
    if (!buffer) return NULL;

    gsize cindex = 0, outsize = 1024; guchar* binbuffer = malloc(outsize);
    guchar c;   /* general variables */

    /* for simplicity reasons this compression implementation only
       yields scheme 2 literal data symbols. the major downside of
       this technique is that it almost always guarantees that the
       compressed data is bigger than the uncompressed data.      */

    /* yield a RESET_2 control symbol */
    for (guint8 i = 0; i < 13; i++)
    {
        REALLOC(binbuffer,cindex,outsize);
        binbuffer[cindex++] = (CTRLSYMB_RESET_2 >> (12 - i)) & 1;
    }

    /* yield scheme 2 literal bytes */
    for (gsize k = 0; k < len; k++)
    {
        c = buffer[k];
        for (guint8 i = 0; i < 8; i++)
        {
            REALLOC(binbuffer,cindex,outsize);
            binbuffer[cindex++] = (c >> (7 - i)) & 1;
        }
        if  (c == 0xFF)
        {
            REALLOC(binbuffer,cindex,outsize);
            binbuffer[cindex++] = 0;
        }
    }

    /* yield an END_OF_RECORD control symbol */
    for (guint8 i = 0; i < 13; i++)
    {
        REALLOC(binbuffer,cindex,outsize);
        binbuffer[cindex++] = (CTRLSYMB_END_OF_RECORD >> (12 - i)) & 1;
    }

    /* add zero-padding to align data on 32 bits boundary */
    while (cindex % 32)
    {
        REALLOC(binbuffer,cindex,outsize);
        binbuffer[cindex++] = 0;
    }

    /* create and populate the output compressed buffer */
    *outlen = cindex / 8;
    guchar* cbuffer = malloc(*outlen);
    for (gsize k = 0; k < *outlen; k++)
        cbuffer[k] = bin2int(binbuffer, k << 3, 8);

    /* free memory resources */
    free(binbuffer);

    return cbuffer;
}

guchar* sldc_decompress(guchar* buffer, gsize len, gsize* outlen)
{
    /* if buffer is NULL there's absolutely nothing to decompress */
    if (!buffer) return NULL;

    /* general variables used by the compression algorithm itself */
    guchar hbuffer[HBUFFER_SIZE]; gsize hindex = 0; guint8 scheme = 0;
    guchar c; guint16 MCF, DF; gboolean breakloop = FALSE;

    /* build binary representation of the compressed input buffer */
    guchar* binbuffer = malloc(len << 3); goffset off = 0;
    for (gsize i = 0; i < len << 3; i++)
        binbuffer[i] = (buffer[i / 8] & (1 << (7 - i % 8))) ? 1 : 0;

    /* primary allocation of the uncompressed data output buffer */
    gsize uindex = 0, outsize = 1024; guchar* ubuffer = malloc(outsize);

    /* decoding loop */
    while (off < len << 3)
    {
        /* control symbol */
        if (bin2int(binbuffer, off, 9) == 0x1FF /*0b111111111*/)
        {
            guint16 ctrlsymb = bin2int(binbuffer, off, 13); off += 13;
            switch (ctrlsymb)
            {
                case CTRLSYMB_FLUSH:                                 break;
                case CTRLSYMB_SCHEME_1:                  scheme = 1; break;
                case CTRLSYMB_SCHEME_2:                  scheme = 2; break;
                case CTRLSYMB_FILE_MARK:                             break;
                case CTRLSYMB_END_OF_RECORD: breakloop = TRUE;       break;
                case CTRLSYMB_RESET_1:       hindex = 0; scheme = 1; break;
                case CTRLSYMB_RESET_2:       hindex = 0; scheme = 2; break;
                case CTRLSYMB_END_MARKER:                            break;
                default: return NULL;
            }
            if (breakloop) break;
        }
        else if (scheme == 1) /* scheme 1 */
        {
            gboolean literal = binbuffer[off++] == 0 ? TRUE : FALSE;
            if (literal) /* literal data */
            {
                c = bin2int(binbuffer, off, 8); off += 8;
                REALLOC(ubuffer,uindex,outsize);
                ubuffer[uindex++] = c;
                hbuffer[hindex++] = c; hindex %= HBUFFER_SIZE;
            }
            else         /* copy pointer data */
            {
                if      (bin2int(binbuffer, off, 1) == 0x0 /*0b0*/   ) { MCF = (1<<1) + bin2int(binbuffer, off + 1, 1); off += 1 + 1; }
                else if (bin2int(binbuffer, off, 2) == 0x2 /*0b10*/  ) { MCF = (1<<2) + bin2int(binbuffer, off + 2, 2); off += 2 + 2; }
                else if (bin2int(binbuffer, off, 3) == 0x6 /*0b110*/ ) { MCF = (1<<3) + bin2int(binbuffer, off + 3, 3); off += 3 + 3; }
                else if (bin2int(binbuffer, off, 4) == 0xE /*0b1110*/) { MCF = (1<<4) + bin2int(binbuffer, off + 4, 4); off += 4 + 4; }
                else if (bin2int(binbuffer, off, 4) == 0xF /*0b1111*/) { MCF = (1<<5) + bin2int(binbuffer, off + 4, 8); off += 4 + 8; }
                DF = bin2int(binbuffer, off, 10); off += 10;
                for (int k = DF; k < DF + MCF; k++)
                {
                    c = hbuffer[k % HBUFFER_SIZE];
                    REALLOC(ubuffer,uindex,outsize);
                    ubuffer[uindex++] = c;
                    hbuffer[hindex++] = c; hindex %= HBUFFER_SIZE;
                }
            }
        }
        else if (scheme == 2) /* scheme 2 */
        {
            c = bin2int(binbuffer, off, 8); off += 8 + (c == 0xFF ? 1 : 0);
            REALLOC(ubuffer,uindex,outsize);
            ubuffer[uindex++] = c;
        }
    }
    *outlen = uindex;

    /* free memory resources */
    free(binbuffer);

    return ubuffer;
}

