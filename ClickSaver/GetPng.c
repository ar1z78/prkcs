#include "getpng.h"
#include "rdb.h"
#include "mission.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// NEW: Local fallback definition for EndianSwap32 bit shifting
#ifndef EndianSwap32
#define EndianSwap32(x) ((((x) & 0x000000FF) << 24) | \
                         (((x) & 0x0000FF00) << 8)  | \
                         (((x) & 0x00FF0000) >> 8)  | \
                         (((x) & 0xFF000000) >> 24))
#endif
// Instantiate the signature array here at the top of getpng.c
PUU8 a_xPNGSig[ LEN_PNGSIG ] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };


/* Get item icon from PRK Database */
PUU8 *GetAOIconData( unsigned long lIconNo )
{
    unsigned long lDataLen;
    unsigned long lChunkLen;
    unsigned long lPNGLen;
    unsigned long lLoop;
    unsigned long lLoop2;
    unsigned long lBytesPerRow;
    unsigned long lPNGDataLen;
    unsigned long lPNGImageLen;
    unsigned long lPNGRowOffset;
    PUU8 xFilter;
    PUU8 *a_xData;
    PUU8 *a_xPNG;
    PUU8 *a_xPNGChunk;
    PUU8 *a_xPNGData;
    PUU8 *a_xPNGImage = NULL;
    PUU8 *a_xPNGRow, *a_xPNGRowPrev = NULL;
    udtPNGpixel_struc *udtCLinCByt, *udtCLinPByt, *udtPLinCByt, *udtPLinPByt;
    udtPNGpixel_struc *udtPNGpixel;
    char strChunkID[ 5 ];
    udtPNGihdr_struc *udtPNGihdr;
    udtPNGsbit_struc *udtPNGsbit;
    //FILE *fpDebug;
    PUU8* pImageData = NULL;
    PUU8* pTmp;

    /* Initialise */
    a_xData = NULL;
    a_xPNGImage = NULL;
    a_xPNGRowPrev = NULL;

    /* Read data for this item */
    if( !( a_xData = GetDataChunk( AODB_TYP_ICON, lIconNo, &lDataLen ) ) )
    {
        goto GetAOIconData_Exit_Fail;
    }

    /* Check it contains a valid PNG */
    a_xPNG = a_xData; // + 0x18;
    lPNGLen = lDataLen; // - 0x18;
    if( memcmp( a_xPNG, a_xPNGSig, LEN_PNGSIG ) != 0 )
    {
        goto GetAOIconData_Exit_Fail;
    }
    a_xPNGChunk = a_xPNG + 0x8;     // Point to first chunk

    /* Write PNG icon to file */
    /*
    if( lDebug & DBG_ICN )
    {
        sprintf( strDebugFile, "%sDebug_IconsPNG.DAT", strAOMDPath );
        fpDebug = fopen( strDebugFile, "a+b" );
        fwrite( a_xPNG, sizeof( PUU8 ), lPNGLen, fpDebug );
        fwrite( "****************", sizeof( char ), 0x10 - ( lPNGLen % 0x10 ),
                fpDebug );
        fwrite( "****************", sizeof( char ), 0x10, fpDebug );
        fclose( fpDebug );
    }
    */

    /* Check IHDR chunk - Start of PNG, contains icon properties */
    lChunkLen = EndianSwap32( *(unsigned long *)( a_xPNGChunk ) );
    lChunkLen += 0xC;
    memcpy( strChunkID, ( a_xPNGChunk + 0x4 ), 4 );
    strChunkID[ 4 ] = 0;
    if( _stricmp( strChunkID, "IHDR" ) != 0 )
    {
        goto GetAOIconData_Exit_Fail;
    }
    udtPNGihdr = (udtPNGihdr_struc *)( a_xPNGChunk + 0x8 );
    udtPNGihdr->lWidth = EndianSwap32( udtPNGihdr->lWidth );  // Fix endian
    udtPNGihdr->lHeight = EndianSwap32( udtPNGihdr->lHeight );    // Fix endian
    a_xPNGChunk += lChunkLen;   // Bump to next chunk

    /* Ensure PNG properties are what we expect from AO */
    if( ( udtPNGihdr->lWidth != 48 ) || ( udtPNGihdr->lHeight != 48 ) )
    {
        goto GetAOIconData_Exit_Fail;   // not a 48x48 image
    }
    if( ( udtPNGihdr->xBitDepth != 8 ) || ( udtPNGihdr->xColorType != 2 ) )
    {
        goto GetAOIconData_Exit_Fail;   // not 24bit RGB
    }
    if( ( udtPNGihdr->xCompressMethod != 0 ) || ( udtPNGihdr->xFilterMethod != 0 ) )
    {
        goto GetAOIconData_Exit_Fail;   // non-standard compression or filter
    }
    if( udtPNGihdr->xInterlaceMethod != 0 )
    {
        goto GetAOIconData_Exit_Fail;   // must not be interlaced
    }

    /* Check SBIT chunk - Significant bits */
    lChunkLen = EndianSwap32( *(unsigned long *)( a_xPNGChunk ) );
    lChunkLen += 0xC;
    memcpy( strChunkID, ( a_xPNGChunk + 0x4 ), 4 );
    strChunkID[ 4 ] = 0;
    if( _stricmp( strChunkID, "SBIT" ) != 0 )
    {
        goto GetAOIconData_Exit_Fail;
    }
    udtPNGsbit = (udtPNGsbit_struc *)( a_xPNGChunk + 0x8 );
    a_xPNGChunk += lChunkLen;   // Bump to next chunk

    /* Check IDAT chunk - Contains the icon data*/
    lPNGDataLen = EndianSwap32( *(unsigned long *)( a_xPNGChunk ) );
    lChunkLen = lPNGDataLen + 0xC;
    memcpy( strChunkID, ( a_xPNGChunk + 0x4 ), 4 );
    strChunkID[ 4 ] = 0;
    if( _stricmp( strChunkID, "IDAT" ) != 0 )
    {
        goto GetAOIconData_Exit_Fail;
    }
    a_xPNGData = a_xPNGChunk + 0x8;
    a_xPNGChunk += lChunkLen;   // Bump to next chunk

    /* Allocate bitmap */
    lBytesPerRow = ( ( ( udtPNGihdr->lWidth * 24 ) + 31 ) / 32 ) * 4;
    if( !( pImageData = malloc( udtPNGihdr->lHeight * lBytesPerRow ) ) )
    {
        goto GetAOIconData_Exit_Fail;
    }


    /* Decompress the PNG image data using ZLib */
    lPNGImageLen = udtPNGihdr->lHeight * ( lBytesPerRow + 1 );
    a_xPNGImage = (PUU8 *)malloc( lPNGImageLen );
    if( uncompress( a_xPNGImage, &lPNGImageLen, a_xPNGData, lPNGDataLen ) != Z_OK )
    {
        goto GetAOIconData_Exit_Fail;
    }

    /* Allocate previous row buffer and init to zero */
    a_xPNGRowPrev = (PUU8 *)malloc( lBytesPerRow );
    memset( a_xPNGRowPrev, 0, lBytesPerRow );

    /* Filter each row and copy to bitmap */
    for( lLoop = 0; lLoop < udtPNGihdr->lHeight; lLoop++ )
    {
        lPNGRowOffset = lLoop * ( lBytesPerRow + 1 );
        xFilter = a_xPNGImage[ lPNGRowOffset ];
        a_xPNGRow = a_xPNGImage + lPNGRowOffset + 1;
        switch( xFilter )
        {
            /* Filter 0 - None */
        case 0:
            break;

            /* Filter 1 - Sub */
        case 1:
            udtCLinCByt = (udtPNGpixel_struc *)a_xPNGRow + 1;
            udtCLinPByt = (udtPNGpixel_struc *)a_xPNGRow;
            for( lLoop2 = 1; lLoop2 < ( lBytesPerRow / 3 ); lLoop2++ )
            {
                udtCLinCByt->xRed = (PUU8)( ( (int)( udtCLinCByt->xRed ) +
                    (int)( udtCLinPByt->xRed ) ) & 0xFF );
                udtCLinCByt->xGreen = (PUU8)( ( (int)( udtCLinCByt->xGreen ) +
                    (int)( udtCLinPByt->xGreen ) ) & 0xFF );
                udtCLinCByt->xBlue = (PUU8)( ( (int)( udtCLinCByt->xBlue ) +
                    (int)( udtCLinPByt->xBlue ) ) & 0xFF );
                udtCLinCByt++;
                udtCLinPByt++;
            }
            break;

            /* Filter 2 - Up */
        case 2:
            udtCLinCByt = (udtPNGpixel_struc *)a_xPNGRow;
            udtPLinCByt = (udtPNGpixel_struc *)a_xPNGRowPrev;
            for( lLoop2 = 0; lLoop2 < ( lBytesPerRow / 3 ); lLoop2++ )
            {
                udtCLinCByt->xRed = (PUU8)( ( (int)( udtCLinCByt->xRed ) +
                    (int)( udtPLinCByt->xRed ) ) & 0xFF );
                udtCLinCByt->xGreen = (PUU8)( ( (int)( udtCLinCByt->xGreen ) +
                    (int)( udtPLinCByt->xGreen ) ) & 0xFF );
                udtCLinCByt->xBlue = (PUU8)( ( (int)( udtCLinCByt->xBlue ) +
                    (int)( udtPLinCByt->xBlue ) ) & 0xFF );
                udtCLinCByt++;
                udtPLinCByt++;
            }
            break;

            /* Filter 3 - Average */
        case 3:
            udtCLinCByt = (udtPNGpixel_struc *)a_xPNGRow;
            udtPLinCByt = (udtPNGpixel_struc *)a_xPNGRowPrev;
            udtCLinPByt = (udtPNGpixel_struc *)a_xPNGRow;
            udtCLinCByt->xRed = (PUU8)( ( (int)( udtCLinCByt->xRed ) +
                ( (int)( udtPLinCByt->xRed ) >> 1 ) ) & 0xFF );
            udtCLinCByt->xGreen = (PUU8)( ( (int)( udtCLinCByt->xGreen ) +
                ( (int)( udtPLinCByt->xGreen ) >> 1 ) ) & 0xFF );
            udtCLinCByt->xBlue = (PUU8)( ( (int)( udtCLinCByt->xBlue ) +
                ( (int)( udtPLinCByt->xBlue ) >> 1 ) ) & 0xFF );
            udtCLinCByt++;
            udtPLinCByt++;
            for( lLoop2 = 0; lLoop2 < ( lBytesPerRow / 3 ) - 1; lLoop2++ )
            {
                udtCLinCByt->xRed = (PUU8)( ( (int)( udtCLinCByt->xRed ) +
                    ( (int)( udtPLinCByt->xRed + udtCLinPByt->xRed ) >> 1 ) ) & 0xFF );
                udtCLinCByt->xGreen = (PUU8)( ( (int)( udtCLinCByt->xGreen ) +
                    ( (int)( udtPLinCByt->xGreen + udtCLinPByt->xGreen ) >> 1 ) ) & 0xFF );
                udtCLinCByt->xBlue = (PUU8)( ( (int)( udtCLinCByt->xBlue ) +
                    ( (int)( udtPLinCByt->xBlue + udtCLinPByt->xBlue ) >> 1 ) ) & 0xFF );
                udtCLinCByt++;
                udtPLinCByt++;
                udtCLinPByt++;
            }
            break;

            /* Filter 4 - Paeth */
        case 4:
            udtCLinCByt = (udtPNGpixel_struc *)a_xPNGRow;
            udtPLinCByt = (udtPNGpixel_struc *)a_xPNGRowPrev;
            udtCLinPByt = (udtPNGpixel_struc *)a_xPNGRow;
            udtPLinPByt = (udtPNGpixel_struc *)a_xPNGRowPrev;
            udtCLinCByt->xRed = (PUU8)( ( (int)( udtCLinCByt->xRed ) +
                (int)( udtPLinCByt->xRed ) ) & 0xFF );
            udtCLinCByt->xGreen = (PUU8)( ( (int)( udtCLinCByt->xGreen ) +
                (int)( udtPLinCByt->xGreen ) ) & 0xFF );
            udtCLinCByt->xBlue = (PUU8)( ( (int)( udtCLinCByt->xBlue ) +
                (int)( udtPLinCByt->xBlue ) ) & 0xFF );
            udtCLinCByt++;
            udtPLinCByt++;
            for( lLoop2 = 0; lLoop2 < ( lBytesPerRow / 3 ) - 1; lLoop2++ )
            {
                int lCLinPByt_R, lPLinCByt_R, lPLinPByt_R, lPaethA_R, lPaethB_R,
                    lPaethC_R, lPaeth_R;
                int lCLinPByt_G, lPLinCByt_G, lPLinPByt_G, lPaethA_G, lPaethB_G,
                    lPaethC_G, lPaeth_G;
                int lCLinPByt_B, lPLinCByt_B, lPLinPByt_B, lPaethA_B, lPaethB_B,
                    lPaethC_B, lPaeth_B;

                lCLinPByt_R = udtCLinPByt->xRed;
                lPLinCByt_R = udtPLinCByt->xRed;
                lPLinPByt_R = udtPLinPByt->xRed;
                lPaeth_R = lPLinCByt_R - lPLinPByt_R;
                lPaethC_R = lCLinPByt_R - lPLinPByt_R;
                lPaethA_R = lPaeth_R < 0 ? -lPaeth_R : lPaeth_R;
                lPaethB_R = lPaethC_R < 0 ? -lPaethC_R : lPaethC_R;
                lPaethC_R = ( lPaeth_R + lPaethC_R ) < 0 ? -( lPaeth_R + lPaethC_R ) :
                    lPaeth_R + lPaethC_R;
                lPaeth_R = ( lPaethA_R <= lPaethB_R && lPaethA_R <= lPaethC_R ) ?
                lCLinPByt_R : ( lPaethB_R <= lPaethC_R ) ? lPLinCByt_R : lPLinPByt_R;
                udtCLinCByt->xRed = (PUU8)( ( (int)( udtCLinCByt->xRed ) + lPaeth_R ) &
                                            0xFF );

                lCLinPByt_G = udtCLinPByt->xGreen;
                lPLinCByt_G = udtPLinCByt->xGreen;
                lPLinPByt_G = udtPLinPByt->xGreen;
                lPaeth_G = lPLinCByt_G - lPLinPByt_G;
                lPaethC_G = lCLinPByt_G - lPLinPByt_G;
                lPaethA_G = lPaeth_G < 0 ? -lPaeth_G : lPaeth_G;
                lPaethB_G = lPaethC_G < 0 ? -lPaethC_G : lPaethC_G;
                lPaethC_G = ( lPaeth_G + lPaethC_G ) < 0 ? -( lPaeth_G + lPaethC_G ) :
                    lPaeth_G + lPaethC_G;
                lPaeth_G = ( lPaethA_G <= lPaethB_G && lPaethA_G <= lPaethC_G ) ?
                lCLinPByt_G : ( lPaethB_G <= lPaethC_G ) ? lPLinCByt_G : lPLinPByt_G;
                udtCLinCByt->xGreen = (PUU8)( ( (int)( udtCLinCByt->xGreen ) + lPaeth_G ) &
                                              0xFF );

                lCLinPByt_B = udtCLinPByt->xBlue;
                lPLinCByt_B = udtPLinCByt->xBlue;
                lPLinPByt_B = udtPLinPByt->xBlue;
                lPaeth_B = lPLinCByt_B - lPLinPByt_B;
                lPaethC_B = lCLinPByt_B - lPLinPByt_B;
                lPaethA_B = lPaeth_B < 0 ? -lPaeth_B : lPaeth_B;
                lPaethB_B = lPaethC_B < 0 ? -lPaethC_B : lPaethC_B;
                lPaethC_B = ( lPaeth_B + lPaethC_B ) < 0 ? -( lPaeth_B + lPaethC_B ) :
                    lPaeth_B + lPaethC_B;
                lPaeth_B = ( lPaethA_B <= lPaethB_B && lPaethA_B <= lPaethC_B ) ?
                lCLinPByt_B : ( lPaethB_B <= lPaethC_B ) ? lPLinCByt_B : lPLinPByt_B;
                udtCLinCByt->xBlue = (PUU8)( ( (int)( udtCLinCByt->xBlue ) + lPaeth_B ) &
                                             0xFF );

                udtCLinCByt++;
                udtCLinPByt++;
                udtPLinCByt++;
                udtPLinPByt++;
            }
            break;

            /* Unknown filter value */
        default:
            goto GetAOIconData_Exit_Fail;
        }

        /* Copy processed row to bitmap (have to do this pixel by pixel because
PNG is RGB but DIB is BGR) */
        udtPNGpixel = (udtPNGpixel_struc *)a_xPNGRow;
        pTmp = pImageData + lBytesPerRow * lLoop;
        //      rgbDIBpixel = (RGBTRIPLE *)((PUU8 *)(bmiDIB->bmiColors) + (((udtPNGihdr->lHeight - 1) - lLoop) * lBytesPerRow));
        for( lLoop2 = 0; lLoop2 < ( lBytesPerRow / 3 ); lLoop2++ )
        {
            // PUL doesn't handle color key on images yet and I don't have MSDNs at hand
            // to implement it, so in the meantime... :)
            if( udtPNGpixel->xGreen == 255 && !udtPNGpixel->xRed && !udtPNGpixel->xBlue )
            {
                *pTmp++ = 100;
                *pTmp++ = 100;
                *pTmp++ = 100;
            }
            else
            {
                *pTmp++ = udtPNGpixel->xBlue;
                *pTmp++ = udtPNGpixel->xGreen;
                *pTmp++ = udtPNGpixel->xRed;
            }
            udtPNGpixel++;
        }

        /* Copy processed row to previous row buffer */
        memcpy( a_xPNGRowPrev, a_xPNGRow, lBytesPerRow );
    }

    /* Release previous row buffer */
    free( a_xPNGRowPrev );
    a_xPNGRowPrev = NULL;

    /* Check IEND chunk - This marks the end of PNG */
    memcpy( strChunkID, ( a_xPNGChunk + 0x4 ), 4 );
    strChunkID[ 4 ] = 0;
    if( _stricmp( strChunkID, "IEND" ) != 0 )
    {
        goto GetAOIconData_Exit_Fail;
    }

    /* Release the PNG image and data chunk */
    free( a_xPNGImage );
    a_xPNGImage = NULL;
    free( a_xData );

    /* Write icon bitmap to file */
    /*
    if( lDebug & DBG_ICN )
    {
        sprintf( strDebugFile, "%sDebug_IconsBMP.DAT", strAOMDPath );
        fpDebug = fopen( strDebugFile, "a+b" );
        fwrite( &bmiBMPhdr, sizeof( BITMAPFILEHEADER ), 1, fpDebug );
        fwrite( bmiDIB, sizeof( PUU8 ), lDIBsize, fpDebug );
        fwrite( "****************", sizeof( char ), 0x10 - ( ( sizeof( BITMAPFILEHEADER )
            + lDIBsize ) % 0x10 ), fpDebug );
        fwrite( "****************", sizeof( char ), 0x10, fpDebug );
        fclose( fpDebug );
    }
    */

    /* Success - return the bitmap */
    return pImageData;

GetAOIconData_Exit_Fail:    // Cleanup
    free( pImageData );
    free( a_xPNGRowPrev );
    free( a_xPNGImage );
    free( a_xData );
    return NULL;
}