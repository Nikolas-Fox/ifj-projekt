/******************************************************************************
* Project:		IFJ
* Package:		precedence
* File:			precedence.h
* Date:			5.11.2026
* Last change:	7.11.2026
* Authors: 		xfoxnik00
*
* Description: Header file for precedence syntactic analysis
*
*******************************************************************************/
/**
* @file precedence.h
*
* @brief Header file for precedence syntactic analysis
* @author xfoxnik00
*/

#ifndef PRECEDENCE_H
#define PRECEDENCE_H


// #include "parser.h"
// #include "scanner.h"
// #include "symtable.h"


/******************************************************************************/
// temporary from scanner.h
typedef enum {
    TOK_EOF, TOK_NL, TOK_INDENT, TOK_DEDENT,
    TOK_ID, TOK_KEYWORD,
    TOK_INT, TOK_DOUBLE, TOK_STRING,

    //tokens for operators
    TOK_PLUS, TOK_MINUS, TOK_MUL, TOK_DIV,
    TOK_LESS, TOK_LESS_EQ, TOK_GREATER, TOK_GREATER_EQ,
    TOK_EQ, TOK_NEQ, TOK_ASSIGN,

    //tokens for separators
    TOK_LPAR, TOK_RPAR, TOK_COMMA, TOK_COLON, TOK_SEMICOL,
    TOK_DOT, TOK_AT_MAIN,

    //tokens for extentions
    TOK_AND, TOK_OR, TOK_NOT, TOK_PLUSPLUS,

    TOK_KW_DEF, TOK_KW_DO, TOK_KW_DOUBLE, TOK_KW_ELSE, TOK_KW_IF, TOK_KW_IMPORT,
    TOK_KW_INT, TOK_KW_RETURN, TOK_KW_STRING, TOK_KW_THEN, TOK_KW_UNIT, TOK_KW_VAL,
    TOK_KW_VAR, TOK_KW_WHILE, TOK_KW_EXTENSION, TOK_KW_BOOLEAN
}TokenType;


typedef struct {
    TokenType type;
    char *str; // identifier/string
    int line;
}Token;

Token getToken(void);
/******************************************************************************/

/******************************************************************************/
// temporary from symtable.h
#include <stdbool.h>
#include <stddef.h> // for size_t
typedef enum {
    TYPE_INT,
    TYPE_STRING,
    TYPE_BOOL,
    TYPE_DOUBLE,
    TYPE_UNIT,
    TYPE_ANY,
} DataType;

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
 * @brief Search through all scopes
 *
 * @param sTable Pointer to the Symtable structure.
 * @param name name of the function
 * @return pointer to data
 */
SymbolData *symtableSearch(Symtable *sTable, char *name);

/******************************************************************************/

// symbol type for the precedencial analyser 
// I group some of the symbols from the precedential table,
// because they have the same priority and associativty 
typedef enum
{
    PREC_FUNC,        // f(E)
    PREC_DOT,         // E.E
    PREC_SIGN,        // +P, -P
    PREC_NOT,         // !
    PREC_MUL_DIV,
    PREC_PLUS_MINUS,
    PREC_RELATION,    // >, >=, <, <=
    PREC_EQ_NE,       // ==, !=
    PREC_AND,
    PREC_OR,
    PREC_LPAR,
    PREC_RPAR,
    PREC_ID,
    PREC_ASSIGN,     // =
    PREC_END         // $
}PrecSymbol;

// TODO: Declare precedence function, define a precedence stack


#endif   // PRECEDENCE_H

/*** End of file precedence.h ***/