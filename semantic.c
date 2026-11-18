/******************************************************************************
* Project:		IFJ
* Package:		semantic
* File:			semantic.c
* Date:			16.11.2026
* Last change:	18.11.2026
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


#include <stddef.h>
#include <stdbool.h>

// TEMPORARY: /******************************************************** */
#define INITIAL_CAPACITY 16

typedef enum {
    TYPE_NONE,
    TYPE_INT,
    TYPE_STRING,
    TYPE_BOOL,
    TYPE_DOUBLE,
    TYPE_UNIT,
    TYPE_ANY,
} DataType;

typedef enum {
    VAL,
    VAR,
    FUNCTION,
} SymbolType;

typedef struct FunctionParam {
    char *paramName;
    DataType dType;
    struct FunctionParam *next;
} FunctionParam;

typedef struct {
    FunctionParam *params;
    size_t paramCount;
} FunctionData;

typedef struct {
    SymbolType sType;
    char *symbolName;
    DataType dtype;
    bool isDefined;
    int scope;     // ADDED (NEED TO TELL SAŠA) - I also need to do newData->scope = -1 during insertSymbol

    union {
        FunctionData funcData;
    } value;
} SymbolData;

// AI: (claude) - help with the structure for Built in Functions for global
// scope
typedef struct {
    const char *name;
    DataType ret;
    int paramCount;
    DataType params[3];
    const char *paramNames[3];
} BuiltinDef;

typedef struct Symtable Symtable;

/**
 * @brief Initializes the symbol table (Frame Stack) and allocates the global
 * scope.
 *
 * @return Pointer to created Symtable, or NULL if allocation failure.
 */
Symtable *symtableInit(void){}

/**
 * @brief Pushes a new scope (block) onto the symbol table stack.
 * Automatically expands the  array capacity using realloc if necessary.
 *
 * @param sTable Pointer to the Symtable structure.
 * @return true on success
 * @return false on memory allocation failure
 */
bool symtablePushScope(Symtable *sTable){}

/**
 * @brief Pop a  scope out of  the symbol table stack.
 *
 * @param sTable Pointer to the Symtable structure.
 */
void symtablePopScope(Symtable *sTable){}

/**
 * @brief Free symtable, pop all the scopes
 *
 * @param sTable Pointer to the Symtable structure.
 */
void symtableFree(Symtable *sTable){}

/**
 * @brief Insert Variable to the symtable
 *
 * @param sTable Pointer to the Symtable structure.
 * @param name name of the variable
 * @param dType Type of data i am sending (int, string...)
 * @param isDefinied true or false if the variable has already been definied
 * @return pointer to data that has been inserted
 */
SymbolData *symtableInsertVar(Symtable *sTable, char *name, DataType dType,
                              bool isDefined){}

/**
 * @brief Insert Val to the symtable
 *
 * @param sTable Pointer to the Symtable structure.
 * @param name name of the val
 * @param dType Type of data i am sending (int, string...)
 * @param isDefinied true or false if the variable has already been definied
 * @return pointer to data that has been inserted
 */
SymbolData *symtableInsertVal(Symtable *sTable, char *name, DataType dType,
                              bool isDefined){}

/**
 * @brief Insert Function to the symtable
 *
 * @param sTable Pointer to the Symtable structure.
 * @param name name of the function
 * @param dType Type of data i am sending (int, string...)
 * @param isDefinied true or false if the function has already been definied
 * @return pointer to data that has been inserted
 */
SymbolData *symtableInsertFunction(Symtable *sTable, char *name,
                                   DataType returnType, bool isDefined){}

/**
 * @brief Insert parameters of the function
 *
 * @param funcSymbol Pointer to the function
 * @param paramName name of the parameter
 * @param paramType Type of the param
 * @return true/false
 */
bool symtableAddParam(Symtable *sTable, SymbolData *funcSymbol, char *paramName,
                      DataType paramType){}

/**
 * @brief Search through all scopes
 *
 * @param sTable Pointer to the Symtable structure.
 * @param name name of the function
 * @return pointer to data
 */
SymbolData *symtableSearch(Symtable *sTable, char *name){}

/**
 * @brief Search through current scope
 *
 * @param sTable Pointer to the Symtable structure.
 * @param name name of the function
 * @return pointer to data
 */
SymbolData *symtableSearchCurrentScope(Symtable *sTable, char *name){}

//TEMPORARY


typedef TokenType;
typedef enum
{
    AST_WHOLE, // the whole program - a linked list of functions

    AST_FUNC, // function definition
    AST_GROUP, // group of parameters/arguments (a,b) - they will be in a linked list
    AST_PARAM, // one parameter - parameters will be in a linked list

    AST_ASSIGN,
    AST_DECLARATION,
    AST_IF,
    AST_WHILE,
    AST_RETURN,

    AST_CALL,
    AST_BINARY, // operators
    AST_UNARY,
    AST_LITERAL,
    AST_ID
} AstClass;

typedef struct AstNode AstNode; // declared so AstNode can keep a pointer to itself


struct AstNode
{
    AstClass class;
    DataType type; // The type of the nodes value (literal, expression, variable, function return type) 0 until it is set
    AstNode *next;  // sibling in linked list (statements, arguments and parameters,
                    // groups in calls and definitions, functions)

    union
    {
        char *value; // AST_LITERAL - value (NULL for unit)

        struct
        {
            char *name;
            int scope;
        }id;

        struct
        {
            TokenType op;
            AstNode *operand;
        }unary;

        struct 
        {
            TokenType op;
            AstNode *left;   // operands
            AstNode *right;
        }binary;

        struct
        {
            char *name;
            AstNode *group; // poitner to group (each group has a pointer to the next group, and a pointer to a list of arguments)
            size_t groupCount;
        }call;

        struct
        {
            AstNode *expression;
        }returnStat;

        struct 
        {
            AstNode *condition;
            AstNode *body; // first statement in a linked list of statements
        }whileStat;

        struct
        {
            AstNode *condition;   // expression
            AstNode *thenBody;   // first statement in linked list of statements
            AstNode *elseBody; // first statement in linked list of statements
        }ifStat;

        struct
        {
            char *name;
            bool isVar;     // to tell between variable and value
            AstNode *expression;
            int scope;      // number of scope we are in (so codegen knows which variable is used); not depth
        }declaration;

        struct 
        {
            AstNode *destination;
            AstNode *expression;
        }assign;

        struct
        {
            char *name;
            int scope;
        }param;

        struct
        {
            AstNode *items; // pointer to the first parameter/ argument in a linked list
            size_t count; // size of the group for checking whether there is the right number of parameters/arguments
        }group;

        struct
        {
            char *name;
            AstNode *group; // poitner to group (each group has a pointer to the next group, and a pointer to a list of parameters)
            size_t groupCount;
            AstNode *body;  // first statement in a linked list of statements
            bool isMain;
        } func;

        struct
        {
            AstNode *func; // pointer to first function (it is a linked list of functions)
        }whole;        
    }data;
};
/****************************************************************************************** */






#include "semantic.h"

void semanticAnalysis(AstNode *whole)
{
    Symtable *symtable = symtableInit();
    stInsertFunctions(symtable, whole);
    checkFunctionList(symtable, whole); // this will pass into every function and then check the functions statements...
    symtableFree(symtable);
}

bool hasParams(AstNode *group)
{
    while(group != NULL)
    {
        if(group->data.group.count > 0) // the group has a parameter
        {
            return true;
        }
        group = group->next;
    }
    return false; // if we get to the end then it doesn't have a parameter
}


bool areCompatible(DataType destination, DataType source)
{
    if(destination == source || destination == TYPE_DOUBLE && source == TYPE_INT || destination == TYPE_ANY)
    {
        return true;
    }
    return false;
}

bool isNumber(DataType type)
{
    if(type == TYPE_INT || type == TYPE_DOUBLE)
    {
        return true;
    }
    return false;
}
void stInsertFunctions(Symtable *symtable, AstNode *whole)
{
    size_t mainCount = 0;
    AstNode *node = whole->data.whole.func;

    while(node != NULL)
    {
        bool hasType = (node->type != TYPE_NONE);

        SymbolData *function = symtableInsertFunction(symtable, node->data.func.name, node->type, hasType);
        AstNode *group = node->data.func.group;

        // inserting the parameters into the symtable (first activate a list - group, and then insert individual parameters)
        while(group != NULL)
        {
            symtableStartParamList(function);
            AstNode *param = group->data.group.items;    // node->data.group.items is the pointer to the first parameter in the group
            while(param != NULL)
            {
                symtableAddParam(symtable, function, param->data.param.name, param->type);
                param = param->next;
            }
            group = group->next;
        }

        if(node->data.func.isMain == true)
        {
            mainCount++;
            if(hasParams(node->data.func.group) == true)
            {
                exit(5);
            }
            if(node->type != TYPE_UNIT && hasType)
            {
                exit(6);
            }
        }
        node = node->next;
    }
    
    if(mainCount == 0)
    {
        exit(3);
    }
    else
    if (mainCount > 1)
    {
        exit(4);
    }
}


static void checkFunctionList(Symtable *symtable, AstNode *whole)
{
    AstNode *function = whole->data.whole.func;
    int scopeId = 0;
    while(function != NULL)
    {
        checkFunction(symtable, function, &scopeId);
        function = function->next;
    }
}

static void checkFunction(Symtable *symtable, AstNode *function, int *scopeId)
{
    symtablePushScope(symtable);
    (*scopeId)++;
    // parameters of the functions will have the next scope
    AstNode *group = function->data.func.group;
    while(group != NULL)
    {
        AstNode *param = group->data.group.items;
        while(param != NULL)
        {
            param->data.param.scope = scopeId;
            symtableInsertVal(symtable, param->data.param.name, param->type, true);

            param = param->next;
        }
        group = group->next;
    }

    checkStatementList(symtable, function->data.func.body, scopeId, function->type); // I send function type as well to check compatibility (0 means implicit)

    symtablePopScope(symtable);
}

static void checkStatementList(Symtable *symtable, AstNode *statement, int *scopeId, DataType retType)
{
    if(statement == NULL)
    {
        return;
    }

    symtablePushScope(symtable);
    while(statement != NULL)
    {
        checkStatement(symtable, statement, scopeId, retType);
        statement = statement->next;
    }
    symtablePopScope(symtable);
}

static void checkStatement(Symtable *symtable, AstNode *statement, int *scopeId, DataType retType)
{
    switch(statement->class)
    {
        case AST_ASSIGN:
        {
            AstNode *destination = statement->data.assign.destination;
            SymbolData *symbol = symtableSearch(symtable, destination->data.id.name);

            if(symbol == NULL || symbol->sType == FUNCTION)
            {
                exit(3);
            }

            if(symbol->sType == VAL)    // you can't assign a value or parameter of a function
            {
                exit(7);
            }

            DataType destinationType = symbol->sType;
            DataType expressionType = checkExpr(symtable, statement->data.assign.expression, scopeId);

            if(areCompatible(destinationType, expressionType) == false)
            {
                exit(6); // error of type compatibility
            }
            return;
        }
        
        case AST_DECLARATION:
        {
            DataType expressionType = checkExpr(symtable, statement->data.declaration.expression, scopeId);
            if(statement->type == TYPE_NONE) // statement->type is the explicit type of the variable/value
            {
                if(expressionType == TYPE_NONE)
                {
                    exit(6);  // error of inability of taking on implicit type
                }
                statement->type = expressionType;  // the declared value/variable takes on the expression type
            }
            else
            {
                if(areCompatible(statement->type, expressionType) == false)
                {
                    exit(6); // error of type compatibility
                }
            }

            if(statement->data.declaration.isVar)
            {
                symtableInsertVar(symtable, statement->data.declaration.name, statement->type, true);
            }
            else
            {
                symtableInsertVal(symtable, statement->data.declaration.name, statement->type, true);
            }
            statement->data.declaration.scope = scopeId;

            if(statement->type == TYPE_NONE)
            {
                statement->type = checkExpr(symtable, statement->data.declaration.expression, scopeId);
            }
            return;
        }

        case AST_IF:
        {
            AstNode *condition = statement->data.ifStat.condition;
            if(checkExpr(symtable, condition, scopeId) != TYPE_BOOL)
            {
                exit(6); // incompatible condtion type (isn't bool)
            }
            
            (*scopeId)++;  // incrementing the scopeId before entering a new block
            checkStatementList(symtable, statement->data.ifStat.thenBody, scopeId);
            
            if(statement->data.ifStat.elseBody != NULL)
            {
                (*scopeId)++;
                checkStatementList(symtable, statement->data.ifStat.elseBody, scopeId);
            }
            return;
        }

        case AST_WHILE:
        {
            AstNode *condition = statement->data.ifStat.condition;
            if(checkExpr(symtable, condition, scopeId) != TYPE_BOOL)
            {
                exit(6); // incompatible condtion type (isn't bool)
            }

            (*scopeId++);
            checkStatementList(symtable, statement->data.whileStat.body, scopeId);
            return;
        }

        case AST_RETURN:
        {
            AstNode *expression = statement->data.returnStat.expression;
            if(retType == TYPE_NONE)
            {
                exit(6); // you can't have return if no retType is stated
            }
            if(expression == NULL)
            {
                if(retType != TYPE_UNIT)
                {
                    exit(6); // if there is just return; then the return type must be TYPE_UNIT
                }
                return;
            }
            if(!areCompatible(retType, checkExpr(symtable, expression, scopeId)) && retType != TYPE_UNIT)  // if retType is TYPE_UNIT the result is disregarded (but valid)
            {
                exit(6); // type incompatibility
            }
            return;
        }
        
        default:  // this is just a bare expression
            checkExpr(symtable, statement, scopeId);
            return;
    }
}


static DataType checkExpr(Symtable *symtable, AstNode *expression, int *scopeId)
{
    switch(expression->class)
    {
        case AST_LITERAL:
            return expression->type;

        case AST_ID:
        {
            char *name = expression->data.id.name;
            SymbolData *symbol = symtableSearch(symtable, name);
            if(symbol == NULL)
            {
                if(strcmp(name, "true") == 0 || strcmp(name, "false") == 0)
                {
                    expression->class = AST_LITERAL;
                    expression->type = TYPE_BOOL;
                    return TYPE_BOOL;
                }

                exit(3); // no such value/variable exists
            }
            expression->type = symbol->dtype;
            return expression->type;
        }

        case AST_UNARY:
        {
            DataType type = checkExpr(symtable, expression->data.unary.operand, scopeId);
            if(expression->data.unary.op == TOK_NOT)
            {
                if(type != TYPE_BOOL)
                {
                    exit(6); // NOT must be followed by bool type
                }
            }
            // everything else is sign (so it must be followed by int or double)
            if(isNumber(type) == false)
            {
                exit(6);
            }
            expression->type = type;
            return type;
        }

        case AST_BINARY:
        {
            DataType left = checkExpr(symtable, expression->data.binary.left, scopeId);
            DataType right = checkExpr(symtable, expression->data.binary.right, scopeId);
            TokenType operator = expression->data.binary.op;

            switch(operator)
            {
                case TOK_PLUS:
                    if(left == TYPE_STRING && right == TYPE_STRING)
                    {
                        expression->type = TYPE_STRING;
                        return TYPE_STRING;
                    }
                // FALLTHROUGH
                case TOK_MINUS:
                case TOK_DIV:
                case TOK_MUL:
                {
                    if((isNumber(left) && isNumber(right)) == false)
                    {
                        exit(6); // unallowed type in +-*/
                    }
                    if(left == TYPE_INT && right == TYPE_INT)
                    {
                        expression->type = TYPE_INT;
                        return TYPE_INT;
                    }
                    else  // at least one of them is TYPE_DOUBLE
                    {
                        expression->type = TYPE_DOUBLE;
                        return TYPE_DOUBLE;
                    }
                }

                case TOK_LESS:
                case TOK_LESS_EQ:
                case TOK_GREATER:
                case TOK_GREATER_EQ:
                    if((isNumber(left) && isNumber(right)) || (left == TYPE_STRING && right == TYPE_STRING))
                    {
                        expression->type = TYPE_BOOL;
                        return TYPE_BOOL;
                    }
                    exit(6); 

                case TOK_EQ:
                case TOK_NEQ:
                    if((isNumber(left) && isNumber(right)) || left == right)
                    {
                        expression->type = TYPE_BOOL;
                        return TYPE_BOOL;
                    }
                    exit(6);

                case TOK_AND:
                case TOK_OR:
                    if(left == TYPE_BOOL && right == TYPE_BOOL)
                    {
                        expression->type = TYPE_BOOL;
                        return TYPE_BOOL;
                    }
                    exit(6);

                // TODO: case TOK_DOT
                    
        }
    }
}
/*** End of file semantic.c ***/