#define COBJMACROS      // CRITICAL: Must be at the very top for GCC/MinGW C COM macros to work
#include "aomd.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>       // For abs() tracking
#include <wincodec.h>   // Windows Imaging Component
#include <shlwapi.h>    // For SHCreateMemStream
#include "RDB.h"        // Exposes GetDataChunk, AODB_TYP_ITEM, AODB_TYP_PF, and AODB_TYP_ICON

// Link against required Windows subsystems (Native MSVC directives)
#pragma comment(lib, "Windowscodecs.lib")
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Ole32.lib")

/*******************************
Various parts borrowed from AOMD
(database access, PNG unpacking, playfield names, find item name finder)
********************************/
void GetMissionItem(MissionItem* _pMissionItem, PUU32 _ItemKey1, PUU32
	_ItemKey2, PUU32 _QL)
{
	MissionItem sItem1, sItem2;

	_pMissionItem->QL = _QL;
	if (!_ItemKey1)
	{
		goto FetchItemName_Err_NotFound;
	}

	/* Get description for item number 1 */
	if (!GetAODBItem(&sItem1, _ItemKey1))
	{
		goto FetchItemName_Err_NotFound;
	}

	/* If no item number 2, then just keep the first description */
	if (!_ItemKey2 || _ItemKey2 == _ItemKey1)
	{
		strcpy(_pMissionItem->pName, sItem1.pName);
		_pMissionItem->IconKey = sItem1.IconKey;
		_pMissionItem->Value = sItem1.Value;
	}
	/* Item number 2 exists, must interpolate */
	else
	{
		if (!GetAODBItem(&sItem2, _ItemKey2))
		{
			goto FetchItemName_Err_NotFound;
		}

		if (abs(_QL - sItem1.QL) < abs(sItem2.QL - _QL))
		{
			strcpy(_pMissionItem->pName, sItem1.pName);
			_pMissionItem->IconKey = sItem1.IconKey;
		}
		else
		{
			strcpy(_pMissionItem->pName, sItem2.pName);
			_pMissionItem->IconKey = sItem2.IconKey;
		}

		if ((sItem2.QL - sItem1.QL) == 0)
		{
			_pMissionItem->Value = sItem1.Value;
		}
		else
		{
			_pMissionItem->Value = sItem1.Value + ((sItem2.Value - sItem1.Value) / (sItem2.QL - sItem1.QL) * (_QL - sItem1.QL));
		}
	}

	/* Success */
	return;

FetchItemName_Err_NotFound:
	sprintf(_pMissionItem->pName, "Unknown (%X:%X)", _ItemKey1, _ItemKey2);
	_pMissionItem->IconKey = 0;
	return;
}


/* Get item Data from PRK Database */
PUU8 GetAODBItem(MissionItem* _pMissionItem, PUU32 _ItemKey)
{
	PUU8 *a_xData;
	unsigned long lDataLen = sizeof(MissionItem);
	if (!(a_xData = GetDataChunk(AODB_TYP_ITEM, _ItemKey, &lDataLen)))
	{
		return FALSE;
	}
	if (lDataLen != sizeof(MissionItem))
	{
		return FALSE;
	}
	memcpy(_pMissionItem, a_xData, sizeof(MissionItem));
	return TRUE;
}

PUU8 *GetAOIconData(unsigned long lIconNo)
{
	unsigned long lDataLen = 0;
	PUU8 *a_xData = NULL;
	PUU8 *pImageData = NULL;
	unsigned long lBytesPerRow = 0;
	UINT width = 0;
	UINT height = 0;
	HRESULT hr = S_OK;

	// COM/WIC Core Interfaces
	IStream *pStream = NULL;
	IWICImagingFactory *pFactory = NULL;
	IWICBitmapDecoder *pDecoder = NULL;
	IWICBitmapFrameDecode *pFrame = NULL;
	IWICFormatConverter *pConverter = NULL;

	/* 1. Extract raw binary chunk from local SQLite database using your engine hook */
	a_xData = (PUU8*)GetDataChunk(AODB_TYP_ICON, lIconNo, &lDataLen);
	if (!a_xData)
	{
		return NULL;
	}

	// Initialize COM Apartment thread state
	hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (FAILED(hr) && hr != RPC_E_CHANGED_MODE)
	{
		free(a_xData);
		return NULL;
	}

	/* 2. Wrap raw SQLite array bytes into a standard native Windows IStream object */
	pStream = SHCreateMemStream((const BYTE*)a_xData, lDataLen);
	free(a_xData); // Safe to free data block immediately
	if (!pStream) goto Exit_Cleanup;

	/* 3. Create the WIC Factory */
	hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, &IID_IWICImagingFactory, (void**)&pFactory);
	if (FAILED(hr)) goto Exit_Cleanup;

	/* 4. Stream pixel parsing into the native PNG subsystem */
	hr = IWICImagingFactory_CreateDecoderFromStream(pFactory, pStream, NULL, WICDecodeMetadataCacheOnDemand, &pDecoder);
	if (FAILED(hr)) goto Exit_Cleanup;

	/* 5. Extract frame index 0 */
	hr = IWICBitmapDecoder_GetFrame(pDecoder, 0, &pFrame);
	if (FAILED(hr)) goto Exit_Cleanup;

	/* 6. Mount a Format Converter module */
	hr = IWICImagingFactory_CreateFormatConverter(pFactory, &pConverter);
	if (FAILED(hr)) goto Exit_Cleanup;

	// Fetch the dimensions safely before running layout conversions
	IWICBitmapFrameDecode_GetSize(pFrame, &width, &height);

	// Force automatic execution of PNG filters (0-4) and convert straight to standard 24bpp BGR bitmap layout
	hr = IWICFormatConverter_Initialize(pConverter, (IWICBitmapSource*)pFrame, &GUID_WICPixelFormat24bppBGR,
		WICBitmapDitherTypeNone, NULL, 0.0f, WICBitmapPaletteTypeCustom);
	if (FAILED(hr)) goto Exit_Cleanup;

	/* 7. Calculate bytes per row honoring necessary 32-bit layout alignments */
	lBytesPerRow = (((width * 24) + 31) / 32) * 4;
	pImageData = (PUU8 *)malloc(height * lBytesPerRow);
	if (!pImageData) goto Exit_Cleanup;

	/* 8. Output fully uncompressed, unfiltered bits straight into your target buffer */
	hr = IWICFormatConverter_CopyPixels(pConverter, NULL, lBytesPerRow, height * lBytesPerRow, (BYTE*)pImageData);
	if (FAILED(hr))
	{
		free(pImageData);
		pImageData = NULL;
	}
	else
	{
		/* --- COLOR REPLACEMENT LOOP --- */
		UINT y, x;

		// Loop through each row of the image
		for (y = 0; y < height; y++)
		{
			// Point directly to the start of this row in memory
			PUU8 *pPixel = pImageData + (y * lBytesPerRow);

			// Loop through every pixel in this row
			for (x = 0; x < width; x++)
			{
				// CRITICAL: Remember the layout is BGR (Blue, Green, Red)
				PUU8 blue = pPixel[0];
				PUU8 green = pPixel[1];
				PUU8 red = pPixel[2];

				// Example: Match target color (Pure Green: R=0, G=255, B=0)
				if (green == 255 && red == 0 && blue == 0)
				{
					// Replace with your replacement color (Gray: R=100, G=100, B=100)
					pPixel[0] = 255; // Blue component
					pPixel[1] = 255; // Green component
					pPixel[2] = 255; // Red component
				}

				// Advance the pointer by 3 bytes to move to the next pixel
				pPixel += 3;
			}
		}
	}


Exit_Cleanup:
	// Safely drop internal ref counts via uniform macros
	if (pConverter) IWICFormatConverter_Release(pConverter);
	if (pFrame) IWICBitmapFrameDecode_Release(pFrame);
	if (pDecoder) IWICBitmapDecoder_Release(pDecoder);
	if (pFactory) IWICImagingFactory_Release(pFactory);
	if (pStream) IStream_Release(pStream);

	CoUninitialize();
	return pImageData;
}


/* Return mission PlayField descriptive string database layout mapping */
void MissionPF(PUS32 _PFNum, PUU8* _pPFString)
{
	PUU8 *pData;

	/* Read data for this playfield */
	if (!(pData = (PUU8*)GetDataChunk(AODB_TYP_PF, _PFNum, NULL)))
	{
		return;
	}

	strcpy((char*)_pPFString, (char*)pData);

	free(pData);
}