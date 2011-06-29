/*	-----											*
 *	BMP2ICO by Michael Jordan						*
 *	A temporary thing to convert 32bpp BMPs to ICO	*
 *	05/21/2011										*
 *	-----											*/
 
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <malloc.h>

#include "datatypes.h"

typedef struct{
	FILE* infile;
	BITMAPINFOHEADER infoHeader;
	uint32_t bmpOffset;
} BMPINPUT;

BMPINPUT* parseImage(char* infilename);
void writeFileHeader(FILE* outfile, char acceptedCount);
unsigned long writeIconEntry(FILE* outfile, BMPINPUT* image, char countCurrent, char countTotal, unsigned long offset);

int main(){
	
	char* infiles[] = {"icon_ps\\128.bmp","icon_ps\\48.bmp","icon_ps\\32.bmp","icon_ps\\16.bmp"};
	char outfilename[] = "icon_ps\\output.ico";
	char infilesCount, acceptedCount, x;
	unsigned long offset;
	BMPINPUT** accepted;
	BMPINPUT*  temp;
	FILE* outfile;
	
	infilesCount	= 4;
	acceptedCount	= 0;
	
/*	Allocate space for maximum potential inputs	*/
	if(!(accepted = (BMPINPUT**)malloc(sizeof(BMPINPUT*) * infilesCount))){
		fprintf(stderr, "Error allocating memory.\n");
		return 1;
	}
	
/*	Validate and process input images via parseImage()	*/
	for(x=0; x<infilesCount; x++){
		if(temp = parseImage(infiles[x]))
			accepted[acceptedCount++] = temp;
		else
			fprintf(stderr, "Skipping file %s.\n", infiles[x]);
	}
	
	if(!(acceptedCount > 0)){
		fprintf(stderr, "\nNone of the input files could be used.\n");
		return 1;
	}
	
	printf("Found %i acceptable files.\n", acceptedCount);
	
/*	Shrink **accepted so we don't waste memory.	*/
	if(acceptedCount != infilesCount){
		if(!(accepted = (BMPINPUT**)realloc(accepted, sizeof(BMPINPUT*) * acceptedCount))){
			fprintf(stderr, "Error reallocating memory.\n");
			return 1;
		}
	}
	
/*	Write ICONFILEHEADER to output via writeFileHeader()	*/
	if(!(outfile = fopen(outfilename, "wb"))){
		fprintf(stderr, "Error writing to file %s.\n", outfilename);
		return 1;
	}
	
	writeFileHeader(outfile, acceptedCount);
	
	printf("Output to %s:\n\n", outfilename);
	
/*	Loop through to write ICONENTRY header and copy bitmap data	*/
	offset = sizeof(ICONFILEHEADER) + (sizeof(ICONENTRY) * acceptedCount);
	for(x=0; x<acceptedCount; x++){
		if(!(offset = writeIconEntry(outfile, accepted[x], x, acceptedCount, offset))){
			fprintf(stderr, "Error writing icon #%i to file.  Aborting.\n", x+1);
			fclose(outfile);
			return 1;
		}
		
		printf("%2i: %3i x %3i  (%2i bpp)\tBMP\n", x+1, accepted[x]->infoHeader.biWidth, accepted[x]->infoHeader.biHeight>>1, accepted[x]->infoHeader.biBitCount);
		free(accepted[x]);
	}
	free(accepted);
	fclose(outfile);
	
	return 0;
}

unsigned long writeIconEntry(FILE* outfile, BMPINPUT* image, char countCurrent, char countTotal, unsigned long offset){

	ICONENTRY iconEntry;
	RGBQUAD	rgbQuad;
	uint8_t* buffer, *bufferAnd;
	int x,y,z;
	int sizeAnd;
	
	sizeAnd = (((image->infoHeader.biWidth + 31)>>5)<<2) * (image->infoHeader.biHeight>>1);
	
	iconEntry.imageWidth	= image->infoHeader.biWidth;
	iconEntry.imageHeight	= image->infoHeader.biHeight >> 1;
	iconEntry.colorCount	= 0;
	iconEntry.reserved		= 0;
	iconEntry.colorPlanes	= 1;
	iconEntry.imageBitCount	= image->infoHeader.biBitCount;
	iconEntry.imageBytes	= sizeof(BITMAPINFOHEADER) + image->infoHeader.biSizeImage + sizeAnd;
	iconEntry.imageOffset	= offset;

/*	Write ICONENTRY	*/
	fseek(outfile, sizeof(ICONFILEHEADER) + (countCurrent * sizeof(ICONENTRY)), SEEK_SET);
	fwrite(&iconEntry, sizeof(ICONENTRY), 1, outfile);
	
/*	Write BITMAPINFOHEADER	*/
	fseek(outfile, offset, SEEK_SET);
	fwrite(&(image->infoHeader), sizeof(BITMAPINFOHEADER), 1, outfile);
	
/*	Copy bitmap pixel array from input file to memory, then write it	*/
	if(!(buffer = (uint8_t*)malloc(sizeof(uint8_t) * image->infoHeader.biSizeImage))){
		fprintf(stderr, "Error allocating memory.\n");
		return 0;
	}
	
	fseek(image->infile, image->bmpOffset, SEEK_SET);
	fread(buffer, sizeof(uint8_t), image->infoHeader.biSizeImage, image->infile);
	fwrite(buffer, sizeof(uint8_t), image->infoHeader.biSizeImage, outfile);
	fclose(image->infile);
	
/*	Try some lulzy-type AND mask	*/
	if(!(bufferAnd = (uint8_t*)calloc((((image->infoHeader.biWidth + 31)>>5)<<2), sizeof(uint8_t)))){
		fprintf(stderr, "Error allocating memory for AND mask.\n");
		return 0;
	}
	

	for(x=0; x<iconEntry.imageHeight; x++){
		for(y=0; y<iconEntry.imageWidth/8; y++){
			for(z=0; z<8; z++){
				bufferAnd[y] <<=1;
				memcpy(&rgbQuad, &buffer[((iconEntry.imageHeight * x) + 8*y + z) * sizeof(RGBQUAD)], sizeof(RGBQUAD));
				if(!rgbQuad.rgbReserved)
					bufferAnd[y] |= 1;
			}
		}
		
		fwrite(bufferAnd, sizeof(uint8_t), (((image->infoHeader.biWidth + 31)>>5)<<2), outfile);
	}


	
	free(buffer);
	free(bufferAnd);
	
	return ftell(outfile);
}

BMPINPUT* parseImage(char* infilename){
	
	BMPINPUT* processed;
	BITMAPMAGICHEADER magicHeader;
	BITMAPFILEHEADER fileHeader;
	FILE* infile;
	
	if(!(infile = fopen(infilename, "rb"))){
		fprintf(stderr, "Unable to open file %s.\n", infilename);
		return NULL;
	}
	
	fread(&magicHeader, sizeof(BITMAPMAGICHEADER), 1, infile);
	if(magicHeader != 0x4d42){
		fprintf(stderr, "File %s does not appear to be a bitmap.\n", infilename);
		fclose(infile);
		return NULL;
	}
	
	fread(&fileHeader, sizeof(BITMAPFILEHEADER), 1, infile);
	if((fileHeader.bfReserved1 != 0) || (fileHeader.bfReserved2 != 0) || (fileHeader.bfOffBits < (sizeof(BITMAPMAGICHEADER) + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)))){
		fprintf(stderr, "File %s does not appear to be a valid bitmap.\n", infilename);
		fclose(infile);
		return NULL;
	}
	
/*	Allocate for BMPINPUT	*/
	if(!(processed = (BMPINPUT*)malloc(sizeof(BMPINPUT)))){
		fprintf(stderr, "Error allocating memory for %s.\n", infilename);
		fclose(infile);
		return NULL;
	}
	
	processed->bmpOffset = fileHeader.bfOffBits;
	processed->infile	 = infile;
	fread(&(processed->infoHeader), sizeof(BITMAPINFOHEADER), 1, infile);
	
/*	Check validity.  Issue verbal spankings.	*/
	if((processed->infoHeader.biSize != sizeof(BITMAPINFOHEADER)) || (processed->infoHeader.biPlanes != 1)){
		fprintf(stderr, "File %s does not appear to be a supported bitmap.\n", infilename);
		free(processed);
		fclose(infile);
		return NULL;
	}
	
	if((processed->infoHeader.biWidth != processed->infoHeader.biHeight)){
		fprintf(stderr, "File %s must be a square image.\n", infilename);
		free(processed);
		fclose(infile);
		return NULL;
	}
	
	if((processed->infoHeader.biWidth > 256)){
		fprintf(stderr, "File %s cannot be larger than 256x256 pixels.\n", infilename);
		free(processed);
		fclose(infile);
		return NULL;
	}
	
	if(processed->infoHeader.biCompression != BI_RGB){
		fprintf(stderr, "File %s cannot be compressed.\n", infilename);
		free(processed);
		fclose(infile);
		return NULL;
	}
	
	if(processed->infoHeader.biBitCount != 32){
		fprintf(stderr, "File %s must be 32 bits per pixel for now.\n", infilename);
		free(processed);
		fclose(infile);
		return NULL;
	}
	
/*	File is PROBABLY a valid BMP now. Oy.  Adjust infoHeader for ICON weirdness.	*/

	if(!processed->infoHeader.biSizeImage)
		processed->infoHeader.biSizeImage = sizeof(RGBQUAD) * processed->infoHeader.biWidth * processed->infoHeader.biHeight;
		
	processed->infoHeader.biHeight <<= 1;
	
	return processed;	
}


void writeFileHeader(FILE* outfile, char acceptedCount){

	ICONFILEHEADER fileHeader;
	
	fileHeader.reserved		= 0;
	fileHeader.resourceType	= 1;
	fileHeader.iconCount	= acceptedCount;
	
	fwrite(&fileHeader, sizeof(ICONFILEHEADER), 1, outfile);	
}

bool listImageInfo(int32_t width, int32_t height, uint16_t bitcount, char imageType){
	printf("%3i x %3i\t(%2i bpp)\t%s", width, height, bitcount, ((imageType == IMG_TYPE_PNG) ? "PNG" : "BMP"));
	return true;
}
