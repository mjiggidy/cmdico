/*	-----													*
 *	parseimg_png.c by Michael Jordan						*
 *	Contains all the functions to read/write/grok PNG data	*
 *	Employs libpng (libpng.org)								*
 *	Created 05/30/2011		Last Update 					*
 *	-----													*/
 
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <png.h>

#include "datatypes.h"

int seekPNG(FILE* infile, ICONENTRY* iconEntry){
	if(!iconEntry)
		return !fseek(infile, 0L, SEEK_SET);
	else if(iconEntry && iconEntry->imageOffset)
		return !fseek(infile, iconEntry->imageOffset, SEEK_SET);
	else
		return false;
}

bool initReadPNG(png_structp png_ptr, png_infop info_ptr, FILE* infile){
	if(!(seekPNG(infile, NULL)))
		return false;
	
	if(!(png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL)) || !(info_ptr = png_create_info_struct(png_ptr))){
		fprintf(stderr, "Error initializing libpng.\n");
		return false;
	}
	
	if(setjmp(png_jmpbuf(png_ptr))){
		fprintf(stderr, "Error reading PNG header.\n");
		return false;
	}
	
	png_init_io(png_ptr, infile);
	png_read_info(png_ptr, info_ptr);

	return true;
}



bool listPNGInfo(FILE* infile, ICONENTRY* iconEntry){
	png_structp	png_ptr;
	png_infop	info_ptr;
	
	if(!seekPNG(infile, iconEntry)){
		fprintf(stderr, "Error locating PNG data.\n");
		return false;
	}
	
/*	Init libpng structs	*/
	if(!initReadPNG(png_ptr, info_ptr, infile)){
		fprintf(stderr, "Error initializing libpng.\n");
		return false;
	}
/*	
	if(!listImageInfo(png_get_image_width(png_ptr, info_ptr), png_get_image_height(png_ptr, info_ptr), png_get_bit_depth(png_ptr, info_ptr) * png_get_channels(png_ptr, info_ptr), imagetype_png)){
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		return false;
	}
*/	
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	return true;
}
/*
IMAGEINFO* storePNGInfo(FILE* infile, ICONENTRY* iconEntry){
	IMAGEINFO*	image;
	png_structp	png_ptr;
	png_infop	info_ptr;

	if(!(image = (IMAGEINFO*)malloc(sizeof(IMAGEINFO)))){
		fprintf(stderr, "Error allocating memory.\n");
		return NULL;
	}

	if(!seekPNG(infile, NULL)){
		fprintf(stderr, "Error locating PNG data.\n");
		free(image);
		return NULL;
	}

	if(!initReadPNG(png_ptr, info_ptr, infile)){
		fprintf(stderr, "Error initializing libpng.\n");
		free(image);
		return NULL;
	}
	
	image->imageType = imagetype_png;
	image->imageFile = infile;

	if(iconEntry)
		image->iconEntry = *iconEntry;

	else{
		image->iconEntry.imageWidth		= png_get_image_width(png_ptr, info_ptr);
		image->iconEntry.imageHeight	= png_get_image_height(png_ptr, info_ptr);
		image->iconEntry.imageBitCount	= png_get_bit_depth(png_ptr, info_ptr) * png_get_channels(png_ptr, info_ptr);
		image->iconEntry.imageOffset	= 0;
		image->iconEntry.colorCount		= 0;	/*	For now	
		image->iconEntry.colorPlanes	= 1;
		image->iconEntry.reserved		= 0;

		fseek(infile, 0L, SEEK_END);
		image->iconEntry.imageBytes = ftell(infile);
	}

//	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	return image;
}
*/
