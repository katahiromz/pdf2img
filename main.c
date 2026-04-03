// main.c --- pdf2img converter
// Author: katahiromz
// License: MIT
#include <windows.h>
#include <stdio.h>
#include "pdf2bitmap.h"
#include "gdipm.h"

int wmain(int argc, wchar_t **wargv)
{
    wchar_t* pdf_filename;
    wchar_t* image_filename;
    int page_index;
    int dpi;
    PDF2BITMAP_DATA data;
    HRESULT hr;
    HBITMAP hBitmap;
    BOOL result;
    char password[128];
    password[0] = 0;

    if (argc < 3)
    {
        fprintf(stderr, "Usage: pdf2img pdf-file.pdf image_file [PAGE_NUM] [DPI] [PASSWORD]\n");
        return 1;
    }

    pdf_filename = wargv[1];
    image_filename = wargv[2];

    page_index = (argc >= 4) ? (_wtoi(wargv[3]) - 1) : 0;

    dpi = (argc >= 5) ? _wtoi(wargv[4]) : 150;
    if (dpi <= 0)
        dpi = 150;

    if (argc >= 6)
    {
        WideCharToMultiByte(CP_ACP, 0, wargv[5], -1, password, _countof(password), NULL, NULL);
        password[_countof(password) - 1] = 0;
    }

    hr = pdf2bitmap_init(&data);
    if (FAILED(hr))
    {
        fprintf(stderr, "pdf2bitmap_init failed\n");
        return 1;
    }

    result = FALSE;
    hr = pdf2bitmap(&data, pdf_filename, &hBitmap, page_index, dpi, (password[0] ? password : NULL));
    if (SUCCEEDED(hr))
        result = gdipm_save_pic(data.gdipm, image_filename, hBitmap);

    pdf2bitmap_uninit(&data);

    if (result)
        puts("Success!");
    else
        puts("FAILED.");

    return result ? 0 : 1;
}

int main(void)
{
    int argc;
    LPWSTR *wargv = CommandLineToArgvW(GetCommandLineW(), &argc);
    int ret = wmain(argc, wargv);
    LocalFree(wargv);
    return ret;
}
