/******************************************************************************
 * Project:		IFJ
 * Package:		precedence
 * File:			precedence.h
 * Date:			5.11.2026
 * Last change:	18.11.2026
 * Authors: 		Nikolas Fox xfoxnik00
 *
 * Description: Header file for precedence syntactic analysis
 *
 *******************************************************************************/
/**
 * @file precedence.h
 *
 * @brief Header file for precedence syntactic analysis
 * @author Nikolas Fox xfoxnik00
 */

#ifndef PRECEDENCE_H
#define PRECEDENCE_H

#include "ast.h"
#include "parser.h"

/**
 * @brief Uses precedence analysis to validate expression and builds the AST
 * expression tree
 * @param p Pointer to the parsed token
 * @return Root AstNode of the parsed expression
 */
AstNode *parseExpression(ParsedToken *p);

#endif // PRECEDENCE_H
       /*** End of file precedence.h ***/