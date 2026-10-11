#include "hashtable.h"
#include "symtable.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>


/**
 * @brief Initializes the symbol table (Frame Stack) and allocates the global scope.
 * 
 * @return Pointer to created Symtable, or NULL if allocation failure.
 */
Symtable* symtableInit(void) {
    // Allocate memory for the main Symtable
    Symtable *symTable = malloc(sizeof(Symtable));
    if (symTable == NULL) {
        return NULL;
    }

    // Allocate memory for the array of hash table pointers
    symTable->htArray = malloc(INITIAL_CAPACITY*sizeof(HashTable*));
    if (symTable->htArray == NULL) {
        free(symTable);
        return NULL;
    }
    symTable->top = 0;
    symTable->capacity = INITIAL_CAPACITY;

    //inicialize first global hashtable
    symTable->htArray[0] = htInit(HASH_T_SIZE);
    if (symTable->htArray[0] == NULL) {
        free(symTable->htArray);
        free(symTable);
        return NULL;
    }
    return symTable;
}

/**
 * @brief Pushes a new scope (block) onto the symbol table stack.
 * Automatically expands the  array capacity using realloc if necessary.
 * 
 * @param sTable Pointer to the Symtable structure.
 * @return true on success
 * @return false on memory allocation failure
 */
bool symtablePushScope(Symtable *sTable) {
    if (sTable == NULL) {
        return false;
    }
    // Check if we need to expand the capacity of the pointer array
    if ((sTable->top + 1) >= sTable->capacity) {
        size_t newCapacity = sTable->capacity * 2;
        HashTable **new = realloc(sTable->htArray, newCapacity *(sizeof(HashTable *)));
        if (new == NULL) {
            // If realloc failed, keep the old array safe
            return false; 
        }
        sTable->htArray = new;
        sTable->capacity = newCapacity;
    }

    // Move to the new scope index and initialize a new hash table
    sTable->top++;
    sTable->htArray[sTable->top] = htInit(HASH_T_SIZE);
    if (sTable->htArray[sTable->top] == NULL) {
        // give back top if hash table initialization failed
        sTable->top--;
        return false;
    }

    return true;

}