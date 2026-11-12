/******************************************************************************
* Project:		IFJ
* Package:		precedence
* File:			precedence.h
* Date:			5.11.2026
* Last change:	9.11.2026
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

    // these are used only in the stack (not the precTable)
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
                    // it's used in the tokenToPrec function, so we know which parentheses are PREC_END
} PrecStack;


/**
 * @brief Initialises a precedence stack
 * @param s PrecStack type pointer
 */
void precStackInit(PrecStack *s);


/**
 * @brief Pushes a PrecStackElement onto stack, if it is '('  parenthese count ++;  ')' paranthese count--
 * @param s PrecStack type pointer
 * @param element PrecSymbol being pushed onto the stack (with PrecSymbol, type, string and next)
 */
void precStackPush(PrecStack *s, PrecStackElem *element);


/**
 * @brief Removes and returns first symbol on stack
 * @param s PrecStack type pointer
 * @return -1 if nothing is left on the stack, PrecStackElem otherwise
 */
PrecStackElem precStackPop(PrecStack *s);


/**
 * @brief returns top symbol on stack
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
 * @param p ParsedToken
 * @param previousWasOperand Keeps track of what the last token was (checking +- unary or binary)
 * @param s Pointer to stack (used to track open parenthese count)
 * @return PrecSymbol equivalent of token
 */
PrecSymbol tokenToPrec(ParsedToken *p, bool previousWasOperand, PrecStack *s);


/**
 * @brief Reduces rules on the stack
 * @param s Pointer to PrecStack
 */
void precReduce(PrecStack *s);


/**
 * @author Eliška Holá (xholael00)
 * @brief Parses <loa>, implements LL grammar rules 46
 * @param p Struct of parser state
 */
void parseLoa(ParsedToken *p);


/**
 * @author Eliška Holá (xholael00)
 * @brief Parses <arg_list>, implements LL grammar rules 48 and 49.
 * @param p Struct of parser state
 */
void parseArgList(ParsedToken *p);


/**
 * @author Eliška Holá (xholael00)
 * @brief Parses <arg_end>, implements LL grammar rules 50 and 51.
 * @param p Struct of parser state
 */
void parseArgEnd(ParsedToken *p);


/**
 * @brief Uses precedence analysis to validate expression
 * @param p Pointer to the parsed token
 */
void parseExpression(ParsedToken *p);


#endif   // PRECEDENCE_H

/*** End of file precedence.h ***/