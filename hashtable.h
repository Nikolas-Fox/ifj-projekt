#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <stdbool.h>
#include <stddef.h>
//implicit size of table 
#define HASH_T_SIZE 4093

//3 states for linear search
typedef enum {
    EMPTY,
    OCCUPIED,
    DELETED,
} HashTItemState;

//structure of one item in table
typedef struct  {
    char *key;  //name of the variable/function
    void *data; //void because of the universal hashtable
    HashTItemState state;
} HashTItem;

//Table structure
typedef struct {
    HashTItem *tableItems;
    size_t size;
    size_t count_occupied;
} HashTable;


HashTable* htInit(size_t size);

HashTItem* htInsert (HashTable *table, const char *key, void *data);

HashTItem* htSearch(HashTable *table, const char *key);

bool htDelete(HashTable *table, const char *key);

void htFree(HashTable *table);

size_t hashFunction(const char *key, size_t tableSize);


#endif