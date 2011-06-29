/*	-----																*
 *	CMDICO by Michael Jordan											*
 *	Command-line application to package GIF and PNG files as ICO.		*
 *	For maximum nice-looking-ness, tabs should be set to 4 spaces.		*
 *	04/11/2011															*
 *	-----																*/

#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <unistd.h>
#include <windows.h>
#include <zlib.h>
#include <png.h>



typedef struct{
	BYTE		imageWidth;			/* Image width in pixels (0 if 256)	*/
	BYTE		imageHeight;		/* Image height in pixels(0 if 256)	*/
	BYTE		colorCount;			/* Color resolution (0 if >=8bits)	*/
	BYTE		reserved;			/* Always 0							*/
	WORD		colorPlanes;		/* Number of color planes(always 1?)*/
	WORD		imageBitCount;		/* Bit resolution per pixel			*/
	DWORD		imageBytes;			/* Size of raw image data in bytes	*/
	DWORD		imageOffset;		/* Offset from beginning of file	*/
} ICONENTRY;

typedef struct{
	WORD		reserved;			/* Always 0							*/
	WORD		resourceType;		/* 1 for ICO, 2 for CUR				*/
	WORD		iconCount;			/* Number of images					*/
	ICONENTRY*	iconEntries;		/* Array of image metadata			*/
} ICON;

typedef struct{
	BITMAPINFOHEADER imageHeader;
	RGBQUAD*	colorTable;			/*	Indexed color table for <24bpp	*/
	BYTE*		maskXOR;			/*	Color image						*/
	BYTE*		maskAND;			/*	1bpp monochrome mask for <24bpp	*/
} ICONDIB;

char outfilename[] = "output.bmp";

int isPNG(ICONENTRY iconEntry, FILE* icofile);
int isBMP(ICONENTRY iconEntry, FILE* icofile);

int extractBMP(ICONENTRY iconEntry, FILE* icofile);
int extractWithPallette(ICONENTRY iconEntry, FILE* icofile);
int extractPNG(ICONENTRY iconEntry, FILE* icofile);
int getBitmapHeader(ICONENTRY iconEntry, FILE* icofile);
int writePNG(BITMAPINFOHEADER infoHeader, png_bytep* bitmap);

int main(int argc, char* argv[]){
	char	x;
	char	infile[] = "icon.ico";
	int		iconCount, usr;
	ICON	icon;
	FILE*	icofile;
	
/*	getopt();	*/
	
	if(!((argc > 1) && (usr = atoi(argv[1]))>0))
		usr = 0;
	
	if(!(icofile = fopen(infile,"rb"))){
		fprintf(stderr,"Error opening icon file.\n");
		return 1;
	}
	
/*	Read in reserved, resourceType, iconCount	*/
	fread(&icon, sizeof(WORD), 3, icofile);
	
	if((icon.reserved != 0) || (icon.resourceType != 1) || (icon.iconCount < 1)){
		fprintf(stderr,"File does not appear to be an icon.\n");
		return 1;
	}
	
/*	Allocate iconEntries based on previously-read iconCount and read them in	*/
	if(!(icon.iconEntries = (ICONENTRY*)malloc(sizeof(ICONENTRY) * icon.iconCount))){ 
		fprintf(stderr,"Error reallocating space.\n");
		return 1;
	}

	fread(icon.iconEntries, sizeof(ICONENTRY), icon.iconCount, icofile);
	
	if(!icon.iconCount){
		fprintf(stderr,"File does not contain any icons.\n");
		return 1;
	}

/*	Output icon directory	*/
	printf("\nFile %s contains %i icons:\n\n", infile, icon.iconCount);
	
	for(x=0; x<icon.iconCount; x++){
		printf("%2i: ",x+1);
		
		if(isPNG(icon.iconEntries[x], icofile))
			getPNGHeader(icon.iconEntries[x],icofile);
			
		else if(isBMP(icon.iconEntries[x],icofile))
			printf("%3i x %3i  (%2i bits)  Offset:%6i bytes  BMP\n",icon.iconEntries[x].imageWidth, icon.iconEntries[x].imageHeight, icon.iconEntries[x].imageBitCount, icon.iconEntries[x].imageOffset);

		else printf("???\n");
	}
	
	
	putchar('\n');
	
/*	Extract specified icon	*/
	if(usr && (usr <= icon.iconCount)){
		printf("Extracting icon entry #%i...\n", usr);
		
		if(isBMP(icon.iconEntries[usr-1], icofile) && extractBMP(icon.iconEntries[usr-1], icofile))
			printf("Icon entry #%i extracted to %s\n", usr, outfilename);
		else if(isPNG(icon.iconEntries[usr-1], icofile) && extractPNG(icon.iconEntries[usr-1], icofile))
			printf("Icon entry #%i extracted to %s.png\n", usr, outfilename);
		else
			fprintf(stderr,"Unable to extract icon entry.\n");
	}

	else if(argc > 1)
		fprintf(stderr,"Specified icon entry is invalid.  Must be between 1 and %i.\n", icon.iconCount);
		
	else
		printf("To extract an image, provide its index (1-%i) as an argument.\n", icon.iconCount);
	
	fclose(icofile);
	free(icon.iconEntries);

	return 0;
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
	
	int x;
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

int extractWithPallette(ICONENTRY iconEntry, FILE* icofile){
	int palletteSize, rowSize;
	int x;
	char* buffer;
	FILE* outfile;
	BITMAPFILEHEADER fileHeader;
	BITMAPINFOHEADER infoHeader;
	
	fseek(icofile, iconEntry.imageOffset, SEEK_SET);
	fread(&infoHeader, sizeof(BITMAPINFOHEADER), 1, icofile);
	
	if(infoHeader.biPlanes != 1){
		fprintf(stderr, "This does not seem to be a valid icon entry.\n");
		return 0;
	}
	
/*	Determine size of RGBQUAD pallette.  If biClrUsed is not defined, it is 2^(biBitCount)	*/
	if(infoHeader.biClrUsed)
		palletteSize = infoHeader.biClrUsed;

	else{
		switch(infoHeader.biBitCount){
			case 1:
				palletteSize = 2;
				break;
			case 4:
				palletteSize = 16;
				break;
			case 8:
				palletteSize = 256;
				break;
			default:
				palletteSize = 0;
				break;
		}
	}

/*	Size of bitmapped data per row	*/
	rowSize = infoHeader.biWidth * infoHeader.biPlanes;
	
/*	Contruct BITMAPFILEHEADER		*/
	fileHeader.bfType		= 0x4D42;
	fileHeader.bfSize		= sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + (palletteSize*sizeof(RGBQUAD)) + (sizeof(char) * rowSize * iconEntry.imageHeight);
	fileHeader.bfReserved1	= 0;
	fileHeader.bfReserved2	= 0;
	fileHeader.bfOffBits	= sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + (palletteSize*sizeof(RGBQUAD));
	
/*	Construct BITMAPINFOHEADER.		*/
	infoHeader.biSize		= sizeof(BITMAPINFOHEADER);
	infoHeader.biWidth		= iconEntry.imageWidth;
	infoHeader.biHeight		= iconEntry.imageHeight;
	infoHeader.biPlanes		= 1;
	infoHeader.biBitCount	= iconEntry.imageBitCount;
	infoHeader.biCompression= BI_RGB;
	infoHeader.biSizeImage	= rowSize * iconEntry.imageHeight;
	infoHeader.biXPelsPerMeter = 0;
	infoHeader.biYPelsPerMeter = 0;
	infoHeader.biClrUsed	= infoHeader.biClrUsed;
	infoHeader.biClrImportant  = infoHeader.biClrImportant;
	
/*	Write to disk	*/
	if(!(outfile = fopen(outfilename, "wb"))){
		fprintf(stderr, "Error opening stream to %s.",outfilename);
		return 0;
	}
	
	fwrite(&fileHeader, sizeof(BITMAPFILEHEADER), 1, outfile);
	fwrite(&infoHeader, sizeof(BITMAPINFOHEADER), 1, outfile);
	
	if(!(buffer = (char*)malloc(sizeof(char) * palletteSize * sizeof(RGBQUAD)))){
		fprintf(stderr, "Error allocating memory.\n");
		return 0;
	}

	fseek(icofile, iconEntry.imageOffset + sizeof(BITMAPINFOHEADER), SEEK_SET);
	fread(buffer, sizeof(RGBQUAD), palletteSize, icofile);
	fwrite(buffer, sizeof(RGBQUAD), palletteSize, outfile);
	
	if(!(buffer = (char*)realloc(buffer, sizeof(char) * rowSize * iconEntry.imageHeight))){
		fprintf(stderr, "Error allocating memory.\n");
		return 0;
	}
	
	fread(buffer, sizeof(char) * rowSize, iconEntry.imageHeight, icofile);
	fwrite(buffer, sizeof(char) * rowSize, iconEntry.imageHeight, outfile);
	
	fclose(outfile);
	
	free(buffer);	
	
	return 1;
}

int extractBMP(ICONENTRY iconEntry, FILE* icofile){
	
	int 	rawsize;
	int		x;
	char*	buffer;
	FILE* 	outfile;
	BITMAPFILEHEADER fileHeader;
	BITMAPINFOHEADER infoHeader;
	
	fseek(icofile, iconEntry.imageOffset, SEEK_SET);
	fread(&infoHeader, sizeof(BITMAPINFOHEADER), 1, icofile);
	
	rawsize = sizeof(RGBQUAD) * infoHeader.biWidth * infoHeader.biHeight/2;
	
	printf("Computed rawsize is %i, claimed size is %i.\n", rawsize, infoHeader.biSizeImage);
	
/*	Construct BITMAPFILEHEADER. This precedes BITMAPINFOHEADER. */
	fileHeader.bfType		= 0x4D42;	/*	"BM" (hehe)	*/
	fileHeader.bfSize		= rawsize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	fileHeader.bfReserved1	= 0;
	fileHeader.bfReserved2	= 0;
	fileHeader.bfOffBits	= sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	
/*	Construct BITMAPINFOHEADER. The existing header is incompatible with DIB standards. */
	infoHeader.biSize = sizeof(BITMAPINFOHEADER);
/*	infoHeader.biWidth = infoHeader.biWidth;	*/
	infoHeader.biHeight = infoHeader.biHeight/2;		/*	height is originally twice; xor + and maps	*/
	infoHeader.biPlanes = 1;
	infoHeader.biSizeImage = rawsize;					/* Can be 0 for uncompressed RGB?	*/
/*	infoHeader.biBitCount = iconEntry.imageBitCount;										*/
/*	infoHeader.biCompression = BI_RGB;					/* 0. Are icons always RGB or PNG?	*/
/*	infoHeader.biXPelsPerMeter = 0;						/* metrics? lol no u				*/
/*	infoHeader.biYPelsPerMeter = 0;
	infoHeader.biClrUsed = 0;
	infoHeader.biClrImportant = 0;															*/
	
/*	Write bitmap to disk	*/
	if(!(outfile = fopen(outfilename,"wb"))){
		fprintf(stderr,"Error opening stream to %s\n",outfilename);
		return 0;
	}

	if(!(buffer = (char*)malloc(sizeof(char) * rawsize))){
		fprintf(stderr,"Error allocating memory.\n");
		return 0;
	}
	
	fseek(icofile, iconEntry.imageOffset+sizeof(BITMAPINFOHEADER), SEEK_SET);
	fread(buffer, sizeof(char), rawsize, icofile);
	fwrite(&fileHeader, sizeof(BITMAPFILEHEADER), 1, outfile);
	fwrite(&infoHeader, sizeof(BITMAPINFOHEADER), 1, outfile);
	fwrite(buffer, sizeof(char), rawsize, outfile);
	fclose(outfile);		
		
	free(buffer);
	
	return 1;	
}

int writePNG(BITMAPINFOHEADER infoHeader, png_bytep* bitmap){
	
	FILE* outfile;
	int x;	
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





/*	NEW	
	
/*	Allocate array of pointers. Parent pointers are imageHeight in number	
	if(!(buffer = (png_bytep*)malloc(sizeof(png_bytep) * iconEntry.imageHeight))){
		fprintf(stderr, "Error allocating memory for rows.");
		return 0;
	}
	
	fseek(icofile, iconEntry.imageOffset+sizeof(BITMAPINFOHEADER), SEEK_SET);

	for(x=0; x<iconEntry.imageHeight; x++){
		if(!(buffer[x] = (png_byte*)malloc(sizeof(RGBQUAD) * iconEntry.imageWidth))){
			fprintf(stderr, "Error allocating memory for width.");
			return 0;
		}
		
		fread(buffer[x], sizeof(RGBQUAD), iconEntry.imageWidth, icofile);
	}
	
	writePNG(infoHeader, buffer);
	
	for(x=0; x<iconEntry.imageHeight; x++)
		free(buffer[x]);
	
		
	
	
/*	/NEW	*/	
