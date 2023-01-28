#include "Options.h"
#include <stdio.h>

#ifndef _countof
#define _countof(_Array) (sizeof(_Array) / sizeof(_Array[0]))
#endif

#define COptions_DefaultWith 750
#define COptions_DefaultHeight 460

FORCEINLINE DWORD GetDefaultColumnSize(DWORD ColumnIndex)
{
    switch (ColumnIndex)
    {
    case 0:
    default:
        return 200;
    case 1:
    case 2:
    case 6:
        return 250;
    case 3:
    case 4:
    case 5:
        return 100;
    }
}

COptions::COptions(void)
{
    // set default options
    this->RemindPosition = FALSE;
    this->XPos = 0;
    this->YPos = 0;
    this->Width = COptions_DefaultWith;
    this->Height = COptions_DefaultHeight;

    // default options are inside ini loading
    this->SearchInsideTargetDescription = FALSE;
    this->SearchInsideArguments=FALSE;
    this->SearchInsideDirectory=FALSE;
    this->SearchInsideFileName=FALSE;
    this->SearchInsideIconDirectory=FALSE;
    this->SearchInsideIconFileName=FALSE;
    this->IncludeSubdirectories = FALSE;
    this->IncludeCustomFolder = FALSE;
    this->IncludeUserStartupDirectory = FALSE;
    this->IncludeCommonStartupDirectory = FALSE;
    this->IncludeUserDesktopDirectory = FALSE;
    this->IncludeCommonDesktopDirectory = FALSE;
    this->DisplaySuccessMessageInfosAfterActionCompleted = FALSE;
    this->SearchFullPathMixedMode = FALSE;
    this->ApplyChangesToReadOnlyShortcuts = FALSE;
    this->ReplaceOnlyArguments=FALSE;
    this->ExcludeNetworkPaths=FALSE;
    this->ExcludeUnpluggedDrives=FALSE;

    // clear history string
    DWORD Cnt;
    for (Cnt = 0; Cnt<COption_HistoryDepth; Cnt++)
    {
        this->SearchHistory[Cnt][0] = 0;
        this->ReplaceHistory[Cnt][0] = 0;
        this->PathHistory[Cnt] = _tcsdup(_T(""));// assume PathHistory is not null avoid crash if no ini file
    }
    COLUMN_ORDER_INFOS* pColumnsInfos;
    for (Cnt=0;Cnt<SEARCH_AND_REPLACE_LISTVIEW_NB_COLUMNS;Cnt++)
    {
        pColumnsInfos = &this->ListviewColumnsOrderInfos[Cnt];
        pColumnsInfos->Order = Cnt;
        pColumnsInfos->Size = GetDefaultColumnSize(Cnt);
        pColumnsInfos->Visible = TRUE;
    }

    // create inifile object
    CStdFileOperations::GetAppDirectory(this->FileName,MAX_PATH);
    _tcscat(this->FileName,COption_FILENAME);
    this->pIniFile=new CIniFile(this->FileName);
    this->pIniFile->SetCurrentSectionName(_T("ShortcutsSearchAndReplace"));
}

COptions::~COptions(void)
{
    this->FreeMemory();
    delete this->pIniFile;
}

BOOL COptions::FreeMemory()
{
    for (SIZE_T Cnt = 0; Cnt<COption_HistoryDepth; Cnt++)
    {
        if ( this->PathHistory[Cnt] )
        {
            free(this->PathHistory[Cnt]);
            this->PathHistory[Cnt] = 0;
        }
    }
    return TRUE;
}

BOOL COptions::Load()
{
    if (!CStdFileOperations::DoesFileExists(this->FileName))
        return FALSE;

    this->FreeMemory();

    BOOL bRet = TRUE;
    BOOL bCurrentRet;
    UINT32 Cnt;
    TCHAR KeyName[256];

    bCurrentRet = this->pIniFile->Get(COption_REMINDS_DIALOG_POSITION,0,(DWORD*)&this->RemindPosition);
    bRet = bRet && bCurrentRet;
    bCurrentRet = this->pIniFile->Get(COption_POS_X,0,(DWORD*)&this->XPos);
    bRet = bRet && bCurrentRet;
    bCurrentRet = this->pIniFile->Get(COption_POS_Y,0,(DWORD*)&this->YPos);
    bRet = bRet && bCurrentRet;
    bCurrentRet = this->pIniFile->Get(COption_WIDTH,COptions_DefaultWith,(DWORD*)&this->Width);
    bRet = bRet && bCurrentRet;
    bCurrentRet = this->pIniFile->Get(COption_HEIGHT,COptions_DefaultHeight,(DWORD*)&this->Height);
    bRet = bRet && bCurrentRet;
    bCurrentRet = this->pIniFile->Get(COption_SEARCH_INSIDE_TARGET_DESCRIPTION,FALSE,(DWORD*)&this->SearchInsideTargetDescription);
    bRet = bRet && bCurrentRet;
    bCurrentRet = this->pIniFile->Get(COption_SEARCH_INSIDE_TARGET_ARGUMENTS,FALSE,(DWORD*)&this->SearchInsideArguments);
    bRet = bRet && bCurrentRet;
    bCurrentRet = this->pIniFile->Get(COption_SEARCH_INSIDE_TARGET_DIRECTORY,TRUE,(DWORD*)&this->SearchInsideDirectory);
    bRet = bRet && bCurrentRet;
    bCurrentRet = this->pIniFile->Get(COption_SEARCH_INSIDE_TARGET_FILENAME,FALSE,(DWORD*)&this->SearchInsideFileName);
    bRet = bRet && bCurrentRet;
    bCurrentRet = this->pIniFile->Get(COption_SEARCH_INSIDE_TARGET_ICON_DIRECTORY,TRUE,(DWORD*)&this->SearchInsideIconDirectory);
    bRet = bRet && bCurrentRet;
    bCurrentRet = this->pIniFile->Get(COption_SEARCH_INSIDE_TARGET_ICON_FILENAME,FALSE,(DWORD*)&this->SearchInsideIconFileName);
    bRet = bRet && bCurrentRet;
    bCurrentRet = this->pIniFile->Get(COption_SEARCH_FULL_PATH_MIXED_MODE,FALSE,(DWORD*)&this->SearchFullPathMixedMode);
    bRet = bRet && bCurrentRet;
    bCurrentRet = this->pIniFile->Get(COption_INCLUDE_SUBDIRECTORIES,TRUE,(DWORD*)&this->IncludeSubdirectories);
    bRet = bRet && bCurrentRet;
    bCurrentRet = this->pIniFile->Get(COption_INCLUDE_CUSTOM_FOLDER,TRUE,(DWORD*)&this->IncludeCustomFolder);
    bRet = bRet && bCurrentRet;
    bCurrentRet = this->pIniFile->Get(COption_INCLUDE_USER_STARTUP_DIRECTORY,FALSE,(DWORD*)&this->IncludeUserStartupDirectory);
    bRet = bRet && bCurrentRet;
    bCurrentRet = this->pIniFile->Get(COption_INCLUDE_COMMON_STARTUP_DIRECTORY,FALSE,(DWORD*)&this->IncludeCommonStartupDirectory);
    bRet = bRet && bCurrentRet;
    bCurrentRet = this->pIniFile->Get(COption_INCLUDE_USER_DESKTOP_DIRECTORY,FALSE,(DWORD*)&this->IncludeUserDesktopDirectory);
    bRet = bRet && bCurrentRet;
    bCurrentRet = this->pIniFile->Get(COption_INCLUDE_COMMON_DESKTOP_DIRECTORY,FALSE,(DWORD*)&this->IncludeCommonDesktopDirectory);
    bRet = bRet && bCurrentRet;
    bCurrentRet = this->pIniFile->Get(COption_EXCLUDE_NETWORK_PATHS ,FALSE,(DWORD*)&this->ExcludeNetworkPaths);
    bRet = bRet && bCurrentRet;
    bCurrentRet = this->pIniFile->Get(COption_EXCLUDE_UNPLUGGED_DRIVES,FALSE,(DWORD*)&this->ExcludeUnpluggedDrives);
    bRet = bRet && bCurrentRet;
    bCurrentRet = this->pIniFile->Get(COption_DISPLAY_SUCCESS_MESSAGE_INFOS_AFTER_ACTION_COMPLETED,TRUE,(DWORD*)&this->DisplaySuccessMessageInfosAfterActionCompleted);
    bRet = bRet && bCurrentRet;
    bCurrentRet = this->pIniFile->Get(COption_APPLY_CHANGES_TO_READ_ONLY_SHORTCUTS,FALSE,(DWORD*)&this->ApplyChangesToReadOnlyShortcuts);
    bRet = bRet && bCurrentRet;
    bCurrentRet = this->pIniFile->Get(COption_REPLACE_ONLY_ARGUMENTS,FALSE,(DWORD*)&this->ReplaceOnlyArguments);
    bRet = bRet && bCurrentRet;
    

    // read columns information
    COLUMN_ORDER_INFOS* pColumnsInfos;
    DWORD DefaultColumnSize;
    for (Cnt=0;Cnt<SEARCH_AND_REPLACE_LISTVIEW_NB_COLUMNS;Cnt++)
    {

        _itot(Cnt,KeyName,10);
        DefaultColumnSize = GetDefaultColumnSize(Cnt);
        pColumnsInfos = &this->ListviewColumnsOrderInfos[Cnt];
        pColumnsInfos->Visible =(BOOL)::GetPrivateProfileInt(
                                                            COption_SECTION_NAME_VISIBLE_COLUMNS,
                                                            KeyName,
                                                            TRUE,// default Visible
                                                            this->FileName
                                                            );
        pColumnsInfos->Order =::GetPrivateProfileInt(
                                                    COption_SECTION_NAME_COLUMNS_INDEX,
                                                    KeyName,
                                                    Cnt,// default index = id
                                                    this->FileName
                                                    );
        pColumnsInfos->Size =::GetPrivateProfileInt(
                                                    COption_SECTION_NAME_COLUMNS_SIZE,
                                                    KeyName,
                                                    DefaultColumnSize,// default
                                                    this->FileName
                                                    );
    }


    // msdn : maximum string size for these *ProfileString() functions is 2 bytes less than 32KB
    #define MAX_PROFILESTRING 0x8000
    TCHAR* TmpPath=new TCHAR[MAX_PROFILESTRING];

    for (Cnt = 0; Cnt<COption_HistoryDepth; Cnt++)
    {
        _sntprintf(KeyName,_countof(KeyName),COption_SEARCH_HISTORY _T("_%u"),Cnt);
        bCurrentRet = this->pIniFile->Get(KeyName,_T(""),this->SearchHistory[Cnt],MAX_PATH);
        bRet = bRet && bCurrentRet;

        _sntprintf(KeyName,_countof(KeyName),COption_REPLACE_HISTORY _T("_%u"),Cnt);
        bCurrentRet = this->pIniFile->Get(KeyName,_T(""),this->ReplaceHistory[Cnt],MAX_PATH);
        bRet = bRet && bCurrentRet;
                          
        _sntprintf(KeyName,_countof(KeyName),COption_PATH_HISTORY _T("_%u"),Cnt);
        *TmpPath=0;
        bCurrentRet = this->pIniFile->Get(KeyName,_T(""),TmpPath,MAX_PROFILESTRING);
        bRet = bRet && bCurrentRet;
        this->PathHistory[Cnt]=_tcsdup(TmpPath);
    } 

    delete[] TmpPath;

    return bRet;
}
BOOL COptions::Save()
{
    BOOL bRet = TRUE;
    BOOL bCurrentRet;
    UINT32 Cnt;
    TCHAR KeyName[256];
    TCHAR szValue[128];

    bCurrentRet = this->pIniFile->Set(COption_REMINDS_DIALOG_POSITION,(DWORD)this->RemindPosition);
    bRet = bRet && bCurrentRet;
    bCurrentRet = this->pIniFile->Set(COption_POS_X,this->XPos);
    bRet = bRet && bCurrentRet;
    bCurrentRet = this->pIniFile->Set(COption_POS_Y,this->YPos);
    bRet = bRet && bCurrentRet;
    bCurrentRet = this->pIniFile->Set(COption_WIDTH,this->Width);
    bRet = bRet && bCurrentRet;
    bCurrentRet = this->pIniFile->Set(COption_HEIGHT,this->Height);
    bRet = bRet && bCurrentRet;
    bCurrentRet = this->pIniFile->Set(COption_SEARCH_INSIDE_TARGET_DESCRIPTION,(DWORD)this->SearchInsideTargetDescription);
    bRet = bRet && bCurrentRet;
    bCurrentRet = this->pIniFile->Set(COption_SEARCH_INSIDE_TARGET_ARGUMENTS,(DWORD)this->SearchInsideArguments);
    bRet = bRet && bCurrentRet;
    bCurrentRet = this->pIniFile->Set(COption_SEARCH_INSIDE_TARGET_DIRECTORY,(DWORD)this->SearchInsideDirectory);
    bRet = bRet && bCurrentRet;
    bCurrentRet = this->pIniFile->Set(COption_SEARCH_INSIDE_TARGET_FILENAME,(DWORD)this->SearchInsideFileName);
    bRet = bRet && bCurrentRet;
    bCurrentRet = this->pIniFile->Set(COption_SEARCH_INSIDE_TARGET_ICON_DIRECTORY,(DWORD)this->SearchInsideIconDirectory);
    bRet = bRet && bCurrentRet;
    bCurrentRet = this->pIniFile->Set(COption_SEARCH_INSIDE_TARGET_ICON_FILENAME,(DWORD)this->SearchInsideIconFileName);
    bRet = bRet && bCurrentRet;
    bCurrentRet = this->pIniFile->Set(COption_SEARCH_FULL_PATH_MIXED_MODE,(DWORD)this->SearchFullPathMixedMode);
    bRet = bRet && bCurrentRet;
    bCurrentRet = this->pIniFile->Set(COption_INCLUDE_SUBDIRECTORIES,(DWORD)this->IncludeSubdirectories);
    bRet = bRet && bCurrentRet;
    bCurrentRet = this->pIniFile->Set(COption_INCLUDE_CUSTOM_FOLDER,(DWORD)this->IncludeCustomFolder);
    bRet = bRet && bCurrentRet;
    bCurrentRet = this->pIniFile->Set(COption_INCLUDE_USER_STARTUP_DIRECTORY,(DWORD)this->IncludeUserStartupDirectory);
    bRet = bRet && bCurrentRet;
    bCurrentRet = this->pIniFile->Set(COption_INCLUDE_COMMON_STARTUP_DIRECTORY,(DWORD)this->IncludeCommonStartupDirectory);
    bRet = bRet && bCurrentRet;
    bCurrentRet = this->pIniFile->Set(COption_INCLUDE_USER_DESKTOP_DIRECTORY,(DWORD)this->IncludeUserDesktopDirectory);
    bRet = bRet && bCurrentRet;
    bCurrentRet = this->pIniFile->Set(COption_INCLUDE_COMMON_DESKTOP_DIRECTORY,(DWORD)this->IncludeCommonDesktopDirectory);
    bRet = bRet && bCurrentRet;
    bCurrentRet = this->pIniFile->Set(COption_EXCLUDE_NETWORK_PATHS,(DWORD)this->ExcludeNetworkPaths);
    bRet = bRet && bCurrentRet;
    bCurrentRet = this->pIniFile->Set(COption_EXCLUDE_UNPLUGGED_DRIVES,(DWORD)this->ExcludeUnpluggedDrives);
    bRet = bRet && bCurrentRet;
    bCurrentRet = this->pIniFile->Set(COption_DISPLAY_SUCCESS_MESSAGE_INFOS_AFTER_ACTION_COMPLETED,(DWORD)this->DisplaySuccessMessageInfosAfterActionCompleted);
    bRet = bRet && bCurrentRet;
    bCurrentRet = this->pIniFile->Set(COption_APPLY_CHANGES_TO_READ_ONLY_SHORTCUTS,(DWORD)this->ApplyChangesToReadOnlyShortcuts);
    bRet = bRet && bCurrentRet;
    bCurrentRet = this->pIniFile->Set(COption_REPLACE_ONLY_ARGUMENTS,(DWORD)this->ReplaceOnlyArguments);
    bRet = bRet && bCurrentRet;

    // write columns informations
    COLUMN_ORDER_INFOS* pColumnsInfos;
    for (Cnt=0;Cnt<SEARCH_AND_REPLACE_LISTVIEW_NB_COLUMNS;Cnt++)
    {
        _itot(Cnt,KeyName,10);
        pColumnsInfos = &this->ListviewColumnsOrderInfos[Cnt];

        _itot(pColumnsInfos->Visible,szValue,10);
        WritePrivateProfileString(
                                    COption_SECTION_NAME_VISIBLE_COLUMNS,
                                    KeyName,
                                    szValue,
                                    this->FileName
                                    );

        _itot((int)pColumnsInfos->Order,szValue,10);
        WritePrivateProfileString(
                                    COption_SECTION_NAME_COLUMNS_INDEX,
                                    KeyName,
                                    szValue,
                                    this->FileName
                                    );
        _itot((int)pColumnsInfos->Size,szValue,10);
        WritePrivateProfileString(
                                    COption_SECTION_NAME_COLUMNS_SIZE,
                                    KeyName,
                                    szValue,
                                    this->FileName
                                    );
    }


    for (Cnt = 0; Cnt<COption_HistoryDepth; Cnt++)
    {
        _sntprintf(KeyName,_countof(KeyName),COption_SEARCH_HISTORY _T("_%u"),Cnt);
        bCurrentRet = this->pIniFile->Set(KeyName,this->SearchHistory[Cnt]);
        bRet = bRet && bCurrentRet;

        _sntprintf(KeyName,_countof(KeyName),COption_REPLACE_HISTORY _T("_%u"),Cnt);
        bCurrentRet = this->pIniFile->Set(KeyName,this->ReplaceHistory[Cnt]);
        bRet = bRet && bCurrentRet;

        _sntprintf(KeyName,_countof(KeyName),COption_PATH_HISTORY _T("_%u"),Cnt);
        bCurrentRet = this->pIniFile->Set(KeyName,this->PathHistory[Cnt]);
        bRet = bRet && bCurrentRet;
    }  

    return bRet;
}