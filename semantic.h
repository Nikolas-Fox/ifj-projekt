/******************************************************************************
* Project:		IFJ
* Package:		semantic
* File:			semantic.h
* Date:			16.11.2026
* Last change:	16.11.2026
* Authors: 		Nikolas Fox xfoxnik00
*
* Description: Header file for semantic analysis
*
*******************************************************************************/
/**
* @file semantic.h
*
* @brief Header file for semantic analysis
* @author Nikolas Fox xfoxnik00
*/






//TEMPORARY

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


/**
 * @brief Decides whether the program is semantically correct
 * @param whole Head of the entire AST
 */
void semanticAnalysis(AstNode *whole);


/*** End of file semantic. ***/