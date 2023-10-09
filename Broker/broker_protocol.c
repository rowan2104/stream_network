


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

int add_new_producer(unsigned char newID[3], struct producer_list * prodList, address prod_addr){
    if (search_producers_id(newID, prodList) == -1){
        int lastPos = prodList->size;
        struct producer * temp = malloc(sizeof(struct producer));
        temp->paddr = prod_addr;
        memcpy(temp->id, newID, 3);
        appendProducer(prodList, temp);
        return lastPos;
    }
    return -1;

};

void recv_prod_request_connect(unsigned char * buf, struct producer_list * prodList, address prod_addr){
    printf("Producer request to connect with id:");
    for (int i = 0; i < 3; ++i) {
        printf("%02x", buf[i+1]);
    }
    printf("\n");
    if (add_new_producer(&buf[1], prodList, prod_addr) != -1){
        printf("added new producer succefully!\n");
    } else {
        printf("failed to add new producer!\n");
    }

}


void send_request_connect(unsigned char * buf){
    printf("Producer request to connect with id:");
    for (int i = 0; i < 3; ++i) {
        printf("%02x", buf[i+1]);
    }
    printf("\n");
}

