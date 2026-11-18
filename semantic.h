/******************************************************************************
* Project:		IFJ
* Package:		semantic
* File:			semantic.h
* Date:			16.11.2026
* Last change:	18.11.2026
* Authors: 		Nikolas Fox xfoxnik00
*
* Description: Header file for semantic analysis
*
*******************************************************************************/
/**
* @file semantic.h
*
* @brief Header file for semantic analysis
* @author Nikolas Fox xfoxnik00
*/

#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"

/**
 * @brief Decides whether the program is semantically correct
 * @param whole Head of the entire AST
 */
void semanticAnalysis(AstNode *whole);

#endif
/*** End of file semantic. ***/