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
// Object: class helper for dialog window
//-----------------------------------------------------------------------------

#pragma once
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif
#include <windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

#ifndef DeleteAndCleanIfNotNull
// usage : DeleteAndCleanIfNotNull(this->pObject)
#define DeleteAndCleanIfNotNull(x) if (x){delete x; x=NULL;}
#endif

#ifndef DeleteAndCleanArrayIfNotNull
// usage : DeleteAndCleanArrayIfNotNull(Array)
#define DeleteAndCleanArrayIfNotNull(Array) if (Array){delete[] Array; Array=NULL;}
#endif

#ifndef CloseHandleAndCleanIfNotNull
// usage : CloseHandleAndCleanIfNotNull(hHandle)
#define CloseHandleAndCleanIfNotNull(x) if (x){::CloseHandle(x); x=NULL;}
#endif

#ifndef DestroyIconAndCleanIfNotNull
// usage : DestroyIconAndCleanIfNotNull(hIcon)
#define DestroyIconAndCleanIfNotNull(x) if (x){::DestroyIcon(x); x=NULL;}
#endif

#ifndef DestroyWindowAndCleanIfNotNull
// usage : DestroyIconAndCleanIfNotNull(hWnd)
#define DestroyWindowAndCleanIfNotNull(x) if (x){::DestroyWindow(x); x=NULL;}
#endif

#ifndef DeleteFontAndCleanIfNotNull
// usage : DeleteFontAndCleanIfNotNull(hFont)
#define DeleteFontAndCleanIfNotNull(x) if (x){DeleteObject((HGDIOBJ)(HFONT)(x)); x=NULL;}
#endif

#ifndef _DefineStringLen
// _DefineStringLen : remove string \0 from array size
#define _DefineStringLen(Array) (_countof(Array)-1)
#endif

#ifndef BreakInDebugMode
#ifdef _DEBUG
#define BreakInDebugMode if ( ::IsDebuggerPresent() ) { ::DebugBreak();}
#else 
#define BreakInDebugMode 
#endif
#endif

#define CDialogBase_SpaceBetweenControls 5


class CDialogHelper
{
private:
    typedef struct tagMoveGroupStruct
    {
        POINT  Point;
        HWND   hClient;
    }MOVE_GROUP_STRUCT,*PMOVE_GROUP_STRUCT;

    static BOOL CallBackRedrawGroup(HWND Item,PVOID UserParam);
    static BOOL CallBackShowGroup(HWND Item,PVOID UserParam);
    static BOOL CallBackEnableGroup(HWND Item,PVOID UserParam);
    static BOOL CallBackMoveGroup(HWND Item,PVOID UserParam);

public:
    FORCEINLINE static BOOL SetFocus(HWND hDlg,HWND hWndControl){return (BOOL)::SendMessage(hDlg, WM_NEXTDLGCTL, (WPARAM)hWndControl, TRUE);}// we must use WM_NEXTDLGCTL for a dialog proc

    /////////////////
    // dialog icon actions
    /////////////////
    static void SetIcon(HINSTANCE hInstance,HWND hWnd,int IdIconBig,int IdIconSmall);
    static void SetIcon(HINSTANCE hInstance,HWND hWnd,int IdIcon);
    static void SetIcon(HWND hWnd,int IdIconBig,int IdIconSmall);
    static void SetIcon(HWND hWnd,int IdIcon);
    static void SetIcon(HWND hWnd,HICON hIcon);
    static void SetIcon(HWND hWnd,HICON hIconBig,HICON hIconSmall);

    // do not call Destroy icon for returned icon handle
    FORCEINLINE static HICON LoadIcon(HINSTANCE hInstance,int IconId,int Width,int Height){return (HICON)::LoadImage(hInstance,MAKEINTRESOURCE(IconId),IMAGE_ICON,Width,Height,LR_DEFAULTCOLOR|LR_SHARED);}
    FORCEINLINE static LRESULT SetStaticIcon(HWND hDlg,int StaticControlId,HICON hIcon){return ::SendMessage(::GetDlgItem(hDlg,StaticControlId),STM_SETICON,(WPARAM)hIcon,0);}
    FORCEINLINE static LRESULT SetDlgButtonIcon(HWND hDlg,int ButtonControlId,HICON hIcon){return ::SendMessage(::GetDlgItem(hDlg,ButtonControlId),BM_SETIMAGE,(WPARAM)IMAGE_ICON,(LPARAM)hIcon);}
    static HICON SetDlgButtonIcon(HINSTANCE hInstance,HWND hDlg,int ButtonControlId,int IconId,int Width,int Height);

    FORCEINLINE static HWND GetParentDialog(HWND hWnd){return (HWND)::GetWindowLongPtr(hWnd,GWLP_HWNDPARENT);}
    FORCEINLINE static BOOL IsPopupWindow(HWND hWnd){ return ((::GetWindowLongPtr(hWnd ,GWL_STYLE) & WS_POPUP) == WS_POPUP); }
    static HWND GetParentPopUpDialog(HWND hWnd);

    /////////////////
    // instance
    /////////////////
    static HINSTANCE GetInstance(HWND hWnd);

    /////////////////
    // positioning
    /////////////////
    static BOOL ScreenToClient (HWND hDlg, LPRECT pRect);
    static BOOL ClientToScreen (HWND hDlg, LPRECT pRect);
    static BOOL GetClientWindowRect(HWND hDlg,HWND hItem,LPRECT lpRect);
    static BOOL GetClientWindowRectFromId(HWND hDlg,int Id,LPRECT lpRect);
    static BOOL SetWindowPosUnderItem(HWND hClient,HWND hWnd,HWND hWindowReferenceItem,DWORD SpaceBetweenControls);
    static BOOL SetWindowPosAtRightOfItem(HWND hClient,HWND hWnd,HWND hWindowReferenceItem,DWORD SpaceBetweenControls);

    static BOOL GetDialogInternalRect(HWND hDlg,LPRECT lpRect);
    
    /////////////////
    // checked button
    /////////////////
    static BOOL IsButtonChecked(HWND hWnd);
    static BOOL IsButtonChecked(HWND hWndDialog,int ButtonID);
    static void SetButtonCheckState(HWND hWnd,BOOL bChecked);
    static void SetButtonCheckState(HWND hWndDialog,int ButtonID,BOOL bChecked);

    ////////////////
    // Combo box
    ////////////////
    
    static BOOL IsComboboxHandle(HWND hCombo, HWND hItemToTest);
    static HWND GetComboboxEditHandle(HWND hCombo);
    static HWND GetComboboxExEditHandle(HWND hCombo);
    static LRESULT AddStringToComboboxEx(HWND hComboBoxEx,TCHAR* String, LPARAM Param = NULL);

    /////////////////
    // redraw
    /////////////////
    static BOOL Redraw(HWND hItem);
    static BOOL EnableRedrawFromHwnd(IN HWND hWnd,IN BOOL bEnable);

    /////////////////
    // transparency and fading
    /////////////////
    static void SetTransparency(HWND hWnd,BYTE Opacity);
    static void Fading(HWND hWnd,BYTE BeginOpacity,BYTE EndOpacity,BYTE Step);
    static void SetTransparentColor(HWND hWnd,COLORREF crKey);

    /////////////////
    // group actions
    /////////////////
    static BOOL RedrawGroup(HWND hWndGroup);
    static BOOL ShowGroup(HWND hWndGroup,int CmdShow);
    static BOOL EnableGroup(HWND hWndGroup,BOOL bEnable);
    static BOOL MoveGroupTo(HWND hClient,HWND hWndGroup,POINT* pPoint);
    static BOOL SetGroupPosUnderWindow(HWND hClient,HWND hWnd,HWND hWindowReferenceItem,DWORD SpaceBetweenControls);
    static BOOL SetGroupPosAtRightOfWindow(HWND hClient,HWND hWnd,HWND hWindowReferenceItem,DWORD SpaceBetweenControls);

    typedef BOOL (*pfCallBackGroupAction)(HWND Item,PVOID UserParam);
    static BOOL ParseGroupForAction(HWND hWndGroup,CDialogHelper::pfCallBackGroupAction CallBackFunction,PVOID UserParam);

    /////////////////
    // controls creation
    /////////////////
    static HWND CreateButton(HWND hWndParent,int x,int y,int Width,int Height,int Identifier,LPCTSTR ButtonText);
    static HWND CreateStatic(HWND hWndParent,int x,int y,int Width,int Height,int Identifier,LPCTSTR StaticText);
    static HWND CreateStaticCentered(HWND hWndParent,int x,int y,int Width,int Height,int Identifier,LPCTSTR StaticText);
    static HWND CreateEdit(HWND hWndParent,int x,int y,int Width,int Height,int Identifier,LPCTSTR EditText);
    static HWND CreateEditMultiLines(HWND hWndParent,int x,int y,int Width,int Height,int Identifier,LPCTSTR EditText);

};
