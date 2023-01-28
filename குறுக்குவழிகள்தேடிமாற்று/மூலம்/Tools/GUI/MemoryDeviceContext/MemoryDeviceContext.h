#pragma once

#include <Windows.h>

class CMemoryDeviceContext
{
protected:
    HWND hAssociatedWnd;
    HDC DeviceContext;
    HBITMAP hBitmap;
    HBITMAP hOldBitmap;
    RECT Rect;
public:
    CMemoryDeviceContext()
    {
        this->DeviceContext = 0;
        this->hBitmap = 0;
        this->hOldBitmap = 0;
        this->hAssociatedWnd = 0;
    }
    ~CMemoryDeviceContext()
    {
        this->ReleaseGdiObjects();
    }
    FORCEINLINE HDC GetDc(){return this->DeviceContext;}

    FORCEINLINE BOOL SetWindowHandle(HWND hWnd)
    {
        this->hAssociatedWnd = hWnd;
        return TRUE;
    }

    // Update : Update internal device context from window handle and rectangle
    //          (create compatible DC for this->DeviceContext and create a compatible bitmap,
    //          so user can work in this->DeviceContext as the original DC)
    // if pSubRect == NULL, SetWindowHandle must have been called (GetClientRect is called with previously provided hwnd)
    FORCEINLINE BOOL Update(HDC hdc,RECT* pSubRect = NULL)
    {
        this->ReleaseGdiObjects();

        this->DeviceContext = ::CreateCompatibleDC(hdc);
        // use full window ! rect provided by pPaintStruct->rcPaint provides bad value when moving outside of desktop limits
        if (pSubRect)
            memcpy(&this->Rect,pSubRect,sizeof(RECT));
        else
            ::GetClientRect(this->hAssociatedWnd,&this->Rect);

        // MSDN : CreateCompatibleBitmap
        // "
        // Note: When a memory device context is created, it initially has a 1-by-1 monochrome bitmap selected into it. 
        //       If this memory device context is used in CreateCompatibleBitmap, the bitmap that is created is a monochrome bitmap. 
        //       To create a color bitmap, use the hDC that was used to create the memory device context, as shown in the following code:
        //        HDC memDC = CreateCompatibleDC ( hDC );
        //        HBITMAP memBM = CreateCompatibleBitmap ( hDC );
        //        SelectObject ( memDC, memBM );
        // "
        // --> use hdc but not this->DeviceContext
        this->hBitmap = ::CreateCompatibleBitmap(hdc,  this->Rect.right - this->Rect.left, this->Rect.bottom - this->Rect.top);
        this->hOldBitmap = (HBITMAP)::SelectObject ( this->DeviceContext, this->hBitmap );
        // set window origin to avoid user to translate all in left top 0,0 world of the new bitmap
        ::SetWindowOrgEx(this->DeviceContext,this->Rect.left,this->Rect.top,NULL);
        return (this->hBitmap != NULL);
    }

    // ReleaseGdiObjects : release the gdi objects allocated by Update() (this->DeviceContext and this->hBitmap)
    FORCEINLINE BOOL ReleaseGdiObjects()
    {
        // Swap back the original bitmap.
        if (this->hOldBitmap)
        {
            ::SelectObject(this->DeviceContext,this->hOldBitmap);
            this->hOldBitmap = NULL;
        }

        // delete current bitmap
        if (this->hBitmap)
        {
            ::DeleteObject(this->hBitmap);
            this->hBitmap = NULL;
        }

        // delete current context
        if (this->DeviceContext)
        {
            ::DeleteDC(this->DeviceContext);
            this->DeviceContext = NULL;
        }

        return TRUE;
    }

    // FillFromDc : copy Srchdc content to this->DeviceContext
    FORCEINLINE BOOL FillFromDc(HDC Srchdc)
    {
        return  ::BitBlt(this->DeviceContext, this->Rect.left, this->Rect.top, this->Rect.right - this->Rect.left, this->Rect.bottom - this->Rect.top, Srchdc, this->Rect.left, this->Rect.top, SRCCOPY);
        // if SetWindowOrgEx is not called
        // return  ::BitBlt(this->DeviceContext, 0, 0, this->Rect.right - this->Rect.left, this->Rect.bottom - this->Rect.top, Dsthdc, this->Rect.left, this->Rect.top, SRCCOPY);
    }

    // DoCopy : copy this->DeviceContext content to Dsthdc
    FORCEINLINE BOOL DoCopy(HDC Dsthdc)
    {
        return  ::BitBlt(Dsthdc, this->Rect.left, this->Rect.top, this->Rect.right - this->Rect.left, this->Rect.bottom - this->Rect.top, this->DeviceContext, this->Rect.left, this->Rect.top, SRCCOPY);
        // if SetWindowOrgEx is not called
        // return  ::BitBlt(hdc, this->Rect.left, this->Rect.top, this->Rect.right - this->Rect.left, this->Rect.bottom - this->Rect.top, this->DeviceContext, 0, 0, SRCCOPY);
    }

    // DoCopy : copy this->DeviceContext content to hdc
    FORCEINLINE BOOL DoStretchCopy(HDC Dsthdc,LONG Dstx,int Dsty,int DstWidth,int DstHeight)
    {
        return  ::StretchBlt(Dsthdc,
                             Dstx, Dsty, 
                             DstWidth, DstHeight, 
                             this->DeviceContext, 
                             this->Rect.left, this->Rect.top, 
                             this->Rect.right - this->Rect.left, this->Rect.bottom - this->Rect.top,
                             SRCCOPY);
    }
};