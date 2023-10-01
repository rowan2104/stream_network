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
    int xPixelsPerMeter;
    int yPixelsPerMeter;
    unsigned int colors;
    unsigned int importantColors;
} __attribute__((packed));

typedef struct {
    int size;
    unsigned char * pixelData;
} BMPImage;



void printBMPHeader(struct BMPHeader header) {
    printf("Signature: %c%c\n", header.signature[0], header.signature[1]);
    printf("File Size: %u\n", header.fileSize);
    printf("Reserved: %u\n", header.reserved);
    printf("Data Offset: %u\n", header.dataOffset);
    printf("Header Size: %u\n", header.headerSize);
    printf("Width: %d\n", header.width);
    printf("Height: %d\n", header.height);
    printf("Planes: %hu\n", header.planes);
    printf("Bits Per Pixel: %hu\n", header.bitsPerPixel);
    printf("Compression: %u\n", header.compression);
    printf("Image Size: %u\n", header.imageSize);
    printf("X Pixels Per Meter: %d\n", header.xPixelsPerMeter);
    printf("Y Pixels Per Meter: %d\n", header.yPixelsPerMeter);
    printf("Colors: %u\n", header.colors);
    printf("Important Colors: %u\n", header.importantColors);
}


BMPImage * read_BMP_image(char * filename){
    FILE *file = fopen(filename, "rb");

    if (!file) {
        printf("Error opening file");
        return NULL;
    }

    struct BMPHeader header;

    fread(&header, sizeof(header), 1, file);

    if (header.signature[0] != 'B' || header.signature[1] != 'M') {
        printf("Not a valid BMP file\n");
        fclose(file);
        return NULL;
    }

    int imageSize = header.width * header.height * (header.bitsPerPixel / 8);
    unsigned char *imageData = (unsigned char *)malloc(imageSize);

    fseek(file, header.dataOffset, SEEK_SET);
    fread(imageData, 1, imageSize, file);
    fclose(file);
    BMPImage * frameData = malloc(sizeof(BMPImage));
    frameData->size = imageSize;
    frameData->pixelData = imageData;
    return frameData;
}

void createBMP(const char* filename, unsigned char* pixelData, int width, int height) {
    struct BMPHeader header;
    FILE* file = fopen(filename, "wb");

    if (!file) {
        perror("Error opening file");
        return;
    }

    header.signature[0] = 'B';
    header.signature[1] = 'M';
    header.fileSize = sizeof(struct BMPHeader) + (width * height * 4);
    header.reserved = 0;
    header.dataOffset = sizeof(struct BMPHeader);
    header.headerSize = sizeof(struct BMPHeader) - 14;
    header.width = width;
    header.height = height;
    header.planes = 1;
    header.bitsPerPixel = 32;
    header.compression = 0;
    header.imageSize = 0;
    header.xPixelsPerMeter = 0;
    header.yPixelsPerMeter = 0;
    header.colors = 0;
    header.importantColors = 0;

    fwrite(&header, sizeof(struct BMPHeader), 1, file);


    fwrite(pixelData, sizeof(unsigned char), width * height * 4, file);

    fclose(file);
}



