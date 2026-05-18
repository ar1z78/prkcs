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
#include "BuyingAgent.h"

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

void CleanUp()
{
	if (g_Thread != INVALID_HANDLE_VALUE)
	{
		TerminateThread(g_Thread, 0);
	}

	if (g_Mutex != INVALID_HANDLE_VALUE)
	{
		CloseHandle(g_Mutex);
	}

	ReleaseAODatabase();

	puDeleteObjectCollection(g_pCol);
	puClear();
}
// ========== WINDOW SUBCLASS FOR TIMER HANDLING ==========
LRESULT CALLBACK MainWndProcHook(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	if (uMsg == WM_TIMER && wParam == BUYINGAGENT_TIMER)
	{
		// Timer expired – kill it and post an application message to the main loop
		KillTimer(hWnd, BUYINGAGENT_TIMER);
		g_TimerID = 0;
		puPostAppMessage(CSAM_BUYINGAGENT_DOMISSION, 0);
		return 0;
	}
	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}
// ========================================================
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
	g_Mutex = CreateMutex(NULL, FALSE, "Ar1zClickSaver");
	DWORD dwMutexError = GetLastError(); // Immediately cache the error status

	if (g_Mutex == NULL)
	{
		ShowErrorMessage("Couldn't create mutex.");

		CleanUp();
		return -1;
	}

	if (dwMutexError == ERROR_ALREADY_EXISTS)
	{
		HWND hWnd = FindWindow("Ar1zClickSaverHookWindowClass", "Ar1zClickSaverHookWindow");
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

	// Subclass the main window to catch WM_TIMER
	HWND hMainWnd = (HWND)puGetAttribute(g_MainWin, PUA_WINDOW_HANDLE);
	SetWindowSubclass(hMainWnd, MainWndProcHook, 0, 0);

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
			// Kill any pending timer
			if (g_TimerID) {
				KillTimer(hMainWnd, BUYINGAGENT_TIMER);
				g_TimerID = 0;
			}
            g_BuyingAgentCount = 0;
            g_BuyingAgentMissions = 0;
			EndBuyingAgent();

            // Fall through

		case CSAM_BUYINGAGENT_DOMISSION:
			// Timer expired – send the click if not paused
			if (g_BuyingAgentCount > 0)
			{
				HWND AOWnd = FindWindow("Anarchy client", NULL);
				if (AOWnd)
				{
					// 1. Get the thread IDs for both windows
					DWORD foregroundThreadID = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
					DWORD targetThreadID = GetWindowThreadProcessId(AOWnd, NULL);

					// 2. Attach threads if they are different
					if (foregroundThreadID != targetThreadID)
					{
						AttachThreadInput(foregroundThreadID, targetThreadID, TRUE);
					}

					// 3. Force the window to the foreground and show it
					ShowWindow(AOWnd, SW_RESTORE); // Ensures it isn't minimized
					SetForegroundWindow(AOWnd);
					SetFocus(AOWnd);

					// 4. Detach the threads immediately
					if (foregroundThreadID != targetThreadID)
					{
						AttachThreadInput(foregroundThreadID, targetThreadID, FALSE);
					}

					// 5. Run your mouse logic
					POINT MousePos = { 99, 180 };
					LPARAM lParam = MousePos.y << 16 | MousePos.x;
					if (g_bFirstRound)
					{
						ClientToScreen(AOWnd, &MousePos);
						SetCursorPos(MousePos.x, MousePos.y);
						g_bFirstRound = FALSE;
					}
					SendMessage(AOWnd, WM_LBUTTONDOWN, 0, lParam);
					SendMessage(AOWnd, WM_LBUTTONUP, 0, lParam);
				}


				// Decrement counters after sending click
				g_BuyingAgentCount--;

			}
			break;

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
                //puSetAttribute( g_MainWin, PUA_WINDOW_DEFERUPDATE, TRUE );

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

				if (PUL_GET_CB(CS_SOUNDS_CB) && !(pAppMsg->Message == CSAM_STOPBUYINGAGENT))
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

        case CSAM_STARTBUYINGAGENT:
			if (puGetAttribute(puGetObjectFromCollection(g_pCol, CS_BAINFO_CB), PUA_CHECKBOX_CHECKED))
			{
				ShowInfoMessage("The buying agent will generate missions from the terminal automatically "
					"until it finds a mission that matches your search criteria. "
					"You have to open the mission terminal window and put it in the upper left corner "
					"before starting the buying agent. "
					"Be sure to set up a reasonnable amount of maximum number tries before starting.");
			}
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



