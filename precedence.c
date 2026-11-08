/******************************************************************************
* Project:		IFJ
* Package:		precedence
* File:			precedence.c
* Date:			5.11.2026
* Last change:	8.11.2026
* Authors: 		xfoxnik00
*
* Description: Implementation of precedence syntactic analysis logic
*
*******************************************************************************/
/**
* @file precedence.c
*
* @brief Implementation of precedence syntactic analysis logic
* @author xfoxnik00
*/

#include "precedence.h"

#define PREC_TABLE_SIZE 14
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
    //          f(E)  E.E  +-P   !   */   +-   REL EQ-NE  &&  ||    (    )    i    $
    /* f(E) */ { 'X', '>', 'X', 'X', '>', '>', '>', '>', '>', '>', 'X', '>', 'X', '>'},
    /* E.E  */ { '<', '>', '>', '>', '>', '>', '>', '>', '>', '>', 'X', '>', '<', '>'},
    /* +-P  */ { '<', '<', '<', '<', '>', '>', '>', '>', '>', '>', '<', '>', '<', '>'},
    /*  !   */ { '<', '<', '<', '<', '>', '>', '>', '>', '>', '>', '<', '>', '<', '>'},
    /*  /*  */ { '<', '<', '<', '<', '>', '>', '>', '>', '>', '>', '<', '>', '<', '>'},
    /*  +-  */ { '<', '<', '<', '<', '<', '>', '>', '>', '>', '>', '<', '>', '<', '>'},
    /* REL  */ { '<', '<', '<', '<', '<', '<', 'X', '>', '>', '>', '<', '>', '<', '>'},
    /* EQ-NE*/ { '<', '<', '<', '<', '<', '<', '<', 'X', '>', '>', '<', '>', '<', '>'},
    /*  &&  */ { '<', '<', '<', '<', '<', '<', '<', '<', '>', '>', '<', '>', '<', '>'},
    /*  ||  */ { '<', '<', '<', '<', '<', '<', '<', '<', '<', '>', '<', '>', '<', '>'},
    /*  (   */ { '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '=', '<', 'X'},
    /*  )   */ { 'X', '>', 'X', 'X', '>', '>', '>', '>', '>', '>', 'X', '>', 'X', '>'},
    /*  i   */ { 'X', '>', 'X', 'X', '>', '>', '>', '>', '>', '>', 'X', '>', 'X', '>'},
    /*  $   */ { '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', 'X', '<', 'X'}
};

void precStackInit(PrecStack *stack)
{
    stack->top = NULL;
}

void precStackPush(PrecStack *stack, PrecSymbol symbol)
{
    PrecStackElem *newElem = (PrecStackElem *)malloc(sizeof(PrecStackElem));
    // ERROR HANDLING

    newElem->symbol = symbol;
    newElem->next = stack->top;
    stack->top = newElem;

    if(symbol == PREC_LPAR)
    {
        stack->openParCnt++;
    }
}


PrecSymbol precStackPop(PrecStack *stack)
{
    if(stack->top == NULL)
    {
        return -1;
    }

    PrecSymbol pop = stack->top->symbol;
    PrecStackElem *tmp = stack->top;
    stack->top = (stack->top->next);

    if(tmp->str != NULL)
    {
    free(tmp->str);
    }
    free(tmp);

    if(pop == PREC_LPAR)
    {
        stack->openParCnt--;
    }

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


void PrecStackFree(PrecStack *stack)
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
    // ERROR (we don't have any terminal on the stack)
    return -1;
}

void precStackShiftAfterFirstTerm(PrecStack *stack)
{
    PrecStackElem *newElem = (PrecStackElem *)malloc(sizeof(PrecStackElem));
    // TEST FOR ERROR

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

    // if we find no terminal we need to add it straight to the top of the stack (This should never happen, we should always have $)
    newElem->next = stack->top;
    stack->top = newElem;
}


PrecSymbol tokenToPrec(ParsedToken *p, bool previousWasOperand, PrecStack *stack)
{
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
            SymbolData *id = symtableSearch(&sTable, p->currentToken.str);

            getTokenAhead(p);
            if(p->aheadToken.type == TOK_LPAR)
            {
                return PREC_FUNC;        // function is caught, if it isn't a function we continue
            }


            if(id == NULL &&                // we have a string that isn't an ID or a soft keyword
                !(strcmp(p->currentToken.str, "true") == 0 || strcmp(p->currentToken.str, "false") == 0))
            {
                exit();
                // TODO: ERROR (we don't know what the string is, 
                // it's not defined anywhere and it isn't a soft KW)
                
            }

        // FALLTHROUGH
        case TOK_INT:
        case TOK_DOUBLE:
        case TOK_STRING:
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


void parseExpression(ParsedToken *p)
{
    bool previousWasOperand = false;
    PrecStack stack;

    precStackInit(&stack);
    precStackPush(&stack, PREC_END);

    PrecSymbol input = tokenToPrec(p, previousWasOperand, &stack);
    

    while (input != PREC_END && 
            !(precStackTop(&stack) == PREC_E && precStackGetFirstTerm(&stack) == PREC_END))
    {
        if(input == PREC_FUNC || input == PREC_DOT || input == PREC_MUL_DIV 
        || input == PREC_PLUS_MINUS || input == PREC_RPAR || input == PREC_ID)
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
            
            case '=':
                precStackPush(&stack, input);
                getNextToken(p);
                input = tokenToPrec(p, previousWasOperand, &stack);
                break;

            case '>':
            // TODO: REDUCTION LOGIC


        }
    }
    precStackFree(&stack);
}





/*** End of file precedence.c ***/

