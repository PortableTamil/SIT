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

#include <Windows.h>
#include <windowsx.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)
#include <malloc.h>
#include <shlwapi.h> // for autocomplete + PathIsNetworkPath
#pragma comment (lib,"shlwapi.lib")
#include "../tools/GUI/ListView/ListView.h"
#include "../tools/GUI/Rebar/Rebar.h"
#include "../Tools/String/WildCharCompare.h"
#include "../Tools/LinkList/SingleThreaded/LinkListTemplateSingleThreaded.cpp"
#include "../Tools/Exception/GenericTryCatchWithExceptionReport.h"

#ifdef USE_STD_CONSOLE
    // CStdConsole redirect CRT calls, but we add CRT reference --> use Win32
    #include "../Tools/Console/StdConsole.h"
    #include "../Tools/Console/StdConsole.cpp"
#else
    #include "../Tools/Console/Win32Console.h"
#endif

#include "SelectColumns.h"
#include "CommandLineParsing.h"
#include "SearchInfos.h"

#ifndef _countof
#define _countof(array) (sizeof(array)/sizeof(array[0]))
#endif

// defines for update checking
#define ShortcutsSearchAndReplace_PAD_Url _T("http://jacquelin.potier.free.fr/ShortcutsSearchAndReplace/ShortcutsSearchAndReplace.xml")
#define ShortcutsSearchAndReplaceVersionSignificantDigits 3
#ifdef _WIN64
#define ShortcutsSearchAndReplaceDownloadLink _T("http://jacquelin.potier.free.fr/exe/ShortcutsSearchAndReplace_bin64.zip")
#else
#define ShortcutsSearchAndReplaceDownloadLink _T("http://jacquelin.potier.free.fr/exe/ShortcutsSearchAndReplace_bin.zip")
#endif


#define MULTIPLE_DIRECTORIES_IN_FILE_PREFIX _T("File:")

#include "resource.h"
// Enable Visual Style
#if defined _AMD64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _WIN64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

#include "about.h"
#include "logs.h"
#include "Options.h"

#include "../Tools/GUI/Dialog/DialogHelper.h"
#include "../Tools/GUI/ListView/ListView.h"
#include "../Tools/GUI/ToolBar/Toolbar.h"
#include "../Tools/File/FileSearch.h"
#include "../Tools/File/StdFileOperations.h"
#include "../Tools/String/StringReplace.h"
#include "../Tools/String/MultipleElementsParsing.h"
#include "../Tools/String/TrimString.h"
#include "../Tools/String/SecureTcscat.h"
#include "../Tools/Thread/Thread.h"
#include "../Tools/Gui/BrowseForFolder/BrowseForFolder.h"
#include "../Tools/Gui/Statusbar/Statusbar.h"
#include "../Tools/Gui/Menu/PopUpMenu.h"
#include "../Tools/Gui/DragAndDrop/DragAndDrop.h"
#include "../Tools/LinkList/LinkListSimple.h"
#include "../Tools/SoftwareUpdate/SoftwareUpdate.h"

#ifndef _WIN64

#define DISABLE_WOW64_FS_REDIRECTION 0 // not working yet on seven for IShellLink
#if (DISABLE_WOW64_FS_REDIRECTION==1)
typedef BOOLEAN (WINAPI *pfWow64EnableWow64FsRedirection)(BOOLEAN Wow64FsEnableRedirection);
pfWow64EnableWow64FsRedirection G_fnWow64EnableWow64FsRedirection;
#endif
#endif


#define LINK_ARGS_MAX_SIZE INFOTIPSIZE
#define SPACE_BETWEEN_CONTROLS 2
#define MAIN_DIALOG_MIN_WIDTH 520
#define MAIN_DIALOG_MIN_HEIGHT 400
#define TIMEOUT_CANCEL_SEARCH_THREAD 5000 // max time in ms to wait for the cancel event to be taken into account
#define SEARCH_AND_REPLACE_LISTVIEW_NB_COLUMNS 7 // must match options.h
CListview::COLUMN_INFO pColumnInfo[SEARCH_AND_REPLACE_LISTVIEW_NB_COLUMNS]={
                                                {_T("Name"),150,LVCFMT_LEFT},
                                                {_T("Target Path"),150,LVCFMT_LEFT},
                                                {_T("Target Working Dir"),110,LVCFMT_LEFT},
                                                {_T("Target Args"),100,LVCFMT_LEFT},
                                                {_T("Target Icon"),100,LVCFMT_LEFT},
                                                {_T("Target Description"),100,LVCFMT_LEFT},
                                                {_T("Path"),100,LVCFMT_LEFT}
                                                };

TCHAR ShortcutsSpecialProtocols[][MAX_PATH]={
                                                _T("hcp://"),
                                                NULL
                                            };

enum tagListviewSearchResultColumnsIndex
{
    ListviewSearchResultColumnsIndex_ShortcutName=0,
    ListviewSearchResultColumnsIndex_TargetPath,
    ListviewSearchResultColumnsIndex_TargetWorkingDir,
    ListviewSearchResultColumnsIndex_TargetArgs,
    ListviewSearchResultColumnsIndex_TargetIcon,
    ListviewSearchResultColumnsIndex_TargetDescription,
    ListviewSearchResultColumnsIndex_ShortcutPath
};

typedef enum tagCheckForReplaceMode
{
    CheckForReplaceMode_CHECK_SAFE,
    CheckForReplaceMode_CHECK_ON_ERROR,
};

#define HELP_FILE _T("ShortcutsSearchAndReplace.chm")

class CDefinedDosDeviceInfos
{
public:
    TCHAR* Device;
    TCHAR* MappedPath;
    CDefinedDosDeviceInfos(TCHAR* PathToMap,TCHAR* Device)// same order as ::DefineDosDevice
    {
        this->Device=_tcsdup(Device);
        this->MappedPath=_tcsdup(PathToMap);
    }
    ~CDefinedDosDeviceInfos()
    {
        free(this->Device);
        free(this->MappedPath);
    }
};

typedef struct tagMultipleDirectoriesFileInfos
{
    BOOL bIncludeSubdir;
    HANDLE CancelEvent;
    SEARCH_INFOS* pSearchInfos;
    BOOL bSearchCanceled;
}MULTIPLE_DIRECTORIES_FILE_INFOS; 

class CShortcutInfos
{
public:
    typedef enum CHANGE_STATE{CHANGE_STATE_NoChange=0,CHANGE_STATE_ChangeSuccess,CHANGE_STATE_ChangeFailure};

    TCHAR LinkPath[SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH]; // path of the .lnk file
    TCHAR TargetPath[SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH]; // target full path
    TCHAR TargetWorkingDir[SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH];
    TCHAR TargetArgs[LINK_ARGS_MAX_SIZE];
    TCHAR TargetIconPath[SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH];
    TCHAR NewTargetPath[SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH];
    TCHAR NewTargetIconPath[SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH];
    TCHAR TargetDescription[SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH];

    BOOL NewTargetPathFileExists;

    int TargetIconIndex;

    BOOL TargetPathNeedsChanges;
    BOOL TargetWorkingDirNeedsChanges;
    BOOL TargetArgsNeedsChanges;
    BOOL TargetIconPathNeedsChanges;
    BOOL TargetDescriptionNeedsChanges;
	HICON hIcon;

    CHANGE_STATE ChangeState;

    CShortcutInfos()
    {
		this->hIcon = 0;
        *this->LinkPath=0;
        *this->TargetPath=0;
        *this->TargetWorkingDir=0;
        *this->TargetArgs=0;
        *this->TargetIconPath=0;
        *this->NewTargetPath=0;
        *this->TargetDescription=0;
        *this->NewTargetIconPath=0;

        this->TargetPathNeedsChanges=FALSE;
        this->TargetWorkingDirNeedsChanges=FALSE;
        this->TargetArgsNeedsChanges=FALSE;
        this->NewTargetPathFileExists=FALSE;
        this->TargetIconPathNeedsChanges=FALSE;
        this->TargetDescriptionNeedsChanges=FALSE;

        this->ChangeState = CHANGE_STATE_NoChange;

        this->TargetIconIndex=0;
    }
    ~CShortcutInfos()
    {
		if (this->hIcon)
		{
			::DestroyIcon(this->hIcon);
			this->hIcon = NULL;
		}
    };
};

class CListviewSearchAndReplace: public CListview
{
public:
    CListviewSearchAndReplace(HWND hWndListView,IListviewEvents* pListViewEventsHandler=NULL,IListViewItemsCompare* pListViewComparator=NULL)
        :CListview(hWndListView,pListViewEventsHandler,pListViewComparator)
    {

    }
protected:
    virtual COLORREF GetBackgroundColor(int ItemIndex,int SubitemIndex);
    COLORREF GetDefaultColor(int ItemIndex);
};

COLORREF CListviewSearchAndReplace::GetDefaultColor(int ItemIndex)
{
    if (ItemIndex%2==0)
        return RGB(255,255,255); // white color
    else
        // set a background color
        return this->DefaultCustomDrawColor;
}

COLORREF CListviewSearchAndReplace::GetBackgroundColor(int ItemIndex,int SubitemIndex)
{
    UNREFERENCED_PARAMETER(SubitemIndex);
    BOOL bSuccess;

    CShortcutInfos* pShortcutInfos=NULL;
    bSuccess = this->GetItemUserData(ItemIndex,(LPVOID*)&pShortcutInfos);
    if ( (!bSuccess) || (pShortcutInfos==NULL) )
        return this->GetDefaultColor(ItemIndex);

    switch (pShortcutInfos->ChangeState)
    {
    default:
    case CShortcutInfos::CHANGE_STATE_NoChange:
        return this->GetDefaultColor(ItemIndex);
    case CShortcutInfos::CHANGE_STATE_ChangeSuccess:
        return RGB(196,240,211);// green light
    case CShortcutInfos::CHANGE_STATE_ChangeFailure:
        return RGB(255,174,174);// red light
    }
}

INT_PTR CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void OnInit();
void OnClose();
void OnSizing(RECT* pWinRect);
void OnSize();

DWORD WINAPI GUI_Search(LPVOID lpParameter);
BOOL CallBackFileFound(TCHAR* Directory,WIN32_FIND_DATA* pWin32FindData,PVOID UserParam);

void OnAbout();
void OnHelp();
void OnDonation();
void OnCheckForUpdate();
void OnStart(tagSearchWay SearchWay);
void OnCancel();
DWORD WINAPI CancelSearch(LPVOID lpParameter);
void SetUserInterfaceMode(BOOL bIsSearching);
void FreeListViewAssociatedMemory();
BOOL AddShortcutToListView(CShortcutInfos* pShortcutInfos);
BOOL AddShortcutToList(CShortcutInfos* pShortcutInfos);
void FreeSearchInfos();
void ReplaceSelectedItems();
void ResolveSelectedItems();
void DeleteSelectedItems();
BOOL ChangeShortCutInfos(CShortcutInfos* pShortcutInfos);
BOOL ResolveShortCutInfos(CShortcutInfos* pShortcutInfos);
void LogEvent(TCHAR* const Msg);
void ShowLogs();
void FreeLogs();
BOOL IsSpecialPath(TCHAR* Path);
void OpenShortcutLocation();
void CopyShortcutDirectoryIntoSearchField();
void ShowProperties();
void ShowProperties(int Index);
void ShowProperties(TCHAR* FilePath);
void SwitchUserInterfaceAccordingToReplaceOnlyParams(BOOL bReplaceParamsOnly);
BOOL DoSearch();

HINSTANCE mhInstance;
HWND mhWndDialog=NULL;
HWND mhWndProgress=NULL;
CListview* pListView=NULL;
CRebar* pRebar=NULL;
CToolbar* pToolbarMain=NULL;
CToolbar* pToolbarFolders=NULL;
CToolbar* pToolbarInclude=NULL;
CToolbar* pToolbarHelp=NULL;
CToolbar* pToolbarListViewItemOperations=NULL;
CStatusbar* pStatusbar=NULL;
HANDLE mhEvtCancel=NULL;
int mListviewWarningIconIndex=0;
int mListviewEmptyIconIndex = 0;
SEARCH_INFOS mSearchInfos;
COutputString mLogs;
BOOL OperationPending=FALSE;
HCURSOR hWaitCursor=NULL;
UINT MenuIdProperties=(UINT)-1;
UINT MenuIdOpenShortcutLocation=(UINT)-1;
UINT MenuIdCopyTargetDirectoryIntoSearchField=(UINT)-1;
UINT MenuIdCheckAll = (UINT)-1;
UINT MenuIdUncheckAll = (UINT)-1;
UINT MenuIdCheckSelected = (UINT)-1;
UINT MenuIdUncheckSelected = (UINT)-1;
UINT MenuIdInvertSelection = (UINT)-1;
BOOL bRawMode = TRUE;
COptions* pOptions = NULL;
HICON mhIconWrite = NULL;
HICON mhIconDelete = NULL;
HICON mhIconResove = NULL;
CPopUpMenu* pMenuExclude = NULL;
UINT MenuExcludeIndexExcludeNetworkDrives=0;
UINT MenuExcludeIndexExcludeUnpluggedDrives=0;
CDragAndDrop* pDragAndDrop = NULL;
CThread* pThreadSearch = NULL;
CThread* pThreadClose = NULL;
TCHAR WindowsDrive[MAX_PATH] = {0};
CLinkListSimple* pDefinedDosDrivesList = NULL;
BOOL mCommandLineMode = FALSE;
BOOL mCommandLineDeleteWithNoPrompt = FALSE;
CCommandLineOptions CommandLineOptions;

CLinkListTemplateSingleThreaded<CShortcutInfos> mCommandLineListOfLinks;

#ifndef USE_STD_CONSOLE
CWin32Console* pWin32Console = NULL;
#endif

// GetDriveTypeFromPath : because GetDriveType returns bad value if full path is provided
DWORD GetDriveTypeFromPath(TCHAR* FilePath)
{
    if ( (FilePath[0]=='\\') && (FilePath[1]=='\\') )
    {
         //could be a network drive or
         //   \\P2P\to150\ 
         //   \\?\UNC\P2P\to150\ 
         //   \\.\HarddiskVolume1\ 
         //   \\?\Volume{2b294683-62aa-11de-9520-806d6172696f}\ 
         //   also working: complete path with GUID and mount point
         //   \\?\Volume{cfe6df15-9df5-11de-aca4-806e6f6e6963}\Removable\SD16#1\ 
        return ::GetDriveType(FilePath);
    }
    else
    {
        TCHAR DriveLetter[4];
        _tcscpy(DriveLetter,_T(" :\\"));

        // DriveLetter[0]=FilePath[0];
        *DriveLetter=*FilePath;

        return ::GetDriveType(DriveLetter);
    }
}

BOOL IsNetWorkPath(TCHAR* FilePath)
{
    if (::PathIsNetworkPath(FilePath))
        return TRUE;
    // Msdn : However, PathIsNetworkPath cannot recognize a network drive mapped to a drive letter through
    //        the Microsoft MS-DOS SUBST command or the DefineDosDevice function.
    UINT DriveType = GetDriveTypeFromPath(FilePath);
    return (DriveType==DRIVE_REMOTE);
}

BOOL IsUnpluggedDrive(TCHAR* FilePath)
{
    UINT DriveType = GetDriveTypeFromPath(FilePath);
    return (DriveType==DRIVE_NO_ROOT_DIR);
}

BOOL IsFixedOrNetworkDrive(TCHAR* FilePath)
{
    //GetDriveType returns
    //DRIVE_UNKNOWN 0 The drive type cannot be determined.
    //DRIVE_NO_ROOT_DIR 1 The root path is invalid; for example, there is no volume mounted at the specified path.
    //DRIVE_REMOVABLE 2 The drive has removable media; for example, a floppy drive, thumb drive, or flash card reader.
    //DRIVE_FIXED 3 The drive has fixed media; for example, a hard drive or flash drive.
    //DRIVE_REMOTE 4 The drive is a remote (network) drive.
    //DRIVE_CDROM 5 The drive is a CD-ROM drive.
    //DRIVE_RAMDISK 6 The drive is a RAM disk.
    UINT DriveType = GetDriveTypeFromPath(FilePath);
    BOOL RetValue = (DriveType==DRIVE_FIXED) || (DriveType==DRIVE_REMOTE);
    return RetValue;
}

FORCEINLINE BOOL DoStrStr(tagSearchWay SearchedWay,TCHAR* String,TCHAR* SearchedPattern)
{
    if ( (SearchedWay == SearchWay_SEARCH) || (pOptions->ReplaceOnlyArguments) )
        return CWildCharCompare::WildCmp(SearchedPattern,String);
    else
        return (_tcsstr(String,SearchedPattern)>0);
}

//-----------------------------------------------------------------------------
// Name: AddToHistoryIfNeeded
// Object: add new string to fixed array of string, checking if string is already existing
// Parameters :
//     in  : TCHAR Array[COption_HistoryDepth][SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH] : array of string containing history
//           TCHAR NewString[SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH] : string to add to history
//     out :
//     return : 
//-----------------------------------------------------------------------------
void AddToHistoryIfNeeded(TCHAR Array[COption_HistoryDepth][SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH],TCHAR NewString[SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH])
{
    SIZE_T Cnt;
    for (Cnt = 0; Cnt<COption_HistoryDepth; Cnt++)
    {
        // if item already in history
        if ( _tcscmp(Array[Cnt],NewString) == 0)
        {
            // put item from position Cnt to position 0

            if (Cnt) // if Cnt == 0, data is the same as last one --> nothing to do
            {
                // put history item from index Cnt to index 0
                SIZE_T Cnt2;
                for (Cnt2 = Cnt; Cnt2 ; Cnt2--)
                {
                    _tcscpy(Array[Cnt2],Array[Cnt2-1]);
                }

                // next add item at pos 0
                _tcscpy(Array[0],NewString);
            }
            break;
        }
    }
    if (Cnt == COption_HistoryDepth) // if not found
    {
        ///////////
        // add item
        ///////////

        // first move items
        for (Cnt = COption_HistoryDepth-1; Cnt; Cnt--)
        {
            _tcscpy(Array[Cnt],Array[Cnt-1]);
        }

        // next add item at pos 0
        _tcscpy(Array[0],NewString);
    }

}

//-----------------------------------------------------------------------------
// Name: AddToHistoryIfNeeded
// Object: add new string to fixed array of string, checking if string is already existing
// Parameters :
//     in  : TCHAR Array[COption_HistoryDepth] : array of string containing history
//           TCHAR* NewString : string to add to history
//     out :
//     return : 
//-----------------------------------------------------------------------------
void AddToHistoryIfNeeded(TCHAR* Array[COption_HistoryDepth],TCHAR* NewString)
{
    SIZE_T Cnt;
    for (Cnt = 0; Cnt<COption_HistoryDepth; Cnt++)
    {
        // if item already in history
        if ( _tcscmp(Array[Cnt],NewString) == 0)
        {
            // put item from position Cnt to position 0

            if (Cnt) // if Cnt == 0, data is the same as last one --> nothing to do
            {
                // save item pointer
                TCHAR* CurrentItem = Array[Cnt];

                // put history item from index Cnt to index 0
                SIZE_T Cnt2;
                for (Cnt2 = Cnt; Cnt2 ; Cnt2--)
                {
                    Array[Cnt2]=Array[Cnt2-1];
                }

                // set item at pos 0
                Array[0]=CurrentItem;
            }
            break;
        }
    }
    if (Cnt == COption_HistoryDepth) // if not found
    {
        ///////////
        // add item
        ///////////

        // free last item
        free(Array[COption_HistoryDepth-1]);

        // next move items
        for (Cnt = COption_HistoryDepth-1; Cnt; Cnt--)
        {
            Array[Cnt]=Array[Cnt-1];
        }

        // next add item at pos 0
        Array[0]=_tcsdup(NewString);
    }

}

class CListViewEventsHandler:public CListviewEventsBase
{
    virtual void OnListViewPopUpMenuClick(CListview* pObject,UINT MenuID)
    {
        if (MenuID==MenuIdProperties)
            ShowProperties();
        else if (MenuID==MenuIdOpenShortcutLocation)
            OpenShortcutLocation();
        else if (MenuID==MenuIdCopyTargetDirectoryIntoSearchField)
            CopyShortcutDirectoryIntoSearchField();
        else if (MenuID==MenuIdCheckAll)
            pObject->SelectAll();
        else if (MenuID==MenuIdUncheckAll)
            pObject->UnselectAll();
        else if (MenuID==MenuIdCheckSelected)
            pObject->CheckSelected();
        else if (MenuID==MenuIdUncheckSelected)
            pObject->UncheckSelected();
        else if (MenuID==MenuIdInvertSelection)
            pObject->InvertSelection();
    }
};

CListViewEventsHandler ListViewEventsHandler;

void ConsoleWrite(TCHAR* Msg,BOOL bAddCarriageReturn=TRUE)
{
#ifdef USE_STD_CONSOLE
    // CStdConsole redirect CRT calls
    _tprintf(Msg);
    if (bAddCarriageReturn)
        _tprintf(TEXT("\r\n"));
#else
    if (pWin32Console)
    {
        pWin32Console->Write(Msg);
        if (bAddCarriageReturn)
            pWin32Console->Write(TEXT("\r\n"));
    }
#endif
}

TCHAR ConsoleGetChar()
{
    TCHAR Answer;
#ifdef USE_STD_CONSOLE
    // CStdConsole redirect CRT calls
    Answer = (TCHAR)getchar();
#else
    Answer = 0;
    if (pWin32Console)
        Answer = pWin32Console->GetChar();
#endif

    return Answer;
}

void ReportError(TCHAR* Msg)
{
    if (mCommandLineMode)
        ConsoleWrite(Msg);
    else
        MessageBox(mhWndDialog,Msg,_T("Error"),MB_OK|MB_ICONERROR);
}

void ReportInfo(TCHAR* Msg)
{
    if (mCommandLineMode)
        ConsoleWrite(Msg);
    else
        MessageBox(mhWndDialog,Msg,_T("Information"),MB_OK|MB_ICONINFORMATION);
}

void SetStatus(TCHAR* Msg)
{
    if (mCommandLineMode)
        ConsoleWrite(Msg);
    else
        pStatusbar->SetText(0,Msg);
}

void SetProgressMaxPosition(WORD MaxPosition)
{
    if (!mCommandLineMode)
        SendMessage(mhWndProgress,PBM_SETRANGE,0,MAKELPARAM(0,MaxPosition));
}

void SetProgressPosition(WORD Position)
{
    if (!mCommandLineMode)
        SendMessage(mhWndProgress,PBM_SETPOS,Position,0);
}

//-----------------------------------------------------------------------------
// Name: MultipleDirectoriesFileLinesParsingCallBack
// Object: called for each line in MultipleDirectoriesFile (CTextFile::ParseLines call back)
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
BOOL MultipleDirectoriesFileLinesParsingCallBack(TCHAR* FileName,TCHAR* Line,DWORD dwLineNumber,BOOL bLastLine,LPVOID UserParam)
{
    UNREFERENCED_PARAMETER(bLastLine);
    MULTIPLE_DIRECTORIES_FILE_INFOS* pInfos= (MULTIPLE_DIRECTORIES_FILE_INFOS*)UserParam;

    // if empty line
    if (*Line==0)
        // continue parsing
        return TRUE;

    // assume path exists
    if (!CStdFileOperations::DoesDirectoryExists(Line))
    {
        SIZE_T Size = 260+_tcslen(FileName) + 1;
        TCHAR* Msg = new TCHAR[Size];
        _sntprintf(Msg,Size,_T("Invalid directory %s in %s at line %u"),Line,FileName,dwLineNumber);
        ReportError(Msg);
        delete[] Msg;
        // continue parsing
        return TRUE;
    }

    // assume search path ends with '\'
    SIZE_T Size=_tcslen(Line);

    TCHAR* SecureLine = (TCHAR*)_alloca((Size+20)*sizeof(TCHAR));
    _tcscpy(SecureLine,Line);
    if (SecureLine[Size-1]!='\\')
        _tcscat(SecureLine,_T("\\"));
    // search link in directory
    _tcscat(SecureLine,_T("*.lnk"));

    // search in specified folder
    CFileSearch::Search(SecureLine,pInfos->bIncludeSubdir,pInfos->CancelEvent,CallBackFileFound,pInfos->pSearchInfos,&pInfos->bSearchCanceled,TRUE);
    // continue parsing
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: UpdateCombos
// Object: UpdateCombos for history of search, replace and path edits after changes occurs in history
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void UpdateCombos()
{
    SIZE_T Cnt;
    HWND hWndSearch = ::GetDlgItem(mhWndDialog,IDC_EDIT_SEARCH);
    HWND hWndReplace = ::GetDlgItem(mhWndDialog,IDC_EDIT_REPLACE);
    HWND hWndPath = ::GetDlgItem(mhWndDialog,IDC_EDIT_PATH);


    ComboBox_ResetContent(hWndSearch);
    ComboBox_ResetContent(hWndReplace);
    ComboBox_ResetContent(hWndPath);

    BOOL bAddToCombo;
    for (Cnt = 0; Cnt<COption_HistoryDepth; Cnt++)
    {
        
        /////////////////////////////////////////////////
        // search history
        /////////////////////////////////////////////////
        if (*pOptions->SearchHistory[Cnt])
            bAddToCombo = TRUE;
        else
        {
            // if not last element
            if (Cnt<(COption_HistoryDepth-1))
            {
                // if next item is not empty
                bAddToCombo = (*pOptions->SearchHistory[Cnt+1]!=0);
            }
            else
                // history contains only empty strings --> no need to add to combo
                bAddToCombo = FALSE;
        }
        if (bAddToCombo)
            ComboBox_AddString(hWndSearch,pOptions->SearchHistory[Cnt]);

        /////////////////////////////////////////////////
        // replace history
        /////////////////////////////////////////////////
        if (*pOptions->ReplaceHistory[Cnt])
            bAddToCombo = TRUE;
        else
        {
            // if not last element
            if (Cnt<(COption_HistoryDepth-1))
            {
                // if next item is not empty
                bAddToCombo = (*pOptions->ReplaceHistory[Cnt+1]!=0);
            }
            else
                // history contains only empty strings --> no need to add to combo
                bAddToCombo = FALSE;
        }
        if (bAddToCombo)
            ComboBox_AddString(hWndReplace,pOptions->ReplaceHistory[Cnt]);

        /////////////////////////////////////////////////
        // paths history
        /////////////////////////////////////////////////
        if (*pOptions->PathHistory[Cnt])
            bAddToCombo = TRUE;
        else
        {
            // if not last element
            if (Cnt<(COption_HistoryDepth-1))
            {
                // if next item is not empty
                bAddToCombo = (*pOptions->PathHistory[Cnt+1]!=0);
            }
            else
                // history contains only empty strings --> no need to add to combo
                bAddToCombo = FALSE;
        }
        if (bAddToCombo)
            ComboBox_AddString(hWndPath,pOptions->PathHistory[Cnt]);
    }
    ComboBox_SetCurSel(hWndSearch,0);
    ComboBox_SetCurSel(hWndReplace,0);
    ComboBox_SetCurSel(hWndPath,0);
    ::PostMessage(hWndSearch,CB_SETEDITSEL,0,0);
    ::PostMessage(hWndReplace,CB_SETEDITSEL,0,0);
    ::PostMessage(hWndPath,CB_SETEDITSEL,0,0);
}

//-----------------------------------------------------------------------------
// Name: SaveHistory
// Object: save history of search, replace and path edits
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void SaveHistory()
{
    TCHAR Tmp[SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH];
    HWND hWndSearch = ::GetDlgItem(mhWndDialog,IDC_EDIT_SEARCH);
    HWND hWndReplace = ::GetDlgItem(mhWndDialog,IDC_EDIT_REPLACE);
    HWND hWndPath = ::GetDlgItem(mhWndDialog,IDC_EDIT_PATH);

    ::GetWindowText(hWndSearch,Tmp,SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH);
    AddToHistoryIfNeeded(pOptions->SearchHistory,Tmp);

    ::GetWindowText(hWndReplace,Tmp,SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH);
    AddToHistoryIfNeeded(pOptions->ReplaceHistory,Tmp);

    SIZE_T PathLen = ::GetWindowTextLength(::GetDlgItem(mhWndDialog,IDC_EDIT_PATH))+1;
    TCHAR* TmpPath = new TCHAR[PathLen];
    *TmpPath=0;
    if (PathLen)
        ::GetWindowText(hWndPath,TmpPath,(int)PathLen);
    AddToHistoryIfNeeded(pOptions->PathHistory,TmpPath);
    delete[] TmpPath;

    UpdateCombos();
}



//-----------------------------------------------------------------------------
// Name: SetWaitCursor
// Object: set wait cursor or standard cursor
// Parameters :
//     in  : BOOL bWaitCursor : TRUE to set wait cursor, FALSE to restore default cursor
//     out :
//     return : 
//-----------------------------------------------------------------------------
void SetWaitCursor(BOOL bWaitCursor)
{
    OperationPending=bWaitCursor;
    // force cursor update
    POINT pt;
    GetCursorPos(&pt);
    SetCursorPos(pt.x,pt.y);
}

//-----------------------------------------------------------------------------
// Name: FreePreviousSearchResults
// Object: free memory containing results of previous search
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void FreePreviousSearchResults()
{
    if (mCommandLineMode)
        mCommandLineListOfLinks.RemoveAllItems();
    else
    {
        FreeListViewAssociatedMemory();
        pListView->Clear();
    }
}


//-----------------------------------------------------------------------------
// Name: FreeSearchInfos
// Object: free global memory containing search infos
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void FreeSearchInfos()
{
    if (mSearchInfos.pszReplaceString)
    {
        delete[] mSearchInfos.pszReplaceString;
        mSearchInfos.pszReplaceString=0;
    }
    if (mSearchInfos.pszSearchStringUpper)
    {
        delete[] mSearchInfos.pszSearchStringUpper;
        mSearchInfos.pszSearchStringUpper=0;
    }
    memset(&mSearchInfos,0,sizeof(SEARCH_INFOS));
}

//-----------------------------------------------------------------------------
// Name: WinMain
// Object: Entry point of app
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow
                   )
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(nCmdShow);
    UNREFERENCED_PARAMETER(lpCmdLine);

    CGenericTryCatchWithExceptionReport_TRY

    // enable Xp style (for msgbox error)
    InitCommonControls();

#ifndef _WIN64

	SYSTEM_INFO SystemInfo={0};
    ::GetNativeSystemInfo(&SystemInfo);
    switch(SystemInfo.wProcessorArchitecture)
    {
        case PROCESSOR_ARCHITECTURE_AMD64:    // 9    x64 (AMD or Intel)
			// as we need to send messages to explorer process, structs of messages sent MUST BE of the same size as the explorer process
			// ans as size of structs are different in 32 and 64 bits process we have to put limitation
			::MessageBox(NULL,
						_T("Version not fully compatible with 64 bit operating system !\r\n")
						_T("You should download the 64 bit version at http://jacquelin.potier.free.fr"),
						_T("Warning"),
						MB_ICONWARNING | MB_OK | MB_SYSTEMMODAL
						);
			// return -1;
            break;
	}


#if (DISABLE_WOW64_FS_REDIRECTION==1) // not working yet on seven for IShellLink
    // disable fs redirection for dialog thread
    SYSTEM_INFO SystemInfo={0};
    ::GetNativeSystemInfo(&SystemInfo);
    if (SystemInfo.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64)
    {
        G_fnWow64EnableWow64FsRedirection=(pfWow64EnableWow64FsRedirection)::GetProcAddress(::GetModuleHandle(_T("kernel32.dll")),"Wow64EnableWow64FsRedirection");
        if(G_fnWow64EnableWow64FsRedirection)
            G_fnWow64EnableWow64FsRedirection(FALSE);
    }
#endif
#endif
    mhInstance=hInstance;

    // get system drive for optional drive mappings
    TCHAR SystemDir[MAX_PATH];
    ::GetSystemDirectory(SystemDir,_countof(SystemDir));
    CStdFileOperations::GetFileDrive(SystemDir,WindowsDrive);
    // create list for created drive
    pDefinedDosDrivesList = new CLinkListSimple();

    // initialize mSearchInfos and pOptions
    memset(&mSearchInfos,0,sizeof(SEARCH_INFOS));
    pOptions = new COptions();

#ifdef _DEBUG
//    ::MessageBox(NULL,TEXT("Console Mode Ready For Debug !"),TEXT("SSAR"),MB_ICONINFORMATION);
#endif

    if (!ParseCommandLine(&mCommandLineMode,&CommandLineOptions,&mSearchInfos,pOptions))
        goto CleanUp;

    if (mCommandLineMode)
    {
#ifdef USE_STD_CONSOLE
        CStdConsole StdConsole;
        StdConsole.AttachStdToConsole();
#else
        pWin32Console = new CWin32Console(TRUE);
#endif

        // initialize COM for current thread required to use IShellLink (thread is created for search)
        CoInitialize(0);

        // do the search base on &mSearchInfos,pOptions grabbed from ParseCommandLine
        DoSearch();

        switch (mSearchInfos.SearchWay)
        {
        case SearchWay_SEARCH:
        case SearchWay_SEARCH_DEAD_LINKS:
            switch(CommandLineOptions.PostOperation)
            {
            case PostOperation_RESOLVE:
                ResolveSelectedItems();
                break;
            case PostOperation_DELETE:
                DeleteSelectedItems();
                break;
            }
            break;
        case SearchWay_SEARCH_AND_REPLACE:
            ReplaceSelectedItems();
            break;
        }

        CoUninitialize();
#ifdef USE_STD_CONSOLE
        StdConsole.DetachStdFromConsole();
#else
        delete pWin32Console;
#endif
    }
    else
    {
        pThreadClose = new CThread();
        pThreadSearch = new CThread();
        
        mhEvtCancel=CreateEvent(NULL,TRUE,FALSE,NULL);
        // load wait cursor
        hWaitCursor=LoadCursor(NULL,IDC_WAIT);

        // load options
        pOptions->Load();

        ///////////////////////////////////////////////////////////////////////////
        // show main window
        //////////////////////////////////////////////////////////////////////////
        DialogBox(hInstance, (LPCTSTR)IDD_DIALOG_SHORTCUT_SEARCH_AND_REPLACE, NULL, DlgProc);

        pOptions->Save();

        if (mhEvtCancel)
            CloseHandle(mhEvtCancel);

        if (!pThreadSearch->WaitForSuccessFullEnd(0,5000))
            pThreadSearch->Stop();

        if (!pThreadClose->WaitForSuccessFullEnd(0,5000))
            pThreadClose->Stop();

        delete pThreadSearch;
        delete pThreadClose;
    }

CleanUp:
    FreeSearchInfos();
    FreeLogs();

    delete pDefinedDosDrivesList;

    delete pOptions;

    CGenericTryCatchWithExceptionReport_CATCH
}

//-----------------------------------------------------------------------------
// Name: BrowseForDirectory
// Object: show browse for folder dialog
// Parameters :
//     in  : HWND hResultWindow : window handle which text is going to be field by BrowseForDirectory
//     out :
//     return : 
//-----------------------------------------------------------------------------
void BrowseForDirectory(HWND hResultWindow)
{

    CBrowseForFolder BrowseForFolder;
    TCHAR CurrentPath[SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH];
    GetWindowText(hResultWindow,CurrentPath,SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH);
    if (*CurrentPath==0)
        ::GetCurrentDirectory(SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH,CurrentPath);
    if (BrowseForFolder.BrowseForFolder(mhWndDialog,CurrentPath,
                                        _T("Select Search Path"),
                                        BIF_EDITBOX | BIF_NEWDIALOGSTYLE
                                        )
        )
        SetWindowText(hResultWindow,BrowseForFolder.szSelectedPath);
}

class CToolbarEventsManager:public CToolbarEventsBase
{
public:
    virtual void OnToolbarDropDownMenu(CToolbar* pSender,CPopUpMenu* PopUpMenu,UINT MenuId)
    {
        UNREFERENCED_PARAMETER(pSender);
        if (PopUpMenu == pMenuExclude)
        {
            if (MenuId == MenuExcludeIndexExcludeNetworkDrives)
            {
                BOOL NewState = !pMenuExclude->IsChecked(MenuId);
                pMenuExclude->SetCheckedState(MenuId,NewState);
                pOptions->ExcludeNetworkPaths = NewState;
            }
            else if (MenuId == MenuExcludeIndexExcludeUnpluggedDrives)
            {
                BOOL NewState = !pMenuExclude->IsChecked(MenuId);
                pMenuExclude->SetCheckedState(MenuId,NewState);
                pOptions->ExcludeUnpluggedDrives = NewState;
            }
        }
    }
};
CToolbarEventsManager mToolbarEventsManager;


void FileDroppedCallBack(HWND hWndControl, TCHAR* DroppedFile, SIZE_T FileDroppedIndex, LPVOID UserParam)
{
    UNREFERENCED_PARAMETER(UserParam);
    UNREFERENCED_PARAMETER(FileDroppedIndex);
    
    HWND hWndComboPath = ::GetDlgItem(mhWndDialog,IDC_EDIT_PATH);
    HWND hwndEditPath = ::FindWindowEx(hWndComboPath, 0,0,0);
    if ( (hWndControl==hWndComboPath) || (hWndControl==hwndEditPath) )
    {
        if (CStdFileOperations::IsDirectory(DroppedFile))
            ::SetWindowText(hWndControl,DroppedFile);
        else
        {
            TCHAR Path[SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH];
            CStdFileOperations::GetFileDirectory(DroppedFile,Path,_countof(Path));
            ::SetWindowText(hWndControl,Path);
        }
    }
}

void EnableCustomFolder(BOOL bEnable)
{
    ::EnableWindow(::GetDlgItem(mhWndDialog,IDC_EDIT_PATH),bEnable);
    ::EnableWindow(::GetDlgItem(mhWndDialog,IDC_BUTTON_BROWSE_FOR_DIRECTORY),bEnable);
}

//-----------------------------------------------------------------------------
// Name: OnInit
// Object: Initialize objects that requires the Dialog exists
//          and sets some dialog properties
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void OnInit()
{
    ShowWindow(GetDlgItem(mhWndDialog,IDC_STATIC_RESOLVE),FALSE);
    ShowWindow(GetDlgItem(mhWndDialog,IDC_BUTTON_RESOLVE),FALSE);
    ShowWindow(GetDlgItem(mhWndDialog,IDC_STATIC_DELETE),TRUE);
    ShowWindow(GetDlgItem(mhWndDialog,IDC_BUTTON_DELETE),TRUE);
    ShowWindow(GetDlgItem(mhWndDialog,IDC_STATIC_REPLACE),FALSE);
    ShowWindow(GetDlgItem(mhWndDialog,IDC_BUTTON_REPLACE),FALSE);

    // assume COM is initialized for window msg thread
    CoInitialize(NULL);

    CDialogHelper::SetIcon(mhWndDialog,IDI_ICON_APP);

    pStatusbar=new CStatusbar(mhInstance,mhWndDialog,0);
    
    pStatusbar->AddPart(350); // text part
    pStatusbar->AddPart(104); // progress bar part
    RECT Rect;
    pStatusbar->GetRect(1, &Rect);

    // Create the progress bar.
    mhWndProgress = CreateWindowEx(
                                    0,                       // no extended styles
                                    PROGRESS_CLASS,          // name of progress bar class
                                    (LPCTSTR) NULL,          // no text when first created
                                    WS_VISIBLE |             // show window
                                    WS_CHILD,                // creates a child window
                                    Rect.left+2,             // size and position
                                    Rect.top+(Rect.bottom-Rect.top)/8,
                                    Rect.right-Rect.left-4,
                                    (Rect.bottom-Rect.top)*6/8, 
                                    pStatusbar->GetControlHandle(), // handle to parent window
                                    (HMENU) 0,               // child window identifier
                                    mhInstance,               // handle to application instance
                                    NULL);                   // no window creation data


    pListView=new CListviewSearchAndReplace(GetDlgItem(mhWndDialog,IDC_LIST_SEARCH_RESULTS),&ListViewEventsHandler);

    pListView->InitListViewColumns(SEARCH_AND_REPLACE_LISTVIEW_NB_COLUMNS,pColumnInfo);
    pListView->SetStyle(TRUE,FALSE,FALSE,TRUE);

    pListView->SetSortingType(CListview::SortingTypeString);

    pRebar = new CRebar(mhInstance,mhWndDialog,0);

    pToolbarMain=new CToolbar(mhInstance,mhWndDialog,TRUE,TRUE,24,24,0);
    pToolbarMain->SetEventsHandler((IToolbarEvents*)(CToolbarEventsBase*)&mToolbarEventsManager);

    pToolbarMain->AddButton(IDC_BUTTON_SEARCH,IDI_ICON_SEARCH,IDI_ICON_SEARCH_DISABLED,IDI_ICON_SEARCH_HOT,_T("Simple Search"));
    pToolbarMain->AddButton(IDC_BUTTON_SEARCH_AND_REPLACE,IDI_ICON_REPLACE,IDI_ICON_REPLACE_DISABLED,IDI_ICON_REPLACE_HOT,_T("Search for Replace (provides replace preview before effective replace)"));

    pMenuExclude = new CPopUpMenu();
    MenuExcludeIndexExcludeNetworkDrives = pMenuExclude->Add(_T("Exclude network drives"));
    pMenuExclude->SetCheckedState(MenuExcludeIndexExcludeNetworkDrives,pOptions->ExcludeNetworkPaths);
    MenuExcludeIndexExcludeUnpluggedDrives = pMenuExclude->Add(_T("Exclude unplugged drives"));
    pMenuExclude->SetCheckedState(MenuExcludeIndexExcludeUnpluggedDrives,pOptions->ExcludeUnpluggedDrives);

    pToolbarMain->AddDropDownButton(IDC_BUTTON_SEARCH_DEAD_LINKS,IDI_ICON_SEARCH_DEAD,IDI_ICON_SEARCH_DEAD_DISABLED,IDI_ICON_SEARCH_DEAD_HOT,_T("Search Dead Links"),pMenuExclude,FALSE);
    pToolbarMain->AddSeparator();
    pToolbarMain->AddButton(IDC_BUTTON_CANCEL,IDI_ICON_CANCEL,IDI_ICON_CANCEL_DISABLED,IDI_ICON_CANCEL_HOT,_T("Cancel Current Operation"));
    pToolbarMain->AddSeparator();
    pRebar->AddToolBarBand(pToolbarMain->GetControlHandle(),NULL,FALSE,FALSE,TRUE);

    pToolbarFolders=new CToolbar(mhInstance,mhWndDialog,TRUE,TRUE,24,24,0);
    

    // quick directory access
    pToolbarFolders->AddCheckButton(IDC_BUTTON_CUSTOM_FOLDER,IDI_ICON_CUSTOM_FOLDER,IDI_ICON_CUSTOM_FOLDER_DISABLED,IDI_ICON_CUSTOM_FOLDER_HOT,_T("Include the Search Folder specified below for search and replace operations"));
    pToolbarFolders->AddCheckButton(IDC_BUTTON_CURRENT_USER_START_MENU,IDI_ICON_USER_STARTUP,IDI_ICON_USER_STARTUP_DISABLED,IDI_ICON_USER_STARTUP_HOT,_T("Include User Start Menu for search and replace operations"));
    pToolbarFolders->AddCheckButton(IDC_BUTTON_COMMON_START_MENU,IDI_ICON_COMMON_STARTUP,IDI_ICON_COMMON_STARTUP_DISABLED,IDI_ICON_COMMON_STARTUP_HOT,_T("Include Common Start Menu for search and replace operations"));
    pToolbarFolders->AddCheckButton(IDC_BUTTON_CURRENT_USER_DESKTOP,IDI_ICON_DESKTOP,IDI_ICON_DESKTOP_DISABLED,IDI_ICON_DESKTOP_HOT,_T("Include User Desktop for search and replace operations"));
    pToolbarFolders->AddCheckButton(IDC_BUTTON_COMMON_DESKTOP,IDI_ICON_COMMON_DESKTOP,IDI_ICON_COMMON_DESKTOP_DISABLED,IDI_ICON_COMMON_DESKTOP_HOT,_T("Include Common Desktop for search and replace operations"));
    pToolbarFolders->AddSeparator();
    pToolbarFolders->AddCheckButton(IDC_BUTTON_INCLUDE_SUBDIR,IDI_ICON_SUBDIR,IDI_ICON_SUBDIR_DISABLED,IDI_ICON_SUBDIR_HOT,_T("Include Subdirectories of selected folders for search and replace operations"));
    pToolbarFolders->AddSeparator();
    pRebar->AddToolBarBand(pToolbarFolders->GetControlHandle(),_T("Folders "),FALSE,FALSE,TRUE);

    pToolbarInclude=new CToolbar(mhInstance,mhWndDialog,TRUE,TRUE,24,24,0);
    pToolbarInclude->AddCheckButton(IDC_BUTTON_DIRECTORY_UPDATE,IDI_ICON_APP_DIRECTORY,IDI_ICON_APP_DIRECTORY_DISABLED,IDI_ICON_APP_DIRECTORY_HOT,_T("Include shortcut Target Directory into search and replace operations"));
    pToolbarInclude->AddCheckButton(IDC_BUTTON_FILENAME_UPDATE,IDI_ICON_APP_FILENAME,IDI_ICON_APP_FILENAME_DISABLED,IDI_ICON_APP_FILENAME_HOT,_T("Include shortcut Target File Name into search and replace operations"));
    pToolbarInclude->AddCheckButton(IDC_BUTTON_ICON_DIRECTORY_UPDATE,IDI_ICON_ICON_DIRECTORY,IDI_ICON_ICON_DIRECTORY_DISABLED,IDI_ICON_ICON_DIRECTORY_HOT,_T("Include shortcut Icon Directory into search and replace operations"));
    pToolbarInclude->AddCheckButton(IDC_BUTTON_ICON_FILENAME_UPDATE,IDI_ICON_ICON_FILENAME,IDI_ICON_ICON_FILENAME_DISABLED,IDI_ICON_ICON_FILENAME_HOT,_T("Include shortcut Icon File Name into search and replace operations"));
    pToolbarInclude->AddCheckButton(IDC_BUTTON_DESCRIPTION_UPDATE,IDI_ICON_TXT,IDI_ICON_TXT_DISABLED,IDI_ICON_TXT_HOT,_T("Include shortcut Comments into search and replace operations"));
    pToolbarInclude->AddCheckButton(IDC_BUTTON_ARGUMENTS_UPDATE,IDI_ICON_PARAMS,IDI_ICON_PARAMS_DISABLED,IDI_ICON_PARAMS_HOT,_T("Include shortcut Arguments into search and replace operations"));
    pToolbarInclude->AddCheckButton(IDC_BUTTON_REPLACE_ONLY_ARGUMENTS,IDI_ICON_REPLACE_ARGS_ONLY,IDI_ICON_REPLACE_ARGS_ONLY_DISABLED,IDI_ICON_REPLACE_ARGS_ONLY_HOT,_T("Replace only parameters for all shortcuts matching the provided link target path.\r\n(Discard other replace operations)"));
    pToolbarInclude->AddSeparator();
    pToolbarInclude->AddCheckButton(IDC_BUTTON_DIRECTORY_FILENAME_MIXED_MODE,IDI_ICON_DIRECTORY_FILENAME_MIXED_MODE,IDI_ICON_DIRECTORY_FILENAME_MIXED_MODE_DISABLED,IDI_ICON_DIRECTORY_FILENAME_MIXED_MODE_HOT,_T("When directory and file name are affected, search and replace operations are applied on directory and file name joined\r\n(full path search instead of separated directory search and file name search)"));
    pToolbarInclude->AddSeparator();
    pToolbarInclude->AddCheckButton(IDC_BUTTON_APPLY_CHANGES_TO_READONLY,IDI_ICON_APPLY_CHANGES_TO_READONLY,IDI_ICON_APPLY_CHANGES_TO_READONLY_DISABLED,IDI_ICON_APPLY_CHANGES_TO_READONLY_HOT,_T("Allow to modify/delete shortcuts with the read only attribute"));
    
    pRebar->AddToolBarBand(pToolbarInclude->GetControlHandle(),_T("Applies To "),FALSE,FALSE,TRUE);

    pToolbarHelp=new CToolbar(mhInstance,mhWndDialog,TRUE,TRUE,24,24,0);
    // columns
    pToolbarHelp->AddSeparator();
    pToolbarHelp->AddButton(IDC_BUTTON_SELECT_COLUMNS,IDI_ICON_COLUMNS,_T("Select Columns"));

    // help/about
    pToolbarHelp->AddSeparator();
    pToolbarHelp->AddButton(IDC_BUTTON_HELP,IDI_ICON_HELP,IDI_ICON_HELP,IDI_ICON_HELP_HOT,_T("Help"));
    pToolbarHelp->AddButton(IDC_BUTTON_ABOUT,IDI_ICON_ABOUT,IDI_ICON_ABOUT,IDI_ICON_ABOUT_HOT,_T("About"));
    pToolbarHelp->AddButton(IDC_BUTTON_CHECK_FOR_UPDATE,IDI_ICON_CHECK_FOR_UPDATE,_T("Check For Update"));
    pToolbarHelp->AddSeparator();
    pToolbarHelp->AddButton(IDC_BUTTON_DONATION,IDI_ICON_DONATION,IDI_ICON_DONATION,IDI_ICON_DONATION_HOT,_T("Make Donation"));
    pRebar->AddToolBarBand(pToolbarHelp->GetControlHandle(),NULL,FALSE,FALSE,TRUE);


    pToolbarListViewItemOperations=new CToolbar(mhInstance,mhWndDialog,TRUE,TRUE,16,16,0);
    pToolbarListViewItemOperations->SetDirection(FALSE);
    // icon properties
    pToolbarListViewItemOperations->AddButton(IDC_BUTTON_PROPERTIES,IDI_ICON_PROPERTIES,_T("Display Shortcut Properties"));
    pToolbarListViewItemOperations->AddButton(IDC_BUTTON_OPEN_SHORTCUT_LOCATION,IDI_ICON_OPEN_SHORTCUT_LOCATION,_T("Open Shortcut Location"));

    // check uncheck items
    pToolbarListViewItemOperations->AddSeparator();
    pToolbarListViewItemOperations->AddButton(IDC_BUTTON_CHECK_ALL       ,IDI_ICON_CHECK_ALL            ,_T("Check All"));
    pToolbarListViewItemOperations->AddButton(IDC_BUTTON_UNCHECK_ALL     ,IDI_ICON_MENU_UNCHECK_ALL     ,_T("Uncheck All"));
    pToolbarListViewItemOperations->AddButton(IDC_BUTTON_CHECK_SELECTED  ,IDI_ICON_MENU_CHECK_SELECTED  ,_T("Check Selected"));
    pToolbarListViewItemOperations->AddButton(IDC_BUTTON_UNCHECK_SELECTED,IDI_ICON_MENU_UNCHECK_SELECTED,_T("Uncheck Selected"));
    pToolbarListViewItemOperations->AddButton(IDC_BUTTON_INVERT_SELECTION,IDI_ICON_MENU_INVERT_SELECTION,_T("Invert Checked"));
    pToolbarListViewItemOperations->AddButton(IDC_BUTTON_CHECK_SAFE_FOR_REPLACE,IDI_ICON_MENU_CHECK_SAFE_FOR_REPLACE,IDI_ICON_MENU_CHECK_SAFE_FOR_REPLACE_DISABLED,IDI_ICON_MENU_CHECK_SAFE_FOR_REPLACE,_T("Check Safe for Replace"));
    pToolbarListViewItemOperations->AddButton(IDC_BUTTON_CHECK_ON_ERROR_FOR_REPLACE,IDI_ICON_MENU_CHECK_ON_ERROR_FOR_REPLACE,IDI_ICON_MENU_CHECK_ON_ERROR_FOR_REPLACE_DISABLED,IDI_ICON_MENU_CHECK_ON_ERROR_FOR_REPLACE,_T("Check on Error for Replace"));

    CDialogHelper::GetClientWindowRectFromId(mhWndDialog,IDC_LIST_SEARCH_RESULTS,&Rect);
    pToolbarListViewItemOperations->SetPosition(0,Rect.top,28,pToolbarListViewItemOperations->GetButtonCount()*25);


    // update toolbar states according to options
    if (pOptions->SearchInsideTargetDescription)
        pToolbarInclude->SetButtonState(IDC_BUTTON_DESCRIPTION_UPDATE,TBSTATE_CHECKED);
    if (pOptions->SearchInsideArguments)
        pToolbarInclude->SetButtonState(IDC_BUTTON_ARGUMENTS_UPDATE,TBSTATE_CHECKED);
    if (pOptions->ReplaceOnlyArguments)
        pToolbarInclude->SetButtonState(IDC_BUTTON_REPLACE_ONLY_ARGUMENTS,TBSTATE_CHECKED);
    if (pOptions->SearchInsideDirectory)
        pToolbarInclude->SetButtonState(IDC_BUTTON_DIRECTORY_UPDATE,TBSTATE_CHECKED);
    if (pOptions->SearchInsideFileName)
        pToolbarInclude->SetButtonState(IDC_BUTTON_FILENAME_UPDATE,TBSTATE_CHECKED);
    if (pOptions->SearchInsideIconDirectory)
        pToolbarInclude->SetButtonState(IDC_BUTTON_ICON_DIRECTORY_UPDATE,TBSTATE_CHECKED);
    if (pOptions->SearchInsideIconFileName)
        pToolbarInclude->SetButtonState(IDC_BUTTON_ICON_FILENAME_UPDATE,TBSTATE_CHECKED);
    if(pOptions->SearchFullPathMixedMode)
        pToolbarInclude->SetButtonState(IDC_BUTTON_DIRECTORY_FILENAME_MIXED_MODE,TBSTATE_CHECKED);
    if (pOptions->ApplyChangesToReadOnlyShortcuts)
        pToolbarInclude->SetButtonState(IDC_BUTTON_APPLY_CHANGES_TO_READONLY,TBSTATE_CHECKED);

    if (pOptions->IncludeCustomFolder)
        pToolbarFolders->SetButtonState(IDC_BUTTON_CUSTOM_FOLDER,TBSTATE_CHECKED);
    else
        EnableCustomFolder(FALSE);

    if (pOptions->IncludeUserStartupDirectory)
        pToolbarFolders->SetButtonState(IDC_BUTTON_CURRENT_USER_START_MENU,TBSTATE_CHECKED);

    if (pOptions->IncludeCommonStartupDirectory)
        pToolbarFolders->SetButtonState(IDC_BUTTON_COMMON_START_MENU,TBSTATE_CHECKED);

    if (pOptions->IncludeUserDesktopDirectory)
        pToolbarFolders->SetButtonState(IDC_BUTTON_CURRENT_USER_DESKTOP,TBSTATE_CHECKED);

    if (pOptions->IncludeCommonDesktopDirectory)
        pToolbarFolders->SetButtonState(IDC_BUTTON_COMMON_DESKTOP,TBSTATE_CHECKED);

    if (pOptions->IncludeSubdirectories)
        pToolbarFolders->SetButtonState(IDC_BUTTON_INCLUDE_SUBDIR,TBSTATE_CHECKED);

    // add icon to buttons (no effect on Windows XP)
    mhIconDelete=(HICON)::LoadImage(mhInstance,MAKEINTRESOURCE(IDI_ICON_DELETE),IMAGE_ICON,16,16,LR_DEFAULTCOLOR|LR_SHARED);
    ::SendDlgItemMessage(mhWndDialog,IDC_BUTTON_DELETE,BM_SETIMAGE,IMAGE_ICON,(LPARAM)mhIconDelete);

    mhIconWrite=(HICON)::LoadImage(mhInstance,MAKEINTRESOURCE(IDI_ICON_WRITE),IMAGE_ICON,16,16,LR_DEFAULTCOLOR|LR_SHARED);
    ::SendDlgItemMessage(mhWndDialog,IDC_BUTTON_REPLACE,BM_SETIMAGE,IMAGE_ICON,(LPARAM)mhIconWrite);

    mhIconResove=(HICON)::LoadImage(mhInstance,MAKEINTRESOURCE(IDI_ICON_RESOLVE),IMAGE_ICON,16,16,LR_DEFAULTCOLOR|LR_SHARED);
    ::SendDlgItemMessage(mhWndDialog,IDC_BUTTON_RESOLVE,BM_SETIMAGE,IMAGE_ICON,(LPARAM)mhIconResove);

    // add icon to default listview popup menu
    pListView->SetDefaultMenuIcons(mhInstance,
                                   IDI_ICON_MENU_COPY,// don't mind : clear menu is not shown
                                   IDI_ICON_MENU_COPY,
                                   IDI_ICON_MENU_COPY_ALL,
                                   IDI_ICON_MENU_SAVE,
                                   IDI_ICON_MENU_SAVE_ALL,
                                   16,16
                                   );


    // add icons to listview menu
    UINT MenuIndex = 0;
    MenuIdProperties=pListView->pPopUpMenu->Add(_T("Properties"),IDI_ICON_PROPERTIES,mhInstance,16,16,MenuIndex++);
    MenuIdOpenShortcutLocation=pListView->pPopUpMenu->Add(_T("Open Shortcut Location"),IDI_ICON_OPEN_SHORTCUT_LOCATION,mhInstance,16,16,MenuIndex++);
    MenuIdCopyTargetDirectoryIntoSearchField=pListView->pPopUpMenu->Add(_T("Copy Target Directory Into Search Field"),IDI_ICON_SEARCH,mhInstance,16,16,MenuIndex++);
    pListView->pPopUpMenu->AddSeparator(MenuIndex++);

    MenuIdCheckAll=pListView->pPopUpMenu->Add(_T("Check All"),IDI_ICON_CHECK_ALL,mhInstance,16,16,MenuIndex++);
    MenuIdUncheckAll=pListView->pPopUpMenu->Add(_T("Uncheck All"),IDI_ICON_MENU_UNCHECK_ALL,mhInstance,16,16,MenuIndex++);
    MenuIdCheckSelected=pListView->pPopUpMenu->Add(_T("Check Selected"),IDI_ICON_MENU_CHECK_SELECTED,mhInstance,16,16,MenuIndex++);
    MenuIdUncheckSelected=pListView->pPopUpMenu->Add(_T("Uncheck Selected"),IDI_ICON_MENU_UNCHECK_SELECTED,mhInstance,16,16,MenuIndex++);
    MenuIdInvertSelection=pListView->pPopUpMenu->Add(_T("Invert Checked"),IDI_ICON_MENU_INVERT_SELECTION,mhInstance,16,16,MenuIndex++);
    pListView->pPopUpMenu->AddSeparator(MenuIndex++);

    // add autocomplete feature on path edit
    ::SHAutoComplete(GetDlgItem(mhWndDialog,IDC_EDIT_PATH),SHACF_FILESYSTEM | SHACF_URLALL | SHACF_USETAB);

    // add drag and drop
    pDragAndDrop = new CDragAndDrop(mhWndDialog,FileDroppedCallBack,NULL);
    pDragAndDrop->Start();

    // set gui mode in paused state
    SetUserInterfaceMode(FALSE);

    // update gui according to ReplaceOnlyArguments
    if (pOptions->ReplaceOnlyArguments)
        SwitchUserInterfaceAccordingToReplaceOnlyParams(TRUE);

    // set GUI from options
    if (pOptions->RemindPosition)
    {
        if ( (pOptions->Height != 0) && (pOptions->Width != 0 ) )
        {
            RECT Rect={0};
            Rect.left = pOptions->XPos;
            if (Rect.left<0)
                Rect.left=0;
            Rect.top = pOptions->YPos;
            if (Rect.top<0)
                Rect.top=0;

            RECT DesktopRect={0};
            ::SystemParametersInfo(SPI_GETWORKAREA,0,&DesktopRect,0);

            Rect.bottom = Rect.top + pOptions->Height;
            Rect.right = Rect.left + pOptions->Width;

            if ( (Rect.right-Rect.left) > (DesktopRect.right-DesktopRect.left) )
            {
                Rect.left = DesktopRect.left;
                Rect.right = DesktopRect.right;
            }
            if ( (Rect.bottom-Rect.top) > (DesktopRect.bottom-DesktopRect.top) )
            {
                Rect.top = DesktopRect.top;
                Rect.bottom = DesktopRect.bottom;
            }

            if (Rect.left<DesktopRect.left)
                Rect.left=DesktopRect.left;

            if (Rect.right>DesktopRect.right)
                Rect.right=DesktopRect.right;

            if (Rect.top<DesktopRect.top)
                Rect.top=DesktopRect.top;

            if (Rect.bottom>DesktopRect.bottom)
            {
                Rect.bottom=DesktopRect.bottom;
            }

            SetWindowPos(mhWndDialog,HWND_NOTOPMOST,
                        Rect.left,
                        Rect.top,
                        Rect.right-Rect.left,
                        Rect.bottom-Rect.top,
                        SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOREDRAW);
        }
    }

    // set columns order size and visibility
    pListView->EnableColumnReOrdering(TRUE);
    int ColumnsOrder[SEARCH_AND_REPLACE_LISTVIEW_NB_COLUMNS];
    BOOL ColumnOrderError = FALSE;
    for (DWORD Cnt=0;Cnt<SEARCH_AND_REPLACE_LISTVIEW_NB_COLUMNS;Cnt++)
    {
        if (pOptions->ListviewColumnsOrderInfos[Cnt].Visible)
        {
            if (pOptions->ListviewColumnsOrderInfos[Cnt].Size) // else let default size set by InitializeMonitoringListview
            {
                if (pOptions->ListviewColumnsOrderInfos[Cnt].Size<5)
                    pOptions->ListviewColumnsOrderInfos[Cnt].Size=5;
                pListView->SetColumnWidth(Cnt,(int)pOptions->ListviewColumnsOrderInfos[Cnt].Size);
            }
        }
        else
            pListView->HideColumn(Cnt);
        ColumnsOrder[Cnt] =(int) pOptions->ListviewColumnsOrderInfos[Cnt].Order;
        if ( ( ColumnsOrder[Cnt] >= (int)SEARCH_AND_REPLACE_LISTVIEW_NB_COLUMNS)
             ||( ColumnsOrder[Cnt] < (int)0)
           )
            ColumnOrderError = TRUE;
    }
    if (!ColumnOrderError)
        pListView->SetColumnOrderArray(SEARCH_AND_REPLACE_LISTVIEW_NB_COLUMNS,ColumnsOrder);

    // update histories
    UpdateCombos();

    // render layout
    OnSize();
}


void GetOptionsFromGui()
{
    // store options
    pOptions->SearchInsideTargetDescription = ((pToolbarInclude->GetButtonState(IDC_BUTTON_DESCRIPTION_UPDATE)&TBSTATE_CHECKED)!=0);
    pOptions->SearchInsideArguments = ((pToolbarInclude->GetButtonState(IDC_BUTTON_ARGUMENTS_UPDATE)&TBSTATE_CHECKED)!=0);
    pOptions->SearchInsideDirectory = ((pToolbarInclude->GetButtonState(IDC_BUTTON_DIRECTORY_UPDATE)&TBSTATE_CHECKED)!=0);
    pOptions->SearchInsideFileName = ((pToolbarInclude->GetButtonState(IDC_BUTTON_FILENAME_UPDATE)&TBSTATE_CHECKED)!=0);
    pOptions->SearchInsideIconDirectory = ((pToolbarInclude->GetButtonState(IDC_BUTTON_ICON_DIRECTORY_UPDATE)&TBSTATE_CHECKED)!=0);
    pOptions->SearchInsideIconFileName = ((pToolbarInclude->GetButtonState(IDC_BUTTON_ICON_FILENAME_UPDATE)&TBSTATE_CHECKED)!=0);
    pOptions->SearchFullPathMixedMode = ((pToolbarInclude->GetButtonState(IDC_BUTTON_DIRECTORY_FILENAME_MIXED_MODE)&TBSTATE_CHECKED)!=0);
    pOptions->ApplyChangesToReadOnlyShortcuts = ((pToolbarInclude->GetButtonState(IDC_BUTTON_APPLY_CHANGES_TO_READONLY)&TBSTATE_CHECKED)!=0);
    pOptions->ReplaceOnlyArguments = ((pToolbarInclude->GetButtonState(IDC_BUTTON_REPLACE_ONLY_ARGUMENTS)&TBSTATE_CHECKED)!=0);

    pOptions->IncludeCustomFolder = ((pToolbarFolders->GetButtonState(IDC_BUTTON_CUSTOM_FOLDER)&TBSTATE_CHECKED)!=0);
    pOptions->IncludeUserStartupDirectory = ((pToolbarFolders->GetButtonState(IDC_BUTTON_CURRENT_USER_START_MENU)&TBSTATE_CHECKED)!=0);
    pOptions->IncludeCommonStartupDirectory = ((pToolbarFolders->GetButtonState(IDC_BUTTON_COMMON_START_MENU)&TBSTATE_CHECKED)!=0);
    pOptions->IncludeUserDesktopDirectory = ((pToolbarFolders->GetButtonState(IDC_BUTTON_CURRENT_USER_DESKTOP)&TBSTATE_CHECKED)!=0);
    pOptions->IncludeCommonDesktopDirectory = ((pToolbarFolders->GetButtonState(IDC_BUTTON_COMMON_DESKTOP)&TBSTATE_CHECKED)!=0);
    pOptions->IncludeSubdirectories = ((pToolbarFolders->GetButtonState(IDC_BUTTON_INCLUDE_SUBDIR)&TBSTATE_CHECKED)!=0);
}

//-----------------------------------------------------------------------------
// Name: ThreadedClose
// Object: Close application
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
HANDLE hThreadStop = NULL;
DWORD WINAPI ThreadedClose(LPVOID lpParameter)
{
    UNREFERENCED_PARAMETER(lpParameter);
    // hide main window to avoid user interaction during closing
    ShowWindow(mhWndDialog,FALSE);

    // stop searching
    OnCancel();

    // save window position if needed
    if (pOptions->RemindPosition)
    {
        RECT Rect={0};
        ::GetWindowRect(mhWndDialog,&Rect);
        pOptions->XPos = Rect.left;
        pOptions->YPos = Rect.top;
        pOptions->Height = Rect.bottom - Rect.top;
        pOptions->Width = Rect.right - Rect.left;
    }

    // save history
    SaveHistory();

    GetOptionsFromGui();

    //////////////////
    // store columns order
    //////////////////
    int ColumnsOrder[SEARCH_AND_REPLACE_LISTVIEW_NB_COLUMNS];
#ifdef _DEBUG
    if(
#endif
    ListView_GetColumnOrderArray(pListView->GetControlHandle(),SEARCH_AND_REPLACE_LISTVIEW_NB_COLUMNS,ColumnsOrder)
#ifdef _DEBUG
    == FALSE)
    ::DebugBreak(); // bad number of columns, columns order saving will fail
#else
    ;
#endif

    for (DWORD Cnt=0;Cnt<SEARCH_AND_REPLACE_LISTVIEW_NB_COLUMNS;Cnt++)
    {
        if (pOptions->ListviewColumnsOrderInfos[Cnt].Visible)
            pOptions->ListviewColumnsOrderInfos[Cnt].Size = pListView->GetColumnWidth(Cnt);

        pOptions->ListviewColumnsOrderInfos[Cnt].Order = ColumnsOrder[Cnt];
    }

    ///////////////////
    // delete objects
    ///////////////////

    // delete pListView
    FreeListViewAssociatedMemory();
    delete pListView;
    pListView=NULL;

    // delete Toolbars / rebar
    delete pRebar;
    pRebar=NULL;
    delete pToolbarMain;
    pToolbarMain=NULL;
    delete pToolbarFolders;
    pToolbarFolders=NULL;
    delete pToolbarInclude;
    pToolbarInclude=NULL;
    delete pToolbarHelp;
    pToolbarHelp=NULL;
    delete pToolbarListViewItemOperations;
    pToolbarListViewItemOperations=NULL;

    // delete Statusbar
    delete pStatusbar;
    pStatusbar=NULL;

    delete pMenuExclude;
    pMenuExclude = NULL;

    pDragAndDrop->Stop();
    delete pDragAndDrop;
    pDragAndDrop = NULL;

    ::DestroyIcon(mhIconWrite);
    ::DestroyIcon(mhIconResove);
    ::DestroyIcon(mhIconDelete);

    ::DestroyWindow(mhWndProgress);

    // end dialog
    EndDialog(mhWndDialog,0);

    ::CloseHandle(hThreadStop);
    hThreadStop = NULL;
    return 0;
}


//-----------------------------------------------------------------------------
// Name: OnClose
// Object: OnClose 
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void OnClose()
{
    if (pThreadClose->IsAlive())
    {
        BreakInDebugMode;
        return;
    }
    // avoid dialog loop message pump deadlock
    pThreadClose->Start(ThreadedClose);
}

void SwitchUserInterfaceAccordingToReplaceOnlyParams(BOOL bReplaceParamsOnly)
{
    if (bReplaceParamsOnly)
    {
        ::SetDlgItemText(mhWndDialog,IDC_STATIC_SEARCH_FOR,_T("Link Target Path"));
        ::SetDlgItemText(mhWndDialog,IDC_STATIC_REPLACE_WITH,_T("New Args"));
    }
    else
    {
        ::SetDlgItemText(mhWndDialog,IDC_STATIC_SEARCH_FOR,_T("Search For"));
        ::SetDlgItemText(mhWndDialog,IDC_STATIC_REPLACE_WITH,_T("Replace With"));
    }

    pToolbarInclude->EnableButton(IDC_BUTTON_DESCRIPTION_UPDATE,!bReplaceParamsOnly);
    pToolbarInclude->EnableButton(IDC_BUTTON_ARGUMENTS_UPDATE,!bReplaceParamsOnly);
    pToolbarInclude->EnableButton(IDC_BUTTON_DIRECTORY_UPDATE,!bReplaceParamsOnly);
    pToolbarInclude->EnableButton(IDC_BUTTON_FILENAME_UPDATE,!bReplaceParamsOnly);
    pToolbarInclude->EnableButton(IDC_BUTTON_ICON_DIRECTORY_UPDATE,!bReplaceParamsOnly);
    pToolbarInclude->EnableButton(IDC_BUTTON_ICON_FILENAME_UPDATE,!bReplaceParamsOnly);
    pToolbarInclude->EnableButton(IDC_BUTTON_DIRECTORY_FILENAME_MIXED_MODE,!bReplaceParamsOnly);
}

void SpecificCheckForReplace(tagCheckForReplaceMode Mode)
{
    if (mSearchInfos.SearchWay != SearchWay_SEARCH_AND_REPLACE)
        return;

    LPVOID UserData;
    int NbItems = pListView->GetItemCount();
    for (int Cnt = 0; Cnt < NbItems; Cnt++)
    {
        pListView->GetItemUserData(Cnt,&UserData);
        if (UserData)
        {
            CShortcutInfos* pShortcutInfos = (CShortcutInfos*)UserData;
            switch (Mode)
            {
            case CheckForReplaceMode_CHECK_SAFE:
                pListView->SetSelectedState(Cnt,pShortcutInfos->NewTargetPathFileExists);
                break;
            case CheckForReplaceMode_CHECK_ON_ERROR:
                pListView->SetSelectedState(Cnt,!pShortcutInfos->NewTargetPathFileExists);
                break;
            }
        }
    }
}

//-----------------------------------------------------------------------------
// Name: WndProc
// Object: Main dialog callback
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
INT_PTR CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        mhWndDialog=hWnd;
        OnInit();
        break;
    case WM_CLOSE:
        OnClose();
        break;
    case WM_SIZING:
        OnSizing((RECT*)lParam);
        break;
    case WM_SIZE:
        if (pRebar)
            pRebar->OnSize(wParam,lParam);
        if (pStatusbar)
            pStatusbar->OnSize(wParam,lParam);
        OnSize();
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_BUTTON_SEARCH:
            OnStart(SearchWay_SEARCH);
            break;
        case IDC_BUTTON_SEARCH_AND_REPLACE:
            OnStart(SearchWay_SEARCH_AND_REPLACE);
            break;
        case IDC_BUTTON_SEARCH_DEAD_LINKS:
            OnStart(SearchWay_SEARCH_DEAD_LINKS);
            break;
        case IDC_BUTTON_CANCEL:
            CloseHandle(CreateThread(NULL,0,CancelSearch,0,0,NULL));
            break;
        case IDC_BUTTON_BROWSE_FOR_DIRECTORY:
            BrowseForDirectory(GetDlgItem(mhWndDialog,IDC_EDIT_PATH));
            break;
        case IDC_BUTTON_REPLACE:
            ReplaceSelectedItems();
            break;
        case IDC_BUTTON_RESOLVE:
            ResolveSelectedItems();
            break;
        case IDC_BUTTON_DELETE:
            DeleteSelectedItems();
            break;
        case IDC_BUTTON_PROPERTIES:
            ShowProperties();
            break;
        case IDC_BUTTON_OPEN_SHORTCUT_LOCATION:
            OpenShortcutLocation();
            break;
        case IDC_BUTTON_CHECK_ALL:
            pListView->SelectAll();
            break;
        case IDC_BUTTON_UNCHECK_ALL:
            pListView->UnselectAll();
            break;
        case IDC_BUTTON_CHECK_SELECTED:
            pListView->CheckSelected();
            break;
        case IDC_BUTTON_UNCHECK_SELECTED:
            pListView->UncheckSelected();
            break;
        case IDC_BUTTON_INVERT_SELECTION:
            pListView->InvertSelection();
            break;
        case IDC_BUTTON_CHECK_SAFE_FOR_REPLACE:
            SpecificCheckForReplace(CheckForReplaceMode_CHECK_SAFE);
            break;
        case IDC_BUTTON_CHECK_ON_ERROR_FOR_REPLACE:
            SpecificCheckForReplace(CheckForReplaceMode_CHECK_ON_ERROR);
            break;
        case IDC_BUTTON_ABOUT:
            OnAbout();
            break;
        case IDC_BUTTON_HELP:
            OnHelp();
            break;
        case IDC_BUTTON_DONATION:
            OnDonation();
            break;
        case IDC_BUTTON_CHECK_FOR_UPDATE:
            OnCheckForUpdate();
            break;
        case IDC_BUTTON_SELECT_COLUMNS:
            CSelectColumns::Show();
            break;
        case IDC_BUTTON_CUSTOM_FOLDER:
            {
                BOOL bCustomFolderMenuChecked = ((pToolbarFolders->GetButtonState(IDC_BUTTON_CUSTOM_FOLDER)&TBSTATE_CHECKED)!=0);
                EnableCustomFolder(bCustomFolderMenuChecked);
            }
            break;
        case IDC_BUTTON_REPLACE_ONLY_ARGUMENTS:
            {
                BOOL bArgsOnlyChecked = ((pToolbarInclude->GetButtonState(IDC_BUTTON_REPLACE_ONLY_ARGUMENTS)&TBSTATE_CHECKED)!=0);
                SwitchUserInterfaceAccordingToReplaceOnlyParams(bArgsOnlyChecked);
            }
            break;
        }

        // 1.6.15 change : force user to click search preview for replace again in case of field change
        switch(HIWORD(wParam))
        {
        case CBN_SELCHANGE:
        case CBN_EDITCHANGE:
            if (    ((HWND)lParam==GetDlgItem(mhWndDialog,IDC_EDIT_SEARCH))
                 || ((HWND)lParam==GetDlgItem(mhWndDialog,IDC_EDIT_REPLACE))
                 || ((HWND)lParam==GetDlgItem(mhWndDialog,IDC_EDIT_PATH))
                )
            {
                EnableWindow(GetDlgItem(mhWndDialog,IDC_BUTTON_REPLACE),FALSE);
            }   
            break;
        }
        break;
    case WM_NOTIFY:
        if (pListView)
        {
            if (pListView->OnNotify(wParam,lParam))
                break;
        }
        if (pRebar)
        {
            if (pRebar->OnNotify(wParam,lParam))
                break;
        }
        if (pToolbarMain)
        {
            if (pToolbarMain->OnNotify(wParam,lParam))
                break;
        }
        if (pToolbarFolders)
        {
            if (pToolbarFolders->OnNotify(wParam,lParam))
                break;
        }
        if (pToolbarInclude)
        {
            if (pToolbarInclude->OnNotify(wParam,lParam))
                break;
        }
        if (pToolbarHelp)
        {
            if (pToolbarHelp->OnNotify(wParam,lParam))
                break;
        }
        if (pToolbarListViewItemOperations)
        {
            if (pToolbarListViewItemOperations->OnNotify(wParam,lParam))
                break;
        }
        if (pStatusbar)
        {
            if (pStatusbar->OnNotify(wParam,lParam))
                break;
        }
        break;
    case WM_DROPFILES:
        if (pDragAndDrop)
            pDragAndDrop->OnWM_DROPFILES(wParam,lParam);
        break;

    case WM_SETCURSOR:
        if (pToolbarMain)
        {
            if ((HWND)wParam==pToolbarMain->GetControlHandle())
            {
                // no use
                // ::SetWindowLong(hWnd, DWL_MSGRESULT, FALSE);
                return FALSE;
            }
        }
        if (OperationPending)
        {
            ::SetCursor(hWaitCursor);
            //must use SetWindowLong to return value from dialog proc
            ::SetWindowLongPtr(hWnd, DWLP_MSGRESULT, TRUE);
            return TRUE;
        }
        //else
        {
            ::SetWindowLongPtr(hWnd, DWLP_MSGRESULT, FALSE);
            return FALSE;
        }
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: OnSizing
// Object: called on WM_SIZING. Assume main dialog has a min with and hight
// Parameters :
//     in  : 
//     out :
//     In Out : RECT* pWinRect : window rect
//     return : 
//-----------------------------------------------------------------------------
void OnSizing(RECT* pWinRect)
{
    // check min width and min height
    if ((pWinRect->right-pWinRect->left)<MAIN_DIALOG_MIN_WIDTH)
        pWinRect->right=pWinRect->left+MAIN_DIALOG_MIN_WIDTH;
    if ((pWinRect->bottom-pWinRect->top)<MAIN_DIALOG_MIN_HEIGHT)
        pWinRect->bottom=pWinRect->top+MAIN_DIALOG_MIN_HEIGHT;
}

//-----------------------------------------------------------------------------
// Name: OnSize
// Object: called on WM_SIZE. OnSize all components
// Parameters :
//     return : 
//-----------------------------------------------------------------------------
void OnSize()
{
    RECT Rect;
    LONG ActionButtonTop;
    RECT RectButtonResolve;
    RECT RectButtonDelete;
    RECT RectButtonBrowse;
    RECT RectButtonReplace;
    RECT RectListView;
    RECT RectDialog;
    RECT RectStatusBar;

    HWND hWnd;

    if (!pStatusbar)
        return;
    if (!pListView)
        return;

    // get dialog rect
    ::GetClientRect(mhWndDialog,&RectDialog);
    CDialogHelper::GetClientWindowRect(mhWndDialog,pStatusbar->GetControlHandle(),&RectStatusBar);


    switch (mSearchInfos.SearchWay)
    {
    case SearchWay_SEARCH:
    default:
        // button delete
        hWnd=GetDlgItem(mhWndDialog,IDC_BUTTON_DELETE);
        CDialogHelper::GetClientWindowRect(mhWndDialog,hWnd,&RectButtonDelete);
        SetWindowPos(hWnd,HWND_NOTOPMOST,
            RectDialog.right-(RectButtonDelete.right- RectButtonDelete.left ) - SPACE_BETWEEN_CONTROLS,
            RectStatusBar.top-(RectButtonDelete.bottom-RectButtonDelete.top) - SPACE_BETWEEN_CONTROLS,
            0,0,
            SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOREDRAW|SWP_NOSIZE);
        // update RectButtonReplace values
        CDialogHelper::GetClientWindowRect(mhWndDialog,hWnd,&RectButtonDelete);

        // static delete
        hWnd=GetDlgItem(mhWndDialog,IDC_STATIC_DELETE);
        CDialogHelper::GetClientWindowRect(mhWndDialog,hWnd,&Rect);
        SetWindowPos(hWnd,HWND_NOTOPMOST,
            RectButtonDelete.left - SPACE_BETWEEN_CONTROLS - (Rect.right-Rect.left),
            RectButtonDelete.top,
            0,0,
            SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOREDRAW|SWP_NOSIZE);  

        // ActionButtonTop=RectStatusBar.top;
        ActionButtonTop = RectButtonDelete.top;
        break;
    case SearchWay_SEARCH_AND_REPLACE:
        // button delete
        hWnd=GetDlgItem(mhWndDialog,IDC_BUTTON_DELETE);
        CDialogHelper::GetClientWindowRect(mhWndDialog,hWnd,&RectButtonDelete);
        SetWindowPos(hWnd,HWND_NOTOPMOST,
            RectDialog.right-(RectButtonDelete.right- RectButtonDelete.left ) - SPACE_BETWEEN_CONTROLS,
            RectStatusBar.top-(RectButtonDelete.bottom-RectButtonDelete.top) - SPACE_BETWEEN_CONTROLS,
            0,0,
            SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOREDRAW|SWP_NOSIZE);
        // update RectButtonReplace values
        CDialogHelper::GetClientWindowRect(mhWndDialog,hWnd,&RectButtonDelete);

        // static delete
        hWnd=GetDlgItem(mhWndDialog,IDC_STATIC_DELETE);
        CDialogHelper::GetClientWindowRect(mhWndDialog,hWnd,&Rect);
        SetWindowPos(hWnd,HWND_NOTOPMOST,
            RectButtonDelete.left - SPACE_BETWEEN_CONTROLS - (Rect.right-Rect.left),
            RectButtonDelete.top,
            0,0,
            SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOREDRAW|SWP_NOSIZE);  


        // button Replace
        hWnd=GetDlgItem(mhWndDialog,IDC_BUTTON_REPLACE);
        CDialogHelper::GetClientWindowRect(mhWndDialog,hWnd,&RectButtonReplace);
        SetWindowPos(hWnd,HWND_NOTOPMOST,
            RectDialog.right-(RectButtonReplace.right- RectButtonReplace.left ) - SPACE_BETWEEN_CONTROLS,
            RectButtonDelete.top-(RectButtonReplace.bottom-RectButtonReplace.top) - SPACE_BETWEEN_CONTROLS,
            0,0,
            SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOREDRAW|SWP_NOSIZE);
        // update RectButtonReplace values
        CDialogHelper::GetClientWindowRect(mhWndDialog,hWnd,&RectButtonReplace);

        // static Replace
        hWnd=GetDlgItem(mhWndDialog,IDC_STATIC_REPLACE);
        CDialogHelper::GetClientWindowRect(mhWndDialog,hWnd,&Rect);
        SetWindowPos(hWnd,HWND_NOTOPMOST,
            RectButtonReplace.left - SPACE_BETWEEN_CONTROLS - (Rect.right-Rect.left),
            RectButtonReplace.top,
            0,0,
            SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOREDRAW|SWP_NOSIZE);  

        ActionButtonTop=RectButtonReplace.top;
        break;
    case SearchWay_SEARCH_DEAD_LINKS:
        // button delete
        hWnd=GetDlgItem(mhWndDialog,IDC_BUTTON_DELETE);
        CDialogHelper::GetClientWindowRect(mhWndDialog,hWnd,&RectButtonDelete);
        SetWindowPos(hWnd,HWND_NOTOPMOST,
            RectDialog.right-(RectButtonDelete.right- RectButtonDelete.left ) - SPACE_BETWEEN_CONTROLS,
            RectStatusBar.top-(RectButtonDelete.bottom-RectButtonDelete.top) - SPACE_BETWEEN_CONTROLS,
            0,0,
            SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOREDRAW|SWP_NOSIZE);
        // update RectButtonReplace values
        CDialogHelper::GetClientWindowRect(mhWndDialog,hWnd,&RectButtonDelete);

        // static delete
        hWnd=GetDlgItem(mhWndDialog,IDC_STATIC_DELETE);
        CDialogHelper::GetClientWindowRect(mhWndDialog,hWnd,&Rect);
        SetWindowPos(hWnd,HWND_NOTOPMOST,
            RectButtonDelete.left - SPACE_BETWEEN_CONTROLS - (Rect.right-Rect.left),
            RectButtonDelete.top,
            0,0,
            SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOREDRAW|SWP_NOSIZE);  

        // button Resolve
        hWnd=GetDlgItem(mhWndDialog,IDC_BUTTON_RESOLVE);
        CDialogHelper::GetClientWindowRect(mhWndDialog,hWnd,&RectButtonResolve);
        SetWindowPos(hWnd,HWND_NOTOPMOST,
            RectDialog.right-(RectButtonResolve.right- RectButtonResolve.left ) - SPACE_BETWEEN_CONTROLS,
            RectButtonDelete.top-(RectButtonResolve.bottom-RectButtonResolve.top) - SPACE_BETWEEN_CONTROLS,
            0,0,
            SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOREDRAW|SWP_NOSIZE);
        // update RectButtonReplace values
        CDialogHelper::GetClientWindowRect(mhWndDialog,hWnd,&RectButtonResolve);

        // static Resolve
        hWnd=GetDlgItem(mhWndDialog,IDC_STATIC_RESOLVE);
        CDialogHelper::GetClientWindowRect(mhWndDialog,hWnd,&Rect);
        SetWindowPos(hWnd,HWND_NOTOPMOST,
            RectButtonResolve.left - SPACE_BETWEEN_CONTROLS - (Rect.right-Rect.left),
            RectButtonResolve.top,
            0,0,
            SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOREDRAW|SWP_NOSIZE);  

        ActionButtonTop=RectButtonResolve.top;
        break;
    }


    // resize ListView
    CDialogHelper::GetClientWindowRect(mhWndDialog,pListView->GetControlHandle(),&RectListView);
    SetWindowPos(pListView->GetControlHandle(),HWND_NOTOPMOST,0,0,
        RectDialog.right-RectListView.left-2*SPACE_BETWEEN_CONTROLS,
        ActionButtonTop-SPACE_BETWEEN_CONTROLS-RectListView.top,
        SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOREDRAW|SWP_NOMOVE);    


    // edit Search
    hWnd=GetDlgItem(mhWndDialog,IDC_EDIT_SEARCH);
    CDialogHelper::GetClientWindowRect(mhWndDialog,hWnd,&Rect);
    SetWindowPos(hWnd,HWND_NOTOPMOST,0,0,
        RectDialog.right-Rect.left-2*SPACE_BETWEEN_CONTROLS,
        Rect.bottom-Rect.top,// keep same height
        SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOREDRAW|SWP_NOMOVE);   


    // edit Replace
    hWnd=GetDlgItem(mhWndDialog,IDC_EDIT_REPLACE);
    CDialogHelper::GetClientWindowRect(mhWndDialog,hWnd,&Rect);
    SetWindowPos(hWnd,HWND_NOTOPMOST,0,0,
        RectDialog.right-Rect.left-2*SPACE_BETWEEN_CONTROLS,
        Rect.bottom-Rect.top,// keep same height
        SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOREDRAW|SWP_NOMOVE);  

    // browse button
    hWnd=GetDlgItem(mhWndDialog,IDC_BUTTON_BROWSE_FOR_DIRECTORY);
    CDialogHelper::GetClientWindowRect(mhWndDialog,hWnd,&RectButtonBrowse);
    SetWindowPos(hWnd,HWND_NOTOPMOST,
        RectDialog.right-(RectButtonBrowse.right-RectButtonBrowse.left)-2*SPACE_BETWEEN_CONTROLS,
        RectButtonBrowse.top,// keep same y position
        0,0,
        SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOREDRAW|SWP_NOSIZE);  
    // update RectButtonBrowse infos
    CDialogHelper::GetClientWindowRect(mhWndDialog,hWnd,&RectButtonBrowse);

    // path
    hWnd=GetDlgItem(mhWndDialog,IDC_EDIT_PATH);
    CDialogHelper::GetClientWindowRect(mhWndDialog,hWnd,&Rect);
    SetWindowPos(hWnd,HWND_NOTOPMOST,0,0,
        RectButtonBrowse.left-Rect.left-2*SPACE_BETWEEN_CONTROLS,
        Rect.bottom-Rect.top,// keep same height
        SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOREDRAW|SWP_NOMOVE);   

    ::SendMessage(GetDlgItem(mhWndDialog,IDC_EDIT_PATH),CB_SETEDITSEL,0,0);
    ::SendMessage(GetDlgItem(mhWndDialog,IDC_EDIT_SEARCH),CB_SETEDITSEL,0,0);
    ::SendMessage(GetDlgItem(mhWndDialog,IDC_EDIT_REPLACE),CB_SETEDITSEL,0,0);

    pStatusbar->SetPartSize(0,RectStatusBar.right-RectStatusBar.left-120);
    pStatusbar->GetRect(1, &Rect);
    // Reposition the progress control correctly	
    SetWindowPos(mhWndProgress,HWND_TOPMOST,
                Rect.left+2, Rect.top+(Rect.bottom-Rect.top)/8, 0, 0,
                SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOREDRAW|SWP_NOSIZE); 

    CDialogHelper::Redraw(mhWndDialog);
}


//-----------------------------------------------------------------------------
// Name: CancelSearch
// Object: OnCancel heap entries walk (for thread cancellation)
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
DWORD WINAPI CancelSearch(LPVOID lpParameter)
{
    UNREFERENCED_PARAMETER(lpParameter);
    OnCancel();
    return 0;
}

//-----------------------------------------------------------------------------
// Name: OnCancel
// Object: OnCancel heap entries walk (blocking)
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void OnCancel()
{
    if (!pThreadSearch->IsAlive())
        return;

    SetStatus(_T("Canceling Search..."));

    // set cancel event
    SetEvent(mhEvtCancel);

    // wait for end of searching thread
    if (!pThreadSearch->WaitForSuccessFullEnd(0,TIMEOUT_CANCEL_SEARCH_THREAD))
    {
        // dirty closing
        pThreadSearch->Stop(); // if not ended after timeout, kill thread

        // reset some GUI stuff not reset by SetUserInterfaceMode
        SetWaitCursor(FALSE);
        SendMessage(mhWndProgress,PBM_SETMARQUEE,FALSE,0);
        DWORD dwProgressBarStyle = GetWindowLong(mhWndProgress,GWL_STYLE);
        SetWindowLong(mhWndProgress,GWL_STYLE,dwProgressBarStyle & (~PBS_MARQUEE) );
    }

    SetStatus(_T("Search Canceled"));
    SetUserInterfaceMode(FALSE);
}

//-----------------------------------------------------------------------------
// Name: OnStart
// Object: OnStart search
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void OnStart(tagSearchWay SearchWay)
{
    if (pThreadSearch->IsAlive()) // previous search is not ended
    {
        BreakInDebugMode; // user interface should avoid this
        OnCancel();// return; OnCancel is better way than direct return
    }

    // reset cancel event
    ResetEvent(mhEvtCancel);

    // store fields content
    SaveHistory();

    // put interface in run mode
    SetUserInterfaceMode(TRUE);

    // do search in a new thread
    pThreadSearch->Start(GUI_Search,(LPDWORD)SearchWay);
}

BOOL IsShortcutAlreadyManaged(TCHAR* LinkPath)
{
    if (!pListView)
        return FALSE;
    CShortcutInfos* pShortcutInfos;
    int NbItems=pListView->GetItemCount();

    for (int Cnt=0;Cnt<NbItems;Cnt++)
    {
        if (pListView->GetItemUserData(Cnt,(LPVOID*)&pShortcutInfos))
        {
            if (_tcsicmp(pShortcutInfos->LinkPath,LinkPath) == 0)
                return TRUE;
        }
    }
    // item was not found : return FALSE 
    return FALSE;
}

//-----------------------------------------------------------------------------
// Name: CallBackFileFound
// Object: called for each dll in directory and sub directories
// Parameters :
//     in  : TCHAR* Directory : directory of search result
//           WIN32_FIND_DATA* pWin32FindData : search result
//           PVOID UserParam : user param, here the search pattern for functions
//     out :
//     return : 
//-----------------------------------------------------------------------------
BOOL CallBackFileFound(TCHAR* Directory,WIN32_FIND_DATA* pWin32FindData,PVOID UserParam)
{
    SEARCH_INFOS* SearchCallbackParam=(SEARCH_INFOS*)UserParam;
    TCHAR FileFullPath[SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH];
    DWORD Flag;
    TCHAR DecodedPathToCheck[SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH];// path without %HOMEPATH%, %xxx%   used if bRawModeSet
    TCHAR* PathToCheck;
    BOOL bAddedToList=FALSE;
    CShortcutInfos* pShortcutInfos=NULL;

    if (CFileSearch::IsDirectory(pWin32FindData))// should not occurs as we search for "*.lnk"
    {
        // continue parsing
        return TRUE;
    }
    if ( (_tcslen(Directory) + _tcslen(pWin32FindData->cFileName)) >= SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH )
    {
        SIZE_T Size = 260+_tcslen(Directory) + _tcslen(pWin32FindData->cFileName) + 1;
        TCHAR* Msg = new TCHAR[Size];
        _sntprintf(Msg,Size,_T("Non standard windows directory found (more than %u characters).\r\nAction stopped\r\nBad directory : %s"),
                    SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH,
                    Directory
                  );
        ReportError(Msg);
        delete[] Msg;

        // stop parsing
        return FALSE;
    }
    // make full path
    _tcscpy(FileFullPath,Directory);
    _tcscat(FileFullPath,pWin32FindData->cFileName);

    // in case of virtual folders, the same file can be reported multiple times
    if (IsShortcutAlreadyManaged(FileFullPath))
        return TRUE;

    mSearchInfos.NbLinkFilesSearched++;

    HRESULT hResult;
    IShellLink *pShellLink = NULL;
    IPersistFile *pPersistFile = NULL;
    WCHAR* pwLinkName;

    TCHAR LinkDataPathUpper[__max(LINK_ARGS_MAX_SIZE,SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH)];
    TCHAR DirectoryOnly[SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH];
    WIN32_FIND_DATA ShellLinkWin32FindData;

    // create IShellLink object
    hResult = CoCreateInstance(  CLSID_ShellLink,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                IID_IShellLink,
                                (LPVOID*)&pShellLink);
    if ( FAILED(hResult) || (pShellLink==NULL) )
        goto Cleanup;

    // get IPersistFile interface
    hResult = pShellLink->QueryInterface(IID_IPersistFile,(LPVOID*)&pPersistFile);
    if ( FAILED(hResult) || (pPersistFile==NULL) )
        goto Cleanup;

#if (defined(UNICODE)||defined(_UNICODE))
    pwLinkName=FileFullPath;
#else
    CAnsiUnicodeConvert::AnsiToUnicode(FileFullPath,&pwLinkName);
#endif

    // load shortcut infos
    hResult = pPersistFile->Load(pwLinkName,STGM_READ);

#if ( (!defined(UNICODE)) && (!defined(_UNICODE)))
    free(pwLinkName);
#endif

    if ( FAILED(hResult) )
        goto Cleanup;


    // create CListViewItemUserDataShortcutInfos object to store shortcut infos
    pShortcutInfos=new CShortcutInfos();

    // store link path in pShortcutInfos
    _tcscpy(pShortcutInfos->LinkPath,FileFullPath);

    if (bRawMode)
        Flag = SLGP_RAWPATH;
    else
        Flag = SLGP_UNCPRIORITY;

////////////////////////////////////////////////////////////////////////////////
    // Testing 
    //TCHAR sz0[SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH];
    //TCHAR sz1[SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH];
    //TCHAR sz2[SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH];
    //TCHAR sz3[SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH];
    //TCHAR sz4[SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH];
    //TCHAR sz5[SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH];
    //TCHAR sz6[SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH];
    //hResult = pShellLink->GetPath(sz0,SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH,&ShellLinkWin32FindData,SLGP_SHORTPATH);
    //hResult = pShellLink->GetPath(sz1,SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH,&ShellLinkWin32FindData,SLGP_UNCPRIORITY);
    //hResult = pShellLink->GetPath(sz2,SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH,&ShellLinkWin32FindData,SLGP_RAWPATH);
    //hResult = pShellLink->GetPath(sz3,SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH,&ShellLinkWin32FindData,SLGP_RELATIVEPRIORITY);
    //hResult = pShellLink->GetPath(sz4,SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH,&ShellLinkWin32FindData,SLGP_RAWPATH | SLGP_UNCPRIORITY);
    //hResult = pShellLink->GetPath(sz5,SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH,&ShellLinkWin32FindData,SLGP_RAWPATH |  SLGP_RELATIVEPRIORITY);
    //hResult = pShellLink->GetPath(sz6,SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH,&ShellLinkWin32FindData,SLGP_SHORTPATH | SLGP_RELATIVEPRIORITY);

    //BOOL bRet;
    //ITEMIDLIST* pIdList;
    //TCHAR sz10[SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH];
    //TCHAR sz11[SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH];
    //TCHAR sz12[SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH];
    //hResult = pShellLink->GetIDList(&pIdList);
    ////GPFIDL_DEFAULT    = 0x0000,      // normal Win32 file name, servers and drive roots included
    ////GPFIDL_ALTNAME    = 0x0001,      // short file name
    ////GPFIDL_UNCPRINTER = 0x0002,      // include UNC printer names too (non file system item)
    //bRet=SHGetPathFromIDListEx(pIdList, sz10,SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH,GPFIDL_DEFAULT);// get the path name from the ID list
    //bRet=SHGetPathFromIDListEx(pIdList, sz11,SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH,GPFIDL_ALTNAME);// get the path name from the ID list
    //bRet=SHGetPathFromIDListEx(pIdList, sz12,SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH,GPFIDL_UNCPRINTER);// get the path name from the ID list

    //CoTaskMemFree(pIdList);
////////////////////////////////////////////////////////////////////////////////

    // get shortcut target path (always get target path to retrieve associated icon)
    hResult = pShellLink->GetPath(pShortcutInfos->TargetPath,SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH,&ShellLinkWin32FindData,Flag);

    if (mSearchInfos.SearchInsideFullPath || mSearchInfos.SearchInsideDirectory || mSearchInfos.SearchInsideFileName 
        || (mSearchInfos.SearchWay==SearchWay_SEARCH_DEAD_LINKS) // we need to assume pShortcutInfos->TargetPath is filled in this case
        )
    {
        // if search or search and replace action
        if ( SUCCEEDED(hResult) )
        {
            if ( (mSearchInfos.SearchWay==SearchWay_SEARCH) || (mSearchInfos.SearchWay==SearchWay_SEARCH_AND_REPLACE))
            {
                // put path in upper case to do an insensitive search
                _tcscpy(LinkDataPathUpper,pShortcutInfos->TargetPath);
                _tcsupr(LinkDataPathUpper);

                if (mSearchInfos.SearchInsideFullPath)
                {
                    if (DoStrStr(mSearchInfos.SearchWay,LinkDataPathUpper,SearchCallbackParam->pszSearchStringUpper))
                        pShortcutInfos->TargetPathNeedsChanges=TRUE;
                }
                else
                {
                    if (mSearchInfos.SearchInsideDirectory)
                    {
                        CStdFileOperations::GetFileDirectory(LinkDataPathUpper, DirectoryOnly,_countof(DirectoryOnly));
                        if (DoStrStr(mSearchInfos.SearchWay,DirectoryOnly,SearchCallbackParam->pszSearchStringUpper))
                            pShortcutInfos->TargetPathNeedsChanges=TRUE;
                    }
                    if (mSearchInfos.SearchInsideFileName)
                    {
                        if (DoStrStr(mSearchInfos.SearchWay,CStdFileOperations::GetFileName(LinkDataPathUpper),SearchCallbackParam->pszSearchStringUpper))
                            pShortcutInfos->TargetPathNeedsChanges=TRUE;
                    } 
                }
               

                // if target path contains searched string
                if (pShortcutInfos->TargetPathNeedsChanges)
                {
                    // if action is search and replace
                    if (mSearchInfos.SearchWay==SearchWay_SEARCH_AND_REPLACE)
                    {
                        if (pOptions->ReplaceOnlyArguments)
                        {
                            pShortcutInfos->TargetPathNeedsChanges = FALSE;
                            pShortcutInfos->TargetArgsNeedsChanges = TRUE;
                        }
                        else
                        {

                            // get max required buffer size for replacement
                            SIZE_T RequieredSize=CStringReplace::ComputeMaxRequieredSizeForReplace(pShortcutInfos->TargetPath,mSearchInfos.pszSearchStringUpper,mSearchInfos.pszReplaceString);

                            // allocate enough memory
                            TCHAR* NewPath=(TCHAR*)_alloca(RequieredSize*sizeof(TCHAR));

                            if (mSearchInfos.SearchInsideFullPath)
                            {
                                CStringReplace::Replace(pShortcutInfos->TargetPath,NewPath,mSearchInfos.pszSearchStringUpper,mSearchInfos.pszReplaceString,FALSE);
                            }
                            else
                            {
                                CStdFileOperations::GetFileDirectory(pShortcutInfos->TargetPath, DirectoryOnly,_countof(DirectoryOnly));
                                // do replace
                                if (mSearchInfos.SearchInsideDirectory)
                                    CStringReplace::Replace(DirectoryOnly,NewPath,mSearchInfos.pszSearchStringUpper,mSearchInfos.pszReplaceString,FALSE);
                                else
                                    _tcscpy(NewPath,DirectoryOnly);

                                TCHAR* NewFileName = &NewPath[_tcslen(NewPath)];
                                if (mSearchInfos.SearchInsideFileName)
                                {
                                    CStringReplace::Replace(CStdFileOperations::GetFileName(pShortcutInfos->TargetPath),NewFileName,mSearchInfos.pszSearchStringUpper,mSearchInfos.pszReplaceString,FALSE);
                                }
                                else
                                    _tcscpy(NewFileName,CStdFileOperations::GetFileName(pShortcutInfos->TargetPath));
                            }


                            // buffer overflow protection
                            if ( _tcslen(NewPath)>=SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH )
                            {
                                SIZE_T Size = 260+_tcslen(NewPath) + 1;
                                TCHAR* Msg = new TCHAR[Size];
                                _sntprintf(Msg,Size,_T("Can't modify link of %s : Target path will exceed max allowed path size\r\n"),NewPath);
                                LogEvent(Msg);
                                delete[] Msg;
                                goto Cleanup;
                            }
                            // store new target path
                            _tcscpy(pShortcutInfos->NewTargetPath,NewPath);
                        }
                    }
                    else
                    {
                        _tcscpy(pShortcutInfos->NewTargetPath,pShortcutInfos->TargetPath);
                    }
                }
                else
                {
                    // target path doesn't change
                    _tcscpy(pShortcutInfos->NewTargetPath,pShortcutInfos->TargetPath);
                }
            }
        }
    }

    if ( (mSearchInfos.SearchInsideDirectory || mSearchInfos.SearchInsideFullPath) && (!pOptions->ReplaceOnlyArguments) )
    {
        // search replacement for working directory 
        hResult = pShellLink->GetWorkingDirectory( pShortcutInfos->TargetWorkingDir,SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH);
        if ( SUCCEEDED(hResult) )
        {
            if ( (mSearchInfos.SearchWay==SearchWay_SEARCH) || (mSearchInfos.SearchWay==SearchWay_SEARCH_AND_REPLACE))
            {
                // put working dir in upper case to do an insensitive search
                _tcscpy(LinkDataPathUpper,pShortcutInfos->TargetWorkingDir);
                _tcsupr(LinkDataPathUpper);

                // if target path contains searched string
                if (DoStrStr(mSearchInfos.SearchWay,LinkDataPathUpper,SearchCallbackParam->pszSearchStringUpper))
                {
                    pShortcutInfos->TargetWorkingDirNeedsChanges=TRUE;
                }
            }
        }
    }

    // search replacement for icon location (always get icon location to retrieve associated icon)
    hResult = pShellLink->GetIconLocation(pShortcutInfos->TargetIconPath,SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH,&pShortcutInfos->TargetIconIndex);

    if (mSearchInfos.SearchInsideIconDirectory || mSearchInfos.SearchInsideIconFileName || mSearchInfos.SearchInsideIconFullPath)
    {
        if ( SUCCEEDED(hResult) )
        {
            if ( (mSearchInfos.SearchWay==SearchWay_SEARCH) || (mSearchInfos.SearchWay==SearchWay_SEARCH_AND_REPLACE))
            {
                // put working dir in upper case to do an insensitive search
                _tcscpy(LinkDataPathUpper,pShortcutInfos->TargetIconPath);
                _tcsupr(LinkDataPathUpper);

                if (mSearchInfos.SearchInsideIconFullPath)
                {
                    if (DoStrStr(mSearchInfos.SearchWay,LinkDataPathUpper,SearchCallbackParam->pszSearchStringUpper))
                        pShortcutInfos->TargetIconPathNeedsChanges=TRUE;
                }
                else
                {
                    // if target icon directory contains searched string
                    if (mSearchInfos.SearchInsideIconDirectory)
                    {
                        CStdFileOperations::GetFileDirectory(LinkDataPathUpper, DirectoryOnly,_countof(DirectoryOnly));
                        if (DoStrStr(mSearchInfos.SearchWay,DirectoryOnly,SearchCallbackParam->pszSearchStringUpper))
                            pShortcutInfos->TargetIconPathNeedsChanges=TRUE;
                    }
                    // if target icon filename contains searched string
                    if (mSearchInfos.SearchInsideIconFileName)
                    {
                        if (DoStrStr(mSearchInfos.SearchWay,CStdFileOperations::GetFileName(LinkDataPathUpper),SearchCallbackParam->pszSearchStringUpper))
                            pShortcutInfos->TargetIconPathNeedsChanges=TRUE;
                    } 
                }

                // if target path contains searched string
                if (pShortcutInfos->TargetIconPathNeedsChanges)
                {
                    // if action is search and replace
                    if (mSearchInfos.SearchWay==SearchWay_SEARCH_AND_REPLACE)
                    {
                        // get max required buffer size for replacement
                        SIZE_T RequieredSize=CStringReplace::ComputeMaxRequieredSizeForReplace(pShortcutInfos->TargetIconPath,mSearchInfos.pszSearchStringUpper,mSearchInfos.pszReplaceString);

                        // allocate enough memory
                        TCHAR* NewPath=(TCHAR*)_alloca(RequieredSize*sizeof(TCHAR));

                        if (mSearchInfos.SearchInsideIconFullPath)
                        {
                            CStringReplace::Replace(pShortcutInfos->TargetIconPath,NewPath,mSearchInfos.pszSearchStringUpper,mSearchInfos.pszReplaceString,FALSE);
                        }
                        else
                        {
                            CStdFileOperations::GetFileDirectory(pShortcutInfos->TargetIconPath, DirectoryOnly,_countof(DirectoryOnly));

                            // do replace
                            if (mSearchInfos.SearchInsideDirectory)
                                CStringReplace::Replace(DirectoryOnly,NewPath,mSearchInfos.pszSearchStringUpper,mSearchInfos.pszReplaceString,FALSE);
                            else
                                _tcscpy(NewPath,DirectoryOnly);

                            TCHAR* NewFileName = &NewPath[_tcslen(NewPath)];
                            if (mSearchInfos.SearchInsideFileName)
                                CStringReplace::Replace(CStdFileOperations::GetFileName(pShortcutInfos->TargetIconPath),NewFileName,mSearchInfos.pszSearchStringUpper,mSearchInfos.pszReplaceString,FALSE);
                            else
                                _tcscpy(NewFileName,CStdFileOperations::GetFileName(pShortcutInfos->TargetIconPath));
                        }


                        // buffer overflow protection
                        if ( _tcslen(NewPath)>=SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH )
                        {
                            SIZE_T Size = 260+_tcslen(NewPath) + 1;
                            TCHAR* Msg = new TCHAR[Size];
                            _sntprintf(Msg,Size,_T("Can't modify link of %s : Icon path will exceed max allowed path size\r\n"),NewPath);
                            LogEvent(Msg);
                            delete[] Msg;
                            goto Cleanup;
                        }
                        // store new target path
                        _tcscpy(pShortcutInfos->NewTargetIconPath,NewPath);
                    }
                    else
                    {
                        _tcscpy(pShortcutInfos->NewTargetIconPath,pShortcutInfos->TargetPath);
                    }
                }
                else
                {
                    // target path doesn't change
                    _tcscpy(pShortcutInfos->NewTargetIconPath,pShortcutInfos->TargetPath);
                }

            }
        }
    }

    // search replacement for args
    if (mSearchInfos.SearchInsideArguments)
    {
        hResult = pShellLink->GetArguments( pShortcutInfos->TargetArgs,LINK_ARGS_MAX_SIZE);
        if ( SUCCEEDED(hResult) )
        {
            if ( (mSearchInfos.SearchWay==SearchWay_SEARCH) || (mSearchInfos.SearchWay==SearchWay_SEARCH_AND_REPLACE))
            {
                // put args in upper case to do an insensitive search
                _tcscpy(LinkDataPathUpper,pShortcutInfos->TargetArgs);
                _tcsupr(LinkDataPathUpper);
                // if args contains searched string
                if (DoStrStr(mSearchInfos.SearchWay,LinkDataPathUpper,SearchCallbackParam->pszSearchStringUpper))
                {
                    pShortcutInfos->TargetArgsNeedsChanges=TRUE;
                }
            }
        }
    }

    // comments
    if (mSearchInfos.SearchInsideTargetDescription)
    {
        hResult = pShellLink->GetDescription( pShortcutInfos->TargetDescription,SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH);
        if ( SUCCEEDED(hResult) )
        {
            if ( (mSearchInfos.SearchWay==SearchWay_SEARCH) || (mSearchInfos.SearchWay==SearchWay_SEARCH_AND_REPLACE))
            {
                // put TargetDescription in upper case to do an insensitive search
                _tcscpy(LinkDataPathUpper,pShortcutInfos->TargetDescription);
                _tcsupr(LinkDataPathUpper);

                // if TargetDescription contains searched string
                if (DoStrStr(mSearchInfos.SearchWay,LinkDataPathUpper,SearchCallbackParam->pszSearchStringUpper))
                {
                    pShortcutInfos->TargetDescriptionNeedsChanges=TRUE;
                }
            }
        }
    }

    // if action is to find dead links
    if (mSearchInfos.SearchWay==SearchWay_SEARCH_DEAD_LINKS)
    {
        BOOL CurrentTargetExists;

        if (bRawMode)
        {
            // convert target path to real path removing all %VarName%
            ::ExpandEnvironmentStrings(pShortcutInfos->TargetPath,DecodedPathToCheck,SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH-1);
            PathToCheck = DecodedPathToCheck;
        }
        else
        {
            PathToCheck = pShortcutInfos->TargetPath;
        }

        if (*PathToCheck)
        {
            if (pOptions->ExcludeNetworkPaths)
            {
                if (IsNetWorkPath(PathToCheck))
                    goto Cleanup;            
            }

            if (pOptions->ExcludeUnpluggedDrives)
            {
                if (!IsFixedOrNetworkDrive(PathToCheck))
                    goto Cleanup;
            }
        }

        // check if target exists
        BOOL bIsDirectory=CStdFileOperations::IsDirectory(PathToCheck /*pShortcutInfos->TargetPath*/);
        
        if (bIsDirectory)
            CurrentTargetExists=CStdFileOperations::DoesDirectoryExists(PathToCheck /*pShortcutInfos->TargetPath*/);
        else
            CurrentTargetExists=CStdFileOperations::DoesFileExists(PathToCheck /*pShortcutInfos->TargetPath*/);

        if (!CurrentTargetExists)
        { 
            if (!IsSpecialPath(PathToCheck /*pShortcutInfos->TargetPath*/))
            {
                mSearchInfos.NbLinkFilesSearchedMatchingConditions++;
                mSearchInfos.NbLinkFilesSearchedForReplaceWithPathValidated++;

                // add to listview
                bAddedToList=AddShortcutToList(pShortcutInfos);
            }
        }
    }
    else
    {
        // if searched string is found add item to list
        if (
            pShortcutInfos->TargetPathNeedsChanges
            || pShortcutInfos->TargetWorkingDirNeedsChanges
            || pShortcutInfos->TargetArgsNeedsChanges
            || pShortcutInfos->TargetIconPathNeedsChanges
            || pShortcutInfos->TargetDescriptionNeedsChanges
            )
        {
            if (pShortcutInfos->TargetPathNeedsChanges)
            {
                if (bRawMode)
                {
                    // convert target path to real path removing all %VarName%
                    ::ExpandEnvironmentStrings(pShortcutInfos->NewTargetPath,DecodedPathToCheck,SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH-1);
                    PathToCheck = DecodedPathToCheck;
                }
                else
                {
                    PathToCheck = pShortcutInfos->NewTargetPath;
                }
            }
            else
            {
                if (bRawMode)
                {
                    // convert target path to real path removing all %VarName%
                    ::ExpandEnvironmentStrings(pShortcutInfos->TargetPath,DecodedPathToCheck,SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH-1);
                    PathToCheck = DecodedPathToCheck;
                }
                else
                {
                    PathToCheck = pShortcutInfos->TargetPath;
                }
            }


            // check if target exists
            BOOL bIsDirectory=CStdFileOperations::IsDirectory(PathToCheck /* pShortcutInfos->NewTargetPath */);

            if (bIsDirectory)
                pShortcutInfos->NewTargetPathFileExists=CStdFileOperations::DoesDirectoryExists(PathToCheck /* pShortcutInfos->NewTargetPath */);
            else
                pShortcutInfos->NewTargetPathFileExists=CStdFileOperations::DoesFileExists(PathToCheck /* pShortcutInfos->NewTargetPath */);

            mSearchInfos.NbLinkFilesSearchedMatchingConditions++;

            if (pShortcutInfos->NewTargetPathFileExists)
                mSearchInfos.NbLinkFilesSearchedForReplaceWithPathValidated++;

            // add to listview
            bAddedToList=AddShortcutToList(pShortcutInfos);
        }
    }


Cleanup:
    if (pShellLink)
        pShellLink->Release();

    if (pPersistFile)
        pPersistFile->Release();
    

    // if pShortcutInfos is not used by listview
    if (!bAddedToList)
    {
        if (pShortcutInfos)
            delete pShortcutInfos;
    }

    // continue parsing
    return TRUE;
}

BOOL DoSearch()
{
    BOOL bSuccess = FALSE;
    BOOL bSearchCanceled=FALSE;

    // initialize COM for current thread required to use IShellLink (thread is created for search)
    CoInitialize(0);

    SIZE_T Size;
    TCHAR Path[SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH];

#ifndef _WIN64
    #if (DISABLE_WOW64_FS_REDIRECTION==1)
        // disable fs redirection for current thread
        if(G_fnWow64EnableWow64FsRedirection)
            G_fnWow64EnableWow64FsRedirection(FALSE);
    #endif
#endif

    mSearchInfos.bSearchedCanceled = FALSE;
    mSearchInfos.NbLinkFilesSearched=0;
    mSearchInfos.NbLinkFilesSearchedMatchingConditions=0;
    mSearchInfos.NbLinkFilesSearchedForReplaceWithPathValidated=0;

    if (pOptions->ReplaceOnlyArguments)
    {
        mSearchInfos.SearchInsideFullPath = TRUE;
        mSearchInfos.SearchInsideTargetDescription=FALSE;
        mSearchInfos.SearchInsideArguments=FALSE;
        mSearchInfos.SearchInsideDirectory=FALSE;
        mSearchInfos.SearchInsideFileName=FALSE;
        mSearchInfos.SearchInsideIconDirectory=FALSE;
        mSearchInfos.SearchInsideIconFileName=FALSE;
        mSearchInfos.SearchFullPathMixedMode = FALSE;
    }
    else
    {
        mSearchInfos.SearchInsideTargetDescription=pOptions->SearchInsideTargetDescription;
        mSearchInfos.SearchInsideArguments=pOptions->SearchInsideArguments;
        mSearchInfos.SearchInsideDirectory=pOptions->SearchInsideDirectory;
        mSearchInfos.SearchInsideFileName=pOptions->SearchInsideFileName;
        mSearchInfos.SearchInsideIconDirectory=pOptions->SearchInsideIconDirectory;
        mSearchInfos.SearchInsideIconFileName=pOptions->SearchInsideIconFileName;
        mSearchInfos.SearchFullPathMixedMode = pOptions->SearchFullPathMixedMode;

        if (mSearchInfos.SearchInsideDirectory && mSearchInfos.SearchInsideFileName && mSearchInfos.SearchFullPathMixedMode)
            mSearchInfos.SearchInsideFullPath = TRUE;

        if (mSearchInfos.SearchInsideIconDirectory && mSearchInfos.SearchInsideIconFileName && mSearchInfos.SearchFullPathMixedMode)
            mSearchInfos.SearchInsideIconFullPath = TRUE;
    }

    if (! (mSearchInfos.SearchInsideTargetDescription
        || mSearchInfos.SearchInsideArguments
        || mSearchInfos.SearchInsideDirectory
        || mSearchInfos.SearchInsideFileName
        || mSearchInfos.SearchInsideFullPath
        || mSearchInfos.SearchInsideIconDirectory
        || mSearchInfos.SearchInsideIconFileName
        || mSearchInfos.SearchInsideIconFullPath
        )
        )
    {
        ReportError(_T("At least a shortcut field must be set for search"));
        goto CleanUp;
    }

    if ( !(    pOptions->IncludeUserDesktopDirectory 
            || pOptions->IncludeCommonDesktopDirectory 
            || pOptions->IncludeCommonStartupDirectory 
            || pOptions->IncludeUserStartupDirectory 
            || pOptions->IncludeCustomFolder
            ) 
        )
    {
        ReportError(_T("At least a folder must be specified"));
        goto CleanUp;
    }

    if (pOptions->IncludeCustomFolder && (*mSearchInfos.CustomFolderPaths==0) ) 
    {
        ReportError(_T("Please specify the \"Search Folder\" directory"));
        goto CleanUp;
    }

    // if a search pattern is not specified
    
    if (   (mSearchInfos.SearchWay != SearchWay_SEARCH_DEAD_LINKS) 
           && ( (mSearchInfos.pszSearchStringUpper == NULL) || (mSearchInfos.pszSearchStringUpper && (*mSearchInfos.pszSearchStringUpper==0)) )
       )
    {
        ReportError(_T("Invalid search pattern"));
        goto CleanUp;
    }

    // disable wild char search for replace operation (basic replace only allowed)
    if ( (mSearchInfos.SearchWay==SearchWay_SEARCH_AND_REPLACE ) && (pOptions->ReplaceOnlyArguments == FALSE) )
    {
        if ( _tcspbrk(mSearchInfos.pszSearchStringUpper,_T("*?") ) )
        {
            ReportError(_T("Wild char search not allowed for replace operation"));
            goto CleanUp;
        }
    }


    if (  ( (mSearchInfos.pszReplaceString == NULL) || (mSearchInfos.pszReplaceString && (*mSearchInfos.pszReplaceString==0) ) )
        && (!pOptions->ReplaceOnlyArguments) // empty args replacement allowed
        ) 
    {
        if (mSearchInfos.SearchWay==SearchWay_SEARCH_AND_REPLACE)
        {
            ReportError(_T("No replace string specified"));
            goto CleanUp;
        }   
    }

    // put interface in searching mode
    SetStatus(_T("Searching..."));

    // if a folder is specified
    if (pOptions->IncludeCustomFolder)
    {
        SIZE_T ArraySize;
        TCHAR** PathsArray;

        if (_tcsnicmp(mSearchInfos.CustomFolderPaths,MULTIPLE_DIRECTORIES_IN_FILE_PREFIX,_tcslen(MULTIPLE_DIRECTORIES_IN_FILE_PREFIX))==0)
        {
            // list of directories is in file

            TCHAR* FilePath;
            FilePath = mSearchInfos.CustomFolderPaths+_tcslen(MULTIPLE_DIRECTORIES_IN_FILE_PREFIX);
            FilePath = CTrimString::TrimString(FilePath,FALSE);

            MULTIPLE_DIRECTORIES_FILE_INFOS Infos;
            Infos.bIncludeSubdir = pOptions->IncludeSubdirectories;
            Infos.CancelEvent = mhEvtCancel;
            Infos.pSearchInfos = &mSearchInfos;
            Infos.bSearchCanceled = bSearchCanceled;
            CTextFile::ParseLines(FilePath,mhEvtCancel,MultipleDirectoriesFileLinesParsingCallBack,&Infos);
            bSearchCanceled = Infos.bSearchCanceled;
        }
        else // Paths contains a single directory or a list of directories splitted by ';'
        {
            // get string array from a list of directories splitted by ';'
            PathsArray = CMultipleElementsParsing::ParseString(mSearchInfos.CustomFolderPaths,&ArraySize);
            if (!PathsArray)
                goto CleanUp;

            //for each specified folder
            SIZE_T ExtensionSize = _tcslen(_T("\\*.lnk"));
            for (SIZE_T Cnt = 0 ;Cnt <ArraySize; Cnt++)
            {
                _tcsncpy( Path,PathsArray[Cnt],_countof(Path)-ExtensionSize );
                Path[_countof(Path)-ExtensionSize-1]=0;

                // assume path exists
                if (!CStdFileOperations::DoesDirectoryExists(Path))
                {
                    TCHAR Msg[260+_countof(Path)];
                    _sntprintf(Msg,_countof(Msg),_T("Invalid directory %s"),Path);
                    ReportError(Msg);
                    continue;
                }

                // assume search path ends with '\'
                Size=_tcslen(Path);
                if (mSearchInfos.CustomFolderPaths[Size-1]!='\\')
                    _tcscat(Path,_T("\\"));
                // search link in directory
                _tcscat(Path,_T("*.lnk"));

                // search in specified folder
                CFileSearch::Search(Path,pOptions->IncludeSubdirectories,mhEvtCancel,CallBackFileFound,&mSearchInfos,&bSearchCanceled,TRUE);
                if (bSearchCanceled)
                {
                    CMultipleElementsParsing::ParseStringArrayFree(PathsArray,ArraySize);
                    goto CleanUp;
                }
            }
            CMultipleElementsParsing::ParseStringArrayFree(PathsArray,ArraySize);
        }
    }

    // if search must be done in user start menu
    if (pOptions->IncludeUserStartupDirectory)
    {
        ITEMIDLIST* pIdlList;
        if (SHGetSpecialFolderLocation(mhWndDialog,CSIDL_STARTMENU,&pIdlList)==S_OK)
            // A typical path is C:\Documents and Settings\username\Start Menu
        {
            if (SHGetPathFromIDList(pIdlList, Path))
            {
                Size=_tcslen(Path);
                if (Path[Size-1]!='\\')
                    _tcscat(Path,_T("\\"));
                _tcscat(Path,_T("*.lnk"));
                CFileSearch::Search(Path,TRUE,mhEvtCancel,CallBackFileFound,&mSearchInfos,&bSearchCanceled,TRUE);
                if (bSearchCanceled)
                {
                    CoTaskMemFree(pIdlList);
                    goto CleanUp;
                }
            }
            CoTaskMemFree(pIdlList);
        }
    }

    // if search must be done in common start menu
    if (pOptions->IncludeCommonStartupDirectory)
    {
        ITEMIDLIST* pIdlList;
        if (SHGetSpecialFolderLocation(mhWndDialog,CSIDL_COMMON_STARTMENU,&pIdlList)==S_OK)
            // C:\Documents and Settings\All Users\Start Menu
        {
            if (SHGetPathFromIDList(pIdlList, Path))
            {
                Size=_tcslen(Path);
                if (Path[Size-1]!='\\')
                    _tcscat(Path,_T("\\"));
                _tcscat(Path,_T("*.lnk"));
                CFileSearch::Search(Path,TRUE,mhEvtCancel,CallBackFileFound,&mSearchInfos,&bSearchCanceled,TRUE);
                if (bSearchCanceled)
                {
                    CoTaskMemFree(pIdlList);
                    goto CleanUp;
                }
            }
            CoTaskMemFree(pIdlList);
        }
    }

    // if search must be done in user desktop
    if (pOptions->IncludeUserDesktopDirectory)
    {
        ITEMIDLIST* pIdlList;
        if (SHGetSpecialFolderLocation(mhWndDialog,CSIDL_DESKTOPDIRECTORY,&pIdlList)==S_OK)
            // A typical path is C:\Documents and Settings\username\Desktop
        {
            if (SHGetPathFromIDList(pIdlList, Path))
            {
                Size=_tcslen(Path);
                if (Path[Size-1]!='\\')
                    _tcscat(Path,_T("\\"));
                _tcscat(Path,_T("*.lnk"));
                CFileSearch::Search(Path,pOptions->IncludeSubdirectories,mhEvtCancel,CallBackFileFound,&mSearchInfos,&bSearchCanceled,TRUE);
                if (bSearchCanceled)
                {
                    CoTaskMemFree(pIdlList);
                    goto CleanUp;
                }
            }
            CoTaskMemFree(pIdlList);
        }
    }

    // if search must be done in common desktop
    if (pOptions->IncludeCommonDesktopDirectory)
    {
        ITEMIDLIST* pIdlList;
        if (SHGetSpecialFolderLocation(mhWndDialog,CSIDL_COMMON_DESKTOPDIRECTORY,&pIdlList)==S_OK)
        {
            if (SHGetPathFromIDList(pIdlList, Path))
            {
                Size=_tcslen(Path);
                if (Path[Size-1]!='\\')
                    _tcscat(Path,_T("\\"));
                _tcscat(Path,_T("*.lnk"));
                CFileSearch::Search(Path,pOptions->IncludeSubdirectories,mhEvtCancel,CallBackFileFound,&mSearchInfos,&bSearchCanceled,TRUE);
                if (bSearchCanceled)
                {
                    CoTaskMemFree(pIdlList);
                    goto CleanUp;
                }
            }
            CoTaskMemFree(pIdlList);
        }
    }
    bSuccess = TRUE;
CleanUp:
    if(bSearchCanceled)
    {
        mSearchInfos.bSearchedCanceled = TRUE;
        SetStatus(_T("Search Canceled"));
        bSuccess = TRUE;
    }
    else if (bSuccess)
    {
#ifdef _DEBUG
        if (!mCommandLineMode)
        {
            DWORD NbLvItems = (DWORD)pListView->GetItemCount();
            if (NbLvItems != mSearchInfos.NbLinkFilesSearchedMatchingConditions)
                ::DebugBreak();
        }
#endif

        TCHAR sz[1024];
        *sz=0;
        switch(mSearchInfos.SearchWay)
        {
        case SearchWay_SEARCH:
            _sntprintf(sz,_countof(sz),_T("Search Completed: %u shortcut(s) found / %u shortcut(s)"),mSearchInfos.NbLinkFilesSearchedMatchingConditions,mSearchInfos.NbLinkFilesSearched);
            break;
        case SearchWay_SEARCH_AND_REPLACE:
            {

#ifdef _DEBUG
                if (!mCommandLineMode)
                {
                    DWORD NbAutoSelectedItems = (DWORD)pListView->GetSelectedCount();
                    if (NbAutoSelectedItems != mSearchInfos.NbLinkFilesSearchedForReplaceWithPathValidated)
                        ::DebugBreak();
                }
#endif
                _sntprintf(sz,_countof(sz),_T("Search Completed: %u shortcut(s) can be replaced / %u shortcut(s). %u with new path validated, %u with potential troubles"),
                    mSearchInfos.NbLinkFilesSearchedMatchingConditions,
                    mSearchInfos.NbLinkFilesSearched,
                    mSearchInfos.NbLinkFilesSearchedForReplaceWithPathValidated,
                    mSearchInfos.NbLinkFilesSearchedMatchingConditions-mSearchInfos.NbLinkFilesSearchedForReplaceWithPathValidated
                    );
            }
            break;
        case SearchWay_SEARCH_DEAD_LINKS:
            _sntprintf(sz,_countof(sz),_T("Search Completed: %u dead link(s) found / %u shortcut(s)"),mSearchInfos.NbLinkFilesSearchedMatchingConditions,mSearchInfos.NbLinkFilesSearched);
            break;
        }

        SetStatus(sz);
    }


    CoUninitialize();

    return bSuccess;
}


//-----------------------------------------------------------------------------
// Name: GUI_Search
// Object: grab user interface options and start searching
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
DWORD WINAPI GUI_Search(LPVOID lpParameter)
{
    SIZE_T Size;
    BYTE ButtonState;
    BOOL bSuccess = FALSE;
    DWORD dwProgressBarStyle = GetWindowLong(mhWndProgress,GWL_STYLE);

    // free previous allocated memory
    FreeLogs();
    FreeSearchInfos();
    FreePreviousSearchResults();

    // update options
    tagSearchWay SearchWay=(tagSearchWay)((DWORD)lpParameter);

    GetOptionsFromGui();

    mSearchInfos.SearchWay=SearchWay;

    // change interface according to searching way
    switch (SearchWay)
    {
    case SearchWay_SEARCH:
        ShowWindow(GetDlgItem(mhWndDialog,IDC_STATIC_RESOLVE),FALSE);
        ShowWindow(GetDlgItem(mhWndDialog,IDC_BUTTON_RESOLVE),FALSE);
        ShowWindow(GetDlgItem(mhWndDialog,IDC_STATIC_DELETE),TRUE);
        ShowWindow(GetDlgItem(mhWndDialog,IDC_BUTTON_DELETE),TRUE);
        ShowWindow(GetDlgItem(mhWndDialog,IDC_STATIC_REPLACE),FALSE);
        ShowWindow(GetDlgItem(mhWndDialog,IDC_BUTTON_REPLACE),FALSE);
        ButtonState = pToolbarListViewItemOperations->GetButtonState(IDC_BUTTON_CHECK_SAFE_FOR_REPLACE);
        ButtonState &= ~TBSTATE_ENABLED;
        pToolbarListViewItemOperations->SetButtonState(IDC_BUTTON_CHECK_SAFE_FOR_REPLACE,ButtonState);
        ButtonState = pToolbarListViewItemOperations->GetButtonState(IDC_BUTTON_CHECK_ON_ERROR_FOR_REPLACE);
        ButtonState &= ~TBSTATE_ENABLED;
        pToolbarListViewItemOperations->SetButtonState(IDC_BUTTON_CHECK_ON_ERROR_FOR_REPLACE,ButtonState);
        break;
    case SearchWay_SEARCH_AND_REPLACE:
        ShowWindow(GetDlgItem(mhWndDialog,IDC_STATIC_RESOLVE),FALSE);
        ShowWindow(GetDlgItem(mhWndDialog,IDC_BUTTON_RESOLVE),FALSE);
        //ShowWindow(GetDlgItem(mhWndDialog,IDC_STATIC_DELETE),FALSE);
        //ShowWindow(GetDlgItem(mhWndDialog,IDC_BUTTON_DELETE),FALSE);
        ShowWindow(GetDlgItem(mhWndDialog,IDC_STATIC_DELETE),TRUE);
        ShowWindow(GetDlgItem(mhWndDialog,IDC_BUTTON_DELETE),TRUE);
        ShowWindow(GetDlgItem(mhWndDialog,IDC_STATIC_REPLACE),TRUE);
        ShowWindow(GetDlgItem(mhWndDialog,IDC_BUTTON_REPLACE),TRUE);
        EnableWindow(GetDlgItem(mhWndDialog,IDC_BUTTON_REPLACE),TRUE);
        ButtonState = pToolbarListViewItemOperations->GetButtonState(IDC_BUTTON_CHECK_SAFE_FOR_REPLACE);
        ButtonState |= TBSTATE_ENABLED;
        pToolbarListViewItemOperations->SetButtonState(IDC_BUTTON_CHECK_SAFE_FOR_REPLACE,ButtonState);
        ButtonState = pToolbarListViewItemOperations->GetButtonState(IDC_BUTTON_CHECK_ON_ERROR_FOR_REPLACE);
        ButtonState |= TBSTATE_ENABLED;
        pToolbarListViewItemOperations->SetButtonState(IDC_BUTTON_CHECK_ON_ERROR_FOR_REPLACE,ButtonState);
        break;
    case SearchWay_SEARCH_DEAD_LINKS:
        ShowWindow(GetDlgItem(mhWndDialog,IDC_STATIC_RESOLVE),TRUE);
        ShowWindow(GetDlgItem(mhWndDialog,IDC_BUTTON_RESOLVE),TRUE);
        ShowWindow(GetDlgItem(mhWndDialog,IDC_STATIC_DELETE),TRUE);
        ShowWindow(GetDlgItem(mhWndDialog,IDC_BUTTON_DELETE),TRUE);
        ShowWindow(GetDlgItem(mhWndDialog,IDC_STATIC_REPLACE),FALSE);
        ShowWindow(GetDlgItem(mhWndDialog,IDC_BUTTON_REPLACE),FALSE);
        ButtonState = pToolbarListViewItemOperations->GetButtonState(IDC_BUTTON_CHECK_SAFE_FOR_REPLACE);
        ButtonState &= ~TBSTATE_ENABLED;
        pToolbarListViewItemOperations->SetButtonState(IDC_BUTTON_CHECK_SAFE_FOR_REPLACE,ButtonState);
        ButtonState = pToolbarListViewItemOperations->GetButtonState(IDC_BUTTON_CHECK_ON_ERROR_FOR_REPLACE);
        ButtonState &= ~TBSTATE_ENABLED;
        pToolbarListViewItemOperations->SetButtonState(IDC_BUTTON_CHECK_ON_ERROR_FOR_REPLACE,ButtonState);
        break;
    }

    // render new layout
    OnSize();

    // remove check boxes for simple search, add them for other search
    // removed in 1.3 to allow icon deletion from simple search
    // pListView->SetStyle(TRUE,FALSE,FALSE,(SearchWay!=SearchWay_SEARCH));
    
    *mSearchInfos.CustomFolderPaths=0;
    if (pOptions->IncludeCustomFolder)
        ::GetDlgItemText(mhWndDialog,IDC_EDIT_PATH,mSearchInfos.CustomFolderPaths,SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH);

    if ( (SearchWay==SearchWay_SEARCH) || (SearchWay==SearchWay_SEARCH_AND_REPLACE))
    {
        // get search
        Size=SendMessage(GetDlgItem(mhWndDialog,IDC_EDIT_SEARCH),WM_GETTEXTLENGTH,0,0);

        if ( (SearchWay==SearchWay_SEARCH) || pOptions->ReplaceOnlyArguments )
        {
            // we are going to make a wild char search, so assume there is '*' or '?' in searched string,
            // else add '*' before and after string

            // allocate memory for the search
            mSearchInfos.pszSearchStringUpper=(TCHAR*)new TCHAR[Size+1+2]; // +1 for '\0'; +2 for potential beginning and ending '*'
            GetDlgItemText(mhWndDialog,IDC_EDIT_SEARCH,mSearchInfos.pszSearchStringUpper,(int)(Size+1));
            mSearchInfos.pszSearchStringUpper[Size]=0;

            // since 1.6.12 : always add * before and * after the search not only if there is no joker (more user friendly)
            if (_tcscmp(mSearchInfos.pszSearchStringUpper,TEXT("*"))!=0) // if not single "*"
            {
                if (Size == 0)
                {
                    mSearchInfos.pszSearchStringUpper[0] = '*';
                    mSearchInfos.pszSearchStringUpper[1] = 0;
                }
                else
                {
                    memmove(&mSearchInfos.pszSearchStringUpper[1],mSearchInfos.pszSearchStringUpper,Size*sizeof(TCHAR));
                    mSearchInfos.pszSearchStringUpper[0] = '*';
                    mSearchInfos.pszSearchStringUpper[Size+1] = '*';
                    mSearchInfos.pszSearchStringUpper[Size+2] = 0;
                }
            }
        }
        else
        {
            // if a search pattern is not specified
            if (Size<=0)
            {
                ReportError(_T("Please enter a valid search pattern"));
                goto CleanUp;
            }

            // allocate memory for the search
            mSearchInfos.pszSearchStringUpper=(TCHAR*)new TCHAR[Size+1];
            GetDlgItemText(mhWndDialog,IDC_EDIT_SEARCH,mSearchInfos.pszSearchStringUpper,(int)(Size+1));
            mSearchInfos.pszSearchStringUpper[Size]=0;
        }

        // as we will do an insensitive search, put search string in upper case
        _tcsupr(mSearchInfos.pszSearchStringUpper);

        // get replace
        Size=SendMessage(GetDlgItem(mhWndDialog,IDC_EDIT_REPLACE),WM_GETTEXTLENGTH,0,0);
        // if a search pattern is not specified
        if ( (Size<=0) && (!pOptions->ReplaceOnlyArguments) ) 
        {
            mSearchInfos.pszReplaceString= NULL; // error report is done later
        }
        else
        {
            // allocate memory for the replace string
            mSearchInfos.pszReplaceString=(TCHAR*)new TCHAR[Size+1];
            GetDlgItemText(mhWndDialog,IDC_EDIT_REPLACE,mSearchInfos.pszReplaceString,(int)(Size+1));
            mSearchInfos.pszReplaceString[Size]=0;
        }
    }


    // add a not found target icon
    mListviewEmptyIconIndex=pListView->AddIcon(CListview::ImageListSmall,mhInstance,IDI_ICON_EMPTY);
    mListviewWarningIconIndex=pListView->AddIcon(CListview::ImageListSmall,mhInstance,IDI_ICON_WARNING);

    SetProgressMaxPosition(100);
    SetProgressPosition(0);
    SetWindowLong(mhWndProgress,GWL_STYLE,dwProgressBarStyle | PBS_MARQUEE);
    SendMessage(mhWndProgress,PBM_SETMARQUEE,TRUE,50);
    SetWaitCursor(TRUE);

    bSuccess = DoSearch();

CleanUp:
    // resort listview if needed
    pListView->ReSort();

    // allow to replace shortcut
    EnableWindow(GetDlgItem(mhWndDialog,IDC_BUTTON_REPLACE),TRUE);

    // warn the user the search is finished
    SetWaitCursor(FALSE);
    SendMessage(mhWndProgress,PBM_SETMARQUEE,FALSE,0);
    SetWindowLong(mhWndProgress,GWL_STYLE,dwProgressBarStyle);

    SetProgressPosition(100);

    if (pOptions->DisplaySuccessMessageInfosAfterActionCompleted)
    {
        if (bSuccess && (mSearchInfos.bSearchedCanceled == FALSE) )
            ReportInfo(_T("Search Completed"));
    }

    // at the end of search, restore toolbar buttons
    SetUserInterfaceMode(FALSE);


    if (bSuccess)
        return 0;
    else
        return (DWORD)-1;
}

//-----------------------------------------------------------------------------
// Name: FreeListViewAssociatedMemory
// Object: free memory of objects stored in listview items user data
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void FreeListViewAssociatedMemory()
{
    pListView->RemoveAllIcons(CListview::ImageListSmall);

    if (!pListView)
        return;
    CShortcutInfos* pShortcutInfos;
    int NbItems=pListView->GetItemCount();

    for (int Cnt=0;Cnt<NbItems;Cnt++)
    {
        if (pListView->GetItemUserData(Cnt,(LPVOID*)&pShortcutInfos))
        {
            if (pShortcutInfos)
                delete pShortcutInfos;
        }
    }
}

//-----------------------------------------------------------------------------
// Name: AddShortcutToList
// Object: add item to listview or command line list depending mode
// Parameters :
//     in  : 
//     out :
//     return : TRUE if item is added to list
//-----------------------------------------------------------------------------
BOOL AddShortcutToList(CShortcutInfos* pShortcutInfos)
{
    if (mCommandLineMode)
    {
        ConsoleWrite(pShortcutInfos->LinkPath, FALSE);
        ConsoleWrite(TEXT(" match"));
        return (mCommandLineListOfLinks.AddItem(pShortcutInfos)!=NULL);
    }
    else
        return AddShortcutToListView(pShortcutInfos);
}

//-----------------------------------------------------------------------------
// Name: AddShortcutToListView
// Object: add item to listview
// Parameters :
//     in  : 
//     out :
//     return : TRUE if item is added to listview
//-----------------------------------------------------------------------------
BOOL AddShortcutToListView(CShortcutInfos* pShortcutInfos)
{
    // get short name
    TCHAR LinkShortName[SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH];
	
    pShortcutInfos->hIcon = NULL;
    _tcscpy(LinkShortName,CStdFileOperations::GetFileName(pShortcutInfos->LinkPath));
    CStdFileOperations::RemoveFileExt(LinkShortName);

    // extract icon before adding item to list 
    // this avoid default icon display in the listview, during the few seconds of associated icon extraction
    int IconIndex=0;
    switch (mSearchInfos.SearchWay)
    {
    case SearchWay_SEARCH_AND_REPLACE:
        if (pShortcutInfos->NewTargetPathFileExists)
        {
            // set icon index
            WORD w=0;
            TCHAR IconPath[SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH];

            if (*pShortcutInfos->TargetIconPath !=0 )
            {
                if (pShortcutInfos->TargetIconPathNeedsChanges)
                {
                    SIZE_T RequieredSize=CStringReplace::ComputeMaxRequieredSizeForReplace(pShortcutInfos->TargetIconPath,mSearchInfos.pszSearchStringUpper,mSearchInfos.pszReplaceString);
                    TCHAR* pszIconPath=(TCHAR*)_alloca(RequieredSize*sizeof(TCHAR));
                    CStringReplace::Replace(pShortcutInfos->TargetIconPath,pszIconPath,mSearchInfos.pszSearchStringUpper,mSearchInfos.pszReplaceString,FALSE);
                    _tcsncpy(IconPath,pszIconPath,SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH-1);
                    IconPath[SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH-1]=0;
                }
                else
                {
                    _tcscpy(IconPath,pShortcutInfos->TargetIconPath);
                }
                ::ExtractIconEx(IconPath,pShortcutInfos->TargetIconIndex,NULL,&pShortcutInfos->hIcon,1);
            }
            else
            {
                TCHAR* pIconPath;
                TCHAR TmpIconPath[SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH];
                if (pShortcutInfos->TargetPathNeedsChanges)
                {
                    if (bRawMode)
                    {
                        // convert target path to real path removing all %VarName%
                        ::ExpandEnvironmentStrings(pShortcutInfos->NewTargetPath,TmpIconPath,SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH-1);
                        pIconPath = TmpIconPath;
                    }
                    else
                    {
                        pIconPath = pShortcutInfos->NewTargetPath;
                    }
                }
                else
                {
                    if (bRawMode)
                    {
                        // convert target path to real path removing all %VarName%
                        ::ExpandEnvironmentStrings(pShortcutInfos->TargetPath,TmpIconPath,SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH-1);
                        pIconPath = TmpIconPath;
                    }
                    else
                    {
                        pIconPath = pShortcutInfos->TargetPath;
                    }
                }
                _tcscpy(IconPath,pIconPath);
                pShortcutInfos->hIcon=::ExtractAssociatedIcon(mhInstance,IconPath,&w);// warning IconPath is an inout parameter
            }
            
            IconIndex=pListView->AddIcon(CListview::ImageListSmall,pShortcutInfos->hIcon);
        }

        break;

    case SearchWay_SEARCH:
        {
            // get current associated icon
            WORD w=0;
            TCHAR IconPath[SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH];
            if (*pShortcutInfos->TargetIconPath !=0 )
            {
                _tcscpy(IconPath,pShortcutInfos->TargetIconPath);
                ::ExtractIconEx(IconPath,pShortcutInfos->TargetIconIndex,NULL,&pShortcutInfos->hIcon,1);
            }
            else
            {
                // debug only : only to know which exe contains icon
                //TCHAR AssociatedDll[MAX_PATH];
                //::FindExecutable(pShortcutInfos->TargetPath,NULL,AssociatedDll);
                TCHAR* pIconPath;
                TCHAR TmpIconPath[SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH];

                if (bRawMode)
                {
                    // convert target path to real path removing all %VarName%
                    ::ExpandEnvironmentStrings(pShortcutInfos->TargetPath,TmpIconPath,SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH-1);
                    pIconPath = TmpIconPath;
                }
                else
                {
                    pIconPath = pShortcutInfos->TargetPath;
                }

                _tcscpy(IconPath,pIconPath);
                pShortcutInfos->hIcon=::ExtractAssociatedIcon(mhInstance,IconPath,&w);// warning IconPath is an inout parameter
            }

            if (pShortcutInfos->hIcon)
                IconIndex=pListView->AddIcon(CListview::ImageListSmall,pShortcutInfos->hIcon);
        }
        break;

    }

    // add item to listview
    int ItemIndex=pListView->AddItem(LinkShortName,pShortcutInfos,FALSE);
    if (ItemIndex<0)
        return FALSE;
    
    pListView->SetItemText(ItemIndex,ListviewSearchResultColumnsIndex_TargetPath,pShortcutInfos->TargetPath);
    pListView->SetItemText(ItemIndex,ListviewSearchResultColumnsIndex_TargetWorkingDir,pShortcutInfos->TargetWorkingDir);
    pListView->SetItemText(ItemIndex,ListviewSearchResultColumnsIndex_TargetArgs,pShortcutInfos->TargetArgs);
    pListView->SetItemText(ItemIndex,ListviewSearchResultColumnsIndex_TargetDescription,pShortcutInfos->TargetDescription);
    pListView->SetItemText(ItemIndex,ListviewSearchResultColumnsIndex_ShortcutPath,pShortcutInfos->LinkPath);
    

    if (*pShortcutInfos->TargetIconPath)
    {
        TCHAR szIcon[SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH+50];
        _sntprintf(szIcon,_countof(szIcon),_T("%s,%d"),pShortcutInfos->TargetIconPath,pShortcutInfos->TargetIconIndex);
        pListView->SetItemText(ItemIndex,ListviewSearchResultColumnsIndex_TargetIcon,szIcon);
    }

    switch (mSearchInfos.SearchWay)
    {
    case SearchWay_SEARCH_AND_REPLACE:
        if (pShortcutInfos->NewTargetPathFileExists)
        {
            // select item if new target exists
            pListView->SetSelectedState(ItemIndex,TRUE,FALSE);
            pListView->SetItemIconIndex(ItemIndex,IconIndex);
        }
        else
            pListView->SetItemIconIndex(ItemIndex,mListviewWarningIconIndex);

        break;

    case SearchWay_SEARCH:
        pListView->SetItemIconIndex(ItemIndex,IconIndex);
        break;

    case SearchWay_SEARCH_DEAD_LINKS:
        // select item
        pListView->SetSelectedState(ItemIndex,TRUE,FALSE);
        // associated default icon
        pListView->SetItemIconIndex(ItemIndex,mListviewWarningIconIndex);
        break;
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: ResolveSelectedItems
// Object: GUI mode : resolve selected shortcuts in listview
//         Console mode : resolve all shortcuts
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void ResolveSelectedItems()
{
    BOOL Success=TRUE;
    BOOL bResolveSuccess;
    DWORD NbShortcutsSuccessfullyResolved = 0;
    DWORD NbShortcutsToResolve = 0;
    DWORD CntItemsTreated;
    CShortcutInfos* pShortcutInfos;

    // free previous logs if any
    FreeLogs();

    SetStatus(_T("Resolving..."));

    if (mCommandLineMode)
    {
        NbShortcutsToResolve = (DWORD)mCommandLineListOfLinks.GetItemsCount();
        SetProgressMaxPosition((WORD)NbShortcutsToResolve);
        SetProgressPosition(0);

        // in command line mode as there is no selection, replace all the items
        CLinkListItemTemplate<CShortcutInfos>* pItemShortcutInfos;
        for (pItemShortcutInfos = mCommandLineListOfLinks.GetHead(),CntItemsTreated=1;
            pItemShortcutInfos;
            pItemShortcutInfos=pItemShortcutInfos->NextItem,CntItemsTreated++)
        {
            bResolveSuccess = ResolveShortCutInfos(pItemShortcutInfos->ItemData);
            Success=Success && bResolveSuccess;

            if (bResolveSuccess)
                NbShortcutsSuccessfullyResolved++;

            SetProgressPosition((WORD)CntItemsTreated);
        }
    }
    else
    {
        WORD wNbItems = (WORD)pListView->GetItemCount();
        SetProgressMaxPosition(wNbItems);
        SetProgressPosition(0);
        // for each listview item
        int ListviewItemIndex;
        for (ListviewItemIndex=0,CntItemsTreated=1;ListviewItemIndex<pListView->GetItemCount();CntItemsTreated++)
        {
            // if item is selected
            if (pListView->IsItemSelected(ListviewItemIndex))
            {
                NbShortcutsToResolve++;

                // get pShortcutInfos object associated to item
                pListView->GetItemUserData(ListviewItemIndex,(LPVOID*)&pShortcutInfos);

                // try to resolve shortcut
                bResolveSuccess=ResolveShortCutInfos(pShortcutInfos);
                Success=Success && bResolveSuccess;

                if (bResolveSuccess)
                {
                    NbShortcutsSuccessfullyResolved++;
                    // auto unselect successfully change items
                    // pListView->SetSelectedState(Cnt,FALSE,FALSE,FALSE);

                    // delete shortcut infos
                    delete pShortcutInfos;
                    // remove item from list
                    pListView->RemoveItem(ListviewItemIndex);

                    // don't increase ListviewItemIndex as we have removed the item.
                    // Next item as same index as current removed item
                }
                else
                {
                    // increase counter
                    ListviewItemIndex++;
                }
            }
            else
            {
                // increase counter
                ListviewItemIndex++;
            }
            SetProgressPosition((WORD)CntItemsTreated);
        }

        // assume to invalidate to update background colors
        ::InvalidateRect(pListView->GetControlHandle(),NULL,FALSE);
    }


    if (Success)
    {
        TCHAR Msg[1024];
        // update status bar
        _sntprintf(Msg,_countof(Msg),_T("All shortcuts successfully resolved (%u shortcuts)"),NbShortcutsSuccessfullyResolved);
        SetStatus(Msg);
        if (pOptions->DisplaySuccessMessageInfosAfterActionCompleted)
            ReportInfo(_T("Resolve Completed"));
    }
    else
    {
        TCHAR Msg[1024];
        // update status bar
        _sntprintf(Msg,_countof(Msg),_T("%u shortcut(s) successfully resolved / %u shortcut(s)"),NbShortcutsSuccessfullyResolved,NbShortcutsToResolve);
        SetStatus(Msg);

        // provide user message
        _sntprintf(Msg,_countof(Msg),_T("Some error(s) occurred when resolving shortcuts\r\n%u shortcut(s) successfully resolved / %u shortcut(s)"),NbShortcutsSuccessfullyResolved,NbShortcutsToResolve);
        ReportError(Msg);

        // GUI mode don't show logs as successful item are removed from list
        // Command line mode ShowLogs() does nothing at all, as error are displayed in real time
        // ShowLogs(); 
    }
}

//-----------------------------------------------------------------------------
// Name: ResolveShortCutInfos
// Object: resolve a single shortcut
// Parameters :
//     in  : CListViewItemUserDataShortcutInfos* pShortcutInfos : shortcut informations
//     out :
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL ResolveShortCutInfos(CShortcutInfos* pShortcutInfos)
{
    if (IsBadReadPtr(pShortcutInfos,sizeof(CShortcutInfos)))
        return FALSE;
    pShortcutInfos->ChangeState = CShortcutInfos::CHANGE_STATE_ChangeFailure;
    BOOL Success=FALSE;
    HRESULT hResult;
    IShellLink *pShellLink = NULL;
    IPersistFile *pPersistFile = NULL;
    WCHAR* pwLinkName=NULL;

    // create IShellLink instance
    hResult = CoCreateInstance( CLSID_ShellLink,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                IID_IShellLink,
                                (LPVOID*)&pShellLink);
    if ( FAILED(hResult) || (pShellLink==NULL) )
        goto Cleanup;

    // get IPersistFile of IShellLink instance
    hResult = pShellLink->QueryInterface(IID_IPersistFile,(LPVOID*)&pPersistFile);
    if ( FAILED(hResult) || (pPersistFile==NULL) )
        goto Cleanup;

#if (defined(UNICODE)||defined(_UNICODE))
    pwLinkName=pShortcutInfos->LinkPath;
#else
    CAnsiUnicodeConvert::AnsiToUnicode(pShortcutInfos->LinkPath,&pwLinkName);
#endif

    // load shortcut infos in read write mode
    hResult = pPersistFile->Load(pwLinkName,STGM_READWRITE);

    if ( FAILED(hResult) )
        goto Cleanup;

    // resolve shortcut
    hResult=pShellLink->Resolve(mhWndDialog,SLR_NO_UI);
    if ( FAILED(hResult) || (hResult==S_FALSE) /* m$ inside S_FALSE is not a failure :$ */ )
        goto Cleanup;

    // save changes
    hResult=pPersistFile->Save(pwLinkName,TRUE);

    if ( FAILED(hResult) )
        goto Cleanup;

    Success=TRUE;
    pShortcutInfos->ChangeState = CShortcutInfos::CHANGE_STATE_ChangeSuccess;

Cleanup:

#if ( (!defined(UNICODE)) && (!defined(_UNICODE)))
    if (pwLinkName)
        free(pwLinkName);
#endif

    if (pShellLink)
        pShellLink->Release();

    if (pPersistFile)
        pPersistFile->Release();

    if (!Success)
    {
        TCHAR Msg[2*MAX_PATH];
        _sntprintf(Msg,2*MAX_PATH,_T("Error Resolving link %s\r\n"),pShortcutInfos->LinkPath);
        LogEvent(Msg);
    }

    return Success;
}

BOOL DeleteShortcut(CShortcutInfos* pShortcutInfos)
{
    DWORD FileAttributs;
    BOOL bDeleteSuccess= ::DeleteFile(pShortcutInfos->LinkPath);

    if ( (!bDeleteSuccess) && pOptions->ApplyChangesToReadOnlyShortcuts )
    {
        FileAttributs = ::GetFileAttributes(pShortcutInfos->LinkPath);
        if (FileAttributs & FILE_ATTRIBUTE_READONLY)
        {
            FileAttributs &= ~FILE_ATTRIBUTE_READONLY;
            if (!::SetFileAttributes(pShortcutInfos->LinkPath,FileAttributs))
            {
                TCHAR Msg[260+SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH];
                _sntprintf(Msg,_countof(Msg),_T("Error modifying the read only file attribute of link %s\r\n"),pShortcutInfos->LinkPath);
                LogEvent(Msg);
                goto CleanUp;
            }
        }

        // delete shortcut
        bDeleteSuccess= ::DeleteFile(pShortcutInfos->LinkPath);

        if (!bDeleteSuccess)
        {
            TCHAR Msg[260+SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH];
            _sntprintf(Msg,_countof(Msg),_T("Error deleting link %s\r\n"),pShortcutInfos->LinkPath);
            LogEvent(Msg);
        }
    }
CleanUp:
    return bDeleteSuccess;
}

//-----------------------------------------------------------------------------
// Name: DeleteSelectedItems
// Object: delete shortcuts
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void DeleteSelectedItems()
{
    BOOL Success=TRUE;
    BOOL bDeleteSuccess;
    DWORD NbShortcutsToDelete=0;
    DWORD NbShortcutsSuccessfullyDeleted=0;
    DWORD CntItemsTreated;
    CShortcutInfos* pShortcutInfos;
    BOOL bContinueDelete = FALSE;

    SetStatus(_T("Deleting selected items..."));

    #define DeleteQuery _T("Are you sure you want to delete selected shortcuts ?")
    if (mCommandLineMode)
    {
        if (CommandLineOptions.DeleteWithNoPrompt)
            bContinueDelete = TRUE;
        else
        {
            ConsoleWrite(DeleteQuery TEXT(" y/n"));
            TCHAR Answer = ConsoleGetChar();
            if ( (Answer == 'y') || (Answer == 'Y') )
                bContinueDelete = TRUE;
        }
    }
    else
    {
        if (::MessageBox(mhWndDialog,DeleteQuery,_T("Question"),MB_ICONQUESTION|MB_YESNO) == IDYES)
            bContinueDelete = TRUE;
    }

    if (!bContinueDelete)
    {
        SetStatus(_T("Delete operation canceled by user"));
        return;
    }

    // free previous logs if any
    FreeLogs();

    if (mCommandLineMode)
    {
        NbShortcutsToDelete = (DWORD)mCommandLineListOfLinks.GetItemsCount();
        SetProgressMaxPosition((WORD)NbShortcutsToDelete);
        SetProgressPosition(0);

        // in command line mode as there is no selection, replace all the items
        CLinkListItemTemplate<CShortcutInfos>* pItemShortcutInfos;
        for (pItemShortcutInfos = mCommandLineListOfLinks.GetHead(),CntItemsTreated=1;
            pItemShortcutInfos;
            pItemShortcutInfos=pItemShortcutInfos->NextItem,CntItemsTreated++)
        {
            // delete shortcut
            bDeleteSuccess= DeleteShortcut(pItemShortcutInfos->ItemData);
            Success=Success && bDeleteSuccess;

            if (bDeleteSuccess)
                NbShortcutsSuccessfullyDeleted++;

            SetProgressPosition((WORD)CntItemsTreated);
        }
    }
    else
    {
        pOptions->ApplyChangesToReadOnlyShortcuts = ((pToolbarInclude->GetButtonState(IDC_BUTTON_APPLY_CHANGES_TO_READONLY)&TBSTATE_CHECKED)!=0);

        WORD wNbItems = (WORD)pListView->GetItemCount();
        SetProgressMaxPosition(wNbItems);
        SetProgressPosition(0);

        // for each listview item
        int ListviewItemIndex;
        for (ListviewItemIndex=0,CntItemsTreated=1;ListviewItemIndex<pListView->GetItemCount();CntItemsTreated++)
        {
            // if item is selected
            if (pListView->IsItemSelected(ListviewItemIndex))
            {
                NbShortcutsToDelete++; 

                // get pShortcutInfos object associated to item
                pListView->GetItemUserData(ListviewItemIndex,(LPVOID*)&pShortcutInfos);
                
                // delete shortcut
                bDeleteSuccess= DeleteShortcut(pShortcutInfos);
                Success=Success && bDeleteSuccess;

                if (bDeleteSuccess)
                {
                    NbShortcutsSuccessfullyDeleted++;

                    // delete shortcut infos
                    delete pShortcutInfos;
                    // remove item from list
                    pListView->RemoveItem(ListviewItemIndex);
                }
                else
                {
                    // increase counter
                    ListviewItemIndex++;
                }
            }
            else
            {
                // increase counter
                ListviewItemIndex++;
            }
            SetProgressPosition((WORD)CntItemsTreated);
        }
    }

    if (Success)
    {
        TCHAR Msg[1024];
        // update status bar
        _sntprintf(Msg,_countof(Msg),_T("All shortcuts successfully deleted (%u shortcuts)"),NbShortcutsSuccessfullyDeleted);
        SetStatus(Msg);

        if (pOptions->DisplaySuccessMessageInfosAfterActionCompleted)
            ReportInfo(_T("Delete Completed"));
    }
    else
    {
        TCHAR Msg[1024];
        // update status bar
        _sntprintf(Msg,_countof(Msg),_T("%u shortcut(s) successfully deleted / %u shortcut(s)"),NbShortcutsSuccessfullyDeleted,NbShortcutsToDelete);
        SetStatus(Msg);

        // provide user message
        _sntprintf(Msg,_countof(Msg),_T("Some error(s) occurred when deleting shortcut\r\n%u shortcut(s) successfully deleted / %u shortcut(s)"),NbShortcutsSuccessfullyDeleted,NbShortcutsToDelete);
        ReportError(Msg);

        // GUI mode don't show logs as successful item are removed from list
        // Command line mode ShowLogs() does nothing at all, as error are displayed in real time
        // ShowLogs();
    }
}

//-----------------------------------------------------------------------------
// Name: ReplaceSelectedItems
// Object: GUI mode : replace selected shortcuts in listview
//         Console mode : replace all shortcuts
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void ReplaceSelectedItems()
{
    BOOL Success=TRUE;
    BOOL bCurrentChangeSuccess;
    DWORD NbShortcutsToReplace=0;
    DWORD NbShortcutsSuccessfullyReplaced=0;
    CShortcutInfos* pShortcutInfos;

    // free previous logs if any
    FreeLogs();

    if (mCommandLineMode)
    {
        SetStatus(_T("Replacing found items..."));
        DWORD Cnt;
        NbShortcutsToReplace = (DWORD)mCommandLineListOfLinks.GetItemsCount();
        SetProgressMaxPosition((WORD)NbShortcutsToReplace);
        SetProgressPosition(0);

        // in command line mode as there is no selection, replace all the items
        CLinkListItemTemplate<CShortcutInfos>* pItemShortcutInfos;
        for (pItemShortcutInfos = mCommandLineListOfLinks.GetHead(),Cnt=1;
            pItemShortcutInfos;
            pItemShortcutInfos=pItemShortcutInfos->NextItem,Cnt++)
        {
            bCurrentChangeSuccess = ChangeShortCutInfos(pItemShortcutInfos->ItemData);
            Success=Success && bCurrentChangeSuccess;

            if (bCurrentChangeSuccess)
                NbShortcutsSuccessfullyReplaced++;

            SetProgressPosition((WORD)Cnt);
        }
    }
    else
    {
        SetStatus(_T("Replacing selected items..."));

        WORD wNbItems = (WORD)pListView->GetItemCount();
        SetProgressMaxPosition(wNbItems);
        SetProgressPosition(0);

        // lock tool bar button and update pOptions->ApplyChangesToReadOnlyShortcuts
        pToolbarInclude->EnableButton(IDC_BUTTON_APPLY_CHANGES_TO_READONLY,FALSE);
        pOptions->ApplyChangesToReadOnlyShortcuts = ((pToolbarInclude->GetButtonState(IDC_BUTTON_APPLY_CHANGES_TO_READONLY)&TBSTATE_CHECKED)!=0);


        // for each listview item
        for (int Cnt=0;Cnt<pListView->GetItemCount();Cnt++)
        {
            // if item is selected
            if (pListView->IsItemSelected(Cnt))
            {
                NbShortcutsToReplace++;

                // get pShortcutInfos object associated to item
                pListView->GetItemUserData(Cnt,(LPVOID*)&pShortcutInfos);

                // change shortcut paths
                // Success=Success && ChangeShortCutInfos(pShortcutInfos); bug && optimization, ChangeShortCutInfos not called anymore after first failure
                bCurrentChangeSuccess = ChangeShortCutInfos(pShortcutInfos);
                Success=Success && bCurrentChangeSuccess;

                if (bCurrentChangeSuccess)
                {
                    NbShortcutsSuccessfullyReplaced++;
                    // auto unselect successfully change items
                    pListView->SetSelectedState(Cnt,FALSE,FALSE,FALSE);
                }
            }

            SetProgressPosition((WORD)(Cnt+1));
        }

        pToolbarInclude->EnableButton(IDC_BUTTON_APPLY_CHANGES_TO_READONLY,TRUE);

        // assume to invalidate to update background colors
        ::InvalidateRect(pListView->GetControlHandle(),NULL,FALSE);
    }

    // as ChangeShortCutInfos may add some device to system,
    // remove all defined devices from system
    CLinkListItem* pItemDosDevice;
    CDefinedDosDeviceInfos* pDefinedDosDeviceInfos;
    for (pItemDosDevice = pDefinedDosDrivesList->Head;pItemDosDevice;pItemDosDevice=pItemDosDevice->NextItem)
    {
        pDefinedDosDeviceInfos = (CDefinedDosDeviceInfos*)pItemDosDevice->ItemData;
        ::DefineDosDevice (DDD_REMOVE_DEFINITION|DDD_EXACT_MATCH_ON_REMOVE,
                           pDefinedDosDeviceInfos->MappedPath,
                           pDefinedDosDeviceInfos->Device
                           );
        delete pDefinedDosDeviceInfos;
    }
    pDefinedDosDrivesList->RemoveAllItems();

    if (Success)
    {
        TCHAR Msg[1024];
        // update status bar
        _sntprintf(Msg,_countof(Msg),_T("All shortcuts successfully replaced (%u shortcuts)"),NbShortcutsSuccessfullyReplaced);
        SetStatus(Msg);

        if (pOptions->DisplaySuccessMessageInfosAfterActionCompleted)
            ReportInfo(_T("Replace Completed"));
    }
    else
    {
        TCHAR Msg[1024];
        // update status bar
        _sntprintf(Msg,_countof(Msg),_T("%u shortcut(s) successfully replaced / %u shortcut(s)"),NbShortcutsSuccessfullyReplaced,NbShortcutsToReplace);
        SetStatus(Msg);

        // provide user message
        _sntprintf(Msg,_countof(Msg),_T("Some error(s) occurred when replacing shortcuts\r\n%u shortcut(s) successfully replaced / %u shortcut(s)"),NbShortcutsSuccessfullyReplaced,NbShortcutsToReplace);
        ReportInfo(Msg);
        ShowLogs();
    }
}

//-----------------------------------------------------------------------------
// Name: ChangeShortCutInfos
// Object: replace strings contained in a single shortcut according to provided informations
// Parameters :
//     in  : CListViewItemUserDataShortcutInfos* pShortcutInfos : shortcut informations
//     out :
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL ChangeShortCutInfos(CShortcutInfos* pShortcutInfos)
{
    if (IsBadReadPtr(pShortcutInfos,sizeof(CShortcutInfos)))
        return FALSE;

    TCHAR* ExtraErrorInfos = NULL;// static only
    BOOL Success=FALSE;
    HRESULT hResult;
    IShellLink *pShellLink = NULL;
    IPersistFile *pPersistFile = NULL;
    WCHAR* pwLinkName=NULL;
    BOOL bReadOnlyModeNeedsToBeRestored=FALSE;
    DWORD FileAttributs;

    pShortcutInfos->ChangeState = CShortcutInfos::CHANGE_STATE_ChangeFailure;

    // create IShellLink instance
    hResult = CoCreateInstance( CLSID_ShellLink,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                IID_IShellLink,
                                (LPVOID*)&pShellLink);
    if ( FAILED(hResult) || (pShellLink==NULL) )
        goto Cleanup;

    // get IPersistFile of IShellLink instance
    hResult = pShellLink->QueryInterface(IID_IPersistFile,(LPVOID*)&pPersistFile);
    if ( FAILED(hResult) || (pPersistFile==NULL) )
        goto Cleanup;

    if (pOptions->ApplyChangesToReadOnlyShortcuts)
    {
        FileAttributs = ::GetFileAttributes(pShortcutInfos->LinkPath);
        if (FileAttributs & FILE_ATTRIBUTE_READONLY)
        {
            bReadOnlyModeNeedsToBeRestored = TRUE;
            FileAttributs &= ~FILE_ATTRIBUTE_READONLY;
            if (!::SetFileAttributes(pShortcutInfos->LinkPath,FileAttributs))
            {
                TCHAR Msg[260+SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH];
                _sntprintf(Msg,_countof(Msg),_T("Error modifying the read only file attribute of link %s\r\n"),pShortcutInfos->LinkPath);
                LogEvent(Msg);
                if (ExtraErrorInfos)
                    LogEvent(ExtraErrorInfos);
            }
        }           
    }

#if (defined(UNICODE)||defined(_UNICODE))
    pwLinkName=pShortcutInfos->LinkPath;
#else
    CAnsiUnicodeConvert::AnsiToUnicode(pShortcutInfos->LinkPath,&pwLinkName);
#endif

    // load shortcut infos in read write mode
    hResult = pPersistFile->Load(pwLinkName,STGM_READWRITE);

    if ( FAILED(hResult) )
        goto Cleanup;


    //MSDN : kb 263324
    //"j:\my long directory\myapplication.exe"
    //When you run this script and drive J does not exist, you can observe the created shortcut, but the target path is:
    //J:\My_long_\Myapplic.exe
    //NOTE: Any characters that are not normally supported by file systems that do not want long file names, such as the space character, are replaced by the underscore symbol "_".
    //To work around this problem, you can use the subst command to point drive J to a local hard disk
    TCHAR FileDrive[MAX_PATH];
    if (CStdFileOperations::GetFileDrive(pShortcutInfos->NewTargetPath,FileDrive)) // can fail for network path
    {
        if (!CStdFileOperations::DoesDirectoryExists(FileDrive))
        {
            if(!::DefineDosDevice(0,FileDrive,WindowsDrive))
            {
                ExtraErrorInfos = _T("Drive is not existing and can't be defined, so shortcut will have short name if replaced. Replaced canceled\r\n");
                goto Cleanup;
            }
            else
            {
                CDefinedDosDeviceInfos* pDefinedDosDeviceInfos = new CDefinedDosDeviceInfos(FileDrive,WindowsDrive);
                pDefinedDosDrivesList->AddItem(pDefinedDosDeviceInfos);
            }
        }
    }

    // set new path
    if (pShortcutInfos->TargetPathNeedsChanges)
    {
        hResult = pShellLink->SetPath( pShortcutInfos->NewTargetPath );
        if ( FAILED(hResult) )
            goto Cleanup;
    }

    // set working directory 
    if (pShortcutInfos->TargetWorkingDirNeedsChanges)
    {
        SIZE_T RequieredSize=CStringReplace::ComputeMaxRequieredSizeForReplace(pShortcutInfos->TargetWorkingDir,mSearchInfos.pszSearchStringUpper,mSearchInfos.pszReplaceString);
        TCHAR* WorkingDir=(TCHAR*)_alloca(RequieredSize*sizeof(TCHAR));
        CStringReplace::Replace(pShortcutInfos->TargetWorkingDir,WorkingDir,mSearchInfos.pszSearchStringUpper,mSearchInfos.pszReplaceString,FALSE);
        hResult = pShellLink->SetWorkingDirectory( WorkingDir );
        if ( FAILED(hResult) )
            goto Cleanup;
    }
    // set icon path
    if (pShortcutInfos->TargetIconPathNeedsChanges)
    {
        hResult = pShellLink->SetIconLocation( pShortcutInfos->NewTargetIconPath,pShortcutInfos->TargetIconIndex );
        if ( FAILED(hResult) )
            goto Cleanup;
    }
    

    // set args
    if (pShortcutInfos->TargetArgsNeedsChanges)
    {
        if (pOptions->ReplaceOnlyArguments)
        {
            hResult = pShellLink->SetArguments(mSearchInfos.pszReplaceString);
        }
        else
        {
            SIZE_T RequieredSize=CStringReplace::ComputeMaxRequieredSizeForReplace(pShortcutInfos->TargetArgs,mSearchInfos.pszSearchStringUpper,mSearchInfos.pszReplaceString);
            TCHAR* Args=(TCHAR*)_alloca(RequieredSize*sizeof(TCHAR));
            CStringReplace::Replace(pShortcutInfos->TargetArgs,Args,mSearchInfos.pszSearchStringUpper,mSearchInfos.pszReplaceString,FALSE);
            hResult = pShellLink->SetArguments(Args);
        }

        if ( FAILED(hResult) )
            goto Cleanup;
    }

    // set Description
    if (pShortcutInfos->TargetDescriptionNeedsChanges)
    {
        SIZE_T RequieredSize=CStringReplace::ComputeMaxRequieredSizeForReplace(pShortcutInfos->TargetDescription,mSearchInfos.pszSearchStringUpper,mSearchInfos.pszReplaceString);
        TCHAR* Description=(TCHAR*)_alloca(RequieredSize*sizeof(TCHAR));
        CStringReplace::Replace(pShortcutInfos->TargetDescription,Description,mSearchInfos.pszSearchStringUpper,mSearchInfos.pszReplaceString,FALSE);
        hResult = pShellLink->SetDescription(Description);
        if ( FAILED(hResult) )
            goto Cleanup;
    }


    // save changes
    hResult=pPersistFile->Save(pwLinkName,TRUE);
    if ( FAILED(hResult) )
        goto Cleanup;

    Success=TRUE;
    pShortcutInfos->ChangeState = CShortcutInfos::CHANGE_STATE_ChangeSuccess;

Cleanup:
#if ( (!defined(UNICODE)) && (!defined(_UNICODE)))
    if (pwLinkName)
        free(pwLinkName);
#endif

    if (pShellLink)
        pShellLink->Release();

    if (pPersistFile)
        pPersistFile->Release();

    if (bReadOnlyModeNeedsToBeRestored)
    {
        FileAttributs = ::GetFileAttributes(pShortcutInfos->LinkPath);
        FileAttributs|= FILE_ATTRIBUTE_READONLY;
        ::SetFileAttributes(pShortcutInfos->LinkPath,FileAttributs);
    }

    if (!Success)
    {
        TCHAR Msg[260+SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH];
        _sntprintf(Msg,_countof(Msg),_T("Error modifying link %s\r\n"),pShortcutInfos->LinkPath);
        LogEvent(Msg);
        if (ExtraErrorInfos)
            LogEvent(ExtraErrorInfos);
    }

    return Success;
}

//-----------------------------------------------------------------------------
// Name: IsSpecialPath
// Object: check for special paths
// Parameters :
//     in  : TCHAR* Path : shortcut target path
//     out :
//     return : TRUE if special path
//-----------------------------------------------------------------------------
BOOL IsSpecialPath(TCHAR* Path)
{
    // empty dir points to computer
    if (*Path==0)
        return TRUE;

    // check protocols
    for (DWORD Cnt=0;*ShortcutsSpecialProtocols[Cnt];Cnt++)
    {
        if (_tcsnicmp(Path,ShortcutsSpecialProtocols[Cnt],_tcslen(ShortcutsSpecialProtocols[Cnt]))==0)
            return TRUE;
    }

    return FALSE;
}

//-----------------------------------------------------------------------------
// Name: LogEvent
// Object: log an event
// Parameters :
//     in  : const TCHAR* Msg : event message
//     out :
//     return : 
//-----------------------------------------------------------------------------
void LogEvent(TCHAR* const Msg)
{
    if (mCommandLineMode)
        ConsoleWrite(Msg);
    else
        mLogs.Append(Msg);
}

//-----------------------------------------------------------------------------
// Name: ShowLogs
// Object: display logs
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void ShowLogs()
{
    if (!mCommandLineMode)
        CLogs::Show(mhInstance,mhWndDialog,mLogs.GetString());
}

//-----------------------------------------------------------------------------
// Name: FreeLogs
// Object: Free memory associated to logs
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void FreeLogs()
{
    mLogs.Clear();
} 

//-----------------------------------------------------------------------------
// Name: SetUserInterfaceMode
// Object: set toolbar mode (enable or disable some functionalities)
// Parameters :
//     in  : BOOL bIsSearching : TRUE if in searching mode
//     out :
//     return : 
//-----------------------------------------------------------------------------
void SetUserInterfaceMode(BOOL bIsSearching)
{
    if (mCommandLineMode)
        return;

    pToolbarMain->EnableButton(IDC_BUTTON_SEARCH,!bIsSearching);
    pToolbarMain->EnableButton(IDC_BUTTON_SEARCH_AND_REPLACE,!bIsSearching);
    pToolbarMain->EnableButton(IDC_BUTTON_SEARCH_DEAD_LINKS,!bIsSearching);
    pToolbarMain->EnableButton(IDC_BUTTON_CANCEL,bIsSearching);

    pToolbarInclude->EnableButton(IDC_BUTTON_DESCRIPTION_UPDATE,!bIsSearching);
    pToolbarInclude->EnableButton(IDC_BUTTON_ARGUMENTS_UPDATE,!bIsSearching);
    pToolbarInclude->EnableButton(IDC_BUTTON_REPLACE_ONLY_ARGUMENTS,!bIsSearching);
    pToolbarInclude->EnableButton(IDC_BUTTON_DIRECTORY_UPDATE,!bIsSearching);
    pToolbarInclude->EnableButton(IDC_BUTTON_FILENAME_UPDATE,!bIsSearching);
    pToolbarInclude->EnableButton(IDC_BUTTON_ICON_DIRECTORY_UPDATE,!bIsSearching);
    pToolbarInclude->EnableButton(IDC_BUTTON_ICON_FILENAME_UPDATE,!bIsSearching);
    pToolbarInclude->EnableButton(IDC_BUTTON_DIRECTORY_FILENAME_MIXED_MODE,!bIsSearching);
    pToolbarInclude->EnableButton(IDC_BUTTON_APPLY_CHANGES_TO_READONLY,!bIsSearching);

    pToolbarFolders->EnableButton(IDC_BUTTON_CUSTOM_FOLDER,!bIsSearching);
    pToolbarFolders->EnableButton(IDC_BUTTON_CURRENT_USER_START_MENU,!bIsSearching);
    pToolbarFolders->EnableButton(IDC_BUTTON_COMMON_START_MENU,!bIsSearching);
    pToolbarFolders->EnableButton(IDC_BUTTON_CURRENT_USER_DESKTOP,!bIsSearching);
    pToolbarFolders->EnableButton(IDC_BUTTON_COMMON_DESKTOP,!bIsSearching);
    pToolbarFolders->EnableButton(IDC_BUTTON_INCLUDE_SUBDIR,!bIsSearching);
    

    BOOL bArgsOnlyChecked = ((pToolbarInclude->GetButtonState(IDC_BUTTON_REPLACE_ONLY_ARGUMENTS)&TBSTATE_CHECKED)!=0);
    SwitchUserInterfaceAccordingToReplaceOnlyParams(bArgsOnlyChecked);
}

//-----------------------------------------------------------------------------
// Name: CopyShortcutDirectoryIntoSearchField
// Object: Copy Shortcut Directory Into Search Field
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CopyShortcutDirectoryIntoSearchField()
{
    int Index=pListView->GetSelectedIndex();
    if (Index<0)
    {
        ReportError(_T("No item selected"));
        return;
    }
    CShortcutInfos* pShortcutInfos;
    pListView->GetItemUserData(Index,(LPVOID*)&pShortcutInfos);
    if (!pShortcutInfos)
        return;
    
    // if target working directory not always set --> prefer using real directory
    // ::SetDlgItemText(mhWndDialog,IDC_EDIT_SEARCH,pShortcutInfos->TargetWorkingDir);
    TCHAR RealTargetDirectory[SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH];
    CStdFileOperations::GetFileDirectory(pShortcutInfos->TargetPath,RealTargetDirectory,SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH);
    ::SetDlgItemText(mhWndDialog,IDC_EDIT_SEARCH,RealTargetDirectory);
}

//-----------------------------------------------------------------------------
// Name: OpenShortcutLocation
// Object: Open Shortcut Location
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void OpenShortcutLocation()
{
    int Index=pListView->GetSelectedIndex();
    if (Index<0)
    {
        ReportError(_T("No item selected"));
        return;
    }
    CShortcutInfos* pShortcutInfos;
    pListView->GetItemUserData(Index,(LPVOID*)&pShortcutInfos);
    if (!pShortcutInfos)
        return;   
         
    // TCHAR Path[SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH];
    // CStdFileOperations::GetFileDirectory(pShortcutInfos->LinkPath,Path,SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH);
    // ::ShellExecute(NULL, _T("explore"), Path, NULL, NULL, SW_SHOWNORMAL);
    LPITEMIDLIST pidl = ::ILCreateFromPath(pShortcutInfos->LinkPath);
    if(pidl)
    {
        ::SHOpenFolderAndSelectItems(pidl,0,0,0);
        ::ILFree(pidl);
    }
}

//-----------------------------------------------------------------------------
// Name: ShowProperties
// Object: show file properties
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void ShowProperties()
{
    int Index=pListView->GetSelectedIndex();
    if (Index<0)
    {
        ReportError(_T("No item selected"));
        return;
    }
    ShowProperties(Index);
}
//-----------------------------------------------------------------------------
// Name: ShowProperties
// Object: show file properties
// Parameters :
//     in  : int Index index in listview
//     out :
//     return : 
//-----------------------------------------------------------------------------
void ShowProperties(int Index)
{
    CShortcutInfos* pShortcutInfos;
    pListView->GetItemUserData(Index,(LPVOID*)&pShortcutInfos);
    if (!pShortcutInfos)
        return;
    ShowProperties(pShortcutInfos->LinkPath);
}

//-----------------------------------------------------------------------------
// Name: ShowProperties
// Object: show file properties
// Parameters :
//     in  : TCHAR* FilePath : file path
//     out :
//     return : 
//-----------------------------------------------------------------------------
void ShowProperties(TCHAR* FilePath)
{
    // if (((int)ShellExecute(NULL,_T("properties"),FilePath,NULL,NULL,SW_SHOWNORMAL))<33)
    SHELLEXECUTEINFO info={0};
    info.cbSize = sizeof(info);
    info.lpVerb = _T("properties");
    info.fMask = SEE_MASK_INVOKEIDLIST;
    info.nShow = SW_SHOWNORMAL;
    info.lpFile = FilePath;
    if (!ShellExecuteEx(&info))
        ReportError(_T("Can't display properties"));
}

//-----------------------------------------------------------------------------
// Name: OnAbout
// Object: show about dlg box
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void OnAbout()
{
    ShowAboutDialog(mhInstance,mhWndDialog);
}
//-----------------------------------------------------------------------------
// Name: OnHelp
// Object: show help file
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void OnHelp()
{
    
    TCHAR sz[MAX_PATH];
    CStdFileOperations::GetAppDirectory(sz,MAX_PATH);
    _tcscat(sz,HELP_FILE);
    if (((int)ShellExecute(NULL,_T("open"),sz,NULL,NULL,SW_SHOWNORMAL))<33)
    {
        _tcscpy(sz,_T("Error opening help file "));
        _tcscat(sz,HELP_FILE);
        ReportError(sz);
    }
}

//-----------------------------------------------------------------------------
// Name: OnDonation
// Object: query to make donation
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void OnDonation()
{
    BOOL bEuroCurrency=FALSE;
    HKEY hKey;
    wchar_t pszCurrency[2];
    DWORD Size=2*sizeof(wchar_t);
    memset(pszCurrency,0,Size);
    TCHAR pszMsg[3*MAX_PATH];

    // check that HKEY_CURRENT_USER\Control Panel\International\sCurrency contains the euro symbol
    // retrieve it in unicode to be quite sure of the money symbol
    if (RegOpenKeyEx(HKEY_CURRENT_USER,_T("Control Panel\\International"),0,KEY_QUERY_VALUE,&hKey)==ERROR_SUCCESS)
    {
        // use unicode version only to make string compare
        if (RegQueryValueExW(hKey,L"sCurrency",NULL,NULL,(LPBYTE)pszCurrency,&Size)==ERROR_SUCCESS)
        {
            if (wcscmp(pszCurrency,L"€")==0)
                bEuroCurrency=TRUE;
        }
        // close open key
        RegCloseKey(hKey);
    }
    // yes, you can do it if u don't like freeware and open sources soft
    // but if you make it, don't blame me for not releasing sources anymore
    _tcscpy(pszMsg,_T("https://www.paypal.com/cgi-bin/webscr?cmd=_xclick&business=jacquelin.potier@free.fr")
        _T("&currency_code=USD&lc=EN&country=US")
        _T("&item_name=Donation%20for%20ShortcutsSearchAndReplace&return=http://jacquelin.potier.free.fr/ShortcutsSearchAndReplace/"));
    // in case of euro monetary symbol
    if (bEuroCurrency)
        // add it to link
        _tcscat(pszMsg,_T("&currency_code=EUR"));

    // open donation web page
    if (((int)ShellExecute(NULL,_T("open"),pszMsg,NULL,NULL,SW_SHOWNORMAL))<33)
        // display error msg in case of failure
        ReportError(_T("Error Opening default browser. You can make a donation going to ")
                    _T("http://jacquelin.potier.free.fr")
                    );
}

//-----------------------------------------------------------------------------
// Name: OnCheckForUpdate
// Object: check for software update
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void OnCheckForUpdate()
{
    CSoftwareUpdate::CheckForUpdate(ShortcutsSearchAndReplace_PAD_Url,ShortcutsSearchAndReplaceVersionSignificantDigits,ShortcutsSearchAndReplaceDownloadLink);
}
