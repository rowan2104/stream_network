#include <stdio.h>
#include <stdlib.h>

#pragma pack(push, 1) // Disable structure padding
typedef struct {
    unsigned char signature[2];
    unsigned int fileSize;
    unsigned int reserved;
    unsigned int dataOffset;
} BMPHeader;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
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
} BMPInfoHeader;
#pragma pack(pop)

typedef struct {
    int size;
    unsigned char * pixelData;
} BMPImage;

int main() {
    FILE *file = fopen("your_image.bmp", "rb"); // Open the BMP file in binary mode

    if (!file) {
        perror("Error opening file");
        return 1;
    }

    BMPHeader header;
    BMPInfoHeader infoHeader;

    // Read BMP header
    fread(&header, sizeof(header), 1, file);
    fread(&infoHeader, sizeof(infoHeader), 1, file);

    // Check if it's a valid BMP file
    if (header.signature[0] != 'B' || header.signature[1] != 'M') {
        fprintf(stderr, "Not a valid BMP file\n");
        fclose(file);
        return 1;
    }

    int imageSize = infoHeader.width * infoHeader.height * (infoHeader.bitsPerPixel / 8);
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

    // Now, 'imageData' contains the pixel data of the BMP image

    // Use the image data as needed

    free(imageData); // Don't forget to free the allocated memory

    return 0;
}
