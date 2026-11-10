/******************************************************************************
* Project:		IFJ
* Package:		parser
* File:			parser.c
* Date:			1.11.2026
* Last change:	8.11.2026
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
    getNextToken(&token);
    parseProg(&token);
}

//temporary scanner.h
Token getToken()
{

}

void getNextToken(ParsedToken *p)
{
    if (p->lookAhead)
    {
        p->currentToken = p->aheadToken;
        p->lookAhead = false;
    }
    else
    {
        p->currentToken = getToken();
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

void getTokenAhead(ParsedToken *p)
{
    if (!p->lookAhead)
    {
        p->lookAhead = true;
        p->aheadToken = getToken();
    }
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
    switch (p->currentToken.type)
    {
        case TOK_AT_MAIN: //2: ⟨definition⟩ → @main⟨opt_NL⟩def id⟨lop⟩⟨ret_type⟩ = ⟨def_body⟩⟨definition⟩
            checkToken(p, TOK_AT_MAIN);
            parseOptNL(p);
            checkToken(p, TOK_KW_DEF);
            checkToken(p, TOK_ID);
            parseLop(p);
            parseRetType(p);
            checkToken(p, TOK_EQ);
            parseDefBody(p);
            parseDefinition(p);
            break;
        case TOK_KW_DEF: //3: ⟨definition⟩ → def id⟨lop⟩⟨ret_type⟩ = ⟨def_body⟩⟨definition⟩
            checkToken(p, TOK_KW_DEF);
            checkToken(p, TOK_ID);
            parseLop(p);
            parseRetType(p);
            checkToken(p, TOK_EQ);
            parseDefBody(p);
            parseDefinition(p);
            break;
        case TOK_KW_EXTENSION: //4: ⟨definition⟩ → extension(id : ⟨type⟩)⟨def_block⟩⟨definition⟩
            checkToken(p, TOK_KW_EXTENSION);
            checkToken(p, TOK_LPAR);
            checkToken(p, TOK_ID);
            checkToken(p, TOK_COLON);
            parseType(p);
            checkToken(p, TOK_RPAR);
            parseDefBlock(p);
            parseDefinition(p);
            break;
        case TOK_EOF: //5: ⟨definition⟩ → epsilon
            return;
        default:
            printError("Unexpected token, expected @main/def/extension/EOF.");
            break;
    }
}

void parseDefBody(ParsedToken *p)
{
    //6: ⟨def_body⟩ → ⟨block⟩
    if (p->currentToken.type == TOK_NL)
    {
        parseBlock(p);
    }
    //7: ⟨def_body⟩ → ⟨stat⟩
    else if (startsStat(p->currentToken.type))
    {
        parseStat(p);
    }
    else
    {
        printError("Unexpected token");
    }
}

void parseDefBlock(ParsedToken *p)
{
    //8: ⟨def_block⟩ → NL INDENT ⟨extension_def⟩ DEDENT
    checkToken(p, TOK_NL);
    checkToken(p, TOK_INDENT);
    parseExtensionDef(p);
    checkToken(p, TOK_DEDENT);
}

void parseExtensionDef(ParsedToken *p)
{
    switch (p->currentToken.type)
    {
        case TOK_KW_DEF: //9: ⟨extension_def⟩ → def id ⟨lop⟩⟨ret_type⟩ = ⟨def_body⟩⟨extension_def⟩
           checkToken(p, TOK_KW_DEF);
            checkToken(p, TOK_ID);
            parseLop(p);
            parseRetType(p);
            checkToken(p, TOK_EQ);
            parseDefBody(p);
            parseExtensionDef(p);
            break;
        case TOK_DEDENT: //10: ⟨extension_def⟩ → epsilon
            return;
        default:
            printError("Unexpected token");
            break;
    }
}

void parseOptNL(ParsedToken *p)
{
    //11: ⟨opt_NL⟩ → NL
    if (p->currentToken.type == TOK_NL)
    {
        checkToken(p, TOK_NL);
    }
    //12: ⟨opt_NL⟩ → epsilon
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
    //13: ⟨block⟩ → NL INDENT ⟨stat⟩⟨st_list⟩ DEDENT
    checkToken(p, TOK_NL);
    checkToken(p, TOK_INDENT);
    parseStat(p);
    parseStList(p);
    checkToken(p, TOK_DEDENT);
}

void parseStList(ParsedToken *p)
{
    switch (p->currentToken.type)
    {
        case TOK_SEMICOL: //14: ⟨st_list⟩ → ⟨stat⟩⟨st_list_NL⟩
            checkToken(p, TOK_SEMICOL);
            parseStListSemi(p);
            break;
        case TOK_DEDENT: //15: ⟨st_list⟩ → epsilon
            return;
        default:
            printError("Unexpected token");
            break;
    }
}

void parseStat(ParsedToken *p)
{
    //26: ⟨stat⟩ → ⟨expression⟩
    if (startsExpression(p->currentToken.type))
    {
        //precedence analysis
    }
    else
    {
        switch (p->currentToken.type)
        {
            case TOK_KW_VAL: //27: ⟨stat⟩ → val id⟨ret_type⟩ = ⟨expression⟩
                checkToken(p, TOK_KW_VAL);
                checkToken(p, TOK_ID);
                parseRetType(p);
                checkToken(p, TOK_EQ);
                //precedence analysis
                break;
            case TOK_KW_VAR: //28: ⟨stat⟩ → var id⟨ret_type⟩ = ⟨expression⟩
                checkToken(p, TOK_KW_VAR);
                checkToken(p, TOK_ID);
                parseRetType(p);
                checkToken(p, TOK_EQ);
                //precedence analysis
                break;
            case TOK_KW_IF: //29: ⟨stat⟩ → if ⟨expression⟩ then ⟨block⟩⟨else_choice⟩
                checkToken(p, TOK_KW_IF);
                //precedence analysis
                checkToken(p, TOK_KW_THEN);
                parseBlock(p);
                parseElseChoice(p);  
                break;
            case TOK_KW_RETURN: //30: ⟨stat⟩ → return⟨return⟩
                checkToken(p, TOK_KW_RETURN);
                parseReturn(p);
                break;
            case TOK_KW_WHILE: //31: ⟨stat⟩ → while ⟨expression⟩ do ⟨block⟩
                checkToken(p, TOK_KW_WHILE);
                //precedence analysis
                checkToken(p, TOK_KW_DO);
                parseBlock(p);
                break;
            default:
                printError("Unexpected token");
                break;
            
        }
    }
}

void parseElseChoice(ParsedToken *p)
{
    switch (p->currentToken.type)
    {
        case TOK_KW_ELSE: //27: ⟨else_choice⟩ → else ⟨else⟩
            checkToken(p, TOK_KW_ELSE);
            parseElse(p);
            break;
        //TODO //28: ⟨else_choice⟩ → epsilon
        default:
            printError("Unexpected token");
            break;
    }
}

void parseElse(ParsedToken *p)
{
    switch (p->currentToken.type)
    {
        case TOK_NL: //29: ⟨else⟩ → ⟨block⟩
            parseBlock(p);
            break;
        case TOK_KW_IF: //30: ⟨else⟩ → if ⟨expression⟩ then ⟨block⟩⟨else_choice⟩
            checkToken(p, TOK_KW_IF);
            //precedence analysis
            checkToken(p, TOK_KW_THEN);
            parseBlock(p);
            parseElseChoice(p);
            break;
        default:
            printError("Unexpected token");
            break;
    }
}

void parseRetType(ParsedToken *p)
{
    switch (p->currentToken.type)
    {
        case TOK_COLON: //31: ⟨ret_type⟩ →: ⟨type⟩
            checkToken(p, TOK_COLON);
            parseType(p);
            break;
        case TOK_EQ: //32: ⟨ret_type⟩ → epsilon
            return;
        default:
            printError("Unexpected token");
            break;
    }
}

void parseType(ParsedToken *p)
{
    switch (p->currentToken.type)
    {
        case TOK_KW_INT: //33: ⟨type⟩ → Int
            checkToken(p, TOK_KW_INT);
            break;
        case TOK_KW_DOUBLE: //34: ⟨type⟩ → Double
            checkToken(p, TOK_KW_DOUBLE);
            break;
        case TOK_KW_STRING: //35: ⟨type⟩ → String
            checkToken(p, TOK_KW_STRING);
            break;
        case TOK_KW_UNIT: //36: ⟨type⟩ → Unit
            checkToken(p, TOK_KW_UNIT);
            break;
        case TOK_KW_BOOLEAN: //37: ⟨type⟩ → Boolean
            checkToken(p, TOK_KW_BOOLEAN);
            break;
        default:
            printError("Unexpected token");
            break;
    }
}

void parseReturn(ParsedToken *p)
{
    //TODO //38: ⟨return⟩ → ⟨expression⟩
    
    //39: ⟨return⟩ → epsilon
    if (p->currentToken.type == TOK_NL || p->currentToken.type == TOK_SEMICOL)
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
    switch (p->currentToken.type)
    {
        case TOK_ID: //40: ⟨param_list⟩ → id : ⟨type⟩⟨param_end⟩
            checkToken(p, TOK_ID);
            checkToken(p, TOK_COLON);
            parseType(p);
            parseParamEnd(p);
            break;
        case TOK_RPAR: //41: ⟨param_list⟩ → epsilon
            return;
        default:
            printError("Unexpected token");
            break;
    }
}

void parseParamEnd(ParsedToken *p)
{
    switch (p->currentToken.type)
    {
        case TOK_COMMA: //42: ⟨param_end⟩ →,⟨param_list⟩
            checkToken(p, TOK_COMMA);
            parseParamList(p);
            break;
        case TOK_RPAR: //43: ⟨param_end⟩ → epsilon
            return;
        default:
            printError("Unexpected token");
            break;
    }
}

void parseLop(ParsedToken *p)
{
    switch(p->currentToken.type)
    {
        case TOK_LPAR: //44: ⟨lop⟩ → (⟨param_list⟩)⟨lop⟩
            checkToken(p, TOK_LPAR);
            parseParamList(p);
            checkToken(p, TOK_RPAR);
            parseLop(p);
            break;
        case TOK_COLON: //45: ⟨lop⟩ → epsilon
        case TOK_EQ:
            return;
        default:
            printError("Unexpected token");
            break;
    }
}

void parseArgList(ParsedToken *p)
{
    //TODO //48: ⟨arg_list⟩ → ⟨expression⟩⟨arg_end⟩
    //49: ⟨arg_list⟩ → epsilon
    if (p->currentToken.type == TOK_RPAR)
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
    switch (p->currentToken.type)
    {
        case TOK_COMMA: //50: ⟨arg_end⟩ →,⟨arg_list⟩
            checkToken(p, TOK_COMMA);
            parseArgList(p);
            break;
        case TOK_RPAR: //51: ⟨arg_end⟩ → epsilon
            return;
        default:
            printError("Unexpected token");
            break;
    }
}

//TODO: precedence analysis
void parseExpression()
{

}
/*** End of file parser.c ***/