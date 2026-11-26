/******************************************************************************
 * Project:      IFJ
 * Package:      Semantic
 * File:         semantic.c
 * Date:         16. 11. 2026
 * Last change:  26. 11. 2026
 * Authors:      Nikolas Fox xfoxnik00
 *
 * Description:  Implementation of semantic analysis logic
 *****************************************************************************/
/**
 * @file semantic.c
 *
 * @brief  Implementation of semantic analysis logic
 * @author Nikolas Fox xfoxnik00
 */

#include "semantic.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void stInsertFunctions(Symtable *, AstNode *, int *error);
static void findCalls(Symtable *, AstNode *, AstNode *, int *error);
static void checkFunctionList(Symtable *, AstNode *, int *error);
static void checkFunction(Symtable *, AstNode *, int *error);
static DataType checkStatementList(Symtable *, AstNode *, DataType, int *error);
static DataType checkStatement(Symtable *, AstNode *, DataType, int *error);
static DataType checkExpr(Symtable *, AstNode *, int *error);

/**
 * @brief Prints error message.
 * @param message Error message to print.
 */
static void printError(char *message) {
    fprintf(stderr, "Semantic analysis error: %s\n", message);
}

void semanticAnalysis(AstNode *whole, int *error) {
    Symtable *symtable = symtableInit();
    *error = 0;
    stInsertFunctions(symtable, whole, error);
    if (*error != 0) // return from stInsertFunctions
    {
        symtableFree(symtable);
        return;
    }
    checkFunctionList(symtable, whole,
                      error); // this will pass into every function and then
                              // check the functions statements...
    if (*error != 0)          // return from checkFunctionList
    {
        symtableFree(symtable);
        return;
    }
    symtableFree(symtable);
}

/**
 * @brief Pass through groups of parameters and checks whether there is at least
 * one parameter
 * @param group Pointer to the first parameter group in a linked list of
 * parameter groups
 * @return False if no group has a parameter / true otherwise
 */
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

/**
 * @brief Checks compatibility of types
 * @param destination The data type of the destination
 * @param source The data type of the source
 * @return True if compatible / false otherwise
 */
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

/**
 * @brief Checks whether a type is a number (double or int)
 * @param type
 * @return true for int and double / false otherwise
 */
static bool isNumber(DataType type) {
    return type == TYPE_INT || type == TYPE_DOUBLE;
}

/**
 * @brief Insert functions and parameters into the symtable
 * @param symtable Pointer to the symtable
 * @param whole Pointer to the first function (root of the tree)
 */
static void stInsertFunctions(Symtable *symtable, AstNode *whole, int *error) {
    size_t mainCount = 0;
    AstNode *node = whole->data.whole.func;

    while (node != NULL) {
        bool hasType = (node->type != TYPE_NONE);

        SymbolData *function = symtableInsertFunction(
            symtable, node->data.func.name, node->type, true);
        if (function == NULL) {
            *error = 4;
            return;
        }
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

        if (node->data.func.isMain) {
            mainCount++;
            if (hasParams(node->data.func.group)) {
                *error = 5;
                printError("");
                return;
            }
            if (node->type != TYPE_UNIT && hasType) {
                *error = 6;
                printError("");
                return;
            }
        }
        node = node->next;
    }

    if (mainCount == 0) {
        *error = 3;
        printError("");
        return;
    } else if (mainCount > 1) {
        *error = 4;
        printError("");
        return;
    }
}

/**
 * @brief Resolves implicit return types of a function recursively and
 * type-checks the function
 *
 * I mapped functions to have three states: NOT_VISITED, PROCESSING, DONE to
 * deduce dependency of functions If a cyclic dependency is found -> exit(6)
 * @param symtable Pointer to the symtable
 * @param whole Pointer to the first function (root of the tree)
 * @param function Pointer to the function to be visited
 */
static void visit(Symtable *symtable, AstNode *whole, AstNode *function,
                  int *error) {
    if (function->data.func.state == DONE) {
        return;
    }

    if (function->data.func.state == PROCESSING) {
        if (function->type == TYPE_NONE) {
            *error = 6;
            printError("a cycle of untyped functions");
            return;
        }
        return; // skip typed functions
    }

    function->data.func.state = PROCESSING;

    // this function will pass through the body of a function, when it finds a
    // function without a return type, it passes through the body of that
    // function
    findCalls(symtable, whole, function->data.func.body, error);
    checkFunction(symtable, function, error);
    if (*error != 0) // return from checkFunction
    {
        return;
    }
    function->data.func.state = DONE;
}

/**
 * @brief Searches through a function linked list and returns a pointer to the
 * function with the specified name
 * @param whole Pointer to the first function in a linked list of functions
 * @param name The name of the function to look for
 * @return function Pointer to the function with the specified name / NULL if it
 * doesn't exist
 */
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

/**
 * @brief Searches through a statement block and visits all called functions
 * @param symtable Pointer to the table of symbols
 * @param whole Pointer to the first function in a linked list of functions
 * @param statement Pointer to the first statement in a statement list
 */
static void findCalls(Symtable *symtable, AstNode *whole, AstNode *statement,
                      int *error) {
    while (statement != NULL) {
        switch (statement->class) {
        case AST_CALL: {
            AstNode *calledFunction = findFunction(
                whole, statement->data.call
                           .name); // built in functions are skipped (we already
                                   // know their return type)
            if (calledFunction != NULL && calledFunction->type == TYPE_NONE) {
                visit(symtable, whole, calledFunction, error);
            }
            if (*error != 0) // return from visit
            {
                return;
            }

            AstNode *group = statement->data.call.group;
            while (group != NULL) // finding function calls in parameters
            {
                findCalls(symtable, whole, group->data.group.items, error);
                group = group->next;
            }
            break;
        }

        case AST_ID: {
            AstNode *calledFunction =
                findFunction(whole, statement->data.id.name);
            if (calledFunction != NULL && calledFunction->type == TYPE_NONE) {
                visit(symtable, whole, calledFunction, error);
            }
            break;
        }

        case AST_ASSIGN:
            findCalls(symtable, whole, statement->data.assign.expression,
                      error);
            break;

        case AST_DECLARATION:
            findCalls(symtable, whole, statement->data.declaration.expression,
                      error);
            break;

        case AST_RETURN:
            findCalls(symtable, whole, statement->data.returnStat.expression,
                      error);
            break;

        case AST_IF:
            findCalls(symtable, whole, statement->data.ifStat.condition, error);
            findCalls(symtable, whole, statement->data.ifStat.thenBody, error);
            findCalls(symtable, whole, statement->data.ifStat.elseBody, error);
            break;

        case AST_WHILE:
            findCalls(symtable, whole, statement->data.whileStat.condition,
                      error);
            findCalls(symtable, whole, statement->data.whileStat.body, error);
            break;

        case AST_UNARY:
            findCalls(symtable, whole, statement->data.unary.operand, error);
            break;

        case AST_BINARY:
            findCalls(symtable, whole, statement->data.binary.left, error);

            if (statement->data.binary.op !=
                TOK_DOT) // I need to guard the right call, because it won't be
                         // a function in the symtable
            {
                findCalls(symtable, whole, statement->data.binary.right, error);
            }
            break;

        default:
            break;
        }
        statement = statement->next;
    }
}

/**
 * @brief Visits every function in a linked list of functions (visit skips over
 * already visited functions)
 * @param symtable Pointer to the table of symbols
 * @param whole Pointer to the first function in a linked list of functions
 */
static void checkFunctionList(Symtable *symtable, AstNode *whole, int *error) {

    AstNode *function = whole->data.whole.func;
    while (function != NULL) {
        visit(symtable, whole, function, error);
        if (*error != 0) // return from visit
        {
            return;
        }
        function = function->next;
    }
}

/**
 * @brief Inserts all parameters of a function into the symtable (helper for
 * checkFunction)
 * @param symtable Pointer to the table of symbols
 * @param function Pointer to the function whose parameters should be inserted
 */
static void stInsertParameters(Symtable *symtable, AstNode *function,
                               int *error) {
    AstNode *group = function->data.func.group;
    while (group != NULL) {
        AstNode *param = group->data.group.items;
        while (param != NULL) {
            param->data.param.scope = symtableScopeID(symtable);
            symtableInsertVal(symtable, param->data.param.name, param->type,
                              true);
            if (function == NULL) {
                *error = 4;
                return;
            }
            param = param->next;
        }
        group = group->next;
    }
}

/**
 * @brief Semantic check of a function
 * @param symtable Pointer to the table of symbols
 * @param function Pointer to the function to be checked
 */
static void checkFunction(Symtable *symtable, AstNode *function, int *error) {
    symtablePushScope(symtable);
    stInsertParameters(symtable, function, error);
    if (*error != 0) // return from stInsertParameters
    {
        return;
    }

    DataType blockType =
        checkStatementList(symtable, function->data.func.body, function->type,
                           error); // function->type to check compatibility
    if (*error != 0)               // return from checkStatementList
    {
        return;
    }
    symtablePopScope(symtable);

    if (function->type == TYPE_NONE) // TYPE_NONE means return type is inferred
    {
        if (blockType == TYPE_NONE) {
            *error = 6;
            printError("function type can't be deduced, return type is "
                       "inferred and the body doesn't have a type");
            return;
        }

        if (function->data.func.isMain && blockType != TYPE_UNIT) {
            *error = 6;
            printError("deduced main type is not TYPE_UNIT");
            return;
        }

        function->type = blockType; // updating AST with deduced function type

        SymbolData *functionSymbol =
            symtableSearch(symtable, function->data.func.name);
        functionSymbol->dtype =
            blockType; // updating symtable with function type
    } else {
        if (function->type != TYPE_UNIT) {
            if (!areCompatible(function->type, blockType) &&
                blockType != TYPE_RETURNED) {
                *error = 6;
                printError(
                    "the block type isn't compatible with the return type");
                return;
            }
        }
    }
}

/**
 * @brief Finds the data type of a linked list of statements
 * If no return is found then the type is inferred from the last statement
 * @param symtable Pointer to the table of symbols
 * @param statement Pointer to the first statement in a linked list of
 * statements
 * @param retType The functions declared return type (TYPE_NONE if it relies on
 * inferrence)
 */
static DataType checkStatementList(Symtable *symtable, AstNode *statement,
                                   DataType retType, int *error) {
    DataType last = TYPE_UNIT;
    while (statement != NULL) {
        last = checkStatement(symtable, statement, retType, error);
        if (*error != 0) // return from checkStatement
        {
            return TYPE_NONE;
        }
        statement = statement->next;
    }
    return last; // the type of the last statement of a block (it must
                 // correspond with the declared return type)
}

/**
 * @brief Finds the data type of a statements
 * ASSIGN, DEFINITION and LOOPS are TYPE_UNIT
 * @param symtable Pointer to the table of symbols
 * @param statement Pointer to the statement to be checked
 * @param retType The functions declared return type (TYPE_NONE if it relies on
 * inferrence)
 * @return The deduced type of the statement
 */
static DataType checkStatement(Symtable *symtable, AstNode *statement,
                               DataType retType, int *error) {
    switch (statement->class) {
    case AST_ASSIGN: {
        AstNode *destination = statement->data.assign.destination;
        AstNode *expression = statement->data.assign.expression;
        SymbolData *symbol =
            symtableSearch(symtable, destination->data.id.name);

        if (symbol == NULL || symbol->sType == FUNCTION) {
            *error = 3;
            printError("the destination isn't defined yet or the destination "
                       "is a function");
            return TYPE_NONE;
        }

        if (symbol->sType == VAL) {
            *error = 7;
            printError("assigning a value to val or parameter is forbidden");
            return TYPE_NONE;
        }

        destination->type = symbol->dtype;
        expression->type = checkExpr(symtable, expression, error);
        if (*error != 0) // return from checkExpr
        {
            return TYPE_NONE;
        }

        if (!areCompatible(destination->type, expression->type)) {
            *error = 6;
            printError("error of type compatibility in assign");
            return TYPE_NONE;
        }

        destination->data.id.scope = symbol->scopeID;
        return TYPE_UNIT;
    }

    case AST_DECLARATION: {
        DataType expressionType =
            checkExpr(symtable, statement->data.declaration.expression, error);
        // statement->type represents the variables declared type
        if (*error != 0) // return from checkExpr
        {
            return TYPE_NONE;
        }

        if (statement->type == TYPE_NONE) // inferred type
        {
            if (expressionType == TYPE_NONE) {
                *error = 6;
                printError("type cannot be inferred");
                return TYPE_NONE;
            }
            statement->type = expressionType; // the declared value/variable
                                              // acquires the expression type
        } else {
            if (!areCompatible(statement->type, expressionType)) {
                *error = 6;
                printError("error of type compatibility");
                return TYPE_NONE;
            }
        }

        SymbolData *inserted;

        if (statement->data.declaration.isVar) {
            inserted =
                symtableInsertVar(symtable, statement->data.declaration.name,
                                  statement->type, true);
            if (inserted == NULL) {
                *error = 4;
                return TYPE_NONE;
            }
        } else {
            inserted =
                symtableInsertVal(symtable, statement->data.declaration.name,
                                  statement->type, true);
            if (inserted == NULL) {
                *error = 4;
                return TYPE_NONE;
            }
        }
        statement->data.declaration.scope = symtableScopeID(symtable);
        return TYPE_UNIT;
    }

    case AST_WHILE: {
        AstNode *condition = statement->data.whileStat.condition;
        if (checkExpr(symtable, condition, error) != TYPE_BOOL) {
            if (*error != 0) // return from checkExpr
            {
                return TYPE_NONE;
            }
            *error = 6;
            printError("incompatible condition type (isn't bool)");
            return TYPE_NONE;
        }

        symtablePushScope(symtable);
        checkStatementList(symtable, statement->data.whileStat.body, retType,
                           error);
        symtablePopScope(symtable);
        return TYPE_UNIT;
    }

    case AST_IF: {
        AstNode *condition = statement->data.ifStat.condition;
        if (checkExpr(symtable, condition, error) != TYPE_BOOL) {
            if (*error != 0) // return from checkExpr
            {
                return TYPE_NONE;
            }
            *error = 6;
            printError("incompatible condition type (isn't bool)");
            return TYPE_NONE;
        }

        symtablePushScope(symtable);
        DataType thenType = checkStatementList(
            symtable, statement->data.ifStat.thenBody, retType, error);
        if (*error != 0) // return from checkStatementList
        {
            return TYPE_NONE;
        }

        symtablePopScope(symtable);

        DataType elseType =
            TYPE_UNIT; // undefined else body defaults to TYPE_UNIT

        if (statement->data.ifStat.elseBody != NULL) {
            symtablePushScope(symtable);
            elseType = checkStatementList(
                symtable, statement->data.ifStat.elseBody, retType, error);
            if (*error != 0) // return from checkStatementList
            {
                return TYPE_NONE;
            }
            symtablePopScope(symtable);
        }

        if (thenType == TYPE_RETURNED && elseType == TYPE_RETURNED) {
            return TYPE_RETURNED; // all branches must end with TYPE_RETURNED
        }

        if (thenType == TYPE_UNIT && elseType == TYPE_UNIT &&
            retType == TYPE_NONE) {
            return TYPE_UNIT;
        }

        return TYPE_NONE; // this is only valid if the retType is TYPE_UNIT
    }

    case AST_RETURN: {
        AstNode *expression = statement->data.returnStat.expression;
        if (retType == TYPE_NONE) {
            *error = 6;
            printError(
                "return is forbidden if the function type is inferred"); // changed
                                                                         // exit
                                                                         // code
                                                                         // from
                                                                         // 10
                                                                         // to 6
                                                                         // due
                                                                         // to
                                                                         // test
                                                                         // case
                                                                         // 137
                                                                         // and
                                                                         // 138
                                                                         // in
                                                                         // ifj26-pokusne
            return TYPE_NONE;
        }
        if (expression == NULL) { // a blank 'return' defaults to TYPE_UNIT
            if (retType != TYPE_UNIT) {
                *error = 6;
                printError(
                    "incompatible return type (a blank return is TYPE_UNIT)");
                return TYPE_NONE;
            }
            return TYPE_RETURNED; // TYPE_RETURNED means that the keyword RETURN
                                  // was used
        }

        DataType expressionType = checkExpr(symtable, expression, error);
        if (*error != 0) // return from checkExpr
        {
            return TYPE_NONE;
        }

        // if retType is TYPE_UNIT the result is disregarded (but valid)
        if (retType != TYPE_UNIT && !areCompatible(retType, expressionType)) {
            *error = 6;
            printError("incompatible return type");
            return TYPE_NONE;
        }
        return TYPE_RETURNED; // TYPE_RETURNED tells the function that keyword
                              // RETURN was used
    }

    default: // this is just a bare expression
        return checkExpr(symtable, statement, error);
    }
}

/**
 * @brief Converts an AST_BINARY node:'parameter.id' to AST_CALL
 * node'id(parameter)'
 * @param node The node that will be retyped
 * @param parameter The left side of the call
 * @param id The right side of the call
 * @param returnType Return type of the called function
 * @param name The name of the corresponding built-in function
 * @return The return type of the called function
 */
static DataType dotToCall(AstNode *node, AstNode *parameter, AstNode *id,
                          DataType returnType, const char *name) {
    AstNode *group = createNodeGroup(parameter, 1);

    node->class = AST_CALL;
    node->type = returnType;
    node->data.call.group = group;
    node->data.call.groupCount = 1;
    node->data.call.name =
        copyString(name); // copyString so freeAst doesn't free a constant
    astFree(id);
    return returnType;
}

/**
 * @brief Semantic check of VALMEM call 'parameter.id', and node retyping
 * id can be one of 4 hard coded variants: toInt, toString, floor, strLength
 * @param symtable Pointer to the table of symbols
 * @param expression The node of the dot binary expression
 * @return The return type of the called function
 */
static DataType checkValmem(Symtable *symtable, AstNode *expression,
                            int *error) {
    AstNode *parameter = expression->data.binary.left;
    AstNode *id = expression->data.binary.right;

    if (id->class != AST_ID) {
        *error = 3;
        printError("the rightside must be an identifier");
        return TYPE_NONE;
    }

    char *name = id->data.id.name;
    DataType parameterType = checkExpr(symtable, parameter, error);
    if (*error != 0) // return from checkExpr
    {
        return TYPE_NONE;
    }

    if (strcmp(name, "toInt") == 0) {
        if (areCompatible(TYPE_DOUBLE, parameterType)) {
            return dotToCall(expression, parameter, id, TYPE_INT, "toInt");
        }
        *error = 5;
        printError("invalid argument type");
        return TYPE_NONE;
    } else if (strcmp(name, "toString") == 0) {
        if (areCompatible(TYPE_ANY, parameterType)) {
            return dotToCall(expression, parameter, id, TYPE_STRING, "toStr");
        }
        *error = 5;
        printError("invalid argument type");
        return TYPE_NONE;
    } else if (strcmp(name, "floor") == 0) {
        if (areCompatible(TYPE_DOUBLE, parameterType)) {
            return dotToCall(expression, parameter, id, TYPE_DOUBLE, "floor");
        }
        *error = 5;
        printError("invalid argument type");
        return TYPE_NONE;
    } else if (strcmp(name, "length") == 0) {
        if (areCompatible(TYPE_STRING, parameterType)) {
            return dotToCall(expression, parameter, id, TYPE_INT, "strLength");
        }
        *error = 5;
        printError("invalid argument type");
        return TYPE_NONE;
    }
    *error = 3;
    printError("invalid function call (this call isn't defined in VALMEM)");
    return TYPE_NONE;
}

/**
 * @brief Finds the data type of an expression
 * @param symtable Pointer to the table of symbols
 * @param statement Pointer to the expression to be checked
 * @return The deduced type of the expression
 */
static DataType checkExpr(Symtable *symtable, AstNode *expression, int *error) {
    switch (expression->class) {
    case AST_LITERAL:
        return expression->type;

    case AST_ID: {
        char *name = expression->data.id.name;
        SymbolData *symbol = symtableSearch(symtable, name);
        if (symbol == NULL) {
            if (strcmp(name, "true") == 0 || strcmp(name, "false") == 0) {
                expression->data.value = name;
                expression->class = AST_LITERAL;
                expression->type = TYPE_BOOL;
                return TYPE_BOOL;
            }
            *error = 3;
            printError("no such value/variable/parameterless function exists");
            return TYPE_NONE;
        }

        // a function without a parameter list gets clasified as ID ('foo')
        if (symbol->sType == FUNCTION) {
            if (symbol->value.funcData.listCount > 0) {
                *error = 5;
                printError("the parameter count should be greater than 0");
                return TYPE_NONE;
            }
            // retype the ID to a function call
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
        DataType type =
            checkExpr(symtable, expression->data.unary.operand, error);
        if (*error != 0) // return from checkExpr
        {
            return TYPE_NONE;
        }
        if (expression->data.unary.op == TOK_NOT) {
            if (type != TYPE_BOOL) {
                *error = 6;
                printError("NOT must be followed by bool type");
                return TYPE_NONE;
            }
        } else {
            if (!isNumber(type)) {
                *error = 6;
                printError("unary +/- must be followed by a number");
                return TYPE_NONE;
            }
        }
        expression->type = type;
        return type;
    }

    case AST_BINARY: {
        TokenType operator= expression->data.binary.op;

        if (operator== TOK_DOT) {
            return checkValmem(symtable, expression, error);
        }

        AstNode *left = expression->data.binary.left;
        AstNode *right = expression->data.binary.right;
        DataType leftType = checkExpr(symtable, left, error);
        if (*error != 0) // return from checkExpr
        {
            return TYPE_NONE;
        }
        DataType rightType = checkExpr(symtable, right, error);
        if (*error != 0) // return from checkExpr
        {
            return TYPE_NONE;
        }

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
            if (!(isNumber(leftType) && isNumber(rightType))) {
                *error = 6;
                printError("unallowed type in +-*/");
                return TYPE_NONE;
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
            *error = 6;
            printError("");
            return TYPE_NONE;

        case TOK_EQ:
        case TOK_NEQ:
            if ((isNumber(leftType) && isNumber(rightType)) ||
                (leftType != TYPE_NONE &&
                 leftType ==
                     rightType)) // TYPE_NONE would mean they aren't defined
            {
                expression->type = TYPE_BOOL;
                return TYPE_BOOL;
            }
            *error = 6;
            printError("");
            return TYPE_NONE;

        case TOK_AND:
        case TOK_OR:
            if (leftType == TYPE_BOOL && rightType == TYPE_BOOL) {
                expression->type = TYPE_BOOL;
                return TYPE_BOOL;
            }
            *error = 6;
            printError("AND, OR must have bool operands");
            return TYPE_NONE;

        default:
            *error = 99;
            printError("undefined binary operator");
            return TYPE_NONE;
        }
    }

    case AST_CALL: {
        SymbolData *function =
            symtableSearch(symtable, expression->data.call.name);
        if (function == NULL || function->sType != FUNCTION) {
            *error = 3;
            printError("undefined function");
            return TYPE_NONE;
        }

        if (function->value.funcData.listCount !=
            expression->data.call.groupCount) {
            *error = 5;
            printError("the number of defined parameter lists and argument "
                       "lists differs");
            return TYPE_NONE;
        }

        size_t list = 0; // the symtable uses an array for parameters

        AstNode *group = expression->data.call.group;

        // validate argument counts and types for each parameter list
        while (group != NULL) {
            if (group->data.group.count !=
                symtableParamListSize(function, list)) {
                *error = 5;
                printError("argument count mismatch");
                return TYPE_NONE;
            }

            AstNode *arg = group->data.group.items;
            for (size_t parameterIndex = 0;
                 parameterIndex < group->data.group.count; parameterIndex++) {
                const FunctionParam *parameter =
                    symtableGetParam(function, list, parameterIndex);
                DataType parameterType = parameter->dType;

                DataType argType = checkExpr(symtable, arg, error);
                if (*error != 0) // return from checkExpr
                {
                    return TYPE_NONE;
                }

                if (!areCompatible(parameterType, argType)) {
                    *error = 5;
                    printError("incompatible argument type");
                    return TYPE_NONE;
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
