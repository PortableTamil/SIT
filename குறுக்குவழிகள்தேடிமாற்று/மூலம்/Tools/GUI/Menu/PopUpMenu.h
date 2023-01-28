/*
Copyright (C) 2004-2020 Jacquelin POTIER <jacquelin.potier@free.fr>
Dynamic aspect ratio code Copyright (C) 2004-2020 Jacquelin POTIER <jacquelin.potier@free.fr>

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
// Object: class helper for popupmenu control
//-----------------------------------------------------------------------------

#pragma once
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501 // for xp os
#endif
#include <windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)
#include <vector>

#include <commctrl.h>
#pragma comment (lib,"comctl32.lib")

class CPopUpMenu;
class IPopUpMenuEvents
{
public:
    virtual void OnPopUpMenuMouseRightButtonUp(CPopUpMenu* pSender,WPARAM wParam, LPARAM lParam)=0;
    virtual void OnPopUpMenuMenuSelect(CPopUpMenu* pSender,WPARAM wParam, LPARAM lParam)=0;
};

class CPopUpMenuEventsBase
{
public:
    virtual void OnPopUpMenuMouseRightButtonUp(CPopUpMenu* pSender,WPARAM wParam, LPARAM lParam)
    {
        UNREFERENCED_PARAMETER(pSender);
        UNREFERENCED_PARAMETER(wParam);
        UNREFERENCED_PARAMETER(lParam);
    }
    virtual void OnPopUpMenuMenuSelect(CPopUpMenu* pSender,WPARAM wParam, LPARAM lParam)
    {
        UNREFERENCED_PARAMETER(pSender);
        UNREFERENCED_PARAMETER(wParam);
        UNREFERENCED_PARAMETER(lParam);
    }
};

class CPopUpMenu
{
public:
    CPopUpMenu();
    CPopUpMenu(CPopUpMenu* ParentPopUpMenu);
    ~CPopUpMenu();

    BOOL SetEventsHandler(IPopUpMenuEvents* pPopUpMenuEvents);

    HMENU GetControlHandle();
    CPopUpMenu* GetParentPopUpMenu();

    UINT Add(TCHAR* Name);
    UINT Add(TCHAR* Name,UINT Index);
    UINT Add(TCHAR* Name,HICON hIcon);
    UINT Add(TCHAR* Name,HICON hIcon,UINT Index);
    UINT Add(TCHAR* Name,int IdIcon,HINSTANCE hInstance);
    UINT Add(TCHAR* Name,int IdIcon,HINSTANCE hInstance,UINT Index);
    UINT Add(TCHAR* Name,int IdIcon,HINSTANCE hInstance,int Width,int Height);
    UINT Add(TCHAR* Name,int IdIcon,HINSTANCE hInstance,int Width,int Height,UINT Index);

    UINT AddEx(TCHAR* Name,UINT MenuId);
    UINT AddEx(TCHAR* Name,HICON hIcon,UINT MenuId);
    UINT AddEx(TCHAR* Name,HICON hIcon,UINT Index,UINT MenuId);
    UINT AddEx(TCHAR* Name,int IdIcon,HINSTANCE hInstance,int Width,int Height,UINT MenuId);
    UINT AddEx(TCHAR* Name,int IdIcon,HINSTANCE hInstance,int Width,int Height,UINT Index,UINT MenuId);

    UINT AddSeparator();
    UINT AddSeparator(UINT Index);

    UINT AddSubMenu(TCHAR* SubMenuName,CPopUpMenu* SubMenu,UINT Index);
    UINT AddSubMenu(TCHAR* SubMenuName,CPopUpMenu* SubMenu);
    UINT AddSubMenu(TCHAR* SubMenuName,CPopUpMenu* SubMenu,int IdIcon,HINSTANCE hInstance,UINT Index);
    UINT AddSubMenu(TCHAR* SubMenuName,CPopUpMenu* SubMenu,int IdIcon,HINSTANCE hInstance,int Width,int Height,UINT Index);
    UINT AddSubMenu(TCHAR* SubMenuName,CPopUpMenu* SubMenu,int IdIcon,HINSTANCE hInstance);
    UINT AddSubMenu(TCHAR* SubMenuName,CPopUpMenu* SubMenu,int IdIcon,HINSTANCE hInstance,int Width,int Height);
    UINT AddSubMenu(TCHAR* SubMenuName,CPopUpMenu* SubMenu,HICON hIcon,UINT Index);
    UINT AddSubMenu(TCHAR* SubMenuName,CPopUpMenu* SubMenu,HICON hIcon);

    void SetCheckedState(UINT MenuID,BOOL bChecked);
    BOOL IsChecked(UINT MenuID);
    void SetEnabledState(UINT MenuID,BOOL bEnabled);
    BOOL IsEnabled(UINT MenuID);
    BOOL SetText(UINT MenuID,TCHAR* pszText);
    BOOL SetIcon(UINT MenuID,int IdIcon,HINSTANCE hInstance);
    BOOL SetIcon(UINT MenuID,int IdIcon,HINSTANCE hInstance,int Width,int Height);
    BOOL SetIcon(UINT MenuID,HICON hIcon);
    int  GetText(UINT MenuID,TCHAR* pszText,int pszTextMaxSize);
    CPopUpMenu* GetSubMenu(UINT MenuID);

    void Remove(UINT MenuID);
    void RemoveAll();
    void RemoveAllEx(BOOL ResetMenuId);

    int GetItemCount();
    int GetID(UINT MenuIndex);
    int GetIndex(UINT MenuId);
    BOOL GetNameLength(IN UINT MenuIndex,OUT SIZE_T* pLength);
    BOOL GetName(IN UINT MenuIndex,IN OUT TCHAR* Name,IN SIZE_T NameMaxLength);
    int GetIndexFromName(TCHAR* Name);
    BOOL IsNameUsed(TCHAR* Name){return (this->GetIndexFromName(Name)!=-1);}
    UINT GetItemStateFromIndex(int Index){return ::GetMenuState(this->hPopUpMenu,Index,MF_BYPOSITION);}
    UINT GetItemStateFromID(int ID){return ::GetMenuState(this->hPopUpMenu,ID,MF_BYCOMMAND);}

    BOOL IsItemHighLightedFromIndex(int Index)
    {
        UINT ui = ::GetMenuState(this->hPopUpMenu,Index,MF_BYPOSITION);
        ui = ui & MF_HILITE;
        return ( ui == MF_HILITE );
    }
    BOOL IsItemHighLightedFromID(int ID)
    {
        UINT ui = ::GetMenuState(this->hPopUpMenu,ID,MF_BYCOMMAND);
        ui = ui & MF_HILITE;
        return ( ui == MF_HILITE );
    }
    BOOL IsItemSubMenuFromIndex(int Index)
    {
        UINT ui = ::GetMenuState(this->hPopUpMenu,Index,MF_BYPOSITION);
        ui = ui & MF_POPUP;
        return ( ui == MF_POPUP );
    }
    BOOL IsItemSubMenuFromID(int ID)
    {
        UINT ui = ::GetMenuState(this->hPopUpMenu,ID,MF_BYCOMMAND);
        ui = ui & MF_POPUP;
        return ( ui == MF_POPUP );
    }
    BOOL IsItemHighLightedSubMenuFromIndex(int Index)
    {
        UINT ui = ::GetMenuState(this->hPopUpMenu,Index,MF_BYPOSITION);
        ui = ui & (MF_POPUP | MF_HILITE);
        return ( ui == (MF_POPUP | MF_HILITE) );
    }
    BOOL IsItemHighLightedSubMenuFromID(int ID)
    {
        UINT ui = ::GetMenuState(this->hPopUpMenu,ID,MF_BYCOMMAND);
        ui = ui & (MF_POPUP | MF_HILITE);
        return ( ui == (MF_POPUP | MF_HILITE) );
    }

    UINT Show(int x,int y, HWND hOwner);
    UINT Show(int x,int y, HWND hOwner,BOOL PositionRelativeToOwner);
    UINT Show(int x,int y, HWND hOwner,BOOL PositionRelativeToOwner,BOOL ShowUpper);

    UINT GetNextMenuId();
    UINT GetMaxMenuId();
    BOOL ReplaceMenuId(UINT OldMenuID,UINT NewMenuID);

    BOOL bAllowIconsEffects;
private:
    CPopUpMenu* ParentPopUpMenu;
    HMENU hPopUpMenu;
    int CurrentMenuId;
    BOOL bUseThemingForVistaAndSeven;
    std::vector<HBITMAP> ListLoadedBitmapToFree;

    IPopUpMenuEvents* pPopUpMenuEvents;

    BOOL GetMenuFromPoint(POINT* pt,HMENU* phWndMenu,UINT* pMenuIndex);
    void CommonConstructor();
    void SetMenuItemBitmapInfo(MENUITEMINFO* pMenuItem,HICON hIcon);
    BOOL IsSubMenu(HMENU hMenu,HMENU hSubMenu);
    BOOL OnMeasureItem(HWND hwnd, LPMEASUREITEMSTRUCT lpmis);
    BOOL OnDrawItem(HWND hwnd, LPDRAWITEMSTRUCT lpdis);
    void OnMouseRightButtonUp(WPARAM wParam, LPARAM lParam);
    void OnMenuSelect(WPARAM wParam, LPARAM lParam);
    void FreeItemMemory(UINT MenuID);
    void FreeItemBitmap(UINT MenuID);
    static LRESULT CALLBACK SubClassWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData);
};
