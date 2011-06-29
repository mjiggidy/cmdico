/*	-----													*
 *	parseimg_bmp.c by Michael Jordan						*
 *	Contains all the functions to read/write/grok PNG data	*
 *	Employs libpng (libpng.org)								*
 *	Created 05/30/2011		Last Update 					*
 *	-----													*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "datatypes.h"

bool seekBMP(FILE* infile, ICONENTRY* iconEntry){
	if(!iconEntry)
		return !fseek(infile, sizeof(BITMAPMAGICHEADER) + sizeof(BITMAPFILEHEADER), SEEK_SET);
	if(iconEntry && iconEntry->imageOffset)
		return !fseek(infile, iconEntry->imageOffset, SEEK_SET);
	else
		return false;
}





BITMAPINFOHEADER* getBMPInfo(FILE* infile, ICONENTRY* iconEntry){
	BITMAPINFOHEADER* infoHeader;

	if(!seekBMP(infile, iconEntry))
		return NULL;

	if(!(infoHeader = (BITMAPINFOHEADER*)malloc(sizeof(BITMAPINFOHEADER))))
		return NULL;

	fread(infoHeader, sizeof(BITMAPINFOHEADER), 1, infile);

	return infoHeader;
}

bool listBMPInfo(FILE* infile, ICONENTRY* iconEntry){
	BITMAPINFOHEADER infoHeader;
	
	if(!iconEntry)
		fseek(infile, sizeof(BITMAPMAGICHEADER) + sizeof(BITMAPFILEHEADER), SEEK_SET);
	else if(iconEntry && iconEntry->imageOffset)
		fseek(infile, iconEntry->imageOffset, SEEK_SET);
	else{
		fprintf(stderr, "Icon header appears to be invalid.\n");
		return false;
	}
	
	fread(&infoHeader, sizeof(BITMAPINFOHEADER), 1, infile);
	
	if(infoHeader.biSize != sizeof(BITMAPINFOHEADER)){
		fprintf(stderr, "Bitmap info header is an unsupported version.\n");
		return false;
	}
	
//	return listImageInfo(infoHeader.biWidth, iconEntry ? infoHeader.biHeight>>1 : infoHeader.biHeight, infoHeader.biBitCount, imagetype_bmp);

return true;
}
/*
IMAGEINFO* storeBMPInfo(FILE* infile, ICONENTRY* iconEntry){
	IMAGEINFO* image;
	BITMAPINFOHEADER infoHeader;
	uint32_t sizeAnd;

	if(!isBMP(infile, iconEntry)){
		fprintf(stderr, "File is not a BMP image.\n");
		return NULL;
	}

	if(!(image = (IMAGEINFO*)malloc(sizeof(IMAGEINFO)))){
		fprintf(stderr, "Error allocating memory.\n");
		return false;
	}
	
	image->imageType = imagetype_bmp;
	image->imageFile = infile;
	
/*	If accompanies by iconEntry, it already has all the stuff we need.	
	if(iconEntry){
		image->iconEntry = *iconEntry;
		return image;
	}

/*	Otherwise build iconEntry header, sans imageOffset.	
	seekBMP(infile, iconEntry);
	fread(&infoHeader, sizeof(BITMAPINFOHEADER), 1, infile);

/*	Calculate space required for 1bit AND mask aligned to WORD size	
	sizeAnd = (((infoHeader.biWidth + 31)>>5)<<2) * infoHeader.biHeight;

	image->iconEntry.imageWidth		= infoHeader.biWidth;
	image->iconEntry.imageHeight	= infoHeader.biHeight;
	image->iconEntry.colorCount		= infoHeader.biClrUsed;	/* For now	
	image->iconEntry.reserved		= 0;
	image->iconEntry.colorPlanes	= infoHeader.biPlanes;
	image->iconEntry.imageBitCount	= infoHeader.biBitCount;
	image->iconEntry.imageBytes		= sizeof(BITMAPINFOHEADER) + infoHeader.biSizeImage + sizeAnd;
	image->iconEntry.imageOffset	= 0;

	return image;
}
*/
