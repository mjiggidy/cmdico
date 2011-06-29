#include <stdio.h>
#include <windows.h>

int main(int argc, char** argv){
	
	FILE* infile;
	BITMAPFILEHEADER fileHeader;
	BITMAPINFOHEADER infoHeader;
	
	if(!(infile = fopen("output.bmp","rb"))){
		puts("Unable to open bitmap.");
		return 1;
	}
	
	fread(&fileHeader, sizeof(BITMAPFILEHEADER), 1, infile);
	
	if((fileHeader.bfType != 0x4d42) || (fileHeader.bfReserved1 != 0) || (fileHeader.bfReserved2 != 0)){
		puts("File does not appear to be a valid bitmap.");
		return 1;
	}
	
	printf("Bitmap is %i bytes.  Offset is %i bytes. File header is %i bytes.\n", fileHeader.bfSize, fileHeader.bfOffBits, sizeof(BITMAPFILEHEADER));
	
	fread(&infoHeader, sizeof(BITMAPINFOHEADER), 1, infile);
	
	printf("Header requests %i bytes. Standard header is %i bytes.\n", infoHeader.biSize, sizeof(BITMAPINFOHEADER));
	printf("Bitmap is %i x %i (%i bpp)\n", infoHeader.biWidth, infoHeader.biHeight, infoHeader.biBitCount);
	printf("Resolution is %i x %i per meter\n", infoHeader.biXPelsPerMeter, infoHeader.biYPelsPerMeter);
	printf("Compression used is %i\n", infoHeader.biCompression);
	
	fclose(infile);

	return 0;
}
