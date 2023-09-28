#include <stdio.h>
#include <stdlib.h>

// Define a BMP header struct with packed attribute
struct BMPHeader {
    char signature[2];
    unsigned int fileSize;
    unsigned int reserved;
    unsigned int dataOffset;
    unsigned int headerSize;
    int width;
    int height;
    unsigned short int planes;
    unsigned short int bitsPerPixel;
    unsigned int compression;
    unsigned int imageSize;
    int xResolution;
    int yResolution;
    unsigned int colors;
    unsigned int importantColors;
} __attribute__((packed));

typedef struct {
    int size;
    unsigned char * pixelData;
} BMPImage;

BMPImage * read_BMP_Image(char * filename){
    FILE *file = fopen(filename, "rb"); // Open the BMP file in binary mode
    if (file == 0) {
        printf("Error opening file %s!\n", filename);
        return void;
    }

    BMPHeader header;

    // Read BMP header
    fread(&header, sizeof(header), 1, file);
    // Check if it's a valid BMP file
    if (header.signature[0] != 'B' || header.signature[1] != 'M') {
        fprintf(stderr, "Not a valid BMP file\n");
        fclose(file);
        return 1;
    }

    int imageSize = header.width * header.height * (header.bitsPerPixel / 8);
    unsigned char *imageData = (unsigned char *)malloc(imageSize);

    if (!imageData) {
        perror("Memory allocation failed");
        fclose(file);
        return 1;
    }

    // Read image data
    fseek(file, header.dataOffset, SEEK_SET);
    fread(imageData, 1, imageSize, file);

    fclose(file);
}

int main() {


    // Now, 'imageData' contains the pixel data of the BMP image

    // Use the image data as needed

    free(imageData); // Don't forget to free the allocated memory

    return 0;
}
