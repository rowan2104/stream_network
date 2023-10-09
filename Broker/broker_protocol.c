
void print_id(unsigned char * buf){
    for (int i = 0; i < 3; ++i) {
        printf("%02x", buf[i]);
    }
}

int search_producers_id(unsigned char newID[3], struct producer_list * prodList){
    int i = 0;
    int y = (newID[0] << 0) | (newID[1] << 8) | (newID[2] << 16);
    struct producer * temp;
    while (getProducer(prodList, i) != NULL){
        temp = getProducer(prodList, i);
        int x = (temp->id[0] << 0) | (temp->id[1] << 8) | (temp->id[2] << 16);
        if (x == y){
            return i;
        }
        i++;
    }
    return -1;
}


int search_consumers_ip(char * ip, struct consumer_list * consList){
    int i = 0;
    struct producer * temp;
    while (getConsumer(consList, i) != NULL){
        if (strcmp(ip, getConsumer(consList, i)->caddr.ipAddr) == 0){
            return i;
        }
        i++;
    }
    return -1;
}


int add_new_consumer(struct consumer_list * consList, address cons_addr){
    if (search_consumers_ip(cons_addr.ipAddr, consList) == -1){
        int lastPos = consList->size;
        struct consumer * temp = malloc(sizeof(struct consumer));
        memcpy(&temp->caddr, &cons_addr, sizeof(address));
        appendConsumer(consList, temp);
        return lastPos;
    }
    return -1;

};

int add_new_producer(unsigned char newID[3], struct producer_list * prodList, address prod_addr){
    if (search_producers_id(newID, prodList) == -1){
        int lastPos = prodList->size;
        struct producer * temp = malloc(sizeof(struct producer));
        memcpy(&temp->paddr, &prod_addr, sizeof(address));
        memcpy(temp->id, newID, 3);
        appendProducer(prodList, temp);
        return lastPos;
    }
    return -1;

};

int recv_prod_request_connect(unsigned char * buf, struct producer_list * prodList, address prod_addr){
    printf("Producer request to connect with id:");
    print_id(&buf[1]);
    printf("\n");
    int result = add_new_producer(&buf[1], prodList, prod_addr);
    if (result != -1){
        printf("added new producer succefully!\n");
        return result;
    } else {
        printf("failed to add new producer!\n");
        return -1;
    }

}

int recv_cons_request_connect(unsigned char * buf, struct consumer_list * consList, address cons_addr){
    printf("Consumer request to connect, %s:%hu\n", cons_addr.ipAddr, cons_addr.portNum);
    int result = add_new_consumer(consList, cons_addr);
    if (result != -1){
        printf("added new consumer succefully!\n");
        return result;
    } else {
        printf("failed to add new consumer!\n");
        return -1;
    }

}









