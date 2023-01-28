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

#include "StdFileOperations.h"
#include <malloc.h>

#ifndef _DefineStringLen
    // _DefineStringLen : remove string \0 from array size
    #define _DefineStringLen(Array) (_countof(Array)-1)
#endif

#ifndef BreakInDebugMode
    #ifdef _DEBUG
        #define BreakInDebugMode if ( ::IsDebuggerPresent() ) { ::DebugBreak();}
    #else 
        #define BreakInDebugMode 
    #endif
#endif

#define UNC_PREFIX          TEXT("\\\\?\\")
#define UNC_PREFIX_LEN     _DefineStringLen(UNC_PREFIX)

// #include <intrin.h>
// #pragma intrinsic(_ReturnAddress)
// __declspec(noinline)
// PBYTE GetEip(void) // Notice : not the best has an RIP access is quicker but avoid to include asm files in x64
// {
//     return _ReturnAddress();
// }

EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)

__declspec(noinline) // force no inline so GetCurrentModule address will exist and be the same for all
HMODULE CStdFileOperations::GetCurrentModuleHandle()
{ 
    return HINST_THISCOMPONENT;
    
    // More portable solution NB: XP+ solution!
    // HMODULE hModule = NULL;
    // ::GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,(LPCTSTR)GetCurrentModuleHandle, &hModule);
    // return hModule;
}

//-----------------------------------------------------------------------------
// Name: GetCurrentModuleDirectory
// Object: get current module directory (keep last '\')
// Parameters :
//     TCHAR* ModulePath : fill a user allocated buffer
//     DWORD ModulePathMaxSize : user allocated buffer max size
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CStdFileOperations::GetCurrentModuleDirectory(OUT TCHAR* ModulePath,IN DWORD ModulePathMaxSize)
{
    return CStdFileOperations::GetModuleDirectory(CStdFileOperations::GetCurrentModuleHandle(),ModulePath,ModulePathMaxSize);
}

//-----------------------------------------------------------------------------
// Name: GetCurrentModulePath
// Object: get current module full path (directory + file name)
// Parameters :
//     TCHAR* ModulePath : fill a user allocated buffer
//     DWORD ModulePathMaxSize : user allocated buffer max size
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CStdFileOperations::GetCurrentModulePath(OUT TCHAR* ModulePath,IN DWORD ModulePathMaxSize)
{
    return CStdFileOperations::GetModulePath(CStdFileOperations::GetCurrentModuleHandle(),ModulePath,ModulePathMaxSize);
}

//-----------------------------------------------------------------------------
// Name: DoesFileExists
// Object: check if file exists
// Parameters :
//     in : TCHAR* FullPath : full path
// Return : TRUE if file exists
//-----------------------------------------------------------------------------
BOOL CStdFileOperations::DoesFileExists(IN TCHAR* FullPath)
{
    BOOL bFileExists=FALSE;

    HANDLE hFile=::CreateFile(FullPath,
                            FILE_READ_ATTRIBUTES, 
                            FILE_SHARE_READ|FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING, 
                            0, 
                            NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        bFileExists=TRUE;
        ::CloseHandle(hFile);
    }
    return bFileExists;
}

//-----------------------------------------------------------------------------
// Name: GetFileSize
// Object: Get file size 
// Parameters :
//     in : TCHAR* FullPath : full path
//     out : ULONG64* pFileSize : file size
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CStdFileOperations::GetFileSize(IN TCHAR* FullPath,OUT ULONG64* pFileSize)
{
    *pFileSize = 0;
    BOOL bRet;
    // Open the file 
    HANDLE hFile=::CreateFile(FullPath,GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if( hFile == INVALID_HANDLE_VALUE ) 
        return FALSE; 

    // Obtain the size of the file 
    LARGE_INTEGER li;
    bRet=::GetFileSizeEx( hFile, &li );
    if ( bRet )
        *pFileSize=li.QuadPart;

    ::CloseHandle(hFile);
    return bRet; 
}

//-----------------------------------------------------------------------------
// Name: GetFileTime
// Object: Get file time 
// Parameters :
//     in : TCHAR* FullPath : full path
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CStdFileOperations::GetFileTime(IN TCHAR* FullPath,
                                    OUT LPFILETIME lpCreationTime,  // can be NULL
                                    OUT LPFILETIME lpLastAccessTime,// can be NULL
                                    OUT LPFILETIME lpLastWriteTime  // can be NULL
                                    )
{
    BOOL bRet;
    // Open the file 
    HANDLE hFile=::CreateFile(FullPath,GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if( hFile == INVALID_HANDLE_VALUE ) 
        return FALSE; 

    // Obtain the size of the file 
    bRet = ::GetFileTime(hFile,
                        lpCreationTime,  // can be NULL
                        lpLastAccessTime,// can be NULL
                        lpLastWriteTime  // can be NULL
                        );

    ::CloseHandle(hFile);
    return bRet; 

}

//-----------------------------------------------------------------------------
// Name: GetFileName
// Object: Get file name from FullPath
// Parameters :
//     in : TCHAR* FullPath : full path
// Return : pointer on FileName inside FullPath buffer, NULL on error
//-----------------------------------------------------------------------------
TCHAR* CStdFileOperations::GetFileName(IN TCHAR* FullPath)
{
    if (FullPath == NULL)
    {
        BreakInDebugMode;
        return FALSE;
    }

    TCHAR* psz;

    // find last '\'
    psz=_tcsrchr(FullPath,'\\');

    // if not found
    if (!psz)
        return FullPath;

    // go after '\'
    psz++;

    return psz;
}

//-----------------------------------------------------------------------------
// Name: GetFileDirectory
// Object: Get Directory from FullPath keeps last '\'
// Parameters :
//     in : TCHAR* FullPath : full path
//          DWORD PathMaxSize : path max size in TCHAR
//          TCHAR* Path : app path with ending '\'
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CStdFileOperations::GetFileDirectory(IN TCHAR* FullPath,OUT TCHAR* Path,IN DWORD PathMaxSize)
{
    if (FullPath == NULL)
    {
        BreakInDebugMode;
        return FALSE;
    }
    if (Path == NULL)
    {
        BreakInDebugMode;
        return FALSE;
    }

    TCHAR* psz;
    // don't put *Path=0; in case FullPath and Path point to the same buffer
    // *Path=0;

    // find last '\'
    psz=_tcsrchr(FullPath,'\\');

    // if not found
    if (!psz)
    {
        *Path = 0;
        return FALSE;
    }

    // we want to keep '\'
    psz++;
    if (PathMaxSize<(DWORD)(psz-FullPath))
    {
        *Path = 0;
        return FALSE;
    }

    _tcsncpy(Path,FullPath,psz-FullPath);
    Path[psz-FullPath]=0;

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: ChangeFileExt
// Object: replace old extension of FileName by the new provided one
// Parameters :
//     in : TCHAR* FileName : filename, buffer should long enough to support new extension
//          TCHAR* NewExtension : new extension without '.'
// Return : FALSE on error
//-----------------------------------------------------------------------------
BOOL CStdFileOperations::ChangeFileExt(IN OUT TCHAR* FileName,IN TCHAR* NewExtension)
{
    if (FileName == NULL)
    {
        BreakInDebugMode;
        return FALSE;
    }
    if (NewExtension == NULL)
    {
        BreakInDebugMode;
        return FALSE;
    }
    // assume new extension
    if (*NewExtension == '.')
    {
        BreakInDebugMode;
        NewExtension++;
    }

    TCHAR* psz=CStdFileOperations::GetFileExt(FileName);
    if (*psz==0)
    {
        // no extension
        _tcscat(FileName,_T("."));
        _tcscat(FileName,NewExtension);
        return TRUE;
    }
    // else
    _tcscpy(psz,NewExtension);
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: RemoveFileExt
// Object: remove extension of FileName
// Parameters :
//     in : TCHAR* FileName : filename
// Return : FALSE on error
//-----------------------------------------------------------------------------
BOOL CStdFileOperations::RemoveFileExt(IN OUT TCHAR* FileName)
{
    
    TCHAR* psz=CStdFileOperations::GetFileExt(FileName);
    // on failure GetFileExt returns an empty const string
    if (*psz==0)// protect us against our empty const string which is in non writable section
    {
        BreakInDebugMode;
        return FALSE;
    }
    
    // go back to '.' as GetFileExt gives us a pointer after '.'
    psz--;
    *psz=0;
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: GetFileDrive
// Object: get file drive
// Parameters :
//     in : TCHAR* FileName : filename
//     out :TCHAR* FileDrive : must be _MAX_DRIVE len at least
//
// Return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CStdFileOperations::GetFileDrive(IN TCHAR* FilePath,OUT TCHAR* FileDrive)
{
    *FileDrive = 0;
    
    if (!CStdFileOperations::IsDrivePath(FilePath))
    {
        BreakInDebugMode;
        return FALSE;
    }

    // remove UNC_PREFIX if any
    CStdFileOperations::AssumePathSplitterIsBackslash(FilePath);
    TCHAR* pc = FilePath;
    if (_tcsncmp(UNC_PREFIX,pc,UNC_PREFIX_LEN) == 0)
        pc+=UNC_PREFIX_LEN;

    _tsplitpath(pc,FileDrive,NULL,NULL,NULL);

#ifdef _DEBUG
    if (*FileDrive==0)
        BreakInDebugMode;
#endif

    return (*FileDrive!=0);
}

//-----------------------------------------------------------------------------
// Name: GetFileExt
// Object: get pointer on file extension (point after last '.' or _T("") if no '.' found)
// Parameters :
//     in : TCHAR* FileName : filename
// Return : pointer on file ext in FileName buffer, _T("") on error
//-----------------------------------------------------------------------------
TCHAR* CStdFileOperations::GetFileExt(IN TCHAR* FileName)
{
    if (FileName == NULL)
    {
        BreakInDebugMode;
        return FALSE;
    }

    // _tsplitpath can be used too

    TCHAR* psz;
    TCHAR* psz2;
    // _tcsrtok
    
    CStdFileOperations::AssumePathSplitterIsBackslash(FileName);

    // find last point
    psz=_tcsrchr(FileName,'.');
    psz2=_tcsrchr(FileName,'\\');

    // if not found
    if ( (!psz) 
         || (psz2>psz) // file with no ext
       )
    {
        BreakInDebugMode;
        return _T("");
    }
    // else

    // go after point
    psz++;
    
    return psz;
}

//-----------------------------------------------------------------------------
// Name: DoesExtensionMatch
// Object: check if filename has the same extension as the provided one
// Parameters :
//     in : TCHAR* FileName : filename
//          TCHAR* Extension : extension to check (without "." like "exe", "zip", ...)
// Return : TRUE if extension match
//-----------------------------------------------------------------------------
BOOL CStdFileOperations::DoesExtensionMatch(IN TCHAR* FileName,IN TCHAR* Extension)
{
    TCHAR* psz;
    
    // get extension
    psz=CStdFileOperations::GetFileExt(FileName);
    if(*psz==0)
        return FALSE;

    // check extension
    return (_tcsicmp(psz,Extension)==0);
}

//-----------------------------------------------------------------------------
// Name: GetAppPath
// Object: get current application full path (directory + file name)
// Parameters :
//     TCHAR* ApplicationPath : fill full path
//     DWORD ApplicationPathMaxSize : user allocated buffer max size
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CStdFileOperations::GetAppPath(OUT TCHAR* ApplicationPath,IN DWORD ApplicationPathMaxSize)
{
    if (ApplicationPath == NULL)
    {
        BreakInDebugMode;
        return FALSE;
    }
    *ApplicationPath=0;
    return ::GetModuleFileName(::GetModuleHandle(NULL),ApplicationPath,ApplicationPathMaxSize);
}

//-----------------------------------------------------------------------------
// Name: GetAppDirectory
// Object: get current application directory (keep last '\')
// Parameters :
//     TCHAR* ApplicationDirectory : fill a user allocated buffer
//     DWORD ApplicationDirectoryMaxSize : user allocated buffer max size
//
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CStdFileOperations::GetAppDirectory(OUT TCHAR* ApplicationDirectory,IN DWORD ApplicationDirectoryMaxSize)
{
    if (!CStdFileOperations::GetAppPath(ApplicationDirectory,ApplicationDirectoryMaxSize))
        return FALSE;
    return CStdFileOperations::GetFileDirectory(ApplicationDirectory,ApplicationDirectory,ApplicationDirectoryMaxSize);
}

//-----------------------------------------------------------------------------
// Name: GetModuleDirectory
// Object: get specified module directory (keep last '\')
// Parameters :
//     HMODULE hModule : Module handle
//     TCHAR* ModuleDirectory : fill a user allocated buffer
//     DWORD ModuleDirectoryMaxSize : user allocated buffer max size
//     
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CStdFileOperations::GetModuleDirectory(IN HMODULE hModule,OUT TCHAR* ModuleDirectory,IN DWORD ModuleDirectoryMaxSize)
{
    if (ModuleDirectory == NULL)
    {
        BreakInDebugMode;
        return FALSE;
    }
    *ModuleDirectory=0;
    // get module file name
    if (!CStdFileOperations::GetModulePath(hModule,ModuleDirectory,ModuleDirectoryMaxSize))
        return FALSE;

    return CStdFileOperations::GetFileDirectory(ModuleDirectory,ModuleDirectory,ModuleDirectoryMaxSize);
}

//-----------------------------------------------------------------------------
// Name: GetModulePath
// Object: get specified module full path (directory + file name)
// Parameters :
//     HMODULE hModule : Module handle
//     TCHAR* ModuleFullPath : fill a user allocated buffer
//     DWORD ModuleFullPathMaxSize : user allocated buffer max size
//     
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CStdFileOperations::GetModulePath(IN HMODULE hModule,OUT TCHAR* ModuleFullPath,IN DWORD ModuleFullPathMaxSize)
{
    if (ModuleFullPath == NULL)
    {
        BreakInDebugMode;
        return FALSE;
    }
    *ModuleFullPath=0;
    DWORD dwRes=::GetModuleFileName(hModule,ModuleFullPath,ModuleFullPathMaxSize);
    // in case of failure or if buffer is too small
    if ((dwRes==0)||(dwRes==ModuleFullPathMaxSize))
    {
        BreakInDebugMode;
        return FALSE;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: IsFullPath
// Object: Check if filename contains full path
// Parameters :
//     in : TCHAR* FileName : file name to check 
// Return : TRUE if contains full path, FALSE if FileName doesn't contain full path info 
//-----------------------------------------------------------------------------
BOOL CStdFileOperations::IsFullPath(IN TCHAR* FileName)
{
    return !(CStdFileOperations::IsRelativePath(FileName));
}

//-----------------------------------------------------------------------------
// Name: GetTempFileName
// Object: get temporary file name in the directory of BaseFileName
// Parameters :
//     in : TCHAR* BaseFileName : temporary file wil be created in the same directory as BaseFileName
//     in out : TCHAR* TempFileName: This buffer should be MAX_PATH characters
//                  to accommodate the path plus the terminating null character
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CStdFileOperations::GetTempFileName(IN TCHAR* BaseFileName,OUT TCHAR* TempFileName)
{
    if (BaseFileName == NULL)
    {
        BreakInDebugMode;
        return FALSE;
    }
    if (TempFileName == NULL)
    {
        BreakInDebugMode;
        return FALSE;
    }

    *TempFileName=0;
    TCHAR Path[MAX_PATH];
    if (!CStdFileOperations::GetFileDirectory(BaseFileName,Path,MAX_PATH))
        return FALSE;
    return CStdFileOperations::GetTempFileNameInsideDirectory(Path,TempFileName);
}
//-----------------------------------------------------------------------------
// Name: GetTempFileNameInsideDirectory
// Object: get temporary file name in the directory specified
// Parameters :
//     in : TCHAR* BaseFileName : name used to create temp file
//     in out : TCHAR* TempFileName: This buffer should be MAX_PATH characters
//                  to accommodate the path plus the terminating null character
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CStdFileOperations::GetTempFileNameInsideDirectory(IN TCHAR* Directory,OUT TCHAR* TempFileName)
{
    if (TempFileName == NULL)
    {
        BreakInDebugMode;
        return FALSE;
    }

    *TempFileName=0;
    return (::GetTempFileName(Directory,_T("~"),0,TempFileName)!=0);
}
//-----------------------------------------------------------------------------
// Name: GetTempFileName
// Object: get temporary file name in windows temporary directory
// Parameters :
//     in out : TCHAR* TempFileName: This buffer should be MAX_PATH characters
//                  to accommodate the path plus the terminating null character
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CStdFileOperations::GetTempFileName(OUT TCHAR* TempFileName)
{
    if (TempFileName == NULL)
    {
        BreakInDebugMode;
        return FALSE;
    }

    TCHAR Path[MAX_PATH];
    *TempFileName=0;
    if (::GetTempPath(MAX_PATH,Path) == 0)
    {
        BreakInDebugMode;
        return FALSE;
    }
    return CStdFileOperations::GetTempFileNameInsideDirectory(Path,TempFileName);
}

//-----------------------------------------------------------------------------
// Name: CreateDirectory
// Object: create directory with all intermediate directories 
// Parameters :
//     in : TCHAR* Directory : Directory to create
// Return : TRUE on success or if directory was already existing
//-----------------------------------------------------------------------------
BOOL CStdFileOperations::CreateDirectory(IN TCHAR* Directory)
{
    if ( (Directory == NULL) || (*Directory == 0) )
    {
        BreakInDebugMode;
        return FALSE;
    }

    BOOL bRetValue = FALSE;
    DWORD dwLastError;
    size_t Len=_tcslen(Directory);
    TCHAR* LocalDir = _tcsdup(Directory);

    CStdFileOperations::AssumePathSplitterIsBackslash(Directory);

    // remove ending back slash if any
    if (LocalDir[Len-1]=='\\')
        LocalDir[Len-1]=0;

    if (CStdFileOperations::DoesDirectoryExists(LocalDir))
    {
        bRetValue = TRUE;
        goto CleanUp;
    }

    // create directory
    if (::CreateDirectory(LocalDir,NULL)!=0)
    {
        bRetValue = TRUE;
        goto CleanUp;
    }

    // on failure
    // get last error
    dwLastError=GetLastError();
    switch(dwLastError)
    {
    case ERROR_ALREADY_EXISTS:
        // directory already exists --> nothing to do
        bRetValue = TRUE;
        goto CleanUp;
    case ERROR_PATH_NOT_FOUND:
        {
            // upper dir doesn't exists --> create it

            // get upper dir
            TCHAR* psz;
            // find last '\\'
            psz=_tcsrchr(LocalDir,'\\');
            if (!psz)
                goto CleanUp;
            // ends upper dir
            *psz=0;

             if (!CStdFileOperations::CreateDirectory(LocalDir))
                 goto CleanUp;

             bRetValue =::CreateDirectory(Directory,NULL)!=0;
             goto CleanUp;
        }
    default:
        goto CleanUp;
    }
CleanUp:
    if (LocalDir)
        free(LocalDir);

    return bRetValue;
}

//-----------------------------------------------------------------------------
// Name: CreateDirectoryForFile
// Object: create directory if directory does not exists for specified FileName
//         can be used before calling CreateFile, to assume directory exists and avoid CreateFile failure
// Parameters :
//     in : TCHAR* FilePathToCreate : path of file that is going to be created
// Return : TRUE on success or if directory was already existing
//-----------------------------------------------------------------------------
BOOL CStdFileOperations::CreateDirectoryForFile(IN TCHAR* FilePathToCreate)
{
    TCHAR szPath[MAX_PATH];
    // get path
    CStdFileOperations::GetFileDirectory(FilePathToCreate,szPath,MAX_PATH);
    // check if directory already exists
    if (CStdFileOperations::DoesDirectoryExists(szPath))
        return TRUE;
    // create directory
    return CStdFileOperations::CreateDirectory(szPath);
}

//-----------------------------------------------------------------------------
// Name: DoesDirectoryExists
// Object: check if directory exists
// Parameters :
//     in : TCHAR* Directory : directory name
// Return : TRUE if directory exists
//-----------------------------------------------------------------------------
BOOL CStdFileOperations::DoesDirectoryExists(IN TCHAR* Directory)
{
    return CStdFileOperations::IsDirectory(Directory);
    /*
    this test that file/directory can be opened, but returns TRUE for a file too :(
    use CStdFileOperations::IsDirectory which check to existence and directory flag

    BOOL bFileExists=FALSE;
    HANDLE hFile=CreateFile (
                            Directory,
                            GENERIC_READ,
                            FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_FLAG_BACKUP_SEMANTICS,
                            NULL
                            );
    if (hFile != INVALID_HANDLE_VALUE)
    {
        bFileExists=TRUE;
        CloseHandle(hFile);
    }
    return bFileExists;
    */
}

//-----------------------------------------------------------------------------
// Name: IsDirectory
// Object: assume FullPath exists and is a directory
// Parameters :
//     in : TCHAR* Directory : directory name
// Return : TRUE if directory exists
//-----------------------------------------------------------------------------
BOOL CStdFileOperations::IsDirectory(IN TCHAR* FullPath)
{
    if (FullPath == NULL)
    {
        BreakInDebugMode;
        return FALSE;
    }
    DWORD Ret;
    Ret=::GetFileAttributes(FullPath);
    if (Ret==INVALID_FILE_ATTRIBUTES)
        return FALSE;

    return (Ret & FILE_ATTRIBUTE_DIRECTORY);
}

//-----------------------------------------------------------------------------
// Name: GetAbsolutePath
// Object: get absolute path from relative path
// Parameters :
//     in : TCHAR* RelativePath : Relative path
//     out : TCHAR* AbsolutePath : absolute path must be at least MAX_PATH len in TCHAR
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CStdFileOperations::GetAbsolutePath(IN TCHAR* RelativePath,OUT TCHAR* AbsolutePath)
{
    return CStdFileOperations::GetAbsolutePath(NULL,RelativePath,AbsolutePath);
}

//-----------------------------------------------------------------------------
// Name: GetAbsolutePath
// Object: get absolute path from relative path
// Parameters :
//     in :  TCHAR* ReferenceDirectory : if NULL the application directory will be used
//           TCHAR* RelativePath : Relative path
//     out : TCHAR* AbsolutePath : absolute path must be at least MAX_PATH len in TCHAR
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CStdFileOperations::GetAbsolutePath(IN TCHAR* ReferenceDirectory,IN TCHAR* RelativePath,OUT TCHAR* AbsolutePath)
{
    if (AbsolutePath == NULL)
    {
        BreakInDebugMode;
        return FALSE;
    }
    *AbsolutePath=0;

    LPTSTR pszFileName = NULL;
    //if (ReferenceDirectory == NULL) NULL ReferenceDirectory is allowed
    //{
    //    BreakInDebugMode;
    //    return FALSE;
    //}
    if (RelativePath == NULL)
    {
        BreakInDebugMode;
        return FALSE;
    }

    if (RelativePath == AbsolutePath)
    {
        // Relative path must differ from absolute path
        BreakInDebugMode;
        return FALSE;
    }

    CStdFileOperations::AssumePathSplitterIsBackslash(RelativePath);

    // if relative path is current folder
    if(
        (*RelativePath == 0)
        || ( _tcscmp(RelativePath,TEXT(".\\"))==0 )
       )
    {
        if (ReferenceDirectory)
            _tcscpy(AbsolutePath,ReferenceDirectory);
        else
        {
            CStdFileOperations::GetAppDirectory(AbsolutePath,MAX_PATH);
        }
        return TRUE;
    }
    // else


    TCHAR CurrentDirectory[MAX_PATH];
    // save current directory
    ::GetCurrentDirectory(_countof(CurrentDirectory),CurrentDirectory);
    if (ReferenceDirectory)
    {
        // adjust current directory for GetFullPathName
        ::SetCurrentDirectory(ReferenceDirectory);
    }
    else
    {   
        TCHAR ApplicationDirectory[MAX_PATH];
        // we need to force directory to application directory (else CurrentDirectory can depends of process who launched the application, or of user browsing)
        CStdFileOperations::GetAppDirectory(ApplicationDirectory,MAX_PATH);

        // adjust current directory for GetFullPathName
        ::SetCurrentDirectory(ApplicationDirectory);
    }

    // _tfullpath can be used too for more portable way (in this file we call as much API as we can to be linked only to kernel32)
    BOOL bRet = (::GetFullPathName(RelativePath,MAX_PATH,AbsolutePath,&pszFileName)!=0);

    // restore current directory
    ::SetCurrentDirectory(CurrentDirectory);

    return bRet;
}


//-----------------------------------------------------------------------------
// Name: ContainsInvalidPathChar
// Object: check if string contains invalid path chars
// Parameters :
//     in : TCHAR* Path : string to be tested
// Return : TRUE if string contains invalid chars
//-----------------------------------------------------------------------------
BOOL CStdFileOperations::ContainsInvalidPathChar(IN TCHAR* Path)
{
    if (Path == NULL)
    {
        BreakInDebugMode;
        return FALSE;
    }
    if (*Path == 0)
    {
        BreakInDebugMode;
        return FALSE;
    }

    TCHAR* psz = _tcspbrk(Path,_T("\"\\/:*?|<>"));
    return (psz!=NULL);
}

//-----------------------------------------------------------------------------
// Name: ReplaceInvalidPathChar
// Object: replace invalid path chars by '_' (string size stay the same)
// Parameters :
//     in : TCHAR* Path : string to be adjusted
// Return : TRUE if string contains invalid chars
//-----------------------------------------------------------------------------
void CStdFileOperations::ReplaceInvalidPathChar(IN OUT TCHAR* Path)
{
    if (Path == NULL)
    {
        BreakInDebugMode;
        return;
    }
    if (*Path == 0)
    {
        BreakInDebugMode;
        return;
    }

    TCHAR* psz;

    psz = Path;
    for (;;)
    {
        psz = _tcspbrk(psz,_T("\"\\/:*?|<>"));
        if (psz == NULL)
            break;
        // else
        *psz = '_';
        psz++;
    }

}

//-----------------------------------------------------------------------------
// Name: AssumeDirectoryUseBackslash
// Object: replace '/' by '\\'
// Parameters :
//      IN OUT TCHAR* Path : path to check
// Return : 
//-----------------------------------------------------------------------------
void CStdFileOperations::AssumePathSplitterIsBackslash(IN OUT TCHAR* Path)
{
    if (Path == NULL)
    {
        BreakInDebugMode;
        return;
    }

    if (*Path == 0)
        return;

    TCHAR* pc;
    for (pc = Path; *pc; pc++)
    {
        if (*pc =='/')
            *pc ='\\';
    }
}

BOOL CStdFileOperations::IsNetworkPath(IN TCHAR* Path)
{
    if (Path == NULL)
    {
        BreakInDebugMode;
        return FALSE;
    }
    if (*Path == 0)
    {
        BreakInDebugMode;
        return FALSE;
    }

    CStdFileOperations::AssumePathSplitterIsBackslash(Path);
    TCHAR* pc = Path;
    if (_tcsncmp(UNC_PREFIX,pc,UNC_PREFIX_LEN)==0) // if unc path, remove "\\?\"
        pc+=UNC_PREFIX_LEN;

    if (_tcsncmp(TEXT("\\\\"),pc,2)==0)
    {
        // can be 
        // "\\server\"                  network path
        // "\\.\PhysicalDriveX"         Opens the physical drive X
        // "\\.\C:" 	                Opens the C: volume
        // "\\.\C:\" 	                Opens the file system of the C: volume.
        // "\\.\Changerx"
        // "\\.\TAPEx"
        if ( (*pc == '.') && (*(pc+1)=='\\'))
            return FALSE;

        return TRUE;
    }
    else
        return FALSE;
}

BOOL CStdFileOperations::IsDevicePath(IN TCHAR* Path)
{
    if (Path == NULL)
    {
        BreakInDebugMode;
        return FALSE;
    }
    if (*Path == 0)
    {
        BreakInDebugMode;
        return FALSE;
    }

    CStdFileOperations::AssumePathSplitterIsBackslash(Path);
    TCHAR* pc = Path;
    if (_tcsncmp(UNC_PREFIX,pc,UNC_PREFIX_LEN)==0) // if unc path, remove "\\?\"
        pc+=UNC_PREFIX_LEN;

    if (_tcsncmp(TEXT("\\\\.\\"),pc,4)==0)
    {
        // can be 
        // "\\.\PhysicalDriveX"         Opens the physical drive X
        // "\\.\C:" 	                Opens the C: volume
        // "\\.\C:\" 	                Opens the file system of the C: volume.
        // "\\.\Changerx"
        // "\\.\TAPEx"
        return TRUE;
    }
    else
        return FALSE;
}

BOOL CStdFileOperations::IsDrivePath(IN TCHAR* Path)
{
    if (Path == NULL)
    {
        BreakInDebugMode;
        return FALSE;
    }
    if (*Path == 0)
    {
        BreakInDebugMode;
        return FALSE;
    }

    CStdFileOperations::AssumePathSplitterIsBackslash(Path);
    TCHAR* pc = Path;
    if (_tcsncmp(UNC_PREFIX,pc,UNC_PREFIX_LEN)==0) // if unc path, remove "\\?\"
        pc+=UNC_PREFIX_LEN;

    // assume we get drive splitter
    if (*(pc+1) != ':')
        return FALSE;

    // "c:" is the drive; "c:\" is the root folder of "c:", so '\' must be present
    if (*(pc+2) != '\\') 
        return FALSE;

    // included by previous test
    //// can be 
    //// "\\server\"
    //// "\\.\PhysicalDriveX"         Opens the physical drive X
    //// "\\.\C:" 	                Opens the C: volume
    //// "\\.\C:\" 	                Opens the file system of the C: volume.
    //// "\\.\Changerx"
    //// "\\.\TAPEx"
    //if (_tcsncmp(TEXT("\\"),pc,2)==0)
    //    return FALSE;

    if (!CStdFileOperations::IsDriveLetter(*pc))
        return FALSE;

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: GetRelativePath
// Object: create relative path from one location to another location
// Parameters :
//      IN TCHAR* From : Original directory
//      IN TCHAR* To : Destination directory
//      OUT TCHAR* RelativePath : Relative path from "From" to "To"
// Return : TRUE if a relative path can be made, FALSE if it can't be done (in this case RelativePath is a copy of To)
//-----------------------------------------------------------------------------
BOOL CStdFileOperations::GetRelativePath(IN TCHAR* FromDirectory,IN TCHAR* ToDirectoryOrFile,OUT TCHAR* RelativePath)
{
    TCHAR* LocalFrom=NULL;
    TCHAR* pLocalFrom;
    TCHAR* LocalTo=NULL;
    TCHAR* pLocalTo;
    TCHAR* pRelativePath;
    SIZE_T SizeFrom;
    SIZE_T SizeTo;
    BOOL bRetValue = FALSE;

    if (FromDirectory == NULL)
    {
        BreakInDebugMode;
        return FALSE;
    }
    if (ToDirectoryOrFile == NULL)
    {
        BreakInDebugMode;
        return FALSE;
    }
    if (RelativePath == NULL)
    {
        BreakInDebugMode;
        return FALSE;
    }

    #define CStdFileOperations_MIN_PATH_LEN 2 // "c:" or "\\"

    SizeFrom = _tcslen(FromDirectory);
    if (SizeFrom<CStdFileOperations_MIN_PATH_LEN)
    {
        // no relative path can be made
        // copy full target path
        _tcscpy(RelativePath,ToDirectoryOrFile);
        goto CleanUp;
    }

    SizeTo = _tcslen(ToDirectoryOrFile);
    if (SizeTo<CStdFileOperations_MIN_PATH_LEN)
    {
        // no relative path can be made
        *RelativePath = 0;
        goto CleanUp;
    }

    LocalFrom = new TCHAR[SizeFrom+1];
    _tcscpy(LocalFrom,FromDirectory);
    LocalTo = new TCHAR[SizeTo+1];
    _tcscpy(LocalTo,ToDirectoryOrFile);

    pLocalFrom = LocalFrom;
    pLocalTo = LocalTo;

    /////////////////////////////////////////////////////////////////////////
    // File name could be like
    // "\\?\" to bypass MAX_PATH limitation
    // "\\server\"                  network path
    // "\\.\PhysicalDriveX"         Opens the physical drive X
    // "\\.\C:" 	                Opens the C: volume
    // "\\.\C:\" 	                Opens the file system of the C: volume.
    // "\\.\Changerx"
    // "\\.\TAPEx"
    /////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////
    // replace '/' by '\\'
    //////////////////////////////////////
    CStdFileOperations::AssumePathSplitterIsBackslash(pLocalFrom);
    CStdFileOperations::AssumePathSplitterIsBackslash(pLocalTo);

    //////////////////////////////////////
    // remove starting "\\?\" if any
    //////////////////////////////////////
    if (_tcsncmp(pLocalFrom,UNC_PREFIX,UNC_PREFIX_LEN)==0)
    {
        pLocalFrom+=UNC_PREFIX_LEN;
        SizeFrom-=UNC_PREFIX_LEN;
    }

    if (_tcsncmp(pLocalTo,UNC_PREFIX,UNC_PREFIX_LEN)==0)
    {
        pLocalTo+=UNC_PREFIX_LEN;
        SizeTo-=UNC_PREFIX_LEN;
    }

    /////////////////////////////////////////
    // check the easy case where FromDirectory is fully included into ToDirectoryOrFile
    /////////////////////////////////////////
    if (pLocalFrom[SizeFrom-1]=='\\') // take into account case of FromDirectory not ending with backslash and ToDirectoryOrFile ending with backslash
    {
        pLocalFrom[SizeFrom-1] = 0;
        SizeFrom--; 
    }

    if (_tcsnicmp(pLocalTo,pLocalFrom,SizeFrom) == 0)
    {
        TCHAR c;
        c =  pLocalTo[SizeFrom];
        if (c==0)
        {
            // ( c == 0 ) : avoid an empty string and set TEXT(".\\")
            _tcscpy( RelativePath, TEXT(".\\") );

            bRetValue = TRUE;
            goto CleanUp;
        }
        else
        {
            TCHAR c1 = pLocalTo[SizeFrom+1];
            if ( (c=='\\') && (c1==0) )
            {
                // (c=='\\') && (c1==0) ) :  avoid a single '\' when directory are the sames
                _tcscpy( RelativePath, TEXT(".\\") );

                bRetValue = TRUE;
                goto CleanUp;
            }
            // else

            // we must ensure that c == '\\'
            // else we are in the case TEXT("c:\\f0\\f1\\f2\\f3") vs TEXT("c:\\f0\\f1\\f2\\f3dum")
            if (c=='\\')
            {
                // From is fully included inside the To string
                _tcscpy(RelativePath,&pLocalTo[SizeFrom+1]);

                bRetValue = TRUE;
                goto CleanUp;
            }
        }
    }

    /////////////////////////////////////////
    // assume drive / server is the same
    /////////////////////////////////////////

    BOOL bOnSameDriveOrNetwork = FALSE;
    // check that files are on the same server or drive
    if ( CStdFileOperations::IsDrivePath(pLocalTo) && CStdFileOperations::IsDrivePath(pLocalFrom) )
    {
        if (*pLocalTo==*pLocalFrom)
            bOnSameDriveOrNetwork = TRUE;
    }
    else if ( CStdFileOperations::IsNetworkPath(pLocalTo) && CStdFileOperations::IsNetworkPath(pLocalFrom) )
    {
        // bypass the 2 first chars which are "\\"
        TCHAR* ServerNameFromEnd = _tcschr(pLocalFrom+2,'\\');
        TCHAR* ServerNameToEnd = _tcschr(pLocalTo+2,'\\');
        SIZE_T Size;

        if (ServerNameFromEnd == NULL)
            ServerNameFromEnd = pLocalFrom+_tcslen(pLocalFrom);
        else
            ServerNameFromEnd--; // remove '\'

        if (ServerNameToEnd == NULL)
            ServerNameToEnd = pLocalTo+_tcslen(pLocalTo);
        else
            ServerNameToEnd--; // remove '\'

        Size = ServerNameFromEnd-pLocalFrom;
        // if name size is the same
        if (Size == (SIZE_T)(ServerNameToEnd-pLocalTo))
        {
            if (_tcsncmp(pLocalFrom,pLocalTo,Size) == 0)
                bOnSameDriveOrNetwork = TRUE;
        }
    }

    if (bOnSameDriveOrNetwork == FALSE)
    {
        // no relative path can be made
        // copy full target path
        _tcscpy(RelativePath,ToDirectoryOrFile);
        goto CleanUp;
    }

    /////////////////////////////////////////
    // find common part
    /////////////////////////////////////////

    TCHAR* pszSplitterFrom;
    TCHAR* pszSplitterTo;

    TCHAR* pszPreviousSplitterFrom = pLocalFrom;
    TCHAR* pszPreviousSplitterTo = pLocalTo;
    BOOL bRestoreFromPathSplitter;
    BOOL bRestoreToPathSplitter;
    // find common part
    for(;pszPreviousSplitterFrom && pszPreviousSplitterTo;)
    {
        pszSplitterFrom = _tcschr(pszPreviousSplitterFrom+1,'\\');
        pszSplitterTo = _tcschr(pszPreviousSplitterTo+1,'\\');

        bRestoreFromPathSplitter = FALSE;
        bRestoreToPathSplitter = FALSE;
        if (pszSplitterFrom)
        {
            *pszSplitterFrom = 0;
            bRestoreFromPathSplitter = TRUE;
        }
        if (pszSplitterTo)
        {
            *pszSplitterTo = 0;
            bRestoreToPathSplitter = TRUE;
        }
        if(_tcsicmp(pszPreviousSplitterFrom+1,pszPreviousSplitterTo+1)!=0)
        {
            // check if we need to restore splitter (needed by further parsing)
            if (bRestoreFromPathSplitter)
                *pszSplitterFrom = '\\';
            if (bRestoreToPathSplitter)
                *pszSplitterTo = '\\';

            break;
        }

        pszPreviousSplitterFrom = pszSplitterFrom;
        pszPreviousSplitterTo = pszSplitterTo;
    }

    // get the upper directories
    *RelativePath =0;
    pRelativePath = RelativePath;
    for(;pszPreviousSplitterFrom;)
    {
        pszSplitterFrom = _tcschr(pszPreviousSplitterFrom+1,'\\');
        if (!pszSplitterFrom)
        {
            // if content
            if (*pszPreviousSplitterFrom+1)
            {
                // from directory is not ending with '\'
                // add last directory walk up
                _tcscpy(pRelativePath,_T("..\\"));
                pRelativePath+=3;
            }
            break;
        }
        pszPreviousSplitterFrom = pszSplitterFrom;

        // add directory walk up
        _tcscpy(pRelativePath,_T("..\\"));
        pRelativePath+=3;
    }
    // add relative part
    if (pszPreviousSplitterTo)
    {
        // in case relative part starts with '\'.
        if (*(pszPreviousSplitterTo+1) == '\\')
            pszPreviousSplitterTo++;

        _tcscpy(pRelativePath,pszPreviousSplitterTo+1);
    }


    bRetValue = TRUE;
CleanUp:
    delete[] LocalFrom;
    delete[] LocalTo;

    return bRetValue;
}

//-----------------------------------------------------------------------------
// Name: IsFileInDirectoryOrSubDirectory
// Object: check if FullFilePath is included in directory or subdirectory of "Directory"
//         we only check that FullFilePath starts with Directory
//         Notice : Do not check if file exists ! Use the CStdFileOperations::DoesFileExists in this case
//                  Checking is only done on paths string
// Parameters :
//      IN TCHAR* File : file to check
//      IN TCHAR* Directory : root of directories to check (can ends with '\\' or not)
// Return : TRUE if "File" belongs to directory or subdirectory of "Directory"
//-----------------------------------------------------------------------------
BOOL CStdFileOperations::IsFileInDirectoryOrSubDirectory(IN TCHAR* FullFilePath,IN TCHAR* Directory)
{
    if (FullFilePath == NULL)
    {
        BreakInDebugMode;
        return FALSE;
    }
    if (Directory == NULL)
    {
        BreakInDebugMode;
        return FALSE;
    }

    // only check if begin of path is the same
    SIZE_T DirectorySize = _tcslen(Directory);
    if (_tcsnicmp(FullFilePath,Directory,DirectorySize) != 0)
        return FALSE;
    // if directory ends with '\\'
    if (Directory[DirectorySize-1]=='\\')
        DirectorySize--; // we must access char at (DirectorySize-1) in File name for checking
    if (FullFilePath[DirectorySize]!='\\') // assume file has not directory prefix followed other alphanumerics chars 
        return FALSE;
    return TRUE;
}

BOOL CStdFileOperations::IsDriveLetter(IN TCHAR Letter)
{
    return ( ( 'a'<=Letter ) && ( Letter<='z' ) ) ||( ( 'A'<=Letter ) && ( Letter<='Z' ) );
}

//-----------------------------------------------------------------------------
// Name: IsRelativePath
// Object: check if path is relative (avoid to include extra shell lib)
// Parameters :
//      IN TCHAR* File : Path
// Return : TRUE if relative path
//-----------------------------------------------------------------------------
BOOL CStdFileOperations::IsRelativePath(IN TCHAR* Path)
{
    TCHAR FirstChar;
    if (Path==NULL)
    {
        BreakInDebugMode;
        return FALSE;
    }

    if (*Path == 0)
    {
        BreakInDebugMode;
        return FALSE;
    }
#if (defined(UNICODE)||defined(_UNICODE))

#else
    if (::IsDBCSLeadByte(*Path))
        return FALSE;
#endif

    /////////////////////////////////////////////
    // looks for full path and return (!fullpath)
    /////////////////////////////////////////////

    CStdFileOperations::AssumePathSplitterIsBackslash(Path);

    FirstChar = *Path;

    // if UNC_PREFIX or network or volume or physical drive path
    if (FirstChar=='\\')
    {
        return FALSE;
    }

    // check local drive path
    // if second char is not ':', that means path is not drive path
    if (Path[1] != ':')
        return TRUE;

    // second char is ':', check if first char is a drive letter
    // if it is a drive letter, that means path is absolute
    return !CStdFileOperations::IsDriveLetter(FirstChar);
}
//-----------------------------------------------------------------------------
// Name: GetUpperDirectory
// Object: Notice : provided path is directly affected
// Parameters :
//      IN OUT TCHAR* File : in : Path, out : upper directory
// Return : TRUE if upper directory exists
//-----------------------------------------------------------------------------
BOOL CStdFileOperations::GetUpperDirectory(IN OUT TCHAR* Path)
{
    TCHAR* psz;

    if (Path == NULL)
    {
        BreakInDebugMode;
        return FALSE;
    }
    if (*Path == 0)
    {
        BreakInDebugMode;
        return FALSE;
    }

    CStdFileOperations::AssumePathSplitterIsBackslash(Path);

    // if path is a directory, remove ending '\\' if any
    SIZE_T PathSize = _tcslen(Path);
    if (Path[PathSize-1] == '\\')
        Path[PathSize-1] = 0;

    // get upper dir
    psz=_tcsrchr(Path,'\\');
    if (psz==NULL)
        return FALSE;

    // if '\\' is first char
    if (psz == Path)
        return FALSE;

    // check for network drive begin
    if (*(psz-1) == '\\')
        return FALSE;

#if (defined(UNICODE)||defined(_UNICODE))
    // check for long filenames
    // aka check for directories beginning with TEXT("\\\\?\\")
    if (*(psz-1) == '?')
        return FALSE;
#endif

    // ends Path so it contains upper directory
    *psz = 0;

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: IsFileOpen
// Object: check if file is open by current or another application
// Parameters :
//      IN TCHAR* File : Path
// Return : TRUE if file is opened by someone else
//-----------------------------------------------------------------------------
BOOL CStdFileOperations::IsFileOpen(IN TCHAR* Path)
{
    if (Path == NULL)
    {
        BreakInDebugMode;
        return FALSE;
    }
    if (*Path == 0)
    {
        BreakInDebugMode;
        return FALSE;
    }

    HANDLE Handle;
    Handle = ::CreateFile(Path, GENERIC_READ, 0 /* no sharing! exclusive */, NULL, OPEN_EXISTING, 0, NULL);
    if (Handle != INVALID_HANDLE_VALUE)
    {
        ::CloseHandle(Handle);
        return FALSE;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: IsFileOpenInWriteMode
// Object: check if file is open in write mode by current or another application
// Parameters :
//      IN TCHAR* File : Path
// Return : TRUE if file is opened by someone else
//-----------------------------------------------------------------------------
BOOL CStdFileOperations::IsFileOpenInWriteMode(IN TCHAR* Path)
{
    if (Path == NULL)
    {
        BreakInDebugMode;
        return FALSE;
    }
    if (*Path == 0)
    {
        BreakInDebugMode;
        return FALSE;
    }

    HANDLE Handle;
    Handle = ::CreateFile(Path, GENERIC_READ, FILE_SHARE_READ /* no sharing write! exclusive */, NULL, OPEN_EXISTING, 0, NULL);
    if (Handle != INVALID_HANDLE_VALUE)
    {
        ::CloseHandle(Handle);
        return FALSE;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: HasReadOnlyAttribute
// Object: check if file has the read only attribute
// Parameters :
//      IN TCHAR* File : Path
// Return : TRUE if file has the read only attribute
//-----------------------------------------------------------------------------
BOOL CStdFileOperations::HasReadOnlyAttribute(IN TCHAR* Path)
{
    if (Path == NULL)
    {
        BreakInDebugMode;
        return FALSE;
    }
    if (*Path == 0)
    {
        BreakInDebugMode;
        return FALSE;
    }

    DWORD FileAttributs;
    FileAttributs = ::GetFileAttributes(Path);
    if (FileAttributs == INVALID_FILE_ATTRIBUTES)
        return FALSE;
    return ( (FileAttributs & FILE_ATTRIBUTE_READONLY) != 0 );
}

//-----------------------------------------------------------------------------
// Name: RemoveReadOnlyAttribute
// Object: remove file read only attribute
// Parameters :
//      IN TCHAR* File : Path
// Return : TRUE if file read only attribute has been removed
//-----------------------------------------------------------------------------
BOOL CStdFileOperations::RemoveReadOnlyAttribute(IN TCHAR* Path)
{
    if (Path == NULL)
    {
        BreakInDebugMode;
        return FALSE;
    }
    if (*Path == 0)
    {
        BreakInDebugMode;
        return FALSE;
    }

    DWORD FileAttributs;
    FileAttributs = ::GetFileAttributes(Path);
    if (FileAttributs == INVALID_FILE_ATTRIBUTES)
        return FALSE;
    if (FileAttributs & FILE_ATTRIBUTE_READONLY)
    {
        FileAttributs &= ~FILE_ATTRIBUTE_READONLY;
        return ::SetFileAttributes(Path,FileAttributs);
    } 
    else
        return TRUE;
}

//-----------------------------------------------------------------------------
// Name: IsWritable
// Object: check if file can be written
// Parameters :
//      IN TCHAR* File : Path
// Return : TRUE if file is writable
//-----------------------------------------------------------------------------
BOOL CStdFileOperations::IsWritable(IN TCHAR* Path)
{
    if (Path == NULL)
    {
        BreakInDebugMode;
        return FALSE;
    }
    if (*Path == 0)
    {
        BreakInDebugMode;
        return FALSE;
    }

    HANDLE Handle;
    if (!CStdFileOperations::DoesFileExists(Path))
    {
        if (!CStdFileOperations::CreateDirectoryForFile(Path))
            return FALSE;
    }
    Handle = ::CreateFile(Path, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (Handle != INVALID_HANDLE_VALUE)
    {
        ::CloseHandle(Handle);
        return TRUE;
    }
    return FALSE;
}

//-----------------------------------------------------------------------------
// Name: AssumeDirectoryEndsWithBackSlash
// Object: Assume provided directory name ends with '\'
//          if it's not the case, '\' will be added at the end of Directory
// Parameters :
//      IN TCHAR* Directory : Directory
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CStdFileOperations::AssumeDirectoryEndsWithBackSlash(IN OUT TCHAR* Directory)
{
    if (Directory==NULL)
    {
        BreakInDebugMode;
        return FALSE;
    }

    if (*Directory==0) // assume that length > 0
    {
        BreakInDebugMode;
        return FALSE;
    }

    SIZE_T Len = _tcslen(Directory);
    TCHAR* pLastChar = &Directory[Len-1];
    // _tcscat(Directory,TEXT("\\"));
    if (*pLastChar!='\\')
    {
        pLastChar++;
        *pLastChar = '\\';
        pLastChar++;
        *pLastChar = 0;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: RemoveDirectoryEndingBackSlashIfAny
// Object: Assume provided directory name won't ends with '\'
//          if it's not the case, '\' will be removed from the end of Directory
// Parameters :
//      IN TCHAR* Directory : Directory
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CStdFileOperations::RemoveDirectoryEndingBackSlashIfAny(IN OUT TCHAR* Directory)
{
    if (Directory==NULL)
    {
        BreakInDebugMode;
        return FALSE;
    }

    if (*Directory==0) // assume that length > 0
    {
        BreakInDebugMode;
        return FALSE;
    }

    SIZE_T Len = _tcslen(Directory);
    TCHAR* pLastChar = &Directory[Len-1];
    if (*pLastChar=='\\')
        *pLastChar = 0;

    return TRUE;
}


BOOL CStdFileOperations::CopyFile(IN TCHAR* ExistingFilePath,IN TCHAR* NewFilePath,IN BOOL bFailIfExists)
{
    return ::CopyFile(ExistingFilePath,NewFilePath,bFailIfExists);
}

BOOL CStdFileOperations::CopyDirectory(IN TCHAR* ExistingDirectoryPath,IN TCHAR* NewDirectoryPath,IN BOOL bFailIfExists)
{
    if (bFailIfExists)
    {
        if (CStdFileOperations::DoesDirectoryExists(NewDirectoryPath))
            return FALSE;
    }
    else
    {
        if (!CStdFileOperations::DoesDirectoryExists(NewDirectoryPath))
        {
            if (!CStdFileOperations::CreateDirectory(NewDirectoryPath))
                return FALSE;
        }
    }

    BOOL bSuccess = TRUE;
    BOOL bTmpRet;

    TCHAR* SrcPath;
    TCHAR* DestPath;
    TCHAR* pSubDirPath;
    SIZE_T ExistingDirectoryPathLength = _tcslen(ExistingDirectoryPath);
    SIZE_T NewDirectoryPathLength = _tcslen(NewDirectoryPath);

    // FindFirstFile API need something like "Mydir\\*", so ensure that dir ends with '\\' and add the '*' joker
    TCHAR* SearchContent;
    TCHAR* pSearchContent;
    SIZE_T SearchContentAllocatedSize = __max(ExistingDirectoryPathLength+2,2048);// +2 = +1 for '\\' and +1 for '*'
    SearchContent = new TCHAR[SearchContentAllocatedSize];
    pSearchContent = SearchContent;
    _tcscpy(pSearchContent,ExistingDirectoryPath);
    pSearchContent+=ExistingDirectoryPathLength;
    if (*(pSearchContent-1) != '\\')
    {
        _tcscpy(pSearchContent,TEXT("\\"));
        pSearchContent++;
    }
    _tcscpy(pSearchContent,TEXT("*"));

    // allocate memory for source file/subdir
    SIZE_T SrcPathAllocatedSize = __max(ExistingDirectoryPathLength+MAX_PATH,2048);
    SrcPath = new TCHAR[SrcPathAllocatedSize];
    pSubDirPath = SrcPath;
    _tcscpy(pSubDirPath,ExistingDirectoryPath);
    pSubDirPath+=ExistingDirectoryPathLength;
    if (*(pSubDirPath-1) != '\\')
    {
        _tcscpy(pSubDirPath,TEXT("\\"));
        pSubDirPath++;
        ExistingDirectoryPathLength++;
    }

    // allocate memory for destination file/subdir
    SIZE_T DestPathAllocatedSize = __max(NewDirectoryPathLength+MAX_PATH,2048);
    DestPath = new TCHAR[DestPathAllocatedSize];
    pSubDirPath = DestPath;
    _tcscpy(pSubDirPath,NewDirectoryPath);
    pSubDirPath+=NewDirectoryPathLength;
    if (*(pSubDirPath-1) != '\\')
    {
        _tcscpy(pSubDirPath,TEXT("\\"));
        pSubDirPath++;
        NewDirectoryPathLength++;
    }

    //////////////////////////////////////////////
    // 1) find files in current directory
    //////////////////////////////////////////////
    // find first file
    WIN32_FIND_DATA FindFileData={0};
    HANDLE hFileHandle=INVALID_HANDLE_VALUE;
    hFileHandle=::FindFirstFile(SearchContent,&FindFileData);
    if (hFileHandle!=INVALID_HANDLE_VALUE)
    {
        do 
        {
            // forge source file/subdir full path
            // copy at position to avoid to copy root directory on each loop
            pSubDirPath = &SrcPath[ExistingDirectoryPathLength];
            _tcscpy(pSubDirPath,FindFileData.cFileName);

            // forge destination file/subdir full path
            // copy at position to avoid to copy root directory on each loop
            pSubDirPath = &DestPath[NewDirectoryPathLength];
            _tcscpy(pSubDirPath,FindFileData.cFileName);

            // remove current and upper dir
            if ( FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                if ( (_tcscmp(FindFileData.cFileName,TEXT("."))==0) || (_tcscmp(FindFileData.cFileName,TEXT(".."))==0) )
                    continue;

                bTmpRet = CStdFileOperations::CopyDirectory(SrcPath,DestPath,FALSE); // if bFailIfExists is TRUE, parent dir exists and function has already returned
                bSuccess = bSuccess && bTmpRet;
            }
            else
            {
                bTmpRet = CStdFileOperations::CopyFile(SrcPath,DestPath,FALSE); // if bFailIfExists is TRUE, parent dir exists and function has already returned
                bSuccess = bSuccess && bTmpRet;
            }

            // find next file
        } while(::FindNextFile(hFileHandle, &FindFileData));

        ::FindClose(hFileHandle);
        // prepare for next goto cleanup
        hFileHandle = INVALID_HANDLE_VALUE;
    }

    delete[] SearchContent;
    delete[] DestPath;
    delete[] SrcPath;
    

    return bSuccess;
}
