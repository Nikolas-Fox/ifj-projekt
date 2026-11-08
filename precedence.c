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

// Temporary sTable
Symtable sTable;


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

PrecSymbol tokenToPrec(Token *current, bool previousWasOperand, PrecStack *stack)
{
    switch(current->type)
    {
        case TOK_RPAR:
            if(stack->openParCnt > 0)
            {
                return PREC_RPAR;
            }
        case TOK_SEMICOL:
        case TOK_NL:
        case TOK_KW_THEN:
        case TOK_KW_DO:
        case TOK_COMMA:
            return PREC_END;

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
        
        case TOK_ASSIGN:
            return PREC_ASSIGN;

        case TOK_ID:
            SymbolData *id = symtableSearch(&sTable, current->str);

            if(id == NULL &&                // we don't have bool literals (soft keywords)
                !(strcmp(current->str, "true") == 0 || strcmp(current->str, "false") == 0))
                {
                    // TODO: ERROR (we don't know what the string is, 
                    // it's not defined anywhere and it isn't a soft KW)
                    
                }

            if(id->sType == FUNCTION)      // SYMTABLE WILL BE GLOBAL
                {
                    return PREC_FUNC;        // function is caught, if it isn't a function we continue
                }

            
        case TOK_INT:
        case TOK_DOUBLE:
        case TOK_STRING:
            return PREC_ID;
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
}

void precStackPushAfterFirstTerm(PrecStack *stack, PrecSymbol symbol)
{
    PrecStackElem *newElem = (PrecStackElem *)malloc(sizeof(PrecStackElem));
    // TEST FOR ERROR

    newElem->symbol = symbol;

    if(stack->top == NULL)
    {
        stack->top = newElem;
        newElem->next = NULL;
        return;
    }

    PrecStackElem *current = stack->top;
    while(current != NULL)
    {
        if(current->symbol != PREC_E && current->symbol != PREC_SHIFT)
        {
            newElem->next = current->next;
            current->next = newElem;
            return;
        }
        current = current->next;
    }

    // if we find no terminal we need to add it straight to the start
    newElem->next = stack->top;
    stack->top = newElem;
}



// TODO: Implement precedence functions

/*** End of file precedence.c ***/

void parseExpression(Token *currentToken)
{
    PrecStack stack;
    precStackInit(&stack);
    precStackPush(&stack, PREC_END);

    bool prevWasOperand = false;
    
}