/******************************************************************************
* Project:		IFJ
* Package:		parser
* File:			parser.c
* Date:			1.11.2026
* Last change:	1.11.2026
* Authors: 		xholael00
*
* Description: 
*
*******************************************************************************/
/**
* @file parser.c
*
* @brief 
* @author xholael00
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

//temporary
typedef enum {
    TOK_EOF, TOK_EOL, TOK_INDENT, TOK_DEDENT,
    TOK_IDENT, TOK_KEYWORD,
    TOK_INT, TOK_DOUBLE, TOK_STRING,

    //tokens for operators
    TOK_PLUS, TOK_MINUS, TOK_MUL, TOK_DIV,
    TOK_LESS, TOK_LESS_EQ, TOK_GRATER, TOK_GRATER_EQ,
    TOK_EQ, TOK_NEQ, TOK_ASSIGN,

    //tokens for separators
    TOK_LPAR, TOK_RPAR, TOK_COMMA, TOK_COLON, TOK_SEMICOL,
    TOK_DOT, TOK_AT,

    //tokens for extentions
    TOK_AND, TOK_OR, TOK_NOT, TOK_PLUSPLUS
}TokenType;

typedef struct {
    TokenType type;
    char *str; //identifikátor/řetězec
    int line;
}Token;

Token token;

void printError(char *message)
{
    fprintf(stderr, "Syntactic analysis error: %s\n", message);
    exit(2);
}

Token getToken()
{

}

void checkToken(TokenType expected)
{
    if (token.type != expected)
    {
        printError("Unexpected token");
    }
    else
    {
        token = getToken();
    }
}

//TODO: integrate with lexer
void parseDefinition()
{
    //⟨definition⟩ → @main⟨opt_NL⟩def id() : Unit = ⟨def_body⟩⟨definition⟩
    if (token.type == @main)
    {
        checkToken(@main);
        parseOptNL();
        checkToken(def);
        checkToken(id);
        checkToken(TOK_LPAR); //(
        checkToken(TOK_RPAR); //)
        checkToken(TOK_COLON); //:
        checkToken(Unit);
        checkToken(TOK_EQ); //=
        parseDefBody();
        parseDefinition();
    }
    //⟨definition⟩ → def id⟨lop⟩⟨ret_type⟩ = ⟨def_body⟩⟨definition⟩
    else if (token.type == def)
    {
        checkToken(def);
        checkToken(id);
        parseLop();
        parseRetType();
        checkToken(TOK_EQ); //=
        parseDefBody();
        parseDefinition();
    }
    //⟨definition⟩ → extension(id : ⟨type⟩)⟨def_block⟩⟨definition⟩
    else if (token.type == extension)
    {
        checkToken(extension);
        checkToken(TOK_LPAR); //(
        checkToken(id);
        checkToken(TOK_COLON); //:
        parseType();
        checkToken(TOK_RPAR); //)
        parseDefBlock();
        parseDefinition();
    }
    //⟨definition⟩ → ε
    //Predict(5) = {EOF}
    else if (token.type == TOK_EOF)
    {
        return;
    }
    else
    {
        printError("Unexpected token, expected @main/def/extension/EOF.");
    }
}

/*** End of file parser.c ***/