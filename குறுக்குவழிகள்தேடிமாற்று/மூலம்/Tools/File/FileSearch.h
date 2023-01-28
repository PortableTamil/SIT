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
// Object: File Searching helper
//-----------------------------------------------------------------------------

#pragma once
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501 // for xp os
#endif
#include <Windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

#include "../String/OutputString.h"
#include "../LinkList/SingleThreaded/LinkListSimpleSingleThreaded.h"

#define CFileSearch_MAX_PATH_SIZE 4096

// return TRUE to continue parsing, FALSE to stop it
typedef BOOL (*pfFileFoundCallBack)(TCHAR* Directory,WIN32_FIND_DATA* pWin32FindData,PVOID UserParam);

class CFileSearch
{
public:
    static BOOL IsDirectory(WIN32_FIND_DATA* pWin32FindData);
    
    // in all the following "BOOL ReplaceVirtualDirectoriesByRealDirectories" if TRUE, allows to detect duplicates 
    // by replacing virtual folder name with the original one in case of virtual folder
    // aka pfFileFoundCallBack is called with the real path
    static BOOL Search(TCHAR* PathWithWildChar,BOOL SearchInSubDirectories,pfFileFoundCallBack FileFoundCallBack,PVOID UserParam,BOOL ReplaceVirtualDirectoriesByRealDirectories);
    static BOOL Search(TCHAR* PathWithWildChar,BOOL SearchInSubDirectories,HANDLE hCancelEvent,pfFileFoundCallBack FileFoundCallBack,PVOID UserParam,BOOL ReplaceVirtualDirectoriesByRealDirectories);
    static BOOL Search(TCHAR* PathWithWildChar,BOOL SearchInSubDirectories,HANDLE hCancelEvent,pfFileFoundCallBack FileFoundCallBack,PVOID UserParam,BOOL* pSearchCanceled,BOOL ReplaceVirtualDirectoriesByRealDirectories);
    static BOOL Search(TCHAR* Path,TCHAR* FileFilterWithWildChar,BOOL SearchInSubDirectories,pfFileFoundCallBack FileFoundCallBack,PVOID UserParam,BOOL ReplaceVirtualDirectoriesByRealDirectories);
    static BOOL Search(TCHAR* Path,TCHAR* FileFilterWithWildChar,BOOL SearchInSubDirectories,HANDLE hCancelEvent,pfFileFoundCallBack FileFoundCallBack,PVOID UserParam,BOOL* pSearchCanceled,BOOL ReplaceVirtualDirectoriesByRealDirectories,SIZE_T ExcludedSubDirectoriesCount=0,TCHAR** ExcludedSubDirectories=NULL,BOOL ExcludedSubDirectoriesForMainDirectoryOnly=FALSE);

    // SearchMultipleNames: Use the following to get FileNameWithWildCharArray for multiple extensions filters like "*.dll;*.exe;*.ocx"
    //     SIZE_T ArraySize=0;
    //     TCHAR** pExtensionArray = CMultipleElementsParsing::ParseString("*.dll;*.exe;*.ocx",&ArraySize);
    //     CFileSearch::SearchMultipleNames(pszDirectory,pExtensionArray,ArraySize,SearchInSubDirectories,...);
    //     CMultipleElementsParsing::ParseStringArrayFree(pExtensionArray,ArraySize);
    static BOOL SearchMultipleNames(TCHAR* pszDirectory, TCHAR** FileNameWithWildCharArray,SIZE_T FileNameWithWildCharArraySize,BOOL SearchInSubDirectories,HANDLE hCancelEvent,pfFileFoundCallBack FileFoundCallBack,PVOID UserParam,BOOL* pSearchCanceled,BOOL ReplaceVirtualDirectoriesByRealDirectories,SIZE_T ExcludedSubDirectoriesCount=0,TCHAR** ExcludedSubDirectories=NULL,BOOL ExcludedSubDirectoriesForMainDirectoryOnly=FALSE);

protected:
    static BOOL SearchMultipleNames2(CLinkListSimpleSingleThreaded* pParsedDirectories,TCHAR* pszDirectory, TCHAR** FileNameWithWildCharArray,SIZE_T FileNameWithWildCharArraySize,BOOL SearchInSubDirectories,HANDLE hCancelEvent,pfFileFoundCallBack FileFoundCallBack,PVOID UserParam,BOOL* pSearchCanceled,BOOL ReplaceVirtualDirectoriesByRealDirectories,SIZE_T ExcludedSubDirectoriesCount,COutputString** ExcludedSubDirectories,BOOL ExcludedSubDirectoriesForMainDirectoryOnly);
    static BOOL IsAlreadyParsed(CLinkListSimpleSingleThreaded* pParsedDirectories,COutputString* pOutputString);
    static void FreeDirectories(CLinkListSimpleSingleThreaded* pParsedDirectories);
};