#include "defs.h"



int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    int flag=-1,type_flag=0;
    char buffer[1024];


    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf(RED "\n Socket creation error \n" DEFAULT);
        return -1;
    }

    // Set server address and port
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(NS_PORT_CLIENT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        printf(RED "\nInvalid address/ Address not supported \n" DEFAULT);
        return -1;
    }

    // Connect to server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf(RED "\nConnection Failed \n" DEFAULT);
        return -1;
    }

    //recieve acknowledgement from server
    int ack_bytes_received = 0;
    int recv_ack = 0;
    if((recv(sock, &recv_ack, sizeof(int), 0)) > 0){
        printf(GREEN "Connected to Naming Server\n" DEFAULT);
    }

    printf("\e[1;1H\e[2J");
    printf(GREEN "\e[1mClient\e[m\n" DEFAULT);

    // Get filename from user
    while(flag!=2){
        char *Command=(char*)malloc(sizeof(char)*MAX_COM_LEN);

        char BackupCommand[MAX_COM_LEN];
        flag=-1;
        type_flag=0;        
        memset(Command, 0, sizeof(Command));
        printf(CYAN ">> " DEFAULT);
        scanf(" %[^\n]s",Command);
        // printf("%s\n",Command);
        

        strcpy(BackupCommand,Command);

        char* context;
        char* SToken=strtok(BackupCommand," ");
        char MainComm[20];


        //Read is 1, Write is 2, Retrieve is 3
        if(strcmp(SToken,"READ")==0){
            flag=1;
            type_flag=1;
            strcpy(MainComm,SToken);
        }
        else if(strcmp(SToken,"WRITE")==0){
            flag=1;
            type_flag=2;
            strcpy(MainComm,SToken);
        }
        else if(strcmp(SToken,"RETRIEVE")==0){
            flag=1;
            type_flag=3;
            strcpy(MainComm,SToken);
        }
        else if(strcmp(SToken,"CREATE")==0){
            flag=0;
            strcpy(MainComm,SToken);
            SToken=strtok(NULL," ");
            if(SToken==NULL || (strcmp(SToken,"FILE")!=0 && strcmp(SToken,"FOLDER")!=0)){
                printf(RED "Invalid Command\n" DEFAULT);
                continue;
            }
        }
        else if(strcmp(SToken,"DELETE")==0){
            flag=0;
            strcpy(MainComm,SToken);
            SToken=strtok(NULL," ");
            if(SToken==NULL || (strcmp(SToken,"FILE")!=0 && strcmp(SToken,"FOLDER")!=0)){
                printf(RED "Invalid Command\n" DEFAULT);
                continue;
            }
        }
        else if(strcmp(SToken,"COPY")==0){
            strcpy(MainComm,SToken);
            flag=0;
        }
        else if(strcmp(SToken,"EXIT")==0){
            strcpy(MainComm,SToken);
            flag=2;
        }
        else{
            printf(RED "Invalid Command\n" DEFAULT);
            continue;
        }
        

        int bytes_sent=0;


        if((bytes_sent = send(sock, Command,1024, 0)) < 0){
            printf(RED "Send failed\n" DEFAULT);
            return -1;
        }
        ack_bytes_received = 0;
        int ack_bytes=0;
        if((ack_bytes_received = recv(sock, &ack_bytes, sizeof(int), 0)) > 0){
            if(ack_bytes==1){
                printf(GREEN "%s completed successfully\n" DEFAULT,MainComm);
            }
            else{
                SToken=strtok(NULL," ");
                printf(RED "No %s Found!\n" DEFAULT,SToken);
                continue;
            }
        }
        //printf("bytes:%s\n",Command);

        // Receive file contents from server

        if(flag==1){
            Namingserver_to_client NSTC;
            int NSTC_bytes_received = 0;

            recv(sock,&NSTC.port_num,sizeof(int),0);
            recv(sock,NSTC.ip,20,0);
            // if((NSTC_bytes_received = recv(sock, &NSTC, sizeof(NSTC), 0)) > 0)
            {
                if(NSTC.port_num!=0){
                    printf(BLUE "Storage Server Details:\n");
                    printf("\t IP: %s\n", NSTC.ip);
                    printf("\t Port: %d\n" DEFAULT, NSTC.port_num);
                }

                int sock_ss = 0;
                struct sockaddr_in serv_addr_ss;


                //connecting to storage server
                if ((sock_ss = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                    printf(RED "\n Socket creation error \n" DEFAULT);
                    return -1;
                }

                // Set server address and port
                serv_addr_ss.sin_family = AF_INET;
                serv_addr_ss.sin_port = htons(NSTC.port_num);

                // Convert IPv4 and IPv6 addresses from text to binary form
                if (inet_pton(AF_INET,NSTC.ip, &serv_addr_ss.sin_addr) <= 0) {
                    printf(RED "\nFile not Found \n" DEFAULT);
                    // return -1;
                    continue;
                }

                // Connect to server
                if (connect(sock_ss, (struct sockaddr *)&serv_addr_ss, sizeof(serv_addr_ss)) < 0) {
                    printf(RED "\nConnection Failed \n" DEFAULT);
                    return -1;
                }

                if(send(sock_ss, Command, 1024, 0) < 0){
                    printf(RED "Send failed\n" DEFAULT);
                    return -1;
                }
                // Read is 1, Write is 2, Retrieve is 3

                if(type_flag==1){
                    int bytes_recv=0;
                    char read_buff[4096];
                    printf(GREEN "Data Recieved: \n" DEFAULT);
                    while((bytes_recv = recv(sock_ss,read_buff, 4096, 0))>0 ){
                        if(strcmp(read_buff, "EXIT") == 0){
                            break;
                        }
                        if(strcmp(read_buff, "RxE") == 0){
                            printf(RED "Read Error Occured! Unable to Read\n" DEFAULT);
                            bzero(read_buff, 4096);
                            recv(sock_ss,read_buff, 4096, 0);
                            break;
                        }
                        else if(strcmp(read_buff, "FnF") == 0){
                            printf(RED "File Not Found!\n" DEFAULT);
                            bzero(read_buff, 4096);
                            recv(sock_ss,read_buff, 4096, 0);
                            break;
                        }
                        else if(strcmp(read_buff,"PxD") == 0){
                            printf(RED "Permission Denied! Insufficient permission level.\n" DEFAULT);
                            bzero(read_buff, 4096);
                            recv(sock_ss,read_buff, 4096, 0);
                            break;
                        }
                        printf("%s", read_buff);
                        bzero(read_buff, 4096);
                    }

                    printf(GREEN "READ completed successfully\n" DEFAULT);
                    memset(buffer, 0, sizeof(buffer));
                    strcpy(buffer,"EXIT");
                    if(send(sock_ss, buffer, 1024, 0) < 0){
                        printf(RED "Send failed\n" DEFAULT);
                    }
                }
                else if(type_flag==2){
                    int bytes_recv=0;
                    int ack_bytes=0;
                    if((bytes_recv = recv(sock_ss,&ack_bytes, sizeof(ack_bytes), 0))>0 ){
                        if(ack_bytes == 0){
                            printf(GREEN "WRITE completed successfully\n" DEFAULT);                            
                        }
                        else if (ack_bytes== 404){
                            printf(RED "File Not Found!\n" DEFAULT);
                        }
                        else if (ack_bytes == 500){
                            printf(RED "WRITE Permission level Insufficient!\n" DEFAULT);
                        }
                    }
                    memset(buffer, 0, sizeof(buffer));
                    strcpy(buffer,"EXIT");
                    if(send(sock_ss, buffer, strlen(buffer), 0) < 0){
                        printf(RED "Send failed\n" DEFAULT);
                    }
                }
                else if(type_flag==3){
                    memset(buffer, 0, sizeof(buffer));
                    int bytes_recv=0;
                    struct stat stats;
                    if((bytes_recv=recv(sock_ss, &stats, sizeof(stats),0 )>0)){
                        struct tm dt;
                        if(stats.st_mode == -1){
                            printf(RED "File Not Found!\n" DEFAULT);
                        }
                        else{
                            printf("\nFile access: \n");
                            printf("Permission level: %d", stats.st_mode);
                            printf("\nFile size: %ld", stats.st_size);
                            dt = *(gmtime(&stats.st_ctime));
                            printf("\nCreated on: %d-%d-%d %d:%d:%d", dt.tm_mday, dt.tm_mon, dt.tm_year + 1900, dt.tm_hour, dt.tm_min, dt.tm_sec);
                            dt = *(gmtime(&stats.st_mtime));
                            printf("\nModified on: %d-%d-%d %d:%d:%d", dt.tm_mday, dt.tm_mon, dt.tm_year + 1900, dt.tm_hour, dt.tm_min, dt.tm_sec);
                            printf("\n");
                        }
                        char send_loli[1024];
                        strcpy(send_loli, "EXIT");
                        send(sock_ss, send_loli, 1024, 0);
                    }
                }
                // printf("Terminating connection with Storage Server\n");
                close(sock_ss);
            }           
        }
        free(Command);
    }
    // Close socket
    printf("Closing Client\n");
    close(sock);


    return 0;
}
