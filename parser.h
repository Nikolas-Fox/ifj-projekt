/******************************************************************************
* Project:		IFJ
* Package:		parser
* File:			parser.h
* Date:			1.11.2026
* Last change:	1.11.2026
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

void printError(char *message);

Token getToken();

void checkToken(TokenType expected);

void parseProg();

void parseDefinition();

void parseDefBody();

void parseDefBlock();

void parseExtensionDef();

void parseOptNL();

void parseBlock();

void parseOneline();

void parseOnelineEnd();

void parseStList();

void parseStListSemi();

void parseStListNL();

void parseStat();

void parseElseChoice();

void parseElse();

void parseRetType();

void parseType();

void parseReturn();

void parseParamList();

void parseParamEnd();

void parseLop();

void parseArgList();

void parseArgEnd();

void parseExpression();

#endif
/*** End of file parser.h ***/