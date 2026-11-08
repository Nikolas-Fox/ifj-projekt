/******************************************************************************
* Project:		IFJ
* Package:		precedence
* File:			precedence.c
* Date:			5.11.2026
* Last change:	5.11.2026
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

#define PREC_TABLE_SIZE 15

// Temporary from scanner.c
Token getToken()
{
    
}


// Temporary from symtable.c
SymbolData *symtableSearch(Symtable *sTable, char *name)
{

}




static const char precTable[PREC_TABLE_SIZE][PREC_TABLE_SIZE] =  
{
    //          f(E)  E.E  +-P   !   */   +-   REL EQ-NE  &&  ||    (    )    i    =    $
    /* f(E) */ { 'X', '>', 'X', 'X', '>', '>', '>', '>', '>', '>', 'X', '>', 'X', 'X', '>'},
    /* E.E  */ { '<', '>', '>', '>', '>', '>', '>', '>', '>', '>', 'X', '>', '<', '>', '>'},
    /* +-P  */ { '<', '<', '<', '<', '>', '>', '>', '>', '>', '>', '<', '>', '<', '>', '>'},
    /*  !   */ { '<', '<', '<', '<', '>', '>', '>', '>', '>', '>', '<', '>', '<', '>', '>'},
    /*  /*  */ { '<', '<', '<', '<', '>', '>', '>', '>', '>', '>', '<', '>', '<', '>', '>'},
    /*  +-  */ { '<', '<', '<', '<', '<', '>', '>', '>', '>', '>', '<', '>', '<', '>', '>'},
    /* REL  */ { '<', '<', '<', '<', '<', '<', 'X', '>', '>', '>', '<', '>', '<', '>', '>'},
    /* EQ-NE*/ { '<', '<', '<', '<', '<', '<', '<', 'X', '>', '>', '<', '>', '<', '>', '>'},
    /*  &&  */ { '<', '<', '<', '<', '<', '<', '<', '<', '>', '>', '<', '>', '<', '>', '>'},
    /*  ||  */ { '<', '<', '<', '<', '<', '<', '<', '<', '<', '>', '<', '>', '<', '>', '>'},
    /*  (   */ { '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '=', '<', '<', 'X'},
    /*  )   */ { 'X', '>', 'X', 'X', '>', '>', '>', '>', '>', '>', 'X', '>', 'X', 'X', '>'},
    /*  i   */ { 'X', '>', 'X', 'X', '>', '>', '>', '>', '>', '>', 'X', '>', 'X', '>', '>'},
    /*  =   */ { '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '>', '<', 'X', '>'},
    /*  $   */ { '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', 'X', '<', '<', '<', 'X'}
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

// TODO: Implement precedence functions

/*** End of file precedence.c ***/