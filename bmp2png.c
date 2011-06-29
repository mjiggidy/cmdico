/*	-----														*
 *	BMP2PNG by Michael Jordan									*
 *	Temporary thing for converting 32-bit BMP to 24-bit PNG		*
 *	05/13/2011													*
 *	-----														*/

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
	int		x;
	png_bytep*	rows;
	png_structp	png_ptr;
	png_infop	info_ptr;
	
	
	if(!(infile = fopen(infilename, "rb"))){
		fprintf(stderr, "Error opening %s.\n", infilename);
		return 1;
	}
	
	fread(&fileHeader, sizeof(BITMAPFILEHEADER), 1, infile);
	fread(&infoHeader, sizeof(BITMAPINFOHEADER), 1, infile);
	if(infoHeader.biBitCount != 32){
		fprintf(stderr, "Only working with 32-bit BMPs for now.\n");
		return 1;
	}

	if(!(rows = (png_bytep*)malloc(sizeof(png_bytep) * infoHeader.biHeight))){
		fprintf(stderr, "Error allocating memory.\n");
		return 1;
	}
	
/*	Read in each row.  Backwards.  TO THE MAX.												*/
/*	No but srsly bottom row first, BMP are stored upside down								*/
	for(x=infoHeader.biHeight; x>0; x--){
		if(!(rows[x-1] = (png_bytep)malloc(sizeof(png_byte) * infoHeader.biWidth * sizeof(RGBQUAD)))){
			fprintf(stderr, "Error allocating memory for row.\n");
			return 1;
		}
		
		fread(rows[x-1], sizeof(RGBQUAD), infoHeader.biWidth, infile);
	}

/*	PNG TIME!				*/
/*	Init stream for outfile	*/
	if(!(outfile = fopen(outfilename,"wb"))){
		fprintf(stderr, "Error opening %s.", outfilename);
		return 1;
	}
	
	png_ptr  = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	info_ptr = png_create_info_struct(png_ptr);
	
	if(setjmp(png_jmpbuf(png_ptr))){
		fprintf(stderr,"Error initializing PNG output stream.\n");
		return 1;
	}
	
	png_init_io(png_ptr, outfile);
	
/*	Write header	*/
	if(setjmp(png_jmpbuf(png_ptr))){
		fprintf(stderr,"Error writing IHDR to PNG.\n");
		return 1;
	}
	
	png_set_IHDR(png_ptr, info_ptr, infoHeader.biWidth, infoHeader.biHeight, infoHeader.biBitCount/4, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_write_info(png_ptr, info_ptr);
	
/*	Write from bitmapped data	*/
	if(setjmp(png_jmpbuf(png_ptr))){
		fprintf(stderr,"Error compressing to PNG.\n");
		return 1;
	}
	png_set_bgr(png_ptr);
	png_write_image(png_ptr, rows);

/*	Write lead-out	*/
	if(setjmp(png_jmpbuf(png_ptr))){
		fprintf(stderr,"Error finishing PNG stream.\n");
		return 1;
	}
	
	png_write_end(png_ptr, NULL);
	
	fclose(infile);
	fclose(outfile);
	
	
/*	Release memory	*/
	for(x=0; x<infoHeader.biHeight; x++)
		free(rows[x]);
	
	free(rows);
	
	return 0;
}
