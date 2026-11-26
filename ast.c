/******************************************************************************
 * Project:      IFJ
 * Package:      AST
 * File:         ast.c
 * Date:         12. 11. 2026
 * Last change:  26. 11. 2026
 * Authors:      Nikolas Fox xfoxnik00
 *
 * Description:  Implementation of abstract syntactic tree logic
 *****************************************************************************/
/**
 * @file ast.c
 *
 * @brief  Implementation of abstract syntactic tree logic
 * @author Nikolas Fox xfoxnik00
 */

#include "ast.h"
#include <stdlib.h>

static void astFreeList(AstNode *node);

AstNode *createNode(AstClass class) {
    AstNode *node = calloc(1, sizeof(AstNode));
    if (node == NULL) {
        exit(99);
    }

    node->class = class;
    return node;
}

AstNode *createNodeLiteral(DataType type, char *value) {
    AstNode *node = createNode(AST_LITERAL);
    node->type = type;
    node->data.value = copyString(value);
    return node;
}

AstNode *createNodeID(char *name) {
    AstNode *node = createNode(AST_ID);
    node->data.id.name = copyString(name);
    node->data.id.scope = -1; // this can't be set yet
    return node;
}

AstNode *createNodeUnary(TokenType op, AstNode *operand) {
    AstNode *node = createNode(AST_UNARY);
    node->data.unary.op = op;
    node->data.unary.operand = operand;
    return node;
}

AstNode *createNodeBinary(TokenType op, AstNode *left, AstNode *right) {
    AstNode *node = createNode(AST_BINARY);
    node->data.binary.op = op;
    node->data.binary.left = left;
    node->data.binary.right = right;
    return node;
}

AstNode *createNodeCall(char *name, size_t groupCount, AstNode *group) {
    AstNode *node = createNode(AST_CALL);
    node->data.call.name = copyString(name);
    node->data.call.group = group;
    node->data.call.groupCount = groupCount;
    return node;
}

AstNode *createNodeReturn(AstNode *expression) {
    AstNode *node = createNode(AST_RETURN);
    node->data.returnStat.expression = expression;
    return node;
}

AstNode *createNodeWhile(AstNode *condition, AstNode *body) {
    AstNode *node = createNode(AST_WHILE);
    node->data.whileStat.condition = condition;
    node->data.whileStat.body = body;
    return node;
}

AstNode *createNodeIf(AstNode *condition, AstNode *thenBody,
                      AstNode *elseBody) {
    AstNode *node = createNode(AST_IF);
    node->data.ifStat.condition = condition;
    node->data.ifStat.thenBody = thenBody;
    node->data.ifStat.elseBody = elseBody;
    return node;
}

AstNode *createNodeDeclaration(char *name, bool isVar, DataType type,
                               AstNode *expression) {
    AstNode *node = createNode(AST_DECLARATION);
    node->data.declaration.scope = -1;
    node->data.declaration.name = copyString(name);
    node->data.declaration.isVar = isVar;
    node->type = type;
    node->data.declaration.expression = expression;
    return node;
}

AstNode *createNodeAssign(AstNode *destination, AstNode *expression) {
    AstNode *node = createNode(AST_ASSIGN);
    node->data.assign.destination = destination;
    node->data.assign.expression = expression;
    return node;
}

AstNode *createNodeParam(char *name, DataType type) {
    AstNode *node = createNode(AST_PARAM);
    node->type = type;
    node->data.param.name = copyString(name);
    node->data.param.scope = -1;
    return node;
}

AstNode *createNodeGroup(AstNode *items, size_t count) {
    AstNode *node = createNode(AST_GROUP);
    node->data.group.items = items;
    node->data.group.count = count;
    return node;
}

AstNode *createNodeFunc(char *name, size_t groupCount, AstNode *group,
                        AstNode *body, DataType retType) {
    AstNode *node = createNode(AST_FUNC);
    node->data.func.name = copyString(name);
    node->data.func.groupCount = groupCount;
    node->data.func.group = group;
    node->type = retType;
    node->data.func.state = NOT_VISITED;
    node->data.func.body = body;
    return node;
}

// there is no need to define createNodeWhole as it is empty at first

/**
 * @brief Inserts statement into a linked list of statements
 * @param head Pointer to the first element of the linked list of statements
 * @param newStat Pointer to node of the statement to be inserted
 * @return Pointer to the head of the list of statements.
 */
AstNode *insertStat(AstNode *head, AstNode *newStat) {
    if (newStat == NULL) {
        return head;
    }

    if (head == NULL) {
        return newStat;
    }

    AstNode *node = head;
    while (node->next != NULL) {
        node = node->next;
    }

    node->next = newStat;
    return head;
}

/**
 * @brief Inserts function into a linked list of functions
 * @param head Pointer to the first element of the linked list of functions
 * @param newFunc Pointer to node of the function to be inserted
 * @return Pointer to the head of the list of functions.
 */
AstNode *insertFunc(AstNode *head, AstNode *newFunc) {
    if (newFunc == NULL) {
        return head;
    }

    if (head == NULL) {
        return newFunc;
    }

    AstNode *node = head;
    while (node->next != NULL) {
        node = node->next;
    }

    node->next = newFunc;
    return head;
}

/**
 * @brief Free a linked list
 * @param head Pointer to the first element of a linked list (head)
 */
static void astFreeList(AstNode *head) {
    while (head != NULL) {
        AstNode *next = head->next;
        astFree(head);
        head = next;
    }
}

/**
 * @brief Free the whole AST tree
 * @param node Pointer to the root of the tree
 */
void astFree(AstNode *node) {
    if (node == NULL) {
        return;
    }
    switch (node->class) {
    case AST_LITERAL:
        free(node->data.value);
        break;

    case AST_ID:
        free(node->data.id.name);
        break;

    case AST_UNARY:
        astFree(node->data.unary.operand);
        break;

    case AST_BINARY:
        astFree(node->data.binary.left);
        astFree(node->data.binary.right);
        break;

    case AST_CALL:
        free(node->data.call.name);
        astFreeList(node->data.call.group);
        break;

    case AST_RETURN:
        astFree(node->data.returnStat.expression);
        break;

    case AST_WHILE:
        astFree(node->data.whileStat.condition);
        astFreeList(node->data.whileStat.body);
        break;

    case AST_IF:
        astFree(node->data.ifStat.condition);
        astFreeList(node->data.ifStat.thenBody);
        astFreeList(node->data.ifStat.elseBody);
        break;

    case AST_DECLARATION:
        free(node->data.declaration.name);
        astFree(node->data.declaration.expression);
        break;

    case AST_ASSIGN:
        astFree(node->data.assign.destination);
        astFree(node->data.assign.expression);
        break;

    case AST_PARAM:
        free(node->data.param.name);
        break;

    case AST_GROUP:
        astFreeList(node->data.group.items);
        break;

    case AST_FUNC:
        free(node->data.func.name);
        astFreeList(node->data.func.group);
        astFreeList(node->data.func.body);
        break;

    case AST_WHOLE:
        astFreeList(node->data.whole.func);
        break;
    }
    free(node);
}
/*** End of file ast.c ***/
