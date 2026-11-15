/******************************************************************************
* Project:		IFJ
* Package:		precedence
* File:			precedence.h
* Date:			5.11.2026
* Last change:	13.11.2026
* Authors: 		Nikolas Fox xfoxnik00
*
* Description: Header file for precedence syntactic analysis
*
*******************************************************************************/
/**
* @file precedence.h
*
* @brief Header file for precedence syntactic analysis
* @author Nikolas Fox xfoxnik00
*/

#ifndef PRECEDENCE_H
#define PRECEDENCE_H


// #include "parser.h"
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


// temporary from parser.h:

//Parser state struct
typedef struct {
    Token currentToken;
    Token aheadToken;
    bool lookAhead;
} ParsedToken;

Token getNextToken(ParsedToken *p);

void getTokenAhead(ParsedToken *p);

// temporary from ast

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
    DataType type; // expression result type
    AstNode *next;  // sibling in linked list (statements, arguments and parameters,
                    // groups in calls and definitions, functions)

    union
    {
        char *value; // AST_LITERAL - value

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
            AstNode *group; // poitner to group (each group has a pointer to the next group, and a pointer to a list of parameters)
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
            bool hasType;   // some types are implicit
            DataType declaredType;
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
            DataType declaredType;
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
            bool hasRetType;
            DataType retType;
            AstNode *body;  // first statement in a linked list of statements
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
 * @param hasType True if the type was stated
 * @param declaredType Stated type (only valid if hasType is true)
 * @param expression Subtree of the initialiser
 * @return Pointer to the new node
 */
AstNode *createNodeDeclaration(char *name, bool isVar, bool hasType, DataType declaredType, AstNode *expression);

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
AstNode *createNodeParam(char *name, DataType declaredType);

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
 * @param hasRetType True if the return type was written explicitly
 * @param retType Written return type (only valid if hasRetType is true)
 * @return Pointer to the new node
 */
AstNode *createNodeFunc(char *name, size_t groupCount, AstNode *group, AstNode *body, bool hasRetType, DataType retType);

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
 * @brief Inserts a group to the end of a statement list, increments the group count
 * @param head Pointer to the first statement of the list
 * @param newGroup Pointer to the statement we want to add to the end
 * @param groupCount Pointer to groupCount so I can increment
 * @return head during normal insertion and if newStat is null; newGroup if the list was empty
 */
AstNode *insertGroup(AstNode *head, AstNode *newGroup, size_t *groupCount);

/**
 * @brief Inserts a parameter to the end of a statement list
 * @param head Pointer to the first statement of the list
 * @param newParam Pointer to the statement we want to add to the end
 * @return head during normal insertion and if newParam is null; newStat if the list was empty
 */
AstNode *insertParam(AstNode *head, AstNode *newParam);
/******************************************************************************/





// data type - symbol for the precedencial analyser 
// I group some of the symbols from the precedential table,
// because they have the same priority and associativty 
typedef enum
{
    PREC_DOT,         // E.E
    PREC_SIGN,        // +P, -P
    PREC_NOT,         // !
    PREC_MUL_DIV,
    PREC_PLUS_MINUS,
    PREC_RELATION,    // >, >=, <, <=
    PREC_EQ_NE,       // ==, !=
    PREC_AND,
    PREC_OR,
    PREC_LPAR,
    PREC_RPAR,
    PREC_ID,
    PREC_END,         // $

    // these are used only in the stack (not the precTable)
    PREC_E,           // expression
    PREC_SHIFT        // <
}PrecSymbol;


// data type - element of the precedence stack
typedef struct PrecStackElem
{
    PrecSymbol symbol;
    AstNode *node;
    TokenType operator;
    struct PrecStackElem *next;
    char *str;
    DataType type;
}PrecStackElem;


// adt - precedence stack for symbols 
typedef struct 
{
    PrecStackElem *top;
    int openParCnt;    // this holds a count of how many parentheses are open
                    // it's used in the tokenToPrec function, so we know which parentheses are PREC_END
} PrecStack;


/**
 * @brief Initialises a precedence stack
 * @param s PrecStack type pointer
 */
void precStackInit(PrecStack *s);


/**
 * @brief Pushes a PrecStackElement onto stack, if it is '('  parenthese count ++;  ')' paranthese count--
 * @param s PrecStack type pointer
 * @param element PrecSymbol being pushed onto the stack (with PrecSymbol, type, string and next)
 */
void precStackPush(PrecStack *s, PrecStackElem *element);


/**
 * @brief Removes and returns first symbol on stack
 * @param s PrecStack type pointer
 * @return -1 if nothing is left on the stack, PrecStackElem otherwise
 */
PrecStackElem precStackPop(PrecStack *s);


/**
 * @brief returns top symbol on stack
 * @param s PrecStack type pointer
 * @return -1 if nothing is left on the stack, PrecSymbol otherwise
 */
PrecSymbol precStackTop(PrecStack *s);


/**
 * @brief Frees the whole PrecStack
 * @param s PrecStack type pointer
 */
void precStackFree(PrecStack *s);


/**
 * @brief Returns the first terminal on the stack
 * @param s PrecStack type pointer
 * @return -1 if error, PrecSymbol otherwise
 */
PrecSymbol precStackGetFirstTerm(PrecStack *s);


/**
 * @brief Pushes Shift PrecSymbol onto the stack after the first terminal
 * @param s PrecStack type pointer
 */
void precStackShiftAfterFirstTerm(PrecStack *s);


/**
 * @brief returns the PrecSymbol equivalent of a token
 * @param p ParsedToken
 * @param previousWasOperand Keeps track of what the last token was (checking +- unary or binary)
 * @param s Pointer to stack (used to track open parenthese count)
 * @param node pointer to node we want to change
 * @return PrecSymbol equivalent of token
 */
PrecSymbol tokenToPrec(ParsedToken *p, bool previousWasOperand, PrecStack *s, AstNode **node);


/**
 * @brief Reduces rules on the stack
 * @param s Pointer to PrecStack
 */
void precReduce(PrecStack *s);


/**
 * @author Eliška Holá (xholael00) & Nikolas Fox
 * @brief Parses <loa>, implements LL grammar rules 46
 * @param p Struct of parser state
 * @param groupCount Pointer to the number of groups, incremented with each new group
 */
AstNode *parseLoa(ParsedToken *p, size_t *groupCount);


/**
 * @author Eliška Holá (xholael00) & Nikolas Fox (xfoxnik00)
 * @brief Parses <arg_list>, implements LL grammar rules 48 and 49.
 * @param p Struct of parser state
 * @param count Pointer to number of arguments, incremented with each argument
 * @return Pointer to the first argument in linked list
 */
AstNode *parseArgList(ParsedToken *p, size_t *count);


/**
 * @author Eliška Holá (xholael00) & Nikolas Fox (xfoxnik00)
 * @brief Parses <arg_end>, implements LL grammar rules 50 and 51.
 * @param p Struct of parser state
 * @param count Pointer to the number of arguments (given to parseArgList for the next argument)
 * @return Pointer to the following argument in linked list
 */
AstNode *parseArgEnd(ParsedToken *p, size_t *count);


/**
 * @brief Uses precedence analysis to validate expression
 * @param p Pointer to the parsed token
 */
void parseExpression(ParsedToken *p);


#endif   // PRECEDENCE_H

/*** End of file precedence.h ***/