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
void parseProg()
{
    //⟨prog⟩ → import⟨opt_NL⟩ifj26⟨opt_NL⟩.⟨opt_NL⟩ ∗ NL⟨definition⟩EOF
    token = getToken();
    checkToken(import);
    parseOptNL();
    checkToken(ifj26);
    parseOptNL();
    checkToken(TOK_DOT); //.
    parseOptNL();
    checkToken(*);
    checkToken(TOK_EOL); //NL
    parseDefinition();
    checkToken(TOK_EOF); //EOF
}

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
    //⟨definition⟩ → epsilon
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

void parseDefBody()
{
    //⟨def_body⟩ → ⟨block⟩
    //Predict(6) = {NL}
    if (token.type == TOK_EOL)
    {
        parseBlock();
    }
    //⟨def_body⟩ → ⟨stat⟩⟨oneline⟩NL
    else if (token.type == id || token.type == int || token.type == double || token.type == string || token.type == ! ||
        token.type == TOK_LPAR || token.type == true || token.type == false || token.type == val || token.type == var ||
        token.type == if || token.type == while || token.type == return)
    {
        parseStat();
        parseOneline();
        checkToken(TOK_EOL);
    }
    else
    {
        printError("Unexpected token");
    }
}

void parseDefBlock()
{
    //⟨def_block⟩ → NL INDENT ⟨extension_def⟩NL DEDENT
    checkToken(TOK_EOL);
    checkToken(TOK_INDENT);
    parseExtensionDef();
    checkToken(TOK_EOL);
    checkToken(TOK_DEDENT);
}

void parseExtensionDef()
{
    //⟨extension_def⟩ → def id ⟨lop⟩⟨ret_type⟩ = ⟨def_body⟩⟨extension_def⟩
    if (token.type == def)
    {
        checkToken(def);
        checkToken(id);
        parseLop();
        parseRetType();
        checkToken(TOK_EQ);
        parseDefBody();
        parseExtensionDef();
    }
    //⟨extension_def⟩ → epsilon
    //Predict(10) = {NL}
    else if (token.type == TOK_EOL)
    {
        return;
    }
    else
    {
        printError("Unexpected token");
    }
}

void parseOptNL()
{
    //⟨opt_NL⟩ → NL
    if (token.type == TOK_EOL)
    {
        checkToken(TOK_EOL);
    }
    //⟨opt_NL⟩ → epsilon
    //Predict(12) = {ifj26, ., *, def}
    else if (token.type == ifj26 || token.type == TOK_DOT || token.type == * || token.type == def)
    {
        return;
    }
    else
    {
        printError("Unexpected token");
    }
}

void parseBlock()
{
    //⟨block⟩ → NL INDENT ⟨stat⟩⟨st_list⟩NL DEDENT
    checkToken(TOK_EOL);
    checkToken(TOK_INDENT);
    parseStat();
    parseStList();
    checkToken(TOK_EOL);
    checkToken(TOK_DEDENT);
}

void parseOneline()
{
    //14: ⟨oneline⟩ → ;⟨oneline_end⟩
    if (token.type == TOK_SEMICOL)
    {
        checkToken(TOK_SEMICOL);
        parseOnelineEnd();
    }
    //15: ⟨oneline⟩ → epsilon
    //Predict(15) = {NL}
    else if (token.type == TOK_EOL)
    {
        return;
    }
    else
    {
        printError("Unexpected token");
    }
}

void parseOnelineEnd()
{
    //16: ⟨oneline_end⟩ → epsilon
    //Predict(16) = {NL}
    if (token.type == TOK_EOL)
    {
        return;

    }
    //17: ⟨oneline_end⟩ → ⟨stat⟩⟨oneline⟩
    else if (token.type == id || token.type == int || token.type == double || token.type == string || token.type == ! ||
        token.type == TOK_LPAR || token.type == true || token.type == false || token.type == val || token.type == var ||
        token.type == if || token.type == while || token.type == return)
    {
        parseStat();
        parseOneline();
    }
    else
    {
        printError("Unexpected token");
    }
}

/*** End of file parser.c ***/