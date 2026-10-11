#include "hashtable.h"
#include "symtable.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>


/**
 * Initializes the symbol table (Frame Stack) and allocates the global scope.
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