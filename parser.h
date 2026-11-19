/******************************************************************************
* Project:		IFJ
* Package:		parser
* File:			parser.h
* Date:			1.11.2026
* Last change:	19.11.2026
* Authors: 		Eliška Holá xholael00
*
* Description: Header file of parser logic
*
*******************************************************************************/
/**
* @file parser.h
*
* @brief Parser header file
* @author Eliška Holá xholael00
*/

#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>
#include "scanner.h"
#include "ast.h"

//Parser state struct
typedef struct {
    Scanner *sc;
    Token currentToken;
    Token aheadToken;
    bool lookAhead;    // true if aheadToken holds a token from the lexer
} ParsedToken;

/**
 * @brief Decides if look ahead was done or if it needs to get a new token from lexer.
 * @param p Struct of parser state
 */
void getNextToken(ParsedToken *p);

/**
 * @brief Gets a new token from lexer in lookahead.
 * @param p Struct of parser state
 */
void getTokenAhead(ParsedToken *p);

/**
 * @brief Checks if token has the expected type and calls getToken() for the next one / calls printError if it doesn't.
 * @param p Struct of parser state
 * @param expected Expected token type
 */
void checkToken(ParsedToken *p, TokenType expected);

/**
 * @brief Checks if token type belongs to Predict(<expression>).
 * @param type Token type
 * @return True if it belongs, false if it doesn't.
 */
bool startsExpression(TokenType type);

/**
 * @brief Parses the program and builds its AST, exit(2) on syntax errors
 * @return Pointer to the root of the whole AST
 */
AstNode *parseProgram(Scanner *sc);

#endif
/*** End of file parser.h ***/