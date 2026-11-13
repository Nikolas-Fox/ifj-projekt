/******************************************************************************
* Project:		IFJ
* Package:		parser
* File:			parser.c
* Date:			1.11.2026
* Last change:	13.11.2026
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
#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

//recursive descent functions prototypes
static void parseProg(ParsedToken *p);
static void parseDefinition(ParsedToken *p);
static void parseDefBody(ParsedToken *p);
static void parseDefBlock(ParsedToken *p);
static void parseExtensionDef(ParsedToken *p);
static void parseOptNL(ParsedToken *p);
static void parseBlock(ParsedToken *p);
static void parseStList(ParsedToken *p);
static void parseStat(ParsedToken *p);
static void parseLine(ParsedToken *p);
static void parseLineEnd(ParsedToken *p);
static void parseLineSemi(ParsedToken *p);
static void parseElseChoice(ParsedToken *p);
static void parseElse(ParsedToken *p);
static void parseRetType(ParsedToken *p);
static void parseType(ParsedToken *p);
static void parseReturn(ParsedToken *p);
static void parseParamList(ParsedToken *p);
static void parseParamEnd(ParsedToken *p);
static void parseLop(ParsedToken *p);
static void parseId(ParsedToken *p);

/**
 * @brief Prints error message and exits with code 2 (syntax analysis error).
 * @param message Error message to print.
 */
static void printError(char *message)
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

void getTokenAhead(ParsedToken *p)
{
    if (!p->lookAhead)
    {
        p->lookAhead = true;
        p->aheadToken = getToken();
    }
}

/**
 * @brief Checks if token has the expected type and calls getToken() for the next one / calls printError if it doesn't.
 * @param p Struct of parser state
 * @param expected Expected token type
 */
static void checkToken(ParsedToken *p, TokenType expected)
{
    if (p->currentToken.type != expected)
    {
        printError("Unexpected token");
    }
    else
    {
        getNextToken(p);
    }
}

/**
 * @brief Checks if token has the expected type and name (used for TOK_ID), calls getToken() for the next one / calls printError if it doesn't.
 * @param p Struct of parser state
 * @param expected Expected token type
 * @param name Name of id
 */
static void checkStrToken(ParsedToken *p, TokenType expected, char *name)
{
    if (p->currentToken.type != expected || p->currentToken.str == NULL || strcmp(p->currentToken.str, name) != 0)
    {
        printError("Unexpected token");
    }
    getNextToken(p);
}

/**
 * @brief Checks if token type belongs to Predict(<expression>).
 * @param type Token type
 * @return True if it belongs, false if it doesn't.
 */
static bool startsExpression(TokenType type)
{
    switch (type)
    {
        case TOK_PLUS:
        case TOK_MINUS:
        case TOK_NOT:
        case TOK_LPAR:
        case TOK_ID:
        case TOK_INT:
        case TOK_DOUBLE:
        case TOK_STRING:
            return true;
        default:
            return false;
    }
}

/**
 * @brief Checks if token type belongs to Predict(<stat>).
 * @param type Token type
 * @return True if it belongs, false if it doesn't.
 */
static bool startsStat(TokenType type)
{
    if (startsExpression(type))
    {
        return true;
    }
    
    switch (type)
    {
        case TOK_KW_IF:
        case TOK_KW_WHILE:
        case TOK_KW_VAR:
        case TOK_KW_VAL:
        case TOK_KW_RETURN:
            return true;
        default:
            return false;
    }
}

/**
 * @brief Checks if token type belongs to Predict(<line>).
 * @param type Token type
 * @return True if it belongs, false if it doesn't.
 */
static bool startsLine(TokenType type)
{
    if (startsExpression(type))
    {
        return true;
    }

    switch (type)
    {
        case TOK_KW_VAR:
        case TOK_KW_VAL:
        case TOK_KW_RETURN:
            return true;
        default:
            return false;
    }
}

//TODO: integrate with lexer
static void parseProg(ParsedToken *p)
{
    //1: ⟨prog⟩ → import⟨opt_NL⟩ifj26⟨opt_NL⟩.⟨opt_NL⟩ ∗ NL⟨definition⟩EOF
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

static void parseDefinition(ParsedToken *p)
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

static void parseDefBody(ParsedToken *p)
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

static void parseDefBlock(ParsedToken *p)
{
    //8: ⟨def_block⟩ → NL INDENT ⟨extension_def⟩ DEDENT
    checkToken(p, TOK_NL);
    checkToken(p, TOK_INDENT);
    parseExtensionDef(p);
    checkToken(p, TOK_DEDENT);
}

static void parseExtensionDef(ParsedToken *p)
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

static void parseOptNL(ParsedToken *p)
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

static void parseBlock(ParsedToken *p)
{
    //13: ⟨block⟩ → NL INDENT ⟨stat⟩⟨st_list⟩ DEDENT
    checkToken(p, TOK_NL);
    checkToken(p, TOK_INDENT);
    parseStat(p);
    parseStList(p);
    checkToken(p, TOK_DEDENT);
}

static void parseStList(ParsedToken *p)
{
    //14: ⟨st_list⟩ → ⟨stat⟩⟨st_list⟩
    if (startsStat(p->currentToken.type))
    {
        parseStat(p);
        parseStList(p);
    }
    //15: ⟨st_list⟩ → epsilon
    else if (p->currentToken.type == TOK_DEDENT)
    {
        return;
    }
    else
    {
        printError("Unexpected token");
    }
}

static void parseStat(ParsedToken *p)
{
    //16: ⟨stat⟩ → ⟨line⟩⟨line_end⟩
    if (startsLine(p->currentToken.type))
    {
        parseLine(p);
        parseLineEnd(p);
    }
    else
    {
        switch (p->currentToken.type)
        {
            case TOK_KW_IF: //17: ⟨stat⟩ → if ⟨expression⟩ then ⟨block⟩⟨else_choice⟩
                checkToken(p, TOK_KW_IF);
                parseExpression(p);
                checkToken(p, TOK_KW_THEN);
                parseBlock(p);
                parseElseChoice(p);
                break;
            case TOK_KW_WHILE: //18: ⟨stat⟩ → while ⟨expression⟩ do ⟨block⟩
                checkToken(p, TOK_KW_WHILE);
                parseExpression(p);
                checkToken(p, TOK_KW_DO);
                parseBlock(p);
                break;
            default:
                printError("Unexpected token");
                break;
        }
    }
}

static void parseLine(ParsedToken *p)
{
    //19: ⟨line⟩ → ⟨expression⟩
    if (startsExpression(p->currentToken.type))
    {
        if (p->currentToken.type == TOK_ID)
        {
            parseId(p);
        }
        else
        {
            parseExpression(p);
        }
    }
    //20: ⟨line⟩ → val id ⟨ret_type⟩ = ⟨expression⟩
    else if (p->currentToken.type == TOK_KW_VAL)
    {
        checkToken(p, TOK_KW_VAL);
        checkToken(p, TOK_ID);
        parseRetType(p);
        checkToken(p, TOK_EQ);
        parseExpression(p);
    }
    //21: ⟨line⟩ → var id ⟨ret_type⟩ = ⟨expression⟩
    else if (p->currentToken.type == TOK_KW_VAR)
    {
        checkToken(p, TOK_KW_VAR);
        checkToken(p, TOK_ID);
        parseRetType(p);
        checkToken(p, TOK_EQ);
        parseExpression(p);
    }
    //22: ⟨line⟩ → return ⟨return⟩
    else if (p->currentToken.type == TOK_KW_RETURN)
    {
        checkToken(p, TOK_KW_RETURN);
        parseReturn(p);
    }
    else
    {
        printError("Unexpected token");
    }
}

static void parseLineEnd(ParsedToken *p)
{
    switch (p->currentToken.type)
    {
        case TOK_SEMICOL: //23: ⟨line_end⟩ → ;⟨line_semi⟩
            checkToken(p, TOK_SEMICOL);
            parseLineSemi(p);
            break;
        case TOK_NL: //24: ⟨line_end⟩ → NL
            checkToken(p, TOK_NL);
            break;
        default:
            printError("Unexpected token");
            break;
    }
}

static void parseLineSemi(ParsedToken *p)
{
    //25: ⟨line_semi⟩ → ⟨stat⟩
    if (startsStat(p->currentToken.type))
    {
        parseStat(p);
    }
    //26: ⟨line_semi⟩ → NL
    else if (p->currentToken.type == TOK_NL)
    {
        checkToken(p, TOK_NL);
    }
    else
    {
        printError("Unexpected token after if block, expected 'else' or end of statement.");
    }
}

static void parseElseChoice(ParsedToken *p)
{
    //27: ⟨else_choice⟩ → else ⟨else⟩
    if (p->currentToken.type == TOK_KW_ELSE)
    {
        checkToken(p, TOK_KW_ELSE);
        parseElse(p);
    }
    // 28: ⟨else_choice⟩ → epsilon
    else if (startsStat(p->currentToken.type) || p->currentToken.type == TOK_AT_MAIN || p->currentToken.type == TOK_KW_DEF ||
            p->currentToken.type == TOK_KW_EXTENSION || p->currentToken.type == TOK_EOF || p->currentToken.type == TOK_DEDENT)
    {
        return;
    }
    else
    {
        printError("Unexpected token after if block, expected 'else' or end of statement.");
    }
}

static void parseElse(ParsedToken *p)
{
    switch (p->currentToken.type)
    {
        case TOK_NL: //29: ⟨else⟩ → ⟨block⟩
            parseBlock(p);
            break;
        case TOK_KW_IF: //30: ⟨else⟩ → if ⟨expression⟩ then ⟨block⟩⟨else_choice⟩
            checkToken(p, TOK_KW_IF);
            parseExpression(p);
            checkToken(p, TOK_KW_THEN);
            parseBlock(p);
            parseElseChoice(p);
            break;
        default:
            printError("Unexpected token");
            break;
    }
}

static void parseRetType(ParsedToken *p)
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

static void parseType(ParsedToken *p)
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

static void parseReturn(ParsedToken *p)
{
    //38: ⟨return⟩ → ⟨expression⟩
    if (startsExpression(p->currentToken.type))
    {
        parseExpression(p);
    }
    //39: ⟨return⟩ → epsilon
    else if (p->currentToken.type == TOK_NL || p->currentToken.type == TOK_SEMICOL)
    {
        return;
    }
    else
    {
        printError("Unexpected token");
    }
}

static void parseParamList(ParsedToken *p)
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

static void parseParamEnd(ParsedToken *p)
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

static void parseLop(ParsedToken *p)
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

static void parseId(ParsedToken *p)
{
    getTokenAhead(p);
    //53: ⟨assign⟩ → id = ⟨expression⟩
    if (p->aheadToken.type == TOK_EQ)
    {
        checkToken(p, TOK_ID);
        checkToken(p, TOK_EQ);
        parseExpression(p);
    }
    else
    {
        parseExpression(p);
    }
}
/*** End of file parser.c ***/