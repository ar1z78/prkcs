#ifndef GETPNG_H
#define GETPNG_H

#include "Platform.h"
#include <zlib.h> // NEW: Adds uncompress() and Z_OK definitions

// --- Relocated PNG Parsing Structures ---
typedef struct png_ihdr_struc
{
    unsigned long lWidth;
    unsigned long lHeight;
    PUU8 xBitDepth;
    PUU8 xColorType;
    PUU8 xCompressMethod;
    PUU8 xFilterMethod;
    PUU8 xInterlaceMethod;
} udtPNGihdr_struc;

typedef struct png_sbit_struc
{
    PUU8 xRed;
    PUU8 xGreen;
    PUU8 xBlue;
} udtPNGsbit_struc;

typedef struct png_pixel_struc
{
    PUU8 xRed;
    PUU8 xGreen;
    PUU8 xBlue;
} udtPNGpixel_struc;



#define LEN_PNGSIG 0x8


    extern PUU8 a_xPNGSig[ LEN_PNGSIG ];

    /* Saves an icon directly out of the database as a physical .png file */
    int SaveIconToPng(unsigned long iconKey, const char* destinationFolder);

    /* Relocated 357-line legacy icon parsing routine */
    PUU8* GetAOIconData( unsigned long lIconNo );


#endif // GETPNG_H
