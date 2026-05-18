#include <windows.h> // REQUIRED FOR HWND AND WIN32 API TYPES
#include "resource.h"
#include "globals.h"
#include "clicksaver.h"
#include <shellapi.h>

void ExportSettings(const char* filename);

/* Centralized URL location variable */
static const LPCWSTR IDC_SETTINGS_URL = L"https://github.com/ar1z78/prkcs";

extern HFONT g_hFont;
extern char g_CSDir[MAX_PATH];

// Forward Declaration for Standard C Compilation Linearity
LRESULT CALLBACK SettingsWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

// ============================================================================
// SETTINGS WINDOW CREATION ROUTINE
// ============================================================================
void CreateSettingsWindow(HWND hwndParent) {
	HINSTANCE hInst;
	int i;
	int baseOptID;
	int baseSliderID;
	WNDCLASSEXW wc = { 0 };
	HWND hwndSettings;

	// DECLARE ALL WINDOW HANDLES AT THE TOP FOR C89 COMPLIANCE
	HWND hTxt1, hTxt2;
	HWND hOptGroup, hWaitLbl, hWaitEdit;
	HWND hSliderGroup;
	HWND hInfoGroup, hInfoText;
	HWND hBtnMidSliders;
	HWND hValGroup, hBuyModLbl, hBuyModEdit, hShopLabel;
	HWND hSearchValGroup, hSubText;
	HWND hMatchSingleChk, hMatchSingleEdit;
	HWND hMatchTotalChk, hMatchTotalEdit;
	HWND hBtnExport, hBtnImport;
	WCHAR szVersionBuffer[80];

	// Array layouts declared statically
	LPCWSTR optLabels[] = { L"Start minimized", L"Alert box", L"Select Match", L"Sounds", L"Logging", L"Show buying agent help", L"Auto Expand Team Missions" };
	LPCWSTR sliderLabels[] = { L"Easy/Hard:", L"Good/Bad:", L"Order/Chaos:", L"Open/Hidden:", L"Physical/Mystical:", L"Head On/Stealth:", L"Money/XP:" };
	LPCWSTR warnMsg = L"Choose Mission slider amounts\n0 = full left, 100 = full right\n\nSliders must be in default positions and the 'More Options' section of the mission window must be OPEN! Press (v) button, otherwise this won't work.";
	LPCWSTR subWarn = L"A match in this section will count as an ITEM MATCH\nMake sure you have those enabled if you want to search for values";

	// Now that declarations are over, we can run logic
	hInst = (HINSTANCE)GetWindowLongPtrW(hwndParent, GWLP_HINSTANCE);

	// 1. Register a UNIQUE window class for the Settings Window
	wc.cbSize = sizeof(WNDCLASSEXW);
	wc.lpfnWndProc = SettingsWndProc;
	wc.hInstance = hInst;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	wc.lpszClassName = L"ClickSaverSettingsClass";

	RegisterClassExW(&wc);

	// 2. Create the window using the unique Settings class name
	hwndSettings = CreateWindowExW(0, L"ClickSaverSettingsClass", L"Settings",
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
		CW_USEDEFAULT, CW_USEDEFAULT, 420, 620, hwndParent, NULL, hInst, NULL);

	if (!hwndSettings) return;

	// Top Brand Headers
	// Dynamically mix your macro into a unified wide string layout
	wsprintfW(szVersionBuffer, L"ClickSaver v%hs, by Ar1z for Project RK", CS_VERSION);

	hTxt1 = CreateWindowExW(0, L"STATIC", szVersionBuffer, WS_CHILD | WS_VISIBLE | SS_CENTER, 5, 8, 395, 14, hwndSettings, NULL, hInst, NULL);
	SendMessageW(hTxt1, WM_SETFONT, (WPARAM)g_hFont, TRUE);

	hTxt2 = CreateWindowExW(0, L"BUTTON", IDC_SETTINGS_URL, WS_CHILD | WS_VISIBLE | BS_FLAT, 5, 24, 395, 16, hwndSettings, (HMENU)IDC_SET_LINK_URL, hInst, NULL);
	SendMessageW(hTxt2, WM_SETFONT, (WPARAM)g_hFont, TRUE);

	// --- Options Group Box ---
	hOptGroup = CreateWindowExW(0, L"BUTTON", L"Options", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 25, 45, 355, 175, hwndSettings, NULL, hInst, NULL);
	SendMessageW(hOptGroup, WM_SETFONT, (WPARAM)g_hFont, TRUE);

	// Create and check options checkboxes dynamically
	baseOptID = IDC_SET_CHK_MINIMIZED;
	for (i = 0; i < 7; ++i) {
		HWND hChk = CreateWindowExW(0, L"BUTTON", optLabels[i], WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 35, 65 + (i * 18), 240, 16, hwndSettings, (HMENU)(INT_PTR)(baseOptID + i), hInst, NULL);
		SendMessageW(hChk, WM_SETFONT, (WPARAM)g_hFont, TRUE);
	}

	// Map the 7 unique global checkbox variables to their visual elements
	CheckDlgButton(hwndSettings, IDC_SET_CHK_MINIMIZED, g_Settings.bStartMinimized ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hwndSettings, IDC_SET_CHK_ALERT, g_Settings.bAlertBox ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hwndSettings, IDC_SET_CHK_SELECT, g_Settings.bSelectMatch ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hwndSettings, IDC_SET_CHK_SOUNDS, g_Settings.bSounds ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hwndSettings, IDC_SET_CHK_LOGGING, g_Settings.bLogging ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hwndSettings, IDC_SET_CHK_HELP, g_Settings.bShowHelp ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hwndSettings, IDC_SET_CHK_EXPAND, g_Settings.bAutoExpand ? BST_CHECKED : BST_UNCHECKED);

	hWaitLbl = CreateWindowExW(0, L"STATIC", L"Wait time between clicks in ms:", WS_CHILD | WS_VISIBLE | SS_LEFT, 35, 195, 180, 14, hwndSettings, NULL, hInst, NULL);
	SendMessageW(hWaitLbl, WM_SETFONT, (WPARAM)g_hFont, TRUE);

	hWaitEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", NULL, WS_CHILD | WS_VISIBLE | ES_NUMBER, 200, 193, 50, 18, hwndSettings, (HMENU)IDC_SET_EDIT_WAIT, hInst, NULL);
	SendMessageW(hWaitEdit, WM_SETFONT, (WPARAM)g_hFont, TRUE);
	SetDlgItemInt(hwndSettings, IDC_SET_EDIT_WAIT, g_Settings.dwWaitTime, FALSE);

	// --- Mission Sliders Mapping Block ---
	hSliderGroup = CreateWindowExW(0, L"BUTTON", L"Mission Sliders", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 5, 230, 190, 155, hwndSettings, NULL, hInst, NULL);
	SendMessageW(hSliderGroup, WM_SETFONT, (WPARAM)g_hFont, TRUE);

	baseSliderID = IDC_SET_EDIT_EASY;
	for (i = 0; i < 7; ++i) {
		HWND hLbl = CreateWindowExW(0, L"STATIC", sliderLabels[i], WS_CHILD | WS_VISIBLE | SS_LEFT, 10, 248 + (i * 19), 110, 14, hwndSettings, NULL, hInst, NULL);
		SendMessageW(hLbl, WM_SETFONT, (WPARAM)g_hFont, TRUE);

		HWND hEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", NULL, WS_CHILD | WS_VISIBLE | ES_NUMBER, 120, 245 + (i * 19), 65, 18, hwndSettings, (HMENU)(INT_PTR)(baseSliderID + i), hInst, NULL);
		SendMessageW(hEdit, WM_SETFONT, (WPARAM)g_hFont, TRUE);

		// Injects slider values dynamically out of our global settings loop index array safely
		SetDlgItemInt(hwndSettings, baseSliderID + i, g_Settings.Sliders[i], FALSE);
	}

	// --- Side Alert/Warning Group Box ---
	hInfoGroup = CreateWindowExW(0, L"BUTTON", L"IMPORTANT", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 200, 230, 200, 155, hwndSettings, NULL, hInst, NULL);
	SendMessageW(hInfoGroup, WM_SETFONT, (WPARAM)g_hFont, TRUE);

	hInfoText = CreateWindowExW(0, L"STATIC", warnMsg, WS_CHILD | WS_VISIBLE | SS_LEFT, 208, 245, 185, 135, hwndSettings, NULL, hInst, NULL);
	SendMessageW(hInfoText, WM_SETFONT, (WPARAM)g_hFont, TRUE);

	// Mid-Separator Commitment Action Trigger Bar
	hBtnMidSliders = CreateWindowExW(0, L"BUTTON", L"Set Sliders Now", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 205, 350, 180, 24, hwndSettings, (HMENU)IDC_SET_BTN_SLIDERS, hInst, NULL);
	SendMessageW(hBtnMidSliders, WM_SETFONT, (WPARAM)g_hFont, TRUE);

	//import/export buttons
	hBtnExport = CreateWindowExW(0, L"BUTTON", L"Export Settings", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 5, 390, 193, 24, hwndSettings, (HMENU)IDC_SET_BTN_EXPORT, hInst, NULL);
	SendMessageW(hBtnExport, WM_SETFONT, (WPARAM)g_hFont, TRUE);
	hBtnImport = CreateWindowExW(0, L"BUTTON", L"Import Settings", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 203, 390, 194, 24, hwndSettings, (HMENU)IDC_SET_BTN_IMPORT, hInst, NULL);
	SendMessageW(hBtnImport, WM_SETFONT, (WPARAM)g_hFont, TRUE);

	// --- Item Value Settings Area ---
	hValGroup = CreateWindowExW(0, L"BUTTON", L"Item Value Settings", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 5, 420, 395, 45, hwndSettings, NULL, hInst, NULL);
	SendMessageW(hValGroup, WM_SETFONT, (WPARAM)g_hFont, TRUE);

	hBuyModLbl = CreateWindowExW(0, L"STATIC", L"Buy Mod:", WS_CHILD | WS_VISIBLE | SS_LEFT, 12, 442, 55, 14, hwndSettings, NULL, hInst, NULL);
	SendMessageW(hBuyModLbl, WM_SETFONT, (WPARAM)g_hFont, TRUE);

	hBuyModEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", NULL, WS_CHILD | WS_VISIBLE | ES_NUMBER, 70, 439, 120, 18, hwndSettings, (HMENU)IDC_SET_EDIT_BUYMOD, hInst, NULL);
	SendMessageW(hBuyModEdit, WM_SETFONT, (WPARAM)g_hFont, TRUE);
	SetDlgItemInt(hwndSettings, IDC_SET_EDIT_BUYMOD, g_Settings.iBuyMod, FALSE);

	hShopLabel = CreateWindowExW(0, L"STATIC", L"Clan: 4 / Omni: 5 / Trader Shop: 7", WS_CHILD | WS_VISIBLE | SS_LEFT, 200, 442, 195, 14, hwndSettings, NULL, hInst, NULL);
	SendMessageW(hShopLabel, WM_SETFONT, (WPARAM)g_hFont, TRUE);

	// --- Item Value Search Settings Sub-Area ---
	hSearchValGroup = CreateWindowExW(0, L"BUTTON", L"Item Value Search Settings", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 5, 470, 395, 80, hwndSettings, NULL, hInst, NULL);
	SendMessageW(hSearchValGroup, WM_SETFONT, (WPARAM)g_hFont, TRUE);

	hSubText = CreateWindowExW(0, L"STATIC", subWarn, WS_CHILD | WS_VISIBLE | SS_CENTER, 12, 490, 380, 32, hwndSettings, NULL, hInst, NULL);
	SendMessageW(hSubText, WM_SETFONT, (WPARAM)g_hFont, TRUE);

	hMatchSingleChk = CreateWindowExW(0, L"BUTTON", L"Match Single Item Value:", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 12, 530, 145, 18, hwndSettings, (HMENU)IDC_SET_CHK_MATCH_SINGLE, hInst, NULL);
	SendMessageW(hMatchSingleChk, WM_SETFONT, (WPARAM)g_hFont, TRUE);
	CheckDlgButton(hwndSettings, IDC_SET_CHK_MATCH_SINGLE, g_Settings.bMatchSingle ? BST_CHECKED : BST_UNCHECKED);

	hMatchSingleEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", NULL, WS_CHILD | WS_VISIBLE | ES_NUMBER, 155, 528, 50, 18, hwndSettings, (HMENU)IDC_SET_EDIT_MATCH_SINGLE, hInst, NULL);
	SendMessageW(hMatchSingleEdit, WM_SETFONT, (WPARAM)g_hFont, TRUE);
	SetDlgItemInt(hwndSettings, IDC_SET_EDIT_MATCH_SINGLE, g_Settings.iMatchSingleVal, FALSE);

	hMatchTotalChk = CreateWindowExW(0, L"BUTTON", L"Match Total Value:", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 225, 530, 115, 18, hwndSettings, (HMENU)IDC_SET_CHK_MATCH_TOTAL, hInst, NULL);
	SendMessageW(hMatchTotalChk, WM_SETFONT, (WPARAM)g_hFont, TRUE);
	CheckDlgButton(hwndSettings, IDC_SET_CHK_MATCH_TOTAL, g_Settings.bMatchTotal ? BST_CHECKED : BST_UNCHECKED);

	hMatchTotalEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", NULL, WS_CHILD | WS_VISIBLE | ES_NUMBER, 340, 528, 50, 18, hwndSettings, (HMENU)IDC_SET_EDIT_MATCH_TOTAL, hInst, NULL);
	SendMessageW(hMatchTotalEdit, WM_SETFONT, (WPARAM)g_hFont, TRUE);
	SetDlgItemInt(hwndSettings, IDC_SET_EDIT_MATCH_TOTAL, g_Settings.iMatchTotalVal, FALSE);

	// Final Action Buttons Footer
	//hBtnExport = CreateWindowExW(0, L"BUTTON", L"Export Settings", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 5, 555, 193, 24, hwndSettings, (HMENU)IDC_SET_BTN_EXPORT, hInst, NULL);
	//SendMessageW(hBtnExport, WM_SETFONT, (WPARAM)g_hFont, TRUE);
	hBtnImport = CreateWindowExW(0, L"BUTTON", L"Apply Settings", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 203, 555, 194, 24, hwndSettings, (HMENU)IDC_SET_BTN_APPLYSETTINGS, hInst, NULL);
	SendMessageW(hBtnImport, WM_SETFONT, (WPARAM)g_hFont, TRUE);

	/* --- SET SETTINGS WINDOW ICONS --- */
	{
		HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));
		if (hIcon)
		{
			SendMessageW(hwndSettings, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
			SendMessageW(hwndSettings, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
		}
	}

	ShowWindow(hwndSettings, SW_SHOW);
	UpdateWindow(hwndSettings);
}


// ============================================================================
// SETTINGS WINDOW MESSAGE PROCESSING CALLBACK
// ============================================================================
LRESULT CALLBACK SettingsWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
	switch (msg) {
	case WM_CTLCOLORSTATIC: {
								HDC hdcStatic = (HDC)wp;
								SetBkColor(hdcStatic, GetSysColor(COLOR_BTNFACE));
								return (INT_PTR)GetSysColorBrush(COLOR_BTNFACE);
	}
	case WM_COMMAND:
		/* --- SAVE ALL SETTINGS ON APPLY --- */
		if (LOWORD(wp) == IDC_SET_BTN_APPLYSETTINGS)
		{
			int baseSliderID = IDC_SET_EDIT_EASY;
			int idx;

			/* 1. Extract the 7 core option checkboxes */
			g_Settings.bStartMinimized = (IsDlgButtonChecked(hwnd, IDC_SET_CHK_MINIMIZED) == BST_CHECKED);
			g_Settings.bAlertBox = (IsDlgButtonChecked(hwnd, IDC_SET_CHK_ALERT) == BST_CHECKED);
			g_Settings.bSelectMatch = (IsDlgButtonChecked(hwnd, IDC_SET_CHK_SELECT) == BST_CHECKED);
			g_Settings.bSounds = (IsDlgButtonChecked(hwnd, IDC_SET_CHK_SOUNDS) == BST_CHECKED);
			g_Settings.bLogging = (IsDlgButtonChecked(hwnd, IDC_SET_CHK_LOGGING) == BST_CHECKED);
			g_Settings.bShowHelp = (IsDlgButtonChecked(hwnd, IDC_SET_CHK_HELP) == BST_CHECKED);
			g_Settings.bAutoExpand = (IsDlgButtonChecked(hwnd, IDC_SET_CHK_EXPAND) == BST_CHECKED);

			/* 2. Extract basic timing numeric field */
			g_Settings.dwWaitTime = GetDlgItemInt(hwnd, IDC_SET_EDIT_WAIT, NULL, FALSE);

			/* 3. Extract all 7 mission sliders sequentially into our global array layout */
			for (idx = 0; idx < 7; ++idx)
			{
				g_Settings.Sliders[idx] = GetDlgItemInt(hwnd, baseSliderID + idx, NULL, FALSE);
			}

			/* 4. Extract item value fields and sub-search blocks */
			g_Settings.iBuyMod = GetDlgItemInt(hwnd, IDC_SET_EDIT_BUYMOD, NULL, FALSE);

			g_Settings.bMatchSingle = (IsDlgButtonChecked(hwnd, IDC_SET_CHK_MATCH_SINGLE) == BST_CHECKED);
			g_Settings.iMatchSingleVal = GetDlgItemInt(hwnd, IDC_SET_EDIT_MATCH_SINGLE, NULL, FALSE);

			g_Settings.bMatchTotal = (IsDlgButtonChecked(hwnd, IDC_SET_CHK_MATCH_TOTAL) == BST_CHECKED);
			g_Settings.iMatchTotalVal = GetDlgItemInt(hwnd, IDC_SET_EDIT_MATCH_TOTAL, NULL, FALSE);

			ExportSettings("LastSettings.cs");
			/* 5. Notify user of success */
			//MessageBoxW(hwnd, L"Settings Applied Successfully!", L"Settings", MB_OK | MB_ICONINFORMATION);
		}

		/* --- EXPORT SETTINGS TO FILE --- */
		if (LOWORD(wp) == IDC_SET_BTN_EXPORT)
		{
			char buffer[2000];
			if (GetFile((HWND)puGetAttribute(puGetObjectFromCollection(g_pCol, CS_MAIN_WINDOW), PUA_WINDOW_HANDLE)
				, TRUE, buffer, 2000))
			{
				ExportSettings(buffer);
			}
			SetCurrentDirectory(g_CSDir);
		}
		if (LOWORD(wp) == IDC_SET_LINK_URL)
		{
			ShellExecuteW(hwnd, L"open", IDC_SETTINGS_URL, NULL, NULL, SW_SHOWNORMAL);
		}
		if (LOWORD(wp) == IDC_SET_BTN_SLIDERS)
		{
			int easy_hard = g_Settings.Sliders[0];
			int good_bad = g_Settings.Sliders[1];
			int order_chaos = g_Settings.Sliders[2];
			int open_hidden = g_Settings.Sliders[3];
			int phys_myst = g_Settings.Sliders[4];
			int headon_stealth = g_Settings.Sliders[5];
			int money_xp = g_Settings.Sliders[6];

			_setSliders(easy_hard, good_bad, order_chaos, open_hidden, phys_myst, headon_stealth, money_xp);
		}
		if (LOWORD(wp) == IDC_SET_BTN_IMPORT)
		{
			char buffer[2000];
			if (GetFile((HWND)puGetAttribute(puGetObjectFromCollection(g_pCol, CS_MAIN_WINDOW), PUA_WINDOW_HANDLE)
				, FALSE, buffer, 2000))
			{
				ImportSettings(buffer);
			}
			SetCurrentDirectory(g_CSDir);
		}
		break;
	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;
	default:
		return DefWindowProcW(hwnd, msg, wp, lp);
	}
	return 0;
}
