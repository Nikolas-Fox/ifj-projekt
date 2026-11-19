/******************************************************************************
 * Project:      IFJ
 * Package:      Symbol table
 * File:         symtable.h
 * Date:         1. 11. 2026
 * Last change:  16. 11. 2026
 * Authors:      Alexandra Povalová xpovala00
 *
 * Description:  Header file of the symbol table (stack of scopes): symbol
 *               data types and the public interface.
 *****************************************************************************/
/**
 * @file symtable.h
 *
 * @brief Header file of the symbol table
 * @author Alexandra Povalová xpovala00
 */

#ifndef SYMTABLE_H
#define SYMTABLE_H

#include "hashtable.h"
#include <stdbool.h>
#include <stddef.h>

#define INITIAL_CAPACITY 16

typedef enum {
    TYPE_NONE,
    TYPE_INT,
    TYPE_STRING,
    TYPE_BOOL,
    TYPE_DOUBLE,
    TYPE_UNIT,
    TYPE_ANY,
    TYPE_RETURNED // uses the emantic analyzer to resolve types on explicit
                  // return
} DataType;

typedef enum {
    VAL,
    VAR,
    FUNCTION,
} SymbolType;

typedef struct FunctionParam {
    char *paramName;
    DataType dType;
    size_t listIndex; // which parameter list it belongs to (0-based)
    struct FunctionParam *next;
} FunctionParam;

typedef struct {
    FunctionParam *params;
    size_t paramCount; // total number of params
    size_t listCount;  // 0 = no list, number of lists
} FunctionData;

typedef struct {
    SymbolType sType;
    char *symbolName;
    DataType dtype;
    bool isDefined;
    int scopeID;

    union {
        FunctionData funcData;
    } value;
} SymbolData;

// AI: (claude) - help with the structure for Built in Functions for global
// scope
typedef struct {
    const char *name;
    DataType ret;
    int paramCount;
    DataType params[3];
    const char *paramNames[3];
} BuiltinDef;

typedef struct Symtable Symtable;

/**
 * @brief Initializes the symbol table (Frame Stack) and allocates the global
 * scope.
 *
 * @return Pointer to created Symtable, or NULL if allocation failure.
 */
Symtable *symtableInit(void);

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
 * @brief Copy string and alloc place
 *
 * @param src Pointer to the string
 * @return pointer to data that has been copied
 */
char *copyString(const char *src);

/**
 * @brief Gets the scope ID of the current active scope
 *
 * @param sTable Pointer to the Symtable
 * @return Scope ID of the current top scope
 */
int symtableScopeID(Symtable *sTable);

/**
 * @brief Insert Variable to the symtable
 *
 * @param sTable Pointer to the Symtable structure.
 * @param name name of the variable
 * @param dType Type of data i am sending (int, string...)
 * @param isDefinied true or false if the variable has already been definied
 * @return pointer to data that has been inserted
 */
SymbolData *symtableInsertVar(Symtable *sTable, char *name, DataType dType,
                              bool isDefined);

/**
 * @brief Insert Val to the symtable
 *
 * @param sTable Pointer to the Symtable structure.
 * @param name name of the val
 * @param dType Type of data i am sending (int, string...)
 * @param isDefinied true or false if the variable has already been definied
 * @return pointer to data that has been inserted
 */
SymbolData *symtableInsertVal(Symtable *sTable, char *name, DataType dType,
                              bool isDefined);

/**
 * @brief Insert Function to the symtable
 *
 * @param sTable Pointer to the Symtable structure.
 * @param name name of the function
 * @param dType Type of data i am sending (int, string...)
 * @param isDefinied true or false if the function has already been definied
 * @return pointer to data that has been inserted
 */
SymbolData *symtableInsertFunction(Symtable *sTable, char *name,
                                   DataType returnType, bool isDefined);

/**
 * @brief Insert parameters of the function
 *
 * @param funcSymbol Pointer to the function
 * @param paramName name of the parameter
 * @param paramType Type of the param
 * @return true/false
 */
bool symtableAddParam(Symtable *sTable, SymbolData *funcSymbol, char *paramName,
                      DataType paramType);

/**
 * @brief Starts a new parameter list (parenthesis group) for a function.
 *
 * @param funcSymbol Pointer to the function symbol.
 * @return true on success, false if funcSymbol is NULL or not a function.
 */
bool symtableStartParamList(SymbolData *funcSymbol);

/**
 * @brief Returns the number of parameters in a specific parameter list.
 *
 * @param funcSymbol Pointer to the function symbol.
 * @param list Index of the parameter list (0 for the first group, 1 for the
 * second)
 * @return Number of parameters in the given list, or 0 if invalid/empty.
 */
size_t symtableParamListSize(const SymbolData *funcSymbol, size_t list);

/**
 * @brief Gets a specific parameter from a specific parameter list.
 *
 * @param funcSymbol Pointer to the function symbol.
 * @param list Index of the parameter list (starting from 0).
 * @param index Position of the parameter inside that list (starting from 0).
 * @return Pointer to the FunctionParam, or NULL if not found or invalid.
 */
const FunctionParam *symtableGetParam(const SymbolData *funcSymbol, size_t list,
                                      size_t index);

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

/*** End of file symtable.h ***/
