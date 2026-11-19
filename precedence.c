/******************************************************************************
* Project:		IFJ
* Package:		precedence
* File:			precedence.c
* Date:			5.11.2026
* Last change:	19.11.2026
* Authors: 		Nikolas Fox xfoxnik00
*
* Description: Implementation of precedence syntactic analysis logic
*
*******************************************************************************/
/**
* @file precedence.c
*
* @brief Implementation of precedence syntactic analysis logic
* @author Nikolas Fox xfoxnik00
*/

#include "precedence.h"
#include <stdlib.h>

#define PREC_TABLE_SIZE 13
#define RULE_SIZE 5

/**
 * @brief Precedencial analyser symbols (terminals from the precedencial table and stack nonterminals)
 * grouped operators with the same priority and associativity to minimise precedencial table size
 */
typedef enum
{
    PREC_DOT,         // E.E
    PREC_SIGN,        // unary +P, -P
    PREC_NOT,         // unary !
    PREC_MUL_DIV,     // '*', '/'
    PREC_PLUS_MINUS,  // binary +, -
    PREC_RELATION,    // '>', '>=', '<', '<='
    PREC_EQ_NE,       // '==', '!='
    PREC_AND,         // '&&'
    PREC_OR,          // '||'
    PREC_LPAR,        // '('
    PREC_RPAR,        // ')'
    PREC_ID,          // identifiers, function calls, literals
    PREC_END,         // '$'

    // symbols used only in the stack (absent from precTable):
    PREC_E,           // reduced expression
    PREC_SHIFT        // '<'
}PrecSymbol;


/**
 *  @brief element of the precedence stack
 */
typedef struct PrecStackElem
{
    PrecSymbol symbol;
    AstNode *node;
    TokenType operator;
    struct PrecStackElem *next;
}PrecStackElem;


/**
 * @brief abstract data type - Precedence stack 
 */
typedef struct 
{
    PrecStackElem *top;
    int openParCnt;    // holds a count of how many '(' are on the stack; 
                       // tokenToPrec function uses this to identify which ')' marks PREC_END
} PrecStack;


static const char precTable[PREC_TABLE_SIZE][PREC_TABLE_SIZE] =  
{
    //           E.E  +-P   !   */   +-   REL EQ-NE  &&  ||    (    )    i    $
    /* E.E  */ { '>', '>', '>', '>', '>', '>', '>', '>', '>', 'X', '>', '<', '>'},
    /* +-P  */ { '<', '<', '<', '>', '>', '>', '>', '>', '>', '<', '>', '<', '>'},
    /*  !   */ { '<', '<', '<', '>', '>', '>', '>', '>', '>', '<', '>', '<', '>'},
    /*  / * */ { '<', '<', '<', '>', '>', '>', '>', '>', '>', '<', '>', '<', '>'},
    /*  +-  */ { '<', '<', '<', '<', '>', '>', '>', '>', '>', '<', '>', '<', '>'},
    /* REL  */ { '<', '<', '<', '<', '<', 'X', '>', '>', '>', '<', '>', '<', '>'},
    /* EQ-NE*/ { '<', '<', '<', '<', '<', '<', 'X', '>', '>', '<', '>', '<', '>'},
    /*  &&  */ { '<', '<', '<', '<', '<', '<', '<', '>', '>', '<', '>', '<', '>'},
    /*  ||  */ { '<', '<', '<', '<', '<', '<', '<', '<', '>', '<', '>', '<', '>'},
    /*  (   */ { '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '=', '<', 'X'},
    /*  )   */ { '>', 'X', 'X', '>', '>', '>', '>', '>', '>', 'X', '>', 'X', '>'},
    /*  i   */ { '>', 'X', 'X', '>', '>', '>', '>', '>', '>', 'X', '>', 'X', '>'},
    /*  $   */ { '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', 'X', '<', 'X'}
};


static void precStackInit(PrecStack *stack);
static void precStackPush(PrecStack *stack, PrecStackElem *element);
static PrecStackElem precStackPop(PrecStack *stack);
static PrecSymbol precStackTop(PrecStack *stack);
static void precStackFree(PrecStack *stack);
static PrecSymbol precStackGetFirstTerm(PrecStack *stack);
static void precStackShiftAfterFirstTerm(PrecStack *stack);
static PrecSymbol tokenToPrec(ParsedToken *p, bool previousWasOperand, PrecStack *stack, AstNode **node);
static void precReduce(PrecStack *stack);
static AstNode *parseLoa(ParsedToken *p, size_t *groupCount);
static AstNode *parseArgList(ParsedToken *p, size_t *count);
static AstNode *parseArgEnd(ParsedToken *p, size_t *count);


/**
 * @brief Initialises an empty precedence stack
 * @param stack Pointer to the stack for initialization
 */
static void precStackInit(PrecStack *stack)
{
    stack->top = NULL;
    stack->openParCnt = 0;
}


/**
 * @brief Pushes an element onto the stack; for '('  openParCnt++, for  ')' openParCnt--
 * @param stack Pointer to stack
 * @param element PrecSymbol being pushed onto the stack (with PrecSymbol, type, string and next)
 */
static void precStackPush(PrecStack *stack, PrecStackElem *element)
{
    PrecStackElem *newElem = (PrecStackElem *)malloc(sizeof(PrecStackElem));
    if(newElem == NULL)
    {
        // TODO: ERROR (internal error malloc)
        exit(99);
    }

    newElem->symbol = element->symbol;
    newElem->operator = element->operator;
    newElem->node = element->node;
    newElem->next = stack->top;

    stack->top = newElem;

    if(element->symbol == PREC_LPAR)
    {
        stack->openParCnt++;
    }

    if(element->symbol == PREC_RPAR)
    {
        stack->openParCnt--;
    }
}


/**
 * @brief Removes and returns first element on stack
 * @param stack Pointer to stack
 * @return First element on the stack; for an empty stack an element with all fields zero and symbol = -1
 */
static PrecStackElem precStackPop(PrecStack *stack)
{
    if(stack->top == NULL)
    {
        PrecStackElem empty = {0};
        empty.symbol = -1;
        return empty; // nothing is left on the stack
    }

    PrecStackElem pop = *(stack->top);
    PrecStackElem *tmp = stack->top;
    stack->top = (stack->top->next);

    free(tmp);
    return pop;
}


/**
 * @brief Returns first symbol on stack without removing it
 * @param stack Pointer to stack
 * @return First symbol on stack, -1 if the stack is empty
 */
static PrecSymbol precStackTop(PrecStack *stack)
{
    if(stack->top == NULL)
    {
        return -1;
    }

    return stack->top->symbol;
}

/**
 * @brief Frees the whole PrecStack
 * @param stack Pointer to stack
 */
static void precStackFree(PrecStack *stack)
{
    PrecStackElem *tmp;
    while(stack->top != NULL)
    {
        tmp = stack->top;
        stack->top = (stack->top->next);
        free(tmp);
    }
}

/**
 * @brief Returns the first terminal on the stack (ignores PREC_E and PREC_SHIFT), exit(99) for a stack without terminals
 * @param stack Pointer to stack
 * @return The first terminal on the stack
 */
static PrecSymbol precStackGetFirstTerm(PrecStack *stack)
{
    PrecStackElem *current = stack->top;

    while(current != NULL)
    {
        if(current->symbol != PREC_E && current->symbol != PREC_SHIFT)
        {
            return current->symbol;
        }
        current = current->next;
    }
    // ERROR (we don't have any terminal on the stack) (it should always contain $)
    exit(99);
}


/**
 * @brief Inserts the Shift symbol '<' onto the stack after the first terminal
 * @param stack Pointer to stack
 */
static void precStackShiftAfterFirstTerm(PrecStack *stack)
{
    PrecStackElem *newElem = (PrecStackElem *)calloc(1,sizeof(PrecStackElem));
    if(newElem == NULL)
    {
        // ERROR (calloc failure) (internal error)
        exit(99);
    }

    newElem->symbol = PREC_SHIFT;

    if(stack->top == NULL)
    {
        stack->top = newElem;
        newElem->next = NULL;
        return;
    }

    PrecStackElem *current = stack->top;
    PrecStackElem *previous = NULL;
    while(current != NULL)
    {
        if(current->symbol != PREC_E && current->symbol != PREC_SHIFT)
        {
            if(previous != NULL)
            {
                previous->next = newElem;
                newElem->next = current;
            }
            else
            {
                stack->top = newElem;
                newElem->next = current;
            }
            return;
        }
        previous = current;
        current = current->next;
    }

    // TODO: ERROR - we don't have any terminal on the stack (we should always have at least $)
    //        internal error
    exit(99);
}

/**
 * @brief returns the precedencial symbol equivalent of a token and builds an AST node if it is an ID or literal
 * @param p Pointer to the parsed token
 * @param previousWasOperand True for ')', IDs and literals; used for determining unary and binary + -
 * @param stack Pointer to stack (used to track openParCnt)
 * @param node Pointer to node we want to set
 * @return precedenical symbol equivalent of token
 */
static PrecSymbol tokenToPrec(ParsedToken *p, bool previousWasOperand, PrecStack *stack, AstNode **node)
{
    *node = NULL;
    switch(p->currentToken.type)
    {
        case TOK_DOT:
            return PREC_DOT;
        
        case TOK_PLUS:
        case TOK_MINUS:
            if(previousWasOperand)
            {
                return PREC_PLUS_MINUS;
            }
            else
            {
                return PREC_SIGN;
            }

        case TOK_NOT: 
            return PREC_NOT;
        
        case TOK_MUL:
        case TOK_DIV:
            return PREC_MUL_DIV;
        
        case TOK_LESS:
        case TOK_LESS_EQ:
        case TOK_GREATER:
        case TOK_GREATER_EQ:
            return PREC_RELATION;

        case TOK_EQ:
        case TOK_NEQ:
            return PREC_EQ_NE;

        case TOK_AND:
            return PREC_AND;

        case TOK_OR:
            return PREC_OR;

        case TOK_LPAR:
            return PREC_LPAR;

        case TOK_ID:
            getTokenAhead(p);

            if(p->aheadToken.type == TOK_LPAR)
            {
                AstNode *call = createNodeCall(p->currentToken.str, 0, NULL); // we will free the string
                getNextToken(p);  // parseLoa expects the first symbol to be '(', not id
                
                size_t groupCount = 0;
                call->data.call.group = parseLoa(p, &groupCount);
                call->data.call.groupCount = groupCount;

                *node = call;
                return PREC_ID;   // functions are simplified to ID
            }
            else
            {
                *node = createNodeID(p->currentToken.str);  // this also has true/false, type checker will take care of this
                return PREC_ID;
            }

            // all other identifier tokens are valid
            // true and false are soft keywords, 
            // they don't show up in the symtable but are clasified as TOK_ID by the lexer

        case TOK_INT:
            *node = createNodeLiteral(TYPE_INT, p->currentToken.str);
            return PREC_ID;

        case TOK_DOUBLE:
            *node = createNodeLiteral(TYPE_DOUBLE, p->currentToken.str);
            return PREC_ID;

        case TOK_STRING:
            *node = createNodeLiteral(TYPE_STRING, p->currentToken.str);
            return PREC_ID;

        case TOK_RPAR:
            if(stack->openParCnt > 0)
            {
                return PREC_RPAR;
            }
            return PREC_END; //')' is PREC_END when openParCnt == 0, because an expression can be inside parentheses: 'if (expression)'

        default: 
            return PREC_END;   // everything else is an end (the parser will take care of it if it's not valid)
    }
}

/**
 * @brief Reduces rules on the stack (all elements after '<')
 * @param stack Pointer to stack
 */
static void precReduce(PrecStack *stack)
{
    PrecStackElem poppedElements[RULE_SIZE];
    int popCnt = 0;
    PrecStackElem popped;
    
    do
    {
        popped = precStackPop(stack);
        if(popped.symbol != PREC_SHIFT)
        {
            if(popCnt >= RULE_SIZE)
            {
                // ERROR (syntax)
                exit(2);
            }
            poppedElements[popCnt] = popped; // poppedElements are saved in the oposite direction ([0] is the last element)
            popCnt++;
        }
    }
    while(popped.symbol != PREC_SHIFT && stack->top != NULL);


    // ID or LITERAL
    if(popCnt == 1 && poppedElements[0].symbol == PREC_ID)
    {
        // rule i -> E
        PrecStackElem element = {0};
        element.symbol = PREC_E;
        element.node = poppedElements[0].node;
        precStackPush(stack, &element);
        return;
    }

    // UNARY EXPRESSION
    if(popCnt == 2 && poppedElements[0].symbol == PREC_E) // poppedElements are saved in the oposite direction
    {
        // rules sgnE -> E,    !E -> E
        PrecSymbol symbol = poppedElements[1].symbol;
        if(symbol == PREC_SIGN || symbol == PREC_NOT)
        {
            PrecStackElem element = {0};
            element.symbol = PREC_E;
            element.node = createNodeUnary(poppedElements[1].operator, poppedElements[0].node);
            precStackPush(stack, &element);
            return;
        }
    }

    // UNIT
    if(popCnt == 2 && poppedElements[1].symbol == PREC_LPAR && poppedElements[0].symbol == PREC_RPAR) 
    {
        // rule () -> E
        PrecStackElem element = {0};
        element.symbol = PREC_E;
        element.node = createNodeLiteral(TYPE_UNIT, NULL);
        precStackPush(stack, &element);
        return;
    }

    if(popCnt == 3)
    {
        if(poppedElements[0].symbol == PREC_E && poppedElements[2].symbol == PREC_E) 
        {
            // rules E op E -> E
            if(poppedElements[1].symbol == PREC_DOT
                || poppedElements[1].symbol == PREC_MUL_DIV
                || poppedElements[1].symbol == PREC_PLUS_MINUS
                || poppedElements[1].symbol == PREC_AND
                || poppedElements[1].symbol == PREC_OR
                || poppedElements[1].symbol == PREC_RELATION
                || poppedElements[1].symbol == PREC_EQ_NE)
            {
                PrecStackElem element = {0};
                element.symbol = PREC_E;
                element.node = createNodeBinary(poppedElements[1].operator, poppedElements[2].node, poppedElements[0].node);
                precStackPush(stack, &element);
                return;
            }
        }

        if(poppedElements[0].symbol == PREC_RPAR && poppedElements[2].symbol == PREC_LPAR && poppedElements[1].symbol == PREC_E)
        {
            // rule (E) -> E
            PrecStackElem element = {0};
            element.symbol = PREC_E;
            element.node = poppedElements[1].node;
            precStackPush(stack, &element);
            return;
        }
    }
    // ERROR (syntax)
    // if we reach this point no valid rule to reduce was used
    exit(2);
}

/**
 * @author Eliška Holá (xholael00) & Nikolas Fox (xfoxnik00)
 * @brief Parses <loa>, implements LL grammar rules 46
 * @param p Struct of parser state
 * @param groupCount Pointer to the number of groups, incremented with each new group
 */
static AstNode *parseLoa(ParsedToken *p, size_t *groupCount)
{
    //46: ⟨loa⟩ → (⟨arg_list⟩)⟨loa⟩
    if (p->currentToken.type == TOK_LPAR)
    {
        checkToken(p, TOK_LPAR);

        size_t count = 0; // keeps track of number of arguments

        AstNode *items = parseArgList(p, &count);

        if(p->currentToken.type != TOK_RPAR)
        {
            //ERROR (syntax error)
            exit(2);
        }

        AstNode *group = createNodeGroup(items, count);
        (*groupCount)++;
        
        getTokenAhead(p);
        if(p->aheadToken.type == TOK_LPAR)
        {
            getNextToken(p);   // if the next token is another '(' we have a new group that we add to the list of groups
            group->next = parseLoa(p, groupCount);
        }

        // if the next token isn't a '(', then the current token stays at ')' so it isn't consumed
        return group; 
    }
    return NULL;
}


/**
 * @author Eliška Holá (xholael00) & Nikolas Fox (xfoxnik00)
 * @brief Parses <arg_list>, implements LL grammar rules 48 and 49.
 * @param p Struct of parser state
 * @param count Pointer to number of arguments, incremented with each argument
 * @return Pointer to the first argument in linked list
 */
static AstNode *parseArgList(ParsedToken *p, size_t *count)
{
    //48: ⟨arg_list⟩ → ⟨expression⟩⟨arg_end⟩
    if (startsExpression(p->currentToken.type))
    {
        AstNode *arg = parseExpression(p);
        (*count)++;
        arg->next = parseArgEnd(p,count);
        return arg;
    }
    //49: ⟨arg_list⟩ → epsilon
    else if (p->currentToken.type == TOK_RPAR)
    {
        return NULL; // we are at the end of the list
    }

    // ERROR (syntax error)
    exit(2);
}

/**
 * @author Eliška Holá (xholael00) & Nikolas Fox (xfoxnik00)
 * @brief Parses <arg_end>, implements LL grammar rules 50 and 51.
 * @param p Struct of parser state
 * @param count Pointer to the number of arguments (given to parseArgList for the next argument)
 * @return Pointer to the following argument in linked list
 */
static AstNode *parseArgEnd(ParsedToken *p, size_t *count)
{
    switch (p->currentToken.type)
    {
        case TOK_COMMA: //50: ⟨arg_end⟩ →,⟨arg_list⟩
            checkToken(p, TOK_COMMA);
            return parseArgList(p, count);
            break;
        case TOK_RPAR: //51: ⟨arg_end⟩ → epsilon
            return NULL;
        default:
            // ERROR (syntax error)
            exit(2);
            break;
    }
}


AstNode *parseExpression(ParsedToken *p)
{
    bool previousWasOperand = false;
    PrecStack stack;

    precStackInit(&stack);

    PrecStackElem endSymbol = {0};
    endSymbol.symbol = PREC_END;

    precStackPush(&stack, &endSymbol);

    AstNode *node;
    PrecSymbol input = tokenToPrec(p, previousWasOperand, &stack, &node);
    // the node will change to either AST_LITERAL, AST_ID or function call
    
    // while iterates until only $E is left on the stack and the input is another $ 
    while (!(input == PREC_END && precStackTop(&stack) == PREC_E && precStackGetFirstTerm(&stack) == PREC_END))
    {
        if(input == PREC_ID || input == PREC_RPAR)
        {
            previousWasOperand = true;
        }
        else
        {
            previousWasOperand = false;
        }

        PrecSymbol topTerm = precStackGetFirstTerm(&stack);
        switch(precTable[topTerm][input])
        {
            case '<':
                precStackShiftAfterFirstTerm(&stack);
                // FALLTHROUGH
            
            case '=':
            {
                PrecStackElem element = {0};
                element.symbol = input;
                element.node = node;
                element.operator = p->currentToken.type;
                precStackPush(&stack, &element);
                
                getNextToken(p);
                input = tokenToPrec(p, previousWasOperand, &stack, &node);
                break;
            }
            
            case '>':
                precReduce(&stack);
                break;
            default:
                //ERROR (syntax error)
                exit(2);
        }
    }

    // Now the only thing left on the stack should be E
    PrecStackElem E = precStackPop(&stack);
    precStackFree(&stack);
    return E.node;  // returning the node with the built expression
}


/*** End of file precedence.c ***/

