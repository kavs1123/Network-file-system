#include "defs.h"

naming_server* global_ns;
Cachetable* global_cache;
int cache_entries;

char* strdup(const char *src) {
    // Check if the source string is NULL
    if (src == NULL) {
        return NULL;
    }

    // Allocate memory for the new string
    char *dest = (char *)malloc(strlen(src) + 1);

    // Check if memory allocation was successful
    if (dest == NULL) {
        return NULL;
    }

    // Copy the content of the source string to the new string
    strcpy(dest, src);

    return dest;
}

naming_server* init_ns(){
    naming_server* ns = (naming_server*)malloc(sizeof(naming_server));
    ns->number_of_ss=0;
    ns->head_ptr_to_list= NULL;
    ns->tail_ptr_to_list=NULL;
    ns->addr_len = sizeof(struct sockaddr_in);
}

void addNode(Data_LL **head, const char *name) {
    Data_LL *newNode = (Data_LL *)malloc(sizeof(Data_LL));
    if (newNode == NULL) {
        perror("[-]Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    strcpy(newNode->name, name);
    newNode->next = *head;
    *head = newNode;
}

void printList(Data_LL *head, const char *prefix) {
    Data_LL *current = head;
    while (current != NULL) {
        printf("%s\n", current->name);
        current = current->next;
    }
}

void forward_to_ss(int server_port , char*ip , char*query, ll_of_ss*found_ss){
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        perror("Error creating socket");
    }

    // Set up server address struct
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    inet_pton(AF_INET, ip, &(server_addr.sin_addr));

    printf("Connecting to storage server %s:%d\n",ip, server_port);

    // Connect to server
    if (connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error connecting to storage server");
    } else {
        printf("Connected to  storage server %s:%d\n",ip, server_port);
    }
    
    char buffer[1024];
    strcpy(buffer,query);
    // printf("Buffer: %s\n", buffer);
    send(socket_fd,buffer,1024,0);

    char*copy = strdup(query);
    char*token = strtok(copy," ");

    

    if(strcmp(token,"CREATE")==0){
        Data_LL *head_2 = NULL;
        char buffer[1024];
        bzero(buffer, 1024);
        int bytes_rec =0;

        while ((bytes_rec = recv(socket_fd, buffer, sizeof(buffer), 0) )> 0) {
            buffer[strlen(buffer)]='\0';
        // printf("%s\n",buffer);
            if (strcmp(buffer, "END") == 0) {
                break;
            }
            addNode(&head_2, buffer);
            bzero(buffer, 1024);
        }

        found_ss->ss.head_ptr_for_paths = head_2;

        Data_LL * current = head_2;
        while(current != NULL)
        {
            printf("%s\n",current->name);
            current = current->next;
        }


        bzero(buffer, 1024);
        strcpy(buffer, "EXIT");
        send(socket_fd, buffer, 1024, 0);
    }
    
    else if(strcmp(token,"DELETE")==0){
        Data_LL *head_2 = NULL;
        char buffer[1024];
        bzero(buffer, 1024);
        int bytes_rec =0;

        while ((bytes_rec = recv(socket_fd, buffer, sizeof(buffer), 0) )> 0) {
            buffer[strlen(buffer)]='\0';
            //printf("%s\n",buffer);
            if (strcmp(buffer, "END") == 0) {
                break;
            }
            addNode(&head_2, buffer);
            bzero(buffer, 1024);
        }

        found_ss->ss.head_ptr_for_paths = head_2;

        Data_LL * current = head_2;

        while(current != NULL)
        {
            printf("%s\n",current->name);
            current = current->next;
        }

        bzero(buffer, 1024);

        strcpy(buffer, "EXIT");

        send(socket_fd, buffer, 1024, 0);

    }

    close(socket_fd);


}

char* getSubpath(const char* filepath) {
    char* subpath = strdup(filepath);  // Copy the filepath to avoid modifying the original
    
    char* lastSlash = strrchr(subpath, '/');

    if (lastSlash != NULL) {
        *(lastSlash) = '\0';  // Null-terminate the string after the last '/'
    }

    else{
        subpath = NULL;
    }

    return subpath;
}



void start_ns(naming_server*ns){

    if ((ns->server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    if ((ns->client_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Initialize the server address struct


    ns->ss_addr.sin_family = AF_INET;
    ns->ss_addr.sin_addr.s_addr = INADDR_ANY;
    ns->ss_addr.sin_port = htons(NS_PORT_SS);

    ns->client_addr.sin_family = AF_INET;
        // Check for activity on socket 1
    ns->client_addr.sin_addr.s_addr = INADDR_ANY;
    ns->client_addr.sin_port = htons(NS_PORT_CLIENT);



    // Bind the socket to the specified address and port
    if (bind(ns->server_socket, (struct sockaddr*)&ns->ss_addr, sizeof(ns->ss_addr)) == -1) {
        perror("Socket bind failed");
        exit(EXIT_FAILURE);
    }

    if (bind(ns->client_socket, (struct sockaddr*)&ns->client_addr, sizeof(ns->client_addr)) == -1) {
        perror("Socket bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(ns->server_socket, 1) < 0 || listen(ns->client_socket,1)<0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    FD_ZERO(&ns->readfds);

    // Add the sockets to the set
    FD_SET(ns->client_socket, &ns->readfds);
    FD_SET(ns->server_socket, &ns->readfds);

    ns->max_fd = (ns->client_socket > ns->server_socket) ? ns->client_socket : ns->server_socket;

    printf("Server listening on port %d.and %d..\n", NS_PORT_SS,NS_PORT_CLIENT);


}
int handle_client_req(int* client_port, int*flag, char* ip , char*query){

    ll_of_ss* found_ss=NULL;
    int ack_for_client=1;

    int* server_port =(int*)malloc(sizeof(int));


    char* str1 = strdup(query);
    char*str2 = strdup(query);
    char*token = strtok(str1, " ");

    char*path_of_file;
    while(token!=NULL){
        path_of_file = token;
        token = strtok(NULL," ");  
    }

    char*token2 = strtok(str2," ");
    char*command = token2;


    if(strcmp(command,"READ")==0){
        found_ss = search_all_ss(client_port,ip,path_of_file);
        //printf("%d\n",found_ss);
        if(found_ss==NULL){
            ack_for_client=0;
            return ack_for_client;
        }
        *flag=1;
        return ack_for_client;

    }
    else if(strcmp(command,"WRITE")==0){
        
        found_ss = search_all_ss(client_port,ip,path_of_file);
        if(found_ss==NULL){
            ack_for_client=0;
            return ack_for_client;
        }
        *flag=1;
        return ack_for_client;

    }
    else if(strcmp(command,"RETRIEVE")==0){
        found_ss = search_all_ss(client_port,ip,path_of_file);
        if(found_ss==NULL){
            ack_for_client=0;
            return ack_for_client;
        }
        *flag=1;
        return ack_for_client;

    }
    else if(strcmp(command,"CREATE")==0){
        char* subpath = getSubpath(path_of_file);
        
        if(subpath ==NULL){
            forward_to_ss(global_ns->head_ptr_to_list->ss.server_port,global_ns->head_ptr_to_list->ss.ip,query,global_ns->head_ptr_to_list);
            return ack_for_client;
        }

        found_ss = search_all_ss_2(server_port,ip,subpath);

        if(found_ss==NULL){
            ack_for_client=0;
            return ack_for_client;
        }
        *flag=0;

        forward_to_ss(*server_port,ip,query,found_ss);
        


        //ackno bhere

        
        
        return ack_for_client;

    }
    else if(strcmp(command,"DELETE")==0){
        found_ss = search_all_ss_2(server_port,ip,path_of_file);

        if(found_ss==NULL){
            ack_for_client=0;
            return ack_for_client;
        }

        *flag=0;

        forward_to_ss(*server_port,ip,query,found_ss);

        for(int i=0;i<=cache_entries;i++){
            if(strcmp(global_cache[i].name,path_of_file)==0){
                global_cache[i].client_port=0;
                (global_cache[i].ip)[0]='\0';
                global_cache[i].name[0]='\0';
                global_cache[i].server_port=0;
            }
        }

        return ack_for_client;

    }
    else if(strcmp(command,"COPY")==0){

        char* format = strtok(NULL," ");

        if(strcmp(format, "FILE") == 0)
        {
            int* server_port_src =(int*)malloc(sizeof(int));
            int* server_port_dest =(int*)malloc(sizeof(int));
            char ip_src[20];char ip_dest[20];

            char* filename = strtok(NULL," ");
            char* dest_path = strtok(NULL," ");

            search_all_ss_2(server_port_src,ip_src,filename);
            search_all_ss_2(server_port_dest,ip_dest,dest_path);

            int port = 5691;

            char send_text[1024];
            strcpy(send_text, "READ ");
            strcat(send_text, filename);

            // connect to src
            int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
            if (socket_fd == -1) {
                perror("Error creating socket");
            }

            // Set up server address struct
            struct sockaddr_in server_addr;
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(*server_port_src);
            inet_pton(AF_INET, ip, &(server_addr.sin_addr));

            printf("Connecting to storage server %s:%d\n",ip_src, *server_port_src);

            // Connect to server
            if (connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
                perror("Error connecting to storage server");
            } else {
                printf("Connected to  storage server %s:%d\n",ip_src, *server_port_src);
            }

            send(socket_fd, send_text, 1024, 0);

            // open a new temp file
            FILE* fp;
            fp = fopen("temp.txt", "w");

            // keep recieving till its exit and write it into temp.txt
            char buffer[1024];
            bzero(buffer, 1024);
            int bytes_rec =0;
            while ((bytes_rec = recv(socket_fd, buffer, sizeof(buffer), 0) )> 0) {
                buffer[strlen(buffer)]='\0';
                printf("%s\n",buffer);
                if (strcmp(buffer, "END") == 0) {
                    break;
                }
                char echo[2000];
                strcpy(echo, "echo '");
                strcat(echo, buffer);
                strcat(echo, "' >> temp.txt");
                system(echo);
                bzero(echo, 2000);
                bzero(buffer, 1024);
            }
            fclose(fp);
            bzero(buffer, 1024);
            strcpy(buffer, "EXIT");
            send(socket_fd, buffer, 1024, 0);

            close(socket_fd);


            // connect to dest
            socket_fd = socket(AF_INET, SOCK_STREAM, 0);
            if (socket_fd == -1) {
                perror("Error creating socket");
            }

            // Set up server address struct
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(*server_port_dest);
            inet_pton(AF_INET, ip, &(server_addr.sin_addr));

            printf("Connecting to storage server %s:%d\n",ip_dest, *server_port_dest);

            // Connect to server
            if (connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
                perror("Error connecting to storage server");
            } else {
                printf("Connected to  storage server %s:%d\n",ip_dest, *server_port_dest);
            }
            int size;

            struct stat fileInfo;
            if (stat(filename, &fileInfo) == 0)
            {
                size = fileInfo.st_size;
            }

            // send the file
            char* send_text2 = (char*)malloc(sizeof(char)*size);
            
            fp = fopen("temp.txt", "r");
            fread(send_text2, sizeof(char), size, fp);
            fclose(fp);

            char send_text3[1024];
            strcpy(send_text3, "COPY ");
            strcat(send_text3, dest_path);
            strcat(send_text3, " ");
            strcat(send_text3, size);

            send(socket_fd, send_text3, 1024, 0);

            char buffi[1024];
            bzero(buffi, 1024);
            recv(socket_fd, buffi, 1024, 0);

            close(socket_fd);





            

        }

        return ack_for_client;

    }
    return ack_for_client;
}

void connect_to_ss(naming_server* ns,int new_socket){
    
    // printf("hello1\n");
    ll_of_ss* new_node = (ll_of_ss*)malloc(sizeof(ll_of_ss));
    new_node->next_ss=NULL;


    recv(new_socket, &(new_node->ss.server_port), sizeof(int), 0);
    recv(new_socket,&(new_node->ss.client_port),sizeof(int),0);

    // printf("server port %d\n",new_node->ss.server_port);
    // printf("client port %d\n",new_node->ss.client_port);



    
    
    inet_ntop(AF_INET, &(ns->connection_addr.sin_addr), new_node->ss.ip, 20);

    Data_LL *head = NULL;
    char buffer[1024];
    bzero(buffer, 1024);
    int bytes_rec =0;
    while ((bytes_rec = recv(new_socket, buffer, sizeof(buffer), 0) )> 0) {
        buffer[strlen(buffer)]='\0';
        // printf("%s\n",buffer);
        if (strcmp(buffer, "END") == 0) {
            break;
        }
        addNode(&head, buffer);
        bzero(buffer, 1024);
    }

    new_node->ss.head_ptr_for_paths=head;

    
    if(ns->head_ptr_to_list==NULL){
        ns->head_ptr_to_list=new_node;
        ns->tail_ptr_to_list=new_node;

    }
    else{
        ns->tail_ptr_to_list->next_ss=new_node;
        ns->tail_ptr_to_list=new_node;
    }
    ns->number_of_ss++;


}

void* handle_client(void* arg){


    int new_socket = *((int *)arg);
    //printf("inside handle client and new sock is:%d\n",new_socket);
    int bytes_rec;
    char buffer[MAX_COM_LEN] = {0};

    int connection_ack =1;

    send(new_socket,&connection_ack,sizeof(int),0);








    while ((bytes_rec = recv(new_socket, buffer, sizeof(buffer),0)) > 0) {
        //printf("bytes _rec %d\n",bytes_rec);
        int flag;
        int acknowledgement=0;
        buffer[strlen(buffer)]='\0';
        // Handle the received data (you can replace this with your own logic)
        printf("Received: %s\n", buffer);
        //printf("hello_my_friend\n");
        
        Namingserver_to_client* NSTC = (Namingserver_to_client*)malloc(sizeof(Namingserver_to_client));

        if(strcmp(buffer,"EXIT")==0){
            printf("Closing Client %d\n",new_socket);
            break;
        }
        else{
            acknowledgement=handle_client_req(&NSTC->port_num,&flag,NSTC->ip,buffer);
        }

        if(flag==0){
            if(send(new_socket,&acknowledgement,sizeof(acknowledgement),0)<0)
            {
                printf("Send error\n");

            }
        }
        else{
            send(new_socket,&NSTC->port_num,sizeof(int),0);
            send(new_socket,NSTC->ip,20,0);
        }

        memset(buffer, 0, sizeof(buffer));
    }

    close(new_socket);


}
int cache_hit_1(int* clientPort,char*ip,char*path_of_file){
    for(int i=0;i<=cache_entries;i++){
        if(strcmp(path_of_file,global_cache[i].name)==0){
            *clientPort = global_cache[i].client_port;
            strcpy(ip,global_cache[i].ip);
            
            return 1;
        }



    }
    return 0;
}

int cache_hit_2(int* serverPort , char*ip,char*path_of_file){
    for(int i=0;i<=cache_entries;i++){
        if(strcmp(path_of_file,global_cache[i].name)==0){
            *serverPort = global_cache[i].server_port;
            strcpy(ip,global_cache[i].ip);
            
            return 1;
        }



    }
    return 0;

}

ll_of_ss* search_all_ss(int*client_port,char*ip,char*path_of_file){

    naming_server* ns = global_ns;
    ll_of_ss* head = ns->head_ptr_to_list;
    int found =0;
    char* check_path;
    ll_of_ss* found_ss=NULL;

    if(cache_hit_1(client_port,ip,path_of_file)){
        ll_of_ss* temp = global_ns->head_ptr_to_list;
        while(temp!=NULL){
            if(temp->ss.client_port==*client_port){
                found_ss= temp;
                return found_ss;
            }
            temp=temp->next_ss;
        }
        return found_ss;

        
    }
    while(head!=NULL){
        Data_LL* head_of_paths = head->ss.head_ptr_for_paths;
        while(head_of_paths!=NULL){
            check_path= head_of_paths->name;
            if(strcmp(check_path,path_of_file)==0){
                found_ss = head;
                *client_port = head->ss.client_port;
                strcpy(ip,head->ss.ip);
                // add to cache

                for (int i = cache_entries+1; i > 0; --i) {
                    if(i==CACHE_LEN){
                        continue;
                    }
                    global_cache[i] = global_cache[i - 1];
                }

                global_cache[0].client_port=head->ss.client_port;
                global_cache[0].server_port=head->ss.server_port;
                strcpy(global_cache[0].ip,head->ss.ip);
                strcpy(global_cache[0].name,head_of_paths->name);

                if(cache_entries<CACHE_LEN-1){
                    cache_entries++;
                }
                //
                //
                found=1;
                return found_ss;
            }
            head_of_paths= head_of_paths->next;



        }
        
        head= head->next_ss;
    }
    if(found==0){
        printf("No such item found!\n");
        //error_code
        *client_port=0;
        ip = NULL;
        return NULL;
    }
}

ll_of_ss* search_all_ss_2(int*server_port,char*ip,char*path_of_file){
    naming_server* ns = global_ns;
    ll_of_ss* head = ns->head_ptr_to_list;
    int found =0;
    ll_of_ss* found_ss=NULL;
    char* check_path;
    if(cache_hit_2(server_port,ip,path_of_file)){
        ll_of_ss* temp = global_ns->head_ptr_to_list;
        while(temp!=NULL){
            if(temp->ss.server_port==*server_port){
                found_ss= temp;
                return found_ss;
            }
            temp=temp->next_ss;
        }
        return found_ss;
    
    }

    while(head!=NULL){
        Data_LL* head_of_paths = head->ss.head_ptr_for_paths;
        while(head_of_paths!=NULL){
            check_path= head_of_paths->name;
            if(strcmp(check_path,path_of_file)==0){
                found_ss = head;
                *server_port=head->ss.server_port;;
                // printf("%d\n",*server_port);
                strcpy(ip,head->ss.ip);

               
                // Shift existing elements to make room for the new data
                for (int i = cache_entries+1; i > 0; --i) {
                    if(i==CACHE_LEN){
                        continue;
                    }
                    global_cache[i] = global_cache[i - 1];
                }

                global_cache[0].client_port=head->ss.client_port;
                global_cache[0].server_port=head->ss.server_port;
                strcpy(global_cache[0].ip,head->ss.ip);
                strcpy(global_cache[0].name,head_of_paths->name);

                if(cache_entries<CACHE_LEN-1){
                    cache_entries++;
                }



                // Insert the new data at index 0
                




                found=1;
                
                return found_ss;
            }
            head_of_paths= head_of_paths->next;



        }
        if(found==1){
            break;
        }
        head= head->next_ss;
    }
    if(found==0){
        printf("No such item found!\n");
        //error_code
        *server_port=0;
        ip = NULL;
        return found_ss;
    }
}

int main(){

    naming_server* ns = init_ns();
    Cachetable* cache = (Cachetable*)malloc(sizeof(Cachetable)*CACHE_LEN);
    global_cache = cache;
    global_ns = ns;
    start_ns(ns);
    int new_socket;
    cache_entries=0;
    
    // printf("hello1\n");
    while (1) {

        ns->activity = select(ns->max_fd + 1, &ns->readfds, NULL, NULL, NULL);
        
        if (ns->activity < 0) {
            perror("Select error");
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(ns->server_socket, &ns->readfds)) {

            
            if ((new_socket=accept(ns->server_socket, (struct sockaddr*)&ns->connection_addr, (socklen_t*)&ns->addr_len)) < 0) {
                perror("Accept failed");
                exit(EXIT_FAILURE);
            }
            // print new_socket
            // printf("%d\n", new_socket);
            

            printf("Server connection accepted on port %d\n", NS_PORT_SS);

            connect_to_ss(ns,new_socket);


            close(new_socket);

            
        }

        if (FD_ISSET(ns->client_socket, &ns->readfds)) {
        
            if ((new_socket = accept(ns->client_socket, (struct sockaddr*)&ns->connection_addr, (socklen_t*)&ns->addr_len)) < 0) {
                perror("Accept failed");
                exit(EXIT_FAILURE);
            }

            pthread_t thread_id;




            printf("Client connection accepted on port %d\n", NS_PORT_CLIENT);

            int *client_socket_ptr = (int *)malloc(sizeof(int));
            *client_socket_ptr = new_socket;

            if (pthread_create(&thread_id, NULL, handle_client, (void *)client_socket_ptr) < 0) {
                perror("Thread creation failed");
                exit(EXIT_FAILURE);
            }
            // printf("%d\n",new_socket);

            // Detach the thread to allow it to run independently
            pthread_detach(thread_id);

            
            //close(new_socket);
        }

        FD_ZERO(&ns->readfds);
        FD_SET(ns->server_socket, &ns->readfds);
        FD_SET(ns->client_socket, &ns->readfds);
    }




}