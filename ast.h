/******************************************************************************
* Project:		IFJ
* Package:		ast
* File:			ast.h
* Date:			12.11.2026
* Last change:	18.11.2026
* Authors: 		Nikolas Fox xfoxnik00
*
* Description: Header file containing abstract syntactic tree logic
*
*******************************************************************************/
/**
* @file ast.h
*
* @brief Header file containing abstract syntactic tree logic
* @author Nikolas Fox xfoxnik00
*/

#ifndef AST_H
#define AST_H


// #include "scanner.h"
// #include "symtable.h"


/******************************************************************************/
// temporary from scanner.h
typedef enum {
    TOK_EOF, TOK_NL, TOK_INDENT, TOK_DEDENT,
    TOK_ID, TOK_KEYWORD,
    TOK_INT, TOK_DOUBLE, TOK_STRING,

    //tokens for operators
    TOK_PLUS, TOK_MINUS, TOK_MUL, TOK_DIV,
    TOK_LESS, TOK_LESS_EQ, TOK_GREATER, TOK_GREATER_EQ,
    TOK_EQ, TOK_NEQ, TOK_ASSIGN,

    //tokens for separators
    TOK_LPAR, TOK_RPAR, TOK_COMMA, TOK_COLON, TOK_SEMICOL,
    TOK_DOT, TOK_AT_MAIN,

    //tokens for extentions
    TOK_AND, TOK_OR, TOK_NOT, TOK_PLUSPLUS,

    TOK_KW_DEF, TOK_KW_DO, TOK_KW_DOUBLE, TOK_KW_ELSE, TOK_KW_IF, TOK_KW_IMPORT,
    TOK_KW_INT, TOK_KW_RETURN, TOK_KW_STRING, TOK_KW_THEN, TOK_KW_UNIT, TOK_KW_VAL,
    TOK_KW_VAR, TOK_KW_WHILE, TOK_KW_EXTENSION, TOK_KW_BOOLEAN
}TokenType;


typedef struct {
    TokenType type;
    char *str; // identifier/string
    int line;
}Token;


// temporary from symtable.h:

#include <stdbool.h>
#include <stddef.h> // for size_t
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

    union {
        FunctionData funcData;
    } value;
} SymbolData;

typedef struct Symtable Symtable;

SymbolData *symtableSearch(Symtable *sTable, char *name);

static char *copyString(const char *src);

/*********************************************************/

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

typedef enum
{
    NOT_VISITED,
    PROCESSING,
    DONE
} State;

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
            State state; 
        } func;

        struct
        {
            AstNode *func; // pointer to first function (it is a linked list of functions)
        }whole;        
    }data;
};


/**
 * @brief Creates AST node of certain class
 * @param class The class of the created node
 * @return it returns a pointer to the new node
 */
AstNode *createNode(AstClass class);

/**
 * @brief Creates a literal node
 * @param type the type of the literal
 * @param value the value of the literal
 * @return Pointer to the new node
 */
AstNode *createNodeLiteral(DataType type, char *value);

/**
 * @brief Creates an identifier node, a use of a variable or a function without parameters
 * @param name Name of the identifier (copied)
 * @return Pointer to the new node
 */
AstNode *createNodeID(char *name);

/**
 * @brief Creates a unary operator node
 * @param op Operator token type
 * @param operand Subtree of the operand
 * @return Pointer to the new node
 */
AstNode *createNodeUnary(TokenType op, AstNode *operand);

/**
 * @brief Creates a binary operator node
 * @param op Operator token type
 * @param left Subtree of the left operand
 * @param right Subtree of the right operand
 * @return Pointer to the new node
 */
AstNode *createNodeBinary(TokenType op, AstNode *left, AstNode *right);

/**
 * @brief Creates a function call node
 * @param name Name of the called function
 * @param group First AST_GROUP of arguments (groups form a linked list)
 * @param groupCount Number of argument groups
 * @return Pointer to the new node
 */
AstNode *createNodeCall(char *name, size_t groupCount, AstNode *group);

/**
 * @brief Creates a return statement node
 * @param expression Returned expression
 * @return Pointer to the new node
 */
AstNode *createNodeReturn(AstNode *expression);

/**
 * @brief Creates a while loop node
 * @param condition Subtree of the loop condition
 * @param body First statement of the loop body (statements form a linked list)
 * @return Pointer to the new node
 */
AstNode *createNodeWhile(AstNode *condition, AstNode *body);

/**
 * @brief Creates a conditional statement node
 * @param condition Subtree of the condition
 * @param thenBody First statement of the then branch (statements form a linked list)
 * @param elseBody First statement of the else branch
 * @return Pointer to the new node
 */
AstNode *createNodeIf(AstNode *condition, AstNode *thenBody, AstNode *elseBody);

/**
 * @brief Creates a variable declaration node
 * @param name Name of the variable
 * @param isVar True for var, false for val
 * @param type Declared type (0 if it's not declared)
 * @param expression Subtree of the initialiser
 * @return Pointer to the new node
 */
AstNode *createNodeDeclaration(char *name, bool isVar, DataType type, AstNode *expression);

/**
 * @brief Creates an assignment node
 * @param destination AST_ID node of the assigned variable
 * @param expression Subtree of the assigned value
 * @return Pointer to the new node
 */
AstNode *createNodeAssign(AstNode *destination, AstNode *expression);

/**
 * @brief Creates a formal parameter node
 * @param name Name of the parameter
 * @param declaredType Type of the parameter
 * @return Pointer to the new node
 */
AstNode *createNodeParam(char *name, DataType type);

/**
 * @brief Creates a group node
 * @param items First item of the list (items create a linked list)
 * @param count Number of items in the list
 * @return Pointer to the new node
 */
AstNode *createNodeGroup(AstNode *items, size_t count);

/**
 * @brief Creates a function definition node
 * @param name Name of the function
 * @param groupCount Number of parameter groups
 * @param group First AST_GROUP of parameters
 * @param body First statement of the body (statements create a linked list)
 * @param retType Written return type (0 if it is not stated)
 * @return Pointer to the new node
 */
AstNode *createNodeFunc(char *name, size_t groupCount, AstNode *group, AstNode *body, DataType retType);

/**
 * @brief Inserts a statement to the end of a statement list
 * @param head Pointer to the first statement of the list
 * @param newStat Pointer to the statement we want to add to the end
 * @return head during normal insertion and if newStat is null; newStat if the list was empty
 */
AstNode *insertStat(AstNode *head, AstNode *newStat);

/**
 * @brief Inserts a function to the end of a statement list
 * @param head Pointer to the first statement of the list
 * @param newFunc Pointer to the statement we want to add to the end
 * @return head during normal insertion and if newFunc is null; newFunc if the list was empty
 */
AstNode *insertFunc(AstNode *head, AstNode *newFunc);

/**
 * @brief Free ast tree using recursion
 * @param node Pointer to the node which should be freed
 */
void astFree(AstNode *node);

/**
 * @brief Free a linked list
 * @param node Pointer to the first element of a linked list (head)
 */
void astFreeList(AstNode *node);

#endif
/*** End of file ast.h ***/