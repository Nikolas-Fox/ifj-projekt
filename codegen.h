/******************************************************************************
 * Project:		IFJ
 * Package:		codegen
 * File:			codegen.h
 * Date:			15.11.2026
 * Last change:	18.11.2026
 * Authors: 		Eliška Holá xholael00
 *
 * Description:  Header file of code generation
 *
 *******************************************************************************/
/**
 * @file codegen.h
 *
 * @brief Header file of code generation
 * @author Eliška Holá xholael00
 */

#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"

/**
 * @brief Generates program - calls main and then calls generateWhole
 * @param node Whole program - root of the AST
 */
void generateProgram(AstNode *node);

#endif
/*** End of file codegen.h ***/
