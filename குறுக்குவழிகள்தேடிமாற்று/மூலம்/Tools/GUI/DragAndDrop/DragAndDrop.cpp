/*
Copyright (C) 2004-2016 Jacquelin POTIER <jacquelin.potier@free.fr>
Dynamic aspect ratio code Copyright (C) 2004-2015 Jacquelin POTIER <jacquelin.potier@free.fr>

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

#include "DragAndDrop.h"

#include <shellapi.h>
#pragma comment (lib,"shell32.lib")

CDragAndDrop::CDragAndDrop(HWND hWndDialog, pfFileDroppedCallBack FileDroppedCallBack,LPVOID FileDroppedCallBackUserParam)
{
    this->hWndDialog = hWndDialog;
    this->FileDroppedCallBack = FileDroppedCallBack;
    this->FileDroppedCallBackUserParam = FileDroppedCallBackUserParam;
}

CDragAndDrop::~CDragAndDrop(void)
{
}

void CDragAndDrop::Start()
{
    if (::IsBadCodePtr((FARPROC)this->FileDroppedCallBack))
        this->FileDroppedCallBack = NULL;
    ::DragAcceptFiles(this->hWndDialog,TRUE);
}
void CDragAndDrop::Stop()
{
    ::DragAcceptFiles(this->hWndDialog,FALSE);
}

// must be called by wnd proc on WM_DROPFILES messages
void CDragAndDrop::OnWM_DROPFILES(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    HDROP hDrop = (HDROP)wParam;


    TCHAR pszFileName[MAX_PATH];
    UINT NbFiles;
    UINT Cnt;
    POINT pt;
    HWND SubWindowHandle;

    // retrieve dialog subitem window handle
    ::DragQueryPoint(hDrop, &pt);
    ::ClientToScreen(this->hWndDialog,&pt);
    SubWindowHandle=::WindowFromPoint(pt);

    // get number of files count in the drag and drop
    NbFiles=DragQueryFile(hDrop, 0xFFFFFFFF,NULL, MAX_PATH);

    // for each dropped file
    for (Cnt=0;Cnt<NbFiles;Cnt++)
    {
        // get dropped file name
        ::DragQueryFile(hDrop, Cnt,pszFileName, MAX_PATH);

        // call callback if any
        if (this->FileDroppedCallBack)
            this->FileDroppedCallBack(SubWindowHandle, pszFileName, Cnt, this->FileDroppedCallBackUserParam);
    }

    // end current drag and drop operation
    ::DragFinish(hDrop);
}

