// pdf2bitmap.h --- Convert PDF to HBITMAP
// Author: katahiromz
// License: MIT

#pragma once

#include "pdf_load.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PDF2BITMAP_DATA
{
    PdfiumCtx ctx;
    void *gdipm;
    int page_count;
} PDF2BITMAP_DATA;

HRESULT pdf2bitmap_init(PDF2BITMAP_DATA* data);
void pdf2bitmap_uninit(PDF2BITMAP_DATA* data);

HRESULT
pdf2bitmap(
    PDF2BITMAP_DATA* data,
    const WCHAR *pdf_filename,
    HBITMAP *phBitmap,
    int page_index,
    int dpi,
    const char *password);

#ifdef __cplusplus
} // extern "C"
#endif
