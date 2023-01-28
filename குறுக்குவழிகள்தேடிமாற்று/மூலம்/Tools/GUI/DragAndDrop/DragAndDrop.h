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

#pragma once

#include <Windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

class CDragAndDrop
{

public:
    typedef void (*pfFileDroppedCallBack)(HWND hWndControl, TCHAR* DroppedFile, SIZE_T FileDroppedIndex, LPVOID UserParam);

    CDragAndDrop(HWND hWndDialog, pfFileDroppedCallBack FileDroppedCallBack,LPVOID FileDroppedCallBackUserParam);
    ~CDragAndDrop(void);
    void Start();
    void Stop();
    // must be called by wnd proc on WM_DROPFILES messages
    void OnWM_DROPFILES(WPARAM wParam, LPARAM lParam = 0);

private:
    HWND hWndDialog;
    pfFileDroppedCallBack FileDroppedCallBack;
    LPVOID FileDroppedCallBackUserParam;
};
