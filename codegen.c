/******************************************************************************
* Project:		IFJ
* Package:		codegen
* File:			codegen.c
* Date:			15.11.2026
* Last change:	15.11.2026
* Authors: 		Eliška Holá xholael00
*
* Description:  Implementation of code generation
*
*******************************************************************************/
/**
* @file codegen.c
*
* @brief Implementation of code generation
* @author Eliška Holá xholael00
*/

#include "codegen.h"

//generate functions protypes
static void generateWhole(AstNode *node);
static void generateFunc(AstNode *node);
static void generateGroup(AstNode *node);
static void generateParam(AstNode *node);
static void generateAssign(AstNode *node);
static void generateDeclaration(AstNode *node);
static void generateIf(AstNode *node);
static void generateWhile(AstNode *node);
static void generateReturn(AstNode *node);
static void generateCall(AstNode *node);
static void generateBinary(AstNode *node);
static void generateUnary(AstNode *node);
static void generateLiteral(AstNode *node);
static void generateId(AstNode *node);

void generateProgram(AstNode *node)
{
    printf(".IFJcode26\n");
    //call main
    generateWhole(node);
}

static void generateWhole(AstNode *node)
{
    AstNode *func = node->data.whole.func;
    while (func != NULL)
    {
        generateFunc(func);
        func = func->next; //the whole program is a linked list of functions
    }  
}

static void generateFunc(AstNode *node)
{
    printf("LABEL %s\n", node->data.func.name);
    printf("CREATEFRAME\n");
    printf("PUSHFRAME\n");
    //DEFVAR parametru
    //DEFVAR lokalnich promennych
    //hodnoty parametru v opacnem poradi
    AstNode *body = node->data.func.body;
    while (body != NULL)
    {
        generateBody(body);
        body = body->next; //body is a linked list of statements
    }
    //return
    printf("POPFRAME\n");
    printf("RETURN\n");
}

//TODO: funcName, ifCount, whileCount

static void generateBody(AstNode *node)
{
    switch (node->class)
    {
        case AST_ASSIGN:
            generateAssign(node);
            break;
        case AST_DECLARATION:
            generateDeclaration(node);
            break;
        case AST_IF:
            generateIf(node);
            break;
        case AST_WHILE:
            generateWhile(node);
            break;
        case AST_RETURN:
            generateReturn(node);
            break;
        case AST_CALL:
            generateCall(node);
            break;
        case AST_BINARY:
            generateBinary(node);
            break;
        case AST_UNARY:
            generateUnary(node);
            break;
        case AST_ID:
            generateId(node);
            break;
    }
}

/*** End of file codegen.c ***/
