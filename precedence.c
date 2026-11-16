/******************************************************************************
* Project:		IFJ
* Package:		precedence
* File:			precedence.c
* Date:			5.11.2026
* Last change:	13.11.2026
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
/*****************************************************************/
// Temporary from parser
Token getNextToken(ParsedToken *p)
{
    
}

void getTokenAhead(ParsedToken *p)
{

}

// Temporary from symtable.c
SymbolData *symtableSearch(Symtable *sTable, char *name)
{

}

// Temporary sTable (it will be global)
Symtable sTable;
/*****************************************************************/


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

void precStackInit(PrecStack *stack)
{
    stack->top = NULL;
    stack->openParCnt = 0;
}

void precStackPush(PrecStack *stack, PrecStackElem *element)
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


PrecStackElem precStackPop(PrecStack *stack)
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


PrecSymbol precStackTop(PrecStack *stack)
{
    if(stack->top == NULL)
    {
        return -1;
    }

    return stack->top->symbol;
}


void precStackFree(PrecStack *stack)
{
    PrecStackElem *tmp;
    while(stack->top != NULL)
    {
        tmp = stack->top;
        stack->top = (stack->top->next);
        free(tmp);
    }
}


PrecSymbol precStackGetFirstTerm(PrecStack *stack)
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

void precStackShiftAfterFirstTerm(PrecStack *stack)
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


PrecSymbol tokenToPrec(ParsedToken *p, bool previousWasOperand, PrecStack *stack, AstNode **node)
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

            // functions without a parameter list will also fallthrough
            
        // FALLTHROUGH
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

        // FALLTHROUGH because ) can be an end too
        default: 
            return PREC_END;   // everything else is an end (the parser will take care of it if it's not valid)
    }
}

void precReduce(PrecStack *stack)
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
            poppedElements[popCnt] = popped;
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
    if(popCnt == 2 && poppedElements[0].symbol == PREC_E) // the array is backwards
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


AstNode *parseLoa(ParsedToken *p, size_t *groupCount)
{
    AstNode *groups = NULL;

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
        return group;
    }
    return NULL;
}


AstNode *parseArgList(ParsedToken *p, size_t *count)
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


AstNode *parseArgEnd(ParsedToken *p, size_t *count)
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
    // the node will change to either AST_LITERAL or AST_ID
    

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

