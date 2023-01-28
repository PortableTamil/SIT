/*
Copyright (C) 2020 Jacquelin POTIER <jacquelin.potier@free.fr>
Dynamic aspect ratio code Copyright (C) 2017 Jacquelin POTIER <jacquelin.potier@free.fr>

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

#include "GenericTryCatchWithExceptionReport.h"
#include "../CrashDump/CrashDump.h"
#include "../Version/Version.h"
#include "../File/StdFileOperations.h"
#include "../String/OutputString.h"

BOOL CGenericTryCatchWithExceptionReport::bContinueToReportExceptions = TRUE;

void CGenericTryCatchWithExceptionReport::GenerateCrashDumpAndReportExceptionToAuthor(CExceptionHardware e)
{
    if (CGenericTryCatchWithExceptionReport::bContinueToReportExceptions==FALSE)
        return;

    // dynamically load MessageBox function (avoid static linking)
    HMODULE hModuleUser32 = ::LoadLibrary(_T("user32.dll"));
    HMODULE hModuleShell32 = ::LoadLibrary(_T("Shell32.dll"));

    if ( (hModuleUser32==NULL) || (hModuleShell32==NULL) )
        goto CleanUp;

    typedef int (WINAPI *pfMessageBoxW)(HWND hWnd,LPCWSTR lpText,LPCWSTR lpCaption,UINT uType);
    pfMessageBoxW pMessageBoxW = (pfMessageBoxW)::GetProcAddress(hModuleUser32,"MessageBoxW");

    typedef HINSTANCE (WINAPI *pfShellExecute)(HWND hwnd,LPCTSTR lpOperation,LPCTSTR lpFile,LPCTSTR lpParameters,LPCTSTR lpDirectory,INT nShowCmd);
#if ( defined(UNICODE) || defined(_UNICODE))
    pfShellExecute pShellExecute = (pfShellExecute)::GetProcAddress(hModuleUser32,"ShellExecuteW");
#else
    pfShellExecute pShellExecute = (pfShellExecute)::GetProcAddress(hModuleUser32,"ShellExecuteA");
#endif

    if (pMessageBoxW && pShellExecute)
    {
        if (
            pMessageBoxW(NULL,
                        L"An exception has occurred.\r\nDo you want to save a crash dump file to report error to author ?",
                        L"Exception",
                        MB_ICONQUESTION | MB_YESNO
                        )
            == IDYES
            )
        {
            CCrashDump::GenerateCrashDump(MiniDumpNormal,e.pExceptionInformation);

            TCHAR ModuleFullPath[MAX_PATH];
            TCHAR PrettyVersion[64];
            CStdFileOperations::GetCurrentModuleDirectory(ModuleFullPath,_countof(ModuleFullPath));
            CVersion Version;
            Version.Read(ModuleFullPath);
            *PrettyVersion=0;
            CVersion::GetPrettyVersion(Version.ProductVersion,3,PrettyVersion,_countof(PrettyVersion));

            // see RFC 2368 for mailto protocol
            COutputString OutputString;
            OutputString.Append(TEXT("mailto:jacquelin.potier@free.fr?"));
            OutputString.Append(TEXT("&subject="));
            OutputString.AppendSnprintf(256,TEXT("%s v%s Bug Report"),CStdFileOperations::GetFileName(ModuleFullPath),PrettyVersion);
            OutputString.AppendSnprintf(256,TEXT("&body=An exception occurred at address %p"),
    #ifdef _WIN64
                                        e.pExceptionInformation->ContextRecord->Rip
    #else
                                        e.pExceptionInformation->ContextRecord->Eip
    #endif
                                        );
            OutputString.Append(
                                TEXT("%0D%0APlease attach the generated dump file and provide the most possible details about the bug")
                                TEXT("All these details will be useful for a quicker bug correction%0D%0A%0D%0A")
                                TEXT("Thanks for the report%0D%0A")
                                TEXT("Jacquelin")
                                );

            if (pShellExecute(NULL,TEXT("open"),OutputString.GetString(),NULL,NULL,SW_SHOWNORMAL)<(HINSTANCE)33)
            {
                pMessageBoxW(NULL,L"Error launching default mail client",L"Error",MB_OK|MB_ICONERROR);
            }
        }
        else
            CGenericTryCatchWithExceptionReport::bContinueToReportExceptions = FALSE;
    }
CleanUp:
    if (hModuleUser32)
        ::FreeLibrary(hModuleUser32);
    if (hModuleShell32)
        ::FreeLibrary(hModuleShell32);
}
