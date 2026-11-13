/******************************************************************************
* Project:		IFJ
* Package:		parser
* File:			parser.h
* Date:			1.11.2026
* Last change:	13.11.2026
* Authors: 		xholael00
*
* Description: Header file containing parser logic - recursive descent and
*              precedence analysis logic.
*
*******************************************************************************/
/**
* @file parser.h
*
* @brief Parser header file
* @author xholael00
*/

#ifndef PARSER_H
#define PARSER_H

//temporary
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
    char *str; //identifikátor/řetězec
    int line;
}Token;

Token getToken();

//Parser state struct
typedef struct {
    Token currentToken;
    Token aheadToken;
    bool lookAhead;
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

//temporary precedence
void parseExpression(ParsedToken *p);

#endif
/*** End of file parser.h ***/