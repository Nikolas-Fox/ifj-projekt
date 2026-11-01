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
    ST_IDENTIF
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
    Keyword kw;
} KeywordPair;

static const KeywordPair keywords[] = {
    {"def", KW_DEF},     {"val", KW_VAL},       {"var", KW_VAR},
    {"if", KW_IF},       {"else", KW_ELSE},     {"then", KW_THEN},
    {"while", KW_WHILE}, {"do", KW_DO},         {"return", KW_RETURN},
    {"import", KW_IMPORT}, {"Int", KW_INT},     {"Double", KW_DOUBLE},
    {"String", KW_STRING}, {"Unit", KW_UNIT},
};

// Vrátí řádek tabulky pro dané slovo, nebo NULL, pokud to není klíčové slovo
static const KeywordPair *findKeyword(const char *word) {
    size_t count = sizeof keywords / sizeof keywords[0];
 
    for (size_t index = 0; index < count; index++) {
        if (strcmp(word, keywords[index].name) == 0) {
            return &keywords[index];
        }
    }
    return NULL;
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
                    state = ST_IDENTIF;
                    break;
                }
                //identifier starting with a underscore
                if (character == '_') {
                    buf_reset();
                    buf_add(character);
                    state = ST_UNDERSCORE;
                    break;
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

                case ST_UNDERSCORE:
                if (isalnum(character) || character == '_') {
                    buf_add(character);
                    state = ST_IDENTIF;
                } 
                else {
                    lex_error("samotne podtrzitko neni identifikator");
                }
                break;

                case ST_IDENTIF:
                    if (isalnum(character) || character == '_') {
                        buf_add(character);
                    } 
                    else {
                        ungetc(character, stdin);  // znak už nepatří k identifikátoru
                        TokenType type = ident_type(buf);
                        
                        const KeywordPair *pair = findKeyword(buf);
                        if (pair != NULL) {
                            Token token = makeToken(TOK_KEYWORD, NULL);
                            token.kw = pair->kw;
                            return token;
                        }
                        return makeToken(TOK_IDENT, buf_copy());
                    }
                    break;
        }
    }
}