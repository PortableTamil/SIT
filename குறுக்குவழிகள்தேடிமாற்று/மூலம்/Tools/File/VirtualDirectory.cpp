/*
Copyright (C) 2014 Jacquelin POTIER <jacquelin.potier@free.fr>
Dynamic aspect ratio code Copyright (C) 2014 Jacquelin POTIER <jacquelin.potier@free.fr>

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
// Object: Virtual Directory (symbolic link and junction) helper
//-----------------------------------------------------------------------------

#include "VirtualDirectory.h"
#include "StdFileOperations.h"

#include <winternl.h>

// depending of windows sdk used by Visual studio, some defines are missing
#ifndef InitializeObjectAttributes 
#define InitializeObjectAttributes( p, n, a, r, s ) { \
    (p)->Length = sizeof( OBJECT_ATTRIBUTES );          \
    (p)->RootDirectory = r;                             \
    (p)->Attributes = a;                                \
    (p)->ObjectName = n;                                \
    (p)->SecurityDescriptor = s;                        \
    (p)->SecurityQualityOfService = NULL;               \
    }
#endif
#ifndef OBJ_INHERIT
#define OBJ_INHERIT 0x00000002L
#endif
#ifndef FILE_OPEN_REPARSE_POINT
#define FILE_OPEN_REPARSE_POINT 0x00200000
#endif
#ifndef FILE_OPEN
#define FILE_OPEN 0x00000001
#endif

typedef NTSTATUS (WINAPI *pfNtFsControlFile)(
    _In_       HANDLE FileHandle,
    _In_opt_   HANDLE Event,
    _In_opt_   PVOID ApcRoutine,
    _In_opt_   PVOID ApcContext,
    _Out_      PIO_STATUS_BLOCK IoStatusBlock,
    _In_       ULONG FsControlCode,
    _In_opt_   PVOID InputBuffer,
    _In_       ULONG InputBufferLength,
    _Out_opt_  PVOID OutputBuffer,
    _In_       ULONG OutputBufferLength
    );

typedef NTSTATUS (WINAPI *pfNtCreateFile)(
    _Out_     PHANDLE FileHandle,
    _In_      ACCESS_MASK DesiredAccess,
    _In_      POBJECT_ATTRIBUTES ObjectAttributes,
    _Out_     PIO_STATUS_BLOCK IoStatusBlock,
    _In_opt_  PLARGE_INTEGER AllocationSize,
    _In_      ULONG FileAttributes,
    _In_      ULONG ShareAccess,
    _In_      ULONG CreateDisposition,
    _In_      ULONG CreateOptions,
    _In_      PVOID EaBuffer,
    _In_      ULONG EaLength
    );

typedef struct _RTLP_CURDIR_REF
{
    LONG RefCount;
    HANDLE Handle;
} RTLP_CURDIR_REF, *PRTLP_CURDIR_REF;

typedef struct _RTL_RELATIVE_NAME_U
{
    UNICODE_STRING RelativeName;
    HANDLE ContainingDirectory;
    PRTLP_CURDIR_REF CurDirRef;
} RTL_RELATIVE_NAME_U, *PRTL_RELATIVE_NAME_U;

typedef NTSTATUS (WINAPI *pfRtlDosPathNameToNtPathName_U)(PWSTR DosFileName,PUNICODE_STRING NtFileName,PWSTR* FilePart,PRTL_RELATIVE_NAME_U RelativeName);
typedef struct _REPARSE_DATA_BUFFER {
    ULONG  ReparseTag;
    USHORT ReparseDataLength;
    USHORT Reserved;
    union {
        struct {
            USHORT SubstituteNameOffset;
            USHORT SubstituteNameLength;
            USHORT PrintNameOffset;
            USHORT PrintNameLength;
            ULONG  Flags;
            WCHAR  PathBuffer[1];
        } SymbolicLinkReparseBuffer;
        struct {
            USHORT SubstituteNameOffset;
            USHORT SubstituteNameLength;
            USHORT PrintNameOffset;
            USHORT PrintNameLength;
            WCHAR  PathBuffer[1];
        } MountPointReparseBuffer;
        struct {
            UCHAR DataBuffer[1];
        } GenericReparseBuffer;
    };
} REPARSE_DATA_BUFFER, *PREPARSE_DATA_BUFFER;
#define STATUS_SUCCESS 0
#define MAXIMUM_REPARSE_DATA_BUFFER_SIZE   ( 16 * 1024 )
BOOL CVirtualDirectory::GetRealPathFromVirtalDirectory(IN TCHAR* VirtualDirectory,OUT TCHAR** pRealPath)
{
    /* 
    GetRealPathFromVirtalDirectory
    Copyright (C) 2004-2014 Jacquelin POTIER <jacquelin.potier@free.fr>
    Dynamic aspect ratio code Copyright (C) 2004-2014 Jacquelin POTIER <jacquelin.potier@free.fr>
    */
    pfNtFsControlFile pNtFsControlFile;
    pfNtCreateFile pNtCreateFile;
    pfRtlDosPathNameToNtPathName_U pRtlDosPathNameToNtPathName_U;
    BOOL bRetValue = FALSE;
    HANDLE FileHandle = NULL;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    PREPARSE_DATA_BUFFER ReparseDataBuffer = NULL;
    WCHAR* wVirtualDirectory=NULL;
    WCHAR* wVirtualDirectoryLongName = NULL;
    WCHAR* wRealPath;
    HMODULE hModule;
    SIZE_T LenInCharCount;
    UNICODE_STRING uStrNtName;
    USHORT OffsetInCharCount;
    
    *pRealPath=NULL;

    hModule = ::GetModuleHandle(TEXT("ntdll.dll"));
    if (!hModule)
        goto CleanUp;
    pNtFsControlFile = (pfNtFsControlFile)::GetProcAddress(hModule,"NtFsControlFile");
    pNtCreateFile = (pfNtCreateFile)::GetProcAddress(hModule,"NtCreateFile");
    pRtlDosPathNameToNtPathName_U = (pfRtlDosPathNameToNtPathName_U)::GetProcAddress(hModule,"RtlDosPathNameToNtPathName_U");
    if ( (pNtFsControlFile==NULL) || (pNtCreateFile==NULL) || (pRtlDosPathNameToNtPathName_U==NULL))
        goto CleanUp;

    LenInCharCount = _tcslen(VirtualDirectory);

#if (defined(UNICODE)||defined(_UNICODE))
    wVirtualDirectory = VirtualDirectory;
#else
    wVirtualDirectory = new WCHAR[LenInCharCount+1];
    ::MultiByteToWideChar(CP_ACP, 0,VirtualDirectory,(int)LenInCharCount,wVirtualDirectory,(int)LenInCharCount);
    wVirtualDirectory[LenInCharCount]=0;
#endif
	// wVirtualDirectory=L"\\??\\C:\\ShortcutsSearchAndReplace\\_testing_\\VirtualFolder";
    uStrNtName.Buffer = NULL;
    uStrNtName.Length = 0;
    uStrNtName.MaximumLength = 0;
    pRtlDosPathNameToNtPathName_U(wVirtualDirectory,&uStrNtName,NULL,NULL);
    wVirtualDirectoryLongName = uStrNtName.Buffer;
    InitializeObjectAttributes(&ObjAttributes,&uStrNtName,OBJ_INHERIT,NULL,NULL);
    
    Status = pNtCreateFile(&FileHandle,FILE_GENERIC_READ,&ObjAttributes,&IoStatusBlock,0,0,FILE_SHARE_READ,FILE_OPEN,FILE_OPEN_REPARSE_POINT,NULL,0);
    if (Status!=STATUS_SUCCESS)
        goto CleanUp;
     ReparseDataBuffer =(PREPARSE_DATA_BUFFER) new BYTE[MAXIMUM_REPARSE_DATA_BUFFER_SIZE];
     Status = pNtFsControlFile(FileHandle,
                                NULL,
                                NULL,
                                NULL,
                                &IoStatusBlock,
                                FSCTL_GET_REPARSE_POINT,
                                NULL,
                                0,
                                ReparseDataBuffer,
                                MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
     if (Status!=STATUS_SUCCESS)
         goto CleanUp;
     if (ReparseDataBuffer->ReparseTag == IO_REPARSE_TAG_MOUNT_POINT)
     {
         OffsetInCharCount = ReparseDataBuffer->MountPointReparseBuffer.PrintNameOffset/sizeof(WCHAR);
         LenInCharCount = ReparseDataBuffer->MountPointReparseBuffer.PrintNameLength/sizeof(WCHAR);
         wRealPath = new WCHAR[LenInCharCount+1];
         memcpy(wRealPath,
                &ReparseDataBuffer->MountPointReparseBuffer.PathBuffer[OffsetInCharCount],
                ReparseDataBuffer->MountPointReparseBuffer.PrintNameLength
                );
         wRealPath[LenInCharCount]=0;
     }
     else if (ReparseDataBuffer->ReparseTag == IO_REPARSE_TAG_SYMLINK)
     {
        OffsetInCharCount = ReparseDataBuffer->SymbolicLinkReparseBuffer.PrintNameOffset/sizeof(WCHAR);
        LenInCharCount = ReparseDataBuffer->SymbolicLinkReparseBuffer.PrintNameLength/sizeof(WCHAR);
        wRealPath = new WCHAR[LenInCharCount+1];
        memcpy(wRealPath,
               &ReparseDataBuffer->SymbolicLinkReparseBuffer.PathBuffer[OffsetInCharCount],
               ReparseDataBuffer->SymbolicLinkReparseBuffer.PrintNameLength
               );
        wRealPath[LenInCharCount]=0;
     }
     else
         goto CleanUp;

    // put real path into output buffer
#if (defined(UNICODE)||defined(_UNICODE))
    *pRealPath = wRealPath;
#else
    *pRealPath = new TCHAR[LenInCharCount+1];
    ::WideCharToMultiByte(CP_ACP, 0, wRealPath, (int)LenInCharCount, *pRealPath, (int)LenInCharCount, NULL, NULL);
    (*pRealPath)[LenInCharCount]=0;
    delete[] wRealPath;
#endif


    bRetValue = TRUE;
CleanUp:
#if ( (!defined(UNICODE)) && (!defined(_UNICODE)))
    if (wVirtualDirectory)
        delete[] wVirtualDirectory;
#endif
    if (wVirtualDirectoryLongName)
        ::HeapFree(::GetProcessHeap(), 0, wVirtualDirectoryLongName);
    if (FileHandle)
        ::CloseHandle(FileHandle);
    if (ReparseDataBuffer)
       delete[] ReparseDataBuffer;
     return bRetValue;
}

BOOL CVirtualDirectory::IsVirtualDirectory(TCHAR* FullPath)
{
    DWORD Attributes = ::GetFileAttributes(FullPath);
    if (Attributes == INVALID_FILE_ATTRIBUTES)
        return FALSE;
    if ((Attributes & FILE_ATTRIBUTE_DIRECTORY)==0)
        return FALSE;
    return ( (Attributes & FILE_ATTRIBUTE_REPARSE_POINT) == FILE_ATTRIBUTE_REPARSE_POINT);
}

BOOL CVirtualDirectory::IsContainingVirtualDirectory(TCHAR* FullPath)
{
    TCHAR* LocalDirectory;
    BOOL bRetValue;

    bRetValue = FALSE;
    LocalDirectory = _tcsdup(FullPath);

    for(;;)
    {
        if (CVirtualDirectory::IsVirtualDirectory(LocalDirectory))
        {
            bRetValue = TRUE;
            break;
        }
        // get upper dir
        if (!CStdFileOperations::GetUpperDirectory(LocalDirectory))
            break;
    }

    free(LocalDirectory);
    return bRetValue;
}
BOOL CVirtualDirectory::GetRealPath(IN TCHAR* PathContainingVirtualDirectory,OUT TCHAR* RealPath,IN SIZE_T MaxRealPathSizeInCharCount)
{
    *RealPath = 0;

    if (PathContainingVirtualDirectory==NULL)
        return FALSE;
    if (*PathContainingVirtualDirectory==0)
        return FALSE;

    if (!CVirtualDirectory::IsContainingVirtualDirectory(PathContainingVirtualDirectory))
    {
        _tcsncpy(RealPath,PathContainingVirtualDirectory,MaxRealPathSizeInCharCount);
        RealPath[MaxRealPathSizeInCharCount-1]=0;
        return TRUE;
    }

    TCHAR* LocalDirectory;  // buffer reduced at each GetUpperDirectory call
    TCHAR* ReferenceString; // unchanged buffer used to get ending path
    TCHAR* TmpLocalDirectory;
    TCHAR* EndingPath;
    TCHAR* TmpRealPath;
    SIZE_T TmpRealPathSize;
    SIZE_T Size;

    Size = _tcslen(PathContainingVirtualDirectory);

    LocalDirectory = new TCHAR [Size+1];
    _tcscpy(LocalDirectory,PathContainingVirtualDirectory);

    ReferenceString = new TCHAR [Size+1];
    _tcscpy(ReferenceString,PathContainingVirtualDirectory);

    for(;;)
    {
        if (CVirtualDirectory::IsVirtualDirectory(LocalDirectory))
        {
            // store ending path (path after virtual folder)
            EndingPath = &ReferenceString[_tcslen(LocalDirectory)];

            // get virtual folder real path
            if (!CVirtualDirectory::GetRealPathFromVirtalDirectory(LocalDirectory,&TmpRealPath))
            {
                // in case of failure copy back virtual directory to real directory in case user don't check current function return
                _tcsncpy(RealPath,PathContainingVirtualDirectory,MaxRealPathSizeInCharCount);
                RealPath[MaxRealPathSizeInCharCount-1]=0;

                delete[] LocalDirectory;
                delete[] ReferenceString;
                return FALSE;
            }
            // if virtual folder real path is relative path
            if (CStdFileOperations::IsRelativePath(TmpRealPath))
            {
                TCHAR* CurrentDirectory=new TCHAR[CVirtualDirectory_MAX_PATH_SIZE];
                // save current directory
                ::GetCurrentDirectory(CVirtualDirectory_MAX_PATH_SIZE,CurrentDirectory);

                // set upper directory as current directory
                TCHAR* TmpUpperDirectory=_tcsdup(LocalDirectory);
                // get upper dir
                CStdFileOperations::GetUpperDirectory(TmpUpperDirectory);
                ::SetCurrentDirectory(TmpUpperDirectory);// use path before current virtual directory addition as reference path

                // convert from relative path to full path
                TCHAR* AbsolutePath=new TCHAR[CVirtualDirectory_MAX_PATH_SIZE];
                // _tfullpath can be used too for more portable way (in this file we call as much API as we can to be linked only to kernel32)
                BOOL bRet = (::GetFullPathName(TmpRealPath,CVirtualDirectory_MAX_PATH_SIZE,AbsolutePath,NULL)!=0);

                free(TmpUpperDirectory);

                // restore current directory
                ::SetCurrentDirectory(CurrentDirectory);

                delete[] CurrentDirectory;
                delete[] TmpRealPath;

                // update TmpRealPath
                TmpRealPath = AbsolutePath;

                if (!bRet)
                {
                    // keep virtual path
                    _tcsncpy(RealPath,PathContainingVirtualDirectory,MaxRealPathSizeInCharCount);
                    RealPath[MaxRealPathSizeInCharCount-1]=0;

                    delete[] TmpRealPath;
                    delete[] LocalDirectory;
                    delete[] ReferenceString;
                    return FALSE;
                }
            }
            
            // merge real folder absolute path with ending path
            Size = _tcslen(EndingPath);
            
            TmpRealPathSize = _tcslen(TmpRealPath);
            TmpLocalDirectory = new TCHAR[TmpRealPathSize+Size+1];
            _tcscpy(TmpLocalDirectory,TmpRealPath);
            if (Size>0)
            {
                // _tcscat(TmpLocalDirectory,EndingPath);
                _tcscpy(&TmpLocalDirectory[TmpRealPathSize],EndingPath);
            }
            delete[] TmpRealPath;
            delete[] LocalDirectory;

            // update local directory
            LocalDirectory = TmpLocalDirectory;

            // update reference string
            delete[] ReferenceString;
            Size = _tcslen(LocalDirectory);
            ReferenceString = new TCHAR[Size+1];
            _tcscpy(ReferenceString,LocalDirectory);
            // restart virtual directories searching into new path
            continue;
        }

        // get upper dir
        if (!CStdFileOperations::GetUpperDirectory(LocalDirectory))
            break;
    }
    

    _tcsncpy(RealPath,ReferenceString,MaxRealPathSizeInCharCount);
    RealPath[MaxRealPathSizeInCharCount-1]=0;

    // BOOL bDirectoryExists = CStdFileOperations::DoesDirectoryExists(RealPath);

    delete[] LocalDirectory;
    delete[] ReferenceString;

    return TRUE;
}