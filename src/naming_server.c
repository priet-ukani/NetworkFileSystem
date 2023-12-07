#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <threads.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include <stdarg.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#include "extra_functions.h"
#include <netinet/in.h>
#include <poll.h>
pthread_mutex_t sslock;

#define CLIENT_PORT 44456
#define SERVER_PORT 55587
int serversockfd;
#define MAX_SIZE 1024
#define MAX_WORDS_QUERY 10
int number_of_clients = 1;
int number_of_ss = 1;

struct trie *Trie;
struct trie *Trie_backup_1;
struct trie *Trie_backup_2;

sem_t writelock;
struct trie *writeFiles;
LRUCache *LRU;

int server_socket_backup_1 = -1;
int server_socket_backup_2 = -1;

char **backup_server_parent_paths;
int backup_server_parent_paths_size = 0;
void query_execute(char *buffer, int newSocket, int backup_ind);

typedef struct storage_server_info
{
    int on;
    char storage_server_ip[32];
    int port_number_for_naming_server;
    int port_number_for_client;
    char accessible_paths[MAX_SIZE][MAX_SIZE];
    char *main_parent_directory_path;
    int num_paths;
    int socket_number;
    int is_backup_1;
    int is_backup_2;
} storage_server_info;

storage_server_info servers_info[MAX_SIZE];

struct pass_to_thread
{
    struct sockaddr_in addr;
    int newsocket;
    int index;
};

struct server_initialisation_message
{
    char storage_server_ip[32];
    int port_number_for_naming_server;
    int port_number_for_client;
    int num_paths;
};

int search_in_trie_helper(struct trie *root, char *word)
{

    char *word2 = strdup(word);
    int search_in_LRU = get(LRU, word2);
    if (search_in_LRU != -1)
    {
        // printf("%s Found in LRU\n", word2);
        write_to_book("Found %s  in LRU\n", word2);
        printc(GREEN, "Found %s in LRU.\n", word2);
        return search_in_LRU;
    }
    char *word3 = strdup(word);
    int result = search_in_trie(root, word3);
    printf("result %d\n", result);
    if (result != -1)
    {
        // printf("inserted in lru %s\n", word);
        write_to_book("inserted in lru %s\n", word);
        printc(YELLOW, "Inserted %s in LRU.\n", word);
        put(LRU, word, result);
    }
    return result;
}

int search_in_trie_with_backup(struct trie *T, char *path)
{
    int result = search_in_trie(T, path);
    if (result != -1)
    {
        printf("here found in server\n");
        return result;
    }
    else
    {
        char path1[1024] = "Backup1/";
        char path2[1024] = "Backup2/";
        strcat(path1, path);
        strcat(path2, path);
        result = search_in_trie(T, path1);
        if (result != -1)
        {
            strcpy(path, path1);
            printc(ORANGE, "Getting from %s\n", path);
            return result;
        }
        else
        {
            result = search_in_trie(T, path2);
            if (result != -1)
            {
                strcpy(path, path2);
                printc(ORANGE, "Getting from %s\n", path);
                return result;
            }
        }
    }
    return -1;
}

int sync_storage_server(char *storage_server_parent_path)
{
    // delete first 
    char query[1024]="DELETE Backup1/";
    strcat(query,storage_server_parent_path);
    strcat(query," ");
    query_execute(query, 0, 0);
    // now copy
    char query2[1024]="COPY ";
    strcat(query2,storage_server_parent_path);
    strcat(query2," Backup1/");
    // strcat(query2,storage_server_parent_path);
    strcat(query2," ");
    strcat(query2,storage_server_parent_path);
}

// This function deletes all the paths of that ss from the Trie once a Storage server is disconnected
int delete_storage_server_paths(int ss_id)
{
    for (int i = 0; i < servers_info[ss_id].num_paths; i++)
    {
        delete_from_trie(Trie, servers_info[ss_id].accessible_paths[i]);
        // removeKey(LRU, servers_info[ss_id].accessible_paths[i]);
    }
    return 1;
}

int backup_storage_1_index()
{
    for (int i = 0; i < number_of_ss; i++)
    {
        if (servers_info[i].is_backup_1 == 1)
        {
            return i;
        }
    }
    return -1;
}

int backup_storage_2_index()
{
    for (int i = 0; i < number_of_ss; i++)
    {
        if (servers_info[i].is_backup_2 == 1)
        {
            return i;
        }
    }
    return -1;
}

// we will create two other servers which will only contain backup of servers ::))
int backup_server_data(int server_index)
{
    // copy all the files to other two servers
    // storage servers running in B1 and B2 are the backup storage servers which will store backup of all other servers

    /* we have to copy the main parent directory of the server
    so we find the path in this server with minimum length and
    copy this full dir to the backup storage server 1 and 2 */

    int minimum_length = 99999;
    char *main_parent_directory = (char *)malloc(sizeof(char) * MAX_SIZE);
    for (int j = 0; j < servers_info[server_index].num_paths; j++)
    {
        int len = strlen(servers_info[server_index].accessible_paths[j]);
        if (minimum_length > len)
        {
            minimum_length = len;
            strcpy(main_parent_directory, servers_info[server_index].accessible_paths[j]);
        }
    }
    printc(BLUE, "Main parent directory: %s\n", main_parent_directory);
    // now we need to copy this whole folder to the backup server
    // first check if there are any free slots available or not
    servers_info[server_index].main_parent_directory_path = strdup(main_parent_directory);
    if (server_socket_backup_1 == -1)
    {
        printc(RED, "Backup Server 1 is down. Please restart it.\n");
        return -1;
    }
    else if (server_socket_backup_2 == -1)
    {
        printc(RED, "Backup Server 2 is down. Please restart it.\n");
        return -1;
    }
    // if both are working then so backup

    char create_query[1024] = "CREATE ";
    char create_query2[1024] = "CREATE ";
    strcat(create_query, "Backup1");
    strcat(create_query2, "Backup2");
    strcat(create_query, " ");
    strcat(create_query2, " ");
    char *storage_server_number;
    storage_server_number = (char *)malloc(sizeof(char) * 10);
    snprintf(storage_server_number, sizeof(storage_server_number), "%d", server_index);
    // strcat(create_query, storage_server_number);
    strcat(create_query, servers_info[server_index].main_parent_directory_path);
    strcat(create_query2, servers_info[server_index].main_parent_directory_path);
    // strcat(create_query2, storage_server_number);
    strcat(create_query, " -d ");
    strcat(create_query2, " -d ");

    char query[1024] = "COPY ";
    char query2[1024] = "COPY ";
    strcat(query, main_parent_directory);
    strcat(query2, main_parent_directory);
    strcat(query, " ");
    strcat(query2, " ");
    char f[10] = "Backup1/";
    char f2[10] = "Backup2/";
    char temp[30];
    strcpy(temp, f);
    strcat(temp, servers_info[server_index].main_parent_directory_path);
    insert(Trie, temp, strlen(temp), backup_storage_1_index());
    strcpy(temp, f2);
    strcat(temp, servers_info[server_index].main_parent_directory_path);
    insert(Trie, temp, strlen(temp), backup_storage_2_index());

    strcat(query, f);
    strcat(query2, f2);
    strcat(query, servers_info[server_index].main_parent_directory_path);
    strcat(query2, servers_info[server_index].main_parent_directory_path);
    strcat(query, " ");
    strcat(query2, " ");

    // printf("query: %s", query);
    query_execute(create_query, server_socket_backup_1, 1);
    printc(BLUE, "Backup for Storage Server %d started on Backup1.\n", server_index);
    query_execute(query, server_socket_backup_1, 1);
    int h;
    // recv(server_socket_backup_1, (int *)&h, sizeof(h), 0);
    // printf("Kevin double Chutiya hai\n");
    query_execute(create_query2, server_socket_backup_2, 2);
    printc(BLUE, "Backup for Storage Server %d started on Backup2.\n", server_index);
    query_execute(query2, server_socket_backup_2, 2);
}

int trie_update(int storage_server_id, char *query)
{
    char *words_array[MAX_WORDS_QUERY];
    char *query1 = strdup(query);
    query1[strlen(query1) - 1] = '\0';
    // Tokenize the input string and store words in the array
    char *token = strtok(query1, " ");
    int i = 0;
    while (token != NULL && i < MAX_WORDS_QUERY)
    {
        words_array[i++] = token;
        token = strtok(NULL, " ");
    }

    char *oper = strdup(words_array[0]);
    char *path;
    char *new_name;
    char *flag;

    if (strcmp(oper, "CREATE") == 0)
    {
        path = strdup(words_array[1]);
        new_name = strdup(words_array[2]);
        flag = strdup(words_array[3]);
        // printf("idhar ");
        strcat(path, "/");
        strcat(path, new_name);
        // now insert into the trie under this storage server
        int ss_id = search_in_trie_with_backup(Trie, words_array[1]);
        // printf("Storage Server ID is %d\n", ss_id);
        write_to_book("Storage Server ID is %d\n", ss_id);
        insert(Trie, path, strlen(path), ss_id);
        // printf("%s path inserted in trie\n", path);
        write_to_book("%s path inserted in trie\n", path);
    }
    else if (strcmp(oper, "DELETE") == 0)
    {
        path = strdup(words_array[1]);

        delete_from_trie(Trie, path);
    }

    else if (strcmp(oper, "COPY") == 0)
    {
        char *path1 = strdup(words_array[1]);
        char *path2 = strdup(words_array[2]);
        char *copypath = strdup(path1);
        char *copypath2 = strdup(path2);
        char *filename = strdup(path1);
        char *token = strtok(copypath, "/");
        while (token != NULL)
        {
            strcpy(filename, token);
            token = strtok(NULL, "/");
        }
        strcat(copypath2, "/");
        strcat(copypath2, filename);
        // printf("path2 %s\n", copypath2);
        // write_to_book("path2 %s\n", copypath2);
        int ss_id = search_in_trie_with_backup(Trie, words_array[2]);
        insert(Trie, copypath2, strlen(copypath2), ss_id);
        // printf("New path inserted in SS: %d\n", ss_id);
    }
}

struct send_server
{
    bool is_port; // 1 then port is sent
    int port_client_conn;
    char buffer[MAX_SIZE];
};

// 1 for directory
// 0 for file create
int copy_diffserver(int ss_sock1, int ss_sock2, char *dest_path, int ss_id_2)
{
    int flag;
    bool ok = false;
    char buffer[MAX_SIZE];
    while (1)
    {
        int r = recv(ss_sock1, (int *)&flag, sizeof(int), 0);
        printc(GREEN, "flag is %d.\n", flag);
        if (flag == -1 && !ok)
        {
            ok = true;
            continue;
        }
        send(ss_sock2, (int *)&flag, sizeof(int), 0); // sending flag
        if (flag == -1 || flag == -100)
        {
            // -100 denotes the copying is complete

            break;
        }
        if (flag == 0)
        {
            // file to be copied
            bzero(buffer, sizeof(buffer));
            recv(ss_sock1, (char *)&buffer, sizeof(buffer), 0); // recieving relative path of the file
            char *new_relative_path = strdup(dest_path);
            strcat(new_relative_path, "/");
            strcat(new_relative_path, buffer);

            char *temp = strdup(new_relative_path);
            // printc(GREEN, "Inserted in Trie after Copy%s.\n", new_relative_path);
            insert(Trie, temp, strlen(temp), ss_id_2);

            // printc(GREEN, "name of file is %s.\n", buffer, buffer);

            send(ss_sock2, (char *)&buffer, sizeof(buffer), 0); // sent relative path of the file for creation
            char buffer2[MAX_SIZE];

            while (1)
            {
                bzero(buffer2, sizeof(buffer2));
                recv(ss_sock1, (char *)&buffer2, MAX_SIZE, 0);
                // printc(GREEN, "recieved from server: %s\n", buffer2);
                send(ss_sock2, (char *)&buffer2, MAX_SIZE, 0);
                if (strcmp(buffer2, "END") == 0)
                {
                    // send(ss_sock2, (char *)&buffer2, MAX_SIZE, 0);
                    break;
                }
            }
        }
        else if (flag == 1)
        {
            bzero(buffer, sizeof(buffer));
            recv(ss_sock1, (char *)&buffer, sizeof(buffer), 0); // recieving name of the directory

            char *new_relative_path = strdup(dest_path);
            strcat(new_relative_path, "/");
            strcat(new_relative_path, buffer);

            char *temp = strdup(new_relative_path);
            // printc(GREEN, "Inserted in Trie after Copy%s.\n", new_relative_path);
            insert(Trie, temp, strlen(temp), ss_id_2);

            send(ss_sock2, (char *)&buffer, sizeof(buffer), 0); // sent name of the directory for creation
            // send(ss_sock2, (char *)&buffer, sizeof(buffer), 0);
            printc(GREEN, "Directory %s created.\n", buffer);
        }
    }
    return 0;
}

void query_execute(char *buffer, int newSocket, int backup_ind)
{

    // printc(MAGENTA, "query: %s", buffer);

    char *temp = strdup(buffer);
    // printf("Client: %s\n", buffer);
    printc(ORANGE, "Client: %s\n", buffer);
    write_to_book("Client: %s\n", buffer);
    // send(newSocket, (char *)&buffer, sizeof(buffer), 0); // ack type of thing
    // continue;
    char *words_array[MAX_WORDS_QUERY];
    char *query = strdup(buffer);
    char *buffer2 = strdup(query);
    query[strlen(query) - 1] = '\0';
    // Tokenize the input string and store words in the array
    char *token = strtok(query, " ");
    int i = 0;
    while (token != NULL && i < MAX_WORDS_QUERY)
    {
        // printc(YELLOW,"token is %s\n",token);
        words_array[i++] = strdup(token);
        token = strtok(NULL, " ");
    }
    int ss_index = -1;
    int ss_ind1 = -1;
    int ss_ind2 = -1;
    int port_number_for_client_conn;
    char *oper = strdup(words_array[0]);
    bool send_to_client = false;
    bool send_to_server = false;
    bool send_copy_diff_server = false;
    printc(MAGENTA, "OPERATION: %s\n", oper);
    bool invalid_path = true;
    if (strcmp(oper, "WRITE") == 0)
    {
        printf("Writing to %s\n", words_array[1]); // comment this afterwards
        char *path = strdup(words_array[1]);
        time_t starttime;
        time(&starttime);
        time_t endtime = starttime;
        bool timeout = false;
        while (1)
        {
            time(&endtime);
            if (difftime(endtime, starttime) >= 2)
            {
                timeout = true;
                break;
            }
            if (search_in_trie(writeFiles, path) == -1)
            {
                sem_wait(&writelock);
                insert(writeFiles, path, strlen(path), 1);
                sem_post(&writelock);
                break;
            }
            usleep(50000);
        }
        if (timeout)
        {
            printc(RED, "Timeout occured.\n");
            write_to_book("Timeout occured.\n");
            int send_error = -1;
            send(newSocket, (int *)&send_error, sizeof(int), 0);
        }
        ss_index = search_in_trie_with_backup(Trie, path);
        // ss_index = search_in_trie_with_backup(Trie, path);
        if (ss_index != -1)
        {
            port_number_for_client_conn = servers_info[ss_index].port_number_for_client;
        }

        // sending to server/client for write below
        if (ss_index == -1)
        {
            // path does not exist
            printc(RED, "Invalid path. Not found in any servers.\n");
            int new_conn_portno = -1;
            send(newSocket, (int *)&new_conn_portno, sizeof(int), 0);
        }
        else
        {
            int ss_socket = servers_info[ss_index].socket_number;
            int conn_client_portno = servers_info[ss_index].port_number_for_client;
            // printf("sent port number to client %d\n", port_number_for_client_conn);
            printc(MAGENTA, "Sent port number to client %d\n", port_number_for_client_conn);
            write_to_book("sent port number to client %d\n", port_number_for_client_conn);
            int new_conn_portno = generate_random_port(); // sending a port for client connection to the server and also to client
                                                          // timeout=true; // for testing and checking
            if (timeout)
            {
                int error_code = -200; // this is timeout for write operation
                send(newSocket, (int *)&error_code, sizeof(int), 0);
            }
            else
            {
                send(newSocket, (int *)&new_conn_portno, sizeof(int), 0);
            }

            char buffer4[MAX_SIZE];
            snprintf(buffer4, sizeof(buffer4), "%d", new_conn_portno);
            if (timeout)
            {
                bzero(buffer4, sizeof(buffer4));
                strcpy(buffer4, "-200");
                send(ss_socket, (char *)&buffer4, MAX_SIZE, 0);
            }
            else
            {
                send(ss_socket, (char *)&buffer4, MAX_SIZE, 0);
            }
        }
    }
    else if (strcmp(oper, "READ") == 0 || strcmp(oper, "GET") == 0)
    {
        // send the port number to the client connection port number of the server for connecting directly to the server
        printf("%s\n", words_array[1]); // comment this afterwards
        char *path = strdup(words_array[1]);

        ss_index = search_in_trie_with_backup(Trie, path);
        // ss_index = search_in_trie_with_backup(Trie, path);
        if (ss_index != -1)
        {
            port_number_for_client_conn = servers_info[ss_index].port_number_for_client;
        }
        send_to_client = true;
    }
    else if (strcmp(oper, "COPY") == 0 || strcmp(oper, "CREATE") == 0 || strcmp(oper, "DELETE") == 0)
    {
        char *path = strdup(words_array[1]);
        ss_index = search_in_trie_with_backup(Trie, path);
        if (strcmp(oper, "COPY") == 0)
        {
            // printf("here in copy part\n");
            // printf("from %s to %s\n", words_array[1], words_array[2]);
            ss_ind1 = search_in_trie_with_backup(Trie, words_array[1]);
            ss_ind2 = search_in_trie_with_backup(Trie, words_array[2]);
            printc(YELLOW, "indexes are %d %d\n", ss_ind1, ss_ind2);
            // return;
            if (ss_ind1 != ss_ind2)
            {
                send_copy_diff_server = true;
            }
            else
            {
                send_to_server = true;
            }
        }
        else
        {
            send_to_server = true;
        }

        // continue;
    }
    else
    {
        // printf("Invalid operation\n");
        printc(RED, "Invalid operation.\n");
        write_to_book("Invalid operation.\n");
        // continue;
        invalid_path = false;
        int error_code = -100;
        send(newSocket, (int *)&error_code, sizeof(error_code), 0);
    }

    if (invalid_path && ss_index == -1)
    {
        // perror("Path does not exist in the Network Server under any storage server.\n");
        printc(RED, "Path does not exist in the Network Server under any storage server.\n");
        port_number_for_client_conn = -1;
    }

    if (send_to_client)
    {
        if (ss_index == -1)
        {
            // path does not exist
            int new_conn_portno = -1;
            send(newSocket, (int *)&new_conn_portno, sizeof(int), 0);
            // continue;
            return;
        }
        else
        {
            int ss_socket = servers_info[ss_index].socket_number;
            int conn_client_portno = servers_info[ss_index].port_number_for_client;
            // printf("sent port number to client %d\n", port_number_for_client_conn);
            printc(MAGENTA, "Sent port number to client %d\n", port_number_for_client_conn);
            write_to_book("sent port number to client %d\n", port_number_for_client_conn);
            int new_conn_portno = generate_random_port(); // sending a port for client connection to the server and also to client
            send(newSocket, (int *)&new_conn_portno, sizeof(int), 0);
            char buffer4[MAX_SIZE];
            snprintf(buffer4, sizeof(buffer4), "%d", new_conn_portno);

            send(ss_socket, (char *)&buffer4, MAX_SIZE, 0);
        }
    }
    if (send_to_server)
    {
        int ss_socket_number;
        if (ss_index != -1)
        {

            ss_socket_number = servers_info[ss_index].socket_number;
            // printf("Sending from Naming Server to Storage server on socket %d\n", ss_socket_number);
            printc(MAGENTA, "Sending from Naming Server to Storage server on socket %d\n", ss_socket_number);
            write_to_book("Sending from Naming Server to Storage Server on socket %d\n", ss_socket_number);
            printf("%s\n", buffer2);
            char *buffer3 = strdup(buffer2);

            // int rsp = send(ss_socket_number, (struct send_server *)&test, sizeof(struct send_server), 0);
            int rsp = send(ss_socket_number, (char *)buffer3, MAX_SIZE, 0);
            printf("%d\n", rsp);
            if (rsp < 0)
            {
                // perror("sending erorr");
                printc(RED, "Sending error\n");
            }
            // int acknowledgement = 0;
            // int r = recv(ss_socket_number, (int *)&acknowledgement, sizeof(int), 0);
            // printc(RED,"LAST CALL\n");
            // if (r < 0)
            // {
            //     // perror("error in recieving acknowledgement");
            //     printc(RED, "Error in recieving acknowledgement.\n");
            // }
            // if (acknowledgement == 1)
            // {
            //     // printf("operation successful\n");
            //     printc(GREEN, "Operation Successful of the client.\n");
            //     // here we should delete or insert the path in the trie.
                trie_update(ss_index, buffer3);
            //     write_to_book("Operation Successful of the client.\n");
            // }
            // else
            // {
            //     printc(RED, "Operation Unsuccessful of the client.\n");
            //     write_to_book("Operation Unsuccessful of the client.\n");
            // }
        }
    }
    // printc(RED,"above if condn");
    if (send_copy_diff_server)
    {
        printf("Sending to different servers\n");
        // printf("port number of ss1 %d\n",ss_ind1);
        // printf("port number of ss2 %d\n",ss_ind2);
        printf("storage numbers %d %d\n", ss_ind1, ss_ind2);
        if (ss_ind1 != -1 && ss_ind2 != -1)
        {
            // printc(YELLOW,"eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee");
            int ss_sock1 = servers_info[ss_ind1].socket_number;
            int ss_sock2 = servers_info[ss_ind2].socket_number;
            // char buffer4[1024]=strcat("COPYSEND ",words_array[1]);
            char buffer4[1024];
            strcpy(buffer4, "COPYSEND ");
            strcat(buffer4, words_array[1]);
            int rsp1 = send(ss_sock1, (char *)&buffer4, 1024, 0);
            printc(YELLOW, "sent \n");
            // return;
            // printf("%d\n", rsp1);
            if (rsp1 < 0)
            {
                // perror("sending erorr");
                printc(RED, "Sending error in 1\n");
            }
            char buffer5[1024];
            strcpy(buffer5, "COPYRECV ");
            strcat(buffer5, words_array[2]);
            strcat(buffer5, " ");

            // last_slash = (char*)malloc(sizeof(char)*MAX_SIZE);
            char *last_slash = strrchr(words_array[1], '/');
            if (last_slash != NULL)
            {
                last_slash++;
                strcat(buffer5, last_slash);
            }
            else
            {
                // printf("The string doesn't contain a '/': %s\n", path);
                strcat(buffer5, words_array[1]);
            }
            int rsp2 = send(ss_sock2, (char *)&buffer5, 1024, 0);
            // printf("%d\n", rsp2);
            if (rsp2 < 0)
            {
                // perror("sending erorr");
                printc(RED, "Sending error in 2\n");
            }

            // if(backup_ind==1){
            //        insert(Trie_backup_1, words_array[2], strlen(buff), index);
            // }
            copy_diffserver(ss_sock1, ss_sock2, words_array[2], ss_ind2);
            // jab pura copy karna hoga tab request se nahi karenge manually karenge
        }
        else
        {
            printf("oidncwoibdcw\n");
        }
    }
}

void *client_thread_handler(void *arg)
{
    // pthread_mutex_lock(&sslock);
    char buffer[MAX_SIZE];
    // struct socket_addr_in new_addr=*(struct socket_addr)
    struct pass_to_thread pt = *((struct pass_to_thread *)(arg));
    int newSocket = pt.newsocket;
    struct sockaddr_in newAddr = pt.addr;
    while (1)
    {
        bzero(buffer, MAX_SIZE);
        recv(newSocket, (char *)&buffer, sizeof(buffer), 0);
        // printf("fsdhjkgyhfrhedsgfyuhedsrtydsfghjkbfdhgdfijghdfjgdfhgifdughdfighdsf");
        if (strlen(buffer) == 0 || strcmp(buffer, ":exit") == 0)
        {
            // printf("Client with port number %d disconnected.\n", newAddr.sin_port);
            write_to_book("Client with port number %d disconnected.\n", newAddr.sin_port);
            printc(RED, "Client with port number %d disconnected.\n", newAddr.sin_port);
            break;
        }
        query_execute(buffer, newSocket, -1);
        bzero(buffer, sizeof(buffer));
    }
    // pthread_mutexunlock(&sslock);
}

void signalfunction(int newsocket, int port, int index)
{
    sleep(1);
    // printf("here");
    int signalsocket, ret;
    struct sockaddr_in serverAddr;

    signalsocket = socket(AF_INET, SOCK_STREAM, 0);
    if (signalsocket < 0)
    {
        printc(RED, "[-]Error in creation of socket for signal .\n");
    }
    printc(GREEN, "[+]Signal Socket is created.\n");

    memset(&serverAddr, '\0', sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    ret = connect(signalsocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr));

    // printf("PORT NUMBER: %d\n", serverAddr.sin_port);
    if (ret < 0)
    {
        printc(RED, "[-]Error in connection with Naming server.\n");
    }
    printc(GREEN, "[+]Connected to Storage Server on Signal Socket.\n");
    int val = 4;
    char buffer[MAX_SIZE];
    while (1)
    {
        usleep(100000);
        int r = recv(signalsocket, (int *)&val, MAX_SIZE, 0);
        if (r < 0)
        {
            printc(RED, "Error in recieving signal for Storage Server Disconnection.\n");
        }
        // printc(GREEN, "rec %d ", val);
        if (val == 1)
        {
            printc(ORANGE, "Storage Server disconnected with %d.\n", servers_info[index].is_backup_1);
            servers_info[index].on = 0;
            // check if this was our backup server or not
            if (servers_info[index].is_backup_1 == 1)
            {
                printc(YELLOW, "\tBackup Server 1 has been disconnected.\n");
                server_socket_backup_1 = -1;
            }
            if (servers_info[index].is_backup_2 == 1)
            {
                printc(YELLOW, "\tBackup Server 2 has been disconnected.\n");
                server_socket_backup_2 = -1;
            }
            delete_storage_server_paths(index);

            break;
        }
    }
}

void *ss_thread_handler(void *arg)
{
    // struct socket_addr_in new_addr=*(struct socket_addr)
    // pthread_mutex_lock(&sslock);
    struct server_initialisation_message st;

    struct pass_to_thread pt = *((struct pass_to_thread *)(arg));
    int newSocket = pt.newsocket;
    struct sockaddr_in newAddr = pt.addr;
    int index = pt.index;
    recv(newSocket, (struct server_initialisation_message *)&st, sizeof(struct server_initialisation_message), 0);

    servers_info[index].num_paths = st.num_paths;
    servers_info[index].port_number_for_naming_server = st.port_number_for_naming_server;
    servers_info[index].port_number_for_client = st.port_number_for_client;
    servers_info[index].socket_number = newSocket; // this enables to send data anytime in any thread to the server
    servers_info[index].on = 1;
    for (int i = 0; i < st.num_paths; i++)
    {
        char buff[MAX_SIZE];
        int r = recv(newSocket, (char *)&buff, sizeof(buff), 0);
        if (r < 0)
        {
            // perror("error in recieving paths ind");
            printc(RED, "Error in recieving accessible paths.\n");
        }
        printc(BLUE, "%s\n", buff);
        // write_to_book("%s\n",buff);

        if (strcmp(buff, "Backup1") == 0)
        {
            // this denotes this server is backup 1
            printf("Backup1 connected\n");
            server_socket_backup_1 = newSocket;
            servers_info[index].is_backup_1 = 1;
            insert(Trie_backup_1, buff, strlen(buff), index);
            insert(Trie, buff, strlen(buff), index);
        }
        else if (strcmp(buff, "Backup2") == 0)
        {
            // this denotes this server is backup 2
            printf("Backup2 connected\n");
            server_socket_backup_2 = newSocket;
            servers_info[index].is_backup_2 = 1;
            printf("%d\n", servers_info[index].is_backup_1);
            insert(Trie_backup_2, buff, strlen(buff), index);
            insert(Trie, buff, strlen(buff), index);
        }
        else
        {
            servers_info[index].is_backup_1 = 0;
            insert(Trie, buff, strlen(buff), index);
        }

        strcpy(servers_info[index].accessible_paths[i], (buff));
    }
    strcpy(servers_info[index].storage_server_ip, st.storage_server_ip);
    // now take backup

    int port = generate_random_port();
    send(newSocket, (int *)&port, sizeof(int), 0);
    printf("the sent socket num is %d.\n", port);

    // bool backup_exists = false;
    // for (int i = 0; i < backup_server_parent_paths_size; i++)
    // {
    //     /* code */
    //     char *temp = strdup(backup_server_parent_paths[i]);
    //     if (strcmp(temp, servers_info[index].main_parent_directory_path) == 0)
    //     {
    //         printc(BLUE, "Backup Exists\n");
    //         backup_exists = true;
    //         // int result1 = sync_storage_server(temp);
    //         // if (result1 == -1)
    //         // {
    //         //     printc(RED, "No Backup Servers are connected.\n");
    //         // }
    //         // else
    //         // {
    //         //     printc(GREEN, "Storage Server synced.\n");
    //         // }
    //         break;
    //     }
    // }
    // if (!backup_exists)
    // {
        // printc(BLUE, "Backup Doesn't Exists\n");
        if (servers_info[index].is_backup_1 == 0 && servers_info[index].is_backup_2 == 0)
        {
            backup_server_data(index);
            backup_server_parent_paths[backup_server_parent_paths_size++] = (char *)malloc(sizeof(char) * MAX_SIZE);
            strcpy(backup_server_parent_paths[backup_server_parent_paths_size - 1], servers_info[index].main_parent_directory_path);
        }
    // }

    signalfunction(newSocket, port, index);
    // printc(ORANGE, "brooo\n");
}

void *connect_clients_thread(void *arg)
{
    int sockfd, ret;
    struct sockaddr_in serverAddr;

    int newSocket;
    struct sockaddr_in newAddr;

    socklen_t addr_size;

    char buffer[MAX_SIZE];
    pid_t childpid;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        // printf("[-]Error in connection.\n");
        printc(RED, "Error in creating sockfd for client connection.\n"); // exit(1);
    }
    // printf("[+]Server Socket is created.\n");
    printc(GREEN, "[+]Server Socket is created.\n");
    write_to_book("[+]Server Socket is created.\n");
    memset(&serverAddr, '\0', sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(CLIENT_PORT);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    ret = bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (ret < 0)
    {
        // printf("[-]Error in binding.\n");
        printc(RED, "Error in binding.\n");
        // exit(1);
    }
    // printf("[+]Bind to CLIENT_PORT %d\n", CLIENT_PORT);
    printc(GREEN, "[+]Bind successful to CLIENT_PORT %d\n", CLIENT_PORT);
    write_to_book("[+]Bind successful to CLIENT_PORT %d\n", CLIENT_PORT);
    if (listen(sockfd, 10) == 0)
    {
        // printf("[+]Listening for client....\n");
        printc(GREEN, "[+]Listening for client....\n");
        write_to_book("[+]Listening for client....\n");
    }
    else
    {
        // printf("[-]Error in binding.\n");
        printc(RED, "Error in binding with client port.\n");
    }

    int current_client = 0;
    pthread_t client_threads[MAX_SIZE];

    while (1)
    {
        // printf("in while loop");
        sleep(1);
        newSocket = accept(sockfd, (struct sockaddr *)&newAddr, &addr_size);
        if (newSocket < 0)
        {
            // exit(1);
        }
        // printf("Connection accepted from %s:%d\n", inet_ntoa(newAddr.sin_addr), ntohs(newAddr.sin_CLIENT_PORT));
        // printf("Connection accepted from %d\n", newAddr.sin_port);
        printc(GREEN, "[+]Client Connection accepted from Port Number: %d\n", newAddr.sin_port);
        write_to_book("[+]Client Connection accepted from Port Number: %d\n", newAddr.sin_port);
        struct pass_to_thread pt;
        pt.addr = newAddr;
        pt.newsocket = newSocket;
        pt.index = current_client;
        int response = pthread_create(&client_threads[current_client], NULL, client_thread_handler, (struct pass_to_thread *)&pt);
        if (response < 0)
        {
            // perror("Thread not created");
            printc(RED, "Client Handler thread not created.\n");
        }
        // pthread_join(client_threads)
        current_client++;
        number_of_clients++;
    }
    for (int i = 0; i < number_of_clients; i++)
    {
        pthread_join(client_threads[i], NULL);
    }

    close(newSocket);
}
void *connect_ss_thread(void *arg)
{
    int sockfd, ret;
    struct sockaddr_in serverAddr;

    int newSocket;
    struct sockaddr_in newAddr;

    socklen_t addr_size;

    char buffer[MAX_SIZE];
    pid_t childpid;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    serversockfd = sockfd;
    if (sockfd < 0)
    {
        // printf("[-]Error in ss.\n");
        printc(RED, "Error in creating sockfd for storage server connection.\n");
        // exit(1);
    }
    // printf("[+]Server Socket is created in ss.\n");
    printc(GREEN, "[+]Server Socket is created in ss.\n");
    write_to_book("[+]Server Socket is created in ss.\n");
    memset(&serverAddr, '\0', sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    ret = bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (ret < 0)
    {
        // printf("[-]Error in binding ss.\n");
        printc(RED, "Error in binding Storage server socket.\n");
        // exit(1);
    }
    // printf("[+]Bind to CLIENT_PORT in ss %d\n", SERVER_PORT);
    printc(GREEN, "[+]Bind successful to CLIENT_PORT in ss %d\n", SERVER_PORT);
    write_to_book("[+]Bind successful to CLIENT_PORT in ss %d\n", SERVER_PORT);
    if (listen(sockfd, 10) == 0)
    {
        // printf("[+]Listening in ss....\n");
        printc(GREEN, "[+]Listening for ss....\n");
        write_to_book("[+]Listening for ss....\n");
    }
    else
    {
        // printf("[-]Error in binding ss.\n");
        printc(RED, "Error in binding with Storage server port.\n");
    }

    int current_ss = 0;
    pthread_t ss_threads[MAX_SIZE];

    while (1)
    {
        // printf("in while loop");
        newSocket = accept(sockfd, (struct sockaddr *)&newAddr, &addr_size);
        if (newSocket < 0)
        {
            // exit(1);
            printc(RED, "Error in accepting storage server connection request.\n");
        }
        // printf("Connection accepted from %s:%d\n", inet_ntoa(newAddr.sin_addr), ntohs(newAddr.sin_CLIENT_PORT));
        printc(GREEN, "Connection accepted from %d\n", newAddr.sin_port);
        write_to_book("Connection accepted from %d\n", newAddr.sin_port);
        struct pass_to_thread pt;
        pt.addr = newAddr;
        pt.newsocket = newSocket;
        pt.index = current_ss;
        int response = pthread_create(&ss_threads[current_ss], NULL, ss_thread_handler, (struct pass_to_thread *)&pt);

        if (response < 0)
        {
            // perror("Storage Server thread not created\n");
            printc(RED, "Storage Server thread not created\n");
        }
        // pthread_join(client_threads)
        current_ss++;
        number_of_ss++;
    }
    for (int i = 0; i < number_of_ss; i++)
    {
        pthread_join(ss_threads[i], NULL);
    }

    close(newSocket);
}

int main()
{
    backup_server_parent_paths = (char **)malloc(sizeof(char *) * MAX_SIZE);
    for (int i = 0; i < MAX_SIZE; i++)
    {
        backup_server_parent_paths[i] = (char *)malloc(sizeof(char) * MAX_SIZE);
    }

    Trie = createTrie();
    Trie_backup_1 = createTrie();
    Trie_backup_2 = createTrie();

    writeFiles = createTrie();
    sem_init(&writelock, 0, 1);
    insert(Trie, "hello", 5, 55);
    LRU = createLRUCache(10);
    pthread_t client_thread;

    int response = pthread_create(&client_thread, NULL, connect_clients_thread, NULL);
    if (response < 0)
    {
        // perror("Client Handler thread not created");
        printc(RED, "Client Handler thread not created");
    }

    pthread_t ss_thread;

    int response1 = pthread_create(&ss_thread, NULL, connect_ss_thread, NULL);

    if (response1 < 0)
    {
        // perror("Storage Server thread not created");
        printc(RED, "Storage Server thread not created");
    }
    sleep(5);
    pthread_join(client_thread, NULL);
    pthread_join(ss_thread, NULL);

    return 0;
}