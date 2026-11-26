/******************************************************************************
 * Project:		IFJ
 * Package:		parser
 * File:			parser.c
 * Date:			1.11.2026
 * Last change:	26.11.2026
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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void printError(char *message);
static void checkStrToken(ParsedToken *p, TokenType expected, char *name);
static bool startsStat(TokenType type);
static bool startsLine(TokenType type);

static AstNode *parseProg(ParsedToken *p);
static AstNode *parseFunction(ParsedToken *p);
static AstNode *parseDefinition(ParsedToken *p);
static AstNode *parseDefBody(ParsedToken *p);
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
// parseArgList, parseLoa, parseArgEnd are in precedence.c, as they aren't used
// in parser

AstNode *parseProgram(Scanner *sc) {
    ParsedToken token = {0};
    token.sc = sc;
    token.lookAhead = false;
    token.error = 0;
    getNextToken(&token);

    AstNode *whole = createNode(AST_WHOLE);
    whole->data.whole.func = parseProg(&token);
    if (token.error != 0)
    {
        astFree(whole);
        return NULL;
    }
    return whole;
}

/**
 * @brief Prints error message.
 * @param message Error message to print.
 */
static void printError(char *message) {
    fprintf(stderr, "Syntactic analysis error: %s\n", message);
}

void getNextToken(ParsedToken *p) {
    if (p->currentToken.str != NULL)
    {
        free(p->currentToken.str);
        p->currentToken.str = NULL;
    }

    if (p->lookAhead) {
        p->currentToken = p->aheadToken;
        p->lookAhead = false;
    } else {
        p->currentToken = getToken(p->sc);
        if (p->sc->error != 0 && p->error == 0)
        {
            p->error = p->sc->error;
            return;
        }
    }
}

void getTokenAhead(ParsedToken *p) {
    if (!p->lookAhead) {
        p->lookAhead = true;
        p->aheadToken = getToken(p->sc);
        if (p->sc->error != 0 && p->error == 0)
        {
            p->error = p->sc->error;
            return;
        }
    }
}

void checkToken(ParsedToken *p, TokenType expected) {
    if (p->currentToken.type != expected) {
        printError("Unexpected token");
        p->error = 2;
        return;
    } else {
        getNextToken(p);
        if (p->error != 0)
        {
            return;
        }
    }
}

/**
 * @brief Checks if token has the expected type and name (used for TOK_ID),
 * calls getToken() for the next one / calls printError if it doesn't.
 * @param p Struct of parser state
 * @param expected Expected token type
 * @param name Name of id
 */
static void checkStrToken(ParsedToken *p, TokenType expected, char *name) {
    if (p->currentToken.type != expected || p->currentToken.str == NULL ||
        strcmp(p->currentToken.str, name) != 0) {
        printError("Unexpected token");
        p->error = 2;
        return;
    }
    getNextToken(p);
    if (p->error != 0)
    {
        return;
    }
}

bool startsExpression(TokenType type) {
    switch (type) {
    case TOK_PLUS:
    case TOK_MINUS:
    case TOK_NOT:
    case TOK_LPAR:
    case TOK_ID:
    case TOK_INT:
    case TOK_DOUBLE:
    case TOK_STRING:
    case TOK_INTERP_START:
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
static bool startsStat(TokenType type) {
    if (startsExpression(type)) {
        return true;
    }

    switch (type) {
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
static bool startsLine(TokenType type) {
    if (startsExpression(type)) {
        return true;
    }

    switch (type) {
    case TOK_KW_VAR:
    case TOK_KW_VAL:
    case TOK_KW_RETURN:
        return true;
    default:
        return false;
    }
}

static AstNode *parseProg(ParsedToken *p) {
    AstNode *functions;

    // 1: ⟨prog⟩ → import⟨opt_NL⟩ifj26⟨opt_NL⟩.⟨opt_NL⟩ ∗ NL⟨definition⟩EOF
    checkToken(p, TOK_KW_IMPORT);
    if (p->error != 0)
    {
        return NULL;
    }
    parseOptNL(p);
    if (p->error != 0)
    {
        return NULL;
    }
    checkStrToken(p, TOK_ID, "ifj26");
    if (p->error != 0)
    {
        return NULL;
    }
    parseOptNL(p);
    if (p->error != 0)
    {
        return NULL;
    }
    checkToken(p, TOK_DOT);
    if (p->error != 0)
    {
        return NULL;
    }
    parseOptNL(p);
    if (p->error != 0)
    {
        return NULL;
    }
    checkToken(p, TOK_MUL);
    if (p->error != 0)
    {
        return NULL;
    }
    checkToken(p, TOK_NL);
    if (p->error != 0)
    {
        return NULL;
    }
    functions = parseDefinition(p);
    checkToken(p, TOK_EOF);
    if (p->error != 0)
    {
        return NULL;
    }

    return functions;
}

static AstNode *parseFunction(ParsedToken *p) {
    AstNode *function =
        createNodeFunc(p->currentToken.str, 0, NULL, NULL,
                       TYPE_NONE); // the name (str) will be freed
    checkToken(p, TOK_ID);
    if (p->error != 0)
    {
        return NULL;
    }
    size_t groupCount = 0;
    function->data.func.group = parseLop(p, &groupCount);
    if (p->error != 0)
    {
        astFree(function);
        return NULL;
    }
    function->data.func.groupCount = groupCount;

    function->type = parseRetType(p);
    if (p->error != 0)
    {
        astFree(function);
        return NULL;
    }

    checkToken(p, TOK_ASSIGN);
    if (p->error != 0)
    {
        return NULL;
    }
    function->data.func.body = parseDefBody(p);
    if (p->error != 0)
    {
        astFree(function);
        return NULL;
    }
    return function;
}

static AstNode *parseDefinition(ParsedToken *p) {
    switch (p->currentToken.type) {
    case TOK_AT_MAIN: // 2: ⟨definition⟩ → @main⟨opt_NL⟩def id⟨lop⟩⟨ret_type⟩ =
                      // ⟨def_body⟩⟨definition⟩
    {
        checkToken(p, TOK_AT_MAIN);
        if (p->error != 0)
        {
            return NULL;
        }
        parseOptNL(p);
        if (p->error != 0)
        {
            return NULL;
        }
        checkToken(p, TOK_KW_DEF);
        if (p->error != 0)
        {
            return NULL;
        }

        AstNode *function = parseFunction(p);
        if (p->error != 0)
        {
            return NULL;
        }
        function->data.func.isMain = true;
        insertFunc(function, parseDefinition(p));
        if (p->error != 0)
        {
            return NULL;
        }
        return function;
    }

    case TOK_KW_DEF: // 3: ⟨definition⟩ → def id⟨lop⟩⟨ret_type⟩ =
                     // ⟨def_body⟩⟨definition⟩
    {
        checkToken(p, TOK_KW_DEF);
        if (p->error != 0)
        {
            return NULL;
        }
        AstNode *function = parseFunction(p);
        if (p->error != 0)
        {
            return NULL;
        }
        insertFunc(function, parseDefinition(p));
        if (p->error != 0)
        {
            return NULL;
        }
        return function;
    }

    case TOK_EOF: // 4: ⟨definition⟩ → epsilon
        return NULL;
    default:
        printError("Unexpected token, expected @main/def/extension/EOF.");
        p->error = 2;
        return NULL;
    }
}

static AstNode *parseDefBody(ParsedToken *p) {
    // 5: ⟨def_body⟩ → ⟨block⟩
    if (p->currentToken.type == TOK_NL) {
        return parseBlock(p);
    }
    // 6: ⟨def_body⟩ → ⟨stat⟩
    else if (startsStat(p->currentToken.type)) {
        return parseStat(p);
    } else {
        printError("Unexpected token");
        p->error = 2;
        return NULL;
    }
}

static void parseOptNL(ParsedToken *p) {
    // 7: ⟨opt_NL⟩ → NL
    if (p->currentToken.type == TOK_NL) {
        checkToken(p, TOK_NL);
        if (p->error != 0)
        {
            return;
        }
    }
    // 8: ⟨opt_NL⟩ → epsilon
    else if ((p->currentToken.type == TOK_ID &&
              (strcmp(p->currentToken.str, "ifj26") == 0)) ||
             p->currentToken.type == TOK_DOT ||
             p->currentToken.type == TOK_MUL ||
             p->currentToken.type == TOK_KW_DEF) {
        return;
    } else {
        printError("Unexpected token");
        p->error = 2;
        return;
    }
}

static AstNode *parseBlock(ParsedToken *p) {
    // 9: ⟨block⟩ → NL INDENT ⟨stat⟩⟨st_list⟩ DEDENT
    checkToken(p, TOK_NL);
    if (p->error != 0)
    {
        return NULL;
    }
    checkToken(p, TOK_INDENT);
    if (p->error != 0)
    {
        return NULL;
    }
    AstNode *stat = parseStat(p);
    if (p->error != 0)
    {
        return NULL;
    }
    AstNode *statNext = parseStList(p); // not done using stat->next because
                                        // stat can also be a list of statements
    if (p->error != 0)
    {
        return NULL;
    }
    checkToken(p, TOK_DEDENT);
    if (p->error != 0)
    {
        return NULL;
    }
    return insertStat(stat, statNext);
}

static AstNode *parseStList(ParsedToken *p) {
    // 10: ⟨st_list⟩ → ⟨stat⟩⟨st_list⟩
    if (startsStat(p->currentToken.type)) {
        AstNode *stat = parseStat(p);
        if (p->error != 0)
        {
            return NULL;
        }
        AstNode *statNext = parseStList(p);
        if (p->error != 0)
        {
            return NULL;
        }
        return insertStat(stat, statNext);
    }
    // 11: ⟨st_list⟩ → epsilon
    else if (p->currentToken.type == TOK_DEDENT) {
        return NULL;
    } else {
        printError("Unexpected token");
        p->error = 2;
        return NULL;
    }
}

static AstNode *parseStat(ParsedToken *p) {
    // 12: ⟨stat⟩ → ⟨line⟩⟨line_end⟩
    if (startsLine(p->currentToken.type)) {
        AstNode *line = parseLine(p);
        if (p->error != 0)
        {
            return NULL;
        }
        AstNode *lineNext = parseLineEnd(p);
        if (p->error != 0)
        {
            return NULL;
        }
        return insertStat(line, lineNext);
    } else {
        switch (p->currentToken.type) {
        case TOK_KW_IF: // 13: ⟨stat⟩ → if ⟨expression⟩ then
                        // ⟨block⟩⟨else_choice⟩
        {
            checkToken(p, TOK_KW_IF);
            if (p->error != 0)
            {
                return NULL;
            }
            AstNode *condition = parseExpression(p);
            if (p->error != 0)
            {
                return NULL;
            }
            checkToken(p, TOK_KW_THEN);
            if (p->error != 0)
            {
                return NULL;
            }
            AstNode *thenBody = parseBlock(p);
            if (p->error != 0)
            {
                return NULL;
            }
            AstNode *elseBody = parseElseChoice(p);
            if (p->error != 0)
            {
                return NULL;
            }
            return createNodeIf(condition, thenBody, elseBody);
        }

        case TOK_KW_WHILE: // 14: ⟨stat⟩ → while ⟨expression⟩ do ⟨block⟩
        {
            checkToken(p, TOK_KW_WHILE);
            if (p->error != 0)
            {
                return NULL;
            }
            AstNode *condition = parseExpression(p);
            if (p->error != 0)
            {
                return NULL;
            }
            checkToken(p, TOK_KW_DO);
            if (p->error != 0)
            {
                return NULL;
            }
            AstNode *body = parseBlock(p);
            if (p->error != 0)
            {
                return NULL;
            }
            return createNodeWhile(condition, body);
        }

        default:
            printError("Unexpected token");
            p->error = 2;
            return NULL;
        }
    }
}

static AstNode *parseLine(ParsedToken *p) {
    // 15: ⟨line⟩ → ⟨expression⟩
    if (startsExpression(p->currentToken.type)) {
        if (p->currentToken.type == TOK_ID) {
            return parseId(p);
        } else {
            return parseExpression(p);
        }
    }
    // 17: ⟨line⟩ → val id ⟨ret_type⟩ = ⟨expression⟩
    else if (p->currentToken.type == TOK_KW_VAL) {
        checkToken(p, TOK_KW_VAL);
        if (p->error != 0)
        {
            return NULL;
        }

        AstNode *declaration = createNodeDeclaration(
            p->currentToken.str, false, 0, NULL); // isVar is false (value)
        checkToken(p, TOK_ID);
        if (p->error != 0)
        {
            astFree(declaration);
            return NULL;
        }
        declaration->type = parseRetType(p);
        if (p->error != 0)
        {
            astFree(declaration);
            return NULL;
        }
        checkToken(p, TOK_ASSIGN);
        if (p->error != 0)
        {
            astFree(declaration);
            return NULL;
        }
        declaration->data.declaration.expression = parseExpression(p);
        if (p->error != 0)
        {
            astFree(declaration);
            return NULL;
        }
        return declaration;
    }
    // 18: ⟨line⟩ → var id ⟨ret_type⟩ = ⟨expression⟩
    else if (p->currentToken.type == TOK_KW_VAR) {
        checkToken(p, TOK_KW_VAR);
        if (p->error != 0)
        {
            return NULL;
        }
        AstNode *declaration = createNodeDeclaration(
            p->currentToken.str, true, 0, NULL); // isVar is true (variable)
        checkToken(p, TOK_ID);
        if (p->error != 0)
        {
            astFree(declaration);
            return NULL;
        }
        declaration->type = parseRetType(p);
        if (p->error != 0)
        {
            astFree(declaration);
            return NULL;
        }
        checkToken(p, TOK_ASSIGN);
        if (p->error != 0)
        {
            return NULL;
        }
        declaration->data.declaration.expression = parseExpression(p);
        if (p->error != 0)
        {
            astFree(declaration);
            return NULL;
        }
        return declaration;
    }
    // 19: ⟨line⟩ → return ⟨return⟩
    else if (p->currentToken.type == TOK_KW_RETURN) {
        checkToken(p, TOK_KW_RETURN);
        if (p->error != 0)
        {
            return NULL;
        }
        return createNodeReturn(parseReturn(p));
    } else {
        printError("Unexpected token");
        p->error = 2;
        return NULL;
    }
}

static AstNode *parseLineEnd(ParsedToken *p) {
    switch (p->currentToken.type) {
    case TOK_SEMICOL: // 20: ⟨line_end⟩ → ;⟨line_semi⟩
        checkToken(p, TOK_SEMICOL);
        if (p->error != 0)
        {
            return NULL;
        }
        return parseLineSemi(p);

    case TOK_NL: // 21: ⟨line_end⟩ → NL
        checkToken(p, TOK_NL);
        if (p->error != 0)
        {
            return NULL;
        }
        return NULL;
    default:
        printError("Unexpected token");
        p->error = 2;
        return NULL;
    }
}

static AstNode *parseLineSemi(ParsedToken *p) {
    // 22: ⟨line_semi⟩ → ⟨stat⟩
    if (startsStat(p->currentToken.type)) {
        return parseStat(p);
    }
    // 23: ⟨line_semi⟩ → NL
    else if (p->currentToken.type == TOK_NL) {
        checkToken(p, TOK_NL);
        if (p->error != 0)
        {
            return NULL;
        }
        return NULL;
    } else {
        printError("Unexpected token after if block, expected 'else' or end of "
                   "statement.");
        p->error = 2;
        return NULL;
    }
}

static AstNode *parseElseChoice(ParsedToken *p) {
    // 24: ⟨else_choice⟩ → else ⟨else⟩
    if (p->currentToken.type == TOK_KW_ELSE) {
        checkToken(p, TOK_KW_ELSE);
        if (p->error != 0)
        {
            return NULL;
        }
        return parseElse(p);
    }
    // 25: ⟨else_choice⟩ → epsilon
    else if (startsStat(p->currentToken.type) ||
             p->currentToken.type == TOK_AT_MAIN ||
             p->currentToken.type == TOK_KW_DEF ||
             p->currentToken.type == TOK_KW_EXTENSION ||
             p->currentToken.type == TOK_EOF ||
             p->currentToken.type == TOK_DEDENT) {
        return NULL;
    } else {
        printError("Unexpected token after if block, expected 'else' or end of "
                   "statement.");
        p->error = 2;
        return NULL;
    }
}

static AstNode *parseElse(ParsedToken *p) {
    switch (p->currentToken.type) {
    case TOK_NL: // 26: ⟨else⟩ → ⟨block⟩
    {
        AstNode *block = parseBlock(p); // block is a pointer to the first
                                        // statement of the list of statements
        if (p->error != 0)
        {
            return NULL;
        }
        return block;
    }

    case TOK_KW_IF: // 27: ⟨else⟩ → if ⟨expression⟩ then ⟨block⟩⟨else_choice⟩
    {
        checkToken(p, TOK_KW_IF);
        if (p->error != 0)
        {
            return NULL;
        }
        AstNode *condition = parseExpression(p);
        if (p->error != 0)
        {
            return NULL;
        }
        checkToken(p, TOK_KW_THEN);
        if (p->error != 0)
        {
            return NULL;
        }
        AstNode *thenBody = parseBlock(p);
        if (p->error != 0)
        {
            return NULL;
        }
        AstNode *elseBody = parseElseChoice(p);
        if (p->error != 0)
        {
            return NULL;
        }
        return createNodeIf(condition, thenBody, elseBody);
    }

    default:
        printError("Unexpected token");
        p->error = 2;
        return NULL;
    }
}

static DataType parseRetType(ParsedToken *p) {
    switch (p->currentToken.type) {
    case TOK_COLON: // 28: ⟨ret_type⟩ →: ⟨type⟩
        checkToken(p, TOK_COLON);
        if (p->error != 0)
        {
            return TYPE_NONE;
        }
        return parseType(p);

    case TOK_ASSIGN: // 29: ⟨ret_type⟩ → epsilon
        return TYPE_NONE;

    default:
        printError("Unexpected token");
        p->error = 2;
        return TYPE_NONE;
    }
}

static DataType parseType(ParsedToken *p) {
    switch (p->currentToken.type) {
    case TOK_KW_INT: // 30: ⟨type⟩ → Int
        checkToken(p, TOK_KW_INT);
        if (p->error != 0)
        {
            return TYPE_NONE;
        }
        return TYPE_INT;
    case TOK_KW_DOUBLE: // 31: ⟨type⟩ → Double
        checkToken(p, TOK_KW_DOUBLE);
        if (p->error != 0)
        {
            return TYPE_NONE;
        }
        return TYPE_DOUBLE;
    case TOK_KW_STRING: // 32: ⟨type⟩ → String
        checkToken(p, TOK_KW_STRING);
        if (p->error != 0)
        {
            return TYPE_NONE;
        }
        return TYPE_STRING;
    case TOK_KW_UNIT: // 33: ⟨type⟩ → Unit
        checkToken(p, TOK_KW_UNIT);
        if (p->error != 0)
        {
            return TYPE_NONE;
        }
        return TYPE_UNIT;
    case TOK_KW_BOOLEAN: // 34: ⟨type⟩ → Boolean
        checkToken(p, TOK_KW_BOOLEAN);
        if (p->error != 0)
        {
            return TYPE_NONE;
        }
        return TYPE_BOOL;
    default:
        printError("Unexpected token");
        p->error = 2;
        return TYPE_NONE;
    }
}

static AstNode *parseReturn(ParsedToken *p) {
    // 35: ⟨return⟩ → ⟨expression⟩
    if (startsExpression(p->currentToken.type)) {
        return parseExpression(p);
    }
    // 36: ⟨return⟩ → epsilon
    else if (p->currentToken.type == TOK_NL ||
             p->currentToken.type == TOK_SEMICOL) {
        return NULL;
    } else {
        printError("Unexpected token");
        p->error = 2;
        return NULL;
    }
}

static AstNode *parseParamList(ParsedToken *p, size_t *count) {
    switch (p->currentToken.type) {
    case TOK_ID: // 37: ⟨param_list⟩ → id : ⟨type⟩⟨param_end⟩
    {
        AstNode *param = createNodeParam(p->currentToken.str, TYPE_NONE);
        (*count)++;
        checkToken(p, TOK_ID);
        if (p->error != 0)
        {
            astFree(param);
            return NULL;
        }
        checkToken(p, TOK_COLON);
        if (p->error != 0)
        {
            astFree(param);
            return NULL;
        }
        param->type = parseType(p);
        if (p->error != 0)
        {
            astFree(param);
            return NULL;
        }
        param->next = parseParamEnd(p, count);
        if (p->error != 0)
        {
            astFree(param);
            return NULL;
        }
        return param;
    }

    case TOK_RPAR: // 38: ⟨param_list⟩ → epsilon
        return NULL;
    default:
        printError("Unexpected token");
        p->error = 2;
        return NULL;
    }
}

static AstNode *parseParamEnd(ParsedToken *p, size_t *count) {
    switch (p->currentToken.type) {
    case TOK_COMMA: // 39: ⟨param_end⟩ →,⟨param_list⟩
        checkToken(p, TOK_COMMA);
        if (p->error != 0)
        {
            return TYPE_NONE;
        }
        return parseParamList(p, count);
    case TOK_RPAR: // 40: ⟨param_end⟩ → epsilon
        return TYPE_NONE;
    default:
        printError("Unexpected token");
        p->error = 2;
        return TYPE_NONE;
    }
}

static AstNode *parseLop(ParsedToken *p, size_t *groupCount) {
    switch (p->currentToken.type) {
    case TOK_LPAR: // 41: ⟨lop⟩ → (⟨param_list⟩)⟨lop⟩
    {
        checkToken(p, TOK_LPAR);
        if (p->error != 0)
        {
            return NULL;
        }
        size_t count = 0;
        AstNode *params = parseParamList(p, &count);
        if (p->error != 0)
        {
            return TYPE_NONE;
        }
        checkToken(p, TOK_RPAR);
        if (p->error != 0)
        {
            return NULL;
        }

        AstNode *group = createNodeGroup(params, count);
        (*groupCount)++;

        group->next = parseLop(p, groupCount);
        if (p->error != 0)
        {
            astFree(group);
            return TYPE_NONE;
        }
        return group;
    }
    case TOK_COLON: // 42: ⟨lop⟩ → epsilon
    case TOK_ASSIGN:
        return TYPE_NONE;
    default:
        printError("Unexpected token");
        p->error = 2;
        return TYPE_NONE;
    }
}

static AstNode *parseId(ParsedToken *p) {
    getTokenAhead(p);
    // 50: ⟨assign⟩ → id = ⟨expression⟩
    if (p->aheadToken.type == TOK_ASSIGN) {
        AstNode *destination = createNodeID(p->currentToken.str);
        checkToken(p, TOK_ID);
        if (p->error != 0)
        {
            astFree(destination);
            return NULL;
        }
        checkToken(p, TOK_ASSIGN);
        if (p->error != 0)
        {
            astFree(destination);
            return NULL;
        }

        AstNode *expression = parseExpression(p);
        if (p->error != 0)
        {
            astFree(destination);
            return NULL;
        }
        return createNodeAssign(destination, expression);
    } else {
        return parseExpression(p);
    }
}
/*** End of file parser.c ***/