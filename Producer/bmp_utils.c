#include <stdio.h>
#include <stdlib.h>

// BMPheader reads in the header info of a BMP file
//attribute packed is because C would fill struct with padding bytes for alignment, which would
//mess up the alignment for reading in the BMP header which has no padding
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

BMPImage * read_BMP_image(char * filename){
    FILE *file = fopen(filename, "rb");

    if (!file) {
        printf("Error opening file");
        return NULL;
    }

    struct BMPHeader header;

    // Read BMP header
    fread(&header, sizeof(header), 1, file);

    // Check if it's a valid BMP file
    if (header.signature[0] != 'B' || header.signature[1] != 'M') {
        printf("Not a valid BMP file\n");
        fclose(file);
        return NULL;
    }

    int imageSize = header.width * header.height * (header.bitsPerPixel / 8);
    unsigned char *imageData = (unsigned char *)malloc(imageSize);

    // Read image data
    fseek(file, header.dataOffset, SEEK_SET);
    fread(imageData, 1, imageSize, file);
    fclose(file);
    BMPImage * frameData = malloc(sizeof(BMPImage));
    frameData->size = imageSize;
    frameData->pixelData = imageData;
    return frameData;
}

