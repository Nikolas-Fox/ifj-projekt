#ifndef SYMTABLE_H
#define SYMTABLE_H

#include <stdbool.h>
#include <stddef.h>
#include "hashtable.h"

#define INITIAL_CAPACITY 16

typedef enum {
    TYPE_INT,
    TYPE_STRING,
    TYPE_BOOL,
    TYPE_DOUBLE,
    TYPE_UNIT,
}DataType;

typedef enum {
    VAL,
    VAR,
    FUNCTION,
} SymbolType;

typedef struct FunctionParam {
    char *paramName;
    DataType dType;
    struct FunctionParam *next;
} FunctionParam;

typedef struct {
    FunctionParam *params;
    size_t paramCount;
} FunctionData;

typedef struct {
    SymbolType sType;
    char *symbolName;
    DataType dtype;
    bool isDefined;

    union {
        FunctionData funcData;
    } value;    
} SymbolData;

typedef struct {
    HashTable **htArray; //array of pointers to Hashtables
    size_t top;   //actual hashtable
    size_t capacity; 
}Symtable;


Symtable* symtableInit(void);

bool symtablePushScope(Symtable *sTable);
void symtablePopScope(Symtable *sTable);

void symtableFree(Symtable *sTable);


SymbolData *symtableInsertVar(Symtable *sTable, char *name, DataType dType, bool isDefined);
SymbolData *symtableInsertVal(Symtable *sTable, char *name, DataType dType, bool isDefined);
SymbolData *symtableInsertFunction(Symtable *sTable, char *name, DataType returnType, bool isDefined);

bool symtableAddParam(SymbolData *funcSymbol, char *paramName, DataType paramType);

SymbolData *symtableSearch(Symtable *sTable, char *name); 
SymbolData *symtableSearchCurrentScope(Symtable *sTable, char *name); 

#endif