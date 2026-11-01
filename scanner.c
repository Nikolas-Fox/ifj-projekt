//scanner
//author: Eliška Opršalová, xoprsae00
//last change: 26. 10. 2026

#include <stdio.h>
#include <stdlib.h>
#include "scanner.h"

//auromat states
typedef enum {
    ST_START,
    ST_UNDERSCORE,
    ST_IDENTIF
}States;

static int line = 1;

static Token makeToken(TokenType type, char *str) {
    Token tok = {0};
    tok.type = type;
    tok.str = str;
    tok.line = line;
    return tok;
}

