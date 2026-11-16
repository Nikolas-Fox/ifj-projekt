/******************************************************************************
* Project:		IFJ
* Package:		codegen
* File:			codegen.c
* Date:			15.11.2026
* Last change:	16.11.2026
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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

//generate functions protypes
static void generateWhole(AstNode *node);
static void generateFunc(AstNode *node, CurrentFunc *cFunc);
static void generateBody(AstNode *node, CurrentFunc *cFunc);
static void generateAssign(AstNode *node, CurrentFunc *cFunc);
static void generateDeclaration(AstNode *node, CurrentFunc *cFunc);
static void generateIf(AstNode *node, CurrentFunc *cFunc);
static void generateWhile(AstNode *node, CurrentFunc *cFunc);
static void generateReturn(AstNode *node, CurrentFunc *cFunc);
static void generateCall(AstNode *node, CurrentFunc *cFunc);
static void generateBinary(AstNode *node, CurrentFunc *cFunc);
static void generateUnary(AstNode *node, CurrentFunc *cFunc);
static void generateLiteral(AstNode *node, CurrentFunc *cFunc);
static void generateId(AstNode *node, CurrentFunc *cFunc);
static void generateExpression(AstNode *node, CurrentFunc *cFunc);
static void generateBlock(AstNode *node, CurrentFunc *cFunc);

static void generateAnd(CurrentFunc *cFunc);
static void generateOr(CurrentFunc *cFunc);

static AstNode *findMain(AstNode *node);
static void defvarLocalVariables(AstNode *node);

static void cycleGroups(AstNode *node);
static void cycleParams(AstNode *node);

void generateProgram(AstNode *node)
{
    printf(".IFJcode26\n");
    AstNode *mainFunc = findMain(node->data.whole.func);
    printf("DEFVAR GF@%%tmp\n");
    printf("CALL %s\n", mainFunc->data.func.name);
    printf("POPS GF@%%tmp\n");
    printf("EXIT int@0\n");
    generateWhole(node);

    //TODO: vestavene funkce
}

/**
 * @brief Goes through the program, which is a linked list of functions, and generates for each one.
 * @param node Whole program - root of the AST
 */
static void generateWhole(AstNode *node)
{
    CurrentFunc cFunc;
    AstNode *func = node->data.whole.func;
    while (func != NULL)
    {
        generateFunc(func, &cFunc);
        func = func->next;
    }  
}

/**
 * @brief Generates function.
 * 
 * Updates current function information.
 * Declares all local variables used in the function and all parameters, then pops arguments from stack last to first.
 * Goes through the body, which is a linked list of statements, and generates for each one.
 * Returns the return value on stack.
 * 
 * @param node AST node of the function
 * @param cFunc Current function information
 */
static void generateFunc(AstNode *node, CurrentFunc *cFunc)
{
    cFunc->funcName = node->data.func.name;
    cFunc->evalCount = 0;
    cFunc->ifCount = 0;
    cFunc->whileCount = 0;

    printf("LABEL %s\n", node->data.func.name);
    printf("CREATEFRAME\n");
    printf("PUSHFRAME\n");

    defvarLocalVariables(node->data.func.body);

    //DEFVAR params, POP values from last to first
    AstNode *group = node->data.func.group;
    cycleGroups(group);

    AstNode *body = node->data.func.body;
    while (body != NULL)
    {
        generateBody(body, cFunc);
        body = body->next; //body is a linked list of statements
    }
    printf("LABEL %s$end\n", node->data.func.name);
    if (node->type == TYPE_UNIT) //return type Unit, return value is evaluated and thrown out
    {
        printf("POPS GF@%%tmp\n");
        printf("PUSHS nil@nil\n");
    }
    //return value is on stack
    printf("POPFRAME\n");
    printf("RETURN\n");
}

/**
 * @brief Recursively goes through all groups of parameters.
 * @param node group
 */
static void cycleGroups(AstNode *node)
{
    if (node == NULL)
    {
        return;
    }
    cycleGroups(node->next);
    cycleParams(node->data.group.items);
}

/**
 * @brief Recursively goes through all parameters.
 * 
 * Declares parameters and pops their value from stack from last to first.
 * (Arguments are pushed onto stack first to last.)
 * 
 * @param node param
 */
static void cycleParams(AstNode *node)
{
    if (node == NULL)
    {
        return;
    }
    cycleParams(node->next);

    printf("DEFVAR LF@%s$%d\n", node->data.param.name, node->data.param.scope);
    printf("POPS LF@%s$%d\n", node->data.param.name, node->data.param.scope);
}

/**
 * @brief Generates body, which is a linked list of statements
 * 
 * Generates based on the class of node.
 * If the statement is the last in the function body and it has not returned explicitly, it leaves the return value on stack.
 * 
 * @param node 
 * @param cFunc Current function information
 */
static void generateBody(AstNode *node, CurrentFunc *cFunc)
{
    switch (node->class)
    {
        case AST_ASSIGN:
            generateAssign(node, cFunc);
            //body ending with assignment returns ()
            if (node->next == NULL) //last statement in function body
            {
                printf("PUSHS nil@nil\n");
            }
            break;
        case AST_DECLARATION:
            generateDeclaration(node, cFunc);
            //body ending with declaration returns ()
            if (node->next == NULL) //last statement in function body
            {
                printf("PUSHS nil@nil\n");
            }
            break;
        case AST_IF:
            generateIf(node, cFunc);
            //body ending with a cycle returns ()
            if (node->next == NULL) //last statement in function body
            {
                printf("PUSHS nil@nil\n");
            }
            break;
        case AST_WHILE:
            generateWhile(node, cFunc);
            //body ending with a cycle returns ()
            if (node->next == NULL) //last statement in function body
            {
                printf("PUSHS nil@nil\n");
            }
            break;
        case AST_RETURN:
            generateReturn(node, cFunc);
            break;
        case AST_CALL:
        case AST_BINARY:
        case AST_UNARY:
        case AST_ID:
        case AST_LITERAL:
            generateExpression(node, cFunc);
            if (node->next == NULL) //last statement in function body
            {
                //keep the return value on stack
            }
            else
            {
                printf("POPS GF@%%tmp\n"); //get rid of the return value
            }
    }
}

/**
 * @brief Generates code for assignment.
 * 
 * Generates right-hand side expression which is left on stack and pops it into the destination variable.
 * 
 * @param node AST_ASSIGN
 * @param cFunc Current function information
 */
static void generateAssign(AstNode *node, CurrentFunc *cFunc)
{
    generateExpression(node->data.assign.expression, cFunc);
    printf("POPS LF@%s$%d\n", node->data.assign.destination->data.id.name, node->data.assign.destination->data.id.scope);
}

/**
 * @brief Generates code for declaration.
 * 
 * Generates right-hand side expression which is left on stack and pops it into the at the start of the function declared variable.
 * 
 * @param node AST_DECLARATION
 * @param cFunc Current function information
 */
static void generateDeclaration(AstNode *node, CurrentFunc *cFunc)
{
    //DEFVAR is done at the beginning of the function
    generateExpression(node->data.declaration.expression, cFunc);
    printf("POPS LF@%s$%d\n", node->data.declaration.name, node->data.declaration.scope);
}

/**
 * @brief Generates code for an if statement.
 * 
 * Prints custom start, else and end labels.
 * Evaluates the condition.
 * Generates code for the then and else blocks, which are linked lists of statements.
 * 
 * @param node AST_IF
 * @param cFunc Current function information - this function uses name and count of if cycles for custom labels
 */
static void generateIf(AstNode *node, CurrentFunc *cFunc)
{
    int currentIf = cFunc->ifCount;
    cFunc->ifCount++;

    printf("LABEL %s$if$%d$start\n", cFunc->funcName, currentIf);
    //condition evaluation, leaves result on stack
    generateExpression(node->data.ifStat.condition, cFunc);
    //pops result into tmp
    printf("POPS GF@%%tmp\n");
    printf("JUMPIFEQ %s$if$%d$else GF@%%tmp bool@false\n", cFunc->funcName, currentIf);

    AstNode *block = node->data.ifStat.thenBody;
    while (block != NULL)
    {
        generateBlock(block, cFunc);
        block = block->next;
    }

    printf("JUMP %s$if$%d$end\n", cFunc->funcName, currentIf);
    printf("LABEL %s$if$%d$else\n", cFunc->funcName, currentIf); //if there is no else branch, it will fall through to end

    block = node->data.ifStat.elseBody;
    while (block != NULL)
    {
        generateBlock(block, cFunc);
        block = block->next;
    }
    
    printf("LABEL %s$if$%d$end\n", cFunc->funcName, currentIf);
}

/**
 * @brief Generates code for a while statement.
 * 
 * Prints custom start and end labels.
 * Evaluates the condition.
 * Generates code for the while block, which is a linked list of statements.
 * 
 * @param node AST_WHILE
 * @param cFunc Current function information - this function uses name and count of while cycles for custom labels
 */
static void generateWhile(AstNode *node, CurrentFunc *cFunc)
{
    int currentWhile = cFunc->whileCount;
    cFunc->whileCount++;

    printf("LABEL %s$while$%d$start\n", cFunc->funcName, currentWhile);
    //condition evaluation, leaves result on stack
    generateExpression(node->data.whileStat.condition, cFunc);
    //pops result into tmp
    printf("POPS GF@%%tmp\n");
    printf("JUMPIFEQ %s$while$%d$end GF@%%tmp bool@false\n", cFunc->funcName, currentWhile);

    AstNode *block = node->data.whileStat.body;
    while (block != NULL)
    {
        generateBlock(block, cFunc);
        block = block->next;
    }

    printf("JUMP %s$while$%d$start\n", cFunc->funcName, currentWhile);
    printf("LABEL %s$while$%d$end\n", cFunc->funcName, currentWhile);
}

/**
 * @brief Generates code for a return statement.
 * 
 * Pushes the result value onto stack and jumps to the end of the function.
 * 
 * @param node AST_RETURN
 * @param cFunc Current function information - this function uses name for a label to jump to over dead code
 */
static void generateReturn(AstNode *node, CurrentFunc *cFunc)
{
    if (node->data.returnStat.expression == NULL) //standalone return returns ()
    {
        printf("PUSHS nil@nil\n");
    }
    else
    {
        generateExpression(node->data.returnStat.expression, cFunc); //leaves result on stack
    }
    printf("JUMP %s$end\n", cFunc->funcName); //the rest of the function is dead code
}

/**
 * @brief Generates code for a function call.
 * 
 * Pushes arguments onto stack left to right and prints CALL.
 * 
 * @param node AST_CALL
 * @param cFunc Current function information
 */
static void generateCall(AstNode *node, CurrentFunc *cFunc)
{
    //TODO: vestavene funkce
    //push arguments on stack
    AstNode *group = node->data.call.group;
    while (group != NULL)
    {
        AstNode *item = group->data.group.items;
        while (item != NULL)
        {
            generateExpression(item, cFunc);
            item = item->next;
        }
        group = group->next;
    }

    printf("CALL %s\n", node->data.call.name);
}

/**
 * @brief Generates code for a binary operator.
 * 
 * Performs implicit conversion Int->Double if needed.
 * Evaluates both expressions, results are left on stack.
 * Generates based on the operator.
 * 
 * @param node AST_BINARY
 * @param cFunc Current function information
 */
static void generateBinary(AstNode *node, CurrentFunc *cFunc)
{
    //implicit conversion Int->Double
    bool isDouble = false;
    if (node->data.binary.left->type == TYPE_DOUBLE || node->data.binary.right->type == TYPE_DOUBLE)
    {
        isDouble = true;
    }

    generateExpression(node->data.binary.left, cFunc);
    if (node->data.binary.left->type == TYPE_INT && isDouble)
    {
        printf("INT2FLOATS\n");
    }

    generateExpression(node->data.binary.right, cFunc);
    if (node->data.binary.right->type == TYPE_INT && isDouble)
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
            generateAnd(cFunc);
            break;
        case TOK_OR:
            generateOr(cFunc);
            break;
    }
}

/**
 * @brief Generates code for a unary operator.
 * 
 * Evaluates the expression, result is left on stack.
 * Generates based on the operator.
 * 
 * @param node AST_UNARY
 * @param cFunc Current function information
 */
static void generateUnary(AstNode *node, CurrentFunc *cFunc)
{
    //TODO: conversion
    generateExpression(node->data.unary.operand, cFunc);
    switch (node->data.unary.op)
    {
        case TOK_NOT:
            printf("NOTS\n");
            break;
        case TOK_PLUS:
            break;
        case TOK_MINUS:
            printf("POPS GF@%%tmp\n"); //pops x into temporary
            printf("PUSHS int@0\n");
            printf("PUSHS GF@%%tmp\n");
            printf("SUBS\n"); //0-x => -x
            break;
    }
}

/**
 * @brief Generates code for a literal based on its type.
 * 
 * @param node AST_LITERAL
 * @param cFunc Current function information
 */
static void generateLiteral(AstNode *node, CurrentFunc *cFunc)
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

/**
 * @brief Generates code for an ID.
 * 
 * @param node AST_ID
 * @param cFunc Current function information
 */
static void generateId(AstNode *node, CurrentFunc *cFunc)
{
    printf("PUSHS LF@%s$%d\n", node->data.id.name, node->data.id.scope);
}

/**
 * @brief Evaluates an expression
 * 
 * Calls a generate function based on the class of the node.
 * Leaves the evaluation result on stack.
 * 
 * @param node Evaluated node
 * @param cFunc Current function information
 */
static void generateExpression(AstNode *node, CurrentFunc *cFunc)
{
    switch (node->class)
    {
        case AST_CALL:
            generateCall(node, cFunc);
            break;
        case AST_BINARY:
            generateBinary(node, cFunc);
            break;
        case AST_UNARY:
            generateUnary(node, cFunc);
            break;
        case AST_ID:
            generateId(node, cFunc);
            break;
        case AST_LITERAL:
            generateLiteral(node, cFunc);
            break;
    }
}

/**
 * @brief Generates code for statements in a block.
 * 
 * Calls a generate function based on the class of the node, pops the return value from stack.
 * 
 * @param node Evaluated node
 * @param cFunc Current function information
 */
static void generateBlock(AstNode *node, CurrentFunc *cFunc)
{
    switch (node->class)
    {
        case AST_ASSIGN:
            generateAssign(node, cFunc);
            break;
        case AST_DECLARATION:
            generateDeclaration(node, cFunc);
            break;
        case AST_IF:
            generateIf(node, cFunc);
            break;
        case AST_WHILE:
            generateWhile(node, cFunc);
            break;
        case AST_RETURN:
            generateReturn(node, cFunc);
            break;
        case AST_CALL:
        case AST_BINARY:
        case AST_UNARY:
        case AST_ID:
        case AST_LITERAL:
            generateExpression(node, cFunc);
            printf("POPS GF@%%tmp\n"); //get rid of the return value
            break;
    }
}

/**
 * @brief Generates && short-circuit evaluation.
 * 
 * Evaluates x first, if x is false, y is not evaluated and the result is false.
 * If x is true, the result is undecided and y has to be evaluated.
 * 
 * @param cFunc Current function information - this function uses name and count of evaluations for custom labels
 */
static void generateAnd(CurrentFunc *cFunc)
{
    printf("DEFVAR LF@%%eval1$%d\n", cFunc->evalCount);
    printf("DEFVAR LF@%%eval2$%d\n", cFunc->evalCount);
    printf("POPS LF@%%eval2$%d\n", cFunc->evalCount); //right
    printf("POPS LF@%%eval1$%d\n", cFunc->evalCount); //left

    //jump if x is false
    printf("JUMPIFEQ %s$%d$$eval$decided LF@%%eval1$%d bool@false\n", cFunc->funcName, cFunc->evalCount, cFunc->evalCount);

    //x is not false -> undecided
    printf("PUSHS LF@%%eval1$%d\n", cFunc->evalCount);
    printf("PUSHS LF@%%eval2$%d\n", cFunc->evalCount);
    printf("ANDS\n");
    printf("JUMP %s$%d$$eval$end\n", cFunc->funcName, cFunc->evalCount);

    //x is false -> false
    printf("LABEL %s$%d$$eval$decided\n", cFunc->funcName, cFunc->evalCount);
    printf("PUSHS bool@false\n");

    printf("LABEL %s$%d$$eval$end\n", cFunc->funcName, cFunc->evalCount);
}

/**
 * @brief Generates || short-circuit evaluation.
 * 
 * Evaluates x first, if x is true, y is not evaluated and the result is true.
 * If x is false, the result is undecided and y has to be evaluated.
 * 
 * @param cFunc Current function information - this function uses name and count of evaluations for custom labels
 */
static void generateOr(CurrentFunc *cFunc)
{
    printf("DEFVAR LF@%%eval1$%d\n", cFunc->evalCount);
    printf("DEFVAR LF@%%eval2$%d\n", cFunc->evalCount);
    printf("POPS LF@%%eval2$%d\n", cFunc->evalCount); //right
    printf("POPS LF@%%eval1$%d\n", cFunc->evalCount); //left

    //jump if x is true
    printf("JUMPIFEQ %s$%d$$eval$decided LF@%%eval1$%d bool@true\n", cFunc->funcName, cFunc->evalCount, cFunc->evalCount);

    //x is not true -> undecided
    printf("PUSHS LF@%%eval1$%d\n", cFunc->evalCount);
    printf("PUSHS LF@%%eval2$%d\n", cFunc->evalCount);
    printf("ORS\n");
    printf("JUMP %s$%d$$eval$end\n", cFunc->funcName, cFunc->evalCount);

    //x is true -> true
    printf("LABEL %s$%d$$eval$decided\n", cFunc->funcName, cFunc->evalCount);
    printf("PUSHS bool@true\n");

    printf("LABEL %s$%d$$eval$end\n", cFunc->funcName, cFunc->evalCount);
}

/**
 * @brief Goes through the linked list of functions of the program and finds main.
 * 
 * @param node Starting point
 * @return Main function
 */
static AstNode *findMain(AstNode *node)
{
    while (node != NULL)
    {
        if (node->data.func.isMain)
        {
            return node;
        }
        node = node->next;
    }
    return NULL;
}

/**
 * @brief Goes through the function subtree and declares all local variables.
 * 
 * Called recursively on blocks in if and while statements.
 * 
 * @param node Starting point
 */
static void defvarLocalVariables(AstNode *node)
{
    while (node != NULL)
    {
        switch (node->class)
        {
            case AST_DECLARATION:
                printf("DEFVAR LF@%s$%d\n", node->data.declaration.name, node->data.declaration.scope);
                break;
            case AST_IF:
                findLocalVariables(node->data.ifStat.thenBody);
                findLocalVariables(node->data.ifStat.elseBody);
                break;
            case AST_WHILE:
                findLocalVariables(node->data.whileStat.body);
                break;
            default:
                break;
        }
        node = node->next;
    }
}
/*** End of file codegen.c ***/
