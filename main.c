/******************************************************************************
 * Project:      IFJ
 * Package:      Main
 * File:         main.c
 * Date:         19. 11. 2026
 * Last change:  26. 11. 2026
 * Authors:      Eliška Holá xholael00
 *
 * Description:  Entry point of the compiler
 *****************************************************************************/
/**
 * @file main.c
 *
 * @brief Entry point of the compiler
 * @author Eliška Holá xholael00
 */

#include "ast.h"
#include "codegen.h"
#include "parser.h"
#include "scanner.h"
#include "semantic.h"

#include <stdlib.h>

int main() {
    Scanner *sc = scannerInit();
    if (sc == NULL) {
        exit(99);
    }

    int error = 0;

    AstNode *node = parseProgram(sc);
    if (node == NULL) //error in parser
    {
        scannerFree(sc);
        return 2;
    }

    semanticAnalysis(node, &error);
    if (error != 0)
    {
        astFree(node);
        scannerFree(sc);
        return error;
    }
    generateProgram(node);
    astFree(node);
    scannerFree(sc);
}

/*** End of file main.c ***/
