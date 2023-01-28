/*
Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>
Dynamic aspect ratio code Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

//-----------------------------------------------------------------------------
// Object: class helper for StatusBar control
//-----------------------------------------------------------------------------
#include "Statusbar.h"

//-----------------------------------------------------------------------------
// Name: CStatusbar
// Object: create control
// Parameters :
//     in  : HINSTANCE hInstance : module instance
//           HWND hwndParent : parent window handle
//     out : 
//     return : 
//-----------------------------------------------------------------------------
CStatusbar::CStatusbar(HINSTANCE hInstance,HWND hwndParent,int ControlId,BOOL IncludeGrip)
{
    // Ensure that the common control DLL is loaded.
    InitCommonControls();

    this->hInstance=hInstance;
	this->pEventsHandler = NULL;

    this->SizablePartIndex = -1;
    DWORD Style;
    LONG_PTR OldParentStyle=0;
    BOOL OldParentStyleMustBeRestored = FALSE;

    Style = SBARS_TOOLTIPS |         // allow tooltips
            WS_VISIBLE |             // show window
            WS_GROUP |               // make group
            // WS_CLIPSIBLINGS | do not add clip sibling to avoid dialog controls to appear upper the status bar in case of resizing
            WS_CLIPCHILDREN | // try to avoid flickering of sub controls
            WS_CHILD;                // creates a child window

    if (IncludeGrip)
        Style |= SBARS_SIZEGRIP; // includes a sizing grip
    else
    {
        // if parent has the WS_THICKFRAME style, windows force the gripper... --> bypass this "feature"
        OldParentStyle = ::GetWindowLongPtr(hwndParent,GWL_STYLE);
        if (OldParentStyle & WS_THICKFRAME)
        {
            OldParentStyleMustBeRestored = TRUE;
            ::SetWindowLongPtr(hwndParent,GWL_STYLE,OldParentStyle & ~WS_THICKFRAME);
        }
    }

    // Create the status bar.
    this->hwndStatus = CreateWindowEx(
                                        0,                       // no extended styles
                                        STATUSCLASSNAME,         // name of status bar class
                                        (LPCTSTR) NULL,          // no text when first created
                                        Style,
                                        0, 0, 0, 0,              // ignores size and position
                                        hwndParent,              // handle to parent window
                                        (HMENU)ControlId,        // child window identifier
                                        hInstance,               // handle to application instance
                                        NULL);                   // no window creation data

    if (OldParentStyleMustBeRestored)
    {
        ::SetWindowLongPtr(hwndParent,GWL_STYLE,OldParentStyle);
    }
}

CStatusbar::~CStatusbar(void)
{
    if (this->hwndStatus)
    {
        ::DestroyWindow(this->hwndStatus);
        this->hwndStatus = NULL;
    }
}
//-----------------------------------------------------------------------------
// Name: GetControlHandle
// Object: get control window handle
// Parameters :
//     in  : 
//     out : 
//     return : control HWND
//-----------------------------------------------------------------------------
HWND CStatusbar::GetControlHandle()
{
    return this->hwndStatus;
}

void CStatusbar::Invalidate()
{
    if (this->hwndStatus)
        ::InvalidateRect(this->hwndStatus,NULL,FALSE);
}
void CStatusbar::Redraw()
{
    if (this->hwndStatus)
        ::RedrawWindow(this->hwndStatus,NULL,NULL,RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_UPDATENOW);
}

//-----------------------------------------------------------------------------
// Name: SetSizablePart
// Object: set a part different than last sizable
// Parameters :
//     in  : int PartIndex : sizable part index
//     out : 
//     return : control HWND
//-----------------------------------------------------------------------------
BOOL CStatusbar::SetSizablePart(int PartIndex)
{
    this->SizablePartIndex = PartIndex;
    this->AdjustSizablePart();
    return TRUE;
}

int CStatusbar::GetPartSize(int PartIndex)
{
    if ( PartIndex == 0 )
        return this->PartsInfo[0].Width;
    else
    {
        if (PartIndex<0)
            return -1;
        if ((size_t)PartIndex >= this->PartsInfo.size())
            return -1;

        if (this->PartsInfo[PartIndex].Width == -1)
            return -1;
        else
            return (this->PartsInfo[PartIndex].Width-this->PartsInfo[PartIndex-1].Width);
    }
}

void CStatusbar::AdjustSizablePart()
{
    if (this->SizablePartIndex != -1)
    {
        LONG FixedUsedSize;
        BYTE Cnt;
        FixedUsedSize = 0;
        for (Cnt = 0;Cnt<this->PartsInfo.size();Cnt++)
        {
            if (Cnt != this->SizablePartIndex)
                FixedUsedSize+=this->GetPartSize(Cnt);
        }
        RECT Rect;
        ::GetWindowRect(this->hwndStatus,&Rect);
        LONG Width = Rect.right-Rect.left;
        if (Width>FixedUsedSize)
            Width -= FixedUsedSize;
        else
            Width = 0;
        this->SetPartSize(this->SizablePartIndex,Width);
        if (this->hwndStatus)
            ::InvalidateRect(this->hwndStatus,&Rect,FALSE);
    }
}

void CStatusbar::UpdateEmbeddedControlsPositions()
{
    std::vector<PART_INFOS>::iterator Iterator;
    int Cnt;
    RECT Rect;
    for (Cnt = 0,Iterator = this->PartsInfo.begin(); 
        Iterator!=this->PartsInfo.end(); 
        Cnt++ ,++Iterator)
    {
        if (Iterator->hWndControl)
        {
            this->GetRect(Cnt,&Rect);
            ::SetWindowPos(Iterator->hWndControl,0,
                            Rect.left+Iterator->ControlMargin,
                            Rect.top+Iterator->ControlMargin,
                            Rect.right-Rect.left-2*Iterator->ControlMargin,
                            Rect.bottom-Rect.top-2*Iterator->ControlMargin,
                            SWP_NOACTIVATE|SWP_NOZORDER|SWP_SHOWWINDOW
                            );
        }
    }

}

//-----------------------------------------------------------------------------
// Name: OnSize
// Object: must be called on parent WM_SIZE notification
// Parameters :
//     in  : WPARAM wParam, LPARAM lParam : parent WndProc WPARAM and LPARAM
//     out : 
//     return : result of WM_SIZE message sent to control
//-----------------------------------------------------------------------------
BOOL CStatusbar::OnSize(WPARAM wParam, LPARAM lParam)
{
    BOOL bRetValue;
    bRetValue = (BOOL)SendMessage(this->hwndStatus,WM_SIZE,wParam,lParam);
    this->AdjustSizablePart();

    this->UpdateEmbeddedControlsPositions();
    return bRetValue;
}
//-----------------------------------------------------------------------------
// Name: OnNotify
// Object: must be called on parent WM_NOTIFY notification
// Parameters :
//     in  : WPARAM wParam, LPARAM lParam : parent WndProc WPARAM and LPARAM
//     out : 
//     return : TRUE if message has been processed
//-----------------------------------------------------------------------------
BOOL CStatusbar::OnNotify(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);
    NMHDR* pnm = (NMHDR*)lParam;
    if (pnm->hwndFrom!=this->hwndStatus)
        return FALSE;

    switch (pnm->code)
    {
    case NM_CLICK:
        {
            LPNMMOUSE pnmMouse = (LPNMMOUSE) lParam;
            if (this->pEventsHandler)
                this->pEventsHandler->OnStatusbarClick(this,(int)pnmMouse->dwItemSpec,&pnmMouse->pt);
        }
        break;
    case NM_DBLCLK:
        {
            LPNMMOUSE pnmMouse = (LPNMMOUSE) lParam;
			if (this->pEventsHandler)
				this->pEventsHandler->OnStatusbarDoubleClick(this,(int)pnmMouse->dwItemSpec,&pnmMouse->pt);
        }
        break;
    case NM_RCLICK:
        {
            LPNMMOUSE pnmMouse = (LPNMMOUSE) lParam;
			if (this->pEventsHandler)
				this->pEventsHandler->OnStatusbarRightClick(this,(int)pnmMouse->dwItemSpec,&pnmMouse->pt);
        }
        break;
    case NM_RDBLCLK:
        {
            LPNMMOUSE pnmMouse = (LPNMMOUSE) lParam; 
			if (this->pEventsHandler)
				this->pEventsHandler->OnStatusbarDoubleRightClick(this,(int)pnmMouse->dwItemSpec,&pnmMouse->pt);
        }
        break;
    //case SBN_SIMPLEMODECHANGE:
    //    break;
    }

    return TRUE;
}

void CStatusbar::SetEventsHandler(IStatusbarEvents* pEventsHandler)
{
    this->pEventsHandler=pEventsHandler;
}

int CStatusbar::AddPart()
{
    return this->AddPart(CStatusbar_DEFAULT_SIZE);
}
int CStatusbar::AddPart(TCHAR* PartText)
{
    return this->AddPart(PartText,CStatusbar_DEFAULT_SIZE);
}
int CStatusbar::AddPart(TCHAR* PartText,TCHAR* ToolTip)
{
    return this->AddPart(PartText,ToolTip,CStatusbar_DEFAULT_SIZE);
}
int CStatusbar::AddPart(TCHAR* PartText,HICON hIcon)
{
    return this->AddPart(PartText,hIcon,CStatusbar_DEFAULT_SIZE);
}
int CStatusbar::AddPart(TCHAR* PartText,HINSTANCE hInstance,int IdIcon)
{
    HICON hIcon=(HICON)LoadImage(hInstance, MAKEINTRESOURCE(IdIcon),IMAGE_ICON,0,0,LR_SHARED);
    return this->AddPart(PartText,hIcon);
}
int CStatusbar::AddPart(HICON hIcon,TCHAR* ToolTip)
{
    return this->AddPart(hIcon,ToolTip,CStatusbar_DEFAULT_SIZE);
}
int CStatusbar::AddPart(HINSTANCE hInstance,int IdIcon,TCHAR* ToolTip)
{
    HICON hIcon=(HICON)LoadImage(hInstance, MAKEINTRESOURCE(IdIcon),IMAGE_ICON,0,0,LR_SHARED);
    return this->AddPart(hIcon,ToolTip);
}
int CStatusbar::AddPart(HINSTANCE hInstance,int IdIcon,int IconWidth,int IconHeight,TCHAR* ToolTip,int PartSize)
{
    int Index;
    Index=this->AddPart(PartSize);
    if (Index<0)
        return Index;

    HICON hIcon=(HICON)LoadImage(hInstance, MAKEINTRESOURCE(IdIcon),IMAGE_ICON,IconWidth,IconHeight,LR_SHARED);
    this->SetToolTip(Index,ToolTip);
    this->SetIcon(Index,hIcon);
    return Index;
}

int CStatusbar::AddPart(int Size)
{
    PART_INFOS PartInfo;
    memset(&PartInfo,0,sizeof(PART_INFOS));
    
    if (Size>=0)
    {
        // array of right client edge
        if (this->PartsInfo.size()>0)
        {
            // add previous right edge to current size
            PartInfo.Width=this->PartsInfo.back().Width;
        }

        PartInfo.Width+=Size;
    }
    else
        PartInfo.Width=Size;


    this->PartsInfo.push_back(PartInfo);

    size_t NbItems = this->PartsInfo.size();
    int* Array = (int*)_alloca(sizeof(int)*NbItems);
    for (size_t Cnt=0;Cnt<NbItems;Cnt++)
        Array[Cnt]=this->PartsInfo[Cnt].Width;

    if (!SendMessage(this->hwndStatus, SB_SETPARTS,(WPARAM)NbItems,(LPARAM)Array))
    {
        this->PartsInfo.pop_back();
        return -1;
    }

    
    return (int)(this->PartsInfo.size()-1);
}
int CStatusbar::AddPart(TCHAR* PartText,int Size)
{
    int Index;
    Index=this->AddPart(Size);
    if (Index<0)
        return Index;

    this->SetText(Index,PartText);
    return Index;
}
int CStatusbar::AddPart(TCHAR* PartText,TCHAR* ToolTip,int Size)
{
    int Index;
    Index=this->AddPart(Size);
    if (Index<0)
        return Index;
    this->SetToolTip(Index,ToolTip);
    this->SetText(Index,PartText);
    return Index;
}
int CStatusbar::AddPart(TCHAR* PartText,HICON hIcon,int Size)
{
    int Index;
    Index=this->AddPart(Size);
    if (Index<0)
        return Index;
    this->SetIcon(Index,hIcon);
    this->SetText(Index,PartText);
    return Index;
}
int CStatusbar::AddPart(TCHAR* PartText,HINSTANCE hInstance,int IdIcon,int Size)
{
    HICON hIcon=(HICON)LoadImage(hInstance, MAKEINTRESOURCE(IdIcon),IMAGE_ICON,0,0,LR_SHARED);
    return this->AddPart(PartText,hIcon,Size);
}
int CStatusbar::AddPart(HICON hIcon,TCHAR* ToolTip,int Size)
{
    int Index;
    Index=this->AddPart(Size);
    if (Index<0)
        return Index;
    this->SetToolTip(Index,ToolTip);
    this->SetIcon(Index,hIcon);
    return Index;
}

BOOL CStatusbar::SetPartSize(int PartIndex,int Size)
{
    if ( (PartIndex<0) || ( (size_t)PartIndex>=this->PartsInfo.size() ) )
        return FALSE;
    if (Size == -1)
    {
        if ((size_t)PartIndex!=this->PartsInfo.size()-1)
            return FALSE;
        this->PartsInfo[PartIndex].Width=Size;
    }
    else
    {
        int OldPartSize = this->PartsInfo[PartIndex].Width;
        if (OldPartSize==-1)
            OldPartSize=0;
        this->PartsInfo[PartIndex].Width=Size;
        int Delta= Size - OldPartSize;
        // update all parts after the changed one
        for(int Cnt=PartIndex+1;(size_t)Cnt<this->PartsInfo.size();Cnt++)
        {
            if (this->PartsInfo[Cnt].Width != -1)
                this->PartsInfo[Cnt].Width+=Delta;
        }
    }

    size_t NbItems = this->PartsInfo.size();
    int* Array = (int*)_alloca(sizeof(int)*NbItems);
    for (size_t Cnt=0;Cnt<NbItems;Cnt++)
        Array[Cnt]=this->PartsInfo[Cnt].Width;

    if (!SendMessage(this->hwndStatus, SB_SETPARTS,(WPARAM)NbItems,(LPARAM)Array))
        return FALSE;

    this->UpdateEmbeddedControlsPositions();

    return TRUE;
}

BOOL CStatusbar::SetControl(int PartIndex,HWND hWnd,int ControlMargin)
{
    if ( (PartIndex<0) || ( (size_t)PartIndex>=this->PartsInfo.size() ) )
        return FALSE;

    this->PartsInfo[PartIndex].hWndControl = hWnd;
    this->PartsInfo[PartIndex].ControlMargin = ControlMargin;
    RECT Rect;
    this->GetRect(PartIndex,&Rect);
    // position item and assume it is visible
    ::SetParent(hWnd,this->hwndStatus);// assume we are parent before calling SetWindowPos
    ::SetWindowPos(hWnd,0,
                    Rect.left+ControlMargin,
                    Rect.top+ControlMargin,
                    Rect.right-Rect.left-2*ControlMargin,
                    Rect.bottom-Rect.top-2*ControlMargin,
                    SWP_NOACTIVATE|SWP_NOZORDER|SWP_SHOWWINDOW
                    );
    return TRUE;
}

BOOL CStatusbar::SetIcon(int PartIndex,HICON hIcon)
{
    return (BOOL)SendMessage(this->hwndStatus,SB_SETICON,PartIndex,(LPARAM)hIcon);
}
BOOL CStatusbar::SetIcon(int PartIndex,HINSTANCE hInstance,int IdIcon)
{
    HICON hIcon=(HICON)LoadImage(hInstance, MAKEINTRESOURCE(IdIcon),IMAGE_ICON,0,0,LR_SHARED);
    return this->SetIcon(PartIndex,hIcon);
}
BOOL CStatusbar::SetIcon(int PartIndex,HINSTANCE hInstance,int IdIcon,int Width,int Height)
{
    HICON hIcon=(HICON)LoadImage(hInstance, MAKEINTRESOURCE(IdIcon),IMAGE_ICON,Width,Height,LR_SHARED);
    return this->SetIcon(PartIndex,hIcon);
}
BOOL CStatusbar::SetText(int PartIndex,TCHAR* PartText)
{
    return (BOOL)SendMessage(this->hwndStatus,SB_SETTEXT,PartIndex,(LPARAM)PartText);
}

// msdn : shown only if icon or text not fully displayed
BOOL CStatusbar::SetToolTip(int PartIndex,TCHAR* ToolTip)
{
    SendMessage(this->hwndStatus,SB_SETTIPTEXT,PartIndex,(LPARAM)ToolTip);
    // msdn : The SB_SETTIPTEXT return value is not used
    return TRUE;
}

// Retrieves the bounding rectangle of a part in a status window
BOOL CStatusbar::GetRect(int PartIndex,OUT RECT* pRect)
{
    return (BOOL)SendMessage(this->hwndStatus, SB_GETRECT,(WPARAM)PartIndex,(LPARAM)pRect);
}
/*
can't be set/unset after control creation
BOOL CStatusbar::ShowGrip(BOOL bShow)
{
    LONG_PTR Style = ::GetWindowLongPtr(this->hwndStatus,GWL_STYLE);
    if (bShow)
        Style |= SBARS_SIZEGRIP;
    else
        Style &= ~SBARS_SIZEGRIP;
    return ( ::SetWindowLongPtr(this->hwndStatus,GWL_STYLE,Style) != 0 );
}
*/

DWORD CStatusbar::GetHeight()
{
    RECT Rect;
    ::GetWindowRect(this->hwndStatus,&Rect);
    return (Rect.bottom - Rect.top);
}
