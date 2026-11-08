//scanner
//author: Eliška Opršalová, xoprsae00
//last change: 8. 11. 2026

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
    ST_NEQ,
    ST_SLASH,          
    ST_LINE_COMMENT,   
    ST_BLOCK_COMMENT,  
    ST_BLOCK_SLASH,    
    ST_BLOCK_STAR,     
    ST_AFTER_BLOCK,
    ST_START_DOT,
    ST_ZERO,
    ST_INT,
    ST_INT_US,
    ST_DOT,
    ST_DECIMAL,
    ST_DECIMAL_US,
    ST_EXP,
    ST_EXP_SIGN,
    ST_EXP_DIGITS,
    ST_EXP_US,
    ST_SUFFIX,
    ST_STRING_START,
    ST_STRING,
    ST_ESCAPE,
    ST_EMPTY_STRING,
    ST_MUL_LINE_STRING,
    ST_MUL_LINE_QUOTE1,
    ST_MUL_LINE_QUOTE2,
    ST_MUL_LINE_QUOTE3,
    ST_LINE_START,
    ST_PAREN_LINE  // počítá mezery na dalším řádku uvnitř závorek
}States;

static int line = 1;

static int at_line_start = 1;  // 1 = začíná nový logický řádek
static int paren_depth = 0;    // hloubka kulatých závorek
static int line_indent = 0;    // počet mezer na začátku aktuálního logického řádku
static int line_tab = 0;       // 1 = odsazení obsahuje tabulátor

//Buffer for the lexeme text
static char *buf = NULL;
static size_t buf_len = 0;
static size_t buf_cap = 0;

static void buf_reset(void) {
    if (buf == NULL) {
        buf_cap = 64;
        buf = malloc(buf_cap);
        if (buf == NULL) {
            exit(99);    
        }
    }
    buf_len = 0;
    buf[0] = '\0';
}

static void buf_add(int c) {
    if (buf_len + 2 > buf_cap) {
        size_t new_cap = buf_cap * 2;
        char *new_buf = realloc(buf, new_cap);
        if (new_buf == NULL) exit(99);
        buf = new_buf;
        buf_cap = new_cap;
    }
    buf[buf_len++] = (char)c;
    buf[buf_len] = '\0';
}
 
//Copy of the buffer for the token (the token owns its text)
static char *buf_copy(void) {
    char *copy = malloc(buf_len + 1);
    if (!copy) exit(99);
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

static Token endNumber(TokenType type, int character) {
    if (isalnum(character) || character == '_') {
        fprintf(stderr, "Lexikalni chyba, neplatny znak za ciselnym literalem");
        exit(1);
    }
    ungetc(character, stdin);
    return makeToken(type, buf_copy());
}

Token readToken(void) {
    States state = ST_START;
    int comment_depth = 0;
    int comment_newline = 0;
    int indent_count = 0;
    if (at_line_start == 1) {
        at_line_start = 0;
        line_tab = 0;
        state = ST_LINE_START;
    }

    while(1) {
        int character = getchar();

        switch (state) {
            case ST_START:

                if(character == EOF) {
                    return makeToken(TOK_EOF, NULL);
                }  

                if (character == ' ' || character == '\t' || character == '\r') break;

                if (character == '\n') {
                    if (paren_depth > 0) {
                        line++;
                        indent_count = 0;
                        state = ST_PAREN_LINE;
                        break;
                    }
                    Token token = makeToken(TOK_EOL, NULL);
                    line++;
                    at_line_start = 1;
                    return token;
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
                if (character == '/') { state = ST_SLASH; break; }
                if (character == '.') { state = ST_START_DOT; break; }

                if (isdigit(character)) {
                    buf_reset();
                    buf_add(character);
                    if (character == '0') {
                        state = ST_ZERO;
                    } else {
                        state = ST_INT;
                    }
                    break;
                }

                if (character == '"') {
                    buf_reset();
                    state = ST_STRING_START;
                    break;
                }

                //one character tokens
                switch (character) {
                    case '(':
                        paren_depth++;
                        return makeToken(TOK_LPAR, NULL);

                    case ')': 
                         if (paren_depth > 0) {
                            paren_depth--;
                        }
                        return makeToken(TOK_RPAR, NULL);

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
                fprintf(stderr, "Lexikalni chyba, samostatny vykricnik az v rozsireni");
                    exit(1);
            
            case ST_SLASH:
                if (character == '/') {
                    state = ST_LINE_COMMENT;
                } else if (character == '*') {
                    comment_depth = 1;
                    comment_newline = 0;
                    state = ST_BLOCK_COMMENT;
                } else {
                    ungetc(character, stdin);
                    return makeToken(TOK_DIV, NULL);
                }
                break;

            case ST_LINE_COMMENT:
                if (character == '\n' || character == EOF) {
                    ungetc(character, stdin);  
                    state = ST_START;
                }
                break;

            case ST_BLOCK_COMMENT:
                if (character == EOF) {
                    fprintf(stderr, "Lexikalni chyba, neukonceny blokovy komentar");
                    exit(1);
                } else if (character == '\n') {
                    line++;
                    comment_newline = 1;
                } else if (character == '/') {
                    state = ST_BLOCK_SLASH;
                } else if (character == '*') {
                    state = ST_BLOCK_STAR;
                }
                break;
            
            case ST_BLOCK_SLASH:
                if (character == '*') {
                    comment_depth++;
                    state = ST_BLOCK_COMMENT;
                } else {
                    ungetc(character, stdin);
                    state = ST_BLOCK_COMMENT;
                }
                break;

            case ST_BLOCK_STAR:
                if (character == '/') {
                    comment_depth--;
                    if (comment_depth == 0) {
                        if (comment_newline == 1 && paren_depth == 0) {
                            state = ST_AFTER_BLOCK;
                        } else {
                            state = ST_START;
                        }
                    } else {
                        state = ST_BLOCK_COMMENT;
                    }
                } else {
                    ungetc(character, stdin);
                    state = ST_BLOCK_COMMENT;
                }
                break;
                
            case ST_AFTER_BLOCK:
                if (character == ' ' || character == '\t' || character == '\r') {
                    break;
                }
                if (character == '\n' || character == EOF) {
                    ungetc(character, stdin);
                    state = ST_START;
                    break;
                }
                fprintf(stderr, "Lexikalni chyba, po viceradkovem komentari nemuze nasledovat token");
                    exit(1);
                break;

            case ST_START_DOT:
                if (isdigit(character)) {
                    buf_reset();
                    buf_add('.');
                    buf_add(character);
                    state = ST_DECIMAL;
                } else {
                    ungetc(character, stdin);
                    return makeToken(TOK_DOT, NULL);
                }
                break;

            case ST_ZERO:
                if (isdigit(character)) {
                    fprintf(stderr, "Lexikalni chyba, nenulovy literal nesmi zacinat nulou");
                    exit(1);
                } else if (character == '.') {
                    buf_add(character);
                    state = ST_DOT;
                } else if (character == 'e' || character == 'E') {
                    buf_add(character);
                    state = ST_EXP;
                } else if (character == 'd' || character == 'D') {
                    state = ST_SUFFIX;
                } else {
                    return endNumber(TOK_INT, character);
                }
                break;

            case ST_INT:
                if (isdigit(character)) {
                    buf_add(character);
                } else if (character == '_') {
                    state = ST_INT_US;
                } else if (character == '.') {
                    buf_add(character);
                    state = ST_DOT;
                } else if (character == 'e' || character == 'E') {
                    buf_add(character);
                    state = ST_EXP;
                } else if (character == 'd' || character == 'D') {
                    state = ST_SUFFIX;
                } else {
                    return endNumber(TOK_INT, character);
                }
                break;

            case ST_INT_US:
                if (isdigit(character)) {
                    buf_add(character);
                    state = ST_INT;
                } else if (character != '_') {
                    fprintf(stderr, "Lexikalni chyba, podtrzitko musi byt mezi cislicemi");
                    exit(1);
                }
                break;

            case ST_DOT:
                if (isdigit(character)) {
                    buf_add(character);
                    state = ST_DECIMAL;
                } else {
                    fprintf(stderr, "Lexikalni chyba, za teckou sse ocekava cislice");
                    exit(1);
                }
                break;

            case ST_DECIMAL:
                if (isdigit(character)) {
                    buf_add(character);
                } else if (character == '_') {
                    state = ST_DECIMAL_US;
                } else if (character == 'e' || character == 'E') {
                    buf_add(character);
                    state = ST_EXP;
                } else if (character == 'd' || character == 'D') {
                    state = ST_SUFFIX;
                } else {
                    return endNumber(TOK_DOUBLE, character);
                }
                break;

            case ST_DECIMAL_US:
                if (isdigit(character)) {
                    buf_add(character);
                    state = ST_DECIMAL;
                } else if (character != '_') {
                    fprintf(stderr, "Lexikalni chyba, podtrzitko musi byt mezi cislicemi");
                    exit(1);
                }
                break;

            case ST_EXP:
                if (character == '+' || character == '-') {
                    buf_add(character);
                    state = ST_EXP_SIGN;
                } else if (isdigit(character)) {
                    buf_add(character);
                    state = ST_EXP_DIGITS;
                } else {
                    fprintf(stderr, "Lexikalni chyba, exponent musi obsahovat cislici");
                    exit(1);
                }
                break;

            case ST_EXP_SIGN:
                if (isdigit(character)) {
                    buf_add(character);
                    state = ST_EXP_DIGITS;
                } else {
                    fprintf(stderr, "Lexikalni chyba, exponent musi obsahovat cislici");
                    exit(1);
                }
                break;

            case ST_EXP_DIGITS:
                if (isdigit(character)) {
                    buf_add(character);
                } else if (character == '_') {
                    state = ST_EXP_US;
                } else if (character == 'd' || character == 'D') {
                    state = ST_SUFFIX;
                } else {
                    return endNumber(TOK_DOUBLE, character);
                }
                break;

            case ST_EXP_US:
                if (isdigit(character)) {
                    buf_add(character);
                    state = ST_EXP_DIGITS;
                } else if (character != '_') {
                    fprintf(stderr, "Lexikalni chyba, podtrzitko musi byt mezi cislicemi");
                    exit(1);
                }
                break;

            case ST_SUFFIX:
                return endNumber(TOK_DOUBLE, character);

            case ST_STRING_START:
                if (character == '"') {
                    state = ST_EMPTY_STRING;
                } else {
                    ungetc(character, stdin);
                    state = ST_STRING;
                }
                break;

            case ST_STRING:
                if (character == '"') {
                    return makeToken(TOK_STRING, buf_copy());
                } else if (character == '\\') {
                    state = ST_ESCAPE;
                } else if (character == '\n' || character == EOF) {
                    fprintf(stderr, "Lexikalni chyba, neukonceny retezec");
                    exit(1);
                } else {
                    buf_add(character);
                }
                break;

            case ST_ESCAPE:
                if (character == 'n') {
                    buf_add('\n');
                } else if (character == 'r') {
                    buf_add('\r');
                } else if (character == 't') {
                    buf_add('\t');
                } else if (character == '\\') {
                    buf_add('\\');
                } else if (character == '"') {
                    buf_add('"');
                } else {
                    fprintf(stderr, "Lexikalni chyba, neplatna escape sekvence");
                    exit(1);
                }
                state = ST_STRING;
                break;
            
            case ST_EMPTY_STRING:
                if (character == '"') {
                    state = ST_MUL_LINE_STRING;
                } else {
                    ungetc(character, stdin);
                    return makeToken(TOK_STRING, buf_copy());
                }
                break;

            case ST_MUL_LINE_STRING:
                if (character == '"') {
                    state = ST_MUL_LINE_QUOTE1;
                } else if (character == EOF) {
                    fprintf(stderr, "Lexikalni chyba, neukonceny viceradkovy retezec");
                    exit(1);
                } else {
                    if (character == '\n') {
                        line++;
                    }
                    buf_add(character);
                }
                break;

            case ST_MUL_LINE_QUOTE1:
                if (character == '"') {
                    state = ST_MUL_LINE_QUOTE2;
                } else {
                    buf_add('"');
                    ungetc(character, stdin);
                    state = ST_MUL_LINE_STRING;
                }
                break;

            case ST_MUL_LINE_QUOTE2:
                if (character == '"') {
                    state = ST_MUL_LINE_QUOTE3;
                } else {
                    buf_add('"');
                    buf_add('"');
                    ungetc(character, stdin);
                    state = ST_MUL_LINE_STRING;
                }
                break;

            case ST_MUL_LINE_QUOTE3:
                if (character == '"') {
                    buf_add('"');
                } else {
                    ungetc(character, stdin);
                    return makeToken(TOK_STRING, buf_copy());
                }
                break;

            case ST_LINE_START:
                if (character == ' ') {
                    indent_count++;
                } else if (character == '\t') {
                    line_tab = 1;
                } else if (character == '\r') {
                    // CR před LF (CRLF) se ignoruje
                } else {
                    ungetc(character, stdin);
                    line_indent = indent_count;
                    state = ST_START;
                }
                break;

            case ST_PAREN_LINE:
                if (character == ' ') {
                    indent_count++;
                } else if (character == '\t' || character == '\r') {
                    // bílý znak
                } else if (character == '\n') {
                    line++;
                    indent_count = 0;
                } else {
                    if (character != EOF && indent_count <= line_indent) {
                        fprintf(stderr, "Lexikalni chyba, radek uvnitr zavorek musi byt odsazen vice nez zacatek prikazu");
                        exit(1);
                    }
                    ungetc(character, stdin);
                    state = ST_START;
                }
                break;
        }
    }
}

#define MAX_INDENT 128
static int indent_stack[MAX_INDENT] = {0};
static int indent_top = 0;        // index vrcholu zásobníku (0 = žádné odsazení)
static int pending_dedents = 0;   // kolik DEDENT ještě vydat
static int has_queued = 0;        // 1 = token ve frontě čeká
static Token queued;              // token čekající za INDENT / DEDENT
static int line_has_token = 0;    // měl aktuální logický řádek už nějaký token?

Token getToken(void) {
    return readToken();
}