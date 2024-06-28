# final-project-040



# Network File System
## Group 40

### Instructions:
1. Open 1 terminal window for the naming server
2. Open n terminal windows for clients.
3. To run storage naming servers, create a separate file and paste the code and change the server and client ports.
4. To run the executables in existing folder, do `make clean`, then `make` in the terminal.

### Assumptions
1. The naming server is always running.
2. The storage servers are always running.
3. Caching is sufficient for quick retrieval, especially with a larger cache size.
4. The User does not try to acces files outside of the root directory of the storage server.

### Command Format
1. READ filename
2. WRITE "text to write" filename
3. RETRIEVE filename
4. CREATE FILE/FOLDER filename
5. DELETE FILE/FOLDER filename
6. COPY FILE/FOLDER source_filename destination_filename

### Design
#### Naming Server
1. The naming server is a single server that handles all the requests from the clients.
2. It also connects and acts as a middleman to execute commands between the client and the storage servers.
3. The program initializes the naming server (ns) and a cache table (global_cache), which listens for connections from both storage servers (SS) and clients.
4. New storage server connections are added to a linked list (ll_of_ss), storing server/client ports and file paths.
5. Threads are created to handle client requests. The handle_client function processes client queries.
6. The search_all_ss and search_all_ss_2 functions locate storage servers based on client requests.
7. A cache table (global_cache) stores information about recent file accesses to optimize performance.
8. Pthreads are used for concurrent handling of multiple client connections.

#### Storage Server

1. The storage server is a single server that handles all the requests from the naming server.
2. The program initializes server and client sockets and connects to the naming server (NS) using a socket (sock) and sends information about its IP, ports, and file structure.
3. On connecting to the naming server, the storage server sends its IP, ports, and file structure to the naming server.
4. For operations such as Read, Write and Retrieve, the storage server directly connects to the client to communicate.
5. Threads are created for handling client connections.
   
#### Clients

1. The client program initializes a client socket and connects to the naming server (NS) using a socket (sock).
2. If the user command requires interaction with a Storage Server (e.g., READ, WRITE, RETRIEVE), the client establishes a connection with the Storage Server.
3. For READ operations, it receives and displays file content from the Storage Server. For WRITE operations, it sends data to the Storage Server and receives an acknowledgment. For RETRIEVE operations, it receives and displays file information from the Storage Server.
4. The client includes error handling for various scenarios, such as invalid commands, unsuccessful operations, and issues related to file not found or insufficient permissions.
5. The client categorizes commands into different types (READ, WRITE, RETRIEVE, CREATE, DELETE, COPY, EXIT). The server acknowledges the completion or failure of an operation, and the client appropriately informs the user.

