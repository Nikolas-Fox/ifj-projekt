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

// TEMP from symtable.c (add this function to utils)
static char *copyString(const char *src)
{

}


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

AstNode *createNodeLiteral(DataType type, char* value)
{
    AstNode *node = createNode(AST_LITERAL);
    node->type = type;
    node->data.value = copyString(value);
    return node;
}

AstNode *createNodeID(char* name)
{
    AstNode *node = createNode(AST_ID);
    node->data.id.name = copyString(name);
    node->data.id.scope = -1; // this can't be set yet
    return node;
}



/*** End of file ast.c ***/