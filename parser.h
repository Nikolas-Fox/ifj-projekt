/******************************************************************************
* Project:		IFJ
* Package:		parser
* File:			parser.h
* Date:			1.11.2026
* Last change:	2.11.2026
* Authors: 		xholael00
*
* Description: 
*
*******************************************************************************/
/**
* @file parser.h
*
* @brief 
* @author xholael00
*/

#ifndef PARSER_H
#define PARSER_H

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