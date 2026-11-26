/******************************************************************************
 * Project:		IFJ
 * Package:		semantic
 * File:			semantic.c
 * Date:			16.11.2026
 * Last change:	25.11.2026
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
static void findCalls(Symtable *, AstNode *, AstNode *);
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

/**
 * @brief Pass through groups of parameters and checks whether there is at least one parameter
 * @param group Pointer to the first parameter group in a linked list of parameter groups
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

        if (node->data.func.isMain) {
            mainCount++;
            if (hasParams(node->data.func.group)) {
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

/**
 * @brief Resolves implicit return types of a function recursively and type-checks the function
 * 
 * I mapped functions to have three states: NOT_VISITED, PROCESSING, DONE to deduce dependency of functions
 * If a cyclic dependency is found -> exit(6)
 * @param symtable Pointer to the symtable
 * @param whole Pointer to the first function (root of the tree)
 * @param function Pointer to the function to be visited
 */
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
    findCalls(symtable, whole, function->data.func.body);
    checkFunction(symtable, function);
    function->data.func.state = DONE;
}

/**
 * @brief Searches through a function linked list and returns a pointer to the function with the specified name
 * @param whole Pointer to the first function in a linked list of functions
 * @param name The name of the function to look for
 * @return function Pointer to the function with the specified name / NULL if it doesn't exist
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
static void findCalls(Symtable *symtable, AstNode *whole, AstNode *statement) {
    while (statement != NULL) {
        switch (statement->class) {
        case AST_CALL: {
            AstNode *calledFunction =
                findFunction(whole, statement->data.call.name); // built in functions are skipped (we already know their return type)
            if (calledFunction != NULL && calledFunction->type == TYPE_NONE) {
                visit(symtable, whole, calledFunction);
            }

            AstNode *group = statement->data.call.group;
            while (group != NULL) // finding function calls in parameters
            {
                findCalls(symtable, whole, group->data.group.items);
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
            findCalls(symtable, whole, statement->data.assign.expression);
            break;

        case AST_DECLARATION:
            findCalls(symtable, whole, statement->data.declaration.expression);
            break;

        case AST_RETURN:
            findCalls(symtable, whole, statement->data.returnStat.expression);
            break;

        case AST_IF:
            findCalls(symtable, whole, statement->data.ifStat.condition);
            findCalls(symtable, whole, statement->data.ifStat.thenBody);
            findCalls(symtable, whole, statement->data.ifStat.elseBody);
            break;

        case AST_WHILE:
            findCalls(symtable, whole, statement->data.whileStat.condition);
            findCalls(symtable, whole, statement->data.whileStat.body);
            break;

        case AST_UNARY:
            findCalls(symtable, whole, statement->data.unary.operand);
            break;

        case AST_BINARY:
            findCalls(symtable, whole, statement->data.binary.left);

            if(statement->data.binary.op != TOK_DOT) // I need to guard the right call, because it won't be a function in the symtable
            {
                findCalls(symtable, whole, statement->data.binary.right);
            }
            break;

        default:
            break;
        }
        statement = statement->next;
    }
}

/**
 * @brief Visits every function in a linked list of functions (visit skips over already visited functions)
 * @param symtable Pointer to the table of symbols
 * @param whole Pointer to the first function in a linked list of functions
 */
static void checkFunctionList(Symtable *symtable, AstNode *whole) {
    AstNode *function = whole->data.whole.func;
    while (function != NULL) {
        visit(symtable, whole, function);
        function = function->next;
    }
}

/**
 * @brief Inserts all parameters of a function into the symtable (helper for checkFunction)
 * @param symtable Pointer to the table of symbols
 * @param function Pointer to the function whose parameters should be inserted
 */
static void stInsertParameters(Symtable *symtable, AstNode *function)
{
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
}

/**
 * @brief Semantic check of a function
 * @param symtable Pointer to the table of symbols
 * @param function Pointer to the function to be checked
 */
static void checkFunction(Symtable *symtable, AstNode *function) 
{
    symtablePushScope(symtable);
    stInsertParameters(symtable, function);

    DataType blockType = checkStatementList(symtable, function->data.func.body, function->type); // function->type to check compatibility
    symtablePopScope(symtable);

    if (function->type == TYPE_NONE) // TYPE_NONE means return type is inferred
    {
        if (blockType == TYPE_NONE) 
        {
            exit(6); // function type can't be deduced, return type is inferred and the body doesn't have a type
        }

        if (function->data.func.isMain && blockType != TYPE_UNIT) {
            exit(6); // deduced main type is not TYPE_UNIT
        }

        function->type = blockType; // updating AST with deduced function type

        SymbolData *functionSymbol =
            symtableSearch(symtable, function->data.func.name);
        functionSymbol->dtype =
            blockType; // updating symtable with function type
    } else {
        if (function->type != TYPE_UNIT) {
            if ( !areCompatible(function->type, blockType) &&
                blockType != TYPE_RETURNED) {
                exit(6); // the block type isn't compatible with the return type
            }
        }
    }
}

/**
 * @brief Finds the data type of a linked list of statements
 * If no return is found then the type is inferred from the last statement
 * @param symtable Pointer to the table of symbols
 * @param statement Pointer to the first statement in a linked list of statements
 * @param retType The functions declared return type (TYPE_NONE if it relies on inferrence)
 */
static DataType checkStatementList(Symtable *symtable, AstNode *statement,
                                   DataType retType) {
    DataType last = TYPE_UNIT;
    while (statement != NULL) {
        last = checkStatement(symtable, statement, retType);
        statement = statement->next;
    }
    return last; // the type of the last statement of a block (it must correspond with the declared return type)
}

/**
 * @brief Finds the data type of a statements
 * ASSIGN, DEFINITION and LOOPS are TYPE_UNIT
 * @param symtable Pointer to the table of symbols
 * @param statement Pointer to the statement to be checked
 * @param retType The functions declared return type (TYPE_NONE if it relies on inferrence)
 * @return The deduced type of the statement
 */
static DataType checkStatement(Symtable *symtable, AstNode *statement,
                               DataType retType) {
    switch (statement->class) {
    case AST_ASSIGN: {
        AstNode *destination = statement->data.assign.destination;
        AstNode *expression = statement->data.assign.expression;
        SymbolData *symbol =
            symtableSearch(symtable, destination->data.id.name);

        if (symbol == NULL || symbol->sType == FUNCTION) {
            exit(3); // the destination isn't defined yet or the destination is a function
        }

        if (symbol->sType == VAL) 
        {
            exit(7); // assigning a value to a VAL or parameter is forbidden
        }

        destination->type = symbol->dtype;
        expression->type = checkExpr(symtable, expression);

        if (!areCompatible(destination->type, expression->type)) {
            exit(6); // error of type compatibility in assign
        }

        destination->data.id.scope = symbol->scopeID;
        return TYPE_UNIT;
    }

    case AST_DECLARATION: {
        DataType expressionType =
            checkExpr(symtable, statement->data.declaration.expression);
        // statement->type represents the variables declared type
        if (statement->type == TYPE_NONE) // inferred type
        {
            if (expressionType == TYPE_NONE) {
                exit(6); // type cannot be inferred
            }
            statement->type = expressionType; // the declared value/variable
                                              // acquires the expression type
        } else {
            if (!areCompatible(statement->type, expressionType)) {
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
            TYPE_UNIT; // undefined else body defaults to TYPE_UNIT

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

        return TYPE_NONE; // this is only valid if the retType is TYPE_UNIT
    }

    case AST_RETURN: {
        AstNode *expression = statement->data.returnStat.expression;
        if (retType == TYPE_NONE) {
            exit(6); // return is forbidden if the function type is inferred, changed exit code from 10 to 6 due to test case 137 and 138 in ifj26-pokusne
        }
        if (expression == NULL) { // a blank 'return' defaults to TYPE_UNIT 
            if (retType != TYPE_UNIT) {
                exit(6);
            }
            return TYPE_RETURNED; // TYPE_RETURNED means that the keyword RETURN was used
        }

        DataType expressionType = checkExpr(symtable, expression);

        // if retType is TYPE_UNIT the result is disregarded (but valid)
        if (retType != TYPE_UNIT && !areCompatible(retType, expressionType))
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

/**
 * @brief Converts an AST_BINARY node:'parameter.id' to AST_CALL node'id(parameter)'
 * @param node The node that will be retyped
 * @param parameter The left side of the call
 * @param id The right side of the call
 * @param returnType Return type of the called function
 * @param name The name of the corresponding built-in function
 * @return The return type of the called function
 */
static DataType dotToCall(AstNode *node, AstNode *parameter, AstNode *id, DataType returnType, const char *name)
{
    AstNode *group = createNodeGroup(parameter, 1);

    node->class = AST_CALL;
    node->type = returnType;
    node->data.call.group = group;
    node->data.call.groupCount = 1;
    node->data.call.name = copyString(name); // copyString so freeAst doesn't free a constant
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
static DataType checkValmem(Symtable *symtable, AstNode *expression)
{
    AstNode *parameter = expression->data.binary.left;
    AstNode *id = expression->data.binary.right;

    if(id->class != AST_ID)
    {
        exit(3); // the rightside must be an identifier
    }

    char *name = id->data.id.name;
    DataType parameterType = checkExpr(symtable, parameter);

    if(strcmp(name, "toInt") == 0)
    {
        if(areCompatible(TYPE_DOUBLE, parameterType))
        {
            return dotToCall(expression, parameter, id, TYPE_INT, "toInt");
        }
        exit(5); // invalid argument type 
    } 
    else if(strcmp(name, "toString") == 0)
    {
        if(areCompatible(TYPE_ANY, parameterType))
        {
            return dotToCall(expression, parameter, id, TYPE_STRING, "toStr");
        }
        exit(5); // invalid argument type 
    }
    else if(strcmp(name, "floor") == 0)
    {
        if(areCompatible(TYPE_DOUBLE, parameterType))
        {
            return dotToCall(expression, parameter, id, TYPE_DOUBLE, "floor");
        }
        exit(5); // invalid argument type 
    }
    else if(strcmp(name, "length") == 0)
    {
        if(areCompatible(TYPE_STRING, parameterType))
        {
            return dotToCall(expression, parameter, id, TYPE_INT, "strLength");
        }
        exit(5); // invalid argument type 
    }
    exit(3); // invalid function call (this call isn't defined in VALMEM)
}



/**
 * @brief Finds the data type of an expression
 * @param symtable Pointer to the table of symbols
 * @param statement Pointer to the expression to be checked
 * @return The deduced type of the expression
 */
static DataType checkExpr(Symtable *symtable, AstNode *expression) {
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
            exit(3); // no such value/variable/ parameterless function exists
        }

        // a function without a parameter list gets clasified as ID ('foo')
        if (symbol->sType == FUNCTION)
        {
            if (symbol->value.funcData.listCount >0)
            {
                exit(5); // the parameter count should be greater than 0
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
        DataType type = checkExpr(symtable, expression->data.unary.operand);
        if (expression->data.unary.op == TOK_NOT) {
            if (type != TYPE_BOOL) {
                exit(6); // NOT must be followed by bool type
            }
        } else {
            if (!isNumber(type))
            {
                exit(6); // unary +- must be followed by a number
            }
        }
        expression->type = type;
        return type;
    }

    case AST_BINARY: {
        TokenType operator= expression->data.binary.op;

        if(operator == TOK_DOT)
        {
            return checkValmem(symtable, expression);
        }

        AstNode *left = expression->data.binary.left;
        AstNode *right = expression->data.binary.right;
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
            if (!(isNumber(leftType) && isNumber(rightType))) {
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
            exit(6); // AND, OR must have bool operands

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
            exit(5); // the number of defined parameter lists and argument lists differs
        }

        size_t list = 0; // the symtable uses an array for parameters

        AstNode *group = expression->data.call.group;
        
        // validate argument counts and types for each parameter list
        while (group != NULL) {
            if (group->data.group.count !=
                symtableParamListSize(function, list)) {
                exit(5); // argument count mismatch
            }

            AstNode *arg = group->data.group.items;
            for (size_t parameterIndex = 0;
                 parameterIndex < group->data.group.count; parameterIndex++) {
                const FunctionParam *parameter =
                    symtableGetParam(function, list, parameterIndex);
                DataType parameterType = parameter->dType;

                DataType argType = checkExpr(symtable, arg);

                if (!areCompatible(parameterType, argType)) {
                    exit(5); // incompatible argument type
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
