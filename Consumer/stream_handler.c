//Created by Rowan Barr
#include <stdio.h>
#include <stdlib.h>



// Function to append a stream to the list
void appendStream(struct stream_list* list, struct stream* data) {
    struct stream_node* newNode = malloc(sizeof(struct stream_node));
    newNode->data = data;
    newNode->next = NULL;
    if (list->head == NULL) {
        list->head = newNode;
    } else {
        struct stream_node* current = list->head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = newNode;
    }
    list->size++;
}

// Function to remove a stream at a given position
void removeStream(struct stream_list* list, int position) {
    if (position < 0 || position >= list->size) {
        printf("Invalid position for removal\n");
        return;
    }

    if (position == 0) {
        struct stream_node* temp = list->head;
        list->head = list->head->next;
        free(temp);
    } else {
        struct stream_node* current = list->head;
        for (int i = 0; i < position - 1; i++) {
            current = current->next;
        }
        struct stream_node* temp = current->next;
        current->next = temp->next;
        free(temp);
    }
    list->size--;
}

// Function to get a stream at a given position
struct stream* getStream(struct stream_list* list, int position) {
    if (position < 0 || position >= list->size || list->head == NULL) {
        return NULL;
    }

    struct stream_node* current = list->head;
    for (int i = 0; i < position; i++) {
        if (current == NULL) {
            return NULL;
        }
        current = current->next;
    }
    if (current == NULL || current->data == NULL) {
        return NULL;
    }
    return current->data;
}

// Function to get a stream position in the list
int getStreamPosition(struct stream_list* list, struct stream* target) {
    if (list->head == NULL) {
        return -1;
    }

    struct stream_node* current = list->head;
    int position = 0;
    while (current != NULL) {
        if (current->data == target) {
            return position;
        }
        current = current->next;
        position++;
    }
    return -1;
}

void freeStreamList(struct stream_list* list) {
    struct stream_node* current = list->head;
    while (current != NULL) {
        struct stream_node* temp = current;
        current = current->next;
        free(temp);
    }
    list->head = NULL;
    list->size = 0;
}