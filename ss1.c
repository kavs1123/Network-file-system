
#include "defs.h"

typedef struct Info
{
    char ip[10];
    int port;
    int clientPort;
} Info;

struct Node
{
    char name[1024];
    struct Node *next;
};

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int writer = 0;

void addNode(Data_LL **head, const char *name)
{
    Data_LL *newNode = (Data_LL *)malloc(sizeof(Data_LL));
    if (newNode == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    strcpy(newNode->name, name);
    newNode->next = *head;
    *head = newNode;
}

void printList(Data_LL *head, const char *prefix)
{
    Data_LL *current = head;
    while (current != NULL)
    {
        printf("%s%s\n", prefix, current->name);
        current = current->next;
    }
}

void listFilesRecursively(const char *basePath, Data_LL **head, const char *prefix)
{
    char path[1000];
    struct dirent *entry;
    DIR *dir = opendir(basePath);

    if (!dir)
    {
        return;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
        {
            sprintf(path, "%s/%s", basePath, entry->d_name);

            char newPrefix[1000];
            sprintf(newPrefix, "%s%s/", prefix, entry->d_name);

            addNode(head, newPrefix);

            if (entry->d_type == DT_DIR)
            {
                listFilesRecursively(path, head, newPrefix);
            }
        }
    }

    closedir(dir);
}

void sendLinkedList(int sock, Data_LL *head)
{
    Data_LL *current = head;
    printf("Files in Storage Server: ");
    while (current != NULL)
    {
        send(sock, current->name, 1024, 0);
        printf("-: %s\n", current->name);
        current = current->next;
    }
    send(sock, "END", strlen("END"), 0);
}

void NScall(char *buffer, char *command)
{
    char *token = strtok(buffer, " ");
    int i = 0;
    //printf("buffer is %s \n", buffer);

    if (token == NULL)
    {
        printf("Invalid Command!\n");
        return;
    }
    if (strcmp(token, "CREATE") == 0)
    {
        token = strtok(NULL, " ");
        if (strcmp(token, "FILE") == 0)
        {
            token = strtok(NULL, " ");
            char *filename = token;
            // now command should be "touch filename"
            strcpy(command, "touch ");
            strcat(command, filename);
        }
        else if (strcmp(token, "FOLDER") == 0)
        {
            token = strtok(NULL, " ");
            char *foldername = token;
            // now command should be "mkdir foldername"
            strcpy(command, "mkdir ");
            strcat(command, foldername);
        }
    }
    else if (strcmp(token, "DELETE") == 0)
    {
        token = strtok(NULL, " ");
        if (strcmp(token, "FILE") == 0)
        {
            token = strtok(NULL, " ");
            char *filename = token;
            // now command should be "rm filename"
            strcpy(command, "rm ");
            strcat(command, filename);
        }
        else if (strcmp(token, "FOLDER") == 0)
        {
            token = strtok(NULL, " ");
            char *foldername = token;
            // now command should be "rm -r foldername"
            strcpy(command, "rm -r ");
            strcat(command, foldername);
        }
    }
    else if (strcmp(token, "READ") == 0)
    {
        token = strtok(NULL, " ");
        char *filename = token;
        // now command should be "cat filename"
        strcpy(command, "cat ");
        strcat(command, filename);
    }
    else if (strcmp(token, "WRITE") == 0)
    {

        // the command is WRITE "text is here" text.txt
        // convert this to echo "text is here" >> text.txt
        token = strtok(NULL, "\"");
        char *text = token;
        token = strtok(NULL, "\"");
        char *filename = token;
        token = strtok(NULL, " ");

        // now command should be "echo text >> filename"
        strcpy(command, "echo ");
        strcat(command, text);
        strcat(command, " >>");
        strcat(command, filename);
    }
    else if (strcmp(token, "RETRIEVE") == 0)
    {
        token = strtok(NULL, " ");
        char *filename = token;
        // now i need the size and permissions of the file

        strcpy(command, "stat ");
        strcat(command, filename);
    }
    else if(strcmp(token, "COPY") == 0)
    {
        token = strtok(NULL, " ");
        char *filename = token;
        token = strtok(NULL, " ");
        char *size = token;
        strcpy(command, "cp ");
        strcat(command, filename);
        strcat(command, " ");
        strcat(command, size);
        printf("%s\n", command);
    }
    else
    {
        // set command to NULL
        strcpy(command, NULL);
        printf("Invalid Command!\n");
    }
}

void *handle_client_ss(void *arg)
{
    int newClientSock = *((int *)arg);
    char oper_buffer[1024];

    while (1)
    {
        int bytes_rec;
        bzero(oper_buffer, 1024);
        if ((bytes_rec = recv(newClientSock, oper_buffer, sizeof(oper_buffer), 0)) < 0)
        {
            printf("error");
        }
        oper_buffer[strlen(oper_buffer)] = '\0';


        // recv(newServerSock, oper_buffer, sizeof(oper_buffer), 0);
        // printf("%s\n", oper_buffer);

        if (strcmp(oper_buffer, "EXIT") == 0)
        {
            break;
        }
        char command[1024];
        NScall(oper_buffer, command);
        if (command[0] == 'c' && command[1] == 'a' && command[2] == 't')
        {

            while(writer==1){
                //do nothing
            }
            char read_buff[4096];
            strtok(command, " ");
            char *filename = strtok(NULL, " ");
            // open this file, and read into read_buff and send it to the client and recv acknowledgement
            // continue till the entire file is sent
            FILE *fp;
            fp = fopen(filename, "r");
            if (fp == NULL)
            {
                if (errno = ENOENT)
                {
                    strcpy(read_buff, "FnF");
                }
                else if (errno = EACCES)
                {
                    strcpy(read_buff, "PxD");
                }
                else
                {
                    strcpy(read_buff, "RxE");
                }
                send(newClientSock, read_buff, sizeof(read_buff), 0);
                memset(read_buff, 0, sizeof(read_buff));
                strcpy(read_buff, "EXIT");
                send(newClientSock, read_buff, sizeof(read_buff), 0);
                fclose(fp);
            }
            while (fgets(read_buff, sizeof(read_buff), fp) != NULL && fp != NULL)
            {
                send(newClientSock, read_buff, sizeof(read_buff), 0);
                bzero(read_buff, 4096);
                // recv(newClientSock, oper_buffer, sizeof(oper_buffer), 0);
            }
            memset(read_buff, 0, sizeof(read_buff));
            strcpy(read_buff, "EXIT");
            send(newClientSock, read_buff, sizeof(read_buff), 0);
            fclose(fp);
        }
        else if (command[0] == 's' && command[1] == 't' && command[2] == 'a' && command[3] == 't')
        {
            strtok(command, " ");
            char *filename = strtok(NULL, " ");
            struct stat fileInfo;
            if (stat(filename, &fileInfo) == 0)
            {
                send(newClientSock, &fileInfo, sizeof(fileInfo), 0);
                char buff_loli[1024];
                recv(newClientSock, buff_loli, 1024, 0);
                if (strcmp(buff_loli, "EXIT") == 0)
                {
                    break;
                }
            }
            else
            {
                // Print an error message if stat fails
                fileInfo.st_mode = -1;
                send(newClientSock, &fileInfo, sizeof(fileInfo), 0);
                perror("Error");
            }
        }
        else
        {
            int ack;

            if (command[0] == 'e' && command[1] == 'c' && command[2] == 'h' && command[3] == 'o')
            {
                pthread_mutex_lock(&mutex);
                writer=1;
                int ack = system(command);
                pthread_mutex_unlock(&mutex);
            }
            else{
                int ack = system(command);

            }
            if (errno == ENOENT)
            {
                ack = 404;
            }
            else if (errno == EACCES)
            {
                ack = 500;
            }
            send(newClientSock, &ack, sizeof(ack), 0);

        }

        //bzero(buffer, 1024);
        // strcpy(oper_buffer, "ACK");
        // send(newClientSock, oper_buffer, sizeof(oper_buffer), 0);
        // int ack = 1;
        // send(newClientSock, &ack, sizeof(ack), 0);
    }

    close(newClientSock);
    printf("Disconnected from the new client connection.\n");
}

int main()
{

    char *ip = "127.0.0.1";
    int port = 8080; // initialization
    int clientPort = 9080;
    int serverPort = 5691;
    char port_char[5];
    char clientPort_char[5];
    sprintf(port_char, "%d", port);
    sprintf(clientPort_char, "%d", clientPort);

    // make an info struct
    SS_to_NS info;
    // strcpy(info.ip, ip);
    info.port = port;
    info.clientPort = clientPort;

    int sock;
    struct sockaddr_in addr;
    int clientSock;
    struct sockaddr_in clientAddr;
    socklen_t addr_size;
    char buffer[1024];
    int n;
    // fd_set addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("[-]Socket error");
        exit(1);
    }

    clientSock = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSock < 0)
    {
        perror("[-]Client Socket error");
        exit(1);
    }

    memset(&addr, '\0', sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    memset(&clientAddr, '\0', sizeof(clientAddr));
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_port = htons(clientPort);
    clientAddr.sin_addr.s_addr = inet_addr(ip);

    if (bind(clientSock, (struct sockaddr *)&clientAddr, sizeof(addr)) < 0)
    {
        printf("%d\n", port);
        perror("[-]Bind error");
        exit(1);
    }

    connect(sock, (struct sockaddr *)&addr, sizeof(addr)); // connecting to namingserver

    printf("Connected to the server.\n");

    Data_LL *head = NULL;
    listFilesRecursively(".", &head, "");
    // go through the linked list and remove the last / from each name
    Data_LL *test = head;
    while (test != NULL)
    {
        char *lastChar = strrchr(test->name, '/');
        if (lastChar != NULL)
        {
            *lastChar = '\0';
        }
        test = test->next;
    }

    send(sock, &serverPort, sizeof(serverPort), 0);
    send(sock, &clientPort, sizeof(clientPort), 0);
    // send(sock, port_char, sizeof(port_char), 0);
    // send(sock, clientPort_char, sizeof(clientPort_char), 0);
    // Send the linked list to the server
    sendLinkedList(sock, head);

    // Clean up the linked list
    Data_LL *current = head;
    Data_LL *next;
    while (current != NULL)
    {
        next = current->next;
        free(current);
        current = next;
    }

    close(sock);
    close(clientSock);

    sleep(2);

    // now I need two sockets, one for the naming and one for client
    int serverSock = socket(AF_INET, SOCK_STREAM, 0);

    if (serverSock < 0)
    {
        perror("[-]Server Socket error");
        exit(1);
    }

    clientSock = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSock < 0)
    {
        perror("[-]Client Socket error");
        exit(1);
    }

    int enable = 1;

    struct sockaddr_in serverAddr;
    memset(&serverAddr, '\0', sizeof(clientAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
    serverAddr.sin_addr.s_addr = inet_addr(ip);

    // Set SO_REUSEADDR option for server socket
    if (setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    {
        perror("[-]Server setsockopt(SO_REUSEADDR) error");
        exit(1);
    }

    // Bind the server socket
    if (bind(serverSock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("[-]Server Bind error");
        exit(1);
    }


    // Set SO_REUSEADDR option for client socket
    if (setsockopt(clientSock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    {
        perror("[-]Client setsockopt(SO_REUSEADDR) error");
        exit(1);
    }

    // Bind the client socket
    if (bind(clientSock, (struct sockaddr *)&clientAddr, sizeof(clientAddr)) < 0)
    {
        perror("[-]Client Bind error");
        exit(1);
    }


    // now i need to listen on both the sockets and connect to the one where i get a response

    // Listen on the server socket
    if (listen(serverSock, 5) < 0)
    {
        perror("[-]Server Listen error");
        exit(1);
    }
    printf("Waiting for incoming connections on server port %d...\n", serverPort);
    // Listen on the client socket
    if (listen(clientSock, 5) < 0)
    {
        perror("[-]Client Listen error");
        exit(1);
    }
    printf("Waiting for incoming connections on client port %d...\n", clientPort);

    // Set up FD sets for select
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(serverSock, &readfds);
    FD_SET(clientSock, &readfds);
    int max_fd = (serverSock > clientSock) ? serverSock : clientSock;

    while (1)
    {
        // Use select to wait for activity on any of the sockets
        int activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);

        if (activity < 0)
        {
            perror("[-]Select error");
            exit(1);
        }

        if (FD_ISSET(serverSock, &readfds))
        {
            // Handle server socket connection
            addr_size = sizeof(serverAddr);
            int newServerSock = accept(serverSock, (struct sockaddr *)&serverAddr, &addr_size);
            if (newServerSock < 0)
            {
                perror("[-]Server Accept error");
                exit(1);
            }

            char oper_buffer[1024];
            while (1)
            {
                int bytes_rec;
                bzero(oper_buffer, 1024);
                if (bytes_rec = recv(newServerSock, oper_buffer, 1024, 0) < 0)
                {
                    printf("error");
                }
                oper_buffer[strlen(oper_buffer)] = '\0';
                printf("Command: %s\n", oper_buffer);

                // recv(newServerSock, oper_buffer, sizeof(oper_buffer), 0);

                if (strcmp(oper_buffer, "EXIT") == 0)
                {
                    break;
                }
                char command[1024];
                NScall(oper_buffer, command);
                // system(command);
                bzero(buffer, 1024);

                if (command == NULL)
                {
                    break;
                }
                // int check = 1;
                // send(newServerSock, &check, sizeof(check), 0);
                char *check = strtok(oper_buffer, " ");
                if (strcmp(check, "CREATE") == 0 || strcmp(check, "DELETE") == 0)
                {
                    if (strcmp(check, "DELETE") == 0)
                    {
                        system(command);
                    }
                    else
                    {
                        char *format = strtok(command, " ");
                        if (strcmp(format, "touch") == 0)
                        {
                            char *filename = strtok(NULL, " ");
                            FILE *fp;
                            fp = fopen(filename, "w");
                            fclose(fp);
                        }
                        else if (strcmp(format, "mkdir") == 0)
                        {
                            char *foldername = strtok(NULL, " ");
                            mkdir(foldername, 0777);
                        }
                    }
                    Data_LL *head = NULL;
                    listFilesRecursively(".", &head, "");
                    // go through the linked list and remove the last / from each name
                    Data_LL *test = head;
                    while (test != NULL)
                    {
                        char *lastChar = strrchr(test->name, '/');
                        if (lastChar != NULL)
                        {
                            *lastChar = '\0';
                        }
                        test = test->next;
                    }
                    sendLinkedList(newServerSock, head);

                    // Clean up the linked list
                    Data_LL *current = head;
                    Data_LL *next;
                    while (current != NULL)
                    {
                        next = current->next;
                        free(current);
                        current = next;
                    }
                    char buffi[1024];
                    recv(newServerSock, buffi, 1024, 0);
                    if (strcmp(buffi, "EXIT") == 0)
                    {
                        break;
                    }
                }
                else if (command[0] == 'c' && command[1] == 'a' && command[2] == 't')
                {
                    char read_buff[1024];
                    strtok(command, " ");
                    char *filename = strtok(NULL, " ");
                    // open this file, and read into read_buff and send it to the client and recv acknowledgement
                    // continue till the entire file is sent
                    FILE *fp;
                    fp = fopen(filename, "r");
                    if (fp == NULL)
                    {
                        if (errno = ENOENT)
                        {
                            strcpy(read_buff, "FnF");
                        }
                        else if (errno = EACCES)
                        {
                            strcpy(read_buff, "PxD");
                        }
                        else
                        {
                            strcpy(read_buff, "RxE");
                        }
                        printf("Cannot open file \n");
                        send(newServerSock, read_buff, sizeof(read_buff), 0);
                        memset(read_buff, 0, sizeof(read_buff));
                        strcpy(read_buff, "EXIT");
                        send(newServerSock, read_buff, sizeof(read_buff), 0);
                        fclose(fp);
                    }
                    while (fgets(read_buff, sizeof(read_buff), fp) != NULL && fp != NULL)
                    {
                        send(newServerSock, read_buff, sizeof(read_buff), 0);
                        bzero(read_buff, 1024);
                        printf("Sent: %s\n", read_buff);
                        // recv(newClientSock, oper_buffer, sizeof(oper_buffer), 0);
                    }
                    memset(read_buff, 0, sizeof(read_buff));
                    strcpy(read_buff, "END");
                    send(newServerSock, read_buff, sizeof(read_buff), 0);
                    fclose(fp);
                }
                else if(command[0] == 'c' && command[1] == 'p')
                {
                    char *filename = strtok(command, " ");
                    filename = strtok(NULL, " ");
                    char *size = strtok(NULL, " ");
                    int size_int = atoi(size);
                    char *buffer = (char *)malloc(size_int * sizeof(char));
                    FILE *fp;
                    fp = fopen(filename, "w");
                    recv(newServerSock, buffer, size_int, 0);
                    fprintf(fp, "%s", buffer);
                    fclose(fp);
                    free(buffer);
                    char buffi[1024];
                    strcpy(buffi, "ACK");
                    send(newServerSock, buffi, 1024, 0);

                }
                // 11
            }
            // 12

            close(newServerSock);
            printf("Disconnected from the new server connection.\n");
        }

        if (FD_ISSET(clientSock, &readfds))
        {

            pthread_t thread_id;

            // Handle client socket connection
            addr_size = sizeof(clientAddr);
            int newClientSock = accept(clientSock, (struct sockaddr *)&clientAddr, &addr_size);
            if (newClientSock < 0)
            {
                perror("[-]Client Accept error");
                exit(1);
            }

            int *client_socket_ptr = (int *)malloc(sizeof(int));
            *client_socket_ptr = newClientSock;

            // Perform operations on new client connection if needed

            if (pthread_create(&thread_id, NULL, handle_client_ss, (void *)client_socket_ptr) < 0)
            {
                perror("Thread creation failed");
                exit(EXIT_FAILURE);
            }

            pthread_detach(thread_id);

            // Clear the FD sets for the next iteration
        }

        FD_ZERO(&readfds);
        FD_SET(serverSock, &readfds);
        FD_SET(clientSock, &readfds);

    }
}