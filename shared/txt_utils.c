//
// Created by rowan on 02/10/2023.
//
#include <stdio.h>
#include <stdlib.h>

char *readFileIntoString(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *file_contents = malloc(file_size + 1);
    size_t bytes_read = fread(file_contents, 1, file_size, file);
    file_contents[file_size] = '\0';
    fclose(file);
    return file_contents;
}
