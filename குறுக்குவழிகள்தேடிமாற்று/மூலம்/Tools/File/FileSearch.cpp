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
// Object: File Searching helper
//-----------------------------------------------------------------------------

#include "FileSearch.h"
#include "VirtualDirectory.h"


BOOL CFileSearch::IsDirectory(WIN32_FIND_DATA* pWin32FindData)
{
    return ((pWin32FindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY);
}

//-----------------------------------------------------------------------------
// Name: Search
// Object: search for files and directories. file name can contain * and ?
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
BOOL CFileSearch::Search(TCHAR* PathWithWildChar,BOOL SearchInSubDirectories,pfFileFoundCallBack FileFoundCallBack,PVOID UserParam,BOOL ReplaceVirtualDirectoriesByRealDirectories)
{
    return CFileSearch::Search(PathWithWildChar,SearchInSubDirectories,NULL,FileFoundCallBack,UserParam,ReplaceVirtualDirectoriesByRealDirectories);
}

BOOL CFileSearch::Search(TCHAR* PathWithWildChar,BOOL SearchInSubDirectories,HANDLE hCancelEvent,pfFileFoundCallBack FileFoundCallBack,PVOID UserParam,BOOL ReplaceVirtualDirectoriesByRealDirectories)
{
    BOOL SearchCanceled;
    return CFileSearch::Search(PathWithWildChar,SearchInSubDirectories,hCancelEvent,FileFoundCallBack,UserParam,&SearchCanceled,ReplaceVirtualDirectoriesByRealDirectories);
}

BOOL CFileSearch::Search(TCHAR* PathWithWildChar,BOOL SearchInSubDirectories,HANDLE hCancelEvent,pfFileFoundCallBack FileFoundCallBack,PVOID UserParam,BOOL* pSearchCanceled,BOOL ReplaceVirtualDirectoriesByRealDirectories)
{
    TCHAR* pszFileFilter;
    // split search in directory + file filter
    pszFileFilter=_tcsrchr(PathWithWildChar,'\\');
    if (!pszFileFilter)
        return TRUE;
    // point after '\'
    pszFileFilter++;

    // get directory
    SIZE_T Size=pszFileFilter-PathWithWildChar;
    TCHAR szPath[CFileSearch_MAX_PATH_SIZE];
    Size = __min(Size,CFileSearch_MAX_PATH_SIZE-1);
    _tcsncpy(szPath,PathWithWildChar,Size);
    szPath[Size-1]=0;

    return CFileSearch::SearchMultipleNames(szPath,&pszFileFilter,1,SearchInSubDirectories,hCancelEvent,FileFoundCallBack,UserParam,pSearchCanceled,ReplaceVirtualDirectoriesByRealDirectories);
}

BOOL CFileSearch::Search(TCHAR* Path,TCHAR* FileFilterWithWildChar,BOOL SearchInSubDirectories,pfFileFoundCallBack FileFoundCallBack,PVOID UserParam,BOOL ReplaceVirtualDirectoriesByRealDirectories)
{
    BOOL bSearchCanceled;
    return CFileSearch::SearchMultipleNames(Path,&FileFilterWithWildChar,1,SearchInSubDirectories,NULL,FileFoundCallBack,UserParam,&bSearchCanceled,ReplaceVirtualDirectoriesByRealDirectories); 
}

BOOL CFileSearch::Search(TCHAR* Path,TCHAR* FileFilterWithWildChar,BOOL SearchInSubDirectories,HANDLE hCancelEvent,pfFileFoundCallBack FileFoundCallBack,PVOID UserParam,BOOL* pSearchCanceled,BOOL ReplaceVirtualDirectoriesByRealDirectories
                        ,SIZE_T ExcludedSubDirectoriesCount,TCHAR** ExcludedSubDirectories,BOOL ExcludedSubDirectoriesForMainDirectoryOnly)
{
    return CFileSearch::SearchMultipleNames(Path,&FileFilterWithWildChar,1,SearchInSubDirectories,hCancelEvent,FileFoundCallBack,UserParam,pSearchCanceled,ReplaceVirtualDirectoriesByRealDirectories,ExcludedSubDirectoriesCount,ExcludedSubDirectories,ExcludedSubDirectoriesForMainDirectoryOnly);
}

// SearchMultipleNames: Use the following to get FileNameWithWildCharArray for multiple extensions filters like "*.dll;*.exe;*.ocx"
//     SIZE_T ArraySize=0;
//     TCHAR** pExtensionArray = CMultipleElementsParsing::ParseString("*.dll;*.exe;*.ocx",&ArraySize);
//     CFileSearch::SearchMultipleNames(pszDirectory,pExtensionArray,ArraySize,SearchInSubDirectories,...);
//     CMultipleElementsParsing::ParseStringArrayFree(pExtensionArray,ArraySize);
BOOL CFileSearch::SearchMultipleNames(TCHAR* pszDirectory, TCHAR** FileNameWithWildCharArray,SIZE_T FileNameWithWildCharArraySize,BOOL SearchInSubDirectories,HANDLE hCancelEvent,pfFileFoundCallBack FileFoundCallBack,PVOID UserParam,BOOL* pSearchCanceled,BOOL ReplaceVirtualDirectoriesByRealDirectories,SIZE_T ExcludedSubDirectoriesCount,TCHAR** ExcludedSubDirectories,BOOL ExcludedSubDirectoriesForMainDirectoryOnly)
{
    SIZE_T Cnt;
    COutputString** ExcludedSubDirectoryOutputStringArray=NULL;
    COutputString* pOutputString;
    BOOL bRetValue;

    if (ExcludedSubDirectoriesCount>0)
    {
        ExcludedSubDirectoryOutputStringArray = new COutputString*[ExcludedSubDirectoriesCount];
        for (Cnt=0;Cnt<ExcludedSubDirectoriesCount;Cnt++)
        {
            // assume all provided directories are ending with "\\" for quicker compare
            pOutputString = new COutputString(MAX_PATH);
            pOutputString->Append(ExcludedSubDirectories[Cnt]);
            if (pOutputString->IsEndingWith('\\'))
                pOutputString->RemoveEnd(1);

            ExcludedSubDirectoryOutputStringArray[Cnt] = pOutputString;
        }
    }

    // with junction directories, there can be some infinite loops
    // cd Folder1
    // mklink /j Folder2 Folder1  creates an infinite loop as Folder2 is a sub folder of folder1 and points to folder1
    CLinkListSimpleSingleThreaded ParsedDirectories;
    bRetValue = CFileSearch::SearchMultipleNames2(&ParsedDirectories,pszDirectory, 
                                                FileNameWithWildCharArray,
                                                FileNameWithWildCharArraySize,
                                                SearchInSubDirectories,
                                                hCancelEvent,
                                                FileFoundCallBack,
                                                UserParam,
                                                pSearchCanceled,
                                                ReplaceVirtualDirectoriesByRealDirectories,
                                                ExcludedSubDirectoriesCount,
                                                ExcludedSubDirectoryOutputStringArray,
                                                ExcludedSubDirectoriesForMainDirectoryOnly
                                                );
    
    if (ExcludedSubDirectoryOutputStringArray)
    {
        for (Cnt=0;Cnt<ExcludedSubDirectoriesCount;Cnt++)
        {
            delete ExcludedSubDirectoryOutputStringArray[Cnt];
        }
        delete[] ExcludedSubDirectoryOutputStringArray;
    }

    CFileSearch::FreeDirectories(&ParsedDirectories);

    return bRetValue;
}
BOOL CFileSearch::SearchMultipleNames2(CLinkListSimpleSingleThreaded* pParsedDirectories,TCHAR* pszDirectory, TCHAR** FileNameWithWildCharArray,SIZE_T FileNameWithWildCharArraySize,BOOL SearchInSubDirectories,HANDLE hCancelEvent,pfFileFoundCallBack FileFoundCallBack,PVOID UserParam,BOOL* pSearchCanceled,BOOL ReplaceVirtualDirectoriesByRealDirectories,SIZE_T ExcludedSubDirectoriesCount,COutputString** ExcludedSubDirectories,BOOL ExcludedSubDirectoriesForMainDirectoryOnly)
{
    BOOL bRetValue;
    HANDLE hFileHandle=INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA FindFileData={0};
    *pSearchCanceled=FALSE;
    bRetValue = FALSE;

    if (::IsBadCodePtr((FARPROC)FileFoundCallBack))
        return FALSE;

    // if cancel event is signaled
    if (::WaitForSingleObject(hCancelEvent,0)==WAIT_OBJECT_0)
        return TRUE;

    if (::IsBadReadPtr(pszDirectory,sizeof(TCHAR)))
        return FALSE;

    if (::IsBadReadPtr(FileNameWithWildCharArray,sizeof(TCHAR*)*FileNameWithWildCharArraySize))
        return FALSE;

    SIZE_T Cnt;

    TCHAR* pRealPath=NULL;
    // COutputString Directory;
    COutputString* pDirectory = new COutputString;
    COutputString FullPath;
    COutputString SearchSubDirectoriesPattern;
    COutputString SubDirectory;
    BOOL bSkipCurrentSubDirectory;

    pDirectory->Append(pszDirectory);
    // assume that directory ends with '\\'
    if (!pDirectory->IsEndingWith('\\'))
        pDirectory->Append('\\');

    if (IsAlreadyParsed(pParsedDirectories,pDirectory))
    {
        delete pDirectory;
        return TRUE;
    }

    pParsedDirectories->AddItem(pDirectory);

    FullPath.Append(pDirectory->GetString(),pDirectory->GetLength());

    //////////////////////////////////////////////
    // 1) find files in current directory
    //////////////////////////////////////////////

    for (Cnt=0;Cnt<FileNameWithWildCharArraySize;Cnt++)
    {
        if (::IsBadReadPtr(FileNameWithWildCharArray[Cnt],sizeof(TCHAR)))
        {
#ifdef _DEBUG
            if (::IsDebuggerPresent())
                ::DebugBreak();
#endif
            continue;
        }

        // keep only directory
        FullPath.Clear();
        FullPath.Append(pDirectory->GetString(),pDirectory->GetLength());

        // forge new path
        FullPath.Append(FileNameWithWildCharArray[Cnt]);

        // find first file
        hFileHandle=::FindFirstFile(FullPath.GetString(),&FindFileData);
        if (hFileHandle!=INVALID_HANDLE_VALUE)// if no matching file found
        {
            do 
            {
                // remove current and upper dir
                if ( (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                     && ( (_tcscmp(FindFileData.cFileName,TEXT("."))==0) || (_tcscmp(FindFileData.cFileName,TEXT(".."))==0) )
                   )
                    continue;

                // call callback
                if (!FileFoundCallBack(pDirectory->GetString(),&FindFileData,UserParam))
                {
                    *pSearchCanceled=TRUE;
                    bRetValue = TRUE;
                    goto CleanUp;
                }

                // check cancel event
                if (hCancelEvent)
                {
                    // if event is signaled
                    if (::WaitForSingleObject(hCancelEvent,0)==WAIT_OBJECT_0)
                    {
                        *pSearchCanceled=TRUE;
                        bRetValue = TRUE;
                        goto CleanUp;
                    }
                }

                // find next file
            } while(::FindNextFile(hFileHandle, &FindFileData));

            ::FindClose(hFileHandle);
            // prepare for next goto cleanup
            hFileHandle = INVALID_HANDLE_VALUE;
        }
    }

    //////////////////////////////////////////////
    // 2) find sub directories
    //////////////////////////////////////////////

    // check if we should search in subdirectories
    if (!SearchInSubDirectories)
    {
        bRetValue = TRUE;
        goto CleanUp;
    }

    // SearchSubDirectoriesPattern="path\*"
    SearchSubDirectoriesPattern.Append(pDirectory->GetString(),pDirectory->GetLength());
    SearchSubDirectoriesPattern.Append('*');

    // find first file
    hFileHandle=::FindFirstFile(SearchSubDirectoriesPattern.GetString(),&FindFileData);
    if (hFileHandle==INVALID_HANDLE_VALUE)// if no file found
    {
        bRetValue = TRUE;
        goto CleanUp;
    }

    COutputString** ChildrenExcludedSubDirectories = ExcludedSubDirectories;
    SIZE_T ChildrenExcludedSubDirectoriesCount = ExcludedSubDirectoriesCount;
    BOOL bChildrenExcludedSubDirectories = ExcludedSubDirectoriesForMainDirectoryOnly;
    if (ExcludedSubDirectoriesForMainDirectoryOnly)
    {
        bChildrenExcludedSubDirectories = FALSE;
        ChildrenExcludedSubDirectoriesCount = 0;
        ChildrenExcludedSubDirectories = NULL;
    }

    pRealPath= new TCHAR[CFileSearch_MAX_PATH_SIZE];
    do 
    {
        if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
        {
            if (     (_tcscmp(FindFileData.cFileName,_T("."))==0)
                || (_tcscmp(FindFileData.cFileName,_T(".."))==0)
                )
                continue;

            bSkipCurrentSubDirectory = FALSE;
            for (Cnt=0;Cnt<ExcludedSubDirectoriesCount;Cnt++)
            {
                if (_tcsicmp(FindFileData.cFileName,ExcludedSubDirectories[Cnt]->GetString())==0)
                {
                    bSkipCurrentSubDirectory = TRUE;
                    break;
                }
            }
            if (bSkipCurrentSubDirectory)
                continue;

            SubDirectory.Clear();
            SubDirectory.Append(pDirectory->GetString(),pDirectory->GetLength());
            SubDirectory.Append(FindFileData.cFileName);

            if (ReplaceVirtualDirectoriesByRealDirectories)
            {
                if (CVirtualDirectory::GetRealPath(SubDirectory.GetString(),pRealPath,CFileSearch_MAX_PATH_SIZE))
                {
                    SubDirectory.Clear();
                    SubDirectory.Append(pRealPath);
                }
            }

            if (!SubDirectory.IsEndingWith('\\'))
                SubDirectory.Append('\\');

            CFileSearch::SearchMultipleNames2(pParsedDirectories,SubDirectory.GetString(),
                                            FileNameWithWildCharArray,
                                            FileNameWithWildCharArraySize,
                                            SearchInSubDirectories,
                                            hCancelEvent,
                                            FileFoundCallBack,
                                            UserParam,
                                            pSearchCanceled,
                                            ReplaceVirtualDirectoriesByRealDirectories,
                                            ChildrenExcludedSubDirectoriesCount,
                                            ChildrenExcludedSubDirectories,
                                            bChildrenExcludedSubDirectories
                                            );
            if (*pSearchCanceled)
            {
                bRetValue = TRUE;
                goto CleanUp;
            }
        }

        // check cancel event
        if (hCancelEvent)
        {
            // if event is signaled
            if (::WaitForSingleObject(hCancelEvent,0)==WAIT_OBJECT_0)
            {
                *pSearchCanceled=TRUE;
                bRetValue = TRUE;
                goto CleanUp;
            }
        }

        // find next file
    } while(::FindNextFile(hFileHandle, &FindFileData));

    bRetValue = TRUE;
CleanUp:
    if (hFileHandle!=INVALID_HANDLE_VALUE)
        ::FindClose(hFileHandle);
    if (pRealPath)
        delete[] pRealPath;

    return bRetValue;
}

void CFileSearch::FreeDirectories(CLinkListSimpleSingleThreaded* pParsedDirectories)
{
    CLinkListItem* pItem;
    COutputString* pAlreadyParsedString;
    for (pItem = pParsedDirectories->Head;pItem;pItem=pItem->NextItem)
    {
        pAlreadyParsedString = (COutputString*)pItem->ItemData;
        delete pAlreadyParsedString;
    }

    pParsedDirectories->RemoveAllItems();
}

BOOL CFileSearch::IsAlreadyParsed(CLinkListSimpleSingleThreaded* pParsedDirectories,COutputString* pOutputString)
{
    CLinkListItem* pItem;
    COutputString* pAlreadyParsedString;
    for (pItem = pParsedDirectories->Head;pItem;pItem=pItem->NextItem)
    {
        pAlreadyParsedString = (COutputString*)pItem->ItemData;
        if (pAlreadyParsedString->GetLength() != pOutputString->GetLength())
            continue;
        if (_tcsicmp(pAlreadyParsedString->GetString(),pOutputString->GetString())==0)
            return TRUE;
    }

    return FALSE;
}
