#ifndef EXTRA_FUNCTIONS_H 
#define EXTRA_FUNCTIONS_H
// Include necessary libraries.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/socket.h>
#include <sys/select.h>
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

#define ALPHABET_SIZE 128 // Assuming ASCII characters, adjust as needed
#define MAX_PORT 80000
#define MIN_PORT 1500


// Define ANSI color codes
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

void printc(const char *color, const char *format, ...);

// trie function below
struct trie
{
    struct trie *child[ALPHABET_SIZE];
    struct trie *parent;
    char character;
    int is_end_character;
    int path_number;
};
struct trie *createNode();
struct trie *createTrie();
void insert(struct trie *root, char *word, int length, int path_number);
int search_in_trie(struct trie *root, char *word);
void delete_from_trie(struct trie *root, char *word);

// Node structure for doubly linked list
typedef struct Node {
    char* key;
    int value;
    struct Node* prev;
    struct Node* next;
} Node;

// LRUCache structure
typedef struct LRUCache {
    int capacity;
    int size;
    Node* head;
    Node* tail;
    Node** map;  // Hash map for quick lookup of nodes
} LRUCache;

LRUCache* createLRUCache(int capacity);
void addToFront(LRUCache *cache, Node *node);
void removeFromList(LRUCache *cache, Node *node);
void moveToFront(LRUCache *cache, Node *node);
void removeKey(LRUCache *cache, const char *key);
int get(LRUCache *cache, const char *key);
void put(LRUCache *cache, const char *key, int value);
void freeLRUCache(LRUCache *cache);

int generate_random_port();

int is_socket_connected(int socket_fd);
int isSocketConnected(int socket);
int write_to_book(char*format, ...);


#endif