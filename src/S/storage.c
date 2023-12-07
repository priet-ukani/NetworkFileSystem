#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <threads.h>
#include <ctype.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <pthread.h>
#include <time.h>
#include <stdbool.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/select.h>
#include <signal.h>

#define RED "\x1b[31m"
#define GREEN "\x1b[32m"
#define RESET "\x1b[0m"
#define YELLOW "\x1b[33m"
#define BLUE "\x1b[34m"
#define CYAN "\x1b[36m"
#define ORANGE "\033[38;5;208m"
#define WHITE "\x1b[37m"
#define MAGENTA "\x1b[35m"
#define YELLOW_BOLD "\x1b[1;33m"
#define CYAN_BOLD "\x1b[1;36m"
#define WHITE_BOLD "\x1b[1;37m"
#define BLACK_BACKGROUND "\x1b[40m"
#define RED_BACKGROUND "\x1b[41m"
#define GREEN_BACKGROUND "\x1b[42m"
#define YELLOW_BACKGROUND "\x1b[43m"
#define BLUE_BACKGROUND "\x1b[44m"
#define MAGENTA_BACKGROUND "\x1b[45m"
#define CYAN_BACKGROUND "\x1b[46m"
#define WHITE_BACKGROUND "\x1b[47m"
#define BOLD "\x1b[1m"
#define UNDERLINE "\x1b[4m"
#define REVERSED "\x1b[7m"

#define PORT 55587
#define MAX_ENTRIES 100
#define MAX_SIZE 1024

int client_connection_port_number;
int nmserversignalport;
int ssSocket;
int acceptnmsignalport;
char currentPath[MAX_SIZE];
char newpath[MAX_SIZE];

struct server_initialisation_message
{
    char storage_server_ip[32];
    int port_number_for_naming_server;
    int port_number_for_client;
    int numpaths;
    // char accessible_paths[MAX_SIZE][MAX_SIZE];
};

struct read_data
{
    char *buffer;
    int numchunks;
    int size;
};

void ctrlC(int signal)
{
    if (signal == SIGINT)
    {
        int e = 1;
        ssize_t bytesSent = send(acceptnmsignalport, (int *)&e, sizeof(int), 0);
        if (bytesSent > 0)
        {
            exit(1);
        }
    }
}

void printc(const char *color, const char *format, ...)
{
    va_list args;

    va_start(args, format);
    printf("%s", color);
    vprintf(format, args);
    printf("%s", RESET);

    va_end(args);
}

char *subtractSubstring(const char *original, const char *substring)
{
    char *result;
    const char *foundSubstring = strstr(original, substring);

    if (foundSubstring == NULL)
    {
        // Substring not found, return a copy of the original string
        result = strdup(original);
    }
    else
    {
        // Calculate the length of the new string
        size_t originalLength = strlen(original);
        size_t substringLength = strlen(substring);
        size_t newStringLength = originalLength - substringLength;

        // Allocate memory for the new string
        result = (char *)malloc(newStringLength + 1);

        if (result != NULL)
        {
            // Copy the part before the substring
            strncpy(result, original, foundSubstring - original);

            // Copy the part after the substring
            strcpy(result + (foundSubstring - original), foundSubstring + substringLength);
        }
    }

    return result;
}

char *information_about_file(char *dir_path)
{
    // struct dirent *dir_info;
    struct stat fileinfo;
    if (stat(dir_path, &fileinfo) == -1)
    {
        // printf("Stat not working\n");
        printc(RED, "Stat not working\n");
        return NULL;
    }

    // Allocate memory for the result string
    char *result = (char *)malloc(MAX_SIZE); // Adjust the size as needed

    sprintf(result, "%s%s%s%s%s%s%s%s%s%s",
            (S_ISDIR(fileinfo.st_mode)) ? "d" : "-",
            (fileinfo.st_mode & S_IRUSR) ? "r" : "-",
            (fileinfo.st_mode & S_IWUSR) ? "w" : "-",
            (fileinfo.st_mode & S_IXUSR) ? "x" : "-",
            (fileinfo.st_mode & S_IRGRP) ? "r" : "-",
            (fileinfo.st_mode & S_IWGRP) ? "w" : "-",
            (fileinfo.st_mode & S_IXGRP) ? "x" : "-",
            (fileinfo.st_mode & S_IROTH) ? "r" : "-",
            (fileinfo.st_mode & S_IWOTH) ? "w" : "-",
            (fileinfo.st_mode & S_IXOTH) ? "x" : "-");

    sprintf(result + strlen(result), " %lld", (long long)fileinfo.st_size);

    return result;
}

void *storageserverfunction(void *arg)
{
    char *portstring = (char *)arg;
    int port_for_client_connection = atoi(portstring);

    int sockfd, ret;
    struct sockaddr_in serverAddr;

    int newSocket;
    struct sockaddr_in newAddr;

    socklen_t addr_size;

    pid_t childpid;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        printc(RED, "[-]Error in connection.\n");
        // exit(1);
        return NULL;
    }
    printc(BLUE, "[+]Socket is created for client interaction.\n");

    memset(&serverAddr, '\0', sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port_for_client_connection);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    ret = bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (ret < 0)
    {
        // printf("[-]Error in binding.\n");
        printc(RED, "[-]Error in binding.\n");
        return NULL;
        // exit(1);
    }
    // printf("[+]Bind to CLIENT_PORT %d\n", port_for_client_connection);

    if (listen(sockfd, 10) == 0)
    {
        // printf("[+]Listening....\n");
        printc(GREEN, "[+]Listening....\n");
    }
    else
    {
        printc(RED, "[-]Error in listening.\n");
        // printf("[-]Error in listening.\n");
    }
    char buffer[MAX_SIZE];
    // while (1)
    // {
    bzero(buffer, sizeof(buffer));
    int resp = accept(sockfd, (struct sockaddr *)&newAddr, &addr_size);
    if (resp < 0)
    {
        // perror("Socket accepting error");
        printc(RED, "Socket accepting error");
        // exit(1);
    }
    // printf("Connection accepted from %d\n", newAddr.sin_port);
    printc(GREEN, "[+]Connection accepted from %d\n", newAddr.sin_port);
    bzero(buffer, sizeof(buffer));
    int test = 0;
    int rsp = recv(resp, (char *)&buffer, sizeof(buffer), 0);

    if (rsp < 0)
    {
        // perror("error in receiving input");
        printc(RED, "Error in recieving input from naming server.\n");
        // continue;
    }
    // printc(MAGENTA, "Input recieved: %s\n Size: %d\n", buffer, rsp); // continue;
    char input[MAX_SIZE];
    char cond[MAX_SIZE];
    char path[MAX_SIZE];
    strcpy(input, buffer);
    char *token = strtok(input, " \n");
    if (token != NULL)
    {
        strcpy(cond, token);
    }
    else
    {
        return NULL;
    }
    token = strtok(NULL, " \n");
    if (token != NULL)
    {
        strcpy(path, newpath);
        strcpy(path, token);
    }
    else
    {
        return NULL;
    }

    if (strcmp(cond, "READ") == 0)
    { // for read
        // printf("OK BRO SENDING NOW\n");
        FILE *file = fopen(path, "r");
        if (file == NULL)
        {
            printc(RED, "The given file path does not exist.\n");
            // perror("Following file do not exists\n");
            return NULL;
        }
        char buffer1[MAX_SIZE];
        size_t bytesRead;
        int totalChunks = 0;
        while ((bytesRead = fread(buffer1, 1, sizeof(buffer1), file)) > 0)
        {
            totalChunks++;
        }
        // printf("tottal chnks %d\n", totalChunks);
        // printc(MAGENTA, "Total chunks to be sent: %d\n", totalChunks);
        char *num = (char *)malloc(sizeof(char) * 100);
        sprintf(num, "%d", totalChunks);
        if (send(resp, (char *)num, 100, 0) < 0)
        {
            // perror("Error sending total number of chunks\n");
            printc(RED, "Error sending total number of chunks\n");
            return NULL;
        }
        // printf("send done1\n");
        bzero(buffer1, sizeof(buffer1));
        // rewind(file);
        if (fseek(file, 0, SEEK_SET) != 0)
        {
            // perror("Error rewinding the file");
            printc(RED, "Error rewinding the file");
            fclose(file);
        }
        int count = 0;
        // char buffermain[totalChunks][MAX_SIZE];
        char buffermain[MAX_SIZE];
        while ((bytesRead = fread(buffermain, sizeof(char) * 1, MAX_SIZE, file)) > 0)
        {
            // printf("Read from file:  %s \n", buffermain); // comment this afterwards
            ssize_t bytesSent = send(resp, (char *)buffermain, sizeof(buffermain), 0);
            // printf("Number of bytes send %ld\n", bytesSent); // comment this afterwards
            if (bytesSent < 0)
            {
                // perror("Error sending data");
                printc(RED, "Error sending chunk of data to the client.\n");
            }
            bzero(buffermain, sizeof(buffermain));
            count++;
        }

        fclose(file);
        close(resp);
    }
    else if (strcmp(cond, "GET") == 0)
    {
        FILE *file = fopen(path, "r");
        if (file == NULL)
        {
            // perror("Following file do not exists\n");
            printc(RED, "No file exists on the given file path\n");
            return NULL;
        }
        char *buffer3 = (char *)malloc(sizeof(char) * MAX_SIZE);
        buffer3 = information_about_file(path);
        if (send(resp, (char *)buffer3, MAX_SIZE, 0) < 0)
        {
            printc(RED, "Error sending file get results- permission and size to the client.\n");
            // perror("Error sending file permission, size\n");
            return NULL;
        }
        fclose(file);
        close(resp);
    }
    else if (strcmp(cond, "WRITE") == 0)
    {
        /// DO NOT REMOVE THIS
        FILE *file = fopen(path, "w");

        if (file == NULL)
        {
            // perror("Error opening file");
            printc(RED, "Error opening file for write operation.\n");
            return NULL;
        }
        const char *position = buffer;
        for (int i = 0; i < 2; ++i)
        {
            while (*position && isspace(*position))
            {
                fputc(*position, file);
                ++position;
            }
            while (*position && !isspace(*position))
            {
                ++position;
            }
        }

        // Write the remaining content to the file
        fputs(position, file);

        // Close the file
        fclose(file);
    }

    return NULL;
}

void listFilesAndDirectories(char path[MAX_SIZE], char entries[MAX_ENTRIES][MAX_SIZE], int *count, char newpath[MAX_SIZE])
{
    DIR *dir;
    struct dirent *entry;
    struct stat statbuf;
    // printf("path is %s and newpath is %s\n",path,newpath);
    dir = opendir(path);
    if (dir == NULL)
    {
        // perror("Error opening directory");
        printc(RED, "Error opening directory for listing all file paths.\n");
    }

    while ((entry = readdir(dir)) != NULL)
    {
        // Build the full path
        char fullpath[MAX_SIZE];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);

        if (stat(fullpath, &statbuf) == -1)
        {
            // perror("Error getting file status");
            printc(RED, "Error getting file status.\n");
        }
        if (S_ISDIR(statbuf.st_mode))
        {
            // Ignore "." and ".." directories
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
            {
                // printf("full path is %s\n");
                char *pushstr = subtractSubstring(fullpath, newpath);
                strcpy(entries[*count], pushstr);

                // printf("pushed %s\n", pushstr);
                (*count)++;
                // Recursively list files and directories in subdirectories
                listFilesAndDirectories(fullpath, entries, count, newpath);
            }
        }
        else if (S_ISREG(statbuf.st_mode))
        {
            // printc(GREEN,"full path is %s\n",fullpath);
            // printc(RED,"newpath is path is %s\n",newpath);

            char *pushstr = subtractSubstring(fullpath, newpath);
            strcpy(entries[*count], pushstr);
            // printf("pushed %s\n", pushstr);
            (*count)++;
        }
    }
    closedir(dir);
}

void copy_another_serverfile(char *path1)
{
    int flag = 0;
    send(ssSocket, (int *)&flag, sizeof(int), 0); // send flag for file
    // printc(GREEN, "sent flag %d\n", flag);
    char *relativepath = subtractSubstring(path1, newpath);
    send(ssSocket, (char *)relativepath, MAX_SIZE, 0); // send relative path of the file
    // printc(GREEN, "sent file name %s.\n", relativepath);

    FILE *file1 = fopen(path1, "r");
    char *copypath = strdup(path1);

    if (file1 == NULL)
    {
        printc(RED, "Unable to open source file1\n");
        return;
    }
    char buffer[MAX_SIZE];
    while (fgets(buffer, sizeof(buffer), file1) != NULL) // read data of the file
    {
        send(ssSocket, (char *)&buffer, MAX_SIZE, 0); // send data of the file
        // printc(YELLOW, "sent data %s.\n", buffer);
        // printc(RED, "sent %s\n", buffer);
        bzero(buffer, sizeof(buffer));
    }
    // printc(MAGENTA, "Copied file %s to the %s directory.\n", filename, path2);
    strcpy(buffer, "END");
    // printc(GREEN, "sent end.\n");
    send(ssSocket, (char *)&buffer, MAX_SIZE, 0); // send end to file
    fclose(file1);
}

void copy_anotherserver_Directory(const char *sourceDir)
{
    int flag = 1;
    send(ssSocket, (int *)&flag, sizeof(int), 0); // send that this is a directory
    // printc(GREEN, " sent flag %d\n", flag);
    // char relativepath[MAX_SIZE];
    char *relativepath = subtractSubstring(sourceDir, newpath);
    send(ssSocket, (char *)relativepath, MAX_SIZE, 0);
    // printc(RED, "sent relative path %s\n", relativepath);
    DIR *source = opendir(sourceDir);
    struct dirent *entry;
    struct stat statBuf;

    while ((entry = readdir(source)) != NULL)
    {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
        {
            char sourcePath[MAX_SIZE];
            snprintf(sourcePath, sizeof(sourcePath), "%s/%s", sourceDir, entry->d_name);
            // Get information about the entry
            stat(sourcePath, &statBuf);
            if (S_ISDIR(statBuf.st_mode))
            {
                copy_anotherserver_Directory(sourcePath);
            }
            else
            {
                copy_another_serverfile(sourcePath);
            }
        }
    }
    closedir(source);
}

void copyDirectory(const char *sourceDir, const char *destDir)
{
    DIR *source = opendir(sourceDir);
    struct dirent *entry;
    struct stat statBuf;

    // Create a new directory in the destination with the same name as the source directory
    char newDestDir[MAX_SIZE];
    snprintf(newDestDir, sizeof(newDestDir), "%s/%s", destDir, strrchr(sourceDir, '/') + 1);
    mkdir(newDestDir, 0777);

    while ((entry = readdir(source)) != NULL)
    {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
        {
            char sourcePath[MAX_SIZE];
            char destPath[MAX_SIZE];
            snprintf(sourcePath, sizeof(sourcePath), "%s/%s", sourceDir, entry->d_name);
            snprintf(destPath, sizeof(destPath), "%s/%s", newDestDir, (char*)entry->d_name);

            // Get information about the entry
            stat(sourcePath, &statBuf);

            if (S_ISDIR(statBuf.st_mode))
            {
                // If it's a directory, recursively copy it
                copyDirectory(sourcePath, newDestDir);
            }
            else
            {
                // If it's a file, copy it
                FILE *sourceFile = fopen(sourcePath, "rb");
                FILE *destFile = fopen(destPath, "wb");

                if (sourceFile && destFile)
                {
                    char buffer[4096];
                    size_t bytesRead;

                    while ((bytesRead = fread(buffer, 1, sizeof(buffer), sourceFile)) > 0)
                    {
                        fwrite(buffer, 1, bytesRead, destFile);
                    }

                    fclose(sourceFile);
                    fclose(destFile);
                }
            }
        }
    }
    closedir(source);
}

void copysend(int socket)
{
    // function to send all files for copying OK
}


void copy_recieve(char RECVPATH[MAX_SIZE], char RPATH[MAX_SIZE])
{
    while (1)
    { // function to recieve all files for copying OK
        int flag;
        recv(ssSocket, (int *)&flag, sizeof(int), 0); // recieve flag for file or directory
        // printc(GREEN, "recvd flag: %d", flag);
        if (flag == -100)
        {
            // denotes completed copying
            printc(GREEN, "Completed copying.\n");
            break;
        }
        if (flag == 0)
        {
            // this is a file
            // recieve the name of the file
            char buffer[MAX_SIZE];
            recv(ssSocket, (char *)&buffer, MAX_SIZE, 0); // this is the name
            // printc(GREEN, "recvd name %s\n", buffer);
            char path[MAX_SIZE];
            strcpy(path, RECVPATH);
            // strcat(path, buffer);
            char *QPATH = subtractSubstring(buffer, RPATH);
            if (strcmp(QPATH, "") == 0)
            {
                strcat(path, buffer);
            }
            else
            {
                strcat(path, QPATH);
            }
            // printc(YELLOW, "new path to access: %s\n", path);
            FILE *file1 = fopen(path, "w");
            if (file1 == NULL)
            {
                printc(RED, "Unable to create file in the destination\n");
                return;
            }
            char buffer1[MAX_SIZE];
            while (1)
            {
                bzero(buffer1, sizeof(buffer1));
                recv(ssSocket, (char *)&buffer1, MAX_SIZE, 0); // recieve data of the file
                // printc(YELLOW, "recieved data from server%s\n", buffer1);
                if (strcmp(buffer1, "END") == 0)
                {
                    break;
                }
                fputs(buffer1, file1);
                // fwrite(buffer1, 1, sizeof(buffer1), file1);
                // bzero(buffer, sizeof(buffer));
            }
            fclose(file1);
        }
        else
        {
            // getname of dir
            char buffer[MAX_SIZE];
            recv(ssSocket, (char *)&buffer, MAX_SIZE, 0); // this is the name of directory
            char path[MAX_SIZE];
            strcpy(path, RECVPATH);
            // strcat(path, buffer);
            char *QPATH = subtractSubstring(buffer, RPATH);
            strcat(path, QPATH);
            // printf("Directory created: %s\n", path);
            mkdir(path, 0777); // this creates dir
        }
    }
}

void copyfile(char *path1, char *path2)
{
    FILE *file1 = fopen(path1, "r");
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

    FILE *file2 = fopen(copypath2, "w");
    if (file1 == NULL)
    {
        printc(RED, "Unable to open source file1\n");
        return;
    }
    if (file2 == NULL)
    {
        printc(RED, "Unable to open destination file2\n");
        return;
    }
    char buffer[MAX_SIZE];
    while (fgets(buffer, sizeof(buffer), file1) != NULL)
    {
        fputs(buffer, file2);
    }
    printc(GREEN, "Copied file %s to the %s directory.\n", filename, path2);
    fclose(file1);
    fclose(file2);
}
void createfile(char *path, char *newfileorpath, char *flag)
{
    chdir(path);
    if (strcmp(flag, "-d") == 0)
    {
        strcat(path, "/");
        strcat(path, newfileorpath);
        // printf("%s final path\n", path);
        mkdir(newfileorpath, 0777);
        printc(GREEN, "Created directory %s\n", newfileorpath);
    }
    else if (strcmp(flag, "-f") == 0)
    {
        strcat(path, "/");
        strcat(path, newfileorpath);
        FILE *file = fopen(newfileorpath, "w");
        if (file == NULL)
        {
            printc(RED, "Unable to create file\n");
        }
        else
        {
            printc(GREEN, "File %s created successfully\n", newfileorpath);
        }
        fclose(file);
    }
    else
    {
        printc(RED, "Invalid flag given in write operation\n");
        // printf("Invalid flag\n");
    }
}
void deletefile(char *path)
{
    // printf("%s\n", path);
    if (remove(path) == 0)
    {
        // printf("File deleted successfully\n");
        printc(GREEN, "File deleted successfully.\n");
    }
    else
    {
        // printf("Unable to delete the file\n");
        printc(RED, "Unable to delete the file.\n");
    }
}

void deleteDirectory(const char *path)
{
    DIR *dir = opendir(path);
    struct dirent *entry;
    struct stat statBuf;

    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
        {
            char entryPath[MAX_SIZE];
            snprintf(entryPath, sizeof(entryPath), "%s/%s", path, entry->d_name);

            // Get information about the entry
            stat(entryPath, &statBuf);

            if (S_ISDIR(statBuf.st_mode))
            {
                // If it's a directory, recursively delete it
                deleteDirectory(entryPath);
            }
            else
            {
                // If it's a file, delete it
                if (remove(entryPath) != 0)
                {
                    // perror("Error deleting file");
                    printc(RED, "Error deleting file.\n");
                }
            }
        }
    }

    closedir(dir);

    // Delete the empty directory itself
    if (rmdir(path) != 0)
    {
        // perror("Error deleting directory");
        printc(RED, "Error deleting directory.\n");
    }
}

int NMcommands(char *arr)
{
    char *input = strdup(arr);
    char *tok = strtok(input, " \n");
    if (strcmp(tok, "COPY") == 0)
    {
        tok = strtok(NULL, " \n");
        char *path1 = strdup(tok);
        tok = strtok(NULL, " \n");
        char *path2 = strdup(tok);

        tok = strtok(NULL, " \n");
        if (tok != NULL)
        {
            printc(RED, "Invalid input format. Extra arguments sent.\n");
            return -1;
        }
        char abs_path1[MAX_SIZE];
        strcpy(abs_path1, newpath);
        strcat(abs_path1, path1);

        char abs_path2[MAX_SIZE];
        strcpy(abs_path2, newpath);
        strcat(abs_path2, path2);

        struct stat path_stat;

        // Use stat to get information about the file or directory
        if (stat(abs_path1, &path_stat) == 0)
        {
            if (S_ISDIR(path_stat.st_mode))
            {
                printc(MAGENTA, "Copying Directory ....\n");
                copyDirectory(abs_path1, abs_path2);
                printc(MAGENTA, "Copied directory.\n");
                return 1;
            }
            else
            {
                printc(MAGENTA, "Copying file.....\n");
                copyfile(abs_path1, abs_path2);
                printc(MAGENTA, "COPIED file.\n");

                return 1;
            }
        }
        else
        {
            printc(RED, "Error getting file/directory information.\n");
            return -1;
        }
        return 1;
    }
    else if (strcmp(tok, "COPYSEND") == 0)
    {
        // printc(GREEN, "copy send.\n");
        tok = strtok(NULL, " \n");
        char *path1 = strdup(tok);

        char abs_path1[MAX_SIZE];
        strcpy(abs_path1, newpath);
        strcat(abs_path1, path1);

        struct stat path_stat;
        if (stat(abs_path1, &path_stat) == 0)
        {
            if (S_ISDIR(path_stat.st_mode))
            {
                // printc(MAGENTA, "Copying Directory ....\n");
                copy_anotherserver_Directory(abs_path1);
            }
            else
            {
                // printc(MAGENTA, "Copying file.....\n");
                copy_another_serverfile(abs_path1);
            }
        }
        int breakflag = -100;
        send(ssSocket, (int *)&breakflag, sizeof(int), 0);
        return 1;
    }
    else if (strcmp(tok, "COPYRECV") == 0)
    {
        printc(GREEN, "copy recv.\n");
        char path[MAX_SIZE];
        tok = strtok(NULL, " \n");
        strcpy(path, newpath);
        strcat(path, tok);
        strcat(path, "/");
        // printf("path is %s\n", path);
        char rpath[MAX_SIZE];
        tok = strtok(NULL, " \n");
        strcpy(rpath, tok);
        copy_recieve(path, rpath);
        return 1;
    }
    else if (strcmp(tok, "CREATE") == 0)
    {
        char abs_path[MAX_SIZE];
        strcpy(abs_path, newpath);
        char *addpath = (char *)malloc(sizeof(char) * strlen(tok) + 1);
        tok = strtok(NULL, " \n");
        strcpy(addpath, tok);
        strcat(abs_path, addpath);

        char *newfileorpath = (char *)malloc(sizeof(char) * strlen(tok) + 1);
        tok = strtok(NULL, " \n");
        strcpy(newfileorpath, tok);

        char *flag = (char *)malloc(sizeof(char) * strlen(tok) + 1);
        tok = strtok(NULL, " \n");
        strcpy(flag, tok);

        createfile(abs_path, newfileorpath, flag);
        return 1;
    }
    else if (strcmp(tok, "DELETE") == 0)
    {
        tok = strtok(NULL, " \n");
        char *path = (char *)malloc(sizeof(char) * strlen(tok) + 1);
        strcpy(path, tok);

        char abs_path[MAX_SIZE];
        strcpy(abs_path, newpath);
        strcat(abs_path, path);

        struct stat path_stat;

        // Use stat to get information about the file or directory
        if (stat(abs_path, &path_stat) == 0)
        {
            if (S_ISDIR(path_stat.st_mode))
            {
                printc(MAGENTA, "Deleting directory ....\n");
                // copyDirectory(path1,path2);
                deleteDirectory(abs_path);
                printc(MAGENTA, "Deleted directory");
                return 1;
            }
            else
            {
                printc(MAGENTA, "Deleting file.....\n");
                deletefile(abs_path);
                printc(MAGENTA, "Deleted file.....\n");
                return 1;
            }
        }
        else
        {
            printc(RED, "Error getting file/directory information.\n");
            return -1;
        }
        return 1;
    }
    return 0;
}

struct send_server
{
    bool is_port; // 1 then port is sent
    int port_client_conn;
    char buffer[MAX_SIZE];
};

void *sendsignalnameserver()
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("Error in socket creation");
        exit(EXIT_FAILURE);
    }

    // Set up server address structure
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(nmserversignalport);

    // Bind the socket
    if (bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("Error in bind");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(sockfd, 5) < 0)
    {
        perror("Error in listen");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    // printf("Server is listening on port 8080...\n");

    // Accept incoming connection
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    int newSocket = accept(sockfd, (struct sockaddr *)&clientAddr, &clientLen);

    if (newSocket < 0)
    {
        perror("Error in accept");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    // printf("Connection accepted from %s:%d\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
    int e = 1;
    // char buff[1000]="Hydraskull";
    acceptnmsignalport = newSocket;
    while (1)
    {
    }
    close(newSocket);
    return NULL;
}

int main()
{
    signal(SIGINT, ctrlC);
    char entries[MAX_ENTRIES][MAX_SIZE];
    if (getcwd(currentPath, sizeof(currentPath)) == NULL)
    {
        printc(RED, "Error getting current directory.\n");
    }
    // char* newpath=strdup(currentPath);
    strcpy(newpath, currentPath);
    strcat(newpath, "/");
    // printc(RED,"sent is %s\n",newpath);
    int count = 0;
    listFilesAndDirectories(currentPath, entries, &count, newpath);

    int ret;
    struct sockaddr_in serverAddr;

    ssSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (ssSocket < 0)
    {
        printc(RED, "[-]Error in creation of socket for storage server.\n");
    }
    printc(BLUE, "[+]Storage Server Socket is created.\n");

    memset(&serverAddr, '\0', sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);

    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    ret = connect(ssSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr));

    // printf("PORT NUMBER: %d\n", serverAddr.sin_port);

    if (ret < 0)
    {
        printc(RED, "[-]Error in connection with Naming server.\n");
    }
    printc(BLUE, "[+]Connected to Naming Server.\n");
    // send storage init msg
    struct server_initialisation_message init_msg;
    strcpy(init_msg.storage_server_ip, "127.0.0.1");
    init_msg.port_number_for_naming_server = PORT;
    init_msg.port_number_for_client = 9999;
    init_msg.numpaths = count;
    int c = send(ssSocket, (struct server_initialisation_message *)&init_msg, sizeof(struct server_initialisation_message), 0);
    if (c < 0)
    {
        printc(RED, "Could not send init mesaage to Naming server\n");
    }
    for (int i = 0; i < count; i++)
    {
        // printf("size of entries[i]=%d",sizeof(entries[i]));
        int v = send(ssSocket, (char *)&entries[i], sizeof(entries[i]), 0);
        if (v < 0)
        {
            printc(RED, "Could not sent path of all files to Naming server in init message.\n");
        }
    }
    printc(GREEN, "Sent init message to Naming Server.\n");
    // char *number = (char *)malloc(sizeof(char) * 1024);
    int q;
    // int l = recv(ssSocket, (char *)number, 1024, 0);
    int l = recv(ssSocket, (int *)&q, sizeof(q), 0);
    if (l < 0)
    {
        printc(RED, "Didn't receive signal port number\n");
    }
    else
    {
        // nmserversignalport = atoi(number);
        nmserversignalport = q;
        // printf("hello guys %d\n", q);
        pthread_t signalthread;
        if (pthread_create(&signalthread, NULL, sendsignalnameserver, NULL) != 0)
        {
            printc(RED, "Error in creating signal thread,\n");
        }
    }
    while (1)
    {
        int client_port;
        char buff[MAX_SIZE];
        int resp = recv(ssSocket, (char *)&buff, sizeof(buff), 0);
        if (resp < 0)
        {
            printc(RED, "[-]Error in receiving data from Naming Server.\n");
        }
        else
        {
            if (strstr(buff, "CREATE") != NULL || strstr(buff, "DELETE") != NULL || strstr(buff, "COPY") != NULL || strstr(buff, "COPYRECV") != NULL || strstr(buff, "COPYSEND") != NULL)
            {
                int rsp = NMcommands(buff);
                // printc(GREEN, "OP is %s\n", buff);
                int r = -1;
                int m = send(ssSocket, (int *)&r, sizeof(int), 0);
                // int k = send(ssSocket, (int *)&rsp, sizeof(int), 0);
                // if (k < 0)
                // {
                //     printc(RED, "Could not send acknowledgement to the Naming server.\n");
                // }
                // else
                // {
                //     printc(GREEN, "Sent acknowledgement to the Naming Server.\n");
                // }
            }
            else
            {
                int portrec = atoi(buff);
                if (portrec == -200) // this is for the write timeout
                {
                    printc(RED, "Timeout recieved for WRITE function call.\n");
                    continue;
                }
                // printc(GREEN, "Port received %d\n", portrec);
                int rsp = 1;
                pthread_t storagefunctionthread; // Thread identifier
                if (pthread_create(&storagefunctionthread, NULL, storageserverfunction, buff) != 0)
                {
                    printc(RED, "Error in creating thread,\n");
                }
            }
        }
    }

    return 0;
}

// mere bharat ka baccha baccha ab jay shree ram bolega :)
// mere bharat ka baccha baccha ab jay shree ram bolega :)