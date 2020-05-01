/*	-----																	*
 *	DATATYPES.H by Michael Jordan									*
 *	Contains typedefs for data structures used, including:						*
 *													*						*
 *		ICONFILEHEADER		ICO file header							*
 *		ICONENTRY		Metadata for a specific icon entry				*
 *		BITMAPMAGICHEADER	BMP magic number 						*
 *		BITMAPFILEHEADER	BMP file header							*
 *		BITMAPINFOHEADER	BMP metadata							*
 *		RGBQUAD			Uncompressed palette color or 32bit pixel			*
 *													*						*
 *	Perhaps more to come later.  For maximum nice-looking-ness, set tabs to four spaces.		*
 *	Created 05/17/2011		Last Update 05/22/2011						*
 *	-----												*/

#ifndef _STDINT_H
#include <stdint.h>
#endif

#define BI_RGB 0

#define IMG_TYPE_BMP 1
#define IMG_TYPE_PNG 2

typedef enum imgTYPE{imagetype_bmp, imagetype_png} IMAGETYPE;

typedef struct icoICONFILEHEADER{
	uint16_t	reserved;			/* Always 0					*/
	uint16_t	resourceType;			/* 1 for ICO, 2 for CUR				*/
	uint16_t	iconCount;			/* Number of images				*/
/*	ICONENTRY*	iconEntries;			/* REMOVED Array of image metadata		*/
} ICONFILEHEADER;

typedef struct icoICONENTRY{
	uint8_t		imageWidth;			/* Image width in pixels (0 if 256)		*/
	uint8_t		imageHeight;			/* Image height in pixels(0 if 256)		*/
	uint8_t		colorCount;			/* Color resolution (0 if >=8bits)		*/
	uint8_t		reserved;			/* Always 0					*/
	uint16_t	colorPlanes;			/* Number of color planes(always 1?)		*/
	uint16_t	imageBitCount;			/* Bit resolution per pixel			*/
	uint32_t	imageBytes;			/* Size of infoheader+xor+and in bytes		*/
	uint32_t	imageOffset;			/* Offset from beginning of file		*/
} ICONENTRY;

typedef uint16_t BITMAPMAGICHEADER;			/* Always 0x4d42 ("BM" little endian)		*
							 * Removed this from file header struct		*
							 * due to data alignment and packing		*
							 * issues across different compilers.		*/

typedef struct bmpFILEHEADER{
/*	uint16_t	bfType;				/* REMOVED Always 0x4d42 ("BM")			*/
	uint32_t	bfSize;				/* Size of entire file with headers		*/
	uint16_t	bfReserved1;			/* Always 0					*/
	uint16_t	bfReserved2;			/* Always 0					*/
	uint32_t	bfOffBits;			/* Offset from headers to bitmap		*/
} BITMAPFILEHEADER;

typedef struct bmpINFOHEADER{
	uint32_t	biSize;				/* sizeof(BITMAPINFOHEADER)			*/
	int32_t		biWidth;			/* Width of bitmap in pixels			*/
	int32_t		biHeight;			/* Height of bitmap in pixels			*/
	uint16_t	biPlanes;			/* Hopefully always 1				*/
	uint16_t	biBitCount;			/* Bit count per pixel				*/
	uint32_t	biCompression;			/* Hopefully 0 for uncompressed RGB		*/
	uint32_t	biSizeImage;			/* In bytes, can be 0 for RGB			*/
	int32_t		biXPelsPerMeter;		/* Physical resolution, default 0		*/
	int32_t		biYPelsPerMeter;		/* Physical resolution, default 0		*/
	uint32_t	biClrUsed;			/* Num colors in palette, 0 if full		*/
	uint32_t	biClrImportant;			/* Important colors, 0 for all			*/
} BITMAPINFOHEADER;

typedef struct bmpRGBQUAD{ 
	uint8_t		rgbBlue;
 	uint8_t		rgbGreen;
	uint8_t		rgbRed;
	uint8_t		rgbReserved;			/* Alpha for 32-bit BMP, otherwise 0		*/
} RGBQUAD;

typedef struct imgINFO{
	IMAGETYPE	imageType;			/*	Enum for bmp or png			*/
	FILE*		imageFile;			/*	File handle for input file		*/
	char*		imageFileName;			/*	File name string			*/
	int		imageWidth;
	int		imageHeight;
	char		imageBitCount;
/*	ICONENTRY	iconEntry;			/*						*/
} IMAGEINFO;
