#include "hashtable.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/**
 * Hash function for strings based on the DJB2 algorithm created by Dan Bernstein.
 * Source / Reference: http://www.cse.yorku.ca/~oz/hash.html
 * 
 * @param key The string to be hashed
 * @param tableSize Capacity of the hash table
 * @return Hash value mapped into the table size range
 */

size_t hashFunction(const char *key, size_t tableSize) {
    if (key == NULL || tableSize == 0) {
        return 0;
    }
    size_t hash = 5381;

    //because char can have negative values, and it wont fit to our table function
    const unsigned char *str = (const unsigned char *)key;

    while (*str != '\0') {
        hash = ((hash << 5) + hash) + *str;
        str++;
    }
    return hash % tableSize;
}


/**
 * Initializes a new hash table with the given capacity.
 * 
 * Allocates memory for both the table structure and its item array.
 * Uses calloc to automatically zero-initialize all items (setting states to EMPTY).
 * 
 * @param size The capacity of the hash table
 * @return Pointer to the initialized HashTable, or NULL if memory allocation fails
 */
 
 
HashTable* htInit(size_t size) {
    HashTable *hTable = malloc(sizeof(HashTable));
    if (hTable == NULL) {
        return NULL;
    }
    hTable->size = size;
    hTable->count_occupied = 0;

    hTable->tableItems = calloc(size, sizeof(HashTItem));
    if (hTable->tableItems == NULL) {
        free(hTable);
        hTable = NULL;
        return NULL;
    }

    return hTable;
    



}