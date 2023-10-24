
void print_id(unsigned char * buf){
    for (int i = 0; i < 3; ++i) {
        printf("%02x", buf[i]);
    }
}

void print_id2(unsigned char * buf){
    for (int i = 0; i < 4; ++i) {
        printf("%02x", buf[i]);
    }
}

struct producer * search_producers_id(unsigned char newID[3], struct producer_list * prodList){
    int i = 0;
    int y = (newID[0] << 0) | (newID[1] << 8) | (newID[2] << 16);
    struct producer * temp;
    while (getProducer(prodList, i) != NULL){
        temp = getProducer(prodList, i);
        int x = (temp->id[0] << 0) | (temp->id[1] << 8) | (temp->id[2] << 16);
        if (x == y){
            return temp;
        }
        i++;
    }
    return NULL;
}

struct stream * search_stream_id(unsigned char newID[4], struct stream_list * strmList){
    int i = 0;
    struct stream * temp;
    while (getStream(strmList, i) != NULL){
        temp = getStream(strmList, i);
        if (memcmp(newID, temp->streamID, 4) == 0){
            return temp;
        }
        i++;
    }
    return NULL;
}


struct consumer * search_consumers_ip(char * ip, struct consumer_list * consList){
    int i = 0;
    struct producer * temp;
    while (getConsumer(consList, i) != NULL){
        if (strcmp(ip, getConsumer(consList, i)->caddr.ipAddr) == 0){
            return getConsumer(consList, i);
        }
        i++;
    }
    return NULL;
}

struct producer * search_producer_ip(char * ip, struct producer_list * prodList){
    int i = 0;
    struct producer * temp;
    while (getProducer(prodList, i) != NULL){
        if (strcmp(ip, getProducer(prodList, i)->paddr.ipAddr) == 0){
            return getProducer(prodList, i);
        }
        i++;
    }
    return NULL;
}

struct producer * search_producer_streamid(unsigned char newID[4], struct producer_list * prodList){
    int i = 0;
    struct producer * temp;
    while (getProducer(prodList, i) != NULL){
        temp = getProducer(prodList, i);
        for (int j = 0; j < temp->myStreams->size; ++j) {
            struct stream * x = getStream(temp->myStreams, j);
            if (newID[0] == x->streamID[0] && newID[1] == x->streamID[1] && newID[2] == x->streamID[2] && newID[3] == x->streamID[3]){
                return temp;
            }
        }

        i++;
    }
    return NULL;
}

int add_new_consumer(struct consumer_list * consList, address cons_addr){
    if (search_consumers_ip(cons_addr.ipAddr, consList) == NULL){
        int lastPos = consList->size;
        struct consumer * temp = malloc(sizeof(struct consumer));
        temp->caddr.ipAddr = strdup(cons_addr.ipAddr);
        temp->caddr.portNum = cons_addr.portNum;
        appendConsumer(consList, temp);
        return lastPos;
    }
    return -1;
}

int add_new_producer(unsigned char newID[3], struct producer_list * prodList, address prod_addr){
    if (search_producers_id(newID, prodList) == NULL){
        int lastPos = prodList->size;
        struct producer * temp = malloc(sizeof(struct producer));
        temp->paddr.ipAddr = strdup(prod_addr.ipAddr);
        temp->paddr.portNum = prod_addr.portNum;
        temp->name = malloc(sizeof(char)*7);
        sprintf(temp->name, "%02x%02x%02x", newID[0], newID[1], newID[2]);
        temp->name[6] = 0;
        memcpy(temp->id, newID, 3);
        temp->myStreams = malloc(sizeof(struct stream_list));
        temp->myStreams->size = 0;
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


struct stream *  recv_request_create_stream(unsigned char * buf, struct producer_list * prodList){
    struct producer * currentProd = search_producers_id(&buf[1], prodList);
    if (currentProd == NULL){return NULL;}
    struct stream * newStream = malloc(sizeof(struct stream));
    memcpy(newStream->streamID, &buf[1], 4);
    int header = 5;
    memcpy(newStream->name, currentProd->name, 6);
    snprintf(&newStream->name[6], 3, "%02x", buf[4]);
    newStream->type = (buf[0] & (~TYPE_MASK));
    newStream->creator = currentProd;
    newStream->subscribers = malloc(sizeof(struct consumer_list));
    newStream->subscribers->head = NULL;
    newStream->subscribers->size = 0;
    if (newStream->type & VIDEO_BIT){
        memcpy(&newStream->vWidth, &buf[header], 2);
        memcpy(&newStream->vHeight, &buf[header+2], 2);
        printf("storing video resolution: %hu x %hu\n",newStream->vWidth,newStream->vHeight);
        header += 4;
    }
    return newStream;
}

int recv_request_delete_stream(unsigned char * buf, struct producer_list * prodList){
    struct producer * currentProd = search_producers_id(&buf[1], prodList);
    if (currentProd == NULL){return 1;}
    unsigned char id[4];
    memcpy(id, &buf[1], 4);
    struct stream * strm = search_stream_id(id, currentProd->myStreams);
    if (strm != NULL) {
        freeConsumerList(strm->subscribers);
        removeStream(currentProd->myStreams, getStreamPosition(currentProd->myStreams, strm));
    } else {
        printf("Stream not found!\n");
    }
    return 0;
}


int send_list_stream(unsigned char * buf, struct producer_list * prodList){
    int length = 5;
    uint32_t num_of_streams = 0;
    const char sizeStreamData = 5;
    const unsigned char requestType = buf[0] & (~TYPE_MASK);
    struct producer * current_prod;
    for (int i = 0; i < prodList->size; ++i) {
        current_prod = getProducer(prodList, i);
        if ((current_prod->myStreams != NULL))
        {
            for (int j = 0; j < current_prod->myStreams->size; ++j) {
                if ((getStream(current_prod->myStreams, j)->type & requestType) != 0) {
                    struct stream * strm = getStream(current_prod->myStreams, j);
                    memcpy(&buf[5 + (sizeStreamData * num_of_streams)], &strm->streamID, 4);
                    buf[5 + (sizeStreamData * num_of_streams) + 4] = strm->type;
                    num_of_streams++;
                    length += sizeStreamData;
                }
            }
        }
    }
    buf[0] = CONTROL_LIST_STREAM | requestType;
    memcpy(&buf[1], &num_of_streams, 4);
    return length;
}

int recv_req_stream_subscribe(unsigned char * buf, struct consumer * requester, struct producer_list * prodList){
    struct producer * current_producer;
    unsigned char packet_id[4];
    memcpy(packet_id, &buf[1], 4);
    current_producer = search_producers_id(&buf[1], prodList);
    if (current_producer == NULL){
        printf("Streamer not found!\n");
        return -1;
    }
    struct stream * strm = search_stream_id(&buf[1],current_producer->myStreams);
    if (strm == NULL){
        printf("Stream not found!\n");
        return -1;
    }
    if (search_consumers_ip(requester->caddr.ipAddr, strm->subscribers) == NULL) {
        appendConsumer(strm->subscribers, requester);
        printf("Stream %s now has subscriber %s:%hu\n", strm->name,
               requester->caddr.ipAddr, requester->caddr.portNum);
        char types = strm->type;
        printf("Content types: ");
        if (types & AUDIO_BIT) { printf("a"); }
        if (types & VIDEO_BIT) { printf("v"); }
        if (types & TEXT_BIT) { printf("t"); }
        printf("\n");
        printf("Sending Confirmation\n");
        buf[0] = CONTROL_SUBSCRIBE;
        buf[0] = buf[0] | types;
        memcpy(&buf[1], strm->streamID, 4);
        int header = 5;
        if (strm->type & VIDEO_BIT) {
            memcpy(&buf[header], &strm->vWidth, 2);
            memcpy(&buf[header + 2], &strm->vHeight, 2);
            printf("sending video resolution: %hu x %hu\n",strm->vWidth,strm->vHeight);
            header += 4;
        }
        return header;
    }else {
        printf("Consumer already subscribed!\n");
    }

    buf[0] == ERROR;
    memset(&buf[1],0,3);
    return 4;
}



int recv_req_stream_unsubscribe(unsigned char * buf, struct consumer * requester, struct producer_list * prodList){
    unsigned char packet_id[4];
    memcpy(packet_id, &buf[1], 4);
    struct producer * current_producer = search_producers_id(packet_id, prodList);
    if (current_producer == NULL){
        printf("ERROR, STREAMER %02x%02x%02x not found!\n",packet_id[0],packet_id[1],packet_id[2]);
        buf[0] == ERROR;
        memset(&buf[1],0,3);
        return 4;
    }
    struct stream * strm = search_stream_id(packet_id, current_producer->myStreams);
    if (strm == NULL){
        printf("ERROR, STREAM %02x%02x%02x%02x not found!\n",packet_id[0],packet_id[1],packet_id[2],packet_id[3]);
        buf[0] == ERROR;
        memset(&buf[1],0,3);
        return 4;
    }

    if (search_consumers_ip(requester->caddr.ipAddr, strm->subscribers) != NULL) {
        removeConsumer(strm->subscribers, getConsumerPosition(strm->subscribers, requester));
        printf(" subscriber %s:%hu for Stream %s now has unsubscribed\n", requester->caddr.ipAddr, requester->caddr.portNum,
               strm->name);
        printf("Sending Confirmation\n");
        buf[0] = CONTROL_UNSUBSCRIBE;
        memcpy(&buf[1], strm->streamID, 4);
        int header = 5;
        return header;
    }else {
        printf("Consumer is not subscribe!\n");
    }

    buf[0] == ERROR;
    memset(&buf[1],0,3);
    return 4;
}


