/******************************************************************************
* Project:		IFJ
* Package:		codegen
* File:			codegen.c
* Date:			15.11.2026
* Last change:	17.11.2026
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

static void generateAnd(AstNode *node, CurrentFunc *cFunc);
static void generateOr(AstNode *node, CurrentFunc *cFunc);

static AstNode *findMain(AstNode *node);
static void defvarLocalVariables(AstNode *node);

static void cycleGroups(AstNode *node);
static void cycleParams(AstNode *node);

static void printBuiltins();

void generateProgram(AstNode *node)
{
    printf(".IFJcode26\n");
    AstNode *mainFunc = findMain(node->data.whole.func);
    printf("DEFVAR GF@%%tmp\n");
    printf("DEFVAR GF@%%type\n");
    printf("CALL %s\n", mainFunc->data.func.name);
    printf("POPS GF@%%tmp\n");
    printf("EXIT int@0\n");
    generateWhole(node);
    printBuiltins();
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
    if (node->data.binary.op == TOK_AND)
    {
        generateAnd(node, cFunc);
        return;
    }

    if (node->data.binary.op == TOK_OR)
    {
        generateOr(node, cFunc);
        return;
    }

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
            if (node->type == TYPE_INT)
            {
                printf("PUSHS int@0\n");
            }
            else
            {
                printf("PUSHS float@%a\n", 0.0);
            }
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
            printf("PUSHS string@");
            for (int i = 0; node->data.value[i] != '\0'; i++)
            {
                char c = node->data.value[i];
                if (c <= 32 || c == 35 || c == 92)
                {
                    if (c >= 10)
                    {
                        printf("\\0%d", c);
                    }
                    else
                    {
                        printf("\\00%d", c);
                    }
                }
                else
                {
                    printf("%c", c);
                }
            }
            printf("\n");
            break;
        case TYPE_BOOL:
            printf("PUSHS bool@%s\n",node->data.value);
            break;
        case TYPE_DOUBLE:
            double value = strtod(node->data.value, NULL);
            printf("PUSHS float@%a\n", value);
            break;
        case TYPE_UNIT:
            printf("PUSHS nil@nil\n");
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
static void generateAnd(AstNode *node, CurrentFunc *cFunc)
{
    int currentEval = cFunc->evalCount;
    cFunc->evalCount++;

    generateExpression(node->data.binary.left, cFunc);
    printf("POPS GF@%%tmp\n");

    //jump if x is false
    printf("JUMPIFEQ %s$%d$$eval$decided GF@%%tmp bool@false\n", cFunc->funcName, currentEval);

    //x is not false -> the result is y
    generateExpression(node->data.binary.right, cFunc); //leaves result on stack
    printf("JUMP %s$%d$$eval$end\n", cFunc->funcName, currentEval);

    //x is false -> false
    printf("LABEL %s$%d$$eval$decided\n", cFunc->funcName, currentEval);
    printf("PUSHS bool@false\n");

    printf("LABEL %s$%d$$eval$end\n", cFunc->funcName, currentEval);
}

/**
 * @brief Generates || short-circuit evaluation.
 * 
 * Evaluates x first, if x is true, y is not evaluated and the result is true.
 * If x is false, the result is undecided and y has to be evaluated.
 * 
 * @param cFunc Current function information - this function uses name and count of evaluations for custom labels
 */
static void generateOr(AstNode *node, CurrentFunc *cFunc)
{
    int currentEval = cFunc->evalCount;
    cFunc->evalCount++;

    generateExpression(node->data.binary.left, cFunc);
    printf("POPS GF@%%tmp\n");

    //jump if x is true
    printf("JUMPIFEQ %s$%d$$eval$decided GF@%%tmp bool@true\n", cFunc->funcName, currentEval);

    //x is not true -> the result is y
    generateExpression(node->data.binary.right, cFunc); //leaves result on stack
    printf("JUMP %s$%d$$eval$end\n", cFunc->funcName, currentEval);

    //x is true -> true
    printf("LABEL %s$%d$$eval$decided\n", cFunc->funcName, currentEval);
    printf("PUSHS bool@true\n");

    printf("LABEL %s$%d$$eval$end\n", cFunc->funcName, currentEval);
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
                defvarLocalVariables(node->data.ifStat.thenBody);
                defvarLocalVariables(node->data.ifStat.elseBody);
                break;
            case AST_WHILE:
                defvarLocalVariables(node->data.whileStat.body);
                break;
            default:
                break;
        }
        node = node->next;
    }
}

static void printBuiltins()
{
    //print(value: Any): Unit
    printf("LABEL print\n");
    printf("POPS GF@%%tmp\n");
    printf("JUMPIFEQ print$unit GF@%%tmp nil@nil\n");
    printf("WRITE GF@%%tmp\n");
    printf("JUMP print$end\n");
    printf("LABEL print$unit\n");
    printf("WRITE string@()\n");
    printf("LABEL print$end\n");
    printf("PUSHS nil@nil\n"); //return value
    printf("RETURN\n");

    //readLine(): String
    printf("LABEL readLine\n");
    printf("READ GF@%%tmp string\n");
    printf("PUSHS GF@%%tmp\n"); //return value
    printf("RETURN\n");

    //readInt(): Int
    printf("LABEL readInt\n");
    printf("READ GF@%%tmp int\n");
    printf("PUSHS GF@%%tmp\n"); //return value
    printf("RETURN\n");

    //readDouble(): Double
    printf("LABEL readDouble\n");
    printf("READ GF@%%tmp float\n");
    printf("PUSHS GF@%%tmp\n"); //return value
    printf("RETURN\n");

    //toStr(value: Any): String
    printf("LABEL toStr\n");
    printf("POPS GF@%%tmp\n");
    printf("TYPE GF@%%type GF@%%tmp\n");

    printf("JUMPIFEQ toStr$bool GF@%%type string@bool\n");
    printf("JUMPIFEQ toStr$int GF@%%type string@int\n");
    printf("JUMPIFEQ toStr$float GF@%%type string@float\n");
    printf("JUMPIFEQ toStr$unit GF@%%type string@nil\n");

    printf("JUMP toStr$end\n"); //string is already string

    printf("LABEL toStr$bool\n");
    printf("JUMPIFEQ toStr$false GF@%%tmp bool@false\n");

    printf("LABEL toStr$true\n");
    printf("MOVE GF@%%tmp string@true\n");
    printf("JUMP toStr$end\n");

    printf("LABEL toStr$false\n");
    printf("MOVE GF@%%tmp string@false\n");
    printf("JUMP toStr$end\n");

    printf("LABEL toStr$int\n");
    printf("INT2STR GF@%%tmp GF@%%tmp\n");
    printf("JUMP toStr$end\n");

    printf("LABEL toStr$float\n");
    printf("FLOAT2STR GF@%%tmp GF@%%tmp\n");
    printf("JUMP toStr$end\n");

    printf("LABEL toStr$unit\n");
    printf("MOVE GF@%%tmp string@()\n");

    printf("LABEL toStr$end\n");
    printf("PUSHS GF@%%tmp\n"); //return value
    printf("RETURN\n");

    //toInt(value: Double) : Int
    printf("LABEL toInt\n");
    printf("FLOAT2INTS\n");
    printf("RETURN\n");

    //strLength(value: String): Int
    printf("LABEL strLength\n");
    printf("POPS GF@%%tmp\n");
    printf("STRLEN GF@%%tmp GF@%%tmp\n");
    printf("PUSHS GF@%%tmp\n");
    printf("RETURN\n");

    //subStr(value: String, start: Int, end: Int): String
    printf("LABEL subStr\n");
    printf("CREATEFRAME\n");
    printf("PUSHFRAME\n");
    printf("DEFVAR LF@value\n");
    printf("DEFVAR LF@start\n");
    printf("DEFVAR LF@end\n");
    printf("DEFVAR LF@result\n");

    printf("DEFVAR LF@condition\n");
    printf("DEFVAR LF@len\n");

    printf("POPS LF@end\n"); //pop arguments from stack last to first
    printf("POPS LF@start\n");
    printf("POPS LF@value\n");

    printf("LT LF@condition LF@start int@0\n"); // start < 0
    printf("JUMPIFEQ subStr$invalid LF@condition bool@true\n");

    printf("LT LF@condition LF@end int@0\n"); // end < 0
    printf("JUMPIFEQ subStr$invalid LF@condition bool@true\n");

    printf("GT LF@condition LF@start LF@end\n"); // start > end
    printf("JUMPIFEQ subStr$invalid LF@condition bool@true\n");

    printf("PUSHS LF@value\n"); //start >= strLength(value)
    printf("CALL strLength\n");
    printf("POPS LF@len\n");
    printf("LT LF@condition LF@start LF@len\n");
    printf("JUMPIFEQ subStr$invalid LF@condition bool@false\n");

    printf("GT LF@condition LF@end LF@len\n"); //end > strLength(value)
    printf("JUMPIFEQ subStr$invalid LF@condition bool@true\n");

    printf("MOVE LF@result string@\n");

    printf("LABEL subStr$cycle\n");
    printf("JUMPIFEQ subStr$end LF@start LF@end\n");
    printf("GETCHAR GF@%%tmp LF@value LF@start\n");
    printf("PUSHS LF@start\n");
    printf("PUSHS int@1\n");
    printf("ADDS\n");
    printf("POPS LF@start\n");

    printf("CONCAT LF@result LF@result GF@%%tmp\n");
    printf("JUMP subStr$cycle\n");

    printf("LABEL subStr$end\n");
    printf("PUSHS LF@result\n"); //return value
    printf("POPFRAME\n");
    printf("RETURN\n");

    printf("LABEL subStr$invalid\n");
    printf("EXIT int@40\n"); //exit

    //ord(value: String, index: Int): Int
    printf("LABEL ord\n");
    printf("CREATEFRAME\n");
    printf("PUSHFRAME\n");
    printf("DEFVAR LF@value\n");
    printf("DEFVAR LF@index\n");
    printf("DEFVAR LF@len\n");
    printf("DEFVAR LF@condition\n");

    printf("POPS LF@index\n"); //pop arguments from stack last to first
    printf("POPS LF@value\n");

    printf("JUMPIFEQ ord$invalid LF@value string@\n"); //empty string

    printf("LT LF@condition LF@index int@0\n"); // index < 0
    printf("JUMPIFEQ ord$invalid LF@condition bool@true\n");

    printf("STRLEN LF@len LF@value\n");
    printf("LT LF@condition LF@index LF@len\n"); // !(index < len)
    printf("JUMPIFEQ ord$invalid LF@condition bool@false\n");

    printf("STRI2INT GF@%%tmp LF@value LF@index\n");
    printf("PUSHS GF@%%tmp\n"); //return value
    printf("JUMP ord$end\n");

    printf("LABEL ord$invalid\n"); //invalid returns -1
    printf("PUSHS int@-1\n");

    printf("LABEL ord$end\n");
    printf("POPFRAME\n");
    printf("RETURN\n");

    //chr(code: Int): String
    printf("LABEL chr\n");
    printf("INT2CHARS\n");
    printf("RETURN\n");
}
/*** End of file codegen.c ***/
