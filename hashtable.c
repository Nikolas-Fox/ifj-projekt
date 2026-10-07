#include "hashtable.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/**
 * @brief Hash function for strings based on the DJB2 algorithm created by Dan Bernstein.
 * Source / Reference: http://www.cse.yorku.ca/~oz/hash.html
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
 * @brief Initializes a new hash table with the given capacity.
 * Allocates memory for both the table structure and its item array.
 * Uses calloc to automatically zero-initialize all items (setting states to EMPTY).
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



/**
 * @brief Searches for an item with the sgiven key in the hash table.
 * Uses linear searching to resolve collisions. Traverses the table starting
 * from the calculated hash index until the key is found
 * @param table Pointer to the hash table
 * @param key Key string to search for
 * @return Pointer to the found HashTItem, or NULL if not found or on invalid input
 */

HashTItem* htSearch(HashTable *table, const char *key) {
    //check for NULL pointers
    if(table == NULL || key == NULL) {
        return NULL; 
    }

    size_t position  = hashFunction(key, table->size);
    size_t count = 0;
    
    while (table->tableItems[position].state != EMPTY && count < table->size) {
        if (table->tableItems[position].state == OCCUPIED) {
            if (strcmp(table->tableItems[position].key, key) == 0) {
                return &table->tableItems[position]; // Key found
            }
        }
        position = (position + 1) % table->size;
        count++;
    }
    return NULL;
}

/**
 * @brief Inserts a new element into the hash table or updates an existing one.
 * @param table Pointer to the hash table.
 * @param key String representing the key.
 * @param data Pointer to the data associated with the key.
 * @return HashTableItem* Pointer to the inserted/updated item on success, NULL on error or if the table is full.
 */

HashTItem*htInsert (HashTable *table, const char *key, void *data) {
    //check for NULL pointers
    if(table == NULL|| key == NULL) {
        return NULL; 
    }
    //the initial index from hash function
    size_t position = hashFunction(key, table->size);
    size_t count = 0;

    //Search through the table for an empty slot, or update an existing key

    while (count < table->size ){

        //Case 1: Found an empty slot
        if (table->tableItems[position].state == EMPTY) {
            table->tableItems[position].key = key;
            table->tableItems[position].data = data;
            table->tableItems[position].state = OCCUPIED;
            table->count_occupied += 1;
            return &table->tableItems[position];
        }
        //Case 2: Found a deleted slot -> reuse it for new item
        if (table->tableItems[position].state == DELETED ) {
            table->tableItems[position].key = key;
            table->tableItems[position].data = data;
            table->tableItems[position].state = OCCUPIED;
            table->count_occupied += 1;
            return &table->tableItems[position];
        }

        //Case 3: Found an occupied slot -> chceck if the key matches, update it
        if (table->tableItems[position].state == OCCUPIED) {
            if (strcmp(table->tableItems[position].key, key) == 0) {
                table->tableItems[position].data = data;
                return &table->tableItems[position];
            }
            
        }
        //move to the next slot (using module if it is the last one)
        position = (position + 1 )% table->size;
        count++;
    }
    //Table is full, cannot insert new item

    return NULL;
}


/**
 * @brief Deletes an item from the hash table 
 * Searches for the item using htSearch. If found, clears its pointers
 * and sets its state to DELETED 
 * @param table Pointer to the hash table.
 * @param key String key of the item to delete.
 * @return bool true if the item was found and deleted, false otherwise.
 */

bool htDelete(HashTable *table, const char *key) {

    HashTItem *ItemForDelete = htSearch(table, key);

    if (ItemForDelete == NULL) {
        return false;
    }

    ItemForDelete->key = NULL;
    ItemForDelete->data = NULL;
    ItemForDelete->state = DELETED;
    table->count_occupied -= 1;
    return true;
}

/**
 * @brief Frees all memory allocated for the hash table.
 * 
 * Frees the dynamically allocated array of items and the table structure itself.
 * Note: Individual item keys and data pointers are NOT freed here, as their
 * memory ownership belongs to the Symtable.
 * 
 * @param table Pointer to the hash table to be freed.
 */
void htFree(HashTable *table){

    if (table != NULL) {
        free(table->tableItems);
        table->tableItems = NULL;
        free(table);
        table = NULL;
    }
}