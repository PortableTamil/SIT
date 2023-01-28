#pragma once

#include <windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

#pragma warning (push)
#pragma warning(disable : 4995)
#include <regex>
#pragma warning (pop)

// MSVC++ 10.0  _MSC_VER == 1600 (Visual Studio 2010 version 10.0)
// MSVC++ 9.0   _MSC_FULL_VER == 150030729 (Visual Studio 2008, SP1)
// MSVC++ 9.0   _MSC_VER == 1500 (Visual Studio 2008 version 9.0)
#if (_MSC_VER < 1600)
    #if (_MSC_VER < 1500)
        #error "vs 2008 sp1 required for std::regex"
    #else
        #if (_MSC_FULL_VER<150030729)
            #error "vs 2008 sp1 required for std::regex"
        #else
            #define REGEX_NAMESPACE std::tr1
        #endif
    #endif
#else
    #define REGEX_NAMESPACE     std
#endif

#if (defined(UNICODE)||defined(_UNICODE))
    #define tregex  REGEX_NAMESPACE::wregex 
#else
    #define tregex  REGEX_NAMESPACE::regex 
#endif

class CRegularExpression
{
protected:
    tregex* RegExpr;

    TCHAR RegExprErrorMessage[256];
public:
    CRegularExpression(TCHAR* RegExpression);
    ~CRegularExpression();
    BOOL IsRegExpressionValid();

    BOOL DoesMatch(TCHAR* String);
    BOOL DoesMatchEx(TCHAR* String,OUT BOOL* bMatch,OUT TCHAR* ErrorMessage, IN SIZE_T ErrorMessageMaxLen);

    static BOOL DoesMatch(TCHAR* String,TCHAR* RegExpression);
    static BOOL DoesMatchEx(TCHAR* String,TCHAR* RegExpression,OUT BOOL* bMatch,OUT TCHAR* ErrorMessage, IN SIZE_T ErrorMessageMaxLen);
};