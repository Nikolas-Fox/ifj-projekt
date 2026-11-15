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

#include <stdlib.h>

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
static void generateExpression(AstNode *node);

//temporary
char *funcName;
int ifCount, whileCount;

void generateProgram(AstNode *node)
{
    printf(".IFJcode26\n");
    //call main
    generateWhole(node);
    //vestavene funkce
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
    if (node == NULL)
    {
        return;
    }

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
        case AST_BINARY:
        case AST_UNARY:
        case AST_ID:
            generateExpression(node);
            if (node->next == NULL)
            {
                //TODO uklidit hodnotu ze zasobniku
            }
            else
            {
                //TODO navratova hodnota
            }
    }
}

static void generateAssign(AstNode *node)
{

}

static void generateDeclaration(AstNode *node)
{
    printf("DEFVAR LF@%s$%d\n", node->data.declaration.name, node->data.declaration.scope);
}

static void generateIf(AstNode *node)
{
    ifCount++;
    printf("LABEL %s$if$%d$start\n", funcName, ifCount);
    //TODO vyhodnoceni podminky
    printf("JUMPIFEQ %s$if$%d$else conditionResult bool@false\n", funcName, ifCount); //TODO conditionResult
    generateBody(node->data.ifStat.thenBody);
    printf("JUMP %s$if$%d$end\n", funcName, ifCount);
    printf("LABEL %s$if$%d$else\n", funcName, ifCount); //if there is no else branch, it will fall through to end
    generateBody(node->data.ifStat.elseBody);
    printf("LABEL %s$if$%d$end\n", funcName, ifCount);
}

static void generateWhile(AstNode *node)
{
    whileCount++;
    printf("LABEL %s$while$%d$start\n", funcName, whileCount);
    //TODO vyhodnoceni podminky
    printf("JUMPIFEQ %s$while$%d$end conditionResult bool@false\n", funcName, whileCount); //TODO conditionResult
    generateBody(node->data.whileStat.body);
    printf("JUMP %s$while$%d$start\n", funcName, whileCount);
    printf("LABEL %s$while$%d$end\n", funcName, whileCount);
}

static void generateReturn(AstNode *node)
{

}

static void generateCall(AstNode *node)
{

}

static void generateBinary(AstNode *node)
{
    //TODO implicitni konverze Int->Double
    generateBody(node->data.binary.left);
    generateBody(node->data.binary.right);

    switch (node->data.binary.op)
    {
        case TOK_PLUS:
            printf("ADDS\n");
            break;
        case TOK_MINUS:
            printf("SUBS\n");
            break;
        case TOK_MUL:
            printf("MULS\n");
            break;
        case TOK_DIV:
            //TODO
            break;
        case TOK_LESS:
            printf("LTS\n");
            break;
        case TOK_GREATER:
            printf("GTS\n");
            break;
        case TOK_EQ:
            printf("EQS\n");
            break;
        case TOK_NOT:
            printf("NOTS\n");
            break;
        case TOK_LESS_EQ:
            //TODO
            break;
        case TOK_GREATER_EQ:
            //TODO
            break;
        case TOK_NEQ:
            //TODO
            break;
        case TOK_AND:
            //TODO
            break;
        case TOK_OR:
            //TODO
            break;
    }
}

static void generateUnary(AstNode *node)
{

}

static void generateId(AstNode *node)
{
    printf("PUSHS LF@%s$%d\n", node->data.id.name, node->data.id.scope);
}

static void generateExpression(AstNode *node) //leaves the result on stack
{
    switch (node->class)
    {
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
        case AST_LITERAL:
            generateLiteral(node);
            break;
    }
}

static void generateLiteral(AstNode *node)
{
    switch (node->type)
    {
        case TYPE_INT:
            printf("PUSHS int@%s\n",node->data.value);
            break;
        case TYPE_STRING:
            //TODO escape sekvence
        case TYPE_BOOL:
            printf("PUSHS bool@%s\n",node->data.value);
            break;
        case TYPE_DOUBLE:
            double value = strtod(node->data.value, NULL);
            printf("PUSHS float@%a\n", value);
            break;
    }
}

/*** End of file codegen.c ***/
