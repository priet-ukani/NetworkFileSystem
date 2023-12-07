#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_SIZE 1024
#define PORT 44456
// change the port above if not connecting yeah bro

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

void printc(const char *color, const char *format, ...)
{
    va_list args;

    __builtin_va_start(args, format);
    printf("%s", color);
    vprintf(format, args);
    printf("%s", RESET);

    __builtin_va_end(args);
}

struct read_data
{
    char *buffer;
    int numchunks;
    int size;
};
void executeconnection(int ssportnum, char *buffer)
{
    char *buffer2 = strdup(buffer);
    int clientSocket, ret;
    struct sockaddr_in serverAddr;

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0)
    {
        printc(RED, "[-]Error in connection to Storage Server\n");
        exit(1);
    }
    printc(BLUE, "[+]Client Socket is created.\n");

    memset(&serverAddr, '\0', sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(ssportnum);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    ret = connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (ret < 0)
    {
        printc(RED, "[-]Error in connection in Client\n");
        // exit(1);
    }
    printc(GREEN, "[+]Connected to Storage Server.\n");
    int val = send(clientSocket, (char *)buffer2, MAX_SIZE, 0);
    // int val=send(clientSocket, (int *)&p, sizeof(p), 0);
    // int val=recv(clientSocket, (int*)&p, sizeof(int), 0);
    if (val < 0)
    {
        printc(RED, "not sent");
    }
    // printf("idhar");

    if (strstr(buffer, "READ"))
    {
        char *numchunks = (char *)malloc(sizeof(char) * 100);
        // char *completestr = (char *)malloc((1024 * 100) * sizeof(char));
        recv(clientSocket, (char *)numchunks, 100, 0);
        int chunks = atoi(numchunks);
        // printc(YELLOW,"No of Chunks of Data Obtained from Server: %d\n", chunks);
        for (int i = 0; i < chunks; i++)
        {
            char *buffer8 = (char *)malloc(sizeof(char) * MAX_SIZE);
            memset(buffer8, 0, MAX_SIZE);
            int val = recv(clientSocket, (char *)buffer8, MAX_SIZE, 0);
            // printf("after size of buf8 :%d ",siz(buffer8));
            if (val < 0)
            {
                // perror("recieve error");
                printc(RED, "recieve error");
            }
            else
            {
                // printf("string char received %d\n",val);
            }
            // strcat(completestr, buffer);
            printc(YELLOW, "%s ", buffer8);
            free(buffer8);
        }
        printf("\n");
        // printf("File contents: %s\n", completestr);
    }
    else if (strstr(buffer, "WRITE"))
    {
        bzero(buffer, 1024);
        // recv(clientSocket, buffer, 1024, 0);
    }
    else if (strstr(buffer, "GET"))
    {
        bzero(buffer, 1024);
        recv(clientSocket, (char *)buffer, 1024, 0);
        printc(MAGENTA, "File information: %s\n", buffer);
    }
    else
    {
        printc(RED, "Invalid command\n");
    }
    close(clientSocket);
}

char buffer[MAX_SIZE];
int main()
{
    int clientSocket, ret;
    struct sockaddr_in serverAddr;

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0)
    {
        printc(RED, "[-]Error in connection in Client\n");
        exit(1);
    }
    printc(BLUE, "[+]Client Socket is created.\n");

    memset(&serverAddr, '\0', sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);

    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    ret = connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr));

    if (ret < 0)
    {
        printc(RED, "[-]Error in connection in with NM\n");
        // exit(1);
    }
    printc(GREEN, "[+]Connected to Naming Server.\n");

    while (1)
    {
        printc(ORANGE, "Client: \t");
        fgets(buffer, sizeof(buffer), stdin);
        send(clientSocket, buffer, strlen(buffer), 0);

        if (strcmp(buffer, ":exit") == 0)
        {
            close(clientSocket);
            printc(RED, "[-]Disconnected from Naming Server.\n");
            exit(1);
        }
        if (strstr(buffer, "CREATE") != NULL || strstr(buffer, "DELETE") != NULL || strstr(buffer, "COPY") != NULL)
        {
            continue;
        }
        else
        {
            int ssportnum;
            if (recv(clientSocket, (int *)&ssportnum, sizeof(int), 0) < 0)
            {
                printc(RED, "[-]Error in receiving data from Naming Server.\n");
            }

            // Distinct error codes for client requests from naming server
            if (ssportnum == -1)
            {
                printc(RED, "Invalid path.\n");
                continue;
            }
            else if (ssportnum == -200)
            {
                // this is timeout for write operation
                printc(RED, "Timeout for write operation.\n");
                continue;
            }
            else if (ssportnum == -100)
            {
                // this is for invalid operation
                printc(RED, "Invalid operation.\n");
                continue;
            }

            // printc(GREEN, "port num received from naming serever: %d\n", ssportnum);
            sleep(1);
            executeconnection(ssportnum, buffer);
            // printf("Naming Server: \t%s\n", buffer);
        }
    }

    return 0;
}