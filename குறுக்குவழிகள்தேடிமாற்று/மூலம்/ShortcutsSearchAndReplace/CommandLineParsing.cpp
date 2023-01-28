#include "CommandLineParsing.h"
#include "../Tools/String/AnsiUnicodeConvert.h"
#include "../Tools/Console/Win32Console.h"
#include "../Tools/Exception/GenericTryCatchWithExceptionReport.h"

#define BASE_EXE_NAME TEXT("ShortcutsSearchAndReplace")

#ifdef _WIN64
    #define EXE_FILE_NAME    BASE_EXE_NAME TEXT("64.exe")
#else
    #define EXE_FILE_NAME    BASE_EXE_NAME TEXT(".exe")
#endif

#ifndef DefineWCharStringLength
    #define DefineWCharStringLengthWithEndingZero(x) (sizeof(x)/sizeof(WCHAR))
    #define DefineWCharStringLength(x) (sizeof(x)/sizeof(WCHAR)-1)
#endif

#define WCharAndSize(x) x,DefineWCharStringLength(x)

void WriteConsoleAndAssumeOutput(CWin32Console* pConsole,WCHAR* Msg)
{
    if (!pConsole->Write(Msg))
    {
        ::MessageBoxW(NULL,Msg,TEXT("Error"),MB_ICONERROR);
    }
}

void ShowCommandLineHelp(CWin32Console* pConsole = NULL)
{
    TCHAR* Msg = 
                TEXT("Shortcuts Search And Replace Command Line Parameters \r\n")
                TEXT(" --- Operations --- \r\n")
                TEXT(" Op=Search/SearchDead/Replace : Operation to do\r\n")
                TEXT(" SearchPostOp=Resolve/Delete  : Post search operation. Applies to Search or SearchDead Operation \r\n")
                TEXT(" Search=\"YourSearchedMotif\" : Motif to search\r\n")
                TEXT(" Replace=\"YourReplaceMotif\" : Motif replacement\r\n")

                TEXT(" --- Directories --- \r\n")
                TEXT(" IncDir=\"YourCustomDirectory\"   : include a custom directory \r\n")
                TEXT(" IncSubDir : Include subdirectories (applies to specified custom folder, user desktop, and common desktop only) \r\n")
                TEXT(" IncUserStartup   : Include current user start menu for search and replace operations (start menu sub directories are always included) \r\n")
                TEXT(" IncCommonStartup : Include the users common start menu for search and replace operations (start menu sub directories are always included) \r\n")
                TEXT(" IncUserDesktop   : Include the current user desktop for search and replace operations. To include sub directories, use the \"IncSubDir\" option \r\n")
                TEXT(" IncCommonDesktop : Include the common desktop for search and replace operations. To include sub directories, use the \"IncSubDir\" option \r\n")

                TEXT(" --- Search & Replace Inside --- \r\n")
                TEXT(" InsideDirName  : Include shortcut target directory into search and replace operations. (Target file name not included, so you can change directory without affecting name)\r\n")
                TEXT(" InsideFileName : Include shortcut target file name into search and replace operations. (Target directory not affected)\r\n")
                TEXT(" InsideIconDirName  : Include shortcut icon directory into search and replace operations. (Target icon file name not included, so you can change icon directory without affecting icon name)\r\n")
                TEXT(" InsideIconFileName : Include shortcut icon file name into search and replace operations. (Target icon directory not affected)\r\n")
                TEXT(" InsideFullPathMix  : to use when search motif includes part of directory and filename. Instead of splitting directory and file name for search and replace, do search and replace operations on full path. To use in addition of InsideDirName,InsideFileName, InsideIconDirName,InsideIconFileName \r\n")
                TEXT(" InsideArgs : Include shortcut arguments into search and replace operations\r\n")
                TEXT(" InsideDescription : Include comments in search and replace operations \r\n")

                TEXT(" --- Extra Options --- \r\n")
                TEXT(" ChangeReadOnly : apply change to shortcuts in read only mode \r\n")
                TEXT(" ArgsOnly : Search and replace is done for shortcuts arguments \r\n")
                TEXT(" ExNetDrv: Exclude shortcuts with network path (SearchDead links operation only)\r\n")
                TEXT(" ExUnpluggedDrv : Exclude shortcuts with references to unplugged drives (SearchDead links operation only)\r\n")
                TEXT(" DeleteWithNoPrompt : Applies to Delete operation, delete found shortcuts without prompt (use at your own risk)\r\n");
            

    //// console is not attached (see WinMain ParseCommandLine is called before AttachConsole) --> use messagebox / Attach console
    //::MessageBox(NULL,Msg,TEXT("Shortcuts Search And Replace Command Line Help"),MB_OK|MB_ICONINFORMATION | MB_SYSTEMMODAL);
    if (pConsole)
    {
        WriteConsoleAndAssumeOutput(pConsole,Msg);
    }
    else
    {
        CWin32Console Console;
        WriteConsoleAndAssumeOutput(&Console,Msg);
    }

}

void ReportCmdLineError(WCHAR* Msg)
{
    CWin32Console Console;
    WriteConsoleAndAssumeOutput(&Console,Msg);
    ShowCommandLineHelp(&Console);
}

// CommandLineToArgvW sucks with " and \" -->use our own function as file name and file path can't have " inside name
// here we know that motif can't contain character " inside there content, so it is easier to parse
// this function can't be used in other conditions
WCHAR** MyCommandLineToArgvW(IN WCHAR* lpCmdLine, OUT int* pNumArgs)
{
    if (lpCmdLine == NULL)
    {
        *pNumArgs = 0;
        return NULL;
    }
    size_t StringLen = wcslen(lpCmdLine);
    SIZE_T MaxNbParams = 0;
    WCHAR* pWchar = lpCmdLine;
    WCHAR* pLastWchar = pWchar+StringLen;

    // remove begin spacers if any
    while (*lpCmdLine == ' ')
        lpCmdLine++;

    // count number of space to know the max number of params
    for (pWchar=lpCmdLine; pWchar<pLastWchar;pWchar++)
    {
        if (*pWchar ==' ')
        {
            // if multiple space, take only 1 into account
            if (*(pWchar+1) ==' ')
                continue;

            // ignore space at the end of cmd line
            if (*(pWchar+1) =='\0')
                break;

            MaxNbParams++;
        }
    }
    MaxNbParams++; // nb params = nb space +1

    // alloc a single buffer for pointers + content
    // layout : WCHAR* pArg1, WCHAR* pArg2, WCHAR* pArgxxx, String with param spliters replaced with \0
    PBYTE Buffer;
    Buffer = new BYTE[MaxNbParams*sizeof(WCHAR*)+(StringLen+1)*sizeof(WCHAR)];
    WCHAR* pArgsStart;
    pArgsStart = (WCHAR*)&Buffer[MaxNbParams*sizeof(WCHAR*)];
    wcscpy(pArgsStart,lpCmdLine);// copy command line into pArgsStart
    pLastWchar= pArgsStart+StringLen;
    BOOL InsideQuotes=FALSE;
    BOOL ArgStartsWithQuotes=FALSE;
    // we always have at least one result : program name
    int NbArgs = 1;
    WCHAR** pArgsResult = (WCHAR**)Buffer; // first arg pointer is at start of Buffer

    // program name can have space and is not enclosed by quotes
    // --> by pass program name for first arg (allow to by pass space if path is not surrounded with quotes)
    TCHAR AppPath[MAX_PATH];
    CStdFileOperations::GetAppPath(AppPath,_countof(AppPath));
    SIZE_T AppPathLen = _tcslen(AppPath);
    if (_wcsnicmp(pArgsStart,AppPath,AppPathLen)==0) // we build in unicode so no conversion to do between TCHAR and WCHAR
    {
        *pArgsResult = pArgsStart;
        NbArgs++;

        // go after app path
        pArgsStart+=AppPathLen; 
        pArgsStart++;
        *pArgsStart=0; // end param
        pArgsStart++;  // goto next param start

        // remove multiple spacers if any
        while (*pArgsStart == ' ')
            pArgsStart++;
    }

    // according to call, from command line or batch, args can be enclosed with quotes, so in this case remove them
    if (*pArgsStart == '"')
    {
        pArgsStart++;
        ArgStartsWithQuotes = TRUE;
        InsideQuotes = TRUE;
    }
    *pArgsResult = pArgsStart;// first arg start is at StrArrayBegin
    for (pWchar=pArgsStart; pWchar<pLastWchar;pWchar++)
    {
        if (*pWchar =='"')
            InsideQuotes=(InsideQuotes+1)%2;// InsideQuotes = InsideQuotes?FALSE:TRUE;
        
        if (*pWchar ==' ')
        {
            if (InsideQuotes)
                continue;

            *pWchar = 0; // replace space by '\0' to end previous arg
            if (ArgStartsWithQuotes && (*(pWchar-1)=='"')) // ArgStartsWithQuotes assume pWchar>(pArgsStart+1)
                *(pWchar-1)=0;

            // go to next char
            pWchar++;

            // remove multiple spacers if any
            while (*pWchar == ' ')
                pWchar++;

            // stop at the end of cmd line
            if (*pWchar =='\0')
                break;

            // increase arg number
            NbArgs++;
            pArgsResult++;

            // check next parameter start
            if (*pWchar=='"')
            {
                ArgStartsWithQuotes = TRUE;
                InsideQuotes = TRUE;
                pWchar++;
                *pArgsResult = pWchar;
            }
            else
            {
                ArgStartsWithQuotes = FALSE;
                *pArgsResult = pWchar;
            }
        }
    }
    *pNumArgs = NbArgs;
    return (WCHAR**)Buffer;
}

//-----------------------------------------------------------------------------
// Name: ParseCommandLine
// Object: parse cmd line
// Parameters :
//     BOOL* pbCommandLineMode : TRUE if command line has parameters
//     return : TRUE on SUCCESS, FALSE on error
//-----------------------------------------------------------------------------
BOOL ParseCommandLine(OUT BOOL* pbCommandLineMode,OUT CCommandLineOptions* pCommandLineOptions,OUT SEARCH_INFOS* pSearchInfos, OUT COptions* pOptions)
{
    BOOL bRes=TRUE;
    LPWSTR* lppwargv;
    int argc=0;
    int cnt;
    WCHAR ErrorMsg[256];

    *pbCommandLineMode = FALSE;

    CGenericTryCatchWithExceptionReport_TRY

    WCHAR* wCommandLine=GetCommandLineW();

    // MessageBoxW(0,wCommandLine,0,0);

    // CommandLineToArgvW sucks with " and \" -->use our own function as file name and file path can't have " inside name
    // use CommandLineToArgvW
    // Notice : CommandLineToArgvW translate 'option="a";"b"' into 'option=a;b'
    //lppwargv=CommandLineToArgvW(
    //                            wCommandLine,
    //                            &argc
    //                            );
    lppwargv = MyCommandLineToArgvW(
                                    wCommandLine,
                                    &argc
                                    );
    if (lppwargv == NULL)
        return TRUE;

    // if no params
    if (argc<=1) // argc == 1 --> only app name aka no args
    {
        // LocalFree(lppwargv);
        delete[] lppwargv;
        return TRUE;
    }

    ///////////////////////////////
    // we are in command line mode
    ///////////////////////////////
    *pbCommandLineMode=TRUE;


    ///////////////////////////////
    // for debugging from console (Allow To attach)
    ///////////////////////////////
//#ifdef _DEBUG
//    ::MessageBox(NULL,TEXT("You can attach process before command line parsing"),TEXT("Ready to be debugged"),MB_OK | MB_ICONINFORMATION);
//#endif

    ///////////////////////////////
    // options with no cmd line 
    ///////////////////////////////
    pOptions->DisplaySuccessMessageInfosAfterActionCompleted = TRUE;

    ///////////////////////////////
    // for each param
    ///////////////////////////////
    for(cnt=0;cnt<argc;cnt++)
    {
        // depending the way CreateProcess is called, param 0 can be the name of the application or not
        if (cnt == 0)
        {
            WCHAR AppPathW[MAX_PATH];
            ::GetModuleFileNameW(NULL,AppPathW,_countof(AppPathW));
            // case of first arg is full path
            if (_wcsicmp(lppwargv[0],AppPathW)==0)
                continue;
            // case of first arg is short path
            if (_wcsicmp(lppwargv[0],CStdFileOperations::GetFileName(AppPathW))==0)
                continue;

        }
        if ( (_wcsicmp(lppwargv[cnt],L"--help")==0)
            || (_wcsicmp(lppwargv[cnt],L"/Help")==0)
            || (_wcsicmp(lppwargv[cnt],L"Help")==0)
            || (_wcsicmp(lppwargv[cnt],L"-H")==0)
            || (_wcsicmp(lppwargv[cnt],L"-?")==0)
            || (_wcsicmp(lppwargv[cnt],L"/H")==0)
            || (_wcsicmp(lppwargv[cnt],L"/?")==0)
            || (_wcsicmp(lppwargv[cnt],L"?")==0)
            )
        {
            // show help message
            ShowCommandLineHelp();
            return FALSE;
        }

        /////////////////
        // operations
        /////////////////
        else if ( _wcsnicmp( lppwargv[cnt],WCharAndSize(L"Op=") )==0)
        {
            WCHAR* wc = lppwargv[cnt]+DefineWCharStringLength(L"Op=");
            if (_wcsicmp(wc,L"Search")==0)
                pSearchInfos->SearchWay = SearchWay_SEARCH;
            else if (_wcsicmp(wc,L"SearchDead")==0)
                pSearchInfos->SearchWay = SearchWay_SEARCH_DEAD_LINKS;
            else if (_wcsicmp(wc,L"Replace")==0)
                pSearchInfos->SearchWay = SearchWay_SEARCH_AND_REPLACE;
            else
            {
                _snwprintf(ErrorMsg,_countof(ErrorMsg),TEXT("Unknown operation %s for op="),wc);
                ErrorMsg[_countof(ErrorMsg)-1]=0;
                ReportCmdLineError(ErrorMsg);
                return FALSE;
            }
        }
        else if ( _wcsnicmp( lppwargv[cnt],WCharAndSize(L"SearchPostOp=") )==0)
        {
            WCHAR* wc = lppwargv[cnt]+DefineWCharStringLength(L"SearchPostOp=");
            if (_wcsicmp(wc,L"Resolve")==0)
                pCommandLineOptions->PostOperation = PostOperation_RESOLVE;
            else if (_wcsicmp(wc,L"Delete")==0)
                pCommandLineOptions->PostOperation = PostOperation_DELETE;
            else
            {
                _snwprintf(ErrorMsg,_countof(ErrorMsg),TEXT("Unknown post operation %s for SearchPostOp="),wc);
                ErrorMsg[_countof(ErrorMsg)-1]=0;
                ReportCmdLineError(ErrorMsg);
                return FALSE;
            }
        }
        else if ( _wcsnicmp( lppwargv[cnt],WCharAndSize(L"Search=") )==0)
        {
            WCHAR* wc = lppwargv[cnt]+DefineWCharStringLength(L"Search=");

            // remove first '"' if any
            if (*wc=='\"')
                wc++;
            // remove last '"' if any
            size_t len = wcslen(wc);
            if (len>0)
            {
                if (wc[len-1]=='\"')
                    wc[len-1]=0;
            }

            CAnsiUnicodeConvert::UnicodeToTchar(wc,&pSearchInfos->pszSearchStringUpper);
            _tcsupr(pSearchInfos->pszSearchStringUpper);
        }
        else if ( _wcsnicmp( lppwargv[cnt],WCharAndSize(L"Replace=") )==0)
        {
            WCHAR* wc = lppwargv[cnt]+DefineWCharStringLength(L"Replace=");

            // remove first '"' if any
            if (*wc=='\"')
                wc++;
            // remove last '"' if any
            size_t len = wcslen(wc);
            if (len>0)
            {
                if (wc[len-1]=='\"')
                    wc[len-1]=0;
            }

            CAnsiUnicodeConvert::UnicodeToTchar(wc,&pSearchInfos->pszReplaceString);
            _tcsupr(pSearchInfos->pszReplaceString);
        }

        /////////////////
        // include
        /////////////////
        else if ( _wcsnicmp( lppwargv[cnt],WCharAndSize(L"IncDir=") )==0)
        {
            WCHAR* wc = lppwargv[cnt]+DefineWCharStringLength(L"IncDir=");

            // remove first '"' if any
            if (*wc=='\"')
                wc++;
            // remove last '"' if any
            size_t len = wcslen(wc);
            if (len>0)
            {
                if (wc[len-1]=='\"')
                    wc[len-1]=0;
            }

            CAnsiUnicodeConvert::UnicodeToTchar(wc,pSearchInfos->CustomFolderPaths,SHORTCUT_SEARCH_AND_REPLACE_MAX_PATH);
            pOptions->IncludeCustomFolder = TRUE;
        }

        else if (_wcsicmp(lppwargv[cnt],L"IncSubDir")==0)
            pOptions->IncludeSubdirectories = TRUE;

        else if (_wcsicmp(lppwargv[cnt],L"IncUserStartup")==0)
            pOptions->IncludeUserStartupDirectory = TRUE;

        else if (_wcsicmp(lppwargv[cnt],L"IncCommonStartup")==0)
            pOptions->IncludeCommonStartupDirectory = TRUE;

        else if (_wcsicmp(lppwargv[cnt],L"IncUserDesktop")==0)
            pOptions->IncludeUserDesktopDirectory = TRUE;

        else if (_wcsicmp(lppwargv[cnt],L"IncCommonDesktop")==0)
            pOptions->IncludeCommonDesktopDirectory = TRUE;

        /////////////////
        // inside
        /////////////////
        else if (_wcsicmp(lppwargv[cnt],L"InsideDirName")==0)
            pOptions->SearchInsideDirectory = TRUE;
        else if (_wcsicmp(lppwargv[cnt],L"InsideFileName")==0)
            pOptions->SearchInsideFileName = TRUE;

        else if (_wcsicmp(lppwargv[cnt],L"InsideIconDirName")==0)
            pOptions->SearchInsideIconDirectory = TRUE;
        else if (_wcsicmp(lppwargv[cnt],L"InsideIconFileName")==0)
            pOptions->SearchInsideIconFileName = TRUE;

        else if (_wcsicmp(lppwargv[cnt],L"InsideFullPathMix")==0)
            pOptions->SearchFullPathMixedMode = TRUE;

        else if (_wcsicmp(lppwargv[cnt],L"InsideArgs")==0)
            pOptions->SearchInsideArguments = TRUE;
        else if (_wcsicmp(lppwargv[cnt],L"InsideDescription")==0)
            pOptions->SearchInsideTargetDescription = TRUE;

        /////////////////
        // extra
        /////////////////
        else if (_wcsicmp(lppwargv[cnt],L"ExNetDrv")==0)
            pOptions->ExcludeNetworkPaths = TRUE;

        else if (_wcsicmp(lppwargv[cnt],L"ExUnpluggedDrv")==0)
            pOptions->ExcludeUnpluggedDrives = TRUE;

        else if (_wcsicmp(lppwargv[cnt],L"ChangeReadOnly")==0)
            pOptions->ApplyChangesToReadOnlyShortcuts = TRUE;
        
        else if (_wcsicmp(lppwargv[cnt],L"ArgsOnly")==0)
            pOptions->ReplaceOnlyArguments = TRUE;

        else if (_wcsicmp(lppwargv[cnt],L"DeleteWithNoPrompt")==0)
            pCommandLineOptions->DeleteWithNoPrompt = TRUE;
   
        else
        {
            _snwprintf(ErrorMsg,_countof(ErrorMsg),TEXT("Unknown option %s"),lppwargv[cnt]);
            ErrorMsg[_countof(ErrorMsg)-1]=0;
            ReportCmdLineError(ErrorMsg);
            return FALSE;
        }
    }
    // LocalFree(lppwargv);
    delete[] lppwargv;

    return bRes;

    CGenericTryCatchWithExceptionReport_CATCH_AND_RETURN_FALSE
}
