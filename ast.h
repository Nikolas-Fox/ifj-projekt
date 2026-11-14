/******************************************************************************
* Project:		IFJ
* Package:		ast
* File:			ast.h
* Date:			12.11.2026
* Last change:	13.11.2026
* Authors: 		Nikolas Fox xfoxnik00
*
* Description: Header file containing abstract syntactic tree logic
*
*******************************************************************************/
/**
* @file ast.h
*
* @brief Header file containing abstract syntactic tree logic
* @author Nikolas Fox xfoxnik00
*/

#ifndef AST_H
#define AST_H


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


// temporary from symtable.h:

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

SymbolData *symtableSearch(Symtable *sTable, char *name);

/*********************************************************/
typedef enum
{
    AST_WHOLE,

    AST_FUNC,
    AST_GROUP, // this is a group of parameters (a,b) - I will implement a linked list of these groups
    AST_PARAM, // this is an individual parameter - also implemented as a linked list

    AST_ASSIGN,
    AST_DECLARATION,
    AST_IF,
    AST_WHILE,
    AST_RETURN,

    AST_BINARY, // operators
    AST_UNARY,
    AST_CALL,
    AST_LITERAL,
    AST_ID,


} AstClass;

typedef struct AstNode
{
    AstClass class;
    DataType type;
}

#endif
/*** End of file ast.c ***/