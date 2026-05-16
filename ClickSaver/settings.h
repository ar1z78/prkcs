#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdio.h>
#include "Platform.h"
#include "clicksaver.h" // Automatically matches the exact original definition of g_pCol

// --- Standardized structural fallback for Window coordinates ---
typedef struct
{
    long X, Y;
    long Width, Height;
} NativeRect;

// --- External Application Pointers (Forward declarations mapping symbols) ---
extern char          g_AODir[MAX_PATH];
extern unsigned long g_MainWin;
extern unsigned long g_ItemWatchList;
extern unsigned long g_LocWatchList;

// --- Clean C Interface Functions ---
void ExportSettings( char* filename );
void ImportSettings( char* filename );

#endif // SETTINGS_H
