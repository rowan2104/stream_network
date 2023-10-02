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
    uint64_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *file_contents = malloc(file_size + 1);
    fread(file_contents, 1, file_size, file);
    file_contents[file_size] = '\0';
    fclose(file);
    return file_contents;
}

void writeStringToFile(const char *filename, const char *text) {
    FILE *file = fopen(filename, "a");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }
    fprintf(file, "%s\n", text);
    fclose(file);
}