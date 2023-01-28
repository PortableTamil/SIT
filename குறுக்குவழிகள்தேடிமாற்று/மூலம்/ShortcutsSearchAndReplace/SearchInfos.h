#pragma once

#include <Windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

#include "Options.h" // for SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH

typedef enum tagSearchWay
{
    SearchWay_SEARCH=0,
    SearchWay_SEARCH_AND_REPLACE,
    SearchWay_SEARCH_DEAD_LINKS
};

typedef enum tagCmdPostOperation
{
    PostOperation_NONE,
    PostOperation_RESOLVE,
    PostOperation_DELETE
};

class CCommandLineOptions
{
public:
    BOOL DeleteWithNoPrompt;
    tagCmdPostOperation PostOperation;

    CCommandLineOptions()
    {
        this->DeleteWithNoPrompt = FALSE;
        this->PostOperation = PostOperation_NONE;
    }
};

// SEARCH_INFOS : info required during file and link parsing
typedef struct tagSearchInfos
{
    /////////////////////
    // DoSearch Inputs
    /////////////////////
    TCHAR CustomFolderPaths[SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH];
    TCHAR* pszSearchStringUpper;
    TCHAR* pszReplaceString;
    tagSearchWay SearchWay;

    /////////////////////
    // DoSearch internal vars (used by callee CallBackFileFound)
    /////////////////////

    // options that can be modified compared to COptions values
    BOOL SearchInsideTargetDescription;                     // Filled by DoSearch from pOptions
    BOOL SearchInsideArguments;                             // Filled by DoSearch from pOptions
    BOOL SearchInsideDirectory;                             // Filled by DoSearch from pOptions
    BOOL SearchInsideFileName;                              // Filled by DoSearch from pOptions
    BOOL SearchInsideIconDirectory;                         // Filled by DoSearch from pOptions
    BOOL SearchInsideIconFileName;                          // Filled by DoSearch from pOptions
    BOOL SearchFullPathMixedMode;                           // Filled by DoSearch from pOptions

    // options values resulting of previous options
    BOOL SearchInsideFullPath;                              // Filled by DoSearch
    BOOL SearchInsideIconFullPath;                          // Filled by DoSearch


    /////////////////////
    // DoSearch Outputs
    /////////////////////

    // results
    DWORD NbLinkFilesSearched;                              // Filled by DoSearch
    DWORD NbLinkFilesSearchedMatchingConditions;            // Filled by DoSearch
    DWORD NbLinkFilesSearchedForReplaceWithPathValidated;   // Filled by DoSearch

    BOOL bSearchedCanceled;
}SEARCH_INFOS,*PSEARCH_INFOS;
