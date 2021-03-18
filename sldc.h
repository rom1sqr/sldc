#include <glib.h>

extern const guint32 sldc_version;
guchar* sldc_compress  (guchar* buffer, gsize len, gsize* outlen);
guchar* sldc_decompress(guchar* buffer, gsize len, gsize* outlen);

