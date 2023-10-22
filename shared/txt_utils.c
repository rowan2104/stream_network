//
// Created by rowan on 14/10/2023.
//

#include <stdio.h>
#include <stdlib.h>

int readLineFromFile(const char *filename, int lineNumber, char *buffer, int bufferSize) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        return -1; // Error opening file
    }

    int totalLines = 0;
    int currentLine = 0;

    // Calculate the total number of lines in the file
    while (fgets(buffer, bufferSize, file) != NULL) {
        totalLines++;
    }

    // Rewind the file to the beginning
    rewind(file);

    if (totalLines == 0) {
        fclose(file);
        return 2; // File is empty
    }

    // Use modulo to wrap around the line number
    lineNumber = (lineNumber + totalLines) % totalLines;

    while (fgets(buffer, bufferSize, file) != NULL) {
        if (currentLine == lineNumber) {
            // Remove the trailing newline character (if it exists)
            int len = strlen(buffer);
            if (len > 0 && buffer[len - 1] == '\n') {
                buffer[len - 1] = '\0'; // Null-terminate the string
            }
            fclose(file);
            return 0; // Successfully read the specified line
        }
        currentLine++;
    }

    fclose(file);
    return 1; // Specified line not found
}

// Function to write/append a string to a text file
int writeToFile(const char *filename, const char *text, int append) {
    FILE *file;

    if (append)
        file = fopen(filename, "a"); // Append mode
    else
        file = fopen(filename, "w"); // Write mode

    if (file == NULL) {
        perror("Error opening file");
        return -1; // Error opening file
    }

    if (fprintf(file, "%s", text) < 0) {
        perror("Error writing to file");
        fclose(file);
        return -2; // Error writing to file
    }

    fclose(file);
    return 0; // Successfully wrote to file
}