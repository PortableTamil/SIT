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
// Object: manages the select columns dialog
//-----------------------------------------------------------------------------

#include "selectcolumns.h"

extern CListview* pListView;// class helper for listview
extern COptions* pOptions;//options object
extern HINSTANCE mhInstance;// app instance
extern HWND mhWndDialog;// app instance

CSelectColumns::CSelectColumns()
{
    this->hWndDialog = NULL;
    this->pListviewSelectedColumns = NULL;
}
CSelectColumns::~CSelectColumns()
{
}

//-----------------------------------------------------------------------------
// Name: Show
// Object: Show current CSelectColumns dialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CSelectColumns::Show()
{
    CSelectColumns* pSelectColumns=new CSelectColumns();
    DialogBoxParam(mhInstance, (LPCTSTR)IDD_DIALOG_SELECT_VISIBLE_COLUMNS, mhWndDialog, (DLGPROC)WndProc,(LPARAM)pSelectColumns);
    delete pSelectColumns;
}

//-----------------------------------------------------------------------------
// Name: Init
// Object: Init current CSelectColumns dialog
// Parameters :
//     in  : HWND hwnd : dialog handle
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CSelectColumns::Init(HWND hwnd)
{
    HDITEM Item;
    TCHAR* psz;
    TCHAR ColumnName[MAX_PATH];
    psz=ColumnName;
    this->hWndDialog=hwnd;
    // create a Clistview object associated with IDC_LIST_VISIBLE_COLUMNS
    this->pListviewSelectedColumns=new CListview(GetDlgItem(hwnd,IDC_LIST_VISIBLE_COLUMNS));
    this->pListviewSelectedColumns->SetStyle(TRUE,FALSE,FALSE,TRUE);
    this->pListviewSelectedColumns->AddColumn(NULL,200,LVCFMT_LEFT);
    this->pListviewSelectedColumns->EnableDefaultCustomDraw(FALSE);
    Item.cchTextMax=MAX_PATH;
    Item.pszText=ColumnName;
    Item.mask=HDI_TEXT;
    // for each column of main window log listview
    COptions::COLUMN_ORDER_INFOS* pColumnsInfos;
    for (DWORD Cnt=0;Cnt<SEARCH_AND_REPLACE_LISTVIEW_NB_COLUMNS;Cnt++)
    {
        // retrieve column name
        Header_GetItem(ListView_GetHeader(pListView->GetControlHandle()),Cnt,&Item);
        // add it to current list
        this->pListviewSelectedColumns->AddItem(psz);
        // if column is marked as visible
        pColumnsInfos = &pOptions->ListviewColumnsOrderInfos[Cnt];
        if (pColumnsInfos->Visible)
            this->pListviewSelectedColumns->SetSelectedState(Cnt,TRUE);
    }
}

//-----------------------------------------------------------------------------
// Name: Close
// Object: close current CSelectColumns dialog
// Parameters :
//     in  :
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CSelectColumns::Close()
{
    delete this->pListviewSelectedColumns;
    EndDialog(this->hWndDialog,0);
}

//-----------------------------------------------------------------------------
// Name: Apply
// Object: apply changes : show selected columns and hide others
// Parameters :
//     in  :
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CSelectColumns::Apply()
{
    COptions::COLUMN_ORDER_INFOS* pColumnsInfos;
    // for each column
    for (int cnt=0;cnt<SEARCH_AND_REPLACE_LISTVIEW_NB_COLUMNS;cnt++)
    {
        pColumnsInfos = &pOptions->ListviewColumnsOrderInfos[cnt];

        // retrieve checked state
        pColumnsInfos->Visible=this->pListviewSelectedColumns->IsItemSelected(cnt);
        // if selected
        if (pColumnsInfos->Visible)
        {
            // if column is not already shown
            if (!pListView->IsColumnVisible(cnt))
            {
                // show
                pListView->ShowColumn(cnt,(int)pColumnsInfos->Size);
            }
        }
        else // else
            // hide
            pListView->HideColumn(cnt);
    }
    // close dialog
    this->Close();
}

//-----------------------------------------------------------------------------
// Name: WndProc
// Object: CSelectColumns window proc
// Parameters :
//     in  :
//     out :
//     return : 
//-----------------------------------------------------------------------------
LRESULT CALLBACK CSelectColumns::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);
    switch (uMsg)
    {
    case WM_INITDIALOG:
        // init dialog
        SetWindowLongPtr(hWnd,GWLP_USERDATA,(LONG)lParam);
        ((CSelectColumns*)lParam)->Init(hWnd);
        break;
    case WM_CLOSE:
        {
            // close dialog
            CSelectColumns* pSelectColumns=(CSelectColumns*)GetWindowLongPtr(hWnd,GWLP_USERDATA);
            if (pSelectColumns)
                pSelectColumns->Close();
            break;
        }
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
            case IDOK:
                {
                    // apply changes
                    CSelectColumns* pSelectColumns=(CSelectColumns*)GetWindowLongPtr(hWnd,GWLP_USERDATA);
                    if (pSelectColumns)
                        pSelectColumns->Apply();
                    break;
                }
        }
        break;
    default:
        return FALSE;
    }
    return TRUE;
}