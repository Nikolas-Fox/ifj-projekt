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

#include <stddef.h>

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
    TOK_KW_BOOLEAN, TOK_KW_EXTENSION, 

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

#define MAX_INTERP 32
#define MAX_INDENT 128

typedef struct Scanner{
int line;
int atLineStart;
int parenDepth;   
int lineIndent;    // number of spaces at the beginning of the current logical line
int lineTab;       // 1 = indentation contains a tab

int dotAfterNumber;

int interpDepth;              // how many interpolated strings are open
int interpKind[MAX_INTERP];       // 0 = s"...", 1 = s"""..."""
int interpInExpr[MAX_INTERP];    // 1 = inside ${ ... }
Token interpQueued;               // token waiting for the next call
int hasInterpQueued;
int resumeId;              // 1 = continue reading the identifier after $

int comma;  // 1 = the last token was a comma
int commaNL;  // 1 = a newline has appeared since that comma

// Buffer for the lexeme text
char *buf;
size_t bufLen;
size_t bufCap;

int indentStack[MAX_INDENT];
int indentTop;        // index of the stack top (0 = no indentation)
int sendDedents;   // how many DEDENT tokens are still to be returned
int hasQueued;        // 1 = a token is waiting in the queue
Token queued;              // token waiting behind EOL / INDENT / DEDENT
int lineHasToken;    // has the current logical line already had a token?
int afterSemicolon;    // 1 = the previous token was ';'
}Scanner;

/**
 * @brief Initializes and returns a new Scanner instance
 * @return Pointer to the allocated and initialized Scanner structure
 */
Scanner *scannerInit(void);

/**
 * @brief Releases the lexer and its buffer.
 * @param sc Pointer to the Scanner structure to be freed.
 */
void scannerFree(Scanner *sc);

/**
* @brief Returns the next token from the standard input
* @return token
*/
Token getToken(Scanner *sc);


#endif

/*** End of file scanner.h ***/