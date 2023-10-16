#include <stdio.h>
#include <jpeglib.h>
#include <stdlib.h>

void convert_to_jpeg(char* inputImage, int width, int height, unsigned char* outPutImage, int* size, int quality_threshold) {
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

    int current_quality = 90;  // Starting quality

    while (current_quality >= 10) {
        jpeg_set_defaults(&cinfo);
        jpeg_set_quality(&cinfo, current_quality, TRUE);

        jpeg_start_compress(&cinfo, TRUE);
        while (cinfo.next_scanline < cinfo.image_height) {
            row_pointer[0] = &inputImage[cinfo.next_scanline * cinfo.image_width * 3];
            jpeg_write_scanlines(&cinfo, row_pointer, 1);
        }
        jpeg_finish_compress(&cinfo);

        // Check if the size is below the threshold
        if (jpeg_size <= quality_threshold) {
            break;
        }
        free(jpeg_data);
        // If the size is above the threshold, decrease quality by 10
        current_quality -= 10;
        jpeg_size = 0;  // Reset the size for the next iteration
        jpeg_mem_dest(&cinfo, &jpeg_data, &jpeg_size);  // Reset memory destination
    }

    jpeg_destroy_compress(&cinfo);

    if (current_quality < 10) {
        fprintf(stderr, "Error: JPEG quality reached 10, but the size is still above the threshold.\n");
        printf("Error: JPEG quality reached 10, but the size is still above the threshold.\n");
        free(jpeg_data);
        exit(1);
    }

    // Copy the JPEG data to the output buffer
    *size = jpeg_size;
    memcpy(outPutImage, jpeg_data, jpeg_size);

    // Clean up the memory buffer
    free(jpeg_data);
}

void decode_jpeg(char * JpegData, char * output, int imageSize) {
    // Assume you have received the JPEG data in a buffer

    // Initialize libjpeg structures
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    // Set the input buffer
    // Create a custom memory data source manager
    jpeg_mem_src(&cinfo, JpegData, imageSize);
    // Read the JPEG header
    jpeg_read_header(&cinfo, TRUE);
    // Start decompression
    jpeg_start_decompress(&cinfo);
    // Allocate memory for the pixel data
    int row_stride = cinfo.output_width * cinfo.output_components;
    char *pixelData = (char *)malloc(row_stride * cinfo.output_height);

    // Read scanlines and store them in the pixelData array
    char *row_pointer = pixelData;
    while (cinfo.output_scanline < cinfo.output_height) {
        jpeg_read_scanlines(&cinfo, (JSAMPARRAY)&row_pointer, 1);
        row_pointer += row_stride;
    }
    memcpy(output, pixelData, row_stride * cinfo.output_height);

    // Finish decompression
    jpeg_finish_decompress(&cinfo);
    // Cleanup
    jpeg_destroy_decompress(&cinfo);

    free(pixelData);
}