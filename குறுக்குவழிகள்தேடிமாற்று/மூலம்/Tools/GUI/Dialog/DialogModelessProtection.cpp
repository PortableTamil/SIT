#include "DialogModelessProtection.h"

int MessageBoxModelessProtected(
    HWND    hWnd,
    LPCTSTR lpText,
    LPCTSTR lpCaption,
    UINT    uType
    )
{
    HWND hWndRoot = ::GetAncestor(hWnd, GA_ROOT);
    HWND hWndRootOwner = ::GetAncestor(hWnd, GA_ROOTOWNER);
    CDialogBase_DisableClosing(hWndRoot);
    CDialogBase_DisableClosing(hWndRootOwner);
    int Ret = ::MessageBox(hWnd,lpText,lpCaption,uType);
    CDialogBase_EnableClosing(hWndRootOwner);
    CDialogBase_EnableClosing(hWndRoot);
    return Ret;
}

BOOL GetOpenFileNameModelessProtected(LPOPENFILENAME pOfn)
{
    HWND hWndRoot = ::GetAncestor(pOfn->hwndOwner, GA_ROOT);
    HWND hWndRootOwner = ::GetAncestor(pOfn->hwndOwner, GA_ROOTOWNER);
    CDialogBase_DisableClosing(hWndRoot);
    CDialogBase_DisableClosing(hWndRootOwner);
    BOOL Ret = GetOpenFileName(pOfn);
    CDialogBase_EnableClosing(hWndRootOwner);
    CDialogBase_EnableClosing(hWndRoot);
    return Ret;
}

BOOL GetSaveFileNameModelessProtected(LPOPENFILENAME pOfn)
{
    HWND hWndRoot = ::GetAncestor(pOfn->hwndOwner, GA_ROOT);
    HWND hWndRootOwner = ::GetAncestor(pOfn->hwndOwner, GA_ROOTOWNER);
    CDialogBase_DisableClosing(hWndRoot);
    CDialogBase_DisableClosing(hWndRootOwner);
    BOOL Ret = GetSaveFileName(pOfn);
    CDialogBase_EnableClosing(hWndRootOwner);
    CDialogBase_EnableClosing(hWndRoot);
    return Ret;
}
