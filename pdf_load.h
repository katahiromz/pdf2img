// pdf_load.h --- PDF loader
// Author: katahiromz
// License: MIT

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef void*       FPDF_DOCUMENT;
typedef void*       FPDF_PAGE;
typedef void*       FPDF_BITMAP;
typedef int         FPDF_BOOL;
typedef const char* FPDF_STRING;

typedef struct {
    int   version;
    void* m_pUserFontPaths;
    void* m_pIsolate;
    unsigned int m_v8EmbedderSlot;
    void* m_pPlatform;
} FPDF_LIBRARY_CONFIG;

#define FPDF_ANNOT          0x01
#define FPDF_LCD_TEXT       0x02
#define FPDF_NO_NATIVETEXT  0x04
#define FPDF_GRAYSCALE      0x08
#define FPDF_REVERSE_BYTE_ORDER 0x10

#define FPDFBitmap_Unknown  0
#define FPDFBitmap_Gray     1
#define FPDFBitmap_BGR      2
#define FPDFBitmap_BGRx     3
#define FPDFBitmap_BGRA     4

typedef void          (*FN_FPDF_InitLibraryWithConfig)(const FPDF_LIBRARY_CONFIG*);
typedef void          (*FN_FPDF_DestroyLibrary)(void);
typedef FPDF_DOCUMENT (*FN_FPDF_LoadMemDocument64)(const void* data_buf, size_t size, const char *password);
typedef unsigned long (*FN_FPDF_GetLastError)(void);
typedef void          (*FN_FPDF_CloseDocument)(FPDF_DOCUMENT);
typedef int           (*FN_FPDF_GetPageCount)(FPDF_DOCUMENT);
typedef FPDF_PAGE     (*FN_FPDF_LoadPage)(FPDF_DOCUMENT, int);
typedef void          (*FN_FPDF_ClosePage)(FPDF_PAGE);
typedef double        (*FN_FPDF_GetPageWidth)(FPDF_PAGE);
typedef double        (*FN_FPDF_GetPageHeight)(FPDF_PAGE);
typedef void          (*FN_FPDF_RenderPageBitmap)(FPDF_BITMAP, FPDF_PAGE, int, int, int, int, int, int);
typedef FPDF_BITMAP   (*FN_FPDFBitmap_Create)(int, int, int);
typedef void          (*FN_FPDFBitmap_FillRect)(FPDF_BITMAP, int, int, int, int, unsigned long);
typedef void*         (*FN_FPDFBitmap_GetBuffer)(FPDF_BITMAP);
typedef int           (*FN_FPDFBitmap_GetWidth)(FPDF_BITMAP);
typedef int           (*FN_FPDFBitmap_GetHeight)(FPDF_BITMAP);
typedef int           (*FN_FPDFBitmap_GetStride)(FPDF_BITMAP);
typedef void          (*FN_FPDFBitmap_Destroy)(FPDF_BITMAP);

typedef struct {
    HMODULE hDll;

    FN_FPDF_InitLibraryWithConfig  InitLibraryWithConfig;
    FN_FPDF_DestroyLibrary         DestroyLibrary;
    FN_FPDF_GetLastError           GetLastError;
    FN_FPDF_LoadMemDocument64      LoadMemDocument64;
    FN_FPDF_CloseDocument          CloseDocument;
    FN_FPDF_GetPageCount           GetPageCount;
    FN_FPDF_LoadPage               LoadPage;
    FN_FPDF_ClosePage              ClosePage;
    FN_FPDF_GetPageWidth           GetPageWidth;
    FN_FPDF_GetPageHeight          GetPageHeight;
    FN_FPDF_RenderPageBitmap       RenderPageBitmap;
    FN_FPDFBitmap_Create           Bitmap_Create;
    FN_FPDFBitmap_FillRect         Bitmap_FillRect;
    FN_FPDFBitmap_GetBuffer        Bitmap_GetBuffer;
    FN_FPDFBitmap_GetWidth         Bitmap_GetWidth;
    FN_FPDFBitmap_GetHeight        Bitmap_GetHeight;
    FN_FPDFBitmap_GetStride        Bitmap_GetStride;
    FN_FPDFBitmap_Destroy          Bitmap_Destroy;
} PdfiumCtx;

BOOL Pdfium_Load(PdfiumCtx* ctx, const TCHAR* dllPath);
void Pdfium_Unload(PdfiumCtx* ctx);

HRESULT PDF_PageToHBITMAP(
    PdfiumCtx*  ctx,
    const void *pdf_memory,
    size_t      pdf_byte_size,
    int         pageIndex,
    int         dpi,
    HBITMAP*    phBitmap,
    int*        page_count,
    const char* password);

#ifdef __cplusplus
} // extern "C"
#endif
