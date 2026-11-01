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

Token getToken(void) {
    States state = ST_START;

    while(1) {
        int character = getchar();

        switch (state) {
            case ST_START:

                if(character == EOF) {
                    return makeToken(TOK_EOF, NULL);
                }  

                //TO DO: indentation at the beginning of a line
                if (character == ' ' || character == '\t' || character == '\r') break;

                if (character == '\n') {
                    Token t = makeToken(TOK_EOL, NULL);
                    line++;
                    return t;   //LF is EOL
                }

                //one character tokens
                switch (character) {
                    case '(': return makeToken(TOK_LPAR, NULL);
                    case ')': return makeToken(TOK_RPAR, NULL);
                    case ',': return makeToken(TOK_COMMA, NULL);
                    case ';': return makeToken(TOK_SEMICOL, NULL);
                    case ':': return makeToken(TOK_COLON, NULL);
                    case '*': return makeToken(TOK_MUL, NULL);
                    case '-': return makeToken(TOK_MINUS, NULL);
                    case '@': return makeToken(TOK_AT, NULL);
                }
                
                fprintf(stderr, "Lexikalni chyba");
                exit(1);

                break;
        }
    }
}