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

#pragma once

#include "HardwareException.h"

// WARNING BREAK POINTS ARE NOT CATCHED by CGenericTryCatchWithExceptionReport_TRY !!!
// that means if you call function like ::DebugBreak() excpetion is not catched and make your software crash !!!
class CGenericTryCatchWithExceptionReport
{
public:
    static BOOL bContinueToReportExceptions;
    static void GenerateCrashDumpAndReportExceptionToAuthor(CExceptionHardware e);
};

// Notice the macros are provided for examples, you can extend them like 
//  #define MY_SPECIFIC_CATCH } catch( CExceptionHardware e ){MyExtraFct(e);CGenericTryCatchWithExceptionReport::GenerateCrashDumpAndReportExceptionToAuthor(e);}

#ifdef _DEBUG
    // let debugger catch exception
    #define CGenericTryCatchWithExceptionReport_TRY
    #define CGenericTryCatchWithExceptionReport_CATCH
    #define CGenericTryCatchWithExceptionReport_CATCH_AND_RETURN_FALSE
    #define CGenericTryCatchWithExceptionReport_CATCH_AND_RETURN_NULL
    #define CGenericTryCatchWithExceptionReport_CATCH_AND_RETURN(x)
#else
    #define CGenericTryCatchWithExceptionReport_TRY try { CExceptionHardware::RegisterTry();
    #define CGenericTryCatchWithExceptionReport_CATCH } catch( CExceptionHardware e ){CGenericTryCatchWithExceptionReport::GenerateCrashDumpAndReportExceptionToAuthor(e);}
    #define CGenericTryCatchWithExceptionReport_CATCH_AND_RETURN_FALSE } catch( CExceptionHardware e ){CGenericTryCatchWithExceptionReport::GenerateCrashDumpAndReportExceptionToAuthor(e);return FALSE;}
    #define CGenericTryCatchWithExceptionReport_CATCH_AND_RETURN_NULL } catch( CExceptionHardware e ){CGenericTryCatchWithExceptionReport::GenerateCrashDumpAndReportExceptionToAuthor(e);return NULL;}
    #define CGenericTryCatchWithExceptionReport_CATCH_AND_RETURN(x) } catch( CExceptionHardware e ){CGenericTryCatchWithExceptionReport::GenerateCrashDumpAndReportExceptionToAuthor(e);return x;}
#endif
