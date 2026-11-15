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
#include <stdbool.h>

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
int ifCount, whileCount, evalCount;

void generateProgram(AstNode *node)
{
    printf(".IFJcode26\n");
    //call main
    printf("DEFVAR GF@%%tmp\n");
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
    //condition evaluation, leaves result on stack
    generateExpression(node->data.ifStat.condition);
    //pops result into tmp
    printf("POPS GF@%%tmp\n");
    printf("JUMPIFEQ %s$if$%d$else GF@%%tmp bool@false\n", funcName, ifCount);
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
    //condition evaluation, leaves result on stack
    generateExpression(node->data.whileStat.condition);
    //pops result into tmp
    printf("POPS GF@%%tmp\n");
    printf("JUMPIFEQ %s$while$%d$end GF@%%tmp bool@false\n", funcName, whileCount);
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
    //implicit conversion Int->Double
    bool isDouble = false;
    if (node->data.binary.left->type == TYPE_DOUBLE || node->data.binary.right->type == TYPE_DOUBLE)
    {
        isDouble = true;
    }

    generateExpression(node->data.binary.left);
    if (node->data.binary.left->type == TYPE_INT)
    {
        printf("INT2FLOATS\n");
    }

    generateExpression(node->data.binary.right);
    if (node->data.binary.right->type == TYPE_INT)
    {
        printf("INT2FLOATS\n");
    }

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
            if (isDouble)
            {
                printf("DIVS\n");
            }
            else
            {
                printf("IDIVS\n");
            }
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
            printf("GTS\n");
            printf("NOTS\n");
            break;
        case TOK_GREATER_EQ:
            printf("LTS\n");
            printf("NOTS\n");
            break;
        case TOK_NEQ:
            printf("EQS\n");
            printf("NOTS\n");
            break;
        case TOK_AND:
            generateAnd();
            break;
        case TOK_OR:
            generateOr();
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

//x && y - if x is false, y is not evaluated and result is false
static void generateAnd()
{
    printf("DEFVAR LF@%%eval1$%d\n", evalCount);
    printf("DEFVAR LF@%%eval2$%d\n", evalCount);
    printf("POPS LF@%%eval2$%d\n", evalCount); //right
    printf("POPS LF@%%eval1$%d\n", evalCount); //left

    //jump if x is false
    printf("JUMPIFEQ %s$%d$$eval$decided LF@%%eval1$%d bool@false\n", funcName, evalCount, evalCount);

    //x is not false -> undecided
    printf("PUSHS LF@%%eval1$%d\n", evalCount);
    printf("PUSHS LF@%%eval2$%d\n", evalCount);
    printf("ANDS\n");
    printf("JUMP %s$%d$$eval$end\n", funcName, evalCount);

    //x is false -> false
    printf("LABEL %s$%d$$eval$decided\n", funcName, evalCount);
    printf("PUSHS bool@false\n");

    printf("LABEL %s$%d$$eval$end\n", funcName, evalCount);
}

//x || y - if x is true, y is not evaluated and result is true
static void generateOr()
{
    printf("DEFVAR LF@%%eval1$%d\n", evalCount);
    printf("DEFVAR LF@%%eval2$%d\n", evalCount);
    printf("POPS LF@%%eval2$%d\n", evalCount); //right
    printf("POPS LF@%%eval1$%d\n", evalCount); //left

    //jump if x is true
    printf("JUMPIFEQ %s$%d$$eval$decided LF@%%eval1$%d bool@true\n", funcName, evalCount, evalCount);

    //x is not true -> undecided
    printf("PUSHS LF@%%eval1$%d\n", evalCount);
    printf("PUSHS LF@%%eval2$%d\n", evalCount);
    printf("ORS\n");
    printf("JUMP %s$%d$$eval$end\n", funcName, evalCount);

    //x is true -> true
    printf("LABEL %s$%d$$eval$decided\n", funcName, evalCount);
    printf("PUSHS bool@true\n");

    printf("LABEL %s$%d$$eval$end\n", funcName, evalCount);
}
/*** End of file codegen.c ***/
