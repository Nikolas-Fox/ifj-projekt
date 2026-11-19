/******************************************************************************
* Project:		IFJ
* Package:		main
* File:			main.c
* Date:			19.11.2026
* Last change:	19.11.2026
* Authors: 		Eliška Holá xholael00
*
* Description:  Entry point of the compiler
*
*******************************************************************************/
/**
* @file main.c
*
* @brief Entry point of the compiler
* @author Eliška Holá xholael00
*/

#include "scanner.h"
#include "ast.h"
#include "parser.h"
#include "semantic.h"
#include "codegen.h"

#include <stdlib.h>

int main()
{
    Scanner *sc = scannerInit();
    if (sc == NULL)
    {
        exit(99);
    }

    AstNode *node = parseProgram(sc);
    semanticAnalysis(node);
    generateProgram(node);
    astFree(node);
    scannerFree(sc);
}

/*** End of file main.c ***/
