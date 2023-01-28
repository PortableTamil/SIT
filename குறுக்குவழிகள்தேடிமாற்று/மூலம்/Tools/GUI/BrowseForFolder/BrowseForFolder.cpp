/*
Copyright (C) 2004-2016 Jacquelin POTIER <jacquelin.potier@free.fr>
Dynamic aspect ratio code Copyright (C) 2004-2016 Jacquelin POTIER <jacquelin.potier@free.fr>

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
// Object: Display Browse for folder dialog
//-----------------------------------------------------------------------------
#include "browseforfolder.h"

//-----------------------------------------------------------------------------
// Name: BrowseForDirectory
// Object: show browse for folder dialog
// Parameters :
//     in  : HWND hResultWindow : window handle which text is going to be field by BrowseForDirectory
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CBrowseForFolder::BrowseForFolderFromHwnd(HWND hDialog,HWND hResultWindow,TCHAR* Title)
{

    CBrowseForFolder BrowseForFolder;
    TCHAR CurrentPath[MAX_PATH];
    GetWindowText(hResultWindow,CurrentPath,MAX_PATH);
    if (*CurrentPath==0)
        ::GetCurrentDirectory(MAX_PATH,CurrentPath);
    if (BrowseForFolder.BrowseForFolder(hDialog,CurrentPath,
                                        Title,
                                        BIF_EDITBOX | BIF_NEWDIALOGSTYLE
                                        )
        )
        SetWindowText(hResultWindow,BrowseForFolder.szSelectedPath);
}

CBrowseForFolder::CBrowseForFolder()
{
    // COM must have been initialize by caller
    // CoInitialize(NULL);
    *this->szSelectedPath=0;
    *this->szInitDir = 0;
}

CBrowseForFolder::~CBrowseForFolder(void)
{
}

BOOL CBrowseForFolder::BrowseForFolder()
{
    return this->BrowseForFolder(NULL);
}

BOOL CBrowseForFolder::BrowseForFolder(HWND Owner)
{
    return this->BrowseForFolder(Owner,NULL);
}

BOOL CBrowseForFolder::BrowseForFolder(HWND Owner,TCHAR* szInitDir)
{
    return this->BrowseForFolder(Owner,szInitDir,NULL,BIF_EDITBOX | BIF_NEWDIALOGSTYLE);
}

BOOL CBrowseForFolder::BrowseForFolder(HWND Owner,TCHAR* szInitDir,TCHAR* szTitle,UINT ulFlags)
{
    LPITEMIDLIST pIDL;
    LPMALLOC pMalloc;
    BOOL bRet=FALSE;

    TCHAR sz[MAX_PATH];
    BOOL bEmptyInitDir=FALSE;

    if (szInitDir == NULL)
        bEmptyInitDir =TRUE;
    else if (*szInitDir == 0)
        bEmptyInitDir =TRUE;

    if (bEmptyInitDir)
    {
        // fill the initial directory with current dir
        ::GetCurrentDirectory(MAX_PATH, sz);
        szInitDir = sz;
    }

    _tcsncpy(this->szInitDir,szInitDir,MAX_PATH);
    this->szInitDir[MAX_PATH-1]=0;

    memset(&this->bi,0,sizeof(bi));
    this->bi.hwndOwner = Owner;
    this->bi.pszDisplayName = this->szSelectedPath;
    this->bi.pidlRoot = 0;
    this->bi.ulFlags = ulFlags;
    this->bi.lpfn = CBrowseForFolder::BrowseCallbackProc;
    this->bi.lParam=(LPARAM)this;
    this->bi.lpszTitle=szTitle;

    if (SHGetMalloc(&pMalloc)!=NOERROR)
        return FALSE;

    pIDL=SHBrowseForFolder(&this->bi);
    if (pIDL)
    {
        bRet=SHGetPathFromIDList(pIDL, this->szSelectedPath);// get the path name from the ID list
        pMalloc->Free((void*)pIDL);
    }
    pMalloc->Release();

    return bRet;
}


INT CALLBACK CBrowseForFolder::BrowseCallbackProc(HWND hwnd, 
                                                    UINT uMsg,
                                                    LPARAM lp, 
                                                    LPARAM pData) 
{
    CBrowseForFolder* pBrowseForFolder=(CBrowseForFolder*) pData;

    switch(uMsg) 
    {
    case BFFM_INITIALIZED: 
        {

            // center dialog
            RECT Rect;
            RECT BrowserRect;
            GetWindowRect(hwnd,&BrowserRect);

            if (pBrowseForFolder->bi.hwndOwner)
            {
                GetWindowRect(pBrowseForFolder->bi.hwndOwner,&Rect);
                BrowserRect.left=Rect.left+((int)(Rect.right-Rect.left-(BrowserRect.right-BrowserRect.left)))/2; // Warning : force int cast to make sign div
                BrowserRect.top=Rect.top+((int)(Rect.bottom-Rect.top-(BrowserRect.bottom-BrowserRect.top)))/2; // Warning : force int cast to make sign div
            }
            else
            {
                BrowserRect.left=((int)(::GetSystemMetrics(SM_CXFULLSCREEN)-(BrowserRect.right-BrowserRect.left)))/2; // Warning : force int cast to make sign div
                BrowserRect.top=((int)(::GetSystemMetrics(SM_CYFULLSCREEN)-(BrowserRect.bottom-BrowserRect.top)))/2; // Warning : force int cast to make sign div
            }
            SetWindowPos(hwnd, HWND_TOP, BrowserRect.left,BrowserRect.top, 0, 0, SWP_NOSIZE);


            if (pBrowseForFolder->szInitDir)
            {
                // WParam is TRUE since you are passing a path.
                // It would be FALSE if you were passing a pidl.
                SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)pBrowseForFolder->szInitDir);
            }
        }
        break;

    // for old dialog style with BIF_STATUSTEXT
    case BFFM_SELCHANGED: 
        // Set the status window to the currently selected path.
        if (SHGetPathFromIDList((LPITEMIDLIST) lp ,pBrowseForFolder->szSelectedPath))
        {
            SendMessage(hwnd,BFFM_SETSTATUSTEXT,0,(LPARAM)pBrowseForFolder->szSelectedPath);
        }
        break;

    }
    return 0;
}
