/******************************************************************************
* Project:		IFJ
* Package:		precedence
* File:			precedence.h
* Date:			5.11.2026
* Last change:	8.11.2026
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



// temporary from parser.h:

//Parser state struct
typedef struct {
    Token currentToken;
    Token aheadToken;
    bool lookAhead;
} ParsedToken;

Token getNextToken(ParsedToken *p);

void getTokenAhead(ParsedToken *p);
/******************************************************************************/





// data type - symbol for the precedencial analyser 
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
    PREC_END,         // $

    // these are used only in the stack
    PREC_E,           // expression
    PREC_SHIFT        // <
}PrecSymbol;


// data type - element of the precedence stack
typedef struct PrecStackElem
{
    PrecSymbol symbol;
    DataType type;
    char *str;      // in case the symbol is a string literal (we must free it at the end)
    struct PrecStackElem *next;
}PrecStackElem;


// adt - precedence stack for symbols 
typedef struct 
{
    PrecStackElem *top;
    int openParCnt;    // this holds a count of how many parentheses are open
                    // it's used in the tokenToPrec function, so we know which parentheses are $ 
} PrecStack;

/**
 * @brief Initialises a precedence stack
 * @param s PrecStack type pointer
 */
void precStackInit(PrecStack *s);

/**
 * @brief Pushes a precSymbol onto stack, if it's a ( we increment open parenthese count
 * @param s PrecStack type pointer
 * @param symbol PrecSymbol being pushed onto the stack
 */
void precStackPush(PrecStack *s, PrecSymbol symbol);

/**
 * @brief Removes and returns first symbol on stack, if it's a ) we decrement open parenthese count
 * @param s PrecStack type pointer
 * @return -1 if nothing is left on the stack, PrecSymbol otherwise
 */
PrecSymbol precStackPop(PrecStack *s);

/**
 * @brief returns first symbol from stack
 * @param s PrecStack type pointer
 * @return -1 if nothing is left on the stack, PrecSymbol otherwise
 */
PrecSymbol precStackTop(PrecStack *s);

/**
 * @brief Frees the whole PrecStack
 * @param s PrecStack type pointer
 */
void precStackFree(PrecStack *s);




/**
 * @brief Returns the first terminal on the stack
 * @param s PrecStack type pointer
 * @return -1 if error, PrecSymbol otherwise
 */
PrecSymbol precStackGetFirstTerm(PrecStack *s);

/**
 * @brief Pushes Shift PrecSymbol onto the stack after the first terminal
 * @param s PrecStack type pointer
 */
void precStackShiftAfterFirstTerm(PrecStack *s);





/**
 * @brief returns the PrecSymbol equivalent of a token
 * @param current Pointer to the current token
 * @param previousWasOperand Keeps track of what the last token was (checking +- unary or binary)
 * @param stack Pointer to stack, where I keep a hold of the number of parentheses
 * @return -1 if token doesn't have symbol equivalent, otherwise PrecSymbol
 */
PrecSymbol tokenToPrec(Token *current, bool previousWasOperand, PrecStack *s);

/**
 * @brief Uses precedence analysis to validate expression
 * @param p Pointer to the parsed token
 */
void parseExpression(ParsedToken *p);


#endif   // PRECEDENCE_H

/*** End of file precedence.h ***/