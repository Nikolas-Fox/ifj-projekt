#include "hashtable.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Hash function for strings based on the DJB2 algorithm created by Dan
// Bernstein. Source / Reference: http://www.cse.yorku.ca/~oz/hash.html

size_t hashFunction(const char *key, size_t tableSize) {
    if (key == NULL || tableSize == 0) {
        return 0;
    }
    size_t hash = 5381;

    // because char can have negative values, and it wont fit to our table
    // function
    const unsigned char *str = (const unsigned char *)key;

    while (*str != '\0') {
        // multiplies a number by 33(bit shift by 5)
        hash = ((hash << 5) + hash) + *str;
        str++;
    }
    return hash % tableSize;
}

HashTable *htInit(size_t size) {
    // i cannot init a table with size 0
    if (size == 0) {
        return NULL;
    }
    HashTable *hTable = malloc(sizeof(HashTable));
    if (hTable == NULL) {
        return NULL;
    }
    hTable->size = size;
    hTable->count_occupied = 0;

    // alloc the table, with values 0
    hTable->tableItems = calloc(size, sizeof(HashTItem));
    if (hTable->tableItems == NULL) {
        free(hTable);
        hTable = NULL;
        return NULL;
    }

    return hTable;
}

HashTItem *htSearch(HashTable *table, const char *key) {
    // check for NULL pointers
    if (table == NULL || key == NULL) {
        return NULL;
    }

    size_t position = hashFunction(key, table->size);
    size_t count = 0;

    // while state is not empty i search, when i find occupied state, i look if
    // its the same, or the next one inst, till there is emty slot
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

HashTItem *htInsert(HashTable *table, const char *key, void *data) {
    // check for NULL pointers
    if (table == NULL || key == NULL) {
        return NULL;
    }
    // the initial index from hash function
    size_t position = hashFunction(key, table->size);
    size_t count = 0;
    int deletedPosition = -1;

    // Search through the table for an empty slot, or update an existing key

    while (count < table->size) {
        // Case 1: Found an empty slot
        if (table->tableItems[position].state == EMPTY) {
            if (deletedPosition != -1) {
                table->tableItems[deletedPosition].key = key;
                table->tableItems[deletedPosition].data = data;
                table->tableItems[deletedPosition].state = OCCUPIED;
                table->count_occupied += 1;
                return &table->tableItems[deletedPosition];
            }
            table->tableItems[position].key = key;
            table->tableItems[position].data = data;
            table->tableItems[position].state = OCCUPIED;
            table->count_occupied += 1;
            return &table->tableItems[position];
        }

        // Case 2: Found a deleted slot -> reuse it for new item
        if (table->tableItems[position].state == DELETED) {
            if (deletedPosition == -1) {
                deletedPosition = (int)position;
            }
        }

        // Case 3: Found an occupied slot -> chceck if the key matches, update
        // it
        if (table->tableItems[position].state == OCCUPIED) {
            if (strcmp(table->tableItems[position].key, key) == 0) {
                table->tableItems[position].data = data;
                return &table->tableItems[position];
            }
        }

        // move to the next slot (using module if it is the last one)
        position = (position + 1) % table->size;
        count++;
    }
    // Table has no empty slot, but we still have the first deleted position
    if (deletedPosition != -1) {
        table->tableItems[deletedPosition].key = key;
        table->tableItems[deletedPosition].data = data;
        table->tableItems[deletedPosition].state = OCCUPIED;
        table->count_occupied += 1;
        return &table->tableItems[deletedPosition];
    }
    // Table is full, cannot insert new item

    return NULL;
}

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

void htFree(HashTable *table) {

    if (table != NULL) {
        free(table->tableItems);
        table->tableItems = NULL;
        free(table);
        table = NULL;
    }
}

void forEachData(HashTable *table, CallBackData callback) {
    if (table == NULL || callback == NULL) {
        return;
    }
    size_t size = table->size;
    for (size_t i = 0; i < size; i++) {
        if (table->tableItems[i].state == OCCUPIED) {
            callback(table->tableItems[i].data);
        }
    }
}