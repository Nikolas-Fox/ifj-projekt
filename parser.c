/******************************************************************************
* Project:		IFJ
* Package:		parser
* File:			parser.c
* Date:			1.11.2026
* Last change:	3.11.2026
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
#include <string.h>
#include <stdbool.h>
#include "parser.h"

void printError(char *message)
{
    fprintf(stderr, "Syntactic analysis error: %s\n", message);
    exit(2);
}

void parseProgram()
{
    ParsedToken token;
    token.lookAhead = false;
    getToken(&token);
    parseProg(&token);
}

Token lexerGetToken()
{

}

void getToken(ParsedToken *p)
{
    if (p->lookAhead)
    {
        p->currentToken = p->aheadToken;
        p->lookAhead = false;
    }
    else
    {
        p->currentToken = lexerGetToken();
    }
}

void checkToken(ParsedToken *p, TokenType expected)
{
    if (p->currentToken.type != expected)
    {
        printError("Unexpected token");
    }
    else
    {
        getToken(p);
    }
}

void checkStrToken(ParsedToken *p, TokenType expected, char *name)
{
    if (p->currentToken.type != expected || p->currentToken.str == NULL || strcmp(p->currentToken.str, name) != 0)
    {
        printError("Unexpected token");
    }
    getToken(p);
}

bool startsStat(TokenType type)
{
    switch (type)
    {
        case TOK_ID:
        case TOK_INT:
        case TOK_DOUBLE:
        case TOK_STRING:
        case TOK_NOT:
        case TOK_LPAR:
        case TOK_KW_VAL:
        case TOK_KW_VAR:
        case TOK_KW_IF:
        case TOK_KW_WHILE:
        case TOK_KW_RETURN:
            return true;
        default:
            return false;
    }
}

bool startsExpression(TokenType type)
{
    switch (type)
    {
        case TOK_ID:
        case TOK_INT:
        case TOK_DOUBLE:
        case TOK_STRING:
        case TOK_NOT:
        case TOK_LPAR:
            return true;
        default:
            return false;
    }
}

//TODO: integrate with lexer
//TODO: lookahead
void parseProg(ParsedToken *p)
{
    //⟨prog⟩ → import⟨opt_NL⟩ifj26⟨opt_NL⟩.⟨opt_NL⟩ ∗ NL⟨definition⟩EOF
    checkToken(p, TOK_KW_IMPORT);
    parseOptNL(p);
    checkStrToken(p, TOK_ID, "ifj26");
    parseOptNL(p);
    checkToken(p, TOK_DOT);
    parseOptNL(p);
    checkToken(p, TOK_MUL);
    checkToken(p, TOK_NL);
    parseDefinition(p);
    checkToken(p, TOK_EOF);
}

void parseDefinition(ParsedToken *p)
{
    //⟨definition⟩ → @main⟨opt_NL⟩def id() : Unit = ⟨def_body⟩⟨definition⟩
    if (p->currentToken.type == TOK_AT_MAIN)
    {
        checkToken(p, TOK_AT_MAIN);
        parseOptNL(p);
        checkToken(p, TOK_KW_DEF);
        checkToken(p, TOK_ID);
        checkToken(p, TOK_LPAR);
        checkToken(p, TOK_RPAR);
        checkToken(p, TOK_COLON);
        checkToken(p, TOK_KW_UNIT);
        checkToken(p, TOK_EQ);
        parseDefBody(p);
        parseDefinition(p);
    }
    //⟨definition⟩ → def id⟨lop⟩⟨ret_type⟩ = ⟨def_body⟩⟨definition⟩
    else if (p->currentToken.type == TOK_KW_DEF)
    {
        checkToken(p, TOK_KW_DEF);
        checkToken(p, TOK_ID);
        parseLop(p);
        parseRetType(p);
        checkToken(p, TOK_EQ);
        parseDefBody(p);
        parseDefinition(p);
    }
    //⟨definition⟩ → extension(id : ⟨type⟩)⟨def_block⟩⟨definition⟩
    else if (p->currentToken.type == TOK_KW_EXTENSION)
    {
        checkToken(p, TOK_KW_EXTENSION);
        checkToken(p, TOK_LPAR);
        checkToken(p, TOK_ID);
        checkToken(p, TOK_COLON);
        parseType(p);
        checkToken(p, TOK_RPAR);
        parseDefBlock(p);
        parseDefinition(p);
    }
    //⟨definition⟩ → epsilon
    //Predict(5) = {EOF}
    else if (p->currentToken.type == TOK_EOF)
    {
        return;
    }
    else
    {
        printError("Unexpected token, expected @main/def/extension/EOF.");
    }
}

void parseDefBody(ParsedToken *p)
{
    //⟨def_body⟩ → ⟨block⟩
    //Predict(6) = {NL}
    if (p->currentToken.type == TOK_NL)
    {
        parseBlock(p);
    }
    //⟨def_body⟩ → ⟨stat⟩⟨oneline⟩NL
    else if (startsStat(p->currentToken.type))
    {
        parseStat(p);
        parseOneline(p);
        checkToken(p, TOK_NL);
    }
    else
    {
        printError("Unexpected token");
    }
}

void parseDefBlock(ParsedToken *p)
{
    //⟨def_block⟩ → NL INDENT ⟨extension_def⟩NL DEDENT
    checkToken(p, TOK_NL);
    checkToken(p, TOK_INDENT);
    parseExtensionDef(p);
    checkToken(p, TOK_NL);
    checkToken(p, TOK_DEDENT);
}

void parseExtensionDef(ParsedToken *p)
{
    //⟨extension_def⟩ → def id ⟨lop⟩⟨ret_type⟩ = ⟨def_body⟩⟨extension_def⟩
    if (p->currentToken.type == TOK_KW_DEF)
    {
        checkToken(p, TOK_KW_DEF);
        checkToken(p, TOK_ID);
        parseLop(p);
        parseRetType(p);
        checkToken(p, TOK_EQ);
        parseDefBody(p);
        parseExtensionDef(p);
    }
    //⟨extension_def⟩ → epsilon
    //Predict(10) = {NL}
    else if (p->currentToken.type == TOK_NL)
    {
        return;
    }
    else
    {
        printError("Unexpected token");
    }
}

void parseOptNL(ParsedToken *p)
{
    //⟨opt_NL⟩ → NL
    if (p->currentToken.type == TOK_NL)
    {
        checkToken(p, TOK_NL);
    }
    //⟨opt_NL⟩ → epsilon
    //Predict(12) = {ifj26, ., *, def}
    else if ((p->currentToken.type == TOK_ID && (strcmp(p->currentToken.str, "ifj26") == 0)) || p->currentToken.type == TOK_DOT ||
            p->currentToken.type == TOK_MUL || p->currentToken.type == TOK_KW_DEF)
    {
        return;
    }
    else
    {
        printError("Unexpected token");
    }
}

void parseBlock(ParsedToken *p)
{
    //⟨block⟩ → NL INDENT ⟨stat⟩⟨st_list⟩NL DEDENT
    checkToken(p, TOK_NL);
    checkToken(p, TOK_INDENT);
    parseStat(p);
    parseStList(p);
    checkToken(p, TOK_NL);
    checkToken(p, TOK_DEDENT);
}

void parseOneline(ParsedToken *p)
{
    //14: ⟨oneline⟩ → ;⟨oneline_end⟩
    if (p->currentToken.type == TOK_SEMICOL)
    {
        checkToken(p, TOK_SEMICOL);
        parseOnelineEnd(p);
    }
    //15: ⟨oneline⟩ → epsilon
    //Predict(15) = {NL}
    else if (p->currentToken.type == TOK_NL)
    {
        return;
    }
    else
    {
        printError("Unexpected token");
    }
}

void parseOnelineEnd(ParsedToken *p)
{
    //16: ⟨oneline_end⟩ → epsilon
    //Predict(16) = {NL}
    if (p->currentToken.type == TOK_NL)
    {
        return;

    }
    //17: ⟨oneline_end⟩ → ⟨stat⟩⟨oneline⟩
    else if (startsStat(p->currentToken.type))
    {
        parseStat(p);
        parseOneline(p);
    }
    else
    {
        printError("Unexpected token");
    }
}

void parseStList(ParsedToken *p)
{
    //18: ⟨st_list⟩ →;⟨st_list_semi⟩
    if (p->currentToken.type == TOK_SEMICOL)
    {
        checkToken(p, TOK_SEMICOL);
        parseStListSemi(p);
    }
    //19: ⟨st_list⟩ → NL⟨st_list_NL⟩
    //20: ⟨st_list⟩ → epsilon
    //Predict(20) = {NL}
    else if (p->currentToken.type == TOK_NL)
    {
        //TODO: lookahead
    }
    else
    {
        printError("Unexpected token");
    }
}

void parseStListSemi(ParsedToken *p)
{
    //21: ⟨st_list_semi⟩ → ⟨stat⟩⟨st_list⟩
    if (startsStat(p->currentToken.type))
    {
        parseStat(p);
        parseStList(p);
    }
    //22: ⟨st_list_semi⟩ → NL⟨st_list_NL⟩
    //23: ⟨st_list_semi⟩ → epsilon
    //Predict(23) = {NL}
    else if (p->currentToken.type == TOK_NL)
    {
        //TODO: lookahead
    }
    else
    {
        printError("Unexpected token");
    }
}

void parseStListNL(ParsedToken *p)
{
    //24: ⟨st_list_NL⟩ → ⟨stat⟩⟨st_list⟩
    if (startsStat(p->currentToken.type))
    {
        parseStat(p);
        parseStList(p);
    }
    //25: ⟨st_list_NL⟩ → epsilon
    else if (p->currentToken.type == TOK_NL)
    {
        return;
    }
    else
    {
        printError("Unexpected token");
    }
}

void parseStat(ParsedToken *p)
{
    //26: ⟨stat⟩ → ⟨expression⟩
    if (startsExpression(p->currentToken.type))
    {
        //precedence analysis
    }
    //27: ⟨stat⟩ → val id⟨ret_type⟩ = ⟨expression⟩
    else if (p->currentToken.type == TOK_KW_VAL)
    {
        checkToken(p, TOK_KW_VAL);
        checkToken(p, TOK_ID);
        parseRetType(p);
        checkToken(p, TOK_EQ);
        //precedence analysis
    }
    //28: ⟨stat⟩ → var id⟨ret_type⟩ = ⟨expression⟩
    else if (p->currentToken.type == TOK_KW_VAR)
    {
        checkToken(p, TOK_KW_VAR);
        checkToken(p, TOK_ID);
        parseRetType(p);
        checkToken(p, TOK_EQ);
        //precedence analysis
    }
    //29: ⟨stat⟩ → if ⟨expression⟩ then ⟨block⟩⟨else_choice⟩
    else if (p->currentToken.type == TOK_KW_IF)
    {
        checkToken(p, TOK_KW_IF);
        //precedence analysis
        checkToken(p, TOK_KW_THEN);
        parseBlock(p);
        parseElseChoice(p);
    }
    //30: ⟨stat⟩ → return⟨return⟩
    else if (p->currentToken.type == TOK_KW_RETURN)
    {
        checkToken(p, TOK_KW_RETURN);
        parseReturn(p);
    }
    //31: ⟨stat⟩ → while ⟨expression⟩ do ⟨block⟩
    else if (p->currentToken.type == TOK_KW_WHILE)
    {
        checkToken(p, TOK_KW_WHILE);
        //precedence analysis
        checkToken(p, TOK_KW_DO);
        parseBlock(p);
    }
    else
    {
        printError("Unexpected token");
    }
}

void parseElseChoice(ParsedToken *p)
{
    //32: ⟨else_choice⟩ → else ⟨else⟩
    if (p->currentToken.type == TOK_KW_ELSE)
    {
        checkToken(p, TOK_KW_ELSE);
        parseElse(p);
    }
    //33: ⟨else_choice⟩ → epsilon
    //Predict(33) = {NL,;}
    else if (p->currentToken.type == TOK_NL || p->currentToken.type == TOK_SEMICOL)
    {
        return;
    }
    else
    {
        printError("Unexpected token");
    }
}

void parseElse(ParsedToken *p)
{
    //34: ⟨else⟩ → ⟨block⟩
    if (p->currentToken.type == TOK_NL)
    {
        parseBlock(p);
    }
    //35: ⟨else⟩ → if ⟨expression⟩ then ⟨block⟩⟨else_choice⟩
    else if (p->currentToken.type == TOK_KW_IF)
    {
        checkToken(p, TOK_KW_IF);
        //precedence analysis
        checkToken(p, TOK_KW_THEN);
        parseBlock(p);
        parseElseChoice(p);
    }
    else
    {
        printError("Unexpected token");
    }
}

void parseRetType(ParsedToken *p)
{
    //36: ⟨ret_type⟩ →: ⟨type⟩
    if (p->currentToken.type == TOK_COLON)
    {
        checkToken(p, TOK_COLON);
        parseType(p);
    }
    //37: ⟨ret_type⟩ → epsilon
    //Predict(37) = {=}
    else if (p->currentToken.type == TOK_EQ)
    {
        return;
    }
    else
    {
        printError("Unexpected token");
    }
}

void parseType(ParsedToken *p)
{
    //38: ⟨type⟩ → Int
    if (p->currentToken.type == TOK_KW_INT)
    {
        checkToken(p, TOK_KW_INT);
    }
    //39: ⟨type⟩ → Double
    else if (p->currentToken.type == TOK_KW_DOUBLE)
    {
        checkToken(p, TOK_KW_DOUBLE);
    }
    //40: ⟨type⟩ → String
    else if (p->currentToken.type == TOK_KW_STRING)
    {
        checkToken(p, TOK_KW_STRING);
    }
    //41: ⟨type⟩ → Unit
    else if (p->currentToken.type == TOK_KW_UNIT)
    {
        checkToken(p, TOK_KW_UNIT);
    }
    //42: ⟨type⟩ → Boolean
    else if (p->currentToken.type == TOK_KW_BOOLEAN)
    {
        checkToken(p, TOK_KW_BOOLEAN);
    }
    else
    {
        printError("Unexpected token");
    }
}

void parseReturn(ParsedToken *p)
{
    //43: ⟨return⟩ → ⟨expression⟩
    if (startsExpression(p->currentToken.type))
    {
        //precedence analysis
    }
    //44: ⟨return⟩ → epsilon
    //Predict(44) = {NL, ;}
    else if (p->currentToken.type == TOK_NL || p->currentToken.type == TOK_SEMICOL)
    {
        return;
    }
    else
    {
        printError("Unexpected token");
    }
}

void parseParamList(ParsedToken *p)
{
    //45: ⟨param_list⟩ → id : ⟨type⟩⟨param_end⟩
    if (p->currentToken.type == TOK_ID)
    {
        checkToken(p, TOK_ID);
        checkToken(p, TOK_COLON);
        parseType(p);
        parseParamEnd(p);
    }
    //46: ⟨param_list⟩ → epsilon
    //Predict(46) = {)}
    else if (p->currentToken.type == TOK_RPAR)
    {
        return;
    }
    else
    {
        printError("Unexpected token");
    }
}

void parseParamEnd(ParsedToken *p)
{
    //47: ⟨param_end⟩ →,⟨param_list⟩
    if (p->currentToken.type == TOK_COMMA)
    {
        checkToken(p, TOK_COMMA);
        parseParamList(p);
    }
    //48: ⟨param_end⟩ → epsilon
    //Predict(48) = {)}
    else if (p->currentToken.type == TOK_RPAR)
    {
        return;
    }
    else
    {
        printError("Unexpected token");
    }
}

void parseLop(ParsedToken *p)
{
    //49: ⟨lop⟩ → (⟨param_list⟩)⟨lop⟩
    if (p->currentToken.type == TOK_LPAR)
    {
        checkToken(p, TOK_LPAR);
        parseParamList(p);
        checkToken(p, TOK_RPAR);
        parseLop(p);
    }
    //50: ⟨lop⟩ → epsilon
    //Predict(50) = {:, =}
    else if (p->currentToken.type == TOK_COLON || p->currentToken.type == TOK_EQ)
    {
        return;
    }
    else
    {
        printError("Unexpected token");
    }
}

void parseArgList(ParsedToken *p)
{
    //51: ⟨arg_list⟩ → ⟨expression⟩⟨arg_end⟩
    if (startsExpression(p->currentToken.type))
    {
        //precedence analysis
        parseArgEnd(p);
    }
    //52: ⟨arg_list⟩ → epsilon
    //Predict(52) = {)}
    else if (p->currentToken.type == TOK_RPAR)
    {
        return;
    }
    else
    {
        printError("Unexpected token");
    }
}

void parseArgEnd(ParsedToken *p)
{
    //53: ⟨arg_end⟩ →,⟨arg_list⟩
    if (p->currentToken.type == TOK_COMMA)
    {
        checkToken(p, TOK_COMMA);
        parseArgList(p);
    }
    //54: ⟨arg_end⟩ → epsilon
    //Predict(54) = {)}
    else if (p->currentToken.type == TOK_RPAR)
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