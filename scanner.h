/******************************************************************************
* Project:		IFJ
* Package:		Lexical analysis
* File:			scanner.h
* Date:			18.10.2026
* Last change:	12.11.2026
* Author: 		Eliška Opršalová xoprsae00
*
* Description: 	header file - defines the token types, the Token structure
*               and the getToken() function
*
*******************************************************************************/
/**
* @file scanner.h
*
* @brief lexer header file
* @author Eliška Opršalová xoprsae00
*/

#ifndef SCANNER_H
#define SCANNER_H

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
    TOK_AND, TOK_OR, TOK_NOT, 
    TOK_KW_BOOLEAN, TOK_KW_TRUE, TOK_KW_FALSE,
    TOK_KW_EXTENSION, 

    //tokens for keywords
    TOK_KW_DEF, TOK_KW_DO, TOK_KW_DOUBLE, TOK_KW_ELSE, TOK_KW_IF, TOK_KW_IMPORT,
    TOK_KW_INT, TOK_KW_RETURN, TOK_KW_STRING, TOK_KW_THEN, TOK_KW_UNIT, TOK_KW_VAL,
    TOK_KW_VAR, TOK_KW_WHILE,
    TOK_INTERP_START, TOK_INTERP_END, TOK_INTERP_EXPR_START, TOK_INTERP_EXPR_END
}TokenType;

typedef struct {
    TokenType type;
    char *str;
    int line;
}Token;

/**
* @brief Returns the next token from the standard input
* @return token
*/
Token getToken(void);


#endif

/*** End of file scanner.h ***/