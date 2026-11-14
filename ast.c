/******************************************************************************
* Project:		IFJ
* Package:		ast
* File:			ast.c
* Date:			12.11.2026
* Last change:	13.11.2026
* Authors: 		Nikolas Fox xfoxnik00
*
* Description: Implementation of abstract syntactic tree logic
*
*******************************************************************************/
/**
* @file ast.c
*
* @brief  Implementation of abstract syntactic tree logic
* @author Nikolas Fox xfoxnik00
*/

#include "ast.h"

AstNode *createNode(AstClass class)
{
    AstNode *node = calloc(1, sizeof(AstNode));
    if(node == NULL)
    {
        exit(99);
    }

    node->class = class;
    return node;
}


/*** End of file ast.c ***/