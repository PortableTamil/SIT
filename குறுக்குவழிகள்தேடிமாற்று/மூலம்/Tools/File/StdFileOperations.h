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
// Object: standard File operations
//-----------------------------------------------------------------------------

#pragma once

#ifndef _WIN32_WINNT
    #define _WIN32_WINNT 0x0501
#endif

#include <Windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

class CStdFileOperations
{
public:
    static BOOL DoesFileExists(IN TCHAR* FullPath);
    static BOOL GetFileSize(IN TCHAR* FullPath,OUT ULONG64* pFileSize);
    static BOOL GetFileTime(IN TCHAR* FullPath,
                            OUT LPFILETIME lpCreationTime,  // can be NULL
                            OUT LPFILETIME lpLastAccessTime,// can be NULL
                            OUT LPFILETIME lpLastWriteTime  // can be NULL
                            );
    static BOOL DoesExtensionMatch(IN TCHAR* FileName,IN TCHAR* Extension);
    static BOOL GetFileDrive(IN TCHAR* FilePath,OUT TCHAR* FileDrive);
    static TCHAR* GetFileExt(IN TCHAR* FileName);
    static BOOL ChangeFileExt(IN OUT TCHAR* FileName,IN TCHAR* NewExtension);
    static BOOL RemoveFileExt(IN OUT TCHAR* FileName);
    static TCHAR* GetFileName(IN TCHAR* FullPath);
    static BOOL GetFileDirectory(TCHAR* FullPath,OUT TCHAR* Path,IN DWORD PathMaxSize);
    static BOOL GetAppPath(OUT TCHAR* ApplicationPath,IN DWORD ApplicationPathMaxSize);
    static BOOL GetAppDirectory(OUT TCHAR* ApplicationDirectory,IN DWORD ApplicationDirectoryMaxSize);
    static BOOL GetModulePath(IN HMODULE hModule,OUT TCHAR* ModuleFullPath,IN DWORD ModuleFullPathMaxSize);
    static BOOL GetModuleDirectory(IN HMODULE hModule,OUT TCHAR* ModuleDirectory,IN DWORD ModuleDirectoryMaxSize);
    static HMODULE GetCurrentModuleHandle();
    static BOOL GetCurrentModulePath(OUT TCHAR* ModulePath,IN DWORD ModulePathMaxSize);
    static BOOL GetCurrentModuleDirectory(OUT TCHAR* ModuleDirectory,IN DWORD ModuleDirectoryMaxSize);
    static BOOL GetUpperDirectory(IN OUT TCHAR* Path);
    // GetTempFileName : get temporary file name in windows temporary directory
    static BOOL GetTempFileName(OUT TCHAR* TempFileName);
    // GetTempFileNameInsideDirectory : get temporary file name in the directory specified
    static BOOL GetTempFileNameInsideDirectory(IN TCHAR* Directory,OUT TCHAR* TempFileName);
    // GetTempFileName : get temporary file name in the directory of BaseFileName
    static BOOL GetTempFileName(IN TCHAR* BaseFileName,OUT TCHAR* TempFileName);
    static BOOL HasReadOnlyAttribute(IN TCHAR* Path);
    static BOOL RemoveReadOnlyAttribute(IN TCHAR* Path);
    static BOOL IsWritable(IN TCHAR* Path);
    static BOOL IsNetworkPath(IN TCHAR* Path);
    static BOOL IsDevicePath(IN TCHAR* Path);
    static BOOL IsDrivePath(IN TCHAR* Path);
    static BOOL IsDriveLetter(IN TCHAR Letter);
    static BOOL IsRelativePath(IN TCHAR* Path);
    static BOOL IsFullPath(IN TCHAR* FileName);
    static BOOL IsDirectory(IN TCHAR* FullPath);
    static BOOL IsFileInDirectoryOrSubDirectory(IN TCHAR* File,IN TCHAR* Directory);
    static BOOL IsFileOpen(IN TCHAR* Path);
    static BOOL IsFileOpenInWriteMode(IN TCHAR* Path);
    static BOOL CreateDirectoryForFile(IN TCHAR* FilePathToCreate);
    static BOOL CreateDirectory(IN TCHAR* Directory);
    static BOOL DoesDirectoryExists(IN TCHAR* Directory);
    static BOOL GetAbsolutePath(IN TCHAR* RelativePath,OUT TCHAR* AbsolutePath);
    static BOOL GetAbsolutePath(IN TCHAR* ReferenceDirectory,IN TCHAR* RelativePath,OUT TCHAR* AbsolutePath);
    static BOOL GetRelativePath(IN TCHAR* From,IN TCHAR* To,OUT TCHAR* RelativePath);
    static BOOL ContainsInvalidPathChar(IN TCHAR* Str);
    static void ReplaceInvalidPathChar(IN OUT TCHAR* Str);
    static void AssumePathSplitterIsBackslash(IN OUT TCHAR* Path);
    static BOOL AssumeDirectoryEndsWithBackSlash(IN OUT TCHAR* Directory);
    static BOOL RemoveDirectoryEndingBackSlashIfAny(IN OUT TCHAR* Directory);

    static BOOL CopyFile(IN TCHAR* ExistingFilePath,IN TCHAR* NewFilePath,IN BOOL bFailIfExists);
    static BOOL CopyDirectory(IN TCHAR* ExistingDirectoryPath,IN TCHAR* NewDirectoryPath,IN BOOL bFailIfExists);
};
