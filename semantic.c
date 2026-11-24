/******************************************************************************
 * Project:		IFJ
 * Package:		semantic
 * File:			semantic.c
 * Date:			16.11.2026
 * Last change:	24.11.2026
 * Authors: 		Nikolas Fox xfoxnik00
 *
 * Description: Implementation of semantic analysis logic
 *
 *******************************************************************************/
/**
 * @file semantic.c
 *
 * @brief  Implementation of semantic analysis logic
 * @author Nikolas Fox xfoxnik00
 */

#include "semantic.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static void stInsertFunctions(Symtable *, AstNode *);
static void findCalls(Symtable *, AstNode *, AstNode *, AstNode *);
static void checkFunctionList(Symtable *, AstNode *);
static void checkFunction(Symtable *, AstNode *);
static DataType checkStatementList(Symtable *, AstNode *, DataType);
static DataType checkStatement(Symtable *, AstNode *, DataType);
static DataType checkExpr(Symtable *, AstNode *);

void semanticAnalysis(AstNode *whole) {
    Symtable *symtable = symtableInit();
    stInsertFunctions(symtable, whole);
    checkFunctionList(symtable,
                      whole); // this will pass into every function and then
                              // check the functions statements...
    symtableFree(symtable);
}

static bool hasParams(AstNode *group) {
    while (group != NULL) {
        if (group->data.group.count > 0) // the group has a parameter
        {
            return true;
        }
        group = group->next;
    }
    return false; // if we get to the end then it doesn't have a parameter
}

static bool areCompatible(DataType destination, DataType source) {
    if (destination == TYPE_NONE || source == TYPE_NONE) {
        return false;
    }
    if (destination == source ||
        (destination == TYPE_DOUBLE && source == TYPE_INT) // int -> double
        || destination == TYPE_ANY) {
        return true;
    }
    return false;
}

static bool isNumber(DataType type) {
    if (type == TYPE_INT || type == TYPE_DOUBLE) {
        return true;
    }
    return false;
}

static void stInsertFunctions(Symtable *symtable, AstNode *whole) {
    size_t mainCount = 0;
    AstNode *node = whole->data.whole.func;

    while (node != NULL) {
        bool hasType = (node->type != TYPE_NONE);

        SymbolData *function = symtableInsertFunction(
            symtable, node->data.func.name, node->type, true);
        AstNode *group = node->data.func.group;

        // inserting the parameters into the symtable (first activate a list -
        // group, and then insert individual parameters)
        while (group != NULL) {
            symtableStartParamList(function);
            AstNode *param =
                group->data.group
                    .items; // node->data.group.items is the pointer to the
                            // first parameter in the group
            while (param != NULL) {
                symtableAddParam(symtable, function, param->data.param.name,
                                 param->type);
                param = param->next;
            }
            group = group->next;
        }

        if (node->data.func.isMain == true) {
            mainCount++;
            if (hasParams(node->data.func.group) == true) {
                exit(5);
            }
            if (node->type != TYPE_UNIT && hasType) {
                exit(6);
            }
        }
        node = node->next;
    }

    if (mainCount == 0) {
        exit(3);
    } else if (mainCount > 1) {
        exit(4);
    }
}

// I will use the following states:   NOT_VISITED, PROCESSING, DONE
static void visit(Symtable *symtable, AstNode *whole, AstNode *function) {
    if (function->data.func.state == DONE) {
        return;
    }

    if (function->data.func.state == PROCESSING) {
        if (function->type == TYPE_NONE) {
            exit(6); // this creates a cycle of untyped functions
        }
        return; // skip typed functions
    }

    function->data.func.state = PROCESSING;

    // this function will pass through the body of a function, when it finds a
    // function without a return type, it passes through the body of that
    // function
    findCalls(symtable, whole, function, function->data.func.body);
    checkFunction(symtable, function);
    function->data.func.state = DONE;
}

// this function returns a pointer to a function with a certain name
static AstNode *findFunction(AstNode *whole, char *name) {
    AstNode *function = whole->data.whole.func;
    while (function != NULL) {
        if (strcmp(function->data.func.name, name) == 0) {
            return function;
        }
        function = function->next;
    }
    return NULL;
}
static void findCalls(Symtable *symtable, AstNode *whole, AstNode *function,
                      AstNode *statement) {
    while (statement != NULL) {
        switch (statement->class) {
        case AST_CALL: {
            AstNode *calledFunction =
                findFunction(whole, statement->data.call.name);
            if (calledFunction != NULL && calledFunction->type == TYPE_NONE) {
                visit(symtable, whole, calledFunction);
            }

            AstNode *group = statement->data.call.group;
            while (group != NULL) // finding function calls in parameters
            {
                findCalls(symtable, whole, function, group->data.group.items);
                group = group->next;
            }
            break;
        }

        case AST_ID: {
            AstNode *calledFunction =
                findFunction(whole, statement->data.id.name);
            if (calledFunction != NULL && calledFunction->type == TYPE_NONE) {
                visit(symtable, whole, calledFunction);
            }
            break;
        }

        case AST_ASSIGN:
            findCalls(symtable, whole, function,
                      statement->data.assign.expression);
            break;

        case AST_DECLARATION:
            findCalls(symtable, whole, function,
                      statement->data.declaration.expression);
            break;

        case AST_RETURN:
            findCalls(symtable, whole, function,
                      statement->data.returnStat.expression);
            break;

        case AST_IF:
            findCalls(symtable, whole, function,
                      statement->data.ifStat.condition);
            findCalls(symtable, whole, function,
                      statement->data.ifStat.thenBody);
            findCalls(symtable, whole, function,
                      statement->data.ifStat.elseBody);
            break;

        case AST_WHILE:
            findCalls(symtable, whole, function,
                      statement->data.whileStat.condition);
            findCalls(symtable, whole, function,
                      statement->data.whileStat.body);
            break;

        case AST_UNARY:
            findCalls(symtable, whole, function, statement->data.unary.operand);
            break;

        case AST_BINARY:
            findCalls(symtable, whole, function, statement->data.binary.left);

            if(statement->data.binary.op != TOK_DOT) // I need to guard the right call, because it won't be a function in the symtable
            {
                findCalls(symtable, whole, function, statement->data.binary.right);
            }
            break;

        default:
            break;
        }
        statement = statement->next;
    }
}

static void checkFunctionList(Symtable *symtable, AstNode *whole) {
    AstNode *function = whole->data.whole.func;
    while (function != NULL) {
        visit(symtable, whole, function);
        function = function->next;
    }
}

static void checkFunction(Symtable *symtable, AstNode *function) {
    symtablePushScope(symtable);
    // parameters of the functions will have the next scope
    AstNode *group = function->data.func.group;
    while (group != NULL) {
        AstNode *param = group->data.group.items;
        while (param != NULL) {
            param->data.param.scope = symtableScopeID(symtable);
            symtableInsertVal(symtable, param->data.param.name, param->type,
                              true);

            param = param->next;
        }
        group = group->next;
    }

    DataType blockType = checkStatementList(
        symtable, function->data.func.body,
        function->type); // function->type to check compatibility (TYPE_NONE
                         // means implicit typing)
    symtablePopScope(symtable);

    if (function->type == TYPE_NONE) // this means implicit typing
    {
        if (blockType == TYPE_NONE) // the type can't be deduced
        {
            exit(6);
        }

        if (function->data.func.isMain == true && blockType != TYPE_UNIT) {
            exit(6); // main is not TYPE_UNIT
        }

        function->type = blockType; // updating AST with function type

        SymbolData *functionSymbol =
            symtableSearch(symtable, function->data.func.name);
        functionSymbol->dtype =
            blockType; // updating symtable with function type
    } else {
        if (function->type != TYPE_UNIT) {
            if (areCompatible(function->type, blockType) == false &&
                blockType != TYPE_RETURNED) {
                exit(6); // the block type isn't compatible with the return type
            }
        }
    }
}

static DataType checkStatementList(Symtable *symtable, AstNode *statement,
                                   DataType retType) {
    DataType last = TYPE_UNIT;
    while (statement != NULL) {
        last = checkStatement(symtable, statement, retType);
        statement = statement->next;
    }
    return last; // this is the type of the last statement of a block (used for
                 // return type)
}

static DataType checkStatement(Symtable *symtable, AstNode *statement,
                               DataType retType) {
    switch (statement->class) {
    case AST_ASSIGN: {
        AstNode *destination = statement->data.assign.destination;
        AstNode *expression = statement->data.assign.expression;
        SymbolData *symbol =
            symtableSearch(symtable, destination->data.id.name);

        if (symbol == NULL || symbol->sType == FUNCTION) {
            exit(3);
        }

        if (symbol->sType == VAL) // you can't assign to a value nor parameter
        {
            exit(7);
        }

        destination->type = symbol->dtype;
        expression->type = checkExpr(symtable, expression);

        if (areCompatible(destination->type, expression->type) == false) {
            exit(6); // error of type compatibility
        }

        destination->data.id.scope =
            symbol->scopeID; // this isn't a declaration, so we have to look at
                             // the symbol scope, not at the current scope
        return TYPE_UNIT;
    }

    case AST_DECLARATION: {
        DataType expressionType =
            checkExpr(symtable, statement->data.declaration.expression);
        if (statement->type == TYPE_NONE) // statement->type is the explicit
                                          // type of the variable/value
        {
            if (expressionType == TYPE_NONE) {
                exit(6); // error of inability of taking on implicit type
            }
            statement->type = expressionType; // the declared value/variable
                                              // takes on the expression type
        } else {
            if (areCompatible(statement->type, expressionType) == false) {
                exit(6); // error of type compatibility
            }
        }

        if (statement->data.declaration.isVar) {
            symtableInsertVar(symtable, statement->data.declaration.name,
                              statement->type, true);
        } else {
            symtableInsertVal(symtable, statement->data.declaration.name,
                              statement->type, true);
        }
        statement->data.declaration.scope = symtableScopeID(symtable);
        return TYPE_UNIT;
    }

    case AST_IF: {
        AstNode *condition = statement->data.ifStat.condition;
        if (checkExpr(symtable, condition) != TYPE_BOOL) {
            exit(6); // incompatible condtion type (isn't bool)
        }

        symtablePushScope(symtable);
        DataType thenType = checkStatementList(
            symtable, statement->data.ifStat.thenBody, retType);

        symtablePopScope(symtable);

        DataType elseType =
            TYPE_UNIT; // if else body isn't defined it should be TYPE_UNIT
        if (statement->data.ifStat.elseBody != NULL) {
            symtablePushScope(symtable);
            elseType = checkStatementList(
                symtable, statement->data.ifStat.elseBody, retType);
            symtablePopScope(symtable);
        }

        if (thenType == TYPE_RETURNED && elseType == TYPE_RETURNED) {
            return TYPE_RETURNED; // all branches must end with TYPE_RETURNED
        }

        if (thenType == TYPE_UNIT && elseType == TYPE_UNIT &&
            retType == TYPE_NONE) {
            return TYPE_UNIT;
        }

        return TYPE_NONE; // this is only valid if the reType is TYPE_UNIT
    }

    case AST_WHILE: {
        AstNode *condition = statement->data.whileStat.condition;
        if (checkExpr(symtable, condition) != TYPE_BOOL) {
            exit(6); // incompatible condtion type (isn't bool)
        }

        symtablePushScope(symtable);
        checkStatementList(symtable, statement->data.whileStat.body, retType);
        symtablePopScope(symtable);
        return TYPE_UNIT;
    }

    case AST_RETURN: {
        AstNode *expression = statement->data.returnStat.expression;
        if (retType == TYPE_NONE) {
            exit(6); // you can't have return if no retType is stated, changed exit code from 10 to 6 due to test case 137 and 138 in ifj26-pokusne
        }
        if (expression == NULL) {
            if (retType != TYPE_UNIT) {
                exit(6); // if there is just return; then the return type must
                         // be TYPE_UNIT
            }
            return TYPE_RETURNED; // TYPE_RETURNED tells the function that
                                  // keyword RETURN was used
        }
        if (!areCompatible(retType, checkExpr(symtable, expression)) &&
            retType != TYPE_UNIT) // if retType is TYPE_UNIT the result is
                                  // disregarded (but valid)
        {
            exit(6); // the type of the returned expression is incompatible with
                     // the return type of the function
        }
        return TYPE_RETURNED; // TYPE_RETURNED tells the function that keyword
                              // RETURN was used
    }

    default: // this is just a bare expression
        return checkExpr(symtable, statement);
    }
}

static DataType dotToCall(AstNode *expression, AstNode *parameter,  AstNode *callId, DataType returnType, const char *name)
{
 // changing the node type
    AstNode *group = createNodeGroup(parameter, 1);
    expression->class = AST_CALL;
    expression->type = returnType;
    expression->data.call.group = group;
    expression->data.call.groupCount = 1;
    expression->data.call.name = copyString(name); // copyString so freeAst doesn't free a constant
    AstFree(callId);
    return returnType;
} 

static DataType checkExpr(Symtable *symtable, AstNode *expression) {
    switch (expression->class) {
    case AST_LITERAL:
        return expression->type;

    case AST_ID: {
        char *name = expression->data.id.name;
        SymbolData *symbol = symtableSearch(symtable, name);
        if (symbol == NULL) {
            if (strcmp(name, "true") == 0 || strcmp(name, "false") == 0) {
                expression->class = AST_LITERAL;
                expression->type = TYPE_BOOL;
                return TYPE_BOOL;
            }

            exit(3); // no such value/variable exists
        }

        if (symbol->sType ==
            FUNCTION) // when a function has no parameter list: 'foo'
        {
            if (symbol->value.funcData.listCount >
                0) // we need to check whether it shouldn't have more parameters
                   // here
            {
                exit(5);
            }
            // retype so it is a call, not an ID
            expression->class = AST_CALL;
            expression->data.call.group = NULL;
            expression->data.call.groupCount = 0;
            expression->type = symbol->dtype;
            return expression->type;
        }
        expression->data.id.scope = symbol->scopeID;
        expression->type = symbol->dtype;
        return expression->type;
    }

    case AST_UNARY: {
        DataType type = checkExpr(symtable, expression->data.unary.operand);
        if (expression->data.unary.op == TOK_NOT) {
            if (type != TYPE_BOOL) {
                exit(6); // NOT must be followed by bool type
            }
        } else {
            if (isNumber(type) == false) // everything else is sign (so it must
                                         // be followed by int or double)
            {
                exit(6);
            }
        }
        expression->type = type;
        return type;
    }

    case AST_BINARY: {

        AstNode *left = expression->data.binary.left;
        AstNode *right = expression->data.binary.right;
        TokenType operator= expression->data.binary.op;

        // VALMEM extension
        // check for expr.id first, because id will not be in the symtable (checkExpr will fail)
         if(operator == TOK_DOT)
         {
            if(right->class != AST_ID)
            {
                exit(3); // invalid function call
            }

            char *name = right->data.id.name; // this is the name of the string after '.'
            DataType expressionType = checkExpr(symtable, expression->data.binary.left);

            if(strcmp(name, "toInt") == 0)
            {
                if(areCompatible(TYPE_DOUBLE, expressionType))
                {
                    return dotToCall(expression, left, right, TYPE_INT, "toInt");
                }
                exit(5); // invalid argument type 
            } 
            else if(strcmp(name, "toString") == 0)
            {
                if(areCompatible(TYPE_ANY, expressionType))
                {
                   return dotToCall(expression, left, right, TYPE_STRING, "toStr");
                }
                exit(5); // invalid argument type 
            }
            else if(strcmp(name, "floor") == 0)
            {
                if(areCompatible(TYPE_DOUBLE, expressionType))
                {
                    return dotToCall(expression, left, right, TYPE_DOUBLE, "floor");
                }
                exit(5); // invalid argument type 
            }
            else if(strcmp(name, "length") == 0)
            {
                if(areCompatible(TYPE_STRING, expressionType))
                {
                    return dotToCall(expression, left, right, TYPE_INT, "strLength");
                }
                exit(5); // invalid argument type 
            }
            exit(3); // invalid function call (this call isn't defined in VALMEM)
        }

        DataType leftType = checkExpr(symtable, left);
        DataType rightType = checkExpr(symtable, right);

        switch (operator) {
        case TOK_PLUS:
            if (leftType == TYPE_STRING && rightType == TYPE_STRING) {
                expression->type = TYPE_STRING;
                return TYPE_STRING;
            }
        // FALLTHROUGH
        case TOK_MINUS:
        case TOK_DIV:
        case TOK_MUL: {
            if ((isNumber(leftType) && isNumber(rightType)) == false) {
                exit(6); // unallowed type in +-*/
            }
            if (leftType == TYPE_INT && rightType == TYPE_INT) {
                expression->type = TYPE_INT;
                return TYPE_INT;
            } else // at least one of them is TYPE_DOUBLE
            {
                expression->type = TYPE_DOUBLE;
                return TYPE_DOUBLE;
            }
        }

        case TOK_LESS:
        case TOK_LESS_EQ:
        case TOK_GREATER:
        case TOK_GREATER_EQ:
            if ((isNumber(leftType) && isNumber(rightType)) ||
                (leftType == TYPE_STRING && rightType == TYPE_STRING)) {
                expression->type = TYPE_BOOL;
                return TYPE_BOOL;
            }
            exit(6);

        case TOK_EQ:
        case TOK_NEQ:
            if ((isNumber(leftType) && isNumber(rightType)) ||
                (leftType != TYPE_NONE &&
                 leftType == rightType)) // TYPE_NONE would mean they aren't defined
            {
                expression->type = TYPE_BOOL;
                return TYPE_BOOL;
            }
            exit(6);

        case TOK_AND:
        case TOK_OR:
            if (leftType == TYPE_BOOL && rightType == TYPE_BOOL) {
                expression->type = TYPE_BOOL;
                return TYPE_BOOL;
            }
            exit(6);


        default:
            exit(99); // undefined binary operator
        }
    }

    case AST_CALL: {
        SymbolData *function =
            symtableSearch(symtable, expression->data.call.name);
        if (function == NULL || function->sType != FUNCTION) {
            exit(3); // undefined function
        }

        if (function->value.funcData.listCount !=
            expression->data.call.groupCount) {
            exit(5); // the number of parameter lists differs
        }

        size_t list = 0; // for looking up parameters in the symtable

        AstNode *group = expression->data.call.group;
        while (group != NULL) {
            if (group->data.group.count !=
                symtableParamListSize(function, list)) {
                exit(5); // the number of parameters in a parameter list differs
            }

            AstNode *arg = group->data.group.items;
            for (size_t parameterIndex = 0;
                 parameterIndex < group->data.group.count; parameterIndex++) {
                const FunctionParam *parameter =
                    symtableGetParam(function, list, parameterIndex);
                DataType parameterType = parameter->dType;

                DataType argType = checkExpr(symtable, arg);

                // parameter type is the destination
                if (areCompatible(parameterType, argType) == false) {
                    exit(5); // invalid argument type
                }

                arg = arg->next;
            }
            list++;
            group = group->next;
        }
        expression->type = function->dtype;
        return expression->type;
    }

    default:
        return TYPE_NONE;
    }
}
/*** End of file semantic.c ***/
