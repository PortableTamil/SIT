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
// Object: class helper for Toolbar control
//-----------------------------------------------------------------------------

#pragma once

// required lib : comctl32.lib
#pragma comment (lib,"comctl32")
// require manifest to make SetView work

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501 // for xp os
#endif

#include <windows.h>
#include <commctrl.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

#include "../menu/popupmenu.h" // for drop down menu

#define CToolbar_DEFAULT_IMAGELIST_SIZE 20
#define CToolbar_IMAGELIST_GROW_SIZE 5

#define CToolbar_MARKER_PROP_NAME _T("CToolbarMarkerPropName")

class CToolbar;
class IToolbarEvents
{
public:
    virtual void OnToolbarDropDownMenuBeforeMenuShow(CToolbar* pSender,CPopUpMenu* PopUpMenu)=0; // provide handler to adjust menu before showing it
    virtual void OnToolbarDropDownMenu(CToolbar* pSender,CPopUpMenu* PopUpMenu,UINT MenuId)=0; // call after valid menu entry click
};

class CToolbarEventsBase:public IToolbarEvents
{
public:
    virtual void OnToolbarDropDownMenuBeforeMenuShow(CToolbar* pSender,CPopUpMenu* PopUpMenu)
    {
        UNREFERENCED_PARAMETER(pSender);
        UNREFERENCED_PARAMETER(PopUpMenu);
    }
    virtual void OnToolbarDropDownMenu(CToolbar* pSender,CPopUpMenu* PopUpMenu,UINT MenuId)
    {
        UNREFERENCED_PARAMETER(pSender);
        UNREFERENCED_PARAMETER(PopUpMenu);
        UNREFERENCED_PARAMETER(MenuId);
    }
};

class CToolbarButtonUserData
{
public:
    CToolbarButtonUserData(TCHAR* ToolTip,CPopUpMenu* PopUpMenu);
    ~CToolbarButtonUserData();
    void SetToolTip(TCHAR* ToolTip);
    TCHAR* ToolTip;
    CPopUpMenu* PopUpMenu;
};

class CToolbar
{
public:
    enum tagCToolbarPosition
    {
        ToolbarPosition_USER=0,
        ToolbarPosition_TOP=CCS_TOP,
        ToolbarPosition_LEFT=CCS_LEFT,
        ToolbarPosition_BOTTOM=CCS_BOTTOM,
        ToolbarPosition_RIGHT=CCS_RIGHT
     };
private:
    HIMAGELIST hImgList;
    HIMAGELIST hImgListDisabled;
    HIMAGELIST hImgListHot;
    HWND hwndTB;
    HWND hwndToolTip;
    HINSTANCE hInstance;
    BOOL VerticalDirection;
    IToolbarEvents* pToolbarEvents;
    void Constructor(HINSTANCE hInstance,HWND hwndParent,BOOL bFlat,BOOL bListStyle,DWORD dwIconWidth,DWORD dwIconHeight,int ControlId);
public:
    enum ImageListType
    {
        ImageListTypeEnable,
        ImageListTypeDisable,
        ImageListTypeHot
    };

    CToolbar(HINSTANCE hInstance,HWND hwndParent,int ControlId);
    CToolbar(HINSTANCE hInstance,HWND hwndParent,BOOL bFlat,BOOL bListStyle,int ControlId);
    CToolbar(HINSTANCE hInstance,HWND hwndParent,BOOL bFlat,BOOL bListStyle,DWORD dwIconWidth,DWORD dwIconHeight,int ControlId);
    ~CToolbar(void);
    BOOL OnNotify(WPARAM wParam, LPARAM lParam);

    BOOL SetEventsHandler(IToolbarEvents* pToolbarEvents);
    IToolbarEvents* GetEventsHandler();

    HWND GetControlHandle();
    void EnableButton(int ButtonID,BOOL bEnable);
    
    void AddSeparator();
    int GetButtonCount();

    BOOL RemoveButton(int ButtonID);
    int GetButtonIndex(int ButtonID);
    int GetButtonId(int ButtonIndex);
    BYTE GetButtonStyle(int ButtonID);
    BYTE SetButtonStyle(int ButtonID,BYTE Style);
    BYTE GetButtonState(int ButtonID);
    FORCEINLINE BOOL IsButtonChecked(int ButtonID)
    {
        return (this->GetButtonState(ButtonID) & TBSTATE_CHECKED);
    }
    FORCEINLINE BOOL IsButtonEnabled(int ButtonID)
    {
        return (this->GetButtonState(ButtonID) & TBSTATE_ENABLED);
    }
    BOOL SetButtonState(int ButtonID,BYTE State);
    BOOL SetButtonCheckedState(int ButtonID,BOOL bChecked);
    

    BOOL AddButton(int ButtonID,TCHAR* ButtonText);
    BOOL AddButton(int ButtonID,TCHAR* ButtonText,TCHAR* ToolTip);
    BOOL AddButton(int ButtonID,TCHAR* ButtonText,int IdIcon,int Width,int Height);
    BOOL AddButton(int ButtonID,TCHAR* ButtonText,int IdIcon,TCHAR* ToolTip,int Width,int Height);
    BOOL AddButton(int ButtonID,TCHAR* ButtonText,int IdIconEnable,int IdIconDisable,int Width,int Height);
    BOOL AddButton(int ButtonID,TCHAR* ButtonText,int IdIconEnable,int IdIconDisable,TCHAR* ToolTip,int Width,int Height);
    BOOL AddButton(int ButtonID,TCHAR* ButtonText,int IdIconEnable,int IdIconDisable,int IdIconHot,int Width,int Height);
    BOOL AddButton(int ButtonID,TCHAR* ButtonText,int IdIconEnable,int IdIconDisable,int IdIconHot,TCHAR* ToolTip,int Width,int Height);
    BOOL AddButton(int ButtonID,int IdIcon,TCHAR* ToolTip,int Width=0,int Height=0);
    BOOL AddButton(int ButtonID,int IdIconEnable,int IdIconDisable,TCHAR* ToolTip,int Width=0,int Height=0);
    BOOL AddButton(int ButtonID,int IdIconEnable,int IdIconDisable,int IdIconHot,TCHAR* ToolTip,int Width=0,int Height=0);
    BOOL AddButton(int ButtonID,TCHAR* ButtonText,BYTE ButtonStyle,BYTE ButtonState,int IdIconEnable,int IdIconDisable,int IdIconHot,TCHAR* ToolTip,int Width=0,int Height=0);
    BOOL AddButton(int ButtonID,TCHAR* ButtonText,BYTE ButtonStyle,BYTE ButtonState,int IdIconEnable,int IdIconDisable,int IdIconHot,TCHAR* ToolTip,CPopUpMenu* PopUpMenu,int Width=0,int Height=0);

    BOOL AddDropDownButton(int ButtonID,int IdIcon,TCHAR* ToolTip,CPopUpMenu* PopUpMenu,BOOL bWholeDropDown,int Width=0,int Height=0);
    BOOL AddDropDownButton(int ButtonID,int IdIconEnable,int IdIconDisable,int IdIconHot,TCHAR* ToolTip,CPopUpMenu* PopUpMenu,BOOL bWholeDropDown,int Width=0,int Height=0);
    BOOL AddDropDownButton(int ButtonID,TCHAR* ButtonText,int IdIcon,TCHAR* ToolTip,CPopUpMenu* PopUpMenu,BOOL bWholeDropDown,int Width=0,int Height=0);
    BOOL AddDropDownButton(int ButtonID,TCHAR* ButtonText,int IdIconEnable,int IdIconDisable,int IdIconHot,TCHAR* ToolTip,CPopUpMenu* PopUpMenu,BOOL bWholeDropDown,int Width=0,int Height=0);
    BOOL AddDropDownButton(int ButtonID,TCHAR* ButtonText,BYTE ButtonState,int IdIconEnable,int IdIconDisable,int IdIconHot,TCHAR* ToolTip,CPopUpMenu* PopUpMenu,BOOL bWholeDropDown,int Width=0,int Height=0);
    CPopUpMenu* GetButtonDropDownMenu(int ButtonID);

    BOOL AddCheckButton(int ButtonID,int IdIcon,TCHAR* ToolTip,int Width=0,int Height=0);
    BOOL AddCheckButton(int ButtonID,int IdIcon,int IdIconDisabled,int IdIconHot,TCHAR* ToolTip,int Width=0,int Height=0);
  
    void Autosize();
    BOOL EnableParentAlign(BOOL bEnable);
    BOOL SetDirection(BOOL bHorizontal);
    BOOL EnableDivider(BOOL bEnable);
    BOOL SetPosition(tagCToolbarPosition Position);
    BOOL SetPosition(LONG x,LONG y,LONG Width,LONG Height);

    BOOL ReplaceIcon(int ButtonID,CToolbar::ImageListType ImgListType,int IdNewIcon,int Width=0,int Height=0);
    BOOL ReplaceAllIcons(int ButtonID,int IdIcon,int IdIconDisabled,int IdIconHot,int Width=0,int Height=0);
    BOOL ReplaceText(int ButtonID,TCHAR* NewText);
    BOOL ReplaceToolTipText(int ButtonID,TCHAR* NewToolTipText);
};
