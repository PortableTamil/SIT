
#pragma once
#include <windows.h>

/*
#define WM_USER                         0x0400
#define WM_APP                          0x8000

0 through WM_USER –1
	Messages reserved for use by the system.
WM_USER through 0x7FFF
	Integer messages for use by private window classes.
WM_APP through 0xBFFF
	Messages available for use by applications.
0xC000 through 0xFFFF
	String messages for use by applications.
Greater than 0xFFFF
	Reserved by the system.
*/
#define WM_DIALOG_BASE_ENABLE_CLOSING  (WM_APP+0x2465) // just something which hope to be unique avoid 0xDAD, 0xBAD there :)

#define CDialogBase_DisableClosing(hWndDialogBase) ::SendMessage(hWndDialogBase,WM_DIALOG_BASE_ENABLE_CLOSING,0,0)
#define CDialogBase_EnableClosing(hWndDialogBase) ::SendMessage(hWndDialogBase,WM_DIALOG_BASE_ENABLE_CLOSING,1,0)

int MessageBoxModelessProtected(
    HWND    hWnd,
    LPCTSTR lpText,
    LPCTSTR lpCaption,
    UINT    uType
    );

BOOL GetOpenFileNameModelessProtected(LPOPENFILENAME pOfn);
BOOL GetSaveFileNameModelessProtected(LPOPENFILENAME pOfn);