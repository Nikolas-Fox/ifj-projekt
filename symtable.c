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

static void freeParams(FunctionParam *params) {
    while (params != NULL) {
        FunctionParam *next = params->next; 
        free(params->paramName);
        free(params);
        params = next;
    }
}

static void freeData(void *data) {
    //to do control if data == NULL
    SymbolData *sData = (SymbolData *) data;
    free(sData->symbolName);
    if(sData->sType == FUNCTION) {
        freeParams(sData->value.funcData.params);
    }

    free(sData);
}


//todo:control if table already isnt null

void symtablePopScope(Symtable *sTable) {
    if(sTable->top != 0){
        forEachData(sTable->htArray[sTable->top], freeData);
        htFree(sTable->htArray[sTable->top]);
        sTable->htArray[sTable->top] = NULL;
        
        sTable->top--;
        }
}

void symtableFree(Symtable *sTable) {
    //todo control if symtable is not already NULL
    while(sTable->top != 0) {
        symtablePopScope(sTable);
    }
    forEachData(sTable->htArray[sTable->top], freeData);
    htFree(sTable->htArray[sTable->top]);
    free(sTable->htArray);
    free(sTable);
}

/* AI-generated (Claude). Prompt: "Function for copiing the name, malloc + memcpy" */
static char *copyString(const char *src) {
    if (src == NULL) {
        return NULL;
    }
    size_t len = strlen(src) + 1; // vrátane '\0'
    char *copy = malloc(len);
    if (copy == NULL) {
        return NULL;
    }
    memcpy(copy, src, len);
    return copy;
}

SymbolData *symtableInsertVar(Symtable *sTable, char *name, DataType dType, bool isDefined) {
    //to do control if symtable is not NULL
    
    if (htSearch(sTable->htArray[sTable->top], name)!= NULL) {
        //redefination in the same scope
        fprintf(stderr, "Error 4");
        exit(4);
    }
    SymbolData *newData = malloc(sizeof(SymbolData));
    //todo control if malloc is not NULL
    newData->sType = VAR;
    newData->symbolName = copyString(name);
    if (newData->symbolName == NULL) {
        //todo return error
        return NULL;
    }
    newData->dtype = dType;
    newData->isDefined = isDefined;
    newData->value.funcData.paramCount = 0;
    newData->value.funcData.params = NULL;

    htInsert(sTable->htArray[sTable->top], newData->symbolName, newData);

    return newData;

}
