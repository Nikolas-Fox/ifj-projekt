/******************************************************************************
* Project:		IFJ
* Package:		parser
* File:			parser.h
* Date:			1.11.2026
* Last change:	2.11.2026
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
    TOK_EOF, TOK_EOL, TOK_INDENT, TOK_DEDENT,
    TOK_IDENT, TOK_KEYWORD,
    TOK_INT, TOK_DOUBLE, TOK_STRING,

    //tokens for operators
    TOK_PLUS, TOK_MINUS, TOK_MUL, TOK_DIV,
    TOK_LESS, TOK_LESS_EQ, TOK_GRATER, TOK_GRATER_EQ,
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

/**
 * @brief Prints error message and exits with code 2 (syntax analysis error).
 * @param message Error message to print.
 */
void printError(char *message);

/**
 * @brief Gets token from lexer.
 * @return Token
 */
Token getToken();

/**
 * @brief Checks if token has the expected type and calls getToken() for the next one / calls printError if it doesn't.
 * @param expected Expected token type
 */
void checkToken(TokenType expected);

/**
 * @brief Parses <prog>, implements LL grammar rule 1.
 */
void parseProg();

/**
 * @brief Parses <definition>, implements LL grammar rules 2-5.
 */
void parseDefinition();

/**
 * @brief Parses <def_body>, implements LL grammar rules 6-8.
 */
void parseDefBody();

/**
 * @brief Parses <def_block>, implements LL grammar rule 8.
 */
void parseDefBlock();

/**
 * @brief Parses <extension_def>, implements LL grammar rules 9 and 10.
 */
void parseExtensionDef();

/**
 * @brief Parses <opt_NL>, implements LL grammar rules 11 and 12.
 */
void parseOptNL();

/**
 * @brief Parses <block>, implements LL grammar rule 13.
 */
void parseBlock();

/**
 * @brief Parses <oneline>, implements LL grammar rules 14 and 15.
 */
void parseOneline();

/**
 * @brief Parses <oneline_end>, implements LL grammar rules 16 and 17.
 */
void parseOnelineEnd();

/**
 * @brief Parses <st_list>, implements LL grammar rules 18-20.
 */
void parseStList();

/**
 * @brief Parses <st_list_semi>, implements LL grammar rules 21-23.
 */
void parseStListSemi();

/**
 * @brief Parses <st_list_NL>, implements LL grammar rules 24, 25.
 */
void parseStListNL();

/**
 * @brief Parses <stat>, implements LL grammar rules 26-31.
 */
void parseStat();

/**
 * @brief Parses <else_choice>, implements LL grammar rules 32 and 33.
 */
void parseElseChoice();

/**
 * @brief Parses <else>, implements LL grammar rules 34 and 35.
 */
void parseElse();

/**
 * @brief Parses <ret_type>, implements LL grammar rules 36 and 37.
 */
void parseRetType();

/**
 * @brief Parses <type>, implements LL grammar rules 38-42.
 */
void parseType();

/**
 * @brief Parses <return>, implements LL grammar rules 43 and 44.
 */
void parseReturn();

/**
 * @brief Parses <param_list>, implements LL grammar rules 45 and 46.
 */
void parseParamList();

/**
 * @brief Parses <param_end>, implements LL grammar rules 47 and 48.
 */
void parseParamEnd();

/**
 * @brief Parses <lop>, implements LL grammar rules 49 and 50.
 */
void parseLop();

/**
 * @brief Parses <arg_list>, implements LL grammar rules 51 and 52.
 */
void parseArgList();

/**
 * @brief Parses <arg_end>, implements LL grammar rules 53 and 54.
 */
void parseArgEnd();

/**
 * @brief Precedence analysis
 */
void parseExpression();

#endif
/*** End of file parser.h ***/