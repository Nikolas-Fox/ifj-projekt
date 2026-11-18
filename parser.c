/******************************************************************************
* Project:		IFJ
* Package:		parser
* File:			parser.c
* Date:			1.11.2026
* Last change:	19.11.2026
* Authors: 		Eliška Holá xholael00, Nikolas Fox xfoxnik00
*
* Description: Implementation of parser - recursive descent
*
*******************************************************************************/
/**
* @file parser.c
*
* @brief Implementation of parser
* @author Eliška Holá xholael00 (recursive descent parsing)
* @author Nikolas Fox xfoxnik00 (AST nodes)
*/

#include "parser.h"
#include "precedence.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

static void printError(char *message);
static void checkStrToken(ParsedToken *p, TokenType expected, char *name);
static bool startsStat(TokenType type);
static bool startsLine(TokenType type);

static AstNode *parseProg(ParsedToken *p);
static AstNode *parseFunction(ParsedToken *p);
static AstNode *parseDefinition(ParsedToken *p);
static AstNode *parseDefBody(ParsedToken *p);
static void parseDefBlock(ParsedToken *p);
static void parseExtensionDef(ParsedToken *p);
static void parseOptNL(ParsedToken *p);
static AstNode *parseBlock(ParsedToken *p);
static AstNode *parseStList(ParsedToken *p);
static AstNode *parseStat(ParsedToken *p);
static AstNode *parseLine(ParsedToken *p);
static AstNode *parseLineEnd(ParsedToken *p);
static AstNode *parseLineSemi(ParsedToken *p);
static AstNode *parseElseChoice(ParsedToken *p);
static AstNode *parseElse(ParsedToken *p);
static DataType parseRetType(ParsedToken *p);
static DataType parseType(ParsedToken *p);
static AstNode *parseReturn(ParsedToken *p);
static AstNode *parseParamList(ParsedToken *p, size_t *count);
static AstNode *parseParamEnd(ParsedToken *p, size_t *count);
static AstNode *parseLop(ParsedToken *p, size_t *groupCount);
static AstNode *parseId(ParsedToken *p);
// parseArgList, parseLoa, parseArgEnd are in precedence.c, as they aren't used in parser


AstNode *parseProgram()
{
    ParsedToken token;
    token.lookAhead = false;
    getNextToken(&token);

    AstNode *whole = createNode(AST_WHOLE);
    whole->data.whole.func = parseProg(&token);
    return whole;
}

/**
 * @brief Prints error message and exits with code 2 (syntax analysis error).
 * @param message Error message to print.
 */
static void printError(char *message)
{
    fprintf(stderr, "Syntactic analysis error: %s\n", message);
    exit(2);
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

void checkToken(ParsedToken *p, TokenType expected)
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

bool startsExpression(TokenType type)
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

static AstNode *parseProg(ParsedToken *p)
{
    AstNode *functions;

    //1: ⟨prog⟩ → import⟨opt_NL⟩ifj26⟨opt_NL⟩.⟨opt_NL⟩ ∗ NL⟨definition⟩EOF
    checkToken(p, TOK_KW_IMPORT);
    parseOptNL(p);
    checkStrToken(p, TOK_ID, "ifj26");
    parseOptNL(p);
    checkToken(p, TOK_DOT);
    parseOptNL(p);
    checkToken(p, TOK_MUL);
    checkToken(p, TOK_NL);
    functions = parseDefinition(p);
    checkToken(p, TOK_EOF);

    return functions;
}
static AstNode *parseFunction(ParsedToken *p)
{
    AstNode *function = createNodeFunc(p->currentToken.str, 0, NULL, NULL, TYPE_NONE); // the name (str) will be freed
    checkToken(p, TOK_ID);
    size_t groupCount = 0;
    function->data.func.group = parseLop(p, &groupCount);
    function->data.func.groupCount = groupCount;
    
    function->type = parseRetType(p);

    checkToken(p, TOK_ASSIGN);
    function->data.func.body = parseDefBody(p);
    return function;
}

static AstNode *parseDefinition(ParsedToken *p)
{
    switch (p->currentToken.type)
    {
        case TOK_AT_MAIN: //2: ⟨definition⟩ → @main⟨opt_NL⟩def id⟨lop⟩⟨ret_type⟩ = ⟨def_body⟩⟨definition⟩
        {
            checkToken(p, TOK_AT_MAIN);
            parseOptNL(p);
            checkToken(p, TOK_KW_DEF);

            AstNode *function = parseFunction(p);
            function->data.func.isMain = true;
            insertFunc(function, parseDefinition(p));
            return function;
        }

        case TOK_KW_DEF: //3: ⟨definition⟩ → def id⟨lop⟩⟨ret_type⟩ = ⟨def_body⟩⟨definition⟩
        {
            checkToken(p, TOK_KW_DEF);
            AstNode *function = parseFunction(p);
            insertFunc(function, parseDefinition(p));
            return function;
        }

        // TODO: extension
        case TOK_KW_EXTENSION: //4: ⟨definition⟩ → extension(id : ⟨type⟩)⟨def_block⟩⟨definition⟩
        {   
            checkToken(p, TOK_KW_EXTENSION);
            checkToken(p, TOK_LPAR);
            checkToken(p, TOK_ID);
            checkToken(p, TOK_COLON);
            parseType(p);
            checkToken(p, TOK_RPAR);
            parseDefBlock(p);
            parseDefinition(p);
            return NULL;
        }

        case TOK_EOF: //5: ⟨definition⟩ → epsilon
            return NULL;
        default:
            printError("Unexpected token, expected @main/def/extension/EOF.");
            return NULL;
    }
}

static AstNode *parseDefBody(ParsedToken *p)
{
    //6: ⟨def_body⟩ → ⟨block⟩
    if (p->currentToken.type == TOK_NL)
    {
        return parseBlock(p);
    }
    //7: ⟨def_body⟩ → ⟨stat⟩
    else if (startsStat(p->currentToken.type))
    {
        return parseStat(p);
    }
    else
    {
        printError("Unexpected token");
        return NULL;
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

// TODO: parseExtensionDef
static void parseExtensionDef(ParsedToken *p)
{
    switch (p->currentToken.type)
    {
        case TOK_KW_DEF: //9: ⟨extension_def⟩ → def id ⟨lop⟩⟨ret_type⟩ = ⟨def_body⟩⟨extension_def⟩
        {
           checkToken(p, TOK_KW_DEF);
            checkToken(p, TOK_ID);
            parseLop(p, 0); // when doing this I should use &groupCount
            parseRetType(p);
            checkToken(p, TOK_ASSIGN);
            parseDefBody(p);
            parseExtensionDef(p);
            break;
        }

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
        return;
    }
}

static AstNode *parseBlock(ParsedToken *p)
{
    //13: ⟨block⟩ → NL INDENT ⟨stat⟩⟨st_list⟩ DEDENT
    checkToken(p, TOK_NL);
    checkToken(p, TOK_INDENT);
    AstNode *stat = parseStat(p);
    AstNode *statNext = parseStList(p); // not done using stat->next because stat can also be a list of statements
    checkToken(p, TOK_DEDENT);
    return insertStat(stat, statNext);
}

static AstNode *parseStList(ParsedToken *p)
{
    //14: ⟨st_list⟩ → ⟨stat⟩⟨st_list⟩
    if (startsStat(p->currentToken.type))
    {
        AstNode *stat = parseStat(p);
        AstNode *statNext = parseStList(p);
        return insertStat(stat, statNext);
    }
    //15: ⟨st_list⟩ → epsilon
    else if (p->currentToken.type == TOK_DEDENT)
    {
        return NULL;
    }
    else
    {
        printError("Unexpected token");
        return NULL;
    }
}

static AstNode *parseStat(ParsedToken *p)
{
    //16: ⟨stat⟩ → ⟨line⟩⟨line_end⟩
    if (startsLine(p->currentToken.type))
    {
        AstNode *line = parseLine(p);
        AstNode *lineNext = parseLineEnd(p);
        return insertStat(line, lineNext);
    }
    else
    {
        switch (p->currentToken.type)
        {
            case TOK_KW_IF: //17: ⟨stat⟩ → if ⟨expression⟩ then ⟨block⟩⟨else_choice⟩
            {
                checkToken(p, TOK_KW_IF);
                AstNode *condition = parseExpression(p);
                checkToken(p, TOK_KW_THEN);
                AstNode *thenBody = parseBlock(p);
                AstNode *elseBody = parseElseChoice(p);
                return createNodeIf(condition, thenBody, elseBody);
            }

            case TOK_KW_WHILE: //18: ⟨stat⟩ → while ⟨expression⟩ do ⟨block⟩
            {
                checkToken(p, TOK_KW_WHILE);
                AstNode *condition = parseExpression(p);
                checkToken(p, TOK_KW_DO);
                AstNode *body = parseBlock(p);
                return createNodeWhile(condition, body);
            }

            default:
                printError("Unexpected token");
                return NULL;
        }
    }
}

static AstNode *parseLine(ParsedToken *p)
{
    //19: ⟨line⟩ → ⟨expression⟩
    if (startsExpression(p->currentToken.type))
    {
        if (p->currentToken.type == TOK_ID)
        {
            return parseId(p);
        }
        else
        {
            return parseExpression(p);
        }
    }
    //20: ⟨line⟩ → val id ⟨ret_type⟩ = ⟨expression⟩
    else if (p->currentToken.type == TOK_KW_VAL)
    {
        checkToken(p, TOK_KW_VAL);

        AstNode *declaration = createNodeDeclaration(p->currentToken.str, false, 0, NULL); // isVar is false (value)
        checkToken(p, TOK_ID);
        declaration->type = parseRetType(p);
        checkToken(p, TOK_ASSIGN);
        declaration->data.declaration.expression = parseExpression(p);
        return declaration;
    }
    //21: ⟨line⟩ → var id ⟨ret_type⟩ = ⟨expression⟩
    else if (p->currentToken.type == TOK_KW_VAR)
    {
        checkToken(p, TOK_KW_VAR);
        AstNode *declaration = createNodeDeclaration(p->currentToken.str, true, 0, NULL); // isVar is true (variable)
        checkToken(p, TOK_ID);
        declaration->type = parseRetType(p);
        checkToken(p, TOK_ASSIGN);
        declaration->data.declaration.expression = parseExpression(p);
        return declaration;
    }
    //22: ⟨line⟩ → return ⟨return⟩
    else if (p->currentToken.type == TOK_KW_RETURN)
    {
        checkToken(p, TOK_KW_RETURN);
        return createNodeReturn(parseReturn(p));
    }
    else
    {
        printError("Unexpected token");
        return NULL;
    }
}

static AstNode *parseLineEnd(ParsedToken *p)
{
    switch (p->currentToken.type)
    {
        case TOK_SEMICOL: //23: ⟨line_end⟩ → ;⟨line_semi⟩
            checkToken(p, TOK_SEMICOL);
            return parseLineSemi(p);

        case TOK_NL: //24: ⟨line_end⟩ → NL
            checkToken(p, TOK_NL);
            return NULL;
        default:
            printError("Unexpected token");
            return NULL;
    }
}

static AstNode *parseLineSemi(ParsedToken *p)
{
    //25: ⟨line_semi⟩ → ⟨stat⟩
    if (startsStat(p->currentToken.type))
    {
        return parseStat(p);
    }
    //26: ⟨line_semi⟩ → NL
    else if (p->currentToken.type == TOK_NL)
    {
        checkToken(p, TOK_NL);
        return NULL;
    }
    else
    {
        printError("Unexpected token after if block, expected 'else' or end of statement.");
        return NULL;
    }
}

static AstNode *parseElseChoice(ParsedToken *p)
{
    //27: ⟨else_choice⟩ → else ⟨else⟩
    if (p->currentToken.type == TOK_KW_ELSE)
    {
        checkToken(p, TOK_KW_ELSE);
        return parseElse(p);
    }
    // 28: ⟨else_choice⟩ → epsilon
    else if (startsStat(p->currentToken.type) || p->currentToken.type == TOK_AT_MAIN || p->currentToken.type == TOK_KW_DEF ||
            p->currentToken.type == TOK_KW_EXTENSION || p->currentToken.type == TOK_EOF || p->currentToken.type == TOK_DEDENT)
    {
        return NULL;
    }
    else
    {
        printError("Unexpected token after if block, expected 'else' or end of statement.");
        return NULL;
    }
}

static AstNode *parseElse(ParsedToken *p)
{
    switch (p->currentToken.type)
    {
        case TOK_NL: //29: ⟨else⟩ → ⟨block⟩
        {
            AstNode *block = parseBlock(p); // block is a pointer to the first statement of the list of statements
            return block;
        }

        case TOK_KW_IF: //30: ⟨else⟩ → if ⟨expression⟩ then ⟨block⟩⟨else_choice⟩
        {
            checkToken(p, TOK_KW_IF);
            AstNode *condition = parseExpression(p);
            checkToken(p, TOK_KW_THEN);
            AstNode *thenBody = parseBlock(p);
            AstNode *elseBody = parseElseChoice(p);
            return createNodeIf(condition, thenBody, elseBody);
        }

        default:
            printError("Unexpected token");
            return NULL;
    }
}

static DataType parseRetType(ParsedToken *p)
{
    switch (p->currentToken.type)
    {
        case TOK_COLON: //31: ⟨ret_type⟩ →: ⟨type⟩
            checkToken(p, TOK_COLON);
            return parseType(p);

        case TOK_ASSIGN: //32: ⟨ret_type⟩ → epsilon
            return TYPE_NONE;
            
        default:
            printError("Unexpected token");
            return TYPE_NONE;
    }
}

static DataType parseType(ParsedToken *p)
{
    switch (p->currentToken.type)
    {
        case TOK_KW_INT: //33: ⟨type⟩ → Int
            checkToken(p, TOK_KW_INT);
            return TYPE_INT;
        case TOK_KW_DOUBLE: //34: ⟨type⟩ → Double
            checkToken(p, TOK_KW_DOUBLE);
            return TYPE_DOUBLE;
        case TOK_KW_STRING: //35: ⟨type⟩ → String
            checkToken(p, TOK_KW_STRING);
            return TYPE_STRING;
        case TOK_KW_UNIT: //36: ⟨type⟩ → Unit
            checkToken(p, TOK_KW_UNIT);
            return TYPE_UNIT;
        case TOK_KW_BOOLEAN: //37: ⟨type⟩ → Boolean
            checkToken(p, TOK_KW_BOOLEAN);
            return TYPE_BOOL;
        default:
            printError("Unexpected token");
            return TYPE_NONE;
    }
}

static AstNode *parseReturn(ParsedToken *p)
{
    //38: ⟨return⟩ → ⟨expression⟩
    if (startsExpression(p->currentToken.type))
    {
        return parseExpression(p);
    }
    //39: ⟨return⟩ → epsilon
    else if (p->currentToken.type == TOK_NL || p->currentToken.type == TOK_SEMICOL)
    {
        return NULL;
    }
    else
    {
        printError("Unexpected token");
        return NULL;
    }
}

static AstNode *parseParamList(ParsedToken *p, size_t *count)
{
    switch (p->currentToken.type)
    {
        case TOK_ID: //40: ⟨param_list⟩ → id : ⟨type⟩⟨param_end⟩
        {
            AstNode *param = createNodeParam(p->currentToken.str, TYPE_NONE);
            (*count)++;
            checkToken(p, TOK_ID);
            checkToken(p, TOK_COLON);
            param->type = parseType(p);
            param->next = parseParamEnd(p, count);
            return param;
        }
        
        case TOK_RPAR: //41: ⟨param_list⟩ → epsilon
            return NULL;
        default:
            printError("Unexpected token");
            return NULL;
    }
}

static AstNode *parseParamEnd(ParsedToken *p, size_t *count)
{
    switch (p->currentToken.type)
    {
        case TOK_COMMA: //42: ⟨param_end⟩ →,⟨param_list⟩
            checkToken(p, TOK_COMMA);
            return parseParamList(p, count);
        case TOK_RPAR: //43: ⟨param_end⟩ → epsilon
            return NULL;
        default:
            printError("Unexpected token");
            return NULL;
    }
}

static AstNode *parseLop(ParsedToken *p, size_t *groupCount)
{
    switch(p->currentToken.type)
    {
        case TOK_LPAR: //44: ⟨lop⟩ → (⟨param_list⟩)⟨lop⟩
        {
            checkToken(p, TOK_LPAR);
            size_t count = 0;
            AstNode *params = parseParamList(p, &count);
            checkToken(p, TOK_RPAR);

            AstNode *group = createNodeGroup(params, count);
            (*groupCount)++;
                
            group->next = parseLop(p, groupCount);
            return group;
        }
        case TOK_COLON: //45: ⟨lop⟩ → epsilon
        case TOK_ASSIGN:
            return NULL;
        default:
            printError("Unexpected token");
            return NULL;
    }
}

static AstNode *parseId(ParsedToken *p)
{
    getTokenAhead(p);
    //53: ⟨assign⟩ → id = ⟨expression⟩
    if (p->aheadToken.type == TOK_ASSIGN)
    {
        AstNode *destination = createNodeID(p->currentToken.str);
        checkToken(p, TOK_ID);
        checkToken(p, TOK_ASSIGN);

        AstNode *expression = parseExpression(p);
        return createNodeAssign(destination, expression);
    }
    else
    {
        return parseExpression(p);
    }
}
/*** End of file parser.c ***/