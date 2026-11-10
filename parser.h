/******************************************************************************
* Project:		IFJ
* Package:		parser
* File:			parser.h
* Date:			1.11.2026
* Last change:	8.11.2026
* Authors: 		xholael00
*
* Description: Header file containing parser logic - recursive descent and
*              precedence analysis logic.
*
*******************************************************************************/
/**
* @file parser.h
*
* @brief Parser header file
* @author xholael00
*/

#ifndef PARSER_H
#define PARSER_H

//temporary
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
    TOK_AND, TOK_OR, TOK_NOT, TOK_PLUSPLUS,

    TOK_KW_DEF, TOK_KW_DO, TOK_KW_DOUBLE, TOK_KW_ELSE, TOK_KW_IF, TOK_KW_IMPORT,
    TOK_KW_INT, TOK_KW_RETURN, TOK_KW_STRING, TOK_KW_THEN, TOK_KW_UNIT, TOK_KW_VAL,
    TOK_KW_VAR, TOK_KW_WHILE, TOK_KW_EXTENSION, TOK_KW_BOOLEAN
}TokenType;

typedef struct {
    TokenType type;
    char *str; //identifikátor/řetězec
    int line;
}Token;

Token getToken();

//Parser state struct
typedef struct {
    Token currentToken;
    Token aheadToken;
    bool lookAhead;
} ParsedToken;

/**
 * @brief Prints error message and exits with code 2 (syntax analysis error).
 * @param message Error message to print.
 */
void printError(char *message);

/**
 * @brief Decides if look ahead was done or if it needs to get a new token from lexer.
 * @param p Struct of parser state
 */
void getNextToken(ParsedToken *p);

/**
 * @brief Checks if token has the expected type and calls getToken() for the next one / calls printError if it doesn't.
 * @param p Struct of parser state
 * @param expected Expected token type
 */
void checkToken(ParsedToken *p, TokenType expected);

/**
 * @brief Checks if token has the expected type and name (used for TOK_ID), calls getToken() for the next one / calls printError if it doesn't.
 * @param p Struct of parser state
 * @param expected Expected token type
 * @param name Name of id
 */
void checkStrToken(ParsedToken *p, TokenType expected, char *name);

/**
 * @brief Gets a new token from lexer in lookahead.
 * @param p Struct of parser state
 */
void getTokenAhead(ParsedToken *p);

/**
 * @brief Checks if token type belongs to Predict(<stat>).
 * @param type Token type
 * @return True if it belongs, false if it doesn't.
 */
bool startsStat(TokenType type);

/**
 * @brief Checks if token type belongs to Predict(<expression>).
 * @param type Token type
 * @return True if it belongs, false if it doesn't.
 */
bool startsExpression(TokenType type);

/**
 * @brief Parses <prog>, implements LL grammar rule 1.
 * @param p Struct of parser state
 */
void parseProg(ParsedToken *p);

/**
 * @brief Parses <definition>, implements LL grammar rules 2-5.
 * @param p Struct of parser state
 */
void parseDefinition(ParsedToken *p);

/**
 * @brief Parses <def_body>, implements LL grammar rules 6-8.
 * @param p Struct of parser state
 */
void parseDefBody(ParsedToken *p);

/**
 * @brief Parses <def_block>, implements LL grammar rule 8.
 * @param p Struct of parser state
 */
void parseDefBlock(ParsedToken *p);

/**
 * @brief Parses <extension_def>, implements LL grammar rules 9 and 10.
 * @param p Struct of parser state
 */
void parseExtensionDef(ParsedToken *p);

/**
 * @brief Parses <opt_NL>, implements LL grammar rules 11 and 12.
 * @param p Struct of parser state
 */
void parseOptNL(ParsedToken *p);

/**
 * @brief Parses <block>, implements LL grammar rule 13.
 * @param p Struct of parser state
 */
void parseBlock(ParsedToken *p);

/**
 * @brief Parses <st_list>, implements LL grammar rules 18-20.
 * @param p Struct of parser state
 */
void parseStList(ParsedToken *p);

/**
 * @brief Parses <stat>, implements LL grammar rules 26-31.
 * @param p Struct of parser state
 */
void parseStat(ParsedToken *p);

/**
 * @brief Parses <line>, implements LL grammar rules 19-22.
 * @param p Struct of parser state
 */
void parseLine(ParsedToken *p);

/**
 * @brief Parses <else_choice>, implements LL grammar rules 32 and 33.
 * @param p Struct of parser state
 */
void parseElseChoice(ParsedToken *p);

/**
 * @brief Parses <else>, implements LL grammar rules 34 and 35.
 * @param p Struct of parser state
 */
void parseElse(ParsedToken *p);

/**
 * @brief Parses <ret_type>, implements LL grammar rules 36 and 37.
 * @param p Struct of parser state
 */
void parseRetType(ParsedToken *p);

/**
 * @brief Parses <type>, implements LL grammar rules 38-42.
 * @param p Struct of parser state
 */
void parseType(ParsedToken *p);

/**
 * @brief Parses <return>, implements LL grammar rules 43 and 44.
 * @param p Struct of parser state
 */
void parseReturn(ParsedToken *p);

/**
 * @brief Parses <param_list>, implements LL grammar rules 45 and 46.
 * @param p Struct of parser state
 */
void parseParamList(ParsedToken *p);

/**
 * @brief Parses <param_end>, implements LL grammar rules 47 and 48.
 * @param p Struct of parser state
 */
void parseParamEnd(ParsedToken *p);

/**
 * @brief Parses <lop>, implements LL grammar rules 49 and 50.
 * @param p Struct of parser state
 */
void parseLop(ParsedToken *p);

/**
 * @brief Parses <arg_list>, implements LL grammar rules 51 and 52.
 * @param p Struct of parser state
 */
void parseArgList(ParsedToken *p);

/**
 * @brief Parses <arg_end>, implements LL grammar rules 53 and 54.
 * @param p Struct of parser state
 */
void parseArgEnd(ParsedToken *p);

/**
 * @brief Precedence analysis
 */
void parseExpression();

#endif
/*** End of file parser.h ***/