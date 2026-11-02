/******************************************************************************
* Project:		IFJ
* Package:		parser
* File:			parser.c
* Date:			1.11.2026
* Last change:	2.11.2026
* Authors: 		xholael00
*
* Description: Implementation of parser - recursive descent and precedence analysis.
*
*******************************************************************************/
/**
* @file parser.c
*
* @brief Implementation of parser
* @author xholael00
*/
#include <stdio.h>
#include <stdlib.h>
#include "parser.h"

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

typedef enum {
    KW_DEF, KW_DO, KW_DOUBLE, KW_ELSE, KW_IF, KW_IMPORT,
    KW_INT, KW_RETURN, KW_STRING, KW_THEN, KW_UNIT, KW_VAL,
    KW_VAR, KW_WHILE
}Keyword;

typedef struct {
    TokenType type;
    char *str; //identifikátor/řetězec
    Keyword kw;
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

void parseStList()
{
    //18: ⟨st_list⟩ →;⟨st_list_semi⟩
    if (token.type == TOK_SEMICOL)
    {
        checkToken(TOK_SEMICOL);
        parseStListSemi();
    }
    //19: ⟨st_list⟩ → NL⟨st_list_NL⟩
    //20: ⟨st_list⟩ → epsilon
    //Predict(20) = {NL}
    else if (token.type = TOK_EOL)
    {
        //TODO: lookahead
    }
    else
    {
        printError("Unexpected token");
    }
}

void parseStListSemi()
{
    //21: ⟨st_list_semi⟩ → ⟨stat⟩⟨st_list⟩
    if (token.type == id || token.type == int || token.type == double || token.type == string || token.type == ! ||
        token.type == TOK_LPAR || token.type == true || token.type == false || token.type == val || token.type == var ||
        token.type == if || token.type == while || token.type == return)
    {
        parseStat();
        parseStList();
    }
    //22: ⟨st_list_semi⟩ → NL⟨st_list_NL⟩
    //23: ⟨st_list_semi⟩ → ε
    //Predict(23) = {NL}
    else if (token.type = TOK_EOL)
    {
        //TODO: lookahead
    }
    else
    {
        printError("Unexpected token");
    }
}

void parseStListNL()
{
    //24: ⟨st_list_NL⟩ → ⟨stat⟩⟨st_list⟩
    if (token.type == id || token.type == int || token.type == double || token.type == string || token.type == ! ||
        token.type == TOK_LPAR || token.type == true || token.type == false || token.type == val || token.type == var ||
        token.type == if || token.type == while || token.type == return)
    {
        parseStat();
        parseStList();
    }
    //25: ⟨st_list_NL⟩ → epsilon
    else if (token.type == TOK_EOL)
    {
        return;
    }
}

void parseStat()
{
    //26: ⟨stat⟩ → ⟨expression⟩
    if (token.type == id || token.type == int || token.type == double || token.type == string || token.type == ! ||
        token.type == TOK_LPAR || token.type == true || token.type == false)
    {
        //precedence analysis
    }
    //27: ⟨stat⟩ → val id⟨ret_type⟩ = ⟨expression⟩
    else if (token.type == val)
    {
        checkToken(val);
        checkToken(id);
        parseRetType();
        checkToken(TOK_EQ);
        //precedence analysis
    }
    //28: ⟨stat⟩ → var id⟨ret_type⟩ = ⟨expression⟩
    else if (token.type == var)
    {
        checkToken(var);
        checkToken(id);
        parseRetType();
        checkToken(TOK_EQ);
        //precedence analysis
    }
    //29: ⟨stat⟩ → if ⟨expression⟩ then ⟨block⟩⟨else_choice⟩
    else if (token.type == if)
    {
        checkToken(if);
        //precedence analysis
        checkToken(then);
        parseBlock();
        parseElseChoice();
    }
    //30: ⟨stat⟩ → return⟨return⟩
    else if (token.type == return)
    {
        checkToken(return);
        parseReturn();
    }
    //31: ⟨stat⟩ → while ⟨expression⟩ do ⟨block⟩
    else if (token.type == while)
    {
        checkToken(while);
        //precedence analysis
        checkToken(do);
        parseBlock();
    }
    else
    {
        printError("Unexpected token");
    }
}

void parseElseChoice()
{
    //32: ⟨else_choice⟩ → else ⟨else⟩
    if (token.type == else)
    {
        checkToken(else);
        parseElse();
    }
    //33: ⟨else_choice⟩ → epsilon
    //Predict(33) = {NL,;}
    else if (token.type == TOK_EOL || token.type == TOK_SEMICOL)
    {
        return;
    }
    else
    {
        printError("Unexpected token");
    }
}

void parseElse()
{
    //34: ⟨else⟩ → ⟨block⟩
    if (token.type == TOK_EOL)
    {
        parseBlock();
    }
    //35: ⟨else⟩ → if ⟨expression⟩ then ⟨block⟩⟨else_choice⟩
    else if (token.type == if)
    {
        checkToken(if);
        //precedence analysis
        checkToken(then);
        parseBlock();
        parseElseChoice();
    }
    else
    {
        printError("Unexpected token");
    }
}

void parseRetType()
{
    //36: ⟨ret_type⟩ →: ⟨type⟩
    if (token.type == TOK_COLON)
    {
        checkToken(TOK_COLON);
        parseType();
    }
    //37: ⟨ret_type⟩ → epsilon
    //Predict(37) = {=}
    else if (token.type == TOK_EQ)
    {
        return;
    }
    else
    {
        printError("Unexpected token");
    }
}

void parseType()
{
    //38: ⟨type⟩ → Int
    if (token.type == Int)
    {
        checkToken(Int);
    }
    //39: ⟨type⟩ → Double
    else if (token.type == Double)
    {
        checkToken(Double);
    }
    //40: ⟨type⟩ → String
    else if (token.type == String)
    {
        checkToken(String);
    }
    //41: ⟨type⟩ → Unit
    else if (token.type == Unit)
    {
        checkToken(Unit);
    }
    //42: ⟨type⟩ → Boolean
    else if (token.type == Boolean)
    {
        checkToken(Boolean);
    }
    else
    {
        printError("Unexpected token");
    }
}

void parseReturn()
{
    //43: ⟨return⟩ → ⟨expression⟩
    if (token.type == id || token.type == int || token.type == double || token.type == string || token.type == ! ||
        token.type == TOK_LPAR || token.type == true || token.type == false)
    {
        //precedence analysis
    }
    //44: ⟨return⟩ → epsilon
    //Predict(44) = {NL, ;}
    else if (token.type == TOK_EOL || token.type == TOK_SEMICOL)
    {
        return;
    }
    else
    {
        printError("Unexpected token");
    }
}

void parseParamList()
{
    //45: ⟨param_list⟩ → id : ⟨type⟩⟨param_end⟩
    if (token.type == id)
    {
        checkToken(id);
        checkToken(TOK_COLON);
        parseType();
        parseParamEnd();
    }
    //46: ⟨param_list⟩ → epsilon
    //Predict(46) = {)}
    else if (token.type == TOK_RPAR)
    {
        return;
    }
    else
    {
        printError("Unexpected token");
    }
}

void parseParamEnd()
{
    //47: ⟨param_end⟩ →,⟨param_list⟩
    if (token.type == TOK_COMMA)
    {
        checkToken(TOK_COMMA);
        parseParamList();
    }
    //48: ⟨param_end⟩ → epsilon
    //Predict(48) = {)}
    else if (token.type == TOK_RPAR)
    {
        return;
    }
    else
    {
        printError("Unexpected token");
    }
}

void parseLop()
{
    //49: ⟨lop⟩ → (⟨param_list⟩)⟨lop⟩
    if (token.type == TOK_LPAR)
    {
        checkToken(TOK_LPAR);
        parseParamList();
        checkToken(TOK_RPAR);
        parseLop();
    }
    //50: ⟨lop⟩ → epsilon
    //Predict(50) = {:, =}
    else if (token.type == TOK_COLON || token.type == TOK_EQ)
    {
        return;
    }
    else
    {
        printError("Unexpected token");
    }
}

void parseArgList()
{
    //51: ⟨arg_list⟩ → ⟨expression⟩⟨arg_end⟩
    if (token.type == id || token.type == int || token.type == double || token.type == string || token.type == ! ||
        token.type == TOK_LPAR || token.type == true || token.type == false)
    {
        //precedence analysis
        parseArgEnd();
    }
    //52: ⟨arg_list⟩ → epsilon
    //Predict(52) = {)}
    else if (token.type == TOK_RPAR)
    {
        return;
    }
    else
    {
        printError("Unexpected token");
    }
}

void parseArgEnd()
{
    //53: ⟨arg_end⟩ →,⟨arg_list⟩
    if (token.type == TOK_COMMA)
    {
        checkToken(TOK_COMMA);
        parseArgList();
    }
    //54: ⟨arg_end⟩ → epsilon
    //Predict(54) = {)}
    else if (token.type == TOK_RPAR)
    {
        return;
    }
    else
    {
        printError("Unexpected token");
    }
}

//TODO: precedence analysis
void parseExpression()
{

}
/*** End of file parser.c ***/