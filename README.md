# Network File System (NFS) 

Network File System (NFS) is a distributed file system protocol that allows a user on a client computer to access files over a network as if they were on the local storage. The basic idea behind NFS is to enable remote access to files stored on a server as if they were local files. This is achieved through a client-server architecture, where the server hosts the file system, and clients can mount that file system, making it accessible as if it were a local directory. 

Key features of NFS include:
- **Transparent Access:** NFS provides a transparent way for remote systems to access files over a network. Users and applications interact with the files as if they were stored locally.
- **Network Transparency:** NFS abstracts the underlying network details, allowing users to access remote files without needing to know the physical location of the server.
- **Client-Server Model:** NFS operates on a client-server model. The server exports one or more directories, and clients mount these directories, gaining access to the files within.
- **Stateless Protocol:** NFS is considered a stateless protocol, meaning that each request from a client is independent. Thus the behaviour of the NFS is independent of the number of clients or servers.  

## Overview
This project implements a simple version of a Network File System (NFS) with the following major components:

1. **Clients:** Represent systems or users requesting access to files within the network file system.
2. **Naming Server (NM):** Serves as a central hub, orchestrating communication between clients and storage servers. Manages directory structure and file locations.
3. **Storage Servers (SS):** Responsible for the physical storage and retrieval of files and folders.

The project provides functionalities such as file reading, writing, deletion, creation, listing, and additional information retrieval. TCP protocol is used for communication in this project.

---

## Project Structure

The implementation is divided into the following major components:

1. **Initialization:**
   - **Naming Server (NM):** Initializes the Naming Server to create threads for concurrent connection of clients and servers dynamically.
   - **Storage Servers (SS):** Initializes each Storage Server with vital details. The information sent consists of:
        - _IP Address_ - For communication between multiple devices.
        - _Port_ for Naming Server Connection - A dedicated port for Naming Server connection with Storage Server.
        - _Port_ for Client Connection: A separate port for clients to interact with the Storage Server.
        - List of _Accessible Paths_ - A comprehensive list of file and folder paths that are accessible from the given storage server. Here, this consists of all the file and directories paths which exist in the subdirectory of the storage server. 

   - **Accepting Client Requests:** Naming Server is always open for new client connection requests. Once a connection request is recieved from a client, a new thread is created for handling each clients in the naming server.

2. **Storage Servers (SS) Functionality:**
   - **Adding New Storage Servers:** New storage servers can be created at any time of execution dynamically. This sends the initialisation message to the naming server where all the accessible paths are stored in the naming server alongwith the storage server number.
   - **Commands Issued by NM:** Execute commands like creating empty files, deleting files/directories, and copying files/directories from one path to another. Naming server identifies the type of operation and sends the relevant queries directly to the storage server for execution.
   - **Client Interactions:** Facilitate client operations like reading, writing, and obtaining file information. These 3 operations are handled by directly connecting to the Storage Server by the client. Every time a new query is made, Naming Server sends the query to the storage server alongwith sending a new free port for independent connection between client and storage server. 

3. **Naming Server (NM) Functionality:** This is the central hub for the NFS. 
   - **Storing Storage Server Data:** Serve as the central repository for critical information provided by Storage Servers such as the port details and the list of accessible paths. All the paths are linked to the storage server index in a global Trie. 
   - **Task Execution in Naming Server:** When a client initiates a task (e.g., reading, writing, creating, deleting) by sending a request to the Naming Server, the NM processes the request and delegates the actual execution of the task to the appropriate Storage Server (SS) or performs necessary actions within the NM itself.
   - **Acknowledgment (ACK) or Stop Packet:** After the successful completion of the task, the SS or NM sends an acknowledgment (ACK) packet to the NM to confirm that the operation was carried out as intended. Alternatively, in some scenarios, a predefined "STOP" packet might be sent to signal the conclusion of the operation. 
   - **NM Notifying the Client:** Once the NM receives the acknowledgment or stop signal from the relevant server, it then provides feedback to the client. This feedback includes information about whether the task was completed successfully or if any errors occurred during the execution.  
   - **Timely Response:** The feedback is sent back to the client in a timely manner to ensure that the client is kept informed about the progress of their request. This helps in maintaining a responsive and efficient communication channel between the NM and clients.

4. **Some More Features:** More features such as concurrent multiple client support, error handling, efficient search, failure detection, data redunduncy are handled. 
    - **Concurrent Client Access:** NFS design accommodates multiple clients attempting to access the Naming Server (NM) simultaneously.  
    - **Concurrent File Reading:** While multiple clients can read the same file simultaneously, only one client can execute write operations on a file at any given time. Here a list is made of all the files which are currently being written, so no other client can write on the files which are in this list.
    - **Error Codes:** User defined error codes are returned as acknowledgement from the naming server to the client or the storage server, which are decoded are correspondinly error messages are printed in ${\color{red}RED}$ colour.
    - **Efficient Search in Naming Server:** Optimising the search process of the naming server to find the storage server number corresponding to the path given is done using the data structure Trie. Here strings of paths are stored in the trie with a number stored at the bottom of each string. This ensures that the search process is O(n) where n is the size of the path.
    - **Least Recently Used(LRU) Caching:** A more optimised way for finding the storage server number for a path is to store the last accessed paths and their results, from where we can retrieve the answer in O(1) time. A double ended queue is maintained of size 10 where older queries keep getting deleted and new ones keep getting added at the last.
    - **Failure Detection:** Once a storage server gets offline, a signal is sent to the naming server. This makes the naming server delete all the paths of the storage server from the trie.


## Code Flow
The project consists of 3 main code files- `naming_server.c`, `client.c`, `storage_server.c`. A helper file `extra_function.c` contains the additional functions that are useful for the project.

### Naming Server
- Data Structres 
    - Program declares a trie (`Trie`) to store the file paths and storage servers corresponding to them, and a Least Recently Used Cache (`LRU`) to store previously accessed paths and their results. LRU is implemented using a double sided queue using linked lists.
- Trie Operations
    - The trie is used to efficiently manage and look up file paths and their corresponding storage server IDs. 
    - Functions like `search_in_trie`, `insert`, `delete_from_trie` are employed to perform trie operations.
- Client and Storage Server Connection Handling
    - The program manages both client and storage server connections concurrently using separate threads. `connect_clients_thread` and `connect_ss_thread` handles the connection of clients and storage servers respectively.
    - Storage server connections involve the exchange of initialization messages and accessible paths.
- Client Thread Handling 
    - Accepts client connections and creates a thread executing on the function `client_thread_handler` for each connected client. 
    - It then receives client requests, including operations like _WRITE_, _READ_, _GET_, _COPY_, _CREATE_, and _DELETE_.
    - Manages timeouts for _WRITE_ operations and handles the corresponding sending of error code to the clients.
    - Determines the appropriate storage server for a given file path and communicates with the corresponding storage server. 
    - Sends a unique free port number for client-server communication to both client and server.
- Storage Server Thread Handling 
    - Accepts storage server connections and creates a thread executing on the function `ss_thread_handler` for each connected storage server.
    - Receives initialization messages from storage servers, including IP, port numbers, and accessible paths.
    - Updates the trie with the paths accessible by each storage server.
- Concurrency Control
    - Semaphores (`writelock`) are used to manage write access to the trie.
    - Mutexes (`sslock`) are used to control access to critical sections, ensuring thread safety.
- Error Handling
    - The code includes error handling mechanisms on every step, such as checking socket status, handling timeouts, and reporting errors during operations.
    - Negative acknowledgment codes are used to signal errors back to clients. 
- Colour Coding 
    - Positive acknowledgements and success messages are printed on the terminal with ${\color{green}GREEN}$ colour.
    - Negative acknowlegements and error are shown on the terminal with ${\color{red}RED}$.
    - A few other intermediate steps are printed with ${\color{magenta}MAGENTA}$ colour.
- Logging
    - The program logs various events and messages, providing insights into the execution flow and interactions with clients and storage servers.
- Main Function
    - The data structures declared in the code `Trie` and `LRU` are initialised here. 
    - Threads for connection of servers (`connect_ss_thread`) and clients (`connect_clients_thread`) are created and the function waits for the completion of the threads (`pthread_join`) before exiting.
- Backup of Storage Servers 
    - Everytime a new storage server connects, backup is stored in two other servers if no backup exists for that server, otherwise the backup is synced with the original data in the storage server. 
    - Search also happens in the backup server if the original storage server is not connected currently. 



## Storage Server 
- File and Directory Maniupulation
    - Functions like `listFilesAndDirectories`, `copy_another_serverfile`, `copy_anotherserver_Directory`, `copyDirectory`, `copysend`, `copy_recieve`, `copyfile`, `createfile`, and `deletefile` handle various file and directory operations, including copying, listing, and deleting. These functions handles the queries recieved from the naming server.

- SS Initilization Information
    - Storage server on connection with the naming server sends the initlisation packet containing all the accessible paths of this server, port number and ip address to the naming server.

- Signal Handling 
    - The code defines a `ctrlC` function to handle the Ctrl+C signal (`SIGINT`). When this signal is received, the function sends a signal to the naming server, indicating the storage server has ben stopped. So that corresponding operations may be done in the naming server.



## Client
- Direct server Connection
    - Direct server connection is handled by the function `executeconnection`.
    - Creates a client socket and connects to a Storage Server using the provided port number (`ssportnum`).
    - Performs the functionality based on the operation given.
- Main Function
    - Creates a client socket and connects to the Naming Server.
    - Enters a loop for continuous user interaction. Takes user input, sends it to the Naming Server, and performs actions based on received information.
    - In case of direct communication with the storage server, it receives a port number from the Naming Server and invokes executeconnection to interact with the Storage Server.
- Colour Coding 
    - Positive acknowledgements and success messages are printed on the terminal with ${\color{green}GREEN}$ colour.
    - Negative acknowlegements and error are shown on the terminal with ${\color{red}RED}$.
    - A few other intermediate messages are printed with ${\color{magenta}MAGENTA}$ colour.


## How to Use
Run the file for client and enjoy :)
```bash
gcc client.c -o client_file
./client_file
```
Please follow the below usage syntax for operations from the client.
- CREATE Operation- ```CREATE <file_path> <file_name> <flag>``` where `file_path` is the path of directory where new file/directory is to be created. `file_name` is the name of file/directory to be created. Flag is passed to notify whether it is a file or directory.
```bash 
CREATE /home priet -f                       # creates a file priet in /home
CREATE /home garvit -d                      # creates a directory garvit in /home
```
- DELETE Operation: ```DELETE <file_path>```. This deleted file/directory whose path is given in `file_path`.
```bash
DELETE /home/garvit                         # deletes the directory garvit in home directory 
DELETE /home/priet                          # deletes the file priet in home directory
```
- COPY Operation: ```COPY <source_path> <destination_path>```. This copies the file/directory from the `source_path` to the `destination_path`.
```bash
COPY /home/priet /home/garvit               # this copies the file priet to the /home/garvit directory
```

- READ Operation: ```READ <file_path>```. This reads the file contents of the file present at `file_path`.
```bash
READ /home/garvit/priet                     # reads the contents of file in /home/garvit 
```
- WRITE Operation: ```WRITE <file_path> <text>```. This write the `text` into the file present at the `file_path`. 
```bash
WRITE /home/shivam.txt shivam mittal        # writes "shivam mittal" in shivam.txt present in /home
```

- GET Operation: ```GET <file_path>```. This gets the size and permissions of the file given by the `file_path`.
```bash
GET /home/shivam.txt                        # prints the size and permissions of shivam.txt in /home
```


## How to Contribute

If you'd like to contribute to this project, please follow these steps:

1. Fork the repository.
2. Create a new branch: `git checkout -b feature/new-feature`.
3. Make your changes and commit them: `git commit -m 'Add new feature'`.
4. Push to the branch: `git push origin feature/new-feature`.
5. Submit a pull request.

## Authors
- [Priet Ukani](https://github.com/priet-ukani)
- [Shivam Mittal](https://github.com/mittalshivam2709)
- [Garvit Gupta](https://github.com/GarvitGuptaIIITH)


## Acknowledgements

 - [Multi Server Client Connection](https://cs.gmu.edu/~setia/cs571-F02/slides/lec9.pdf)
 - [Distributed File System Slides](https://web2.qatar.cmu.edu/~msakr/15440-f11/lectures/Lecture19_15440_MHH_14Nov_2011.ppt)


## ASSUMPTIONS:
- All the inputs and file paths are taken of size 1024.
- Assuming that the path given for creating a new file or directory is absolute to the main directory.
- No storage server may be running in the subdirectory of any other storage server. 