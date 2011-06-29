/*	-----													*
 *	PLTE2PNG by Michael Jordan								*
 *	Temporary thing for converting paletted BMPs to PNG		*
 *	05/15/2011												*
 *	-----													*/

#include <stdio.h>
#include <png.h>
#include <windows.h>

int main(){

	BITMAPFILEHEADER fileHeader;
	BITMAPINFOHEADER infoHeader;
	FILE*	infile;
	FILE*	outfile;
	char	infilename[] = "output.bmp";
	char	outfilename[]= "output.png";
	int		x, paletteSize, rowSize;
	png_colorp	png_palette;
	png_bytep*	png_bitmap;
	png_structp	png_ptr;
	png_infop	info_ptr;
	
	if(!(infile = fopen(infilename, "rb"))){
		fprintf(stderr, "Error opening %s.\n", infilename);
		return 1;
	}
	
	fread(&fileHeader, sizeof(BITMAPFILEHEADER), 1, infile);
	fread(&infoHeader, sizeof(BITMAPINFOHEADER), 1, infile);
	if(infoHeader.biBitCount > 8){
		fprintf(stderr, "Only working with 8-bit palettes for now.\n");
		return 1;
	}
	
/*	Determine number of colors in palette.  If biClrUsed is not defined, it is 2^(biBitCount)	*/
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
	
	printf("Bitmap has %i colors in its palette.\n",paletteSize);
	
/*	png_colorp points to an array of RGBQUAD pallete	*/
	if(!(png_palette = (png_colorp)malloc(sizeof(char) * 3 * paletteSize))){
		fprintf(stderr, "Error allocating memory for color palette.\n");
		return 1;
	}
	
	for(x=0; x<paletteSize; x++){
		fread(&png_palette[x], sizeof(char), 3, infile);
		fseek(infile, 1, SEEK_CUR);
	}
	
/*	Read in bitmap from bottom to top	*/
	rowSize = infoHeader.biWidth * infoHeader.biPlanes;
	
	if(!(png_bitmap = (png_bytep*)malloc(sizeof(png_bytep)*infoHeader.biHeight))){
		fprintf(stderr, "Error allocating memory for bitmap.\n");
		return 1;
	}
	
	for(x=infoHeader.biHeight; x>0; x--){
		if(!(png_bitmap[x-1] = (png_bytep)malloc(sizeof(png_byte) * rowSize))){
			fprintf(stderr, "Error allocating memory for bitmap row.\n");
			return 1;
		}
		
		fread(png_bitmap[x-1], sizeof(png_byte), rowSize, infile);
	}
	
/*	pngtiem	*/
	if(!(outfile = fopen(outfilename, "wb"))){
		fprintf(stderr, "Error opening stream to %s.\n", outfilename);
		return 1;
	}
	
	png_ptr  = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	info_ptr = png_create_info_struct(png_ptr);
	
	if(setjmp(png_jmpbuf(png_ptr))){
		fprintf(stderr,"Error initializing PNG output stream.\n");
		return 1;
	}
	
	png_init_io(png_ptr, outfile);

/*	Write palette	*/

	png_set_PLTE(png_ptr, info_ptr, png_palette, paletteSize);

/*	Write header	*/
	if(setjmp(png_jmpbuf(png_ptr))){
		fprintf(stderr,"Error writing IHDR to PNG.\n");
		return 1;
	}
	
	png_set_IHDR(png_ptr, info_ptr, infoHeader.biWidth, infoHeader.biHeight, infoHeader.biBitCount, PNG_COLOR_TYPE_PALETTE, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_write_info(png_ptr, info_ptr);
	
/*	Write bitmap	*/
	if(setjmp(png_jmpbuf(png_ptr))){
		fprintf(stderr,"Error compressing to PNG.\n");
		return 1;
	}

	png_write_image(png_ptr, png_bitmap);
	png_set_bgr(png_ptr);
/*	Write lead-out	*/
	if(setjmp(png_jmpbuf(png_ptr))){
		fprintf(stderr,"Error finishing PNG stream.\n");
		return 1;
	}
	
	png_write_end(png_ptr, NULL);
	
	
	
	for(x=0; x<infoHeader.biHeight; x++)
		free(png_bitmap[x]);
	free(png_bitmap);
	free(png_palette);
	
	fclose(infile);
	
	return 0;
}
