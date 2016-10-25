#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include <windows.h>
#include <locale.h>
#include <Atlbase.h>
#import <msxml6.dll>

TCHAR szClassName[] = TEXT("Window");
#define ID_EDIT 100
#define ID_LIST 101

LPSTR urlencode(LPCSTR url)
{
	DWORD dwTextLenght = 0;
	const DWORD dwOriginalTextLenght = lstrlenA(url);
	DWORD n;
	for (n = 0; n < dwOriginalTextLenght; n++)
	{
		const unsigned char c = (unsigned char)url[n];
		if (IsCharAlphaNumericA(c) || c == '-' || c == '.' || c == '_' || c == '~')dwTextLenght++;
		else dwTextLenght += 3;
	}
	LPSTR lpszNewText = (LPSTR)GlobalAlloc(GMEM_FIXED, dwTextLenght + 1);
	LPSTR p = lpszNewText;
	for (n = 0; n < dwOriginalTextLenght; n++)
	{
		const unsigned char c = (unsigned char)url[n];
		if (IsCharAlphaNumericA(c) || c == '-' || c == '.' || c == '_' || c == '~') *p++ = c;
		else
		{
			wsprintfA(p, "%%%02X", (DWORD)c);
			p += 3;
		}
	}
	*p = 0;
	return lpszNewText;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static BOOL bAuto = FALSE;
	switch (msg)
	{
	case WM_CREATE:
		CoInitialize(NULL);
		CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("EDIT"), 0, WS_VISIBLE | WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL, 10, 10, 256, 32, hWnd, (HMENU)ID_EDIT, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		CreateWindow(TEXT("LISTBOX"), 0, WS_CHILD | WS_BORDER | WS_VSCROLL | LBS_NOTIFY, 10, 50, 224, 512 - 20 - 50, hWnd, (HMENU)ID_LIST, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		break;
	case WM_COMMAND:
		if (LOWORD(wParam) == ID_EDIT && HIWORD(wParam) == EN_CHANGE && !bAuto)
		{
			SendDlgItemMessage(hWnd, ID_LIST, LB_RESETCONTENT, 0, 0);
			DWORD dwSize = GetWindowTextLength(GetDlgItem(hWnd, ID_EDIT));
			if (!dwSize)
			{
				ShowWindow(GetDlgItem(hWnd, ID_LIST), SW_HIDE);
				return 0;
			}
			LPTSTR lpszText = (LPTSTR)GlobalAlloc(0, (dwSize + 1) * sizeof(TCHAR));
			if (!lpszText) return 0;
			GetDlgItemText(hWnd, ID_EDIT, lpszText, dwSize + 1);
			dwSize = WideCharToMultiByte(CP_UTF8, 0, lpszText, -1, 0, 0, 0, 0);
			if (dwSize)
			{
				try
				{
					MSXML2::IXMLDOMDocumentPtr pXMLDoc = NULL;
					HRESULT hr = pXMLDoc.CreateInstance(__uuidof(MSXML2::DOMDocument60));
					pXMLDoc->async = VARIANT_FALSE;
					LPSTR lpszMessage = (LPSTR)GlobalAlloc(GMEM_FIXED, dwSize);
					WideCharToMultiByte(CP_UTF8, 0, lpszText, -1, lpszMessage, dwSize, 0, 0);
					LPSTR lpszText = urlencode(lpszMessage);
					GlobalFree(lpszMessage);
					dwSize = lstrlenA("http://google.com/complete/search?hl=ja&output=toolbar&q=") + lstrlenA(lpszText) + 1;
					LPSTR p = (LPSTR)GlobalAlloc(GMEM_FIXED, dwSize);
					lstrcpyA(p, "http://google.com/complete/search?hl=ja&output=toolbar&q=");
					lstrcatA(p, lpszText);
					GlobalFree(lpszText);
					hr = pXMLDoc->load(p);
					GlobalFree(p);
					setlocale(LC_ALL, "Japanese");
					if (hr == VARIANT_TRUE)
					{
						MSXML2::IXMLDOMNodeListPtr plName = pXMLDoc->getElementsByTagName("CompleteSuggestion");
						const int nSize = plName->Getlength();
						if (nSize)
						{
							for (int i = 0; i < nSize; i++)
							{
								MSXML2::IXMLDOMElementPtr peName = plName->Getitem(i);
								MSXML2::IXMLDOMElementPtr peString = peName->getElementsByTagName("suggestion")->Getitem(0);
								_variant_t str = peString->getAttribute("data");
								SendDlgItemMessage(hWnd, ID_LIST, LB_ADDSTRING, 0, (LPARAM)str.bstrVal);
							}
							const int nItemHeight = (int)SendDlgItemMessage(hWnd, ID_LIST, LB_GETITEMHEIGHT, 0, 0);
							SetWindowPos(GetDlgItem(hWnd, ID_LIST), 0, 0, 0, 256, nItemHeight * (nSize + 1), SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
							ShowWindow(GetDlgItem(hWnd, ID_LIST), SW_NORMAL);
						}
						else
						{
							ShowWindow(GetDlgItem(hWnd, ID_LIST), SW_HIDE);
						}
						plName = NULL;
					}
				}
				catch (_com_error &e)
				{
					OutputDebugString(e.ErrorMessage());
				}
			}
			GlobalFree(lpszText);
		}
		else if (LOWORD(wParam) == ID_LIST && HIWORD(wParam) == LBN_SELCHANGE)
		{
			const INT_PTR nIndex = SendDlgItemMessage(hWnd, ID_LIST, LB_GETCURSEL, 0, 0);
			if (nIndex != LB_ERR)
			{
				INT_PTR nSize = SendDlgItemMessage(hWnd, ID_LIST, LB_GETTEXTLEN, nIndex, 0);
				LPTSTR lpszText = (LPTSTR)GlobalAlloc(0, (nSize + 1) * sizeof(TCHAR));
				SendDlgItemMessage(hWnd, ID_LIST, LB_GETTEXT, nIndex, (LPARAM)lpszText);
				bAuto = TRUE;
				SetDlgItemText(hWnd, ID_EDIT, lpszText);
				bAuto = FALSE;
				GlobalFree(lpszText);
				ShowWindow(GetDlgItem(hWnd, ID_LIST), SW_HIDE);
			}
		}
		break;
	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;
	case WM_DESTROY:
		CoUninitialize();
		PostQuitMessage(0);
		break;
	default:
		return DefDlgProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreInst, LPSTR pCmdLine, int nCmdShow)
{
	MSG msg;
	WNDCLASS wndclass = {
		0,
		WndProc,
		0,
		DLGWINDOWEXTRA,
		hInstance,
		0,
		LoadCursor(0, IDC_ARROW),
		0,
		0,
		szClassName
	};
	RegisterClass(&wndclass);
	RECT rect;
	SetRect(&rect, 0, 0, 276, 242);
	AdjustWindowRect(&rect, WS_CAPTION | WS_SYSMENU, 0);
	HWND hWnd = CreateWindow(
		szClassName,
		TEXT("Google Suggest"),
		WS_CAPTION | WS_SYSMENU,
		CW_USEDEFAULT,
		0,
		rect.right - rect.left,
		rect.bottom - rect.top,
		0,
		0,
		hInstance,
		0
	);
	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);
	while (GetMessage(&msg, 0, 0, 0))
	{
		if (!IsDialogMessage(hWnd, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return (int)msg.wParam;
}
