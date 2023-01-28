#include "RegularExpression.h"
#include "../String/AnsiUnicodeConvert.h"

CRegularExpression::CRegularExpression(TCHAR* RegExpression)
{
    this->RegExpr = NULL;
    *this->RegExprErrorMessage = 0;

    try
    {
        this->RegExpr = new tregex(RegExpression);
    }
    catch (std::exception& e)
    {
        CAnsiUnicodeConvert::AnsiToTchar(e.what(),this->RegExprErrorMessage,_countof(this->RegExprErrorMessage));
    }
    catch(...)
    {
        _sntprintf(this->RegExprErrorMessage,_countof(this->RegExprErrorMessage),TEXT("Unknown regular expression exception"));
    }
}
CRegularExpression::~CRegularExpression()
{
    if (this->RegExpr)
        delete this->RegExpr;
}

BOOL CRegularExpression::IsRegExpressionValid()
{
    return (this->RegExpr!=NULL);
}

BOOL CRegularExpression::DoesMatch(TCHAR* String)
{
    BOOL bMatch=FALSE;
    this->DoesMatchEx(String,&bMatch,NULL, 0);
    return bMatch;
}

BOOL CRegularExpression::DoesMatchEx(TCHAR* String,OUT BOOL* bMatch,OUT TCHAR* ErrorMessage, IN SIZE_T ErrorMessageMaxLen)
{
    *bMatch = FALSE;
    if (this->RegExpr == NULL)
    {
        if (ErrorMessage)
        {
            _tcsncpy(ErrorMessage,this->RegExprErrorMessage,ErrorMessageMaxLen);
            ErrorMessage[ErrorMessageMaxLen-1] = 0;
        }
        return FALSE;
    }

    try
    {
        *bMatch = (REGEX_NAMESPACE::regex_match(String, *this->RegExpr)==true);
        return TRUE;
    }
    catch (std::exception& e)
    {
        if (ErrorMessage)
            CAnsiUnicodeConvert::AnsiToTchar(e.what(),ErrorMessage,ErrorMessageMaxLen);
    }
    catch(...)
    {
        if (ErrorMessage)
            _sntprintf(ErrorMessage,ErrorMessageMaxLen,TEXT("Unknown regular expression exception"));
    }
    return FALSE;
}

BOOL CRegularExpression::DoesMatch(TCHAR* String,TCHAR* RegExpression)
{
    BOOL bMatch=FALSE;
    DoesMatchEx(String,RegExpression,&bMatch,NULL, 0);
    return bMatch;
}

BOOL CRegularExpression::DoesMatchEx(TCHAR* String,TCHAR* RegExpression,OUT BOOL* bMatch,OUT TCHAR* ErrorMessage, IN SIZE_T ErrorMessageMaxLen)
{
    CRegularExpression RegularExpression(RegExpression);
    return RegularExpression.DoesMatchEx(String,bMatch,ErrorMessage,ErrorMessageMaxLen);
}
