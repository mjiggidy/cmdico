/*	-----														*
 *	PNG2BMP by Michael Jordan									*
 *	Temporary thing for converting 24-bit PNG to 32-bit BMP.	*
 *	05/16/2011													*
 *	-----														*/


#include <stdio.h>
#include <png.h>
#include <windows.h>

int main(){
	int  x;
	char infilename[]	= "input.png";
	char outfilename[]	= "output.bmp";
	char* buffer;
	FILE* infile;
	FILE* outfile;
	BITMAPFILEHEADER fileHeader;
	BITMAPINFOHEADER infoHeader;
	png_structp png_ptr;
	png_infop	info_ptr;
	png_byte*	png_buffer;
	png_bytep*	png_rows;
	
/*	Read in PNG	*/
	if(!(infile = fopen(infilename, "rb"))){
		fprintf(stderr, "Error opening input file %s.\n", infilename);
		return 1;
	}
	
	if(!(png_buffer = (png_byte*)malloc(sizeof(png_byte) * 8))){
		fprintf(stderr, "Error allocating memory for PNG signature.\n");
		return 1;
	}
	
	fread(png_buffer, sizeof(png_byte), 8, infile);
	if(png_sig_cmp(png_buffer, 0, 8)){
		fprintf(stderr, "This file does not appear to be a valid PNG.\n");
		free(png_buffer);
		return 1;
	}
	
	free(png_buffer);
	
	if(!(png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL)) || !(info_ptr = png_create_info_struct(png_ptr))){
		fprintf(stderr, "Error initializing libpng.\n");
		free(png_buffer);
		return 1;
	}
	
	png_set_sig_bytes(png_ptr, 8);

	if(setjmp(png_jmpbuf(png_ptr))){
		fprintf(stderr, "Error reading PNG header.\n");
		return 1;
	}
	
	png_init_io(png_ptr, infile);
	png_read_info(png_ptr, info_ptr);
	
	if((png_get_bit_depth(png_ptr, info_ptr) != 8) || (png_get_color_type(png_ptr, info_ptr) != PNG_COLOR_TYPE_RGB_ALPHA)){
		fprintf(stderr, "This only works with 24-bit-per-pixel (8 bits per channel) RGB PNGs for now.\n");
		return 1;
	}
	
	printf("This is a valid PNG for our purposes.\n");
	
	if(!(png_rows = (png_bytep*)malloc(sizeof(png_bytep) * png_get_image_height(png_ptr, info_ptr)))){
		fprintf(stderr, "Error allocating memory for PNG data.\n");
		return 1;
	}
	

/*	Allocate array of pointers for each uncompressed RGBA row	*/
	for(x=0; x<png_get_image_height(png_ptr, info_ptr); x++){
		if(!(png_rows[x] = (png_bytep)malloc(png_get_rowbytes(png_ptr, info_ptr)))){
			fprintf(stderr, "Error allocating memory for uncompressed row.\n");
			return 1;
		}
	}
	
	png_set_bgr(png_ptr);
	png_read_image(png_ptr, png_rows);
	
/*	Build BMP headers	*/
	fileHeader.bfType		= 0x4d42;
	fileHeader.bfSize		= sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + (png_get_image_height(png_ptr, info_ptr) * png_get_rowbytes(png_ptr, info_ptr));
	fileHeader.bfReserved1	= 0;
	fileHeader.bfReserved2	= 0;
	fileHeader.bfOffBits	= sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	
	infoHeader.biSize			= sizeof(BITMAPINFOHEADER);
	infoHeader.biWidth			= png_get_image_width(png_ptr, info_ptr);
	infoHeader.biHeight			= png_get_image_height(png_ptr, info_ptr);
	infoHeader.biPlanes			= 1;
	infoHeader.biSizeImage		= png_get_image_height(png_ptr, info_ptr) * png_get_rowbytes(png_ptr, info_ptr);
	infoHeader.biBitCount		= png_get_bit_depth(png_ptr, info_ptr) * png_get_channels(png_ptr, info_ptr);
	infoHeader.biCompression	= BI_RGB;
	infoHeader.biXPelsPerMeter 	= 0;				
	infoHeader.biYPelsPerMeter 	= 0;
	infoHeader.biClrUsed		= 0;
	infoHeader.biClrImportant 	= 0;

/*	Write data to BMP file	*/
	if(!(outfile = fopen(outfilename, "wb"))){
		fprintf(stderr, "Unable to open steam to output file %s.\n", outfilename);
		for(x=0; x<png_get_image_height(png_ptr, info_ptr); x++)
			free(png_rows[x]);
		free(png_rows);
		return 1;
	}

	fwrite(&fileHeader, sizeof(BITMAPFILEHEADER), 1, outfile);
	fwrite(&infoHeader, sizeof(BITMAPINFOHEADER), 1, outfile);
	
	for(x=png_get_image_height(png_ptr, info_ptr); x>0; x--)
		fwrite(png_rows[x-1], png_get_rowbytes(png_ptr, info_ptr), 1, outfile);

	for(x=0; x<png_get_image_height(png_ptr, info_ptr); x++)
		free(png_rows[x]);
	free(png_rows);
	
	fclose(outfile);
	fclose(infile);
	
	return 0;
}
