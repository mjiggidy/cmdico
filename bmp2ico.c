/*	-----											*
 *	BMP2ICO by Michael Jordan						*
 *	A temporary thing to convert 32bpp BMPs to ICO	*
 *	05/21/2011										*
 *	-----											*/
 
#include <stdio.h>
#include <stdint.h>
#include <malloc.h>
#include "datatypes.h"

int main(){
	char file1[] = "input1.bmp";
	char file2[] = "input2.bmp";
	char file3[] = "input3.bmp";
	char outfilename[] = "output.ico";
	int  x;
	FILE* infile;
	FILE* outfile;
	uint8_t* buffer;
	
	ICONFILEHEADER icon;
	ICONENTRY iconEntry;
	BITMAPMAGICHEADER magicHeader;
	BITMAPFILEHEADER fileHeader;
	BITMAPINFOHEADER infoHeader;
	
	if(!(infile = fopen(file1, "rb"))){
		fprintf(stderr, "Error opening %s.\n", file1);
		return 1;
	}
	
	fread(&magicHeader, sizeof(BITMAPMAGICHEADER), 1, infile);
	if(magicHeader != 0x4d42){
		fprintf(stderr, "%s does not appear to be a valid BMP.\n", file1);
		return 1;
	}
	
	fread(&fileHeader, sizeof(BITMAPFILEHEADER), 1, infile);
	if(fileHeader.bfReserved1 || fileHeader.bfReserved2){
		fprintf(stderr, "%s does not appear to be a valid BMP.\n", file1);
		return 1;
	}
	
	fread(&infoHeader, sizeof(BITMAPINFOHEADER), 1, infile);
	if((infoHeader.biSize != sizeof(BITMAPINFOHEADER)) || (infoHeader.biCompression != BI_RGB)){
		fprintf(stderr, "%s must be in an uncompressed RGB format.\n");
		return 1;
	}
	
	
/*	Create ico file	*/
	if(!(outfile = fopen(outfilename, "wb"))){
		fprintf(stderr, "Unable to write to file %s.\n", outfilename);
		return 1;
	}
	
/*	Create icon headers	*/
	icon.reserved		= 0;
	icon.resourceType	= 1;
	icon.iconCount		= 1;
	
	iconEntry.imageWidth	= infoHeader.biWidth;
	iconEntry.imageHeight	= infoHeader.biHeight;
	iconEntry.colorCount	= 0;
	iconEntry.reserved		= 0;
	iconEntry.colorPlanes	= 1;
	iconEntry.imageBitCount	= 32;
	iconEntry.imageBytes	= sizeof(BITMAPFILEHEADER) + (sizeof(RGBQUAD) * infoHeader.biWidth * infoHeader.biHeight);
	iconEntry.imageOffset	= sizeof(ICONFILEHEADER) + sizeof(ICONENTRY);
	
	fwrite(&icon, sizeof(ICONFILEHEADER), 1, outfile);
	fwrite(&iconEntry, sizeof(ICONENTRY), 1, outfile);
	
/*	Modify and write BMP header	*/
	infoHeader.biHeight = infoHeader.biHeight * 2;
	fwrite(&infoHeader, sizeof(BITMAPINFOHEADER), 1, outfile);
	
/*	Copy bitmap over to new file	*/
	if(!(buffer = (uint8_t*)malloc(sizeof(uint8_t) * infoHeader.biSizeImage))){
		fprintf(stderr, "Error allocating memory for bitmap.\n");
		return 1;
	}
	
	printf("bfOffBits is %i, total header size is %i.\n", fileHeader.bfOffBits, sizeof(BITMAPMAGICHEADER) + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER));
	
	fseek(infile, fileHeader.bfOffBits, SEEK_SET);	/* Seek past file and info headers based on bfOffBits and not header size, because of allowed nonsense between header and pixel array	*/
	fread(buffer, sizeof(uint8_t), infoHeader.biSizeImage, infile);
	fwrite(buffer, sizeof(uint8_t), infoHeader.biSizeImage, outfile);
	
	free(buffer);
	fclose(infile);
	fclose(outfile);
	
	printf("Changes written.\n");
	
	return 0;
}
