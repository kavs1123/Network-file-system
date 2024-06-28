#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <zlib.h>
#include <dirent.h>
#include <errno.h>

// macros

#define MAX_CLIENTS 16
#define MAX_SS 10
#define NS_PORT_SS 8080
#define NS_PORT_CLIENT 9000
#define PATH_FILE 1024
#define MAX_COM_LEN 1024
#define SERVER_IP "127.0.0.1"
#define CACHE_LEN 20

#define WHITE "\033[0;37m"
#define GREEN "\033[0;32m"
#define YELLOW "\033[0;33m"
#define RED "\033[0;31m"
#define CYAN "\033[0;36m"
#define BLUE "\033[0;34m"
#define L_GREEN "\033[1;32m"
#define DEFAULT "\033[0m"

typedef struct SS_to_NS{
    
    int port;
    int clientPort;
}SS_to_NS;



// struct Node {
//     char name[256];
//     struct Node *next;
// };


// structs
typedef struct storage_server
{
    char ip[20];
    int server_port;
    int client_port;
    struct Data_LL * head_ptr_for_paths;

}storage_server;

typedef struct ll_of_ss{
    struct ll_of_ss* next_ss;
    struct storage_server ss;

}ll_of_ss;


typedef struct naming_server{
    int number_of_ss;
    ll_of_ss* head_ptr_to_list;
    ll_of_ss* tail_ptr_to_list;
    int server_socket, client_socket,max_fd,activity;
    struct sockaddr_in client_addr,ss_addr,connection_addr;
    socklen_t addr_len;
    fd_set readfds;
    
    
    


}naming_server;



typedef struct Data_LL {
    char name[PATH_FILE];
    struct Data_LL * next;
}Data_LL;



typedef struct ClientToNamingServer{
    char Command[4096];
    int flag;
    int ErrorCode;
}ClientToNamingServer;




typedef struct Namingserver_to_client{
    char ip[20];
    int port_num;
}Namingserver_to_client;


typedef struct Cachetable{
    char ip[20];
    int client_port;
    int server_port;
    char name[PATH_FILE];
}Cachetable;





// functions 

// naming_server
naming_server* init_ns();
void start_ns(naming_server*ns);
int handle_client_req(int* client_port, int*flag, char* ip , char*query);
void connect_to_ss(naming_server* ns,int new_socket);
void* handle_client(void* arg);
ll_of_ss* search_all_ss(int*client_port,char*ip,char*path_of_file);
ll_of_ss* search_all_ss_2(int*server_port,char*ip,char*path_of_file);
void forward_to_ss(int clientPort , char*ip , char*query,ll_of_ss*found_ss);

// storage_server











