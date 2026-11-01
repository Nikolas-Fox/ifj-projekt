/******************************************************************************
 * Project:      Implementace překladače imperativního jazyka IFJ26
 * Package:      Symbol table
 * File:         symtable.c
 * Date:         1. 11. 2026
 * Last change:  1. 11. 2026
 * Authors:      xpovala00
 *
 * Description:  Implementation of the symbol table as a stack of hash tables,
 *               one per scope.
 *****************************************************************************/
/**
* @file symtable.c
*
* @brief Implementation of the symbol table
* @author xpovala00
*/



#include "symtable.h"
#include "hashtable.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Symtable {
    HashTable **htArray; // array of pointers to Hashtables
    size_t top;          // actual hashtable
    size_t capacity;
};

// AI: (claude) - help with the structure for Built in Functions for global
// scope
typedef struct {
    const char *name;
    DataType ret;
    int paramCount;
    DataType params[3];
    const char *paramNames[3];
} BuiltinDef;

static const BuiltinDef BUILTINS[] = {
    {"print", TYPE_UNIT, 1, {TYPE_ANY}, {"value"}},
    {"readLine", TYPE_STRING, 0, {0}, {0}},
    {"readInt", TYPE_INT, 0, {0}, {0}},
    {"readDouble", TYPE_DOUBLE, 0, {0}, {0}},
    {"toStr", TYPE_STRING, 1, {TYPE_ANY}, {"value"}},
    {"toInt", TYPE_INT, 1, {TYPE_DOUBLE}, {"value"}},
    {"strLength", TYPE_INT, 1, {TYPE_STRING}, {"value"}},
    {"subStr",
     TYPE_STRING,
     3,
     {TYPE_STRING, TYPE_INT, TYPE_INT},
     {"value", "start", "end"}},
    {"ord", TYPE_INT, 2, {TYPE_STRING, TYPE_INT}, {"value", "index"}},
    {"chr", TYPE_STRING, 1, {TYPE_INT}, {"code"}},
};

Symtable *symtableInit(void) {
    // Allocate memory for the main Symtable
    Symtable *symTable = malloc(sizeof(Symtable));
    if (symTable == NULL) {
        return NULL;
    }

    // Allocate memory for the array of hash table pointers
    symTable->htArray = malloc(INITIAL_CAPACITY * sizeof(HashTable *));
    if (symTable->htArray == NULL) {
        free(symTable);
        return NULL;
    }
    symTable->top = 0;
    symTable->capacity = INITIAL_CAPACITY;

    // inicialize first global hashtable
    symTable->htArray[0] = htInit(HASH_T_SIZE);
    if (symTable->htArray[0] == NULL) {
        free(symTable->htArray);
        free(symTable);
        return NULL;
    }
    // insert builtin functions
    for (size_t i = 0; i < sizeof BUILTINS / sizeof BUILTINS[0];
         i++) { // i < size of whole builtins / one line
        const BuiltinDef *b = &BUILTINS[i];
        SymbolData *f =
            symtableInsertFunction(symTable, (char *)b->name, b->ret, true);
        for (int j = 0; j < b->paramCount; j++) {
            symtableAddParam(f, (char *)b->paramNames[j], b->params[j]);
        }
    }
    return symTable;
}

bool symtablePushScope(Symtable *sTable) {
    if (sTable == NULL) {
        return false;
    }
    // Check if we need to expand the capacity of the pointer array
    if ((sTable->top + 1) >= sTable->capacity) {
        size_t newCapacity = sTable->capacity * 2;
        HashTable **new =
            realloc(sTable->htArray, newCapacity * (sizeof(HashTable *)));
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
    if (data == NULL) {
        return;
    }
    SymbolData *sData = (SymbolData *)data;
    free(sData->symbolName);
    if (sData->sType == FUNCTION) {
        freeParams(sData->value.funcData.params);
    }

    free(sData);
}

void symtablePopScope(Symtable *sTable) {
    if (sTable == NULL || sTable->top == 0) {
        return;
    }
    if (sTable->top != 0) {
        forEachData(sTable->htArray[sTable->top], freeData);
        htFree(sTable->htArray[sTable->top]);
        sTable->htArray[sTable->top] = NULL;

        sTable->top--;
    }
}

void symtableFree(Symtable *sTable) {
    if (sTable == NULL) {
        return;
    }
    while (sTable->top != 0) {
        symtablePopScope(sTable);
    }
    forEachData(sTable->htArray[sTable->top], freeData);
    htFree(sTable->htArray[sTable->top]);
    free(sTable->htArray);
    free(sTable);
}

/* AI-generated (Claude). Prompt: "Function for copiing the name, malloc +
 * memcpy" */
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

static SymbolData *insertSymbol(Symtable *sTable, char *name, SymbolType sType,
                                DataType dType, bool isDefined) {
    if (sTable == NULL || name == NULL) {
        return NULL;
    }

    HashTable *current = sTable->htArray[sTable->top];

    // Redefinition in the same scope (shadowing in outer scopes is allowed)
    if (htSearch(current, name) != NULL) {
        fprintf(stderr, "Error 4: redefinition of '%s'\n", name);
        exit(4);
    }

    SymbolData *newData = malloc(sizeof(SymbolData));
    if (newData == NULL) {
        fprintf(stderr, "Error 99: memory allocation failed\n");
        exit(99);
    }

    newData->symbolName = copyString(name);
    if (newData->symbolName == NULL) {
        free(newData);
        fprintf(stderr, "Error 99: memory allocation failed\n");
        exit(99);
    }

    newData->sType = sType;
    newData->dtype = dType;
    newData->isDefined = isDefined;
    newData->value.funcData.paramCount = 0;
    newData->value.funcData.params = NULL;

    // The key is the copy owned by the symbol, not the token's string
    if (htInsert(current, newData->symbolName, newData) == NULL) {
        free(newData->symbolName);
        free(newData);
        fprintf(stderr, "Error 99: symbol table is full\n");
        exit(99);
    }

    return newData;
}

SymbolData *symtableInsertVar(Symtable *sTable, char *name, DataType dType,
                              bool isDefined) {
    return insertSymbol(sTable, name, VAR, dType, isDefined);
}

SymbolData *symtableInsertVal(Symtable *sTable, char *name, DataType dType,
                              bool isDefined) {
    return insertSymbol(sTable, name, VAL, dType, isDefined);
}

SymbolData *symtableInsertFunction(Symtable *sTable, char *name,
                                   DataType returnType, bool isDefined) {
    return insertSymbol(sTable, name, FUNCTION, returnType, isDefined);
}

bool symtableAddParam(SymbolData *funcSymbol, char *paramName,
                      DataType paramType) {
    if (funcSymbol == NULL || paramName == NULL ||
        funcSymbol->sType != FUNCTION) {
        return false;
    }

    // Go through the list to search if there is not the same param already

    FunctionParam **top = &funcSymbol->value.funcData.params;
    while (*top != NULL) {
        if (strcmp((*top)->paramName, paramName) == 0) {
            fprintf(stderr, "Error 4: duplicate parameter '%s'\n", paramName);
            exit(4);
        }
        top = &(*top)->next;
    }

    FunctionParam *param = malloc(sizeof(FunctionParam));
    if (param == NULL) {
        fprintf(stderr, "Error 99: memory allocation failed\n");
        exit(99);
    }

    param->paramName = copyString(paramName);
    if (param->paramName == NULL) {
        free(param);
        fprintf(stderr, "Error 99: memory allocation failed\n");
        exit(99);
    }

    param->dType = paramType;
    param->next = NULL;

    *top = param;
    funcSymbol->value.funcData.paramCount++;
    return true;
}

SymbolData *symtableSearchCurrentScope(Symtable *sTable, char *name) {
    if (sTable == NULL || name == NULL) {
        return NULL;
    }

    HashTItem *item = htSearch(sTable->htArray[sTable->top], name);
    if (item == NULL) {
        return NULL;
    }
    return (SymbolData *)item->data;
}

SymbolData *symtableSearch(Symtable *sTable, char *name) {
    if (sTable == NULL || name == NULL) {
        return NULL;
    }

    // From the scope on the top till the first match
    //
    for (size_t i = sTable->top + 1; i > 0; i--) {
        HashTItem *item = htSearch(sTable->htArray[i - 1], name);
        if (item != NULL) {
            return (SymbolData *)item->data;
        }
    }
    return NULL;
}
