#include "extra_functions.h"
#define MAX_PORT 80000
#define MIN_PORT 1500

#define RESET "\x1b[0m"
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
// Node structure for doubly linked list
typedef struct Node
{
    char *key;
    int value;
    struct Node *prev;
    struct Node *next;
} Node;

// LRUCache structure
typedef struct LRUCache
{
    int capacity;
    int size;
    Node *head;
    Node *tail;
    Node **map; // Hash map for quick lookup of nodes
} LRUCache;

void printc(const char *color, const char *format, ...)
{
    va_list args;

    __builtin_va_start(args, format);
    printf("%s", color);
    vprintf(format, args);
    printf("%s", RESET);

    __builtin_va_end(args);
}

int write_to_book(char *format, ...)
{
    va_list args;
    __builtin_va_start(args, format);
    FILE *fp;
    fp = fopen("book.txt", "a");
    if (fp == NULL)
    {
        perror("Error opening file");
        return -1;
    }
    vfprintf(fp, format, args);
    fclose(fp);
    __builtin_va_end(args);
    return 0;
}

struct trie *createNode()
{
    struct trie *node = (struct trie *)malloc(sizeof(struct trie));
    node->is_end_character = 0;
    node->parent = NULL;
    node->path_number = -1;

    for (int i = 0; i < ALPHABET_SIZE; i++)
    {
        node->child[i] = NULL;
    }
    return node;
}

// Creates and Initilialises a trie
struct trie *createTrie()
{
    struct trie *root = createNode();
    root->parent = root;
    return root;
}

// Insert a string with a number associated with it
void insert(struct trie *root, char *word, int length, int path_number)
{
    struct trie *temp = root;
    int i = 0;
    do
    {
        int index = (unsigned char)word[i]; // Use unsigned char to handle negative characters
        if (temp->child[index] == NULL)
        {
            temp->child[index] = createNode();
            temp->child[index]->parent = temp;
            temp->child[index]->character = word[i];
        }
        temp = temp->child[index];

    } while (++i < length);

    temp->is_end_character = 1;
    temp->path_number = path_number;
}

// This returns the number associated with the string word path
int search_in_trie(struct trie *root, char *word)
{
    int length = strlen(word);
    int ans = -1;
    struct trie *temp = root;
    int i = 0;
    do
    {
        int index = (unsigned char)word[i];
        if (temp->child[index] == NULL)
        {
            return ans;
        }
        temp = temp->child[index];
    } while (++i < length);

    if (temp->is_end_character)
    {
        ans = temp->path_number;
    }

    return ans;
}

// This function is used to delete a string from the trie
void delete_from_trie(struct trie *root, char *word)
{
    int length = strlen(word);
    struct trie *temp = root;
    int i = 0;
    do
    {
        int index = (unsigned char)word[i];
        if (temp->child[index] == NULL)
        {
            return;
        }
        temp = temp->child[index];
    } while (++i < length);

    if (temp->is_end_character)
    {
        temp->is_end_character = 0;
        temp->path_number = -1;
    }
}

// Hash function for string keys
int hashFunction(const char *key)
{
    int hash = 0;
    while (*key)
    {
        hash = (hash * 31) + (*key++);
    }
    return hash;
}

// Function to initialize LRUCache
LRUCache *createLRUCache(int capacity)
{
    LRUCache *cache = (LRUCache *)malloc(sizeof(LRUCache));
    cache->capacity = capacity;
    cache->size = 0;
    cache->head = NULL;
    cache->tail = NULL;

    // Allocate memory for hash map
    cache->map = (Node **)malloc(sizeof(Node *) * 1000); // Assuming a maximum of 1000 elements for simplicity

    // Initialize map entries to NULL
    for (int i = 0; i < 1000; i++)
    {
        cache->map[i] = NULL;
    }

    return cache;
}

// Function to add a new node to the front of the list
void addToFront(LRUCache *cache, Node *node)
{
    node->next = cache->head;
    node->prev = NULL;

    if (cache->head != NULL)
    {
        cache->head->prev = node;
    }

    cache->head = node;

    if (cache->tail == NULL)
    {
        cache->tail = node;
    }
}

// Function to remove a node from the list
void removeFromList(LRUCache *cache, Node *node)
{
    if (node->prev != NULL)
    {
        node->prev->next = node->next;
    }
    else
    {
        cache->head = node->next;
    }

    if (node->next != NULL)
    {
        node->next->prev = node->prev;
    }
    else
    {
        cache->tail = node->prev;
    }
}

// Function to remove a particular string from LRUCache if found
void removeKey(LRUCache *cache, const char *key)
{
    if (cache->map[hashFunction(key) % 1000] != NULL)
    {
        printc(GREEN, "Key found in cache\n");
        Node *node = cache->map[hashFunction(key) % 1000];
        printf("ptr is %p\n", node);
        if (node == NULL)
        {
            printc(RED, "Node is null\n");
        }
        // while (node != NULL )
        while (1)
        {

            printf("iudbciwc\n");

            if (node == NULL)
                break;
            if (node->next == NULL)
                break;
            if (node == NULL)
            {
                printc(RED, "Node is null\n");
            }
            else
            {
                printc(GREEN, "hi");
            }
            if (node->key != NULL && strcmp(node->key, key) == 0)
            {
                cache->map[hashFunction(key) % 1000] = NULL;
                removeFromList(cache, node);
                free(node->key);
                free(node);
                cache->size--;
                return;
            }
            if (node->next == NULL)
                break;
            node = node->next;
        }
    }
}

// Function to move a node to the front of the list
void moveToFront(LRUCache *cache, Node *node)
{
    removeFromList(cache, node);
    addToFront(cache, node);
}

// Function to get the value from the cache
int get(LRUCache *cache, const char *key)
{
    if (cache->map[hashFunction(key) % 1000] != NULL)
    {
        Node *node = cache->map[hashFunction(key) % 1000];
        while (node != NULL)
        {
            if (strcmp(node->key, key) == 0)
            {
                moveToFront(cache, node);
                return node->value;
            }
            node = node->next;
        }
    }
    return -1; // Key not found
}

// Function to put a new key-value pair into the cache
void put(LRUCache *cache, const char *key, int value)
{
    int hash = hashFunction(key);
    if (cache->map[hash % 1000] != NULL)
    {
        // Update existing key
        Node *node = cache->map[hash % 1000];
        while (node != NULL)
        {
            if (strcmp(node->key, key) == 0)
            {
                node->value = value;
                moveToFront(cache, node);
                return;
            }
            node = node->next;
        }
    }

    // Add new key
    Node *newNode = (Node *)malloc(sizeof(Node));
    newNode->key = strdup(key);
    newNode->value = value;
    newNode->prev = NULL;
    newNode->next = NULL;

    if (cache->size >= cache->capacity)
    {
        // Remove the least recently used item (tail)
        Node *lastNode = cache->tail;
        cache->map[hashFunction(lastNode->key) % 1000] = NULL;
        removeFromList(cache, lastNode);
        free(lastNode->key);
        free(lastNode);
        cache->size--;
    }

    addToFront(cache, newNode);
    cache->map[hash % 1000] = newNode;
    cache->size++;
}

// Function to free memory used by LRUCache
void freeLRUCache(LRUCache *cache)
{
    Node *current = cache->head;
    Node *next;

    while (current != NULL)
    {
        next = current->next;
        free(current->key);
        free(current);
        current = next;
    }

    free(cache->map);
    free(cache);
}

// Function to check if a given port is available
bool is_port_available(int port)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd == -1)
    {
        perror("Socket creation failed in check port available function");
        return false;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    int bind_result = bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));

    close(sockfd);

    // If bind_result is 0, the port is available; otherwise, it is not
    return bind_result == 0;
}

// This checks and generates a random port which is free and not in use
int generate_random_port()
{
    // Seed the random number generator with the current time
    srand((unsigned int)time(NULL));

    int port;
    do
    {
        // Generate a random port within the specified range
        port = rand() % (MAX_PORT - MIN_PORT + 1) + MIN_PORT;
    } while (!is_port_available(port));

    return port;
}

// Function to check if a socket is still connected
int is_socket_connected(int socket_fd)
{
    fd_set read_fds;
    struct timeval timeout;

    FD_ZERO(&read_fds);
    FD_SET(socket_fd, &read_fds);

    // Set a timeout for the select function
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    // Use select to check for readability of the socket
    int result = select(socket_fd + 1, &read_fds, NULL, NULL, &timeout);

    if (result < 0)
    {
        // An error occurred
        perror("select");
        return 0; // Assume socket is disconnected on error
    }
    else if (result == 0)
    {
        // Timeout, socket is still connected
        return 1;
    }
    else
    {
        // Socket is readable, it is still connected
        return 1;
    }
}

int isSocketConnected(int socket)
{
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    if (getpeername(socket, (struct sockaddr *)&addr, &addr_len) == -1)
    {
        // If getpeername fails, it may indicate that the connection has been closed.
        return 0; // Not connected
    }

    // You can also check other conditions if needed, such as the address family or port.
    // For example, if (addr.sin_family != AF_INET || addr.sin_port == 0) return 0;

    return 1; // Connected
}