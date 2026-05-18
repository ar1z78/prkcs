/* ============================================================================
globals.h - Shared Program Definitions and Configuration Layer
============================================================================ */
#pragma once
#include <windows.h>

/* Centralized Configuration Struct Mapping directly to resource.h IDs */
typedef struct {
	/* --- Basic Timing Configurations --- */
	int dwWaitTime;             /* IDC_SET_EDIT_WAIT */

	/* --- Core Application Checkbox Flags (0 = Unchecked, 1 = Checked) --- */
	int bStartMinimized;       /* IDC_SET_CHK_MINIMIZED */
	int bAlertBox;             /* IDC_SET_CHK_ALERT */
	int bSelectMatch;          /* IDC_SET_CHK_SELECT */
	int bSounds;               /* IDC_SET_CHK_SOUNDS */
	int bLogging;              /* IDC_SET_CHK_LOGGING */
	int bShowHelp;             /* IDC_SET_CHK_HELP */
	int bAutoExpand;           /* IDC_SET_CHK_EXPAND */

	/* --- Mission Sliders Numerical Array (Value Bounds: 0 to 100) ---
	Using an array allows us to map them sequentially in a clean loop:
	Sliders[0] = Easy/Hard      (IDC_SET_EDIT_EASY)
	Sliders[1] = Good/Bad       (IDC_SET_EDIT_GOOD)
	Sliders[2] = Order/Chaos    (IDC_SET_EDIT_ORDER)
	Sliders[3] = Open/Hidden    (IDC_SET_EDIT_OPEN)
	Sliders[4] = Phys/Myst      (IDC_SET_EDIT_PHYS)
	Sliders[5] = Head On/Stealth(IDC_SET_EDIT_HEAD)
	Sliders[6] = Money/XP       (IDC_SET_EDIT_MONEY) */
	int Sliders[7];

	/* --- Item Value Properties --- */
	int iBuyMod;               /* IDC_SET_EDIT_BUYMOD (Default: 7) */
	int bMatchSingle;          /* IDC_SET_CHK_MATCH_SINGLE */
	int iMatchSingleVal;       /* IDC_SET_EDIT_MATCH_SINGLE */
	int bMatchTotal;           /* IDC_SET_CHK_MATCH_TOTAL */
	int iMatchTotalVal;        /* IDC_SET_EDIT_MATCH_TOTAL */
} PROGRAM_SETTINGS;

/* Expose the settings instance as a visible external global across files */
#ifdef __cplusplus
extern "C" {
#endif

	extern PROGRAM_SETTINGS g_Settings;

#ifdef __cplusplus
}
#endif