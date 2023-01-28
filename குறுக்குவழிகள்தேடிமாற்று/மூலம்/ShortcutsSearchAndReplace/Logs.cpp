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
// Object: display log dialog
//-----------------------------------------------------------------------------

#include "logs.h"


extern HINSTANCE DllhInstance;

CLogs::CLogs(void)
{
    this->hInstance=0;
    this->hWndDialog=0;
    this->pszLog=NULL;
}
CLogs::~CLogs(void)
{

}

//-----------------------------------------------------------------------------
// Name: Show
// Object: show the dialog box
// Parameters :
//     in  : HINSTANCE hInstance : application instance
//           HWND hWndDialog : main window dialog handle
//           TCHAR* pszFile : file to parse at startup
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CLogs::Show(HINSTANCE hInstance,HWND hWndParent,TCHAR* LogContent)
{
    if (IsBadReadPtr(LogContent,sizeof(TCHAR)))
        return;
    CLogs Logs;
    Logs.hInstance=hInstance;
    Logs.pszLog=LogContent;
    DialogBoxParam(hInstance, (LPCTSTR)IDD_DIALOG_LOGS, hWndParent, (DLGPROC)CLogs::WndProc,(LPARAM)&Logs);
}

//-----------------------------------------------------------------------------
// Name: GetAssociatedDialogObject
// Object: Get object associated to window handle
// Parameters :
//     in  : HWND hWndDialog : handle of the window
//     out :
//     return : associated object if found, NULL if not found
//-----------------------------------------------------------------------------
CLogs* CLogs::GetAssociatedDialogObject(HWND hWndDialog)
{
    return (CLogs*)GetWindowLongPtr(hWndDialog,GWLP_USERDATA);
}

//-----------------------------------------------------------------------------
// Name: OnSizing
// Object: check dialog box size
// Parameters :
//     in out  : RECT* pRect : pointer to dialog rect
//     return : 
//-----------------------------------------------------------------------------
void CLogs::OnSizing(RECT* pRect)
{
    // check min width and min height
    if ((pRect->right-pRect->left)<CLogs_DIALOG_MIN_WIDTH)
        pRect->right=pRect->left+CLogs_DIALOG_MIN_WIDTH;
    if ((pRect->bottom-pRect->top)<CLogs_DIALOG_MIN_HEIGHT)
        pRect->bottom=pRect->top+CLogs_DIALOG_MIN_HEIGHT;
}

//-----------------------------------------------------------------------------
// Name: OnSize
// Object: Resize controls
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CLogs::OnSize()
{
    RECT RectWindow;
    RECT Rect;
    DWORD SpaceBetweenControls;

    // get window Rect
    ::GetClientRect(this->hWndDialog,&RectWindow);

    ///////////////
    // listview 
    ///////////////
    HWND hWnd=::GetDlgItem(this->hWndDialog,IDC_EDIT_LOGS);
    
    CDialogHelper::GetClientWindowRect(this->hWndDialog,hWnd,&Rect);
    SpaceBetweenControls=Rect.left;
    SetWindowPos(hWnd,HWND_NOTOPMOST,
        0,0,
        (RectWindow.right-RectWindow.left)-2*SpaceBetweenControls,
        RectWindow.bottom-Rect.top-SpaceBetweenControls,
        SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);

    // redraw dialog
    CDialogHelper::Redraw(this->hWndDialog);
}

//-----------------------------------------------------------------------------
// Name: Init
// Object: vars init. Called at WM_INITDIALOG
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CLogs::Init()
{
    // render layout
    this->OnSize();

    HWND hwndControl=::GetDlgItem(this->hWndDialog,IDC_EDIT_LOGS);
    ::SetWindowText(hwndControl,this->pszLog);

    // MSDN Q96674 : avoid text selection
    ::SetFocus(hwndControl);
    ::PostMessage(hwndControl,EM_SETSEL,0,0);
}

//-----------------------------------------------------------------------------
// Name: Close
// Object: EndDialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CLogs::Close()
{
    EndDialog(this->hWndDialog,0);

    return;
}

//-----------------------------------------------------------------------------
// Name: WndProc
// Object: dialog callback of the dump dialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
LRESULT CALLBACK CLogs::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            CLogs* pDialog=(CLogs*)lParam;
            pDialog->hWndDialog=hWnd;

            SetWindowLongPtr(hWnd,GWLP_USERDATA,(LONG_PTR)pDialog);

            pDialog->Init();
            // load dlg icons
            CDialogHelper::SetIcon(hWnd,IDI_ICON_APP);
        }
        break;
    case WM_CLOSE:
        {
            CLogs* pDialog=CLogs::GetAssociatedDialogObject(hWnd);
            if (!pDialog)
                break;
            pDialog->Close();
        }
        break;
    case WM_SIZING:
        {
            CLogs* pDialog=CLogs::GetAssociatedDialogObject(hWnd);
            if (!pDialog)
                break;
            pDialog->OnSizing((RECT*)lParam);
        }
        break;
    case WM_SIZE:
        {
            CLogs* pDialog=CLogs::GetAssociatedDialogObject(hWnd);
            if (!pDialog)
                break;
            pDialog->OnSize();
        }
        break;
    case WM_COMMAND:
        {
/*
// not used yet
            CLogs* pDialog=CLogs::GetAssociatedDialogObject(hWnd);
            if (!pDialog)
                break;
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
                pDialog->Close();
                break;
            }
*/
        }
        break;
    default:
        return FALSE;
    }
    return TRUE;
}
