/*
 * $Log: clicksaver.c,v $
 * Revision 1.16  2004/12/27 17:28:12  gnarf37
 * Added Option for multiple missions, quick change
 *
 * Revision 1.15  2004/09/03 19:16:46  gnarf37
 * Version 2.3.1 AI Updates
 *
 * Revision 1.14  2004/08/28 18:04:08  gnarf37
 * Moved some GUI Options arounds, added Skip Rebuild option
 *
 * Revision 1.13  2004/01/25 19:35:52  gnarf37
 * 2.3.0 beta 3 - Shrunk Database a bit, added Item Value options, make options menu smaller a tad so that 800x600 might be able to use it again...
 *
 * Revision 1.12  2004/01/23 08:19:09  ibender
 * added mission slider settings
 *
 * Revision 1.11  2003/11/06 23:41:50  gnarf37
 * Version 2.3.0 beta 2 - Fixed issues with 15.2.0 and added an option for auto expand team missions
 *
 * Revision 1.10  2003/10/31 03:40:50  gnarf37
 * Saving/Loading Configurations
 *
 * Revision 1.9  2003/10/25 21:33:32  gnarf37
 * Fixed date/time checking... Should get rid of the major problem everyone is havving
 *
 * Revision 1.8  2003/05/27 00:14:42  gnarf37
 * Added Checkbox to stop mouse movement, and cleaned up mission info parsing so it doesnt match stale missions
 *
 * Revision 1.7  2003/05/08 09:11:09  gnarf37
 * Fullscreen Mode
 *
 * Revision 1.6  2003/05/08 08:40:04  gnarf37
 * Added Logging to Missions
 *
 * Revision 1.5  2003/05/08 07:36:55  gnarf37
 * Added Sounds
 *
 * Revision 1.4  2003/05/07 14:05:28  gnarf37
 * *** empty log message ***
 *
 */
/*
ClickSaver -  Anarchy Online mission helper
Copyright (C) 2001, 2002 Morb
Some parts Copyright (C) 2003, 2004 gnarf
Some parts Copyright (C) 2012 Darkbane, Adjuster

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "Platform.h"

#include <pul/pul.h>

#include <winuser.h>
#include <shlobj.h>
#include <stdio.h>
#include <string.h>
#include "clicksaver.h"
#include "resource.h"
#include "CoreUtils.h"
#include "rdb.h"
#include "settings.h"

#pragma comment(lib, "shlwapi.lib")


void CleanUp();

void GetFolder( HWND hWndOwner, char *strTitle, char *strPath );
BOOL GetFile( HWND hWndOwner, BOOL saving, char *buffer, int buffersize );

int BuyingAgent();
void EndBuyingAgent();

extern PUU32 g_GUIDef[];
pusObjectCollection* g_pCol;
PULID g_ItemWatchList, g_LocWatchList, g_MainWin;

void _setSliders( int easy_hard, int good_bad, int order_chaos, int open_hidden, int phys_myst, int headon_stealth, int money_xp );

PUU32 g_BuyingAgentCount = 0;
PUU32 g_BuyingAgentMissions = 0;
PUU32 g_bFirstRound = TRUE;
PUU8 g_MishNumber = 0, g_FoundMish = -1;
PUU8 g_bFullscreen = 0;

char g_CurrentPacket[ 65536 ];

char g_AODir[ MAX_PATH ] = { 0 };
char g_CSDir[ MAX_PATH ] = { 0 };

HANDLE g_Mutex = INVALID_HANDLE_VALUE;
HANDLE g_Thread = INVALID_HANDLE_VALUE;
DWORD WINAPI HookManagerThread( void *pParam );

//DB* g_pDB = NULL;

int main( int argc, char** argv )
{
    pusAppMessage* pAppMsg;
    void* pMissionData;
    PULID MissionControls[ 5 ];
    FILE* fp;
    char AOExePath[ 256 ];
    DWORD dwThreadID;
    HANDLE hOrigDB;
    int bUpdateDB = FALSE;
    char DBPath[ 256 * 2 ];

    // Set main thread of clicksaver on a priority above normal
    // Helps a lot. Refreshing of missions infos is much faster.
    SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL );

    // Initialise PUL
    if( !puInit() )
    {
        return -1;
    }

    // Register mission control class
    if( !RegisterMissionClass() )
    {
        CleanUp();
        return -1;
    }

    // Create the windows
    if( !( g_pCol = puCreateObjectCollection( g_GUIDef ) ) )
    {
        CleanUp();
        return -1;
    }

    g_MainWin = puGetObjectFromCollection( g_pCol, CS_MAIN_WINDOW );
    g_ItemWatchList = puGetObjectFromCollection( g_pCol, CS_ITEMWATCH_LIST );
    g_LocWatchList = puGetObjectFromCollection( g_pCol, CS_LOCWATCH_LIST );

    // Get current directory
    GetCurrentDirectory( MAX_PATH, g_CSDir );

    ImportSettings( "LastSettings.cs" );

    if( puGetAttribute( puGetObjectFromCollection( g_pCol, CS_STARTMIN_CB ), PUA_CHECKBOX_CHECKED ) )
        puSetAttribute( g_MainWin, PUA_WINDOW_ICONIFIED, TRUE );

    sprintf( AOExePath, "%s\\anarchy.exe", g_AODir );


	char* promptMessage = (char*)"Please locate the PRK folder, where Anarchy.exe resides.";

	while (!(fp = fopen(AOExePath, "r")))
	{
		GetFolder(NULL, promptMessage, g_AODir);

		if (!g_AODir)
		{
			CleanUp();
			return -1;
		}

		sprintf(AOExePath, "%s\\anarchy.exe", g_AODir);

		if (!(fp = fopen(AOExePath, "r")))
		{
			promptMessage = (char*)"This is not PRK's directory. Please try again.";
		}
	}

	fclose(fp);

	// 1. Create Mutex FIRST to isolate and guarantee GetLastError() accuracy
	g_Mutex = CreateMutex(NULL, FALSE, "ClickSaver");
	DWORD dwMutexError = GetLastError(); // Immediately cache the error status

	if (g_Mutex == NULL)
	{
		ShowErrorMessage("Couldn't create mutex.");
		CleanUp();
		return -1;
	}

	if (dwMutexError == ERROR_ALREADY_EXISTS)
	{
		HWND hWnd = FindWindow("ClickSaverHookWindowClass", "ClickSaverHookWindow");
		if (hWnd)
		{
			// Send message to original window if needed
			// PostMessage(hWnd, WM_USER + 1, 0, 0);
		}

		// This will now display reliably
		ShowErrorMessage("ClickSaver is already running.");

		CloseHandle(g_Mutex);
		CleanUp();
		return -1;
	}

	// 2. Construct the path to check if rdb.db exists
	sprintf(DBPath, "%s\\cd_image\\rdb.db", g_AODir);
	hOrigDB = CreateFile(DBPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

	if (hOrigDB == INVALID_HANDLE_VALUE) {
		char err[100];
		sprintf(err, "System Error Code: %d \nPath: %s", GetLastError(), DBPath);
		ShowErrorMessage(err);
		CloseHandle(g_Mutex); // Ensure mutex cleanup on early exit
		CleanUp();
		return -1;
	}
	CloseHandle(hOrigDB);

	// 3. Initialize SQLite and Prepare statements
	if (!OpenLocalDB()) {
		ShowErrorMessage("Couldn't open the AO database (rdb.db).");
		CloseHandle(g_Mutex); // Ensure mutex cleanup on early exit
		CleanUp();
		return -1;
	}

	// 4. Starts dll hook management thread (Fixed INVALID_HANDLE_VALUE bug to NULL)
	g_Thread = CreateThread(NULL, 0, &HookManagerThread, NULL, 0, &dwThreadID);
	if (g_Thread == NULL)
	{
		ShowErrorMessage("Couldn't create hook thread.");
		ReleaseAODatabase();
		CloseHandle(g_Mutex); // Ensure mutex cleanup on early exit
		CleanUp();
		return -1;
	}


    MissionControls[ 0 ] = puGetObjectFromCollection( g_pCol, CS_MISSION1 );
    MissionControls[ 1 ] = puGetObjectFromCollection( g_pCol, CS_MISSION2 );
    MissionControls[ 2 ] = puGetObjectFromCollection( g_pCol, CS_MISSION3 );
    MissionControls[ 3 ] = puGetObjectFromCollection( g_pCol, CS_MISSION4 );
    MissionControls[ 4 ] = puGetObjectFromCollection( g_pCol, CS_MISSION5 );
    //puSetAttribute( puGetObjectFromCollection( g_pCol, CS_OPTIONSFOLD3 ), PUA_FOLD_FOLDED, TRUE);
    puSetAttribute( g_MainWin, PUA_WINDOW_OPENED, TRUE );

    HICON hIcon = LoadIcon( GetModuleHandle( NULL ), MAKEINTRESOURCE( IDI_ICON1 ) );
    if( hIcon )
    {
        PUU32 uWindowHandle = puGetAttribute( g_MainWin, PUA_WINDOW_HANDLE );
        SendMessage( (HWND)uWindowHandle, WM_SETICON, ICON_BIG,   (LPARAM)hIcon );
        SendMessage( (HWND)uWindowHandle, WM_SETICON, ICON_SMALL, (LPARAM)hIcon );
    }

    do
    {
        pAppMsg = puWaitAppMessages();

        switch( pAppMsg->Message )
        {
        case CSAM_STOPBUYINGAGENT:
            g_BuyingAgentCount = 0;
            g_BuyingAgentMissions = 0;
            EndBuyingAgent();

            // Fall through

        case CSAM_NEWMISSIONS:
            if( !g_BuyingAgentCount && g_bFullscreen )
            {
                g_BuyingAgentCount = puGetAttribute( puGetObjectFromCollection( g_pCol, CS_BUYINGAGENTTRIES ), PUA_TEXTENTRY_VALUE );
            }
            if( g_BuyingAgentCount )
            {
                g_BuyingAgentCount--;
                pMissionData = g_CurrentPacket;

                WaitForSingleObject( g_Mutex, INFINITE );
                g_FoundMish = 255;
                for( g_MishNumber = 0; g_MishNumber < 5; g_MishNumber++ )
                {
                    if( !( pMissionData = (void*)puDoMethod( MissionControls[ g_MishNumber ], CSM_MISSION_PARSEMISSION, (PUU32)pMissionData, 0 ) ) )
                    {
                        break;
                    }
                }
                ReleaseMutex( g_Mutex );

                //if( pMissionData )
                //{
                    if( g_BuyingAgentCount )
                    {
                        BuyingAgent();
                    }
                    else
                    {
                        EndBuyingAgent();
                    }
                //}
            }

            //if( !g_BuyingAgentCount )
            {
                pMissionData = g_CurrentPacket;
                puSetAttribute( g_MainWin, PUA_WINDOW_DEFERUPDATE, TRUE );

                WaitForSingleObject( g_Mutex, INFINITE );
                g_FoundMish = 255;
                for( g_MishNumber = 0; g_MishNumber < 5; g_MishNumber++ )
                {
                    void *pLastMissionData;
                    pLastMissionData = pMissionData;
                    if( !( pMissionData = (void*)puDoMethod( MissionControls[ g_MishNumber ], CSM_MISSION_PARSEMISSION, (PUU32)pMissionData, 0 ) ) )
                    {
                        pMissionData = pLastMissionData;
                    }
                }

                ReleaseMutex( g_Mutex );

                if( pMissionData && !g_bFullscreen )
                {
                    //puSetAttribute( puGetObjectFromCollection( g_pCol, CS_ERROR_WINDOW ), PUA_WINDOW_OPENED, FALSE );

                    // Select mission tab
                    puSetAttribute( puGetObjectFromCollection( g_pCol, CS_TABS ), PUA_REGISTER_CURRENTTAB, 0 );

                    // Uniconify window
                    puSetAttribute( g_MainWin, PUA_WINDOW_ICONIFIED, FALSE );
                }

                puSetAttribute( g_MainWin, PUA_WINDOW_DEFERUPDATE, FALSE );

                if( PUL_GET_CB( CS_SOUNDS_CB ) )
                {
                    if( g_FoundMish == 255 ) // Not Found
                    {
                        PlaySound( "notfound.wav", NULL, SND_FILENAME | SND_NODEFAULT );
                    }
                    else
                    {
                        PlaySound( "found.wav", NULL, SND_FILENAME | SND_NODEFAULT );
                    }
                }
                if( PUL_GET_CB( CS_MOUSEMOVE_CB ) || g_BuyingAgentMissions )
                {
                    HWND AOWnd;
                    POINT MousePos;
                    LPARAM lParam;

                    WriteLog( NULL );

                    // Find AO window
                    if( !( AOWnd = FindWindow( "Anarchy client", NULL ) ) )
                    {
                        //ShowErrorMessage( "Anarchy Online is not running." );
						puSetAttribute(puGetObjectFromCollection(g_pCol, CS_STATUS_TEXT), PUA_TEXT_STRING, (PUU32)"Anarchy Online is not running.");
                        g_BuyingAgentCount = 0;
                    }

                    if( g_FoundMish != 255 && !( pAppMsg->Message == CSAM_STOPBUYINGAGENT ) )
                    { // Move mouse and select mission that finished our Buying Agent
                        MousePos.x = 44 + ( ( g_FoundMish % 3 ) * 58 );
                        MousePos.y = 57 + ( ( g_FoundMish / 3 ) * 57 );
                        lParam = MousePos.y << 16 | MousePos.x;

                        ClientToScreen( AOWnd, &MousePos );
                        SetCursorPos( MousePos.x, MousePos.y );

                        SendMessage( AOWnd, WM_LBUTTONDOWN, 0, lParam );
                        Sleep( 500 );
                        SendMessage( AOWnd, WM_LBUTTONUP, 0, lParam );

						Sleep( 2010 );

                        MousePos.x = 76; MousePos.y = 321;
                        lParam = MousePos.y << 16 | MousePos.x;
                        ClientToScreen( AOWnd, &MousePos );
                        SetCursorPos( MousePos.x, MousePos.y );
                        if( g_BuyingAgentMissions )
                        {
                            SendMessage( AOWnd, WM_LBUTTONDOWN, 0, lParam );
                            Sleep( 500 );
                            SendMessage( AOWnd, WM_LBUTTONUP, 0, lParam );

							Sleep( 2010 );

                            SendMessage( AOWnd, WM_KEYDOWN, 0x45, 0 );
                            Sleep( 500 );
                            SendMessage( AOWnd, WM_KEYUP, 0x45, 0 );

							Sleep( 2010 );

                            {
                                int easy_hard = puGetAttribute( puGetObjectFromCollection( g_pCol, CS_SLIDER_EASY_HARD ), PUA_TEXTENTRY_VALUE );
                                int good_bad = puGetAttribute( puGetObjectFromCollection( g_pCol, CS_SLIDER_GOOD_BAD ), PUA_TEXTENTRY_VALUE );
                                int order_chaos = puGetAttribute( puGetObjectFromCollection( g_pCol, CS_SLIDER_ORDER_CHAOS ), PUA_TEXTENTRY_VALUE );
                                int open_hidden = puGetAttribute( puGetObjectFromCollection( g_pCol, CS_SLIDER_OPEN_HIDDEN ), PUA_TEXTENTRY_VALUE );
                                int phys_myst = puGetAttribute( puGetObjectFromCollection( g_pCol, CS_SLIDER_PHYS_MYST ), PUA_TEXTENTRY_VALUE );
                                int headon_stealth = puGetAttribute( puGetObjectFromCollection( g_pCol, CS_SLIDER_HEADON_STEALTH ), PUA_TEXTENTRY_VALUE );
                                int money_xp = puGetAttribute( puGetObjectFromCollection( g_pCol, CS_SLIDER_MONEY_XP ), PUA_TEXTENTRY_VALUE );

                                _setSliders( easy_hard, good_bad, order_chaos, open_hidden, phys_myst, headon_stealth, money_xp );
                            }

                            g_bFirstRound = TRUE;
                            g_BuyingAgentCount = puGetAttribute( puGetObjectFromCollection( g_pCol, CS_BUYINGAGENTTRIES ), PUA_TEXTENTRY_VALUE );
                            BuyingAgent();
                            g_BuyingAgentMissions--;
                        }
                    }
                }
                WriteLog( NULL ); // Close log file at this point
            }
            break;

        case CSAM_PRESTARTBUYINGAGENT:
            if( puGetAttribute( puGetObjectFromCollection( g_pCol, CS_BAINFO_CB ), PUA_CHECKBOX_CHECKED ) )
            {
                puSetAttribute( puGetObjectFromCollection( g_pCol, CS_BUYINGAGENT_INFOWINDOW ), PUA_WINDOW_OPENED, TRUE );
                break;
            }

            // Fall through

        case CSAM_STARTBUYINGAGENT:
            puSetAttribute( puGetObjectFromCollection( g_pCol, CS_BUYINGAGENT_INFOWINDOW ), PUA_WINDOW_OPENED, FALSE );

           // if( !g_BuyingAgentCount )
            {
                PUU32 bItemListOk = FALSE, bLocListOk = FALSE, bTypeListOk = FALSE;
                PUU32 bWarnItem, bWarnLoc, bWarnType;
                PUU32 bReadyToGo = FALSE;

                //puSetAttribute( puGetObjectFromCollection( g_pCol, CS_ERROR_WINDOW ), PUA_WINDOW_OPENED, FALSE );

                // Make sure that there's something in the relevant watch list
                // (depending on the mission condition settings)
                // before starting eating up player's credits for nothing :)
                bWarnItem = puGetAttribute( puGetObjectFromCollection( g_pCol, CS_ALERTITEM_CB ), PUA_CHECKBOX_CHECKED );
                bWarnLoc = puGetAttribute( puGetObjectFromCollection( g_pCol, CS_ALERTLOC_CB ), PUA_CHECKBOX_CHECKED );
                bWarnType = puGetAttribute( puGetObjectFromCollection( g_pCol, CS_ALERTTYPE_CB ), PUA_CHECKBOX_CHECKED );

                if( puGetAttribute( g_ItemWatchList, PUA_TABLE_NUMRECORDS ) )
                {
                    bItemListOk = TRUE;
                }

                if( puGetAttribute( g_LocWatchList, PUA_TABLE_NUMRECORDS ) )
                {
                    bLocListOk = TRUE;
                }

                if( puGetAttribute( puGetObjectFromCollection( g_pCol, CS_TYPEREPAIR_CB ), PUA_CHECKBOX_CHECKED ) ||
                    puGetAttribute( puGetObjectFromCollection( g_pCol, CS_TYPEFINDP_CB ), PUA_CHECKBOX_CHECKED ) ||
                    puGetAttribute( puGetObjectFromCollection( g_pCol, CS_TYPEFINDI_CB ), PUA_CHECKBOX_CHECKED ) ||
                    puGetAttribute( puGetObjectFromCollection( g_pCol, CS_TYPERETURN_CB ), PUA_CHECKBOX_CHECKED ) ||
                    puGetAttribute( puGetObjectFromCollection( g_pCol, CS_TYPEASS_CB ), PUA_CHECKBOX_CHECKED ) )
                {
                    bTypeListOk = TRUE;
                }

                bReadyToGo = bWarnLoc || bWarnItem || bWarnType;
                if( bWarnItem ) bReadyToGo = bReadyToGo && bItemListOk;
                if( bWarnLoc ) bReadyToGo = bReadyToGo && bLocListOk;
                if( bWarnType ) bReadyToGo = bReadyToGo && bTypeListOk;

                if( bReadyToGo )
                {
                    g_BuyingAgentMissions = puGetAttribute( puGetObjectFromCollection( g_pCol, CS_BUYINGAGENTMISH ), PUA_TEXTENTRY_VALUE ) - 1;
                    g_BuyingAgentCount = puGetAttribute( puGetObjectFromCollection( g_pCol, CS_BUYINGAGENTTRIES ), PUA_TEXTENTRY_VALUE );
                    g_bFirstRound = TRUE;
                    BuyingAgent();
                }
                else
                {
                    // Complain to the user that he/she hasn't told us what to search for
                    //ShowErrorMessage( "I won't ever find any mission with your current settings and watch lists." );
					puSetAttribute(puGetObjectFromCollection(g_pCol, CS_STATUS_TEXT), PUA_TEXT_STRING, (PUU32)"I won't ever find any mission with your current settings.");
                }
            }
            break;

        case CSAM_EXPORTSETTINGS:
        {
            char buffer[ 2000 ];
            if( GetFile( (HWND)puGetAttribute( puGetObjectFromCollection( g_pCol, CS_MAIN_WINDOW ), PUA_WINDOW_HANDLE )
                , TRUE, buffer, 2000 ) )
            {
                ExportSettings( buffer );
            }
            SetCurrentDirectory( g_CSDir );
        }
        break;

        case CSAM_IMPORTSETTINGS:
        {
            char buffer[ 2000 ];
            if( GetFile( (HWND)puGetAttribute( puGetObjectFromCollection( g_pCol, CS_MAIN_WINDOW ), PUA_WINDOW_HANDLE )
                , FALSE, buffer, 2000 ) )
            {
                ImportSettings( buffer );
            }
            SetCurrentDirectory( g_CSDir );
        }
        break;

        case CSAM_STOPFULLSCREEN:
            g_bFullscreen = 0;
            puSetAttribute( puGetObjectFromCollection( g_pCol, CS_FULLSCREEN_WINDOW ), PUA_WINDOW_OPENED, FALSE );
            puSetAttribute( puGetObjectFromCollection( g_pCol, CS_MAIN_WINDOW ), PUA_WINDOW_OPENED, TRUE );
            break;

        case CSAM_STARTFULLSCREEN:
        {
            PUU32 bItemListOk = FALSE, bLocListOk = FALSE, bTypeListOk = FALSE;
            PUU32 bWarnItem, bWarnLoc, bWarnType;
            PUU32 bReadyToGo = FALSE;

            //puSetAttribute( puGetObjectFromCollection( g_pCol, CS_ERROR_WINDOW ), PUA_WINDOW_OPENED, FALSE );

            // Make sure that there's something in the relevant watch list
            // (depending on the mission condition settings)
            // before starting eating up player's credits for nothing :)
            bWarnItem = puGetAttribute( puGetObjectFromCollection( g_pCol, CS_ALERTITEM_CB ), PUA_CHECKBOX_CHECKED );
            bWarnLoc = puGetAttribute( puGetObjectFromCollection( g_pCol, CS_ALERTLOC_CB ), PUA_CHECKBOX_CHECKED );
            bWarnType = puGetAttribute( puGetObjectFromCollection( g_pCol, CS_ALERTTYPE_CB ), PUA_CHECKBOX_CHECKED );

            if( puGetAttribute( g_ItemWatchList, PUA_TABLE_NUMRECORDS ) )
            {
                bItemListOk = TRUE;
            }

            if( puGetAttribute( g_LocWatchList, PUA_TABLE_NUMRECORDS ) )
            {
                bLocListOk = TRUE;
            }

            if( puGetAttribute( puGetObjectFromCollection( g_pCol, CS_TYPEREPAIR_CB ), PUA_CHECKBOX_CHECKED ) ||
                puGetAttribute( puGetObjectFromCollection( g_pCol, CS_TYPEFINDP_CB ), PUA_CHECKBOX_CHECKED ) ||
                puGetAttribute( puGetObjectFromCollection( g_pCol, CS_TYPEFINDI_CB ), PUA_CHECKBOX_CHECKED ) ||
                puGetAttribute( puGetObjectFromCollection( g_pCol, CS_TYPERETURN_CB ), PUA_CHECKBOX_CHECKED ) ||
                puGetAttribute( puGetObjectFromCollection( g_pCol, CS_TYPEASS_CB ), PUA_CHECKBOX_CHECKED ) )
            {
                bTypeListOk = TRUE;
            }

            bReadyToGo = bWarnLoc || bWarnItem || bWarnType;
            if( bWarnItem ) bReadyToGo = bReadyToGo && bItemListOk;
            if( bWarnLoc ) bReadyToGo = bReadyToGo && bLocListOk;
            if( bWarnType ) bReadyToGo = bReadyToGo && bTypeListOk;

            if( bReadyToGo )
            {
                puSetAttribute( puGetObjectFromCollection( g_pCol, CS_FULLSCREEN_WINDOW ), PUA_WINDOW_OPENED, TRUE );
                puSetAttribute( puGetObjectFromCollection( g_pCol, CS_MAIN_WINDOW ), PUA_WINDOW_OPENED, FALSE );
                g_bFullscreen = 1;
            }
            else
            {
                // Complain to the user that he/she hasn't told us what to search for
                //ShowErrorMessage( "I won't ever find any mission with your current settings and watch lists." );
				puSetAttribute(puGetObjectFromCollection(g_pCol, CS_STATUS_TEXT), PUA_TEXT_STRING, (PUU32)"I won't ever find any mission with your current settings.");
            }
        }
        break;

        case CSAM_SET_SLIDERS:
        {
            int easy_hard = puGetAttribute( puGetObjectFromCollection( g_pCol, CS_SLIDER_EASY_HARD ), PUA_TEXTENTRY_VALUE );
            int good_bad = puGetAttribute( puGetObjectFromCollection( g_pCol, CS_SLIDER_GOOD_BAD ), PUA_TEXTENTRY_VALUE );
            int order_chaos = puGetAttribute( puGetObjectFromCollection( g_pCol, CS_SLIDER_ORDER_CHAOS ), PUA_TEXTENTRY_VALUE );
            int open_hidden = puGetAttribute( puGetObjectFromCollection( g_pCol, CS_SLIDER_OPEN_HIDDEN ), PUA_TEXTENTRY_VALUE );
            int phys_myst = puGetAttribute( puGetObjectFromCollection( g_pCol, CS_SLIDER_PHYS_MYST ), PUA_TEXTENTRY_VALUE );
            int headon_stealth = puGetAttribute( puGetObjectFromCollection( g_pCol, CS_SLIDER_HEADON_STEALTH ), PUA_TEXTENTRY_VALUE );
            int money_xp = puGetAttribute( puGetObjectFromCollection( g_pCol, CS_SLIDER_MONEY_XP ), PUA_TEXTENTRY_VALUE );

            _setSliders( easy_hard, good_bad, order_chaos, open_hidden, phys_myst, headon_stealth, money_xp );
        }
        break;
        }
    }
    while( pAppMsg->Message != CSAM_QUIT );

    WriteDebug( NULL );

    SetCurrentDirectory( g_CSDir );

    ExportSettings( "LastSettings.cs" );


    //ReleaseAODatabase();
    CleanUp();
    return 0;
}


void CleanUp()
{
    if( g_Thread != INVALID_HANDLE_VALUE )
    {
        TerminateThread( g_Thread, 0 );
    }

    if( g_Mutex != INVALID_HANDLE_VALUE )
    {
        CloseHandle( g_Mutex );
    }

	ReleaseAODatabase();

    puDeleteObjectCollection( g_pCol );
    puClear();
}


/* Prompt user for folder
   (from AOMD)
*/
void GetFolder( HWND hWndOwner, char *strTitle, char *strPath )
{
    BROWSEINFO udtBI;
    ITEMIDLIST *udtIDList;

    /* Initialise */
    udtBI.hwndOwner = hWndOwner;
    udtBI.pidlRoot = NULL;
    udtBI.pszDisplayName = NULL;
    udtBI.lpszTitle = strTitle;
    udtBI.ulFlags = BIF_RETURNONLYFSDIRS;
    udtBI.lpfn = NULL;
    udtBI.lParam = 0;
    udtBI.iImage = 0;

    /* Prompt user for folder */
    udtIDList = SHBrowseForFolder( &udtBI );

    /* Extract pathname */
    if( !SHGetPathFromIDList( udtIDList, strPath ) )
    {
        strPath[ 0 ] = 0; // Zero-length if failure
    }
}


BOOL GetFile( HWND hWndOwner, BOOL saving, char* buffer, int buffersize )
{
    OPENFILENAME ofn;

    ZeroMemory( &ofn, sizeof( ofn ) );

    /* Initialise */
    ofn.hwndOwner = hWndOwner;
    ofn.lStructSize = sizeof( OPENFILENAME );
    if( saving )
    {
        ofn.Flags = OFN_HIDEREADONLY;
    }
    else
    {
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    }

    ofn.lpstrFilter = "Clicksaver Files\0*.CS\0";
    ofn.lpstrFile = buffer;
    ofn.lpstrFile[ 0 ] = '\0';
    ofn.nMaxFile = buffersize;
    ofn.nFilterIndex = 0;
    ofn.lpstrInitialDir = ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;

    /* Prompt user for folder */
    if( saving )
    {
        return GetSaveFileName( &ofn );
    }
    else
    {
        return GetOpenFileName( &ofn );
    }

    return FALSE;
}


// Generate a mouse movement and button click sequence
// to make AO generate new missions.
// All coordinates are hardcoded because I'm too
// lazy to make them configurable.
// However, it's not that much of a problem, since
// I use coordinates relative to AO window.
// So, it will work regardless of where the AO window is,
// and with the mission window in AO snapped to the
// upper left corner.
int BuyingAgent()
{
    HWND AOWnd, BAWnd;
    POINT MousePos;
    LPARAM lParam;

    // Find AO window
    if( !( AOWnd = FindWindow( "Anarchy client", NULL ) ) )
    {
        //ShowErrorMessage( "Anarchy Online is not running.");
		puSetAttribute(puGetObjectFromCollection(g_pCol, CS_STATUS_TEXT), PUA_TEXT_STRING, (PUU32)"Anarchy Online is not running.");
        g_BuyingAgentCount = 0;
        g_BuyingAgentMissions = 0;
        return FALSE;
    }

    // Close main window
    if( !g_bFullscreen )
    {
       // puSetAttribute( g_MainWin, PUA_WINDOW_OPENED, FALSE );

        // Open buying agent window
        puSetAttribute( puGetObjectFromCollection( g_pCol, CS_BUYINGAGENT_WINDOW ), PUA_WINDOW_OPENED, TRUE );

        // Set keyboard focus on buying agent window
        BAWnd = (HWND)puGetAttribute( puGetObjectFromCollection( g_pCol, CS_BUYINGAGENT_WINDOW ), PUA_WINDOW_HANDLE );
        SetFocus( BAWnd );
    }

    // Delay
    //Sleep( puGetAttribute( puGetObjectFromCollection( g_pCol, CS_BUYINGAGENTDELAY ), PUA_TEXTENTRY_VALUE ) );

	Sleep(puGetAttribute(puGetObjectFromCollection(g_pCol, CS_ROLLWAIT), PUA_TEXTENTRY_VALUE));

    // Force AO on top
    SetForegroundWindow( AOWnd );

    // Now we have the absolute position of the upper-left corner
    // of AO display area on screen. We can now use that
    // as a basis to generate mouse positions relative to
    // the AO window.

    // Click on "request mission"

    // Now that we don't have to move the cursor
    // around to ungray the request button, we
    // move the mouse only once, to make it
    // easy to abort the buying agent while
    // it's running
    MousePos.x = 99;
    MousePos.y = 180;
    lParam = MousePos.y << 16 | MousePos.x;

    if( g_bFirstRound )
    {
        ClientToScreen( AOWnd, &MousePos );
        SetCursorPos( MousePos.x, MousePos.y );
        g_bFirstRound = FALSE;
    }

    SendMessage( AOWnd, WM_LBUTTONDOWN, 0, lParam );
    SendMessage( AOWnd, WM_LBUTTONUP, 0, lParam );

    return TRUE;
}


void EndBuyingAgent()
{

    if( !g_bFullscreen )
    {
        // Remove keyboard focus
        SetFocus( NULL );

        // Close buying agent window
        puSetAttribute( puGetObjectFromCollection( g_pCol, CS_BUYINGAGENT_WINDOW ), PUA_WINDOW_OPENED, FALSE );

        // Open main window
        //puSetAttribute( g_MainWin, PUA_WINDOW_OPENED, TRUE );
    }

}


void DebugPacket( void* pData, unsigned int length )
{
    unsigned int x;
    unsigned char *data = (char *)pData;
    char ps[ 70 ];
    for( x = 0; x < length; x++ )
    {
        sprintf( &( ps[ x % 16 * 3 ] ), "%02X", data[ x ] );
        ps[ x % 16 * 3 + 2 ] = ' ';
        ps[ x % 16 + 48 ] = ( data[ x ] >= 32 && data[ x ] <= 127 ? data[ x ] : '.' );
        ps[ x % 16 + 49 ] = '\n';
        ps[ x % 16 + 50 ] = 0;
        if( x % 16 == 15 ) WriteDebug( ps );
    }

    if( x % 16 != 0 )
    {
        for( x = x % 16; x < 16; x++ )
        {
            sprintf( &( ps[ x % 16 * 3 ] ), "  " );
            ps[ x % 16 * 3 + 2 ] = ' ';
        }
        WriteDebug( ps );
    }
}


void WriteLog( const char* Format, ... )
{
    /**/
    va_list argptr;
    static FILE *fp = NULL;
    if( Format == NULL )
    {
        if( fp )
        {
            fclose( fp );
            fp = NULL;
        }
        return;
    }
    if( PUL_GET_CB( CS_LOG_CB ) )
    {
        if( !fp )
        {
            fp = fopen( "clicksaver.log", "a" );
        }
        va_start( argptr, Format );
        vfprintf( fp, Format, argptr );
        va_end( argptr );
    }
    /**/
}


void WriteDebug( const char* txt )
{
#ifdef _DEBUG
    static FILE *fp = NULL;
    if( txt == NULL )
    {
        if( fp )
        {
            fclose( fp );
            fp = NULL;
        }
        return;
    }
    if( !fp )
    {
        fp = fopen( "clicksaver.debug", "a" );
    }
    fprintf( fp, "%s", txt );
#endif // _DEBUG
}


//slider setting functions

void _dragMouse( int x0, int y0, int x1, int y1 )
{
    POINT MousePos;
    LPARAM lParam;
    HWND AOWnd;

    // Find AO window
    if( !( AOWnd = FindWindow( "Anarchy client", NULL ) ) )
    {
        //ShowErrorMessage( "Anarchy Online is not running." );
		puSetAttribute(puGetObjectFromCollection(g_pCol, CS_STATUS_TEXT), PUA_TEXT_STRING, (PUU32)"Anarchy Online is not running.");
        g_BuyingAgentCount = 0;
        g_BuyingAgentMissions = 0;
        return;
    }
    MousePos.x = x0;
    MousePos.y = y0;
    lParam = MousePos.y << 16 | MousePos.x;
    ClientToScreen( AOWnd, &MousePos );
    SetCursorPos( MousePos.x, MousePos.y );
    SendMessage( AOWnd, WM_LBUTTONDOWN, 0, lParam );
    Sleep( 250 );
    MousePos.x = x1;
    MousePos.y = y1;
    lParam = MousePos.y << 16 | MousePos.x;
    ClientToScreen( AOWnd, &MousePos );
    SetCursorPos( MousePos.x, MousePos.y );
    SendMessage( AOWnd, WM_MOUSEMOVE, 0, lParam );
    Sleep( 250 );
    SendMessage( AOWnd, WM_LBUTTONUP, 0, lParam );
    Sleep( 250 );
}


/*
these coords are from my initial observation with a macro program. they are ofset slightly.
; options button
; 200, 185
; difficulty slider
; 110, 165
; 1st slider
; 110, 210
; then add 15 pixels in Y for each subsequent row
; full left slider
; X = 60
; full right slider
; X = 170
*/


float _linIinterp( float lo, float hi, float ratio )
{
    return ( hi - lo )*ratio + lo;
}


void _setSliders( int easy_hard, int good_bad, int order_chaos, int open_hidden, int phys_myst, int headon_stealth, int money_xp )
{
    int ypos = 210;

    //_dragMouse(200, 165, 200, 165);
    if( easy_hard != 50 ) _dragMouse( 102, 160, (int)_linIinterp( 64, 141, easy_hard / 100.0f ), 160 );
    if( good_bad != 50 ) _dragMouse( 102, ypos, (int)_linIinterp( 64, 141, good_bad / 100.0f ), ypos );
    ypos += 18;
    if( order_chaos != 50 ) _dragMouse( 102, ypos, (int)_linIinterp( 64, 141, order_chaos / 100.0f ), ypos );
    ypos += 18;
    if( open_hidden != 50 ) _dragMouse( 102, ypos, (int)_linIinterp( 64, 141, open_hidden / 100.0f ), ypos );
    ypos += 18;
    if( phys_myst != 50 ) _dragMouse( 102, ypos, (int)_linIinterp( 64, 141, phys_myst / 100.0f ), ypos );
    ypos += 18;
    if( headon_stealth != 50 ) _dragMouse( 102, ypos, (int)_linIinterp( 64, 141, headon_stealth / 100.0f ), ypos );
    ypos += 18;
    if( money_xp != 50 ) _dragMouse( 102, ypos, (int)_linIinterp( 64, 141, money_xp / 100.0f ), ypos );
}
