#pragma once

#include <Windows.h>
#include "../Tools/Ini/IniFile.h"

#define SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH        2048
#define COption_HistoryDepth                        20
#define COption_FILENAME                            _T("ShortcutsSearchAndReplace.ini")

#define COption_REMINDS_DIALOG_POSITION             _T("RemindPosition")
#define COption_POS_X                               _T("X")
#define COption_POS_Y                               _T("Y")
#define COption_WIDTH                               _T("Width")
#define COption_HEIGHT                              _T("Height")
#define COption_SEARCH_HISTORY                      _T("SearchHistory")
#define COption_REPLACE_HISTORY                     _T("ReplaceHistory")
#define COption_PATH_HISTORY                        _T("PathHistory")
#define COption_SEARCH_INSIDE_TARGET_DESCRIPTION    _T("SearchInsideTargetDescription")
#define COption_SEARCH_INSIDE_TARGET_ARGUMENTS      _T("SearchInsideArguments")
#define COption_SEARCH_INSIDE_TARGET_DIRECTORY      _T("SearchInsideDirectory")
#define COption_SEARCH_INSIDE_TARGET_FILENAME       _T("SearchInsideFileName")
#define COption_SEARCH_INSIDE_TARGET_ICON_DIRECTORY _T("SearchInsideIconDirectory")
#define COption_SEARCH_INSIDE_TARGET_ICON_FILENAME  _T("SearchInsideIconFileName")
#define COption_SEARCH_FULL_PATH_MIXED_MODE         _T("SearchFullPathMixedMode")

#define COption_INCLUDE_SUBDIRECTORIES              _T("IncludeSubdirectories")
#define COption_INCLUDE_CUSTOM_FOLDER               _T("IncludeCustomFolder")
#define COption_INCLUDE_USER_STARTUP_DIRECTORY      _T("IncludeUserStartupDirectory")
#define COption_INCLUDE_COMMON_STARTUP_DIRECTORY    _T("IncludeCommonStartupDirectory")
#define COption_INCLUDE_USER_DESKTOP_DIRECTORY      _T("IncludeUserDesktopDirectory ")
#define COption_INCLUDE_COMMON_DESKTOP_DIRECTORY    _T("IncludeCommonDesktopDirectory ")
#define COption_EXCLUDE_NETWORK_PATHS               _T("ExcludeNetworkPaths")
#define COption_EXCLUDE_UNPLUGGED_DRIVES            _T("ExcludeUnpluggedDrives")
#define COption_DISPLAY_SUCCESS_MESSAGE_INFOS_AFTER_ACTION_COMPLETED _T("DisplaySuccessMessageInfosAfterActionCompleted")
#define COption_APPLY_CHANGES_TO_READ_ONLY_SHORTCUTS _T("ApplyChangesToReadOnlyShortcuts")
#define COption_REPLACE_ONLY_ARGUMENTS              _T("ReplaceOnlyArguments")
#define COption_SECTION_NAME_VISIBLE_COLUMNS       _T("VISIBLE_COLUMNS")
#define COption_SECTION_NAME_COLUMNS_INDEX         _T("COLUMNS_ORDER")
#define COption_SECTION_NAME_COLUMNS_SIZE          _T("COLUMNS_SIZE")


class COptions
{
private:
    TCHAR FileName[MAX_PATH];
    CIniFile* pIniFile;
    BOOL FreeMemory();
public:
    BOOL RemindPosition;
    DWORD XPos;
    DWORD YPos;
    DWORD Width;
    DWORD Height;

    TCHAR SearchHistory[COption_HistoryDepth][SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH];
    TCHAR ReplaceHistory[COption_HistoryDepth][SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH];
    TCHAR* PathHistory[COption_HistoryDepth];

    
    typedef struct tagColumnInfos // keep struct order
    {
        DWORD Order;
        BOOL  Visible;
        DWORD Size;
    }COLUMN_ORDER_INFOS;
    #define SEARCH_AND_REPLACE_LISTVIEW_NB_COLUMNS 7 // must match main.cpp
    COLUMN_ORDER_INFOS ListviewColumnsOrderInfos[SEARCH_AND_REPLACE_LISTVIEW_NB_COLUMNS];

    BOOL SearchInsideTargetDescription;
    BOOL SearchInsideArguments;
    BOOL SearchInsideDirectory;
    BOOL SearchInsideFileName;
    BOOL SearchInsideIconDirectory;
    BOOL SearchInsideIconFileName;
    BOOL SearchFullPathMixedMode;

    BOOL IncludeSubdirectories;
    BOOL IncludeCustomFolder;
    BOOL IncludeUserStartupDirectory;
    BOOL IncludeCommonStartupDirectory;
    BOOL IncludeUserDesktopDirectory;
    BOOL IncludeCommonDesktopDirectory;
    BOOL ExcludeNetworkPaths;
    BOOL ExcludeUnpluggedDrives;
    BOOL DisplaySuccessMessageInfosAfterActionCompleted;
    BOOL ApplyChangesToReadOnlyShortcuts;
    BOOL ReplaceOnlyArguments;

    COptions(void);
    ~COptions(void);

    BOOL Load();
    BOOL Save();
};
