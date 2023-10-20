#include <stdio.h>
#include <stdlib.h>


// Function to append a consumer to the list
void appendConsumer(struct consumer_list* list, struct consumer* cons) {
    struct consumer_node* newNode = malloc(sizeof(struct consumer_node));;
    newNode->cons = cons;
    newNode->next = NULL;
    if (list->head == NULL) {
        list->head = newNode;
    } else {
        struct consumer_node* current = list->head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = newNode;
    }
    list->size++;
}

// Function to remove a consumer at a given position
void removeConsumer(struct consumer_list* list, int position) {
    if (position < 0 || position >= list->size) {
        printf("Invalid position for removal\n");
        return;
    }

    if (position == 0) {
        struct consumer_node* temp = list->head;
        list->head = list->head->next;
        free(temp);
    } else {
        struct consumer_node* current = list->head;
        for (int i = 0; i < position - 1; i++) {
            current = current->next;
        }
        struct consumer_node* temp = current->next;
        current->next = temp->next;
        free(temp);
    }
    list->size--;
}

// Function to get a consumer at a given position
struct consumer* getConsumer(struct consumer_list* list, int position) {
    if (position < 0 || position >= list->size || list->head == NULL) {
        return NULL;
    }

    struct consumer_node* current = list->head;
    for (int i = 0; i < position; i++) {
        if (current == NULL) {
            return NULL;
        }
        current = current->next;
    }
    if (current == NULL || current->cons == NULL) {
        return NULL;
    }
    return current->cons;
}

// Function to get a consumer at a given position
int getConsumerPosition(struct consumer_list* list, struct consumer * target) {
    if (list->head == NULL) {
        return 0;
    }

    struct consumer_node* current = list->head;
    for (int i = 0; i < list->size; i++) {
        if (current == NULL) {
            return 0;
        }
        if (current->cons == target){
            return i;
        }
    }
    return 0;
}

void freeConsumerList(struct consumer_list* list) {
    struct consumer_node* current = list->head;
    while (current != NULL) {
        struct consumer_node* temp = current;
        current = current->next;
        free(temp);
    }
    list->head = NULL;
    list->size = 0;
}

