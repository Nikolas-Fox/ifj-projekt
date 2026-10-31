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
    TYPE_ANY,
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

typedef struct Symtable Symtable;


/**
 * @brief Initializes the symbol table (Frame Stack) and allocates the global scope.
 * 
 * @return Pointer to created Symtable, or NULL if allocation failure.
 */
Symtable* symtableInit(void);


/**
 * @brief Pushes a new scope (block) onto the symbol table stack.
 * Automatically expands the  array capacity using realloc if necessary.
 * 
 * @param sTable Pointer to the Symtable structure.
 * @return true on success
 * @return false on memory allocation failure
 */
bool symtablePushScope(Symtable *sTable);

/**
 * @brief Pop a  scope out of  the symbol table stack.
 * 
 * @param sTable Pointer to the Symtable structure.
 */
void symtablePopScope(Symtable *sTable);

/**
 * @brief Free symtable, pop all the scopes
 * 
 * @param sTable Pointer to the Symtable structure.
 */
void symtableFree(Symtable *sTable);


/**
 * @brief Insert Variable to the symtable
 * 
 * @param sTable Pointer to the Symtable structure.
 * @param name name of the variable
 * @param dType Type of data i am sending (int, string...)
 * @param isDefinied true or false if the variable has already been definied
 * @return pointer to data that has been inserted
 */
SymbolData *symtableInsertVar(Symtable *sTable, char *name, DataType dType, bool isDefined);

/**
 * @brief Insert Val to the symtable
 * 
 * @param sTable Pointer to the Symtable structure.
 * @param name name of the val
 * @param dType Type of data i am sending (int, string...)
 * @param isDefinied true or false if the variable has already been definied
 * @return pointer to data that has been inserted
 */
SymbolData *symtableInsertVal(Symtable *sTable, char *name, DataType dType, bool isDefined);


/**
 * @brief Insert Function to the symtable
 * 
 * @param sTable Pointer to the Symtable structure.
 * @param name name of the function
 * @param dType Type of data i am sending (int, string...)
 * @param isDefinied true or false if the function has already been definied
 * @return pointer to data that has been inserted
 */
SymbolData *symtableInsertFunction(Symtable *sTable, char *name, DataType returnType, bool isDefined);

bool symtableAddParam(SymbolData *funcSymbol, char *paramName, DataType paramType);


/**
 * @brief Search through all scopes
 * 
 * @param sTable Pointer to the Symtable structure.
 * @param name name of the function
 * @return pointer to data
 */
SymbolData *symtableSearch(Symtable *sTable, char *name); 

/**
 * @brief Search through current scope
 * 
 * @param sTable Pointer to the Symtable structure.
 * @param name name of the function
 * @return pointer to data
 */
SymbolData *symtableSearchCurrentScope(Symtable *sTable, char *name); 

#endif
