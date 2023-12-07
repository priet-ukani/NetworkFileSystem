#include "extra_functions.h"
LRUCache* cache;
// Example usage
int main() {
    cache = createLRUCache(3);

    put(cache, "path1", 100);
    put(cache, "path2", 200);
    put(cache, "path3", 300);

    printf("Value for key 'path2': %d\n", get(cache, "path2"));  // Output: 200

    put(cache, "path4", 400);  // Evicts key 'path1'

    printf("Value for key 'path1': %d\n", get(cache, "path1"));  // Output: -1 (not found)
    printf("Value for key 'path2': %d\n", get(cache, "path2"));  // Output: 200

    freeLRUCache(cache);

    return 0;
}
