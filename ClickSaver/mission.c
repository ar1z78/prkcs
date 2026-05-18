/*
 * $Log: mission.c,v $
 * Revision 1.14  2006/08/27 10:40:00  Darkbane
 * 2.3.2 - modified parseMission() to cope with 16.3 changes
 *
 * Revision 1.13  2004/01/25 19:36:05  gnarf37
 * 2.3.0 beta 3 - Shrunk Database a bit, added Item Value options, make options menu smaller a tad so that 800x600 might be able to use it again...
 *
 * Revision 1.12  2004/01/23 06:58:00  ibender
 * test to see if I can check in
 *
 * Revision 1.11  2003/11/06 23:41:51  gnarf37
 * Version 2.3.0 beta 2 - Fixed issues with 15.2.0 and added an option for auto expand team missions
 *
 * Revision 1.10  2003/05/27 00:14:42  gnarf37
 * Added Checkbox to stop mouse movement, and cleaned up mission info parsing so it doesnt match stale missions
 *
 * Revision 1.9  2003/05/08 09:11:09  gnarf37
 * Fullscreen Mode
 *
 * Revision 1.8  2003/05/08 08:40:04  gnarf37
 * Added Logging to Missions
 *
 * Revision 1.7  2003/05/08 06:58:50  gnarf37
 * Added XP to mission display
 *
 * Revision 1.6  2003/05/08 06:01:33  gnarf37
 * Fixed Bug with no item reward in missions
 *
 */
/*
ClickSaver mission data parser and display -  Anarchy Online mission helper
Copyright (C) 2001, 2002 Morb
Some parts Copyright (C) 2003, 2004 gnarf

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
#include "Globals.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "aomd.h"
#include "ClickSaver.h"
#include "rdb.h"
#include "CoreUtils.h"

// Turn on this define if you want to output the mission packets in Hex..
//#define DEBUG_MISSION_PACKETS 1

PUU32 MissionSetAttr( PULID _Object, PULID _Class, void* _pData, PUU32 _Attr, PUU32 _Val );
PUU32 MissionParse( PULID _Object, MissionClassData* _pData, PUU8* _pMissionData );
PUU32 ShowItem( MissionClassData* _pData, Item* _pItem, PUU32 _ObjId, PUU32 _ValId );
PUU32 SetAndSearch( PUU8* _pSrcString, PULID _TextEntry, PULID _List );
PUU32 SetAndSearchType( PUU32 TempVal, PULID _TextEntry );
PUU32 ItemMatch( PUU8* ItemName, PUU8* ItemSearch );
PUU32 LocationMatch( PUU8* LocationName, PUU8* LocationSearch );


PULID RegisterMissionClass()
{
    PULID SuperClass;

    if( !( SuperClass = puFindClass( "Container" ) ) )
    {
        return 0;
    }

    return puRegisterClass( "CSMission", MissionClassHandler, sizeof( MissionClassData ), SuperClass );
}


static PUU32 ColDefSingle[] =
{
    PU_ACTION_OBJDEF, ROOTOBJ, ( PUU32 )"HorGroup", PUA_CONTROL_FRAME, PUFRAME_TITLE, 0,
        PUM_ADDCHILD, PU_FIXED_VERGROUP,
            PUM_ADDCHILD, PU_SPACER,
            PUM_ADDCHILD, PU_LABEL( "Loc: " ),
            PUM_ADDCHILD, PU_SPACER,
            PUM_ADDCHILD, PU_LABEL( "Type: " ),
            PUM_ADDCHILD, PU_SPACER,
            PUM_ADDCHILD, PU_LABEL( "Item:" ),
            PUM_ADDCHILD, PU_SPACER,
            PUM_ADDCHILD, PU_LABEL( "Find:" ),
            PUM_ADDCHILD, PU_SPACER,
        PU_ENDGROUP,

        PUM_ADDCHILD, PU_VERGROUP,

            PUM_ADDCHILD, PU_HORGROUP,
                PUM_ADDCHILD, PU_ACTION_OBJDEF, LOCATION, ( PUU32 )"TextEntry",
                    PUA_TEXTENTRY_READONLY, TRUE,
                    PUA_CONTROL_WEIGHT, 1024, 0, 0,
                PUM_ADDCHILD, PU_LABEL( "XP:" ),
                PUM_ADDCHILD, PU_ACTION_OBJDEF, MISHXP, ( PUU32 )"Text",
                    PUA_CONTROL_FRAME, PUFRAME_READONLYTEXTENTRY,
                    PUA_TEXT_STRING, ( PUU32 )" ",
                    PUA_TEXT_CENTERMODE, PU_TEXT_RIGHT,
                    PUA_TEXT_FIXEDWIDTH, TRUE,
                    PUA_TEXT_MINWIDTH, 50, 0, 0,
            PU_ENDGROUP,

            PUM_ADDCHILD, PU_HORGROUP,
                PUM_ADDCHILD, PU_ACTION_OBJDEF, MISHTYPE, ( PUU32 )"TextEntry", PUA_TEXTENTRY_READONLY, TRUE, 0, 0,
                PUM_ADDCHILD, PU_LABEL( "Cash:" ),
                PUM_ADDCHILD, PU_ACTION_OBJDEF, CASH, ( PUU32 )"Text",
                    PUA_CONTROL_FRAME, PUFRAME_READONLYTEXTENTRY,
                    PUA_TEXT_STRING, ( PUU32 )" ",
                    PUA_TEXT_CENTERMODE, PU_TEXT_RIGHT,
                    PUA_TEXT_FIXEDWIDTH, TRUE,
                    PUA_TEXT_MINWIDTH, 50, 0, 0,
            PU_ENDGROUP,

            PUM_ADDCHILD, PU_HORGROUP,
                PUM_ADDCHILD, PU_ACTION_OBJDEF, ITEM1, ( PUU32 )"TextEntry", PUA_TEXTENTRY_READONLY, TRUE, 0, 0,
                PUM_ADDCHILD, PU_LABEL( "Value:" ),
                PUM_ADDCHILD, PU_ACTION_OBJDEF, ITEMVAL1, ( PUU32 )"TextEntry",
                    PUA_TEXTENTRY_READONLY, TRUE,
                    PUA_CONTROL_FRAME, PUFRAME_READONLYTEXTENTRY,
                    PUA_TEXTENTRY_NUMERIC, TRUE,
                    PUA_TEXTENTRY_BUFFERSIZE, 9,
                    PUA_TEXTENTRY_VALUE, 0,
                    PUA_TEXTENTRY_MIN, 0,
                    PUA_TEXTENTRY_MAX, 999999999,
                    PUA_CONTROL_WEIGHT, 1,
                    PUA_CONTROL_MAXWIDTH, 55,
                    PUA_CONTROL_MINWIDTH, 55,
                    0, 0,
            PU_ENDGROUP,

            PUM_ADDCHILD, PU_HORGROUP,
                PUM_ADDCHILD, PU_ACTION_OBJDEF, FINDITEM, ( PUU32 )"TextEntry", PUA_TEXTENTRY_READONLY, TRUE, 0, 0,
                PUM_ADDCHILD, PU_LABEL( "Total:" ),
                PUM_ADDCHILD, PU_ACTION_OBJDEF, TOTALVAL, ( PUU32 )"TextEntry",
                    PUA_TEXTENTRY_READONLY, TRUE,
                    PUA_CONTROL_FRAME, PUFRAME_READONLYTEXTENTRY,
                    PUA_TEXTENTRY_NUMERIC, TRUE,
                    PUA_TEXTENTRY_BUFFERSIZE, 9,
                    PUA_TEXTENTRY_VALUE, 0,
                    PUA_TEXTENTRY_MIN, 0,
                    PUA_TEXTENTRY_MAX, 999999999,
                    PUA_TEXT_FIXEDWIDTH, TRUE,
                    PUA_CONTROL_WEIGHT, 1,
                    PUA_CONTROL_MAXWIDTH, 55,
                    PUA_CONTROL_MINWIDTH, 55,
                    0, 0,
            PU_ENDGROUP,

        PU_ENDGROUP,

        PUM_ADDCHILD, PU_VERGROUP,
            PUM_ADDCHILD, PU_ACTION_OBJDEF, IMAGE, ( PUU32 )"Image",
                PUA_CONTROL_FRAME, PUFRAME_INFOBOX,
                PUA_IMAGE_WIDTH, 48,
                PUA_IMAGE_HEIGHT, 48,
                PUA_IMAGE_PIXFORMAT, 24,
                0, 0,
        PU_ENDGROUP,
    PU_ENDGROUP,

    PU_ACTION_END
};


static PUU32 ColDefTeam[] =
{
    PU_ACTION_OBJDEF, ROOTOBJ, ( PUU32 )"VerGroup", PUA_CONTROL_FRAME, PUFRAME_TITLE, 0,
        PUM_ADDCHILD, PU_HORGROUP,
            PUM_ADDCHILD, PU_FIXED_VERGROUP,
                PUM_ADDCHILD, PU_SPACER,
                PUM_ADDCHILD, PU_LABEL( "Loc: " ),
                PUM_ADDCHILD, PU_SPACER,
                PUM_ADDCHILD, PU_SPACER,
                PUM_ADDCHILD, PU_LABEL( "Type: " ),
                PUM_ADDCHILD, PU_LABEL( "Find:" ),
                PUM_ADDCHILD, PU_SPACER,
            PU_ENDGROUP,

            PUM_ADDCHILD, PU_VERGROUP,

                PUM_ADDCHILD, PU_HORGROUP,
                    PUM_ADDCHILD, PU_ACTION_OBJDEF, LOCATION, ( PUU32 )"TextEntry",
                        PUA_TEXTENTRY_READONLY, TRUE,
                        PUA_CONTROL_WEIGHT, 1024, 0, 0,
                    PUM_ADDCHILD, PU_LABEL( "XP:" ),
                    PUM_ADDCHILD, PU_ACTION_OBJDEF, MISHXP, ( PUU32 )"Text",
                        PUA_CONTROL_FRAME, PUFRAME_READONLYTEXTENTRY,
                        PUA_TEXT_STRING, ( PUU32 )" ",
                        PUA_TEXT_CENTERMODE, PU_TEXT_RIGHT,
                        PUA_TEXT_FIXEDWIDTH, TRUE,
                        PUA_TEXT_MINWIDTH, 50, 0, 0,
                PU_ENDGROUP,
                PUM_ADDCHILD, PU_HORGROUP,
                    PUM_ADDCHILD, PU_ACTION_OBJDEF, MISHTYPE, ( PUU32 )"TextEntry", PUA_TEXTENTRY_READONLY, TRUE, 0, 0,
                    PUM_ADDCHILD, PU_LABEL( "Cash:" ),
                    PUM_ADDCHILD, PU_ACTION_OBJDEF, CASH, ( PUU32 )"Text",
                        PUA_CONTROL_FRAME, PUFRAME_READONLYTEXTENTRY,
                        PUA_TEXT_STRING, ( PUU32 )" ",
                        PUA_TEXT_CENTERMODE, PU_TEXT_RIGHT,
                        PUA_TEXT_FIXEDWIDTH, TRUE,
                        PUA_TEXT_MINWIDTH, 50, 0, 0,
                PU_ENDGROUP,

                PUM_ADDCHILD, PU_HORGROUP,
                    PUM_ADDCHILD, PU_ACTION_OBJDEF, FINDITEM, ( PUU32 )"TextEntry", PUA_TEXTENTRY_READONLY, TRUE, 0, 0,
                    PUM_ADDCHILD, PU_LABEL( "Total:" ),
                    PUM_ADDCHILD, PU_ACTION_OBJDEF, TOTALVAL, ( PUU32 )"TextEntry",
                        PUA_TEXTENTRY_READONLY, TRUE,
                        PUA_CONTROL_FRAME, PUFRAME_READONLYTEXTENTRY,
                        PUA_TEXTENTRY_NUMERIC, TRUE,
                        PUA_TEXTENTRY_BUFFERSIZE, 9,
                        PUA_TEXTENTRY_VALUE, 0,
                        PUA_TEXTENTRY_MIN, 0,
                        PUA_TEXTENTRY_MAX, 999999999,
                        PUA_TEXT_FIXEDWIDTH, TRUE,
                        PUA_CONTROL_WEIGHT, 1,
                        PUA_CONTROL_MAXWIDTH, 55,
                        PUA_CONTROL_MINWIDTH, 55,
                        0, 0,
                PU_ENDGROUP,
            PU_ENDGROUP,

        PU_ENDGROUP,

        PUM_ADDCHILD, PU_ACTION_OBJDEF, FOLD, ( PUU32 )"Fold",
                PUA_FOLD_LABEL, ( PUU32 )"Items",
                PUA_FOLD_FOLDED, TRUE,
                PUA_FOLD_CONTENTS, PU_VERGROUP,
            PUM_ADDCHILD, PU_ACTION_OBJDEF, ITEMROW1, ( PUU32 )"HorGroup", PUA_CONTROL_KEEPROOM, FALSE, 0,
                PUM_ADDCHILD, PU_ACTION_OBJDEF, ITEM1, ( PUU32 )"TextEntry", PUA_TEXTENTRY_READONLY, TRUE, 0, 0,
                PUM_ADDCHILD, PU_LABEL( "Value:" ),
                PUM_ADDCHILD, PU_ACTION_OBJDEF, ITEMVAL1, ( PUU32 )"TextEntry",
                    PUA_TEXTENTRY_READONLY, TRUE,
                    PUA_CONTROL_FRAME, PUFRAME_READONLYTEXTENTRY,
                    PUA_TEXTENTRY_NUMERIC, TRUE,
                    PUA_TEXTENTRY_BUFFERSIZE, 9,
                    PUA_TEXTENTRY_VALUE, 0,
                    PUA_TEXTENTRY_MIN, 0,
                    PUA_TEXTENTRY_MAX, 999999999,
                    PUA_CONTROL_WEIGHT, 1,
                    PUA_CONTROL_MAXWIDTH, 55,
                    PUA_CONTROL_MINWIDTH, 55,
                    0, 0,
            PU_ENDGROUP,
            PUM_ADDCHILD, PU_ACTION_OBJDEF, ITEMROW2, ( PUU32 )"HorGroup", PUA_CONTROL_KEEPROOM, FALSE, 0,
                PUM_ADDCHILD, PU_ACTION_OBJDEF, ITEM2, ( PUU32 )"TextEntry", PUA_TEXTENTRY_READONLY, TRUE, 0, 0,
                PUM_ADDCHILD, PU_LABEL( "Value:" ),
                PUM_ADDCHILD, PU_ACTION_OBJDEF, ITEMVAL2, ( PUU32 )"TextEntry",
                    PUA_TEXTENTRY_READONLY, TRUE,
                    PUA_CONTROL_FRAME, PUFRAME_READONLYTEXTENTRY,
                    PUA_TEXTENTRY_NUMERIC, TRUE,
                    PUA_TEXTENTRY_BUFFERSIZE, 9,
                    PUA_TEXTENTRY_VALUE, 0,
                    PUA_TEXTENTRY_MIN, 0,
                    PUA_TEXTENTRY_MAX, 999999999,
                    PUA_CONTROL_WEIGHT, 1,
                    PUA_CONTROL_MAXWIDTH, 55,
                    PUA_CONTROL_MINWIDTH, 55,
                    0, 0,
            PU_ENDGROUP,
            PUM_ADDCHILD, PU_ACTION_OBJDEF, ITEMROW3, ( PUU32 )"HorGroup", PUA_CONTROL_KEEPROOM, FALSE, 0,
                PUM_ADDCHILD, PU_ACTION_OBJDEF, ITEM3, ( PUU32 )"TextEntry", PUA_TEXTENTRY_READONLY, TRUE, 0, 0,
                PUM_ADDCHILD, PU_LABEL( "Value:" ),
                PUM_ADDCHILD, PU_ACTION_OBJDEF, ITEMVAL3, ( PUU32 )"TextEntry",
                    PUA_TEXTENTRY_READONLY, TRUE,
                    PUA_CONTROL_FRAME, PUFRAME_READONLYTEXTENTRY,
                    PUA_TEXTENTRY_NUMERIC, TRUE,
                    PUA_TEXTENTRY_BUFFERSIZE, 9,
                    PUA_TEXTENTRY_VALUE, 0,
                    PUA_TEXTENTRY_MIN, 0,
                    PUA_TEXTENTRY_MAX, 999999999,
                    PUA_CONTROL_WEIGHT, 1,
                    PUA_CONTROL_MAXWIDTH, 55,
                    PUA_CONTROL_MINWIDTH, 55,
                    0, 0,
            PU_ENDGROUP,
            PUM_ADDCHILD, PU_ACTION_OBJDEF, ITEMROW4, ( PUU32 )"HorGroup", PUA_CONTROL_KEEPROOM, FALSE, 0,
                PUM_ADDCHILD, PU_ACTION_OBJDEF, ITEM4, ( PUU32 )"TextEntry", PUA_TEXTENTRY_READONLY, TRUE, 0, 0,
                PUM_ADDCHILD, PU_LABEL( "Value:" ),
                PUM_ADDCHILD, PU_ACTION_OBJDEF, ITEMVAL4, ( PUU32 )"TextEntry",
                    PUA_TEXTENTRY_READONLY, TRUE,
                    PUA_CONTROL_FRAME, PUFRAME_READONLYTEXTENTRY,
                    PUA_TEXTENTRY_NUMERIC, TRUE,
                    PUA_TEXTENTRY_BUFFERSIZE, 9,
                    PUA_TEXTENTRY_VALUE, 0,
                    PUA_TEXTENTRY_MIN, 0,
                    PUA_TEXTENTRY_MAX, 999999999,
                    PUA_CONTROL_WEIGHT, 1,
                    PUA_CONTROL_MAXWIDTH, 55,
                    PUA_CONTROL_MINWIDTH, 55,
                    0, 0,
            PU_ENDGROUP,
            PUM_ADDCHILD, PU_ACTION_OBJDEF, ITEMROW5, ( PUU32 )"HorGroup", PUA_CONTROL_KEEPROOM, FALSE, 0,
                PUM_ADDCHILD, PU_ACTION_OBJDEF, ITEM5, ( PUU32 )"TextEntry", PUA_TEXTENTRY_READONLY, TRUE, 0, 0,
                PUM_ADDCHILD, PU_LABEL( "Value:" ),
                PUM_ADDCHILD, PU_ACTION_OBJDEF, ITEMVAL5, ( PUU32 )"TextEntry",
                    PUA_TEXTENTRY_READONLY, TRUE,
                    PUA_CONTROL_FRAME, PUFRAME_READONLYTEXTENTRY,
                    PUA_TEXTENTRY_NUMERIC, TRUE,
                    PUA_TEXTENTRY_BUFFERSIZE, 9,
                    PUA_TEXTENTRY_VALUE, 0,
                    PUA_TEXTENTRY_MIN, 0,
                    PUA_TEXTENTRY_MAX, 999999999,
                    PUA_CONTROL_WEIGHT, 1,
                    PUA_CONTROL_MAXWIDTH, 55,
                    PUA_CONTROL_MINWIDTH, 55,
                    0, 0,
            PU_ENDGROUP,
            PUM_ADDCHILD, PU_ACTION_OBJDEF, ITEMROW6, ( PUU32 )"HorGroup", PUA_CONTROL_KEEPROOM, FALSE, 0,
                PUM_ADDCHILD, PU_ACTION_OBJDEF, ITEM6, ( PUU32 )"TextEntry", PUA_TEXTENTRY_READONLY, TRUE, 0, 0,
                PUM_ADDCHILD, PU_LABEL( "Value:" ),
                PUM_ADDCHILD, PU_ACTION_OBJDEF, ITEMVAL6, ( PUU32 )"TextEntry",
                    PUA_TEXTENTRY_READONLY, TRUE,
                    PUA_CONTROL_FRAME, PUFRAME_READONLYTEXTENTRY,
                    PUA_TEXTENTRY_NUMERIC, TRUE,
                    PUA_TEXTENTRY_BUFFERSIZE, 9,
                    PUA_TEXTENTRY_VALUE, 0,
                    PUA_TEXTENTRY_MIN, 0,
                    PUA_TEXTENTRY_MAX, 999999999,
                    PUA_CONTROL_WEIGHT, 1,
                    PUA_CONTROL_MAXWIDTH, 55,
                    PUA_CONTROL_MINWIDTH, 55,
                    0, 0,
            PU_ENDGROUP,
                PU_ENDGROUP,
        0, 0,

    PU_ENDGROUP,

    PU_ACTION_END
};



PUU32 MissionClassHandler( PULID _Object, PULID _Class, void* _pData, PUU32 _MethodID, PUU32 _Param1, PUU32 _Param2 )
{
    MissionClassData* pData;
    pData = (MissionClassData*)_pData;

    switch( _MethodID )
    {
    case PUM_NEW:
        if( !puDoSuperMethod( _Object, _Class, _MethodID, _Param1, _Param2 ) )
        {
            return FALSE;
        }
        if( !( pData->pSingleCol = puCreateObjectCollection( ColDefSingle ) ) )
        {
            return FALSE;
        }
        if( !( pData->pTeamCol = puCreateObjectCollection( ColDefTeam ) ) )
        {
            return FALSE;
        }

        pData->pCol = pData->pSingleCol;

        pData->CashStr[ 0 ] = 0;
        pData->pImageData = NULL;

        puSetAttribute( _Object, PUA_CONTAINER_CONTENTS, puGetObjectFromCollection( pData->pCol, ROOTOBJ ) );
        return TRUE;

    case PUM_DELETE:
        puDeleteObjectCollection( pData->pSingleCol );
        puDeleteObjectCollection( pData->pTeamCol );
        if( pData->pImageData )
        {
            free( pData->pImageData );
        }
        return puDoSuperMethod( _Object, _Class, _MethodID, _Param1, _Param2 );


    case PUM_SET:
        puDoSuperMethod( _Object, _Class, _MethodID, _Param1, _Param2 );
        return MissionSetAttr( _Object, _Class, _pData, _Param1, _Param2 );

    case CSM_MISSION_PARSEMISSION:
        return MissionParse( _Object, _pData, (PUU8*)_Param1 );

    default:
        return puDoSuperMethod( _Object, _Class, _MethodID, _Param1, _Param2 );
    }

    return 0;
}


PUU32 MissionSetAttr( PULID _Object, PULID _Class, void* _pData, PUU32 _Attr, PUU32 _Val )
{
    MissionClassData* pData;
    pData = (MissionClassData*)_pData;

    switch( _Attr )
    {
    case CSA_MISSION_TITLE:
        puSetAttribute( puGetObjectFromCollection( pData->pSingleCol, ROOTOBJ ), PUA_CONTROL_LABEL, _Val );
        puSetAttribute( puGetObjectFromCollection( pData->pTeamCol, ROOTOBJ ), PUA_CONTROL_LABEL, _Val );
        break;
    }

    return TRUE;
}


PUU32 MissionParse( PULID _Object, MissionClassData* _pData, PUU8* _pMissionData )
{
    char TempStr[ 256 ], CharKey[ 6 ];
    char PFName[ 256 ];
    float CoordX, CoordY;
    PUU32 TempVal, MishPF;
    PUU32 Cash, XP, MishQL, MishID, TotalValue;
    //PUU32 Time;
    //PUU32 ItemKey1, ItemKey2, QL;
    PUU32 bAlertItem, bAlertLoc, bAlertType;
    PUU32 bItemFound = FALSE, bLocFound = FALSE, bAlert = FALSE, bTypeFound = FALSE;
    PUU32 Count = 65536 - 4, DescLength;
    PUU8* pEndMissionData;
    PUU8* pDesc;
    Item* pItem;
    Item* pTmpItem;
    PUU32 NumItems = 0, i;
    pusObjectCollection* pPrevCol;

    pEndMissionData = _pMissionData + 65536 - 4;
    bAlertItem = puGetAttribute( puGetObjectFromCollection( g_pCol, CS_ALERTITEM_CB ), PUA_CHECKBOX_CHECKED );
    bAlertLoc = puGetAttribute( puGetObjectFromCollection( g_pCol, CS_ALERTLOC_CB ), PUA_CHECKBOX_CHECKED );
    bAlertType = puGetAttribute( puGetObjectFromCollection( g_pCol, CS_ALERTTYPE_CB ), PUA_CHECKBOX_CHECKED );

    // If one of the watch list is empty, we don't take it into account to decide
    // whether to show an alertbox or not
    if( !puGetAttribute( g_ItemWatchList, PUA_TABLE_NUMRECORDS ) )
    {
        bAlertItem = FALSE;
    }

    if( !puGetAttribute( g_LocWatchList, PUA_TABLE_NUMRECORDS ) )
    {
        bAlertLoc = FALSE;
    }

    // Find start of mission packet (look for 0xDAC3 longword)
    do
    {
        if( _pMissionData >= pEndMissionData )
        {
            puSetAttribute( puGetObjectFromCollection( _pData->pCol, ROOTOBJ ), PUA_CONTROL_HIDDEN, TRUE );
            return 0;
        }
        _pMissionData++;
        TempVal = *(PUU32*)_pMissionData;
        TempVal = EndianSwap32( TempVal );
    }
    while( TempVal != 0xdac3 );

    puSetAttribute( puGetObjectFromCollection( _pData->pCol, ROOTOBJ ), PUA_CONTROL_HIDDEN, FALSE );

#ifdef DEBUG_MISSION_PACKETS // Debug Packet
    WriteDebug( "\nMission Header:\n" );
    DebugPacket( _pMissionData, 6 * 4 );
    WriteDebug( 0 );
#endif

    MishID = *(PUU32*)_pMissionData + 0x04;
    MishID = EndianSwap32( MishID );

    // Skip header (mission ID, etc.), 6 long words (was 5 pre 16.3)
    _pMissionData += 6 * 4;
    if( _pMissionData >= pEndMissionData )
        return 0;

    // Short Description: this now a null terminated string as of 16.3? just need to parse it byte at a time?
    TempVal = *_pMissionData;
    while( TempVal != 0 )
    {
        _pMissionData += 1;
        TempVal = *_pMissionData;
    }
    _pMissionData += 1;
    // Get shortened description length
    //TempVal = *( PUU32* )_pMissionData;
    //TempVal = EndianSwap32( TempVal );
    //TempVal &= 0xff;
    //_pMissionData += 4;
    if( _pMissionData >= pEndMissionData )
    {
        return 0;
    }

    // Skip shortened description
    //_pMissionData += TempVal;
    //if( _pMissionData >= pEndMissionData )
    //  return 0;

    // Get full description length
    TempVal = *(PUU32*)_pMissionData;
    TempVal = EndianSwap32( TempVal );
    _pMissionData += 4;
    if( _pMissionData >= pEndMissionData )
    {
        return 0;
    }

    pDesc = _pMissionData;
    DescLength = TempVal;

    // Skip full description
    _pMissionData += TempVal;
    if( _pMissionData >= pEndMissionData )
    {
        return 0;
    }

    if( ( pEndMissionData - _pMissionData ) < 0xe8 )
    {
        return 0;
    }

#ifdef DEBUG_MISSION_PACKETS // After Descriptions
    WriteDebug("\nAfter Descriptioins:\n");
    DebugPacket(_pMissionData, 0x24);
    WriteDebug(0);
#endif

    Cash = *(PUU32*)( _pMissionData + 0xc );
    TotalValue = Cash = EndianSwap32( Cash );

    XP = *(PUU32*)( _pMissionData + 0x14 );
    XP = EndianSwap32( XP );

    // Get start of items array, and count the number of items
    pTmpItem = pItem = (Item*)( _pMissionData + 0x24 );

    while( pTmpItem->Key1 != 0x2d2d2d2d )
    {
        MissionItem sItem;
        if( !GetAODBItem( &sItem, EndianSwap32( pTmpItem->Key1 ) ) )
        {
            strncpy( CharKey, (char *)&( pTmpItem->Padding ), 4 );
            CharKey[ 4 ] = 0;
            break;
        }

        NumItems++;
        pTmpItem++;

        if( pEndMissionData < (PUU8*)pTmpItem )
        {
            return 0;
        }
    }

    _pMissionData = ( (PUU8*)pTmpItem ) + 4;
    if( _pMissionData >= pEndMissionData )
    {
        return 0;
    }

//#ifdef DEBUG_MISSION_PACKETS // After Items
    WriteDebug( "\nAfter Items:\n" );
    DebugPacket( _pMissionData, 0x100 );
    WriteDebug( 0 );
//#endif

    MishQL = *(PUU32*)( _pMissionData + 0xc );
    MishQL = EndianSwap32( MishQL );

    pPrevCol = _pData->pCol;

    // Display the single or team interface depending on the number of items
    if( NumItems < 2 )
    {
        _pData->pCol = _pData->pSingleCol;
    }
    else
    {
        _pData->pCol = _pData->pTeamCol;
		puSetAttribute(puGetObjectFromCollection(_pData->pCol, FOLD), PUA_FOLD_FOLDED, g_Settings.bAutoExpand ? FALSE : TRUE);
    }

    // Ask a complete layout recalculation if we changed of interface
    //if( !g_BuyingAgentCount )
    {
        //if( pPrevCol != _pData->pCol )
        //{
        puSetAttribute( _Object, PUA_CONTAINER_CONTENTS, puGetObjectFromCollection( _pData->pCol, ROOTOBJ ) );
        //puDoMethod( puGetObjectFromCollection( _pData->pCol, ROOTOBJ ), PUM_CONTROL_RECALC, 0, 0 );
        //puDoMethod( _Object, PUM_CONTROL_RECALC, 0, 0 );
        //}

        // Display cash
        sprintf( _pData->CashStr, "%u", Cash );
        puSetAttribute( puGetObjectFromCollection( _pData->pCol, CASH ), PUA_TEXT_STRING, (PUU32)_pData->CashStr );
        sprintf( _pData->XPStr, "%u", XP );
        puSetAttribute( puGetObjectFromCollection( _pData->pCol, MISHXP ), PUA_TEXT_STRING, (PUU32)_pData->XPStr );
    }

    // Get playfield (was + 0x98 pre 16.3)
    TempVal = *(PUU32*)( _pMissionData + 0xA8 );
    MishPF = TempVal = EndianSwap32( TempVal );
    MissionPF( TempVal, PFName );

    // Get coordinates (was +0xa4, +0xac pre 16.3)
    TempVal = *(PUU32*)( _pMissionData + 0xb4 );
    TempVal = EndianSwap32( TempVal );
    *(PUU32*)( &CoordX ) = TempVal;
    TempVal = *(PUU32*)( _pMissionData + 0xbc );
    TempVal = EndianSwap32( TempVal );
    *(PUU32*)( &CoordY ) = TempVal;

    sprintf( TempStr, "%s (%.1f, %.1f)", PFName, CoordX, CoordY );
    bLocFound = SetAndSearch( TempStr, puGetObjectFromCollection( _pData->pCol, LOCATION ), g_LocWatchList );

    // offset won't be correct for 16.3, but not currently used anyway
    //Time = *( PUU32* )( _pMissionData + 0x20 );
    //Time = EndianSwap32( Time );
    //sprintf(_pData->TimeStr, "%dh", Time/60);

    //if( !g_BuyingAgentCount )
    //{
    //    puSetAttribute( puGetObjectFromCollection( _pData->pCol, TIME ), PUA_TEXT_STRING, ( PUU32 )_pData->TimeStr );
    //}

    // Get Mission Type (was 0x18 pre 16.3)
    TempVal = *(PUU32*)( _pMissionData + 0x28 );
    TempVal = EndianSwap32( TempVal );
    bTypeFound = SetAndSearchType( TempVal, puGetObjectFromCollection( _pData->pCol, MISHTYPE ) );

    // Write Stuff to log:
    WriteLog( "mission\t%u\t%u\t%u\t%u\t%s\n", MishID, MishQL, XP, Cash, CharKey );
    WriteLog( "loc\t%u\t%.1f\t%.1f\t%s\n", MishPF, CoordX, CoordY, PFName );


    // Display items
    for( i = 0; i < NumItems; i++ )
    {
        bItemFound |= ShowItem( _pData, pItem++, i + ITEM1, i + ITEMVAL1 );
		TotalValue += _pData->Reward.Value * g_Settings.iBuyMod / 100;
    }
    //if( !g_BuyingAgentCount )
    {
        puSetAttribute( puGetObjectFromCollection( _pData->pCol, TOTALVAL ), PUA_TEXTENTRY_VALUE, TotalValue );
    }
    if( TotalValue > g_Settings.iMatchTotalVal )
    {
        puSetAttribute( puGetObjectFromCollection( _pData->pCol, TOTALVAL ), PUA_TEXTENTRY_HILIGHT, TRUE );
		if (g_Settings.bMatchTotal) bItemFound = TRUE;
    }
    else
    {
        puSetAttribute( puGetObjectFromCollection( _pData->pCol, TOTALVAL ), PUA_TEXTENTRY_HILIGHT, FALSE );
    }

    if( NumItems == 0 )//&& !g_BuyingAgentCount ) // Special Case.. For Missions with no Item Rewards
    {
        puSetAttribute( puGetObjectFromCollection( _pData->pCol, ITEMVAL1 ), PUA_TEXTENTRY_VALUE, 0 );
        puSetAttribute( puGetObjectFromCollection( _pData->pCol, ITEM1 ), PUA_TEXTENTRY_BUFFER, 0 );
        puSetAttribute( puGetObjectFromCollection( _pData->pSingleCol, IMAGE ), PUA_IMAGE_DATA, 0 );
    }

    // If we're in team mission mode, hide the remaining, unused item fields (if any)
    //if( !g_BuyingAgentCount && _pData->pCol == _pData->pTeamCol )
	if ( _pData->pCol == _pData->pTeamCol)
    {
        for( ; i < 6; i++ )
        {
            puSetAttribute( puGetObjectFromCollection( _pData->pCol, i + ITEMROW1 ), PUA_CONTROL_HIDDEN, TRUE );
        }

        // Hilight the fold control title if any item was found
        if( bItemFound )
        {
            puSetAttribute( puGetObjectFromCollection( _pData->pCol, FOLD ), PUA_FOLD_HILIGHT, TRUE );
        }
        else
        {
            puSetAttribute( puGetObjectFromCollection( _pData->pCol, FOLD ), PUA_FOLD_HILIGHT, FALSE );
        }
    }

	// 1. Create a lowercase copy of the description for searching
	PUU8* pLowerDesc = malloc(DescLength + 1);
	if (pLowerDesc)
	{
		for (i = 0; i < DescLength; i++) {
			PUU8 c = pDesc[i];
			pLowerDesc[i] = (c >= 'A' && c <= 'Z') ? (c + 0x20) : c;
		}
		pLowerDesc[DescLength] = 0;

		// 2. Search for watchlist items in the description
		PUU32 Record = puDoMethod(g_ItemWatchList, PUM_TABLE_GETFIRSTRECORD, 0, 0);
		PUU32 bFindItemInDesc = FALSE;

		while (Record)
		{
			PUU8* pWatchItem = (PUU8*)puDoMethod(g_ItemWatchList, PUM_TABLE_GETFIELDVAL, Record, 0);
			if (pWatchItem && *pWatchItem)
			{
				// Note: SetAndSearch already lowercases the search term internally, 
				// so we just need to compare it against our lowercased description.
				if (strstr((char*)pLowerDesc, (char*)pWatchItem) != NULL)
				{
					bFindItemInDesc = TRUE;
					// Update UI
					puSetAttribute(puGetObjectFromCollection(_pData->pCol, FINDITEM), PUA_TEXTENTRY_BUFFER, (PUU32)pWatchItem);
					puSetAttribute(puGetObjectFromCollection(_pData->pCol, FINDITEM), PUA_TEXTENTRY_HILIGHT, TRUE);
					break;
				}
			}
			Record = puDoMethod(g_ItemWatchList, PUM_TABLE_GETNEXTRECORD, Record, 0);
		}

		// 3. The "Else" logic: Clear the field if nothing was found
		if (!bFindItemInDesc)
		{
			puSetAttribute(puGetObjectFromCollection(_pData->pCol, FINDITEM), PUA_TEXTENTRY_BUFFER, 0);
			puSetAttribute(puGetObjectFromCollection(_pData->pCol, FINDITEM), PUA_TEXTENTRY_HILIGHT, FALSE);
		}

		bItemFound |= bFindItemInDesc;
		free(pLowerDesc);
	}


    bAlert = bAlertItem || bAlertLoc || bAlertType;

    if( bAlertItem ) bAlert = bAlert && bItemFound;
    if( bAlertLoc ) bAlert = bAlert && bLocFound;
    if( bAlertType ) bAlert = bAlert && bTypeFound;

    if( bAlert )
    {
        if( g_FoundMish == 255 ) g_FoundMish = g_MishNumber;
        if( g_BuyingAgentCount )
        {
            // Found mission, stop buying agent
            g_BuyingAgentCount = 0;
        }
        else
        {
            if( g_Settings.bAlertBox && !g_bFullscreen )
            {
                // Uniconify window
                puSetAttribute( g_MainWin, PUA_WINDOW_ICONIFIED, FALSE );
                //puSetAttribute( puGetObjectFromCollection( g_pCol, CS_WATCH_MSGBOX ), PUA_WINDOW_OPENED, TRUE );
				puSetAttribute(puGetObjectFromCollection(g_pCol, CS_STATUS_TEXT), PUA_TEXT_STRING, (PUU32)"Mission(s) matches search criteria!");
            }
        }
    }

    return (PUU32)_pMissionData;
}


PUU32 ShowItem( MissionClassData* _pData, Item* _pItem, PUU32 _ObjId, PUU32 _ValID )
{
    PUU32 ItemKey1, ItemKey2, QL;
    PUU32 bItemFound = FALSE;
    char TempStr[ 256 ];

    ItemKey1 = _pItem->Key1;
    ItemKey1 = EndianSwap32( ItemKey1 );
    ItemKey2 = _pItem->Key2;
    ItemKey2 = EndianSwap32( ItemKey2 );
    QL = _pItem->QL;
    QL = EndianSwap32( QL );

    puSetAttribute( puGetObjectFromCollection( _pData->pCol, _ObjId ), PUA_CONTROL_HIDDEN, FALSE );

    if( ItemKey1 == 0x6af2 && ItemKey2 == 0x6af3 )//&& !g_BuyingAgentCount )
    {
        // Mission with no item reward, clear item name and icon
        puSetAttribute( puGetObjectFromCollection( _pData->pCol, _ObjId ), PUA_TEXTENTRY_BUFFER, 0 );
        puSetAttribute( puGetObjectFromCollection( _pData->pSingleCol, IMAGE ), PUA_IMAGE_DATA, 0 );
    }
    else
    {
        GetMissionItem( &_pData->Reward, ItemKey1, ItemKey2, QL );
        WriteLog( "reward\t%u\t%u\t%u\t%s\n", ItemKey1, ItemKey2, QL, _pData->Reward.pName );

        // Display item name and ql
        sprintf( TempStr, "QL%u %s", QL, _pData->Reward.pName );
        bItemFound = SetAndSearch( TempStr, puGetObjectFromCollection(
            _pData->pCol, _ObjId ), g_ItemWatchList );

		puSetAttribute(puGetObjectFromCollection(_pData->pCol, _ValID), PUA_TEXTENTRY_VALUE, g_Settings.iBuyMod / 100);
		if (_pData->Reward.Value * g_Settings.iBuyMod / 100
        > g_Settings.iMatchSingleVal )
        {
            puSetAttribute( puGetObjectFromCollection( _pData->pCol, _ValID ), PUA_TEXTENTRY_HILIGHT, TRUE );
            if( g_Settings.bMatchSingle ) bItemFound = TRUE;
        }
        else
        {
            puSetAttribute( puGetObjectFromCollection( _pData->pCol, _ValID ), PUA_TEXTENTRY_HILIGHT, FALSE );
        }
        // Don't bother with item icon when running
        // in buying agent mode
        if( _pData->pCol == _pData->pSingleCol )
        {
            // Free previous image buffer, if any
            if( _pData->pImageData )
            {
                free( _pData->pImageData );
            }

            // Display item icon
            _pData->pImageData = GetAOIconData( _pData->Reward.IconKey );
            puSetAttribute( puGetObjectFromCollection( _pData->pSingleCol, IMAGE ),
                            PUA_IMAGE_DATA, (PUU32)_pData->pImageData );
        }
    }

    return bItemFound;
}


PUU32 SetAndSearchType( PUU32 TempVal, PULID _TextEntry )
{
    PUU8 match = 0;
    PUU8 TempStr[ 50 ];
    switch( TempVal )
    {
    case 0x2c4e:
        sprintf( TempStr, "Repair" );
        if( puGetAttribute( puGetObjectFromCollection( g_pCol, CS_TYPEREPAIR_CB ),
            PUA_CHECKBOX_CHECKED ) ) match = 1;
        break;

//    case 0x2c41: 
    case 0x26add:	//PRK new id
		sprintf(TempStr, "Return Item");
        if( puGetAttribute( puGetObjectFromCollection( g_pCol, CS_TYPERETURN_CB ),
            PUA_CHECKBOX_CHECKED ) ) match = 1;
        break;

    case 0x2c47:
        sprintf( TempStr, "Find Person" );
        if( puGetAttribute( puGetObjectFromCollection( g_pCol, CS_TYPEFINDP_CB ),
            PUA_CHECKBOX_CHECKED ) ) match = 1;
        break;

    case 0x2c49:
        sprintf( TempStr, "Find Item" );
        if( puGetAttribute( puGetObjectFromCollection( g_pCol, CS_TYPEFINDI_CB ),
            PUA_CHECKBOX_CHECKED ) ) match = 1;
        break;

    case 0x2c42:
        sprintf( TempStr, "Kill Person" );
        if( puGetAttribute( puGetObjectFromCollection( g_pCol, CS_TYPEASS_CB ),
            PUA_CHECKBOX_CHECKED ) ) match = 1;
        break;

    default: sprintf( TempStr, "Unknown: 0x%08X - Please report", TempVal );
        break;
    }
    // Don't update interface when buying agent is running
   // if( !g_BuyingAgentCount )
    {
        puSetAttribute( _TextEntry, PUA_TEXTENTRY_BUFFER, (PUU32)&TempStr );
        puSetAttribute( _TextEntry, PUA_TEXTENTRY_HILIGHT, match &&
                        puGetAttribute( puGetObjectFromCollection( g_pCol, CS_HIGHLIGHTTYPE_CB ),
                        PUA_CHECKBOX_CHECKED ) );
    }
    return match;
}


/* Set string to a textentry control, search for it in a list,
   and hilight the textentry if the string was found.
*/
PUU32 SetAndSearch( PUU8* _pSrcString, PULID _TextEntry, PULID _List )
{
    PUU32 Record;
    PUU8* pString;
    PUU8 TmpItemName[ 256 ];
    PUU8 c;
    PUU8* pChar;

    // Don't update interface when buying agent is running
    //if( !g_BuyingAgentCount )
    {
        puSetAttribute( _TextEntry, PUA_TEXTENTRY_BUFFER, (PUU32)_pSrcString );
    }

    // Convert item name to lower case
    pChar = TmpItemName;
    do
    {
        c = *_pSrcString++;
        if( c >= 'A' && c <= 'Z' )
        {
            *pChar++ = c + 0x20;
        }
        else
        {
            *pChar++ = c;
        }
    }
    while( c );

    Record = puDoMethod( _List, PUM_TABLE_GETFIRSTRECORD, 0, 0 );
    while( Record )
    {
        if( ( pString = (PUU8*)puDoMethod( _List, PUM_TABLE_GETFIELDVAL, Record, 0 ) ) && *pString )
        {
            if( _List == g_ItemWatchList ) // item
            {
                if( ItemMatch( TmpItemName, pString ) )
                {
                    //if( !g_BuyingAgentCount )
                    {
                        puSetAttribute( _TextEntry, PUA_TEXTENTRY_HILIGHT, puGetAttribute(
                            puGetObjectFromCollection( g_pCol, CS_HIGHLIGHTITEM_CB ),
                            PUA_CHECKBOX_CHECKED ) );
                    }
                    return TRUE;
                }
            }
            else // location
            {
                if( LocationMatch( TmpItemName, pString ) )
                {
                    //if( !g_BuyingAgentCount )
                    {
                        puSetAttribute( _TextEntry, PUA_TEXTENTRY_HILIGHT, puGetAttribute(
                            puGetObjectFromCollection( g_pCol, CS_HIGHLIGHTLOC_CB ),
                            PUA_CHECKBOX_CHECKED ) );
                    }
                    return TRUE;
                }
            }

        }
        Record = puDoMethod( _List, PUM_TABLE_GETNEXTRECORD, Record, 0 );
    }

    //if( !g_BuyingAgentCount )
    {
        puSetAttribute( _TextEntry, PUA_TEXTENTRY_HILIGHT, FALSE );
    }

    return FALSE;
}

/*******************************
Item Search, to allow web-search-like constructors "<text>", -<text> and
word-match
Examples:
Searching for 'decus -gloves' will match all decus items except gloves;
Searching for 'decus armor' will match on 'decus body armor', and 'decus
armor boots'
Searching for '"decus armor"' will match on 'decus armor boots' but not on
'decus body armor'
Searching for '"primus decus" -gloves -boots -body' will match on all primus
decus armor except for gloves, boots and body
********************************/
PUU32 ItemMatch( PUU8* ItemName, PUU8* ItemSearch )
{
    PUU8 TmpString[ 256 ];
    PUU8* pChar;
    PUU8 c, OpenQuoteFlag, ExcludeFlag, HadValidString = FALSE;

    do
    {
        pChar = TmpString;
        OpenQuoteFlag = FALSE;
        ExcludeFlag = FALSE;

        do
        {
            c = *ItemSearch++;

            if( c >= 'A' && c <= 'Z' )
            {
                *pChar++ = c + 0x20;
            }
            else if( c == '"' )
            {
                if( OpenQuoteFlag )
                {
                    *pChar++ = 0;
                    OpenQuoteFlag = FALSE;
                }
                else
                {
                    OpenQuoteFlag = TRUE;
                }
            }
            else if( c == '-' && pChar == TmpString )
            {
                ExcludeFlag = TRUE;
            }
            else if( c != ' ' || OpenQuoteFlag )
            {
                *pChar++ = c;
            }
            else
            {
                *pChar++ = 0;
            }

        }
        while( c && !( c == ' ' && !OpenQuoteFlag ) );

        if( strlen( TmpString ) )
        {
            HadValidString = TRUE;

            if( ExcludeFlag )
            {
                if( strstr( ItemName, TmpString ) )
                {
                    return FALSE;
                }
            }
            else
            {
                if( !strstr( ItemName, TmpString ) )
                {
                    return FALSE;
                }
            }
        }
    }
    while( c );

    return HadValidString;
}


/*******************************
Location Search, to allow as above, plus location range search
Examples:
Searching for 'athen -shire' will match on 'west athens' and 'old athens'
Searching for 'athen (100-200,500-600)' will match on any athem mission with
coords x from 100 to 200, y from 500 to 600.
Searching for 'athen (100.2,200.3)' will match on any athen mission with
coords x and y exacly 100.2 and 200.3 respectively
Searching for 'athen (0-500,3000-999999)' will match on any athen mission
with coords x <=500, y>=3000 (but less than 999999)
********************************/
PUU32 LocationMatch( PUU8* LocationName, PUU8* LocationSearch )
{
    PUU8 Name[ 256 ] = { 0 }, Search[ 256 ] = { 0 };
    PUU8 CoordX[ 20 ] = { 0 }, CoordY[ 20 ] = { 0 };
    PUU8 SearchCoordXFrom[ 20 ] = { 0 }, SearchCoordXTo[ 20 ] = { 0 };
    PUU8 SearchCoordYFrom[ 20 ] = { 0 }, SearchCoordYTo[ 20 ] = { 0 };
    PUU8 *pChar;
    PUU8 c, OpenBracketFlag = FALSE, YCoordFlag = FALSE;
    double x, y, xfrom, xto, yfrom, yto;

    // pull Name, CoordX, CoordY
    pChar = Name;
    do
    {
        c = *LocationName++;

        if( c == '(' )
        {
            *pChar = 0;
            OpenBracketFlag = TRUE;
            pChar = CoordX;
        }
        else if( c == ',' && OpenBracketFlag )
        {
            *pChar = 0;
            pChar = CoordY;
        }
        else
        {
            *pChar++ = c;
        }
    }
    while( c );

    // pull Search, SearchCoordXFrom, SearchCoordXTo, SearchCoordYFrom, SearchCoordYTo
    OpenBracketFlag = FALSE;
    pChar = Search;
    do
    {
        c = *LocationSearch++;

        if( c == '(' )
        {
            *pChar = 0;
            OpenBracketFlag = TRUE;
            pChar = SearchCoordXFrom;
        }
        else if( c == ',' && OpenBracketFlag )
        {
            *pChar = 0;
            YCoordFlag = TRUE;
            pChar = SearchCoordYFrom;
        }
        else if( c == '-' && OpenBracketFlag )
        {
            if( YCoordFlag )
            {
                *pChar = 0;
                pChar = SearchCoordYTo;
            }
            else
            {
                *pChar = 0;
                pChar = SearchCoordXTo;
            }
        }
        else
        {
            *pChar++ = c;
        }
    }
    while( c );

    // compare LocationName to LocationSearch
    if( ItemMatch( Name, Search ) )
    {

        // if matched, compare coordtinates
        x = atof( CoordX );
        y = atof( CoordY );
        xfrom = atof( SearchCoordXFrom );
        xto = atof( SearchCoordXTo );
        yfrom = atof( SearchCoordYFrom );
        yto = atof( SearchCoordYTo );

        if( x > 0 && y > 0 && xfrom > 0 && yfrom > 0 )
        { // carry on only if we have mission location
            if( ( x >= xfrom && ( x <= xto || !xto ) ) && ( y >= yfrom && ( y <= yto || !yto ) ) )
            {
                return TRUE; // loc name matched, coords matched
            }
            else
            {
                return FALSE; // loc name matched; coonds wrong
            }
        }
        else
        {
            return TRUE; // loc name matched; won't compare coords
        }
    }

    return FALSE; // loc name didn't match
}




//typedef struct findname_struc
//{
//    char *strStart;
//    char *strEnd;
//} udtFindName_struc;
//
//#define CNT_FINDNAME 25
//static udtFindName_struc a_udtFindName[CNT_FINDNAME] = {
//    "The enemy is in the process of creating a new prototype ", ". It is of utmost importance",
//    "The enemy is currently making a new prototype ", ". It is of utmost importance",
//    "We have reason to believe finding the ", " in ",
//    "In this case it is the ", " that has gone missing.",
//    "we have at last found a copy of the ", " in ",
//    "According to our sources, the ", " found in ",
//    "Last night, the ", " was stolen from a production facility",
//    "Last night, one ", " was stolen from a production facility",
//    "One of our ", " have been stolen from our ",
//    "One ", " has been stolen from our ",
//    "A hacker wiped the ", " from our database",
//    "I am interested in obtaining a certain ", ". My contacts have",
//    "I am interested in obtaining one ", ". My contacts have",
//    "have developed a prototype ", ".  We would very,",
//    "If we could steal the ", " from the enemy, we would",
//    "you can find the entrance to the place where the ", " has been hidden.",
//    "you can find the entrance to where the ", " has been hidden.",
//    "Would you please find the ", " in ",
//    "you might be able to find the ", ". Please bring it back to us",
//    "Oh yeah, the ", " is set to blow up in",
//    "who or where the traitor is, before you collect the ", " from ",
//    "who or where he is, before you collect the ", " from ",
//    "you might be able to find one ", ". Bring it back to us",
//    "we intercepted a message that a prototype ", " will be moved from",
//    "It is the ", ", please retrieve it "
//};
//
//
///* Return mission item to be found */
//PUU32 MissionFind( PUU8* _pMissionDesc, PUU32 _DescLen, PUU8* _pItemName )
//{
//    long lLoop;
//    long lPosStart;
//    long lRem;
//    long lLength;
//    PUU32 bFoundItem;
//    char *strStart;
//
//    /* Initialise */
//    bFoundItem = FALSE;
//
//    /* Search each of the mission 'find item' text matches */
//    for( lLoop = 0; lLoop < CNT_FINDNAME; lLoop++ )
//    {
//        lPosStart = FindStr( _pMissionDesc, _DescLen, (PUU8
//            *)a_udtFindName[ lLoop ].strStart, strlen( a_udtFindName[ lLoop ].strStart ) );
//        if( lPosStart >= 0 )
//        {
//            /* Set ptr to start of find item name */
//            strStart = _pMissionDesc + lPosStart +
//                strlen( a_udtFindName[ lLoop ].strStart );
//
//            /* Search the remainder for trailing text */
//            lRem = _DescLen - ( lPosStart + strlen( a_udtFindName[ lLoop ].strStart ) );
//            lLength = FindStr( (PUU8 *)strStart, lRem, (PUU8
//                *)a_udtFindName[ lLoop ].strEnd, strlen( a_udtFindName[ lLoop ].strEnd ) );
//            if( lLength >= 0 )
//            {
//                bFoundItem = TRUE;
//                break;
//            }
//        }
//    }
//
//    if( bFoundItem )
//    {
//        memcpy( _pItemName, strStart, lLength );
//        _pItemName[ lLength ] = 0;
//        return TRUE;
//    }
//    else
//    {
//        return FALSE;
//    }
//}
