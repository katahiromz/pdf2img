// pdf_load.c --- PDF loader
// Author: katahiromz
// License: MIT

// Requirement: pdfium.dll
//   https://github.com/bblanchon/pdfium-binaries/releases
//   (Expand pdfium-win-x64.tgz or pdfium-win-x86.tgz)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "gdipm.h"
#include "pdf_load.h"

#define GET_PROC(name) do { \
    fn = GetProcAddress(ctx->hDll, "FPDF_" #name); \
    if (!fn) { \
        fprintf(stderr, "FPDF_" #name ": Not found\n"); \
        FreeLibrary(ctx->hDll); \
        ctx->hDll = NULL; \
        return FALSE; \
    } \
    CopyMemory(&ctx->name, &fn, sizeof(fn)); \
} while (0)

#define GET_PROC_BITMAP(name) do { \
    fn = GetProcAddress(ctx->hDll, "FPDF" #name); \
    if (!fn) { \
        fprintf(stderr, "FPDF" #name ": Not found\n"); \
        FreeLibrary(ctx->hDll); \
        ctx->hDll = NULL; \
        return FALSE; \
    } \
    CopyMemory(&ctx->name, &fn, sizeof(fn)); \
} while (0)

BOOL Pdfium_Load(PdfiumCtx* ctx, const TCHAR* dllPath)
{
    FARPROC fn;
    FPDF_LIBRARY_CONFIG cfg;

    ZeroMemory(ctx, sizeof(*ctx));

    ctx->hDll = LoadLibrary(dllPath);
    if (!ctx->hDll)
        return FALSE;

    GET_PROC(InitLibraryWithConfig);
    GET_PROC(DestroyLibrary);
    GET_PROC(GetLastError);
    GET_PROC(LoadMemDocument64);
    GET_PROC(CloseDocument);
    GET_PROC(GetPageCount);
    GET_PROC(LoadPage);
    GET_PROC(ClosePage);
    GET_PROC(GetPageWidth);
    GET_PROC(GetPageHeight);
    GET_PROC(RenderPageBitmap);

    GET_PROC_BITMAP(Bitmap_Create);
    GET_PROC_BITMAP(Bitmap_FillRect);
    GET_PROC_BITMAP(Bitmap_GetBuffer);
    GET_PROC_BITMAP(Bitmap_GetWidth);
    GET_PROC_BITMAP(Bitmap_GetHeight);
    GET_PROC_BITMAP(Bitmap_GetStride);
    GET_PROC_BITMAP(Bitmap_Destroy);

    ZeroMemory(&cfg, sizeof(cfg));
    cfg.version = 2;
    ctx->InitLibraryWithConfig(&cfg);

    return TRUE;
}

void Pdfium_Unload(PdfiumCtx* ctx)
{
    if (!ctx->hDll)
        return;

    if (ctx->DestroyLibrary)
        ctx->DestroyLibrary();
    FreeLibrary(ctx->hDll);
    ctx->hDll = NULL;
}

HRESULT PDF_PageToHBITMAP(
    PdfiumCtx*  ctx,
    const void *pdf_memory,
    size_t      pdf_byte_size,
    int         pageIndex,
    int         dpi,
    HBITMAP*    phBitmap,
    int*        page_count,
    const char* password)
{
    FPDF_DOCUMENT doc = NULL;
    FPDF_PAGE page = NULL;
    FPDF_BITMAP fpBmp = NULL;
    HBITMAP hBmp = NULL;
    double pageW_pt, pageH_pt;
    int width, height;
    int renderFlags = FPDF_ANNOT | FPDF_LCD_TEXT;
    void* pSrc;
    int y, stride;
    BITMAPINFO bmi;
    void* pDst = NULL;
    HRESULT hr = S_OK;

    if (!ctx || !pdf_memory || !pdf_byte_size || !phBitmap)
        return E_INVALIDARG;

    *phBitmap = NULL;

    doc = ctx->LoadMemDocument64(pdf_memory, pdf_byte_size, password);
    if (!doc)
    {
        unsigned long err = ctx->GetLastError ? ctx->GetLastError() : 0;
        if (err == 4) // FPDF_ERR_PASSWORD
            return E_ACCESSDENIED;

        fprintf(stderr, "[PDFium] Cannot open document (err=%lu)\n", err);
        hr = E_FAIL;
        goto cleanup;
    }

    *page_count = ctx->GetPageCount(doc);
    if (pageIndex < 0 || pageIndex >= *page_count)
        pageIndex = 0;

    page = ctx->LoadPage(doc, pageIndex);
    if (!page)
    {
        fprintf(stderr, "[PDFium] Failed to load page: %d\n", pageIndex);
        hr = E_FAIL;
        goto cleanup;
    }

    pageW_pt = ctx->GetPageWidth(page);
    pageH_pt = ctx->GetPageHeight(page);
    width    = (int)(pageW_pt * dpi / 72.0 + 0.5);
    height   = (int)(pageH_pt * dpi / 72.0 + 0.5);
    if (width <= 0 || height <= 0)
    {
        fprintf(stderr, "[PDFium] width or height was not positive\n");
        hr = E_UNEXPECTED;
        goto cleanup;
    }

    fpBmp = ctx->Bitmap_Create(width, height, 0 /* 0=BGRx */);
    if (!fpBmp)
    {
        fprintf(stderr, "[PDFium] FPDFBitmap_Create failed\n");
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    ctx->Bitmap_FillRect(fpBmp, 0, 0, width, height, 0xFFFFFFFF);
    ctx->RenderPageBitmap(fpBmp, page, 0, 0, width, height, 0, renderFlags);

    pSrc = ctx->Bitmap_GetBuffer(fpBmp);
    stride = ctx->Bitmap_GetStride(fpBmp);

    ZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = width;
    bmi.bmiHeader.biHeight      = -height; /* top-down */
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    hBmp = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, &pDst, NULL, 0);
    if (!hBmp || !pDst)
    {
        fprintf(stderr, "[Win32] CreateDIBSection failed (error=%lu)\n", GetLastError());
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    for (y = 0; y < height; y++)
    {
        const BYTE* src = (const BYTE*)pSrc + (SIZE_T)y * stride;
        BYTE* dst = (BYTE*)pDst + (SIZE_T)y * width * 4;
        CopyMemory(dst, src, (SIZE_T)width * 4);
    }

    GdiFlush();

    *phBitmap = hBmp;
    hBmp = NULL;
    hr = S_OK;

cleanup:
    if (hBmp)
        DeleteObject(hBmp);
    if (fpBmp)
        ctx->Bitmap_Destroy(fpBmp);
    if (page)
        ctx->ClosePage(page);
    if (doc)
        ctx->CloseDocument(doc);
    return hr;
}
