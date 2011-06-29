#include <stdio.h>

void printShortHelp(char* filename){

	printf( "\nCMDICO version 0.1 by Michael Jordan <michael@glowingpixel.com>\n"
			"http://cmdico.sourceforge.net/\n\n");

	printf( "USAGE:\n    %s [options] <infile(s)> [-o <outfile>]\n\n", filename);
}

void printHelp(char* filename){

	printShortHelp(filename);

	printf( "CREATE ICON:\n"
			"An icon can be created from one or more valid PNG or BMP images.\n\n");

	printf( "Examples:\n"
			"    %s input_32.png input_48.png -o output.ico\n"
			"    %s *.png *.bmp\n\n", filename, filename);

	printf( "    -o <outfile>        Provide the filename for the output icon.  If this\n"
			"    --output <outfile>  option is not specified, the filename will default to\n"
			"                        output.ico in the current working directory.\n\n");

	printf( "    For now, valid input images must be 32-bit PNG (24-bit + 8-bit alpha) or\n"
			"    32-bit BMP RGBA format.  Dimensions must be square, and divisible by 8.\n"
			"    For a full list of requirements and recommended resolutions, please visit\n"
			"    http://cmdico.sourceforge.net/.\n\n");


	printf( "EXTRACT IMAGE FROM ICON:\n"
			"An image can be extracted as a PNG from an existing icon by providing an icon\n"
			"file as the <infile> and using the -e or --extract option.  For a list of\n"
			"images contained with an icon, use --list.\n\n");

	printf( "Examples:\n"
			"    %s --list input.ico\n"
			"    %s -e 2 input.ico -o output.png\n\n", filename, filename);

	printf( "    -l                  List the images contained within an icon when an icon\n"
			"    --list              is specified as the <infile>.\n\n");

	printf( "    -e <index>          Extract an image from an icon <infile>, where <index>\n"
			"    --extract <index>   is the numeric index of the image within the icon file.\n"
			"                        Use --list for a list of images and their indexes.\n");
}
