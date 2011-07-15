/*	-----																*
 *	CMDICO by Michael Jordan											*
 *	Command-line application to package GIF and PNG files as ICO.		*
 *	For maximum nice-looking-ness, tabs should be set to 4 spaces.		*
 *	04/11/2011															*
 *	-----																*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <setjmp.h>
#include <getopt.h>
#include <zlib.h>
#include <png.h>

#include "datatypes.h"

char* outfilename = "output.bmp";
char* infilename  = "icon.ico";

int parseArgs(int argc, char** argv);
int isPNG(ICONENTRY iconEntry, FILE* icofile);
int isBMP(ICONENTRY iconEntry, FILE* icofile);
int getPNGHeader(ICONENTRY iconEntry, FILE* icofile);

int extractBMP(ICONENTRY iconEntry, FILE* icofile);
int extractPNG(ICONENTRY iconEntry, FILE* icofile);
int getBitmapHeader(ICONENTRY iconEntry, FILE* icofile);
int writePNG(BITMAPINFOHEADER infoHeader, png_bytep* bitmap);

int main(int argc, char** argv){
	char	x;
	int		usr;
	FILE*	icofile;
	ICONFILEHEADER	icon;
	ICONENTRY*		iconEntries;
	
/*	getopt();	*/

	usr = parseArgs(argc, argv);
	
/*	if(!((argc > 1) && (usr = atoi(argv[1]))>0))
		usr = 0;
*/
	
	if(!(icofile = fopen(infilename,"rb"))){
		fprintf(stderr,"Unable to open file %s.\n", infilename);
		return 1;
	}
	
/*	Read in reserved, resourceType, iconCount	*/
	fread(&icon, sizeof(uint16_t), 3, icofile);
	
	if((icon.reserved != 0) || (icon.resourceType != 1) || (icon.iconCount < 1)){
		fprintf(stderr,"File does not appear to be an icon.\n");
		return 1;
	}
	
/*	Allocate iconEntries based on previously-read iconCount and read them in	*/
	if(!(iconEntries = (ICONENTRY*)malloc(sizeof(ICONENTRY) * icon.iconCount))){ 
		fprintf(stderr,"Error reallocating space.\n");
		return 1;
	}

	fread(iconEntries, sizeof(ICONENTRY), icon.iconCount, icofile);
	
	if(!icon.iconCount){
		fprintf(stderr,"File does not contain any icons.\n");
		return 1;
	}	
	

/*	Output icon directory	*/
	if(!usr){
		printf("\nFile %s contains %i icons:\n\n", infilename, icon.iconCount);
		
		for(x=0; x<icon.iconCount; x++){
			printf("%2i: ",x+1);
		
			if(isPNG(iconEntries[x], icofile))
				getPNGHeader(iconEntries[x],icofile);
				
			else if(isBMP(iconEntries[x],icofile))
				printf("%3i x %3i  (%2i bits)  Offset:%6i bytes  BMP\t%i bytes\n",(iconEntries[x].imageWidth ? iconEntries[x].imageWidth : 256), (iconEntries[x].imageHeight ? iconEntries[x].imageHeight : 256), iconEntries[x].imageBitCount, iconEntries[x].imageOffset, iconEntries[x].imageBytes);
	
			else printf("???\n");
		}
		
		printf("\nTo extract an image, use --extract [1-%i].\n", icon.iconCount);
	}
	
/*	Extract specified icon	*/
	else if(usr <= icon.iconCount){
		printf("\nExtracting icon entry #%i...\n", usr);
		
		if(isBMP(iconEntries[usr-1], icofile) && extractBMP(iconEntries[usr-1], icofile))
			printf("Icon entry #%i extracted to %s\n", usr, outfilename);
		else if(isPNG(iconEntries[usr-1], icofile) && extractPNG(iconEntries[usr-1], icofile))
			printf("Icon entry #%i extracted to %s.png\n", usr, outfilename);
		else
			fprintf(stderr,"The image was not extracted.\n");
	}

	else
		fprintf(stderr,"Specified icon entry is invalid.  Must be between 1 and %i.\n", icon.iconCount);
	
	fclose(icofile);
	free(iconEntries);

	return 0;
}

int parseArgs(int argc, char** argv){
	int c, option_index=0, iconIndex=0;

	struct option long_options[] = {
		{"extract",	required_argument,	0,	'e'},	/*	Extract icon	*/
		{"output",	required_argument,	0,	'o'},
		{0,			0,					0,	0}		/*	Null terminator	*/
	};
	
	while(1){
		c = getopt_long(argc, argv, "e:o:", long_options, &option_index);
		if(c == -1)
			break;
		
/*	Parse args	*/
		switch(c){
			case '0':
				printf("Unknown option\n");
				break;
			case 'e':								/*	Extract index	*/
				if(optarg)
					iconIndex = atoi(optarg);
				break;
			case 'o':
				if(optarg){
					outfilename = malloc(sizeof(char) * strlen(optarg)+1);
					strcpy(outfilename, optarg);
				}
				break;
			case '?':								/*	Unknown option	*/
				break;
			default:
				printf("Default\n");
				break;				
		}
	}
	
	if(optind < argc){	/*	Extraneous argument is input filename		*/
		infilename = malloc(sizeof(char)*strlen(argv[optind])+1);
		strcpy(infilename, argv[optind]);
	}

	return iconIndex;
}

int isPNG(ICONENTRY iconEntry, FILE* icofile){
	
	png_byte* buffer;
	
	if(!(buffer = (png_byte*)malloc(sizeof(png_byte)*8))){
		fprintf(stderr, "Error allocating memory.\n");
		return 0;
	}
	
	fseek(icofile, iconEntry.imageOffset, SEEK_SET);
	fread(buffer, sizeof(png_byte), 8, icofile);
	
/*	png_sig_cmp returns 0 if valid	*/
	return !png_sig_cmp(buffer, 0, 8);
}

int isBMP(ICONENTRY iconEntry, FILE* icofile){
	
	BITMAPINFOHEADER infoheader;

	fseek(icofile, iconEntry.imageOffset, SEEK_SET);
	fread(&infoheader, sizeof(BITMAPINFOHEADER), 1, icofile);
	
	if((infoheader.biSize != sizeof(BITMAPINFOHEADER)) || (infoheader.biPlanes != 1) || (infoheader.biBitCount > 32))
		return 0;
	
	return 1;
}

int getPNGHeader(ICONENTRY iconEntry, FILE* icofile){
	
	png_structp	png_ptr;
	png_infop	info_ptr;
	
/*	Init libpng stucts.													*
 *	At this point, isPNG() should have been called to verify PNG sig	*/
 
	if(!(png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL)) || !(info_ptr = png_create_info_struct(png_ptr))){
		fprintf(stderr, "Error initializing libpng.\n");
		return 0;
	}
	
	if(setjmp(png_jmpbuf(png_ptr))){
		fprintf(stderr, "Error reading PNG header.\n");
		return 0;
	}
	
	fseek(icofile, iconEntry.imageOffset, SEEK_SET);
	png_init_io(png_ptr, icofile);
	png_read_info(png_ptr, info_ptr);
	
	printf("%3i x %3i  (%2i bits)  Offset:%6i bytes  PNG\n", png_get_image_width(png_ptr, info_ptr), png_get_image_height(png_ptr, info_ptr), png_get_bit_depth(png_ptr, info_ptr), iconEntry.imageOffset);
	
	return 1;
}

int getBitmapHeader(ICONENTRY iconEntry, FILE* icofile){
	
	BITMAPINFOHEADER dib;
	
	fseek(icofile, iconEntry.imageOffset, SEEK_SET);
	fread(&dib, sizeof(BITMAPINFOHEADER), 1, icofile);
	
	printf("\tHeader reports: %3i x %3i (%i bits)\n", dib.biWidth, dib.biHeight, dib.biBitCount);
	
	return 1;
}

int extractPNG(ICONENTRY iconEntry, FILE* icofile){
	
	FILE* outfile;
	png_byte* buffer;
	
	if(!(outfile = fopen("output.png","wb"))){
		fprintf(stderr,"Error opening stream to output.png\n");
		return 0;
	}
	
	if(!(buffer = (png_byte*)malloc(sizeof(png_byte) * 8))){
		fprintf(stderr,"Error allocating memory.\n");
		fclose(outfile);
		return 0;
	}
	
	fseek(icofile, iconEntry.imageOffset, SEEK_SET);
	fread(buffer, sizeof(png_byte), 8, icofile);
	
	if(png_sig_cmp(buffer, 0, 8)){
		fprintf(stderr,"Image does not appear to be a valid PNG.\n");
		free(buffer);
		fclose(outfile);
		return 0;
	}
	
/*	PNG is valid, so write to new file	*/
	if(!(buffer = (png_byte*)realloc(buffer, sizeof(png_byte)*iconEntry.imageBytes))){
		fprintf(stderr,"Error allocating memory.\n");
		fclose(outfile);
		return 0;
	}
	
	fseek(icofile, iconEntry.imageOffset, SEEK_SET);
	fread(buffer, sizeof(png_byte), iconEntry.imageBytes, icofile);
	fwrite(buffer,sizeof(png_byte), iconEntry.imageBytes, outfile);
	fclose(outfile);
	
	free(buffer);
	
	return iconEntry.imageBytes;
}

int writePNG(BITMAPINFOHEADER infoHeader, png_bytep* bitmap){
	
	FILE* outfile;
	png_structp	png_ptr;
	png_infop	info_ptr;
	
	if(!(outfile = fopen("output.png","wb"))){
		fprintf(stderr,"Error creating PNG output file.\n");
		return 0;
	}
	
	png_ptr  = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	info_ptr = png_create_info_struct(png_ptr);
	
/*	I don't know what this does exactly. lololol.	*/
	if(setjmp(png_jmpbuf(png_ptr))){
		fprintf(stderr,"Error initializing PNG output stream.\n");
		return 0;
	}
	
	png_init_io(png_ptr, outfile);
	
/*	Write header	*/
	if(setjmp(png_jmpbuf(png_ptr))){
		fprintf(stderr,"Error writing IHDR to PNG.\n");
		return 0;
	}
	
	png_set_IHDR(png_ptr, info_ptr, infoHeader.biWidth, infoHeader.biHeight, infoHeader.biBitCount/4,
				 PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	
	png_write_info(png_ptr, info_ptr);
	
/*	Write from bitmapped data	*/
	if(setjmp(png_jmpbuf(png_ptr))){
		fprintf(stderr,"Error compressing to PNG.\n");
		return 0;
	}
	
	png_write_image(png_ptr, bitmap);
	
/*	Write lead-out	*/
	if(setjmp(png_jmpbuf(png_ptr))){
		fprintf(stderr,"Error finishing PNG stream.\n");
		return 0;
	}
	
	png_write_end(png_ptr, NULL);
	
	fclose(outfile);	
	
	return 1;
}

int extractBMP(ICONENTRY iconEntry, FILE* icofile){
	
	int paletteSize, pixelCount;
	uint8_t* bmpPalette;
	uint8_t* bmpPixels;
	FILE*	 outfile;
	
	BITMAPMAGICHEADER magicHeader = 0x4d42;
	BITMAPFILEHEADER  fileHeader;
	BITMAPINFOHEADER  infoHeader;
	
	fseek(icofile, iconEntry.imageOffset, SEEK_SET);
	fread(&infoHeader, sizeof(BITMAPINFOHEADER), 1, icofile);
	
	if((infoHeader.biSize != sizeof(BITMAPINFOHEADER)) || (infoHeader.biPlanes != 1)){
		fprintf(stderr, "This entry does not seem to be a valid bitmap.\n");
		return 0;
	}
	
	if(infoHeader.biCompression != BI_RGB){
		fprintf(stderr, "This bitmap is compressed.  Currently, only uncompressed RGB bitmaps can be extracted.\n");
		return 0;
	}
	
	pixelCount = infoHeader.biWidth * infoHeader.biHeight/2;
	
/*	Determine palette size if less than 32-bits.	*/

	if(infoHeader.biClrUsed)
		paletteSize = infoHeader.biClrUsed;
		
	else{
		switch(infoHeader.biBitCount){
			case 1:
				paletteSize = 2;
				break;
			case 4:
				paletteSize = 16;
				break;
			case 8:
				paletteSize = 256;
				break;
			default:
				paletteSize = 0;
				break;
		}
	}
	
/*	If this is indeed paletted (<32bpp), read it in	*/
	if(paletteSize){
		if(!(bmpPalette = (uint8_t*)malloc(sizeof(RGBQUAD) * paletteSize))){
			fprintf(stderr, "Error allocating memory for palette.\n");
			return 0;
		}
		
		fread(bmpPalette, sizeof(RGBQUAD), paletteSize, icofile);
	
	/*	Now read in pixel array (only for 8 bits)	*/
		if(!(bmpPixels = (uint8_t*)malloc(sizeof(uint8_t) * pixelCount))){
			fprintf(stderr, "Error allocating memory for pixel array.\n");
			free(bmpPalette);
			return 0;
		}
		
		fread(bmpPixels, sizeof(uint8_t), pixelCount, icofile);
	}

/*	If 32bpp raw, read in RGBQUAD array	*/
	else{
		if(!(bmpPixels = (uint8_t*)malloc(sizeof(RGBQUAD) * pixelCount))){
			fprintf(stderr, "Error allocating memory for raw bitmap.\n");
			return 0;
		}
		
		fread(bmpPixels, sizeof(RGBQUAD), pixelCount, icofile);
	}
	
/*	Construct/refine headers	*/
	fileHeader.bfSize		= sizeof(BITMAPMAGICHEADER) + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + (paletteSize ? ((sizeof(RGBQUAD) * paletteSize) + (sizeof(uint8_t) * pixelCount)) : (sizeof(RGBQUAD) * pixelCount));
	fileHeader.bfReserved1	= 0;
	fileHeader.bfReserved2	= 0;
	fileHeader.bfOffBits	= sizeof(BITMAPMAGICHEADER) + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + (paletteSize ? (sizeof(RGBQUAD) * paletteSize) : 0);
	
	infoHeader.biSize		= sizeof(BITMAPINFOHEADER);
	infoHeader.biHeight		= infoHeader.biHeight / 2;
	infoHeader.biSizeImage	= paletteSize ? (sizeof(uint8_t) * pixelCount) : (sizeof(RGBQUAD) * pixelCount);
	
/*	Write headers	*/
	if(!(outfile = fopen(outfilename, "wb"))){
		fprintf(stderr, "Error writing to file %s.\n", outfilename);
		free(bmpPixels);
		if(paletteSize)
			free(bmpPalette);
		return 0;
	}
	
	fwrite(&magicHeader, sizeof(BITMAPMAGICHEADER), 1, outfile);
	fwrite(&fileHeader,  sizeof(BITMAPFILEHEADER),  1, outfile);
	fwrite(&infoHeader,  sizeof(BITMAPINFOHEADER),  1, outfile);
	
/*	If paletted, write that stuff	*/
	if(paletteSize){
		fwrite(bmpPalette, sizeof(RGBQUAD), paletteSize, outfile);
		fwrite(bmpPixels,  sizeof(uint8_t), pixelCount,  outfile);
		free(bmpPalette);
	}
	
/*	Else write RGBQUAD array	*/
	else
		fwrite(bmpPixels, sizeof(RGBQUAD), pixelCount, outfile);
		
	free(bmpPixels);
	fclose(outfile);
	
	return 1;
}
