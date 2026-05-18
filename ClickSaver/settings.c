#include "settings.h"
#include <stdlib.h>
#include <string.h>
#include "clicksaver.h" 
#include "globals.h"

typedef enum ImportSettingsMode
{
	ISM_CONFIG,
	ISM_LOCWATCH,
	ISM_ITEMWATCH,
	ISM_SLIDERS,
	ISM_DONE,
} ImportSettingsMode;

enum
{
    CFG_AODIR,
    CFG_WINDOWX,
    CFG_WINDOWY,
    CFG_WINDOWWIDTH,
    CFG_STARTMINIMIZED,
    CFG_WATCHMSGBOX,
    CFG_ALERTITEM,
    CFG_ALERTLOC,
    CFG_ALERTTYPE,
    CFG_BUYINGAGENTMAXTRIES,
    CFG_BUYINGAGENTHIDE,
    CFG_BUYINGAGENTSHOWHELP,
    CFG_MISSIONTYPES,
    CFG_HIGHLIGHTOPTS,
    CFG_SOUNDS,
    CFG_LOG,
    CFG_MOUSEMOVE,
    CFG_EXPAND,
    CFG_ITEMVALUE,

    CFG_SLIDER_EASY_HARD,
    CFG_SLIDER_GOOD_BAD,
    CFG_SLIDER_ORDER_CHAOS,
    CFG_SLIDER_OPEN_HIDDEN,
    CFG_SLIDER_PHYS_MYST,
    CFG_SLIDER_HEADON_STEALTH,
    CFG_SLIDER_MONEY_XP,

    CFG_BUYMOD,
	CFG_ROLLWAIT,
};


struct
{
    int id;
    char* keyword;
} CfgKeywords[] =
{
    { CFG_AODIR, "AODIR" },
    { CFG_SOUNDS, "SOUNDS" },
    { CFG_MOUSEMOVE, "MOUSEMOVE" },
    { CFG_LOG, "LOG" },
    { CFG_WINDOWX, "WINDOWX" },
    { CFG_WINDOWY, "WINDOWY" },
    { CFG_WINDOWWIDTH, "WINDOWWIDTH" },
    { CFG_STARTMINIMIZED, "STARTMINIMIZED" },
    { CFG_WATCHMSGBOX, "WATCHMSGBOX" },
    { CFG_BUYINGAGENTSHOWHELP, "BUYINGAGENTSHOWHELP" },
    { CFG_ALERTLOC, "ALERTLOC" },
    { CFG_ALERTITEM, "ALERTITEM" },
    { CFG_ALERTTYPE, "ALERTTYPE" },
    { CFG_BUYINGAGENTMAXTRIES, "BUYINGAGENTMAXTRIES" },
    { CFG_BUYINGAGENTHIDE, "BUYINGAGENTHIDE" },
    { CFG_MISSIONTYPES, "MISHTYPES" },
    { CFG_HIGHLIGHTOPTS, "HIGHLIGHTOPTS" },
    { CFG_EXPAND, "EXPAND" },

    { CFG_SLIDER_EASY_HARD, "SLIDER_EASY_HARD" },
    { CFG_SLIDER_GOOD_BAD, "SLIDER_GOOD_BAD" },
    { CFG_SLIDER_ORDER_CHAOS, "SLIDER_ORDER_CHAOS" },
    { CFG_SLIDER_OPEN_HIDDEN, "SLIDER_OPEN_HIDDEN" },
    { CFG_SLIDER_PHYS_MYST, "SLIDER_PHYS_MYST" },
    { CFG_SLIDER_HEADON_STEALTH, "SLIDER_HEADON_STEALTH" },
    { CFG_SLIDER_MONEY_XP, "SLIDER_MONEY_XP" },

    { CFG_ITEMVALUE, "ITEMVALUE" },

    { CFG_BUYMOD, "BUYMOD" },

	{ CFG_ROLLWAIT, "ROLLWAIT" },

    { 0, NULL }
};

void ImportSettings( char* filename )
{
    FILE* fp;
    PUU32 Record;
    PUU8* pString;
    char buffer[ 1000 ];
    PUU8 Keyword[ 256 ], Value[ 256 ];
    int Id, i;
    PUU32 Val;
    int mode = ISM_DONE;
    char c;

    Record = puDoMethod( g_LocWatchList, PUM_TABLE_GETFIRSTRECORD, 0, 0 );
    while( Record )
    {
        puDoMethod( g_LocWatchList, PUM_TABLE_REMRECORD, Record, 0 );
        Record = puDoMethod( g_LocWatchList, PUM_TABLE_GETFIRSTRECORD, 0, 0 );
    }
    Record = puDoMethod( g_ItemWatchList, PUM_TABLE_GETFIRSTRECORD, 0, 0 );
    while( Record )
    {
        puDoMethod( g_ItemWatchList, PUM_TABLE_REMRECORD, Record, 0 );
        Record = puDoMethod( g_ItemWatchList, PUM_TABLE_GETFIRSTRECORD, 0, 0 );
    }

    if( !( fp = fopen( filename, "r" ) ) )
    {
        return;
    }

    while( fgets( buffer, 1000, fp ) )
    {
        if( sscanf( buffer, "::%s", &buffer ) == 1 )
        {
            strtok( buffer, ":" );
            if( !_stricmp( buffer, "Config" ) ) mode = ISM_CONFIG;
            if( !_stricmp( buffer, "LocWatch" ) ) mode = ISM_LOCWATCH;
            if( !_stricmp( buffer, "ItemWatch" ) ) mode = ISM_ITEMWATCH;

            if( !_stricmp( buffer, "Sliders" ) ) mode = ISM_SLIDERS;
            if( !_stricmp( buffer, "Done" ) ) mode = ISM_DONE;
            continue;
        }
        switch( mode )
        {
        case ISM_DONE:
            break;

        case ISM_CONFIG:
            if( sscanf( buffer, "%[^:]::%[^\n]\n", Keyword, Value ) != EOF )
            {
                i = 0, Id = -1;
                while( CfgKeywords[ i ].keyword )
                {
                    if( !strcmp( Keyword, CfgKeywords[ i ].keyword ) )
                    {
                        Id = CfgKeywords[ i ].id;
                        break;
                    }

                    i++;
                }

                switch( Id )
                {
                case CFG_AODIR:
                    strcpy( g_AODir, Value );
                    break;

                case CFG_WINDOWX:
                    sscanf( Value, "%u", &Val );
                    if( Val < 16384 )
                    {
                        puSetAttribute( g_MainWin, PUA_WINDOW_XPOS, Val );
                    }
                    break;

                case CFG_WINDOWY:
                    sscanf( Value, "%u", &Val );
                    if( Val < 16384 )
                    {
                        puSetAttribute( g_MainWin, PUA_WINDOW_YPOS, Val );
                    }
                    break;

                case CFG_WINDOWWIDTH:
                    sscanf( Value, "%u", &Val );
                    puSetAttribute( g_MainWin, PUA_WINDOW_WIDTH, Val );
                    break;

                case CFG_STARTMINIMIZED:
                    sscanf( Value, "%u", &Val );
                    //puSetAttribute( puGetObjectFromCollection( g_pCol, CS_STARTMIN_CB ), PUA_CHECKBOX_CHECKED, ( Val ? TRUE : FALSE ) );
					g_Settings.bStartMinimized = Val;
                    break;

                case CFG_WATCHMSGBOX:
                    sscanf( Value, "%u", &Val );
					g_Settings.bAlertBox = Val;
                    break;

                case CFG_BUYINGAGENTSHOWHELP:
                    sscanf( Value, "%u", &Val );
					g_Settings.bShowHelp = Val;
					break;

                case CFG_SOUNDS:
                    sscanf( Value, "%u", &Val );
					g_Settings.bSounds = Val;
                    break;

                case CFG_MOUSEMOVE:
                    sscanf( Value, "%u", &Val );
					g_Settings.bSelectMatch = Val;
                    break;

                case CFG_EXPAND:
                    sscanf( Value, "%u", &Val );
					g_Settings.bAutoExpand = Val;
                    break;

                case CFG_LOG:
                    sscanf( Value, "%u", &Val );
                    g_Settings.bLogging = Val;
                    break;

                case CFG_ALERTITEM:
                    sscanf( Value, "%u", &Val );
                    puSetAttribute( puGetObjectFromCollection( g_pCol, CS_ALERTITEM_CB ), PUA_CHECKBOX_CHECKED, ( Val ? TRUE : FALSE ) );
                    break;

                case CFG_ALERTLOC:
                    sscanf( Value, "%u", &Val );
                    puSetAttribute( puGetObjectFromCollection( g_pCol, CS_ALERTLOC_CB ), PUA_CHECKBOX_CHECKED, ( Val ? TRUE : FALSE ) );
                    break;

                case CFG_ALERTTYPE:
                    sscanf( Value, "%u", &Val );
                    puSetAttribute( puGetObjectFromCollection( g_pCol, CS_ALERTTYPE_CB ), PUA_CHECKBOX_CHECKED, ( Val ? TRUE : FALSE ) );
                    break;

                case CFG_BUYINGAGENTMAXTRIES:
                    sscanf( Value, "%d", &Val );
                    puSetAttribute( puGetObjectFromCollection( g_pCol, CS_BUYINGAGENTTRIES ), PUA_TEXTENTRY_VALUE, Val );
                    break;

                case CFG_MISSIONTYPES:
                    sscanf( Value, "%u", &Val );
                    puSetAttribute( puGetObjectFromCollection( g_pCol, CS_TYPEREPAIR_CB ), PUA_CHECKBOX_CHECKED, ( ( Val & 0x01 ) ? TRUE : FALSE ) );
                    puSetAttribute( puGetObjectFromCollection( g_pCol, CS_TYPERETURN_CB ), PUA_CHECKBOX_CHECKED, ( ( Val & 0x02 ) ? TRUE : FALSE ) );
                    puSetAttribute( puGetObjectFromCollection( g_pCol, CS_TYPEFINDP_CB ), PUA_CHECKBOX_CHECKED, ( ( Val & 0x04 ) ? TRUE : FALSE ) );
                    puSetAttribute( puGetObjectFromCollection( g_pCol, CS_TYPEFINDI_CB ), PUA_CHECKBOX_CHECKED, ( ( Val & 0x08 ) ? TRUE : FALSE ) );
                    puSetAttribute( puGetObjectFromCollection( g_pCol, CS_TYPEASS_CB ), PUA_CHECKBOX_CHECKED, ( ( Val & 0x10 ) ? TRUE : FALSE ) );
                    break;

                case CFG_HIGHLIGHTOPTS:
                    sscanf( Value, "%u", &Val );
                    puSetAttribute( puGetObjectFromCollection( g_pCol, CS_HIGHLIGHTITEM_CB ), PUA_CHECKBOX_CHECKED, ( ( Val & 0x01 ) ? TRUE : FALSE ) );
                    puSetAttribute( puGetObjectFromCollection( g_pCol, CS_HIGHLIGHTLOC_CB ), PUA_CHECKBOX_CHECKED, ( ( Val & 0x02 ) ? TRUE : FALSE ) );
                    puSetAttribute( puGetObjectFromCollection( g_pCol, CS_HIGHLIGHTTYPE_CB ), PUA_CHECKBOX_CHECKED, ( ( Val & 0x04 ) ? TRUE : FALSE ) );
                    break;

                case CFG_SLIDER_EASY_HARD:
					sscanf(Value, "%u", &Val);
					g_Settings.Sliders[0] = Val;
				case CFG_SLIDER_GOOD_BAD:
					sscanf(Value, "%u", &Val);
					g_Settings.Sliders[1] = Val;
				case CFG_SLIDER_ORDER_CHAOS:
					sscanf(Value, "%u", &Val);
					g_Settings.Sliders[2] = Val;
				case CFG_SLIDER_OPEN_HIDDEN:
					sscanf(Value, "%u", &Val);
					g_Settings.Sliders[3] = Val;
				case CFG_SLIDER_PHYS_MYST:
					sscanf(Value, "%u", &Val);
					g_Settings.Sliders[4] = Val;
				case CFG_SLIDER_HEADON_STEALTH:
					sscanf(Value, "%u", &Val);
					g_Settings.Sliders[5] = Val;
				case CFG_SLIDER_MONEY_XP:
                    sscanf( Value, "%u", &Val );
					g_Settings.Sliders[6] = Val ;
                    break;

                case CFG_BUYMOD:
                    sscanf( Value, "%u", &Val );
                    g_Settings.iBuyMod = Val ;
                    break;

				case CFG_ROLLWAIT:
					sscanf(Value, "%u", &Val);
					g_Settings.dwWaitTime = Val;
					break;

                case CFG_ITEMVALUE:
                {
                    PUU32 a, b, c, d;
                    sscanf( Value, "%u::%u::%u::%u", &a, &b, &c, &d );
					g_Settings.bMatchTotal = a ;
					g_Settings.iMatchTotalVal = b ;
					g_Settings.bMatchSingle = c ;
					g_Settings.bMatchTotal = d;
                }
                break;
                }
            }
            break;

        case ISM_ITEMWATCH:
        case ISM_LOCWATCH:

            pString = buffer + strlen( buffer );

            while( pString > buffer )
            {
                c = *--pString;
                if( c != ' ' && c != '\t' && c != '\n' )
                {
                    break;
                }
            }

            *( pString + 1 ) = 0;

            // Strip leading spaces/tab
            pString = buffer;

            while( c = *pString++ )
            {
                if( c != ' ' && c != '\t' )
                {
                    break;
                }
            }

            pString--;

            // If the resulting string isn't empty, add it to the list
            if( *pString )
            {
                puDoMethod( ( mode == ISM_ITEMWATCH ? g_ItemWatchList : g_LocWatchList ), PUM_TABLE_NEWRECORD, 0, 0 );
                puDoMethod( ( mode == ISM_ITEMWATCH ? g_ItemWatchList : g_LocWatchList ), PUM_TABLE_ADDRECORD, 0, 0 );
                puDoMethod( ( mode == ISM_ITEMWATCH ? g_ItemWatchList : g_LocWatchList ), PUM_TABLE_SETFIELDVAL, (PUU32)pString, 0 );
            }
            break;
        }
    }

    fclose( fp );
}


void ExportSettings( char* filename )
{
    FILE* fp;
    pusRect Rect;
    PUU32 Record;
    PUU8* pString;
    unsigned int Val = 0;
    char* myfilename;

    myfilename = malloc( strlen( filename ) + 5 );
    strcpy( myfilename, filename );

    if( !strstr( myfilename, ".cs" ) )
    {
        strcat( myfilename, ".cs" );
    }

    if( !( fp = fopen( myfilename, "w" ) ) )
    {
        free( myfilename );
        return;
    }
    free( myfilename );
    fprintf( fp, "::Config::\n" );
    fprintf( fp, "AODIR::%s\n", g_AODir );

    puDoMethod( g_MainWin, PUM_WINDOW_GETRECT, (PUU32)&Rect, 0 );
    fprintf( fp, "WINDOWX::%d\nWINDOWY::%d\nWINDOWWIDTH::%d\n", Rect.X, Rect.Y, Rect.Width );

    fprintf( fp, "STARTMINIMIZED::%u\n", g_Settings.bStartMinimized );

	fprintf( fp, "WATCHMSGBOX::%u\n", g_Settings.bAlertBox );

    fprintf( fp, "BUYINGAGENTSHOWHELP::%u\n", g_Settings.bShowHelp );

	fprintf( fp, "SOUNDS::%u\n", g_Settings.bSounds );

    fprintf( fp, "EXPAND::%u\n", g_Settings.bAutoExpand );

    fprintf( fp, "MOUSEMOVE::%u\n", g_Settings.bSelectMatch );

    fprintf( fp, "LOG::%u\n", g_Settings.bLogging );

    fprintf( fp, "ALERTITEM::%u\n",
             puGetAttribute( puGetObjectFromCollection( g_pCol, CS_ALERTITEM_CB ), PUA_CHECKBOX_CHECKED ) );

    fprintf( fp, "ALERTLOC::%u\n",
             puGetAttribute( puGetObjectFromCollection( g_pCol, CS_ALERTLOC_CB ), PUA_CHECKBOX_CHECKED ) );

    fprintf( fp, "ALERTTYPE::%u\n",
             puGetAttribute( puGetObjectFromCollection( g_pCol, CS_ALERTTYPE_CB ), PUA_CHECKBOX_CHECKED ) );

    fprintf( fp, "BUYINGAGENTMAXTRIES::%u\n",
             puGetAttribute( puGetObjectFromCollection( g_pCol, CS_BUYINGAGENTTRIES ), PUA_TEXTENTRY_VALUE ) );

    if( puGetAttribute( puGetObjectFromCollection( g_pCol, CS_TYPEREPAIR_CB ), PUA_CHECKBOX_CHECKED ) ) Val |= 0x01;
    if( puGetAttribute( puGetObjectFromCollection( g_pCol, CS_TYPERETURN_CB ), PUA_CHECKBOX_CHECKED ) ) Val |= 0x02;
    if( puGetAttribute( puGetObjectFromCollection( g_pCol, CS_TYPEFINDP_CB ), PUA_CHECKBOX_CHECKED ) ) Val |= 0x04;
    if( puGetAttribute( puGetObjectFromCollection( g_pCol, CS_TYPEFINDI_CB ), PUA_CHECKBOX_CHECKED ) ) Val |= 0x08;
    if( puGetAttribute( puGetObjectFromCollection( g_pCol, CS_TYPEASS_CB ), PUA_CHECKBOX_CHECKED ) ) Val |= 0x10;

    fprintf( fp, "MISHTYPES::%u\n", Val );

    Val = 0;
    if( puGetAttribute( puGetObjectFromCollection( g_pCol, CS_HIGHLIGHTITEM_CB ), PUA_CHECKBOX_CHECKED ) ) Val |= 0x01;
    if( puGetAttribute( puGetObjectFromCollection( g_pCol, CS_HIGHLIGHTLOC_CB ), PUA_CHECKBOX_CHECKED ) ) Val |= 0x02;
    if( puGetAttribute( puGetObjectFromCollection( g_pCol, CS_HIGHLIGHTTYPE_CB ), PUA_CHECKBOX_CHECKED ) ) Val |= 0x04;

    fprintf( fp, "HIGHLIGHTOPTS::%u\n", Val );

	fprintf(fp, "SLIDER_EASY_HARD::%u\n", g_Settings.Sliders[0] );

    fprintf( fp, "SLIDER_GOOD_BAD::%u\n", g_Settings.Sliders[1] );

    fprintf( fp, "SLIDER_ORDER_CHAOS::%u\n", g_Settings.Sliders[2] );

    fprintf( fp, "SLIDER_OPEN_HIDDEN::%u\n", g_Settings.Sliders[3] );

    fprintf( fp, "SLIDER_PHYS_MYST::%u\n", g_Settings.Sliders[4] );

    fprintf( fp, "SLIDER_HEADON_STEALTH::%u\n", g_Settings.Sliders[5] );

    fprintf( fp, "SLIDER_MONEY_XP::%u\n", g_Settings.Sliders[6] );

    fprintf( fp, "BUYMOD::%u\n", g_Settings.iBuyMod );

    fprintf( fp, "ITEMVALUE::%u::%u::%u::%u\n", g_Settings.bMatchTotal, g_Settings.iMatchTotalVal, g_Settings.bMatchSingle, g_Settings.bMatchTotal);

	fprintf( fp, "ROLLWAIT::%u\n", g_Settings.dwWaitTime );

    fprintf( fp, "::ItemWatch::\n" );
    Record = puDoMethod( g_ItemWatchList, PUM_TABLE_GETFIRSTRECORD, 0, 0 );
    while( Record )
    {
        if( pString = (PUU8*)puDoMethod( g_ItemWatchList, PUM_TABLE_GETFIELDVAL, Record, 0 ) )
        {
            fprintf( fp, "%s\n", pString );
        }

        Record = puDoMethod( g_ItemWatchList, PUM_TABLE_GETNEXTRECORD, Record, 0 );
    }
    fprintf( fp, "::LocWatch::\n" );
    Record = puDoMethod( g_LocWatchList, PUM_TABLE_GETFIRSTRECORD, 0, 0 );
    while( Record )
    {
        if( pString = (PUU8*)puDoMethod( g_LocWatchList, PUM_TABLE_GETFIELDVAL, Record, 0 ) )
        {
            fprintf( fp, "%s\n", pString );
        }

        Record = puDoMethod( g_LocWatchList, PUM_TABLE_GETNEXTRECORD, Record, 0 );
    }
    fprintf( fp, "::END::\n" );

    fclose( fp );
}
