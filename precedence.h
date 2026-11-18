/******************************************************************************
* Project:		IFJ
* Package:		precedence
* File:			precedence.h
* Date:			5.11.2026
* Last change:	18.11.2026
* Authors: 		Nikolas Fox xfoxnik00
*
* Description: Header file for precedence syntactic analysis
*
*******************************************************************************/
/**
* @file precedence.h
*
* @brief Header file for precedence syntactic analysis
* @author Nikolas Fox xfoxnik00
*/

#ifndef PRECEDENCE_H
#define PRECEDENCE_H

#include "symtable.h"
#include "scanner.h"
#include "ast.h"
#include "parser.h"


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
    AstNode *node;
    TokenType operator;
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
 * @param node pointer to node we want to change
 * @return PrecSymbol equivalent of token
 */
PrecSymbol tokenToPrec(ParsedToken *p, bool previousWasOperand, PrecStack *s, AstNode **node);


/**
 * @brief Reduces rules on the stack
 * @param s Pointer to PrecStack
 */
void precReduce(PrecStack *s);


/**
 * @author Eliška Holá (xholael00) & Nikolas Fox
 * @brief Parses <loa>, implements LL grammar rules 46
 * @param p Struct of parser state
 * @param groupCount Pointer to the number of groups, incremented with each new group
 */
AstNode *parseLoa(ParsedToken *p, size_t *groupCount);


/**
 * @author Eliška Holá (xholael00) & Nikolas Fox (xfoxnik00)
 * @brief Parses <arg_list>, implements LL grammar rules 48 and 49.
 * @param p Struct of parser state
 * @param count Pointer to number of arguments, incremented with each argument
 * @return Pointer to the first argument in linked list
 */
AstNode *parseArgList(ParsedToken *p, size_t *count);


/**
 * @author Eliška Holá (xholael00) & Nikolas Fox (xfoxnik00)
 * @brief Parses <arg_end>, implements LL grammar rules 50 and 51.
 * @param p Struct of parser state
 * @param count Pointer to the number of arguments (given to parseArgList for the next argument)
 * @return Pointer to the following argument in linked list
 */
AstNode *parseArgEnd(ParsedToken *p, size_t *count);


/**
 * @brief Uses precedence analysis to validate expression
 * @param p Pointer to the parsed token
 */
AstNode *parseExpression(ParsedToken *p);


#endif   // PRECEDENCE_H

/*** End of file precedence.h ***/