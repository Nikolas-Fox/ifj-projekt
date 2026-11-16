/******************************************************************************
* Project:		IFJ
* Package:		parser
* File:			parser.h
* Date:			1.11.2026
* Last change:	15.11.2026
* Authors: 		Eliška Holá xholael00
*
* Description: Header file of parser logic
*
*******************************************************************************/
/**
* @file parser.h
*
* @brief Parser header file
* @author xholael00
*/

#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>
#include <stddef.h>
//temporary
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
    char *str; //identifikátor/řetězec
    int line;
}Token;

Token getToken();

typedef enum {
    TYPE_NONE,
    TYPE_INT,
    TYPE_STRING,
    TYPE_BOOL,
    TYPE_DOUBLE,
    TYPE_UNIT,
    TYPE_ANY,
} DataType;

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

AstNode *createNode(AstClass class);
AstNode *createNodeLiteral(DataType type, char *value);
AstNode *createNodeID(char *name);
AstNode *createNodeUnary(TokenType op, AstNode *operand);
AstNode *createNodeBinary(TokenType op, AstNode *left, AstNode *right);
AstNode *createNodeCall(char *name, size_t groupCount, AstNode *group);
AstNode *createNodeReturn(AstNode *expression);
AstNode *createNodeWhile(AstNode *condition, AstNode *body);
AstNode *createNodeIf(AstNode *condition, AstNode *thenBody, AstNode *elseBody);
AstNode *createNodeDeclaration(char *name, bool isVar, DataType declaredType, AstNode *expression);
AstNode *createNodeAssign(AstNode *destination, AstNode *expression);
AstNode *createNodeParam(char *name, DataType declaredType);
AstNode *createNodeGroup(AstNode *items, size_t count);
AstNode *createNodeFunc(char *name, size_t groupCount, AstNode *group, AstNode *body, DataType retType);
AstNode *insertStat(AstNode *head, AstNode *newStat);
AstNode *insertFunc(AstNode *head, AstNode *newFunc);
AstNode *insertGroup(AstNode *head, AstNode *newGroup, size_t *groupCount);
AstNode *insertParam(AstNode *head, AstNode *newParam);
/**********************************************************************/

//Parser state struct
typedef struct {
    Token currentToken;
    Token aheadToken;
    bool lookAhead;
} ParsedToken;

/**
 * @brief Decides if look ahead was done or if it needs to get a new token from lexer.
 * @param p Struct of parser state
 */
void getNextToken(ParsedToken *p);

/**
 * @brief Gets a new token from lexer in lookahead.
 * @param p Struct of parser state
 */
void getTokenAhead(ParsedToken *p);

//temporary precedence
AstNode *parseExpression(ParsedToken *p);

#endif
/*** End of file parser.h ***/