#include <windows.h>
#include "resource.h"

// Add these declarations so menu.c knows they exist in clicksaver.c
//void CreateSearchWindow(HWND hwndParent);
void CreateSettingsWindow(HWND hwndParent);

void ShowSearchWindow(HWND hwndParent) {
	//CreateSearchWindow(hwndParent);
}

void ShowSettingsWindow(HWND hwndParent) {
	CreateSettingsWindow(hwndParent);
}

void CreateMainMenu(HWND hwnd) {
	HMENU hMenu = CreateMenu();
	HMENU hFileMenu = CreatePopupMenu();
	HMENU hWindowMenu = CreatePopupMenu();

	AppendMenuW(hFileMenu, MF_STRING, ID_FILE_EXIT, L"E&xit");

	//AppendMenuW(hWindowMenu, MF_STRING, ID_WINDOWS_MISSIONS, L"&Missions");
	AppendMenuW(hWindowMenu, MF_STRING, ID_WINDOWS_SEARCH, L"&Search...");
	AppendMenuW(hWindowMenu, MF_STRING, ID_WINDOWS_SETTINGS, L"Se&ttings...");

	AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hFileMenu, L"&File");
	AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hWindowMenu, L"&Windows");

	SetMenu(hwnd, hMenu);
}
