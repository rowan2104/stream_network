//Created by Rowan Barr
#include <stdio.h>
#include <stdlib.h>


// Function to append a producer to the list
void appendProducer(struct producer_list* list, struct producer* prod) {
    struct producer_node* newNode = malloc(sizeof(struct producer_node));;
    newNode->prod = prod;
    newNode->next = NULL;
    if (list->head == NULL) {
        list->head = newNode;
    } else {
        struct producer_node* current = list->head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = newNode;
    }
    list->size++;
}

// Function to remove a producer at a given position
void removeProducer(struct producer_list* list, int position) {
    if (position < 0 || position >= list->size) {
        printf("Invalid position for removal\n");
        return;
    }

    if (position == 0) {
        struct producer_node* temp = list->head;
        list->head = list->head->next;
        free(temp);
    } else {
        struct producer_node* current = list->head;
        for (int i = 0; i < position - 1; i++) {
            current = current->next;
        }
        struct producer_node* temp = current->next;
        current->next = temp->next;
        free(temp);
    }
    list->size--;
}

// Function to get a producer at a given position
struct producer* getProducer(struct producer_list* list, int position) {
    if (position < 0 || position >= list->size || list->head == NULL) {
        return NULL;
    }

    struct producer_node* current = list->head;
    for (int i = 0; i < position; i++) {
        if (current == NULL) {
            return NULL;
        }
        current = current->next;
    }
    if (current == NULL || current->prod == NULL) {
        return NULL;
    }
    return current->prod;
}

// Function to get a consumer at a given position
int getProducerPosition(struct producer_list* list, struct producer * target) {
    if (list->head == NULL) {
        return 0;
    }

    struct producer_node* current = list->head;
    for (int i = 0; i < list->size; i++) {
        if (current == NULL) {
            return 0;
        }
        if (current->prod == target){
            return i;
        }
    }
    return 0;
}

void freeProducerList(struct producer_list* list) {
    struct producer_node* current = list->head;
    while (current != NULL) {
        struct producer_node* temp = current;
        current = current->next;
        free(temp);
    }
    list->head = NULL;
    list->size = 0;
}

