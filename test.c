#include <stdio.h>
#include <windows.h>

void main(){

	FILE* file;
	WORD bm = 0x4D42;
	
	file = fopen("test.dat","wb");
	fwrite(&bm, sizeof(WORD), 1, file);
	fclose(file);
}
