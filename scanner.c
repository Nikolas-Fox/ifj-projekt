//scanner
//author: Eliška Opršalová, xoprsae00
//last change: 1. 11. 2026

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "scanner.h"

//auromat states
typedef enum {
    ST_START,
    ST_UNDERSCORE,
    ST_ID,
    ST_LESS,
    ST_GREATER,
    ST_ASSIGN,
    ST_NEQ
}States;

static int line = 1;

//Buffer for the lexeme text (fixed size for now, dynamic later)
#define BUF_MAX 256
static char buf[BUF_MAX];
static int buf_len = 0;

static void buf_reset(void) {
    buf_len = 0;
    buf[0] = '\0';
}

static void buf_add(int c) {
    if (buf_len + 1 >= BUF_MAX) {
        fprintf(stderr, "Prilis dlouhy lexem na radku %d\n", line);
        exit(99);
    }
    buf[buf_len++] = (char)c;
    buf[buf_len] = '\0';
}
 
//Copy of the buffer for the token (the token owns its text)
static char *buf_copy(void) {
    char *copy = malloc(buf_len + 1);
    if (!copy) exit(99);  // interní chyba překladače
    memcpy(copy, buf, buf_len + 1);
    return copy;
}

typedef struct {
    const char *name;
    TokenType kw;
} KeywordPair;

static const KeywordPair keywords[] = {
    {"def", TOK_KW_DEF},     {"val", TOK_KW_VAL},       {"var", TOK_KW_VAR},
    {"if", TOK_KW_IF},       {"else", TOK_KW_ELSE},     {"then", TOK_KW_THEN},
    {"while", TOK_KW_WHILE}, {"do", TOK_KW_DO},         {"return", TOK_KW_RETURN},
    {"import", TOK_KW_IMPORT}, {"Int", TOK_KW_INT},     {"Double", TOK_KW_DOUBLE},
    {"String", TOK_KW_STRING}, {"Unit", TOK_KW_UNIT},
};

// Vrátí typ klíčového slova, nebo TOK_ID, pokud to není klíčové slovo
static TokenType findKeyword(const char *word) {
    size_t count = sizeof keywords / sizeof keywords[0];

    for (size_t index = 0; index < count; index++) {
        if (strcmp(word, keywords[index].name) == 0) {
            return keywords[index].kw;
        }
    }
    return TOK_ID;
}

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

                //identifier starting with a letter
                if (isalpha(character)) {
                    buf_reset();
                    buf_add(character);
                    state = ST_ID;
                    break;
                }
                //identifier starting with a underscore
                if (character == '_') {
                    buf_reset();
                    buf_add(character);
                    state = ST_UNDERSCORE;
                    break;
                }

                //operators that may have a second character
                if (character == '<') { state = ST_LESS; break; }
                if (character == '>') { state = ST_GREATER; break; }
                if (character == '=') { state = ST_ASSIGN; break; }
                if (character == '!') { state = ST_NEQ; break; }

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
                    case '+': return makeToken(TOK_PLUS, NULL);
                }
                
                fprintf(stderr, "Lexikalni chyba");
                exit(1);

                break;

            case ST_UNDERSCORE:
            if (isalnum(character) || character == '_') {
                buf_add(character);
                state = ST_ID;
            } 
            else {
                fprintf(stderr, "Lexikalni chyba, samosttatne podtržítko není identifikátor");
                exit(1);
            }
            break;

            case ST_ID:
                if (isalnum(character) || character == '_') {
                    buf_add(character);
                } else {
                    ungetc(character, stdin);  // znak už nepatří k identifikátoru

                    TokenType type = findKeyword(buf);
                    if (type == TOK_ID) {
                        return makeToken(TOK_ID, buf_copy());
                    }
                    return makeToken(type, NULL);
                }
                break;
            
            case ST_LESS:
                if (character == '=') {
                    return makeToken(TOK_LESS_EQ, NULL);
                }
                ungetc(character, stdin);
                return makeToken(TOK_LESS, NULL);

            case ST_GREATER:
                if (character == '=') {
                    return makeToken(TOK_GREATER_EQ, NULL);
                }
                ungetc(character, stdin);
                return makeToken(TOK_GREATER, NULL);

            case ST_ASSIGN:
                if (character == '=') {
                    return makeToken(TOK_EQ, NULL);
                }
                ungetc(character, stdin);
                return makeToken(TOK_ASSIGN, NULL);

            case ST_NEQ:
                if (character == '=') {
                    return makeToken(TOK_NEQ, NULL);
                }
                lexError("samotny vykricnik (negace je az v rozsireni)");    
        }
    }
}