/******************************************************************************
* Project:		IFJ
* Package:		ast
* File:			ast.c
* Date:			12.11.2026
* Last change:	15.11.2026
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

AstNode *createNodeUnary(TokenType op, AstNode *operand)
{
    AstNode *node = createNode(AST_UNARY);
    node->data.unary.op = op;
    node->data.unary.operand = operand;
    return node;
}

AstNode *createNodeBinary(TokenType op, AstNode *left, AstNode *right)
{
    AstNode *node = createNode(AST_BINARY);
    node->data.binary.op = op;
    node->data.binary.left = left;
    node->data.binary.right = right;
    return node;
}

AstNode *createNodeCall(char *name, size_t groupCount, AstNode *group)
{
    AstNode *node = createNode(AST_CALL);
    node->data.call.name = copyString(name);
    node->data.call.group = group;
    node->data.call.groupCount = groupCount;
    return node;
}

AstNode *createNodeReturn(AstNode *expression)
{
    AstNode *node = createNode(AST_RETURN);
    node->data.returnStat.expression = expression;
    return node;
}

AstNode *createNodeWhile(AstNode *condition, AstNode *body)
{
    AstNode *node = createNode(AST_WHILE);
    node->data.whileStat.condition = condition;
    node->data.whileStat.body = body;
    return node;
}

AstNode *createNodeIf(AstNode *condition, AstNode *thenBody, AstNode *elseBody)
{
    AstNode *node = createNode(AST_IF);
    node->data.ifStat.condition = condition;
    node->data.ifStat.thenBody = thenBody;
    node->data.ifStat.elseBody = elseBody;
    return node;
}

AstNode *createNodeDeclaration(char *name, bool isVar, bool hasType, DataType declaredType, AstNode *expression)
{
    AstNode *node = createNode(AST_DECLARATION);
    node->data.declaration.scope = -1;
    node->data.declaration.name = copyString(name);
    node->data.declaration.isVar = isVar;
    node->data.declaration.hasType = hasType; // this will be ahead, so I need to keep a copy of the name (or the pointer)
    node->data.declaration.declaredType = declaredType;
    node->data.declaration.expression = expression;
    return node;
}

AstNode *createNodeAssign(AstNode *destination, AstNode *expression)
{
    AstNode *node = createNode(AST_ASSIGN);
    node->data.assign.destination = destination;
    node->data.assign.expression = expression;
    return node;
}

AstNode *createNodeParam(char *name, DataType declaredType)
{
    AstNode *node = createNode(AST_PARAM);
    node->data.param.declaredType = declaredType;
    node->data.param.name = copyString(name);
    node->data.param.scope = -1;
    return node;
}

AstNode *createNodeGroup(AstNode *items, size_t count)
{
    AstNode *node = createNode(AST_GROUP);
    node->data.group.items = items;
    node->data.group.count = count;
    return node;
}

AstNode *createNodeFunc(char *name, size_t groupCount,AstNode *group, AstNode *body, bool hasRetType, DataType retType)
{
    AstNode *node = createNode(AST_FUNC);
    node->data.func.name = copyString(name);
    node->data.func.groupCount = groupCount;
    node->data.func.group = group;
    node->data.func.hasRetType = hasRetType;
    node->data.func.retType = retType;
    node->data.func.body = body;
    return node;
}

// there is no need to define createNodeWhole as it is empty at first

AstNode *insertStat(AstNode *head, AstNode *newStat)
{
    if(newStat == NULL)
    {
        return head;
    }

    if(head == NULL)
    {
        return newStat;
    }

    AstNode *node = head;
    while(node->next != NULL)
    {
        node = node->next;
    }

    node->next = newStat;
    return head;
}

AstNode *insertFunc(AstNode *head, AstNode *newFunc)
{
    if(newFunc == NULL)
    {
        return head;
    }

    if(head == NULL)
    {
        return newFunc;
    }

    AstNode *node = head;
    while(node->next != NULL)
    {
        node = node->next;
    }

    node->next = newFunc;
    return head;
}

AstNode *insertGroup(AstNode *head, AstNode *newGroup, size_t *groupCount)
{
    if(newGroup == NULL)
    {
        return head;
    }

    if(head == NULL)
    {
        return newGroup;
    }

    (*groupCount)++;

    AstNode *node = head;
    while(node->next != NULL)
    {
        node = node->next;
    }

    node->next = newGroup;
    head->data.func.groupCount++;
    return head;
}

AstNode *insertParam(AstNode *head, AstNode *newParam)
{
    if(newParam == NULL)
    {
        return head;
    }

    if(head == NULL)
    {
        return newParam;
    }

    AstNode *node = head;
    while(node->next != NULL)
    {
        node = node->next;
    }

    node->next = newParam;
    head->data.group.count++;
    return head;
}

/*** End of file ast.c ***/