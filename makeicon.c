/*	-----													*
 *	MAKEICON.C by Michael Jordan							*
 *	Doin' this for reals this time.							*
 *	Created 06/14/2011	Updated								*
 *	-----													*/

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>
#include <png.h>

#include "datatypes.h"

#define MAX_INPUTS 8

//#define DEBUG 1

int processArgs(int argc, char** argv);
IMAGEINFO* parseImage(char* infileName);
bool isSimilarImage(IMAGEINFO* imageA, IMAGEINFO* imageB);
void destroyImage(IMAGEINFO* image);
unsigned long writeIconEntry(IMAGEINFO* image, int index, int total, unsigned long offset);

char** infileNames;
char*  outfileName="output.ico";
FILE*  outfile;

int main(int argc, char** argv){
	int infileCount=0, acceptedCount=0, writtenCount=0;
	int x, y;
	unsigned long offset=0, offsetTemp=0;
	ICONFILEHEADER iconHeader;
	IMAGEINFO** accepted;
	IMAGEINFO*	image=NULL;
	
/*	Gather command-line arguments		*/
	infileCount = processArgs(argc, argv);
	if(!infileCount){
		printShortHelp(argv[0]);
		printf( "For more information, try %s --help\n", argv[0]);
		return 1;
	}

#ifdef DEBUG
	printf("%i files provided.\n", infileCount);
	for(x=0; x<infileCount; x++)
		printf("%s\t",infileNames[x]);

	putchar('\n');

	if(outfileName)
		printf("Outputting to %s\n", outfileName);
#endif

/*	Verify outfileName can be opened and written	*/
	if(!(outfile = fopen(outfileName, "wb"))){
		fprintf(stderr, "Unable to write icon to %s.\n", outfileName);
		return 1;
	}

/*	Verify files are valid inputs, and only one image per	*
 *	bit depth and width/height								*/

	if(!(accepted = (IMAGEINFO**)malloc(sizeof(IMAGEINFO*) * ((infileCount>MAX_INPUTS) ? MAX_INPUTS : infileCount)))){
		fprintf(stderr, "Error allocating memory for input files.\n");
		return 1;
	}

/*	Create IMAGEINFOs here then destroy them (close file and free memory) via destroyImage() later	*/
	for(x=0; (x<infileCount && acceptedCount<MAX_INPUTS); x++){

#ifdef DEBUG
		printf("Trying %s\n", infileNames[x]);
#endif
		image = (IMAGEINFO*)parseImage(infileNames[x]);
		if(!image){
			fprintf(stderr, "%s will be skipped.\n\n", infileNames[x]);
			free(infileNames[x]);
			continue;
		}

		for(y=0; y<acceptedCount; y++){
			if(isSimilarImage(image, accepted[y])){
				fprintf(stderr, "File %s has the same dimensions and bit depth as %s.\n", image->imageFileName, accepted[y]->imageFileName);
				fprintf(stderr, "%s will be skipped.\n\n", image->imageFileName);
				destroyImage(image);
				image = NULL;
				break;
			}
		}
	/*	If image survives the comparison loop, add it or whatever I don't care.	*/
		if(image)
			accepted[acceptedCount++] = image;
	}

	if(!acceptedCount){
		fprintf(stderr, "No acceptable images found.\n");
		return 1;
	}

/*	Cleanup memory by resizing accepted array and destroying infile array	*/
	if((acceptedCount < infileCount) && (acceptedCount < MAX_INPUTS)){
		if(!(accepted = (IMAGEINFO**)realloc(accepted, sizeof(IMAGEINFO*) * acceptedCount))){
			fprintf(stderr, "Error reallocating memory.\n");
			return 1;
		}
	}

	free(infileNames);	/* Filenames are now stored in the IMAGEINFOs via parseImage()					*/
	image = NULL;		/* So I don't keep using image->property instead of accepted[x]->property oy	*/

	printf("\nFound %i acceptable image(s).\n", acceptedCount);

	for(x=0; x<acceptedCount; x++){
		printf("  %s\t", accepted[x]->imageFileName);
		printf("%3i x %3i  (%2i bpp)  %s\n", accepted[x]->imageWidth, accepted[x]->imageHeight, accepted[x]->imageBitCount, (accepted[x]->imageType==imagetype_bmp) ? "BMP" : "PNG" );
	}

/*	Write images to icon file	*/
	for(x=0; x<acceptedCount; x++){
		if(offsetTemp = writeIconEntry(accepted[x], writtenCount, acceptedCount, offset)){
			offset = offsetTemp;
			writtenCount++;
			printf("Wrote %s to file.\n", accepted[x]->imageFileName);
			printf(" ** writtenCount is %i.\n", writtenCount);
		}

		else
			fprintf(stderr, "Error writing %s to icon file.\n", accepted[x]->imageFileName);

		destroyImage(accepted[x]);
	}

	free(accepted);

/*	Write icon file header	*/
	iconHeader.reserved = 0;
	iconHeader.resourceType = 1;
	iconHeader.iconCount = writtenCount;

	printf("iconHeader.iconCount is %i.\n", iconHeader.iconCount);

	fseek(outfile, 0L, SEEK_SET);
	fwrite(&iconHeader, sizeof(ICONFILEHEADER), 1, outfile);
	
	fclose(outfile);
	free(outfileName);

	return 0;
}

int processArgs(int argc, char** argv){

	int c, option_index=0, iconIndex=0, infileCount;

	struct option long_options[] = {
		{"help",	no_argument,		0,	'h'},	/*	Help				*/
		{"output",	required_argument,	0,	'o'},	/*	Specify output file	*/
		{0,			0,					0,	0}		/*	Null terminator		*/
	};
	
	while(1){
		c = getopt_long(argc, argv, "ho:", long_options, &option_index);
		if(c == -1)
			break;
		
/*	Parse args	*/
		switch(c){
			case 'h':
				printHelp(argv[0]);
				exit(0);
				break;
			case 'o':
				if(optarg){
					if(!(outfileName = malloc(sizeof(char) * strlen(optarg)+1))){
						fprintf(stderr, "Error allocating memory.\n");
						return 0;
					}
					strcpy(outfileName, optarg);
				}
				break;
			case '?':	/*	Unknown option	*/
				break;
			default:
				printf("Default\n");
				break;				
		}
	}

/*	Stray arguments are input files	*/
	if(optind >= argc)
		return 0;

	infileCount = argc - optind;
	if(!(infileNames = (char**)malloc(infileCount * sizeof(char*)))){
		fprintf(stderr, "Error allocating memory.\n");
		return 0;
	}
	
	for(c=0; c<infileCount; c++){
		if(!(infileNames[c] = (char*)calloc(strlen(argv[optind+c]) + 1, sizeof(char)))){
			fprintf(stderr, "Error allocating memory.\n");
			free(infileNames);
			return 0;
		}

		strcpy(infileNames[c], argv[optind+c]);
	}

	return infileCount;
}

bool isBMP(FILE* infile, ICONENTRY* iconEntry){
	
	BITMAPMAGICHEADER magicHeader;
	BITMAPFILEHEADER fileHeader;
	BITMAPINFOHEADER infoHeader;
 
/*	If no iconEntry is passed, assume we're reading from a	*
 *	bitmap file, so check magic number and file header.		*
 *	Otherwise, just check some infoheader stuff.			*/

	if(!iconEntry){
		fseek(infile, 0L, SEEK_SET);
		fread(&magicHeader, sizeof(BITMAPMAGICHEADER), 1, infile);
		
		if(magicHeader != 0x4d42)
			return false;

		fread(&fileHeader, sizeof(BITMAPFILEHEADER), 1, infile);
		
		if(fileHeader.bfReserved1 || fileHeader.bfReserved2){
#ifdef DEBUG
			fprintf(stderr, "Bitmap file header appears malformed.\n");
#endif
			return false;
		}
	}
	
	if(!seekBMP(infile, iconEntry))
		return false;

	fread(&infoHeader, sizeof(BITMAPINFOHEADER), 1, infile);
	
	if((infoHeader.biSize != sizeof(BITMAPINFOHEADER)) || (infoHeader.biPlanes != 1) || (infoHeader.biBitCount > 32)){
#ifdef DEBUG
		fprintf(stderr, "This bitmap is incompatible for now.\n");
#endif
		return false;
	}
	
	return true;
}

bool isPNG(FILE* infile, ICONENTRY* iconEntry){
	
	png_byte* png_buffer;
	
	if(!(png_buffer = (png_byte*)malloc(sizeof(png_byte)*8))){
		fprintf(stderr, "Error allocating memory to read PNG signature.\n");
		return false;
	}
 
	if(!seekPNG(infile, iconEntry)){
		fprintf(stderr,"Error finding PNG data.\n");
		return false;
	}
		
	fread(png_buffer, sizeof(png_byte), 8, infile);
	
/*	png_sig_cmp returns 0 if valid	*/

	if(png_sig_cmp(png_buffer, 0, 8)){
		free(png_buffer);
		return false;
	}
	
	free(png_buffer);
	return true;
}

IMAGEINFO* parseImage(char* infileName){
	IMAGEINFO* image;
	IMAGETYPE imageType;
	FILE* infile;

	png_structp	png_ptr;
	png_infop	info_ptr;
	BITMAPINFOHEADER* bmp_header;

	if(!(infile = fopen(infileName, "rb"))){
		fprintf(stderr, "Cannot open %s.\n", infileName);
		return NULL;
	}

	if(isPNG(infile, NULL))
		imageType = imagetype_png;
		
	else if(isBMP(infile, NULL))
		imageType = imagetype_bmp;
	
	else{
		fprintf(stderr, "%s does not appear to be a supported image format.\n", infileName);
		fclose(infile);
		return NULL;
	}


	if(!(image = (IMAGEINFO*)malloc(sizeof(IMAGEINFO)))){
		fprintf(stderr, "Error allocating memory.\n");
		fclose(infile);
		return NULL;
	}

/*	Fill IMAGEINFO with... um... image info.	*/
	switch(imageType){
		case imagetype_png:

			if(!(seekPNG(infile, NULL)))
				return NULL;
			
			if(!(png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL)) || !(info_ptr = png_create_info_struct(png_ptr))){
				fprintf(stderr, "Error initializing libpng.\n");
				return NULL;
			}
			
			if(setjmp(png_jmpbuf(png_ptr))){
				fprintf(stderr, "Error reading PNG header.\n");
				return NULL;
			}
			
			png_init_io(png_ptr, infile);
			png_read_info(png_ptr, info_ptr);
			image->imageWidth    = png_get_image_width(png_ptr, info_ptr);
			image->imageHeight   = png_get_image_height(png_ptr, info_ptr);
			image->imageBitCount = png_get_bit_depth(png_ptr, info_ptr) * png_get_channels(png_ptr, info_ptr);
			png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
			break;

		case imagetype_bmp:
			if(!(bmp_header = (BITMAPINFOHEADER*)getBMPInfo(infile, NULL))){
				fprintf(stderr, "Unable to parse bitmap for %s.\n", infileName);
				return NULL;
			}
			
			image->imageWidth    = bmp_header->biWidth;
			image->imageHeight   = bmp_header->biHeight;
			image->imageBitCount = bmp_header->biBitCount;
			break;

		default:
			fprintf(stderr, "Unknown error parsing file.\n");
			return NULL;
			break;
	}

	image->imageType	 = imageType;
	image->imageFile	 = infile;
	image->imageFileName = infileName;

/*	Validate	*/
	if(image->imageWidth != image->imageHeight){
		fprintf(stderr, "%s is %ix%i, but valid images must be square.\n", image->imageFileName, image->imageWidth, image->imageHeight);
		fclose(infile);
		free(image);
		return NULL;
	}

	if(image->imageWidth % 8){
		fprintf(stderr, "%s is %ix%i pixels, but valid images must have dimensions divisible by 8.\n", image->imageFileName, image->imageWidth, image->imageHeight);
		fclose(infile);
		free(image);
		return NULL;
	}

	if(image->imageBitCount != 32){
		fprintf(stderr, "%s is %i bits, but currently only 32-bit images are supported.\n", image->imageFileName, image->imageBitCount);
		fclose(infile);
		free(image);
		return NULL;
	}

	if((image->imageWidth == 256) && (image->imageBitCount != 32)){
		fprintf(stderr, "%s is 256x256, but only %i bits.  256x256 icons must be 32-bit images.\n", image->imageFileName, image->imageBitCount);
		fclose(infile);
		free(image);
		return NULL;
	}

	

	return image;
}

bool isSimilarImage(IMAGEINFO* imageA, IMAGEINFO* imageB){
	if((imageA->imageWidth == imageB->imageWidth) && (imageA->imageBitCount == imageB->imageBitCount))
		return true;

	return false; 
}

void destroyImage(IMAGEINFO* image){
	fclose(image->imageFile);
	free(image->imageFileName);
	free(image);
	return;
}

unsigned long writeIconEntry(IMAGEINFO* image, int index, int total, unsigned long offset){

	ICONENTRY iconEntry;
	RGBQUAD	  rgbQuad;
	BITMAPFILEHEADER fileHeader;
	BITMAPINFOHEADER infoHeader;
	int x, y, z, sizeAnd;
	uint8_t*	buffer, *bufferAnd;
	uint8_t**	bmp_rows;
	png_bytep*	png_rows;
	png_structp png_ptr;
	png_infop	info_ptr;

	if(!offset)
		offset = sizeof(ICONFILEHEADER) + (sizeof(ICONENTRY) * total);

/*	If 256x256 32-bit PNG, write it directly from infile		*/
	if((image->imageWidth == 256) && (image->imageType = imagetype_png) && (image->imageBitCount == 32)){

		printf("Writing %s directly as 256x256 PNG.\n", image->imageFileName);
		
		fseek(image->imageFile, 0L, SEEK_END);
		iconEntry.imageBytes = ftell(image->imageFile);
		iconEntry.imageWidth = 0;
		iconEntry.imageHeight = 0;
		iconEntry.colorCount = 0;
		iconEntry.reserved = 0;
		iconEntry.colorPlanes = 1;
		iconEntry.imageBitCount = image->imageBitCount;
		iconEntry.imageOffset = offset;
		
		if(!(buffer = (uint8_t*)malloc(sizeof(uint8_t) * iconEntry.imageBytes))){
			fprintf(stderr, "Error allocating memory for %s.\n", image->imageFileName);
			return 0;
		}

		fseek(image->imageFile, 0L, SEEK_SET);
		fread(buffer, sizeof(uint8_t), iconEntry.imageBytes, image->imageFile);

		fseek(outfile, sizeof(ICONFILEHEADER) + (sizeof(ICONENTRY) * index), SEEK_SET);
		fwrite(&iconEntry, sizeof(ICONENTRY), 1, outfile);

		fseek(outfile, offset, SEEK_SET);
		fwrite(buffer, sizeof(uint8_t), iconEntry.imageBytes, outfile);

		free(buffer);

		return ftell(outfile);
	}
		

/*	Convert PNG to BMP if applicable	*/
	if(image->imageType == imagetype_png){

		printf("Converting %s from PNG to BMP.\n", image->imageFileName);
		
		fseek(image->imageFile, 0L, SEEK_SET);
		
		if(!(png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL)) || !(info_ptr = png_create_info_struct(png_ptr))){
			fprintf(stderr, "Error initializing libpng.\n");
			return false;
		}
		
		if(setjmp(png_jmpbuf(png_ptr))){
			fprintf(stderr, "Error reading PNG header.\n");
			return false;
		}

		png_init_io(png_ptr, image->imageFile);
		png_read_info(png_ptr, info_ptr);

/*		Allocate memory for PNG rows	*/
		if(!(png_rows = (png_bytep*)malloc(sizeof(png_bytep) * image->imageHeight))){
			fprintf(stderr, "Error allocating memory for %s.\n", image->imageFileName);
			return 0;
		}

		for(x=0; x<image->imageHeight; x++){
			if(!(png_rows[x] = (png_bytep)malloc(sizeof(png_byte) * png_get_rowbytes(png_ptr, info_ptr)))){
				fprintf(stderr, "Error allocating memory for %s.\n", image->imageFileName);
				free(png_rows);
				return 0;
			}
		}

		if(setjmp(png_jmpbuf(png_ptr))){
			fprintf(stderr, "Error converting %s from PNG.\n", image->imageFileName);
			for(x=0; x<image->imageHeight; x++)
				free(png_rows[x]);
			free(png_rows);
			png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
			return false;
		}

		png_set_bgr(png_ptr);
		png_read_image(png_ptr, png_rows);

		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	}

	else if((image->imageType == imagetype_bmp) && (image->imageBitCount == 32)){

		printf("Writing %s directly as BMP.\n", image->imageFileName);

		fseek(image->imageFile, sizeof(BITMAPMAGICHEADER), SEEK_SET);
		fread(&fileHeader, sizeof(BITMAPFILEHEADER), 1, image->imageFile);
		fread(&infoHeader, sizeof(BITMAPINFOHEADER), 1, image->imageFile);		

		if(!(bmp_rows = (uint8_t**)malloc(sizeof(uint8_t*) * image->imageHeight))){
			fprintf(stderr, "Error allocating memory for %s.\n", image->imageFileName);
			return 0;
		}

		fseek(image->imageFile, fileHeader.bfOffBits, SEEK_SET);
		printf("**Offset is  %i\n",fileHeader.bfOffBits);

		for(x=0; x<image->imageHeight; x++){
			if(!(bmp_rows[x] = (uint8_t*)malloc(sizeof(RGBQUAD) * image->imageWidth))){
				fprintf(stderr, "Error allocating memory for %s.\n", image->imageFileName);
				free(bmp_rows);
				return 0;
			}

			fread(bmp_rows[x], sizeof(RGBQUAD), image->imageWidth, image->imageFile);

		}

	}	

/*	Adjust BITMAPINFO					*/

	sizeAnd = (((image->imageWidth + 31)>>5)<<2) * image->imageHeight;
	
	infoHeader.biSize 			= sizeof(BITMAPINFOHEADER);
	infoHeader.biWidth			= image->imageWidth;
	infoHeader.biHeight			= image->imageHeight << 1;
	infoHeader.biPlanes			= 1;
	infoHeader.biBitCount		= image->imageBitCount;
	infoHeader.biCompression	= BI_RGB;
	infoHeader.biSizeImage		= sizeof(RGBQUAD) * image->imageWidth * image->imageHeight + sizeAnd;
	infoHeader.biXPelsPerMeter	= 0;
	infoHeader.biYPelsPerMeter	= 0;
	infoHeader.biClrUsed		= 0;
	infoHeader.biClrImportant	= 0;


/*	Write ICONENTRY header to file		*/

	iconEntry.imageWidth		= image->imageWidth;		
	iconEntry.imageHeight		= image->imageHeight;
	iconEntry.colorCount		= infoHeader.biClrUsed;
	iconEntry.reserved			= 0;
	iconEntry.colorPlanes		= infoHeader.biPlanes;
	iconEntry.imageBitCount		= image->imageBitCount;
	iconEntry.imageBytes		= sizeof(BITMAPINFOHEADER) + infoHeader.biSizeImage;
	iconEntry.imageOffset		= offset;

	fseek(outfile, sizeof(ICONFILEHEADER) + (sizeof(ICONENTRY) * index), SEEK_SET);
	fwrite(&iconEntry, sizeof(ICONENTRY), 1, outfile);

/*	Write bitmap to file				*/

	fseek(outfile, offset, SEEK_SET);

	fwrite(&infoHeader, sizeof(BITMAPINFOHEADER), 1, outfile);

	if(image->imageType == imagetype_bmp){
		for(x=0; x<image->imageHeight; x++){
			fwrite(bmp_rows[x], sizeof(RGBQUAD), image->imageWidth, outfile);
			free(bmp_rows[x]);
		}
		free(bmp_rows);
	}

	else if(image->imageType == imagetype_png){
/*	Write from bottom to top, as BMPs are stored	*/
		for(x=0; x<image->imageHeight; x++){
			fwrite(png_rows[image->imageHeight-x-1], sizeof(RGBQUAD), image->imageWidth, outfile);
			free(png_rows[image->imageHeight-x-1]);
		}
		free(png_rows);
	}
	else{
		fprintf(stderr, "Error writing %s, unknown type.\n", image->imageFileName);
		return 0;
	}

/*	Write AND mask to file				*/
/*	Garbage for now						*/
	buffer = (uint8_t*)calloc(sizeof(uint8_t), sizeAnd);

	fwrite(buffer, sizeof(uint8_t), sizeAnd, outfile);

	return ftell(outfile);
}
