#include <stdio.h>
#include <jpeglib.h>

void convert_to_jpeg(char * inputImage, int width, int height, unsigned char ** outPutImage, int * size){
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    JSAMPROW row_pointer[1];

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    // Create a memory buffer for the JPEG data
    unsigned char* jpeg_data = NULL;
    unsigned long jpeg_size = 0;

    jpeg_mem_dest(&cinfo, &jpeg_data, &jpeg_size);

    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, 75, TRUE);

    jpeg_start_compress(&cinfo, TRUE);

    while (cinfo.next_scanline < cinfo.image_height) {
    row_pointer[0] = &inputImage[cinfo.next_scanline * cinfo.image_width * 3];
    jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);

    // Copy the JPEG data to the output buffer
    *size = jpeg_size;
    char *buffer = (unsigned char*)malloc(jpeg_size);
    memcpy(*outPutImage, jpeg_data, jpeg_size);

    // Clean up the memory buffer
    free(jpeg_data);
}

void decode_jpeg(char * JpegData, char * output, int imageSize) {
    // Assume you have received the JPEG data in a buffer

    // Initialize libjpeg structures
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    printf("Here1\n!");
    cinfo.err = jpeg_std_error(&jerr);
    printf("Here2\n!");
    jpeg_create_decompress(&cinfo);
    printf("herheZ\n");
    // Set the input buffer
    // Create a custom memory data source manager
    jpeg_mem_src(&cinfo, JpegData, imageSize);
    printf("imageSize: %d\n",imageSize);
    printf("herheY\n");
    // Read the JPEG header
    jpeg_read_header(&cinfo, TRUE);
    printf("herheX\n");
    // Start decompression
    jpeg_start_decompress(&cinfo);
    printf("herhe3\n");
    // Allocate memory for the pixel data
    int row_stride = cinfo.output_width * cinfo.output_components;
    char *pixelData = (char *)malloc(row_stride * cinfo.output_height);

    // Read scanlines and store them in the pixelData array
    printf("herhe4\n");
    char *row_pointer = pixelData;
    while (cinfo.output_scanline < cinfo.output_height) {
        jpeg_read_scanlines(&cinfo, (JSAMPARRAY)&row_pointer, 1);
        row_pointer += row_stride;
        printf("lmao!\n");
    }
    printf("herhe66\n");
    for (int i = 0; i < 100; ++i) {
        printf("%02x", pixelData[i]);
    }
    printf("\n");
    printf("w*h: %d\n",row_stride * cinfo.output_height);
    memcpy(output, pixelData, row_stride * cinfo.output_height);

    // Finish decompression
    jpeg_finish_decompress(&cinfo);
    printf("herhe6\n");
    // Cleanup
    jpeg_destroy_decompress(&cinfo);
    printf("herhe7\n");

    free(pixelData);
    printf("herhe8\n");
}