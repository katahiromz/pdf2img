// gdipm.c --- GDI plus minus
// Author: katahiromz
// License: MIT

#include <windows.h>
#include <shlwapi.h>
#include "gdipm.h"

typedef enum GpStatus
{
    Ok = 0,
    GenericError = 1,
    InvalidParameter = 2,
    OutOfMemory = 3,
    ObjectBusy = 4,
    InsufficientBuffer = 5,
    NotImplemented = 6,
    Win32Error = 7,
    WrongState = 8,
    Aborted = 9,
    FileNotFound = 10,
    ValueOverflow = 11,
    AccessDenied = 12,
    UnknownImageFormat = 13,
    FontFamilyNotFound = 14,
    FontStyleNotFound = 15,
    NotTrueTypeFont = 16,
    UnsupportedGdiplusVersion = 17,
    GdiplusNotInitialized = 18,
    PropertyNotFound = 19,
    PropertyNotSupported = 20,
    ProfileNotFound = 21
} GpStatus;

typedef LPVOID GpBitmap, GpImage;
typedef DWORD Color;
typedef float REAL;

typedef enum DebugEventLevel
{
    DebugEventLevelFatal,
    DebugEventLevelWarning
} DebugEventLevel;

typedef VOID (WINAPI *DebugEventProc)(enum DebugEventLevel, CHAR *);
typedef GpStatus (WINAPI *NotificationHookProc)(ULONG_PTR *);
typedef void (WINAPI *NotificationUnhookProc)(ULONG_PTR);

typedef struct GdiplusStartupInput
{
    UINT32 GdiplusVersion;
    DebugEventProc DebugEventCallback;
    BOOL SuppressBackgroundThread;
    BOOL SuppressExternalCodecs;
} GdiplusStartupInput;

typedef struct GdiplusStartupOutput
{
    NotificationHookProc NotificationHook;
    NotificationUnhookProc NotificationUnhook;
} GdiplusStartupOutput;

typedef struct EncoderParameter
{
    GUID Guid;
    ULONG NumberOfValues;
    ULONG Type;
    VOID *Value;
} EncoderParameter;

typedef struct EncoderParameters
{
    UINT Count;
    EncoderParameter Parameter[1];
} EncoderParameters;

typedef struct ImageCodecInfo
{
    CLSID Clsid;
    GUID FormatID;
    const WCHAR *CodecName;
    const WCHAR *DllName;
    const WCHAR *FormatDescription;
    const WCHAR *FilenameExtension;
    const WCHAR *MimeType;
    DWORD Flags;
    DWORD Version;
    DWORD SigCount;
    DWORD SigSize;
    const BYTE *SigPattern;
    const BYTE *SigMask;
} ImageCodecInfo;

/* PixelFormat / LockBits 関連 ------------------------------------------------*/
typedef struct BitmapData
{
    UINT     Width;
    UINT     Height;
    INT      Stride;
    INT      PixelFormat;
    VOID    *Scan0;
    UINT_PTR Reserved;
} BitmapData;

typedef struct GpRectI
{
    INT X, Y, Width, Height;
} GpRectI;

#define PixelFormat32bppARGB  0x0026200A   /* non-premultiplied ARGB */
#define ImageLockModeRead     0x0001

typedef GpStatus (WINAPI *FN_GdiplusStartup)(ULONG_PTR *, const struct GdiplusStartupInput *, struct GdiplusStartupOutput *);
typedef void (WINAPI *FN_GdiplusShutdown)(ULONG_PTR);
typedef GpStatus (WINAPI *FN_GdipCreateBitmapFromFile)(const WCHAR *, GpBitmap **);
typedef GpStatus (WINAPI *FN_GdipCreateHBITMAPFromBitmap)(GpBitmap *, HBITMAP *, ARGB);
typedef GpStatus (WINAPI *FN_GdipDisposeImage)(GpImage *);
typedef GpStatus (WINAPI *FN_GdipCreateBitmapFromHBITMAP)(HBITMAP, HPALETTE, GpBitmap **);
typedef GpStatus (WINAPI *FN_GdipSaveImageToFile)(GpImage *, const WCHAR *, const CLSID *, const EncoderParameters *);
typedef GpStatus (WINAPI *FN_GdipDisposeImage)(GpImage *);
typedef GpStatus (WINAPI *FN_GdipGetImageWidth)(GpImage *, UINT *);
typedef GpStatus (WINAPI *FN_GdipGetImageHeight)(GpImage *, UINT *);
typedef GpStatus (WINAPI *FN_GdipBitmapLockBits)(GpBitmap *, const GpRectI *, UINT, INT, BitmapData *);
typedef GpStatus (WINAPI *FN_GdipBitmapUnlockBits)(GpBitmap *, BitmapData *);
#ifndef GDIPM_NO_DPI
typedef GpStatus (WINAPI *FN_GdipGetImageHorizontalResolution)(GpImage *, REAL *);
typedef GpStatus (WINAPI *FN_GdipGetImageVerticalResolution)(GpImage *, REAL *);
typedef GpStatus (WINAPI *FN_GdipBitmapSetResolution)(GpBitmap *, REAL, REAL);
#endif
typedef GpStatus (WINAPI *FN_GdipGetImageEncodersSize)(UINT *, UINT *);
typedef GpStatus (WINAPI *FN_GdipGetImageEncoders)(UINT, UINT, ImageCodecInfo *);

typedef struct gdipm_t
{
    HINSTANCE m_hInst;
    GdiplusStartupInput m_startup_input;
    GdiplusStartupOutput m_startup_output;
    ULONG_PTR m_token;
    FN_GdiplusStartup m_GdiplusStartup;
    FN_GdiplusShutdown m_GdiplusShutdown;
    FN_GdipCreateBitmapFromFile m_GdipCreateBitmapFromFile;
    FN_GdipCreateHBITMAPFromBitmap m_GdipCreateHBITMAPFromBitmap;
    FN_GdipDisposeImage m_GdipDisposeImage;
    FN_GdipCreateBitmapFromHBITMAP m_GdipCreateBitmapFromHBITMAP;
    FN_GdipSaveImageToFile m_GdipSaveImageToFile;
#ifndef GDIPM_NO_DPI
    FN_GdipGetImageHorizontalResolution m_GdipGetImageHorizontalResolution;
    FN_GdipGetImageVerticalResolution m_GdipGetImageVerticalResolution;
    FN_GdipBitmapSetResolution m_GdipBitmapSetResolution;
#endif
    FN_GdipGetImageWidth m_GdipGetImageWidth;
    FN_GdipGetImageHeight m_GdipGetImageHeight;
    FN_GdipBitmapLockBits m_GdipBitmapLockBits;
    FN_GdipBitmapUnlockBits m_GdipBitmapUnlockBits;
    FN_GdipGetImageEncodersSize m_GdipGetImageEncodersSize;
    FN_GdipGetImageEncoders m_GdipGetImageEncoders;
    UINT m_num_encoder;
    ImageCodecInfo *m_encoders;
} gdipm_t;

static GpStatus gdipm_init(gdipm_t *gdipm)
{
    FARPROC fnFar;
    HINSTANCE hInst;

    hInst = gdipm->m_hInst = LoadLibraryW(L"gdiplus.dll");
    if (!hInst)
        return GdiplusNotInitialized;

    gdipm->m_startup_input.GdiplusVersion = 1;
    gdipm->m_startup_input.DebugEventCallback = NULL;
    gdipm->m_startup_input.SuppressBackgroundThread = FALSE;
    gdipm->m_startup_input.SuppressExternalCodecs = FALSE;

#define GET_PROC(fn) do { \
    fnFar = GetProcAddress(hInst, #fn); \
    if (!fnFar) { \
        FreeLibrary(hInst); \
        return GdiplusNotInitialized; \
    } \
    CopyMemory(&gdipm->m_##fn, &fnFar, sizeof(FARPROC)); \
} while (0)

    GET_PROC(GdiplusStartup);
    GET_PROC(GdiplusShutdown);
    GET_PROC(GdipCreateBitmapFromFile);
    GET_PROC(GdipCreateHBITMAPFromBitmap);
    GET_PROC(GdipDisposeImage);
    GET_PROC(GdipCreateBitmapFromHBITMAP);
    GET_PROC(GdipSaveImageToFile);
    GET_PROC(GdipGetImageWidth);
    GET_PROC(GdipGetImageHeight);
    GET_PROC(GdipBitmapLockBits);
    GET_PROC(GdipBitmapUnlockBits);
#ifndef GDIPM_NO_DPI
    GET_PROC(GdipGetImageHorizontalResolution);
    GET_PROC(GdipGetImageVerticalResolution);
    GET_PROC(GdipBitmapSetResolution);
#endif
    GET_PROC(GdipGetImageEncodersSize);
    GET_PROC(GdipGetImageEncoders);

#undef GET_PROC

    gdipm->m_num_encoder = 0;
    gdipm->m_encoders = NULL;

    return gdipm->m_GdiplusStartup(&gdipm->m_token, &gdipm->m_startup_input, &gdipm->m_startup_output);
}

static void gdipm_exit(gdipm_t *gdipm)
{
    free(gdipm->m_encoders);
    gdipm->m_encoders = NULL;
    gdipm->m_GdiplusShutdown(gdipm->m_token);
    gdipm->m_token = 0;
    FreeLibrary(gdipm->m_hInst);
}

HRESULT gdipm_init_ex(void **pgdipm)
{
    GpStatus status;
    *pgdipm = malloc(sizeof(gdipm_t));
    if (!*pgdipm)
        return E_OUTOFMEMORY;
    status = gdipm_init(*pgdipm);
    if (status != Ok)
    {
        free(*pgdipm);
        *pgdipm = NULL;
    }
    return (status == Ok) ? S_OK : E_FAIL;
}

void gdipm_exit_ex(void *gdipm)
{
    if (gdipm)
    {
        gdipm_exit(gdipm);
        free(gdipm);
    }
}

HBITMAP gdipm_load_pic(void *gdipm, const WCHAR *image_filename, ARGB back_color
#ifndef GDIPM_NO_DPI
    , float *x_dpi, float *y_dpi
#endif
)
{
    gdipm_t *p = gdipm;
    GpBitmap   *pBitmap = NULL;
    BitmapData  bdata;
    GpRectI     rect;
    BITMAPINFOHEADER bih;
    HBITMAP     hbm = NULL;
    VOID       *pvBits = NULL;
    HDC         hDC;
    UINT        width, height, y;
    GpStatus    status;

    (void)back_color; /* αチャンネル付きで返すため背景色は使用しない */

    if (p->m_GdipCreateBitmapFromFile(image_filename, &pBitmap) != Ok)
        return NULL;

    /* ---- 画像サイズ取得 -------------------------------------------- */
    if (p->m_GdipGetImageWidth(pBitmap, &width)   != Ok ||
        p->m_GdipGetImageHeight(pBitmap, &height) != Ok ||
        width == 0 || height == 0)
    {
        p->m_GdipDisposeImage(pBitmap);
        return NULL;
    }

#ifndef GDIPM_NO_DPI
    /* ---- DPI 取得 -------------------------------------------------- */
    if (x_dpi)
        p->m_GdipGetImageHorizontalResolution(pBitmap, x_dpi);
    if (y_dpi)
        p->m_GdipGetImageVerticalResolution(pBitmap, y_dpi);
    if (x_dpi || y_dpi)
    {
        HDC hScreenDC = GetDC(NULL);
        if (x_dpi && *x_dpi <= 0)
            *x_dpi = (float)GetDeviceCaps(hScreenDC, LOGPIXELSX);
        if (y_dpi && *y_dpi <= 0)
            *y_dpi = (float)GetDeviceCaps(hScreenDC, LOGPIXELSY);
        ReleaseDC(NULL, hScreenDC);
    }
#endif

    /* ---- 32bpp top-down DIB Section を作成 ------------------------- */
    ZeroMemory(&bih, sizeof(bih));
    bih.biSize        = sizeof(bih);
    bih.biWidth       = (LONG)width;
    bih.biHeight      = -(LONG)height;  /* top-down */
    bih.biPlanes      = 1;
    bih.biBitCount    = 32;
    bih.biCompression = BI_RGB;

    hDC = GetDC(NULL);
    hbm = CreateDIBSection(hDC, (BITMAPINFO *)&bih, DIB_RGB_COLORS, &pvBits, NULL, 0);
    ReleaseDC(NULL, hDC);

    if (!hbm || !pvBits)
    {
        p->m_GdipDisposeImage(pBitmap);
        return NULL;
    }

    /* ---- GDI+ から ARGB ピクセルをそのまま DIB Section へコピー ----------
     *
     *  GDI+ PixelFormat32bppARGB のメモリ配置: [B][G][R][A] (little-endian)
     *  DIB 32bpp のメモリ配置:                [B][G][R][reserved]
     *
     *  事前乗算 (pre-multiply) は呼び出し側 (karasunpo.cpp) で
     *  ii_premultiply() を使って行う。
     * -------------------------------------------------------------------- */
    rect.X      = rect.Y = 0;
    rect.Width  = (INT)width;
    rect.Height = (INT)height;
    ZeroMemory(&bdata, sizeof(bdata));

    status = p->m_GdipBitmapLockBits(pBitmap, &rect,
                                      ImageLockModeRead,
                                      PixelFormat32bppARGB, &bdata);
    if (status == Ok)
    {
        const UINT row_bytes = width * 4;

        for (y = 0; y < height; y++)
        {
            const BYTE *src = (const BYTE *)bdata.Scan0 + (INT_PTR)bdata.Stride * y;
            BYTE       *dst = (BYTE *)pvBits + row_bytes * y;
            CopyMemory(dst, src, row_bytes);
        }

        p->m_GdipBitmapUnlockBits(pBitmap, &bdata);
    }
    else
    {
        DeleteObject(hbm);
        hbm = NULL;
    }

    p->m_GdipDisposeImage(pBitmap);
    return hbm;
}

static HRESULT
gdipm_find_codec(CLSID *pclsid, const WCHAR *dotext, const ImageCodecInfo *pCodecs, UINT nCodecs)
{
    UINT i;
    for (i = 0; i < nCodecs; ++i)
    {
        const WCHAR *pSpecs = pCodecs[i].FilenameExtension;
        int ichOld = 0;

        for (;;)
        {
            const WCHAR *pSep = wcschr(pSpecs + ichOld, L';');
            int ichSep = pSep ? (int)(pSep - pSpecs) : -1;

            int specLen = (ichSep < 0)
                ? (int)wcslen(pSpecs + ichOld)
                : ichSep - ichOld;

            WCHAR strSpec[MAX_PATH];
            const WCHAR *pDot;
            lstrcpynW(strSpec, pSpecs + ichOld, min(specLen + 1, MAX_PATH));

            pDot = wcsrchr(strSpec, L'.');
            if (pDot)
            {
                WCHAR strSpecDotted[MAX_PATH];
                lstrcpynW(strSpecDotted, pDot, _countof(strSpecDotted));

                if (!dotext || _wcsicmp(strSpecDotted, dotext) == 0)
                {
                    *pclsid = pCodecs[i].Clsid;
                    return S_OK;
                }
            }
            else
            {
                if (!dotext || _wcsicmp(strSpec, dotext) == 0)
                {
                    *pclsid = pCodecs[i].Clsid;
                    return S_OK;
                }
            }

            if (ichSep < 0)
                break;
            ichOld = ichSep + 1;
        }
    }

    return E_FAIL;
}

BOOL gdipm_save_pic(void *gdipm, const WCHAR *image_filename, HBITMAP hBitmap
#ifndef GDIPM_NO_DPI
    , float x_dpi, float y_dpi
#endif
)
{
    gdipm_t *p = gdipm;
    GpStatus status;
    GpBitmap *pBitmap = NULL;
    CLSID clsid;

    status = p->m_GdipCreateBitmapFromHBITMAP(hBitmap, NULL, &pBitmap);
    if (status != Ok)
        return FALSE;

#ifndef GDIPM_NO_DPI
    p->m_GdipBitmapSetResolution(pBitmap, x_dpi, y_dpi);
#endif

    if (!p->m_num_encoder)
    {
        UINT total_size;
        p->m_GdipGetImageEncodersSize(&p->m_num_encoder, &total_size);
        if (!total_size)
            return GenericError;

        p->m_encoders = calloc(total_size / sizeof(ImageCodecInfo), sizeof(ImageCodecInfo));
        if (!p->m_encoders)
        {
            p->m_num_encoder = 0;
            return OutOfMemory;
        }

        status = p->m_GdipGetImageEncoders(p->m_num_encoder, total_size, p->m_encoders);
        if (status != Ok)
            return status;
    }

    status = NotImplemented;
    if (gdipm_find_codec(&clsid, PathFindExtensionW(image_filename), p->m_encoders, p->m_num_encoder) == S_OK)
        status = p->m_GdipSaveImageToFile(pBitmap, image_filename, &clsid, NULL);

    p->m_GdipDisposeImage(pBitmap);

    return status == Ok;
}
