/******************************************************************************
 * Project:      IFJ
 * Package:      Symbol table
 * File:         hashtable.h
 * Date:         01. 11. 2026
 * Last change:  26. 11. 2026
 * Authors:      Alexandra Povalová xpovala00
 *
 * Description:  Header file of the hash table - open addressing, linear
 *               probing: item and table structures, function prototypes.
 *****************************************************************************/
/**
 * @file hashtable.h
 *
 * @brief Header file of the hash table
 * @author Alexandra Povalová xpovala00
 */

#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <stdbool.h>
#include <stddef.h>
// implicit size of table
#define HASH_T_SIZE 4093

// 3 states for linear probe
typedef enum {
    EMPTY,
    OCCUPIED,
    DELETED,
} HashTItemState;

// structure of one item in table
typedef struct {
    const char *key; // name of the variable/function
    void *data;      // void because of the universal hashtable
    HashTItemState state;
} HashTItem;

// Table structure
typedef struct {
    HashTItem *tableItems;
    size_t size;
    size_t count_occupied;
    int scopeID;
} HashTable;

// return void, and it is pointer to function
typedef void (*CallBackData)(void *data);

/**
 * @brief Initializes a new hash table with the given capacity.
 * Allocates memory for both the table structure and its item array.
 * Uses calloc to automatically zero-initialize all items (setting states to
 * EMPTY).
 * @param size The capacity of the hash table
 * @param scopeId in whic scope is it
 * @return Pointer to the initialized HashTable, or NULL if memory allocation
 * fails
 */

HashTable *htInit(size_t size, int scopeID);

/**
 * @brief function for return the scope id where the table is
 *
 * @param table the actual table i wnat to know the scope of
 * @return scope id - int
 */
int htScopeID(HashTable *table);

/**
 * @brief Inserts a new element into the hash table or updates an existing one.
 * @param table Pointer to the hash table.
 * @param key String representing the key.
 * @param data Pointer to the data associated with the key.
 * @return HashTableItem* Pointer to the inserted/updated item on success, NULL
 * on error or if the table is full.
 */

HashTItem *htInsert(HashTable *table, const char *key, void *data);

/**
 * @brief Searches for an item with the sgiven key in the hash table.
 * Uses linear probing to resolve collisions. Traverses the table starting
 * from the calculated hash index until the key is found
 * @param table Pointer to the hash table
 * @param key Key string to search for
 * @return Pointer to the found HashTItem, or NULL if not found or on invalid
 * input
 */
HashTItem *htSearch(HashTable *table, const char *key);

/**
 * @brief Deletes an item from the hash table
 * Searches for the item using htSearch. If found, clears its pointers
 * and sets its state to DELETED
 * @param table Pointer to the hash table.
 * @param key String key of the item to delete.
 * @return bool true if the item was found and deleted, false otherwise.
 */

bool htDelete(HashTable *table, const char *key);

/**
 * @brief Frees all memory allocated for the hash table.
 *
 * Frees the dynamically allocated array of items and the table structure
 * itself. Note: Individual item keys and data pointers are NOT freed here, as
 * their memory ownership belongs to the Symtable.
 *
 * @param table Pointer to the hash table to be freed.
 */

void htFree(HashTable *table);

/**
 * @brief Hash function for strings based on the DJB2 algorithm created by Dan
 * Bernstein. Source / Reference: http://www.cse.yorku.ca/~oz/hash.html
 * @param key The string to be hashed
 * @param tableSize Capacity of the hash table
 * @return Hash value mapped into the table size range
 */
size_t hashFunction(const char *key, size_t tableSize);

/**
 * @brief callback function to go through every occupied state and call on them
 * given function
 * @param table pointer to table to go through
 * @param callback given function
 */
void forEachData(HashTable *table, CallBackData callback);

#endif
/*** End of file hashtable.h ***/
