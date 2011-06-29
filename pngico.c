#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <png.h>

#include "datatypes.h"



int main(){

	char* infilename  = "input.png";
	char* outfilename = "output.ico";
	uint8_t* buffer;
	IMAGEINFO* image;
	ICONFILEHEADER iconHeader;

	FILE* infile;
	FILE* outfile;

	if(!(infile = fopen(infilename, "rb"))){
		fprintf(stderr, "Unable to open %s.\n", infilename);
		return 1;
	}

	if(!isPNG(infile, NULL)){
		fprintf(stderr, "File is not a valid PNG.\n");
		return 1;
	}

	if(!(image = (IMAGEINFO*)storePNGInfo(infile, NULL))){
		fprintf(stderr, "PNG could not be read.\n");
		return 1;
	}

	if(!(outfile = fopen(outfilename, "wb"))){
		fprintf(stderr, "Unable to open %s.\n", outfilename);
		free(image);
		return 1;
	}

	iconHeader.reserved = 0;
	iconHeader.resourceType = 1;
	iconHeader.iconCount = 1;

	fwrite(&iconHeader, sizeof(ICONFILEHEADER), 1, outfile);

	image->iconEntry.imageOffset = sizeof(ICONFILEHEADER) + sizeof(ICONENTRY);
	fwrite(&(image->iconEntry), sizeof(ICONENTRY), 1, outfile);

	seekPNG(infile, NULL);
	if(!(buffer = (uint8_t*)malloc(sizeof(uint8_t) * image->iconEntry.imageBytes))){
		fprintf(stderr, "Error allocating memory.\n");
		return 1;
	}
	
	fread(buffer, sizeof(uint8_t), image->iconEntry.imageBytes, infile);
	fwrite(buffer, sizeof(uint8_t), image->iconEntry.imageBytes, outfile);

	free(buffer);
	free(image);

	printf("Wrote %s to file %s successfully.\n", infilename, outfilename);

	fclose(outfile);
	fclose(infile);
	return 0;
}
