// pdf2bitmap.c --- Convert PDF to HBITMAP
// Author: katahiromz
// License: MIT

#include <windows.h>
#include <stdio.h>
#include "gdipm.h"
#include "pdf2bitmap.h"

HRESULT pdf2bitmap_init(PDF2BITMAP_DATA* data)
{
    HRESULT hr;

    if (!Pdfium_Load(&data->ctx, TEXT("pdfium.dll")))
        return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);

    hr = gdipm_init_ex(&data->gdipm);
    if (FAILED(hr))
    {
        Pdfium_Unload(&data->ctx);
        return hr;
    }

    return S_OK;
}

void pdf2bitmap_uninit(PDF2BITMAP_DATA* data)
{
    gdipm_exit_ex(data->gdipm);
    Pdfium_Unload(&data->ctx);
}

HRESULT pdf2bitmap(PDF2BITMAP_DATA* data, const WCHAR *pdf_filename, HBITMAP *phBitmap, int page_index, int dpi, const char *password)
{
    PdfiumCtx *ctx = &data->ctx;
    void *gdipm = data->gdipm;
    WIN32_FIND_DATAW find;
    HANDLE hFind = INVALID_HANDLE_VALUE, hFile = INVALID_HANDLE_VALUE, hMapping = NULL;
    PVOID pView = NULL;
    HRESULT hr;
    size_t pdf_size;

    *phBitmap = NULL;
    data->page_count = 0;

    hFind = FindFirstFileW(pdf_filename, &find);
    if (hFind == INVALID_HANDLE_VALUE)
        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    FindClose(hFind);

    hFile = CreateFileW(pdf_filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        goto cleanup;
    }

    hMapping = CreateFileMappingW(hFile, NULL, PAGE_READONLY, find.nFileSizeHigh, find.nFileSizeLow, NULL);
    if (!hMapping)
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    pView = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
    if (!pView)
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

#ifdef _WIN64
    pdf_size = find.nFileSizeHigh;
    pdf_size <<= 32;
    pdf_size |= find.nFileSizeLow;
#else
    pdf_size = find.nFileSizeLow;
#endif

    hr = PDF_PageToHBITMAP(ctx, pView, pdf_size, page_index, dpi, phBitmap, &data->page_count, password);

cleanup:
    if (pView)
        UnmapViewOfFile(pView);
    if (hMapping != INVALID_HANDLE_VALUE)
        CloseHandle(hMapping);
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);
    return hr;
}
