/******************************************************************************
 * Project:      IFJ
 * Package:      Lexical analysis
 * File:         scanner.c
 * Date:         18. 10. 2026
 * Last change:  26. 11. 2026
 * Authors:      Eliška Opršalová xoprsae00
 *
 * Description:  Implementation of lexer - implemented as a deterministic
 * finite automat
 *****************************************************************************/
/**
 * @file scanner.c
 *
 * @brief implementation of lexer
 *
 * @author Eliška Opršalová xoprsae00
 */

#include "scanner.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// auromat states
typedef enum {
    // general states
    ST_START,
    ST_UNDERSCORE,
    ST_ID,
    ST_LESS,
    ST_GREATER,
    ST_ASSIGN,
    ST_NEQ,
    ST_AT,
    ST_PLUS,
    ST_MINUS,
    ST_AMP,
    ST_PIPE,

    // states for comments
    ST_SLASH,
    ST_LINE_COMMENT,
    ST_BLOCK_COMMENT,
    ST_BLOCK_SLASH,
    ST_BLOCK_STAR,
    ST_AFTER_BLOCK,

    // states for numeric literals
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

    // states for string
    ST_STRING_START,
    ST_STRING,
    ST_ESCAPE,
    ST_EMPTY_STRING,
    ST_MUL_LINE_STRING,
    ST_MUL_LINE_QUOTE1,
    ST_MUL_LINE_QUOTE2,
    ST_MUL_LINE_QUOTE3,
    ST_LINE_START,
    ST_PAREN_LINE, // counts spaces on the next line inside parentheses

    // interpolated strings
    ST_INTERP_OPEN,
    ST_INTERP_OPEN2,
    ST_INTERP_TEXT,
    ST_INTERP_ESCAPE,
    ST_INTERP_DOLLAR,
    ST_INTERP_ID,
    ST_INTERP_QUOTE1,
    ST_INTERP_QUOTE2,
    ST_INTERP_QUOTE3

} States;

// AI (Claude): vygeneruj mi funkce pro dynamickou práci s buffrem
// functions slightly edited after generated
static void bufReset(Scanner *sc) {
    if (sc->buf == NULL) {
        sc->bufCap = 64;
        sc->buf = malloc(sc->bufCap);
        if (sc->buf == NULL) {
            exit(99);
        }
    }
    sc->bufLen = 0;
    sc->buf[0] = '\0';
}

// AI (Claude): same prompt
static void bufAdd(Scanner *sc, int c) {
    if (sc->bufLen + 2 > sc->bufCap) {
        size_t newCap = sc->bufCap * 2;
        char *newBuf = realloc(sc->buf, newCap);
        if (newBuf == NULL)
            exit(99);
        sc->buf = newBuf;
        sc->bufCap = newCap;
    }
    sc->buf[sc->bufLen++] = (char)c;
    sc->buf[sc->bufLen] = '\0';
}

// copy of the buffer for the token (the token owns its text)
// AI (Claude): same prompt
static char *bufCopy(Scanner *sc) {
    char *copy = malloc(sc->bufLen + 1);
    if (!copy)
        exit(99);
    memcpy(copy, sc->buf, sc->bufLen + 1);
    return copy;
}

typedef struct {
    const char *name;
    TokenType kw;
} KeywordPair;

static const KeywordPair keywords[] = {
    {"def", TOK_KW_DEF},         {"val", TOK_KW_VAL},
    {"var", TOK_KW_VAR},         {"if", TOK_KW_IF},
    {"else", TOK_KW_ELSE},       {"then", TOK_KW_THEN},
    {"while", TOK_KW_WHILE},     {"do", TOK_KW_DO},
    {"return", TOK_KW_RETURN},   {"import", TOK_KW_IMPORT},
    {"Int", TOK_KW_INT},         {"Double", TOK_KW_DOUBLE},
    {"String", TOK_KW_STRING},   {"Unit", TOK_KW_UNIT},
    {"Boolean", TOK_KW_BOOLEAN}, {"extension", TOK_KW_EXTENSION},

};

// returns the keyword token type, or TOK_ID if it is not a keyword
static TokenType findKeyword(const char *word) {
    size_t count = sizeof keywords / sizeof keywords[0];

    for (size_t index = 0; index < count; index++) {
        if (strcmp(word, keywords[index].name) == 0) {
            return keywords[index].kw;
        }
    }
    return TOK_ID;
}

static Token makeToken(Scanner *sc, TokenType type, char *str) {
    Token tok = {0};
    tok.type = type;
    tok.str = str;
    tok.line = sc->line;
    return tok;
}

/**
 * @brief Prints error message.
 * @param message Error message to print.
 */
static void printLexicalError(const char *message) {
    fprintf(stderr, "Lexical analysis error: %s\n", message);
}

static void printSyntacticError(const char *message) {
    fprintf(stderr, "Syntactic analysis error: %s\n", message);
}

//author: Nikolas Fox xfoxnik00
static inline Token lexicalError(Scanner *sc,  const char *message)
{
    sc->error = 1;
    printLexicalError(message);
    return makeToken(sc, TOK_ERROR, NULL);
}

static inline Token syntacticError(Scanner *sc, const char *message)
{
    sc->error = 2;
    printSyntacticError(message);
    return makeToken(sc, TOK_ERROR, NULL);
}

// checks the character after a numeric literal, pushes back the excess
// character, and creates the token
static Token endNumber(Scanner *sc, TokenType type, int character) {
    if (isalnum(character) || character == '_') {
        return syntacticError(sc, "invalid character after numeric literal");
    }
    ungetc(character, stdin);
    return makeToken(sc, type, bufCopy(sc));
}

// opens an interpolated string (kind 0 = one line, 1 = multi-line)
// INTERPOLATEDS extention AI (Claude): explained and partialy generated
// prompts:
// vysvětli mi jak funguje rozšíření INTERPOLATEDS
// navrni mi kostru, jak by to zhruba mělo vypadat
static void pushInterp(Scanner *sc, int kind) {
    if (sc->interpDepth + 1 >= MAX_INTERP) {
        exit(99);
    }
    sc->interpKind[sc->interpDepth] = kind;
    sc->interpInExpr[sc->interpDepth] = 0;
    sc->interpDepth++;
}

// closes an interpolated string: returns the pending text (if any) and then
// TOK_INTERP_END
static Token endInterpolation(Scanner *sc) {
    sc->interpDepth--;
    Token end = makeToken(sc, TOK_INTERP_END, NULL);
    if (sc->bufLen == 0) {
        return end;
    }
    sc->interpQueued = end;
    sc->hasInterpQueued = 1;
    return makeToken(sc, TOK_STRING, bufCopy(sc));
}

// starts ${ ... }: returns the pending text (if any) and then
// TOK_INTERP_EXPR_START
static Token startExpression(Scanner *sc) {
    sc->interpInExpr[sc->interpDepth - 1] = 1;
    sc->parenDepth++; // a line break inside ${ } is only whitespace
    Token start = makeToken(sc, TOK_INTERP_EXPR_START, NULL);
    if (sc->bufLen == 0) {
        return start;
    }
    sc->interpQueued = start;
    sc->hasInterpQueued = 1;
    return makeToken(sc, TOK_STRING, bufCopy(sc));
}

// reads the next token from the input stream using a finite state machine
static Token readToken(Scanner *sc) {
    States state = ST_START;
    int commentDepth = 0;
    int commentNL = 0;
    int indentCount = 0;

    // handle any pre-queued interpolation tokens
    if (sc->hasInterpQueued == 1) {
        sc->hasInterpQueued = 0;
        return sc->interpQueued;
    }

    // handle a pending dot token that follows a numeric literal
    if (sc->dotAfterNumber == 1) {
        sc->dotAfterNumber = 0;
        return makeToken(sc, TOK_DOT, NULL);
    }

    // handle resumption states for identifiers, interpolation text, or line
    // starts
    if (sc->resumeId == 1) {
        sc->resumeId = 0;
        state = ST_INTERP_ID;
    } else if (sc->interpDepth > 0 &&
               sc->interpInExpr[sc->interpDepth - 1] == 0) {
        bufReset(sc);
        state = ST_INTERP_TEXT;
    } else if (sc->atLineStart == 1) {
        sc->atLineStart = 0;
        sc->lineTab = 0;
        state = ST_LINE_START;
    }

    while (1) {
        int character = getchar();

        switch (state) {
        case ST_START:

            if (character == EOF) {
                if (sc->interpDepth > 0) {
                    return lexicalError(sc, "unterminated interpolated string");
                }
                return makeToken(sc, TOK_EOF, NULL);
            }

            if (character == ' ' || character == '\t' || character == '\r')
                break;

            // handle NL, indentation checks, and line breaks inside parentheses
            if (character == '\n') {
                if (sc->interpDepth > 0 &&
                    sc->interpKind[sc->interpDepth - 1] == 0) {
                        return lexicalError(sc, "unterminated interpolated string");
                }
                if (sc->parenDepth > 0) {
                    if (sc->comma == 1) {
                        sc->commaNL = 1;
                    }
                    sc->line++;
                    indentCount = 0;
                    state = ST_PAREN_LINE;
                    break;
                }
                Token token = makeToken(sc, TOK_NL, NULL);
                sc->line++;
                sc->atLineStart = 1;
                return token;
            }

            // check for invalid trailing commas before a closing parenthesis
            if (character == ')' && sc->comma == 1 && sc->commaNL == 0) {
                return syntacticError(sc, "trailing comma must be followed by a new line before ')'");
            }
            if (character != '/') {
                sc->comma = 0;
                sc->commaNL = 0;
            }

            // identifier starting with a letter
            if (isalpha(character)) {
                bufReset(sc);
                bufAdd(sc, character);
                state = ST_ID;
                break;
            }
            // identifier starting with a underscore
            if (character == '_') {
                bufReset(sc);
                bufAdd(sc, character);
                state = ST_UNDERSCORE;
                break;
            }

            // operators that may have a second character
            if (character == '<') {
                state = ST_LESS;
                break;
            }
            if (character == '>') {
                state = ST_GREATER;
                break;
            }
            if (character == '=') {
                state = ST_ASSIGN;
                break;
            }
            if (character == '!') {
                state = ST_NEQ;
                break;
            }
            if (character == '/') {
                state = ST_SLASH;
                break;
            }
            if (character == '.') {
                state = ST_START_DOT;
                break;
            }
            if (character == '&') {
                state = ST_AMP;
                break;
            }
            if (character == '|') {
                state = ST_PIPE;
                break;
            }
            if (character == '@') {
                bufReset(sc);
                state = ST_AT;
                break;
            }
            if (character == '+') {
                state = ST_PLUS;
                break;
            }
            if (character == '-') {
                state = ST_MINUS;
                break;
            }

            // numeric literal
            if (isdigit(character)) {
                bufReset(sc);
                bufAdd(sc, character);
                if (character == '0') {
                    state = ST_ZERO;
                } else {
                    state = ST_INT;
                }
                break;
            }

            // string literal
            if (character == '"') {
                bufReset(sc);
                state = ST_STRING_START;
                break;
            }

            // end of an interpolated expression block inside a string
            if (character == '}' && sc->interpDepth > 0 &&
                sc->interpInExpr[sc->interpDepth - 1] == 1) {
                sc->interpInExpr[sc->interpDepth - 1] = 0;
                if (sc->parenDepth > 0) {
                    sc->parenDepth--;
                }
                return makeToken(sc, TOK_INTERP_EXPR_END, NULL);
            }

            // one character tokens
            switch (character) {
            case '(':
                sc->parenDepth++;
                return makeToken(sc, TOK_LPAR, NULL);

            case ')':
                if (sc->parenDepth > 0) {
                    sc->parenDepth--;
                }
                return makeToken(sc, TOK_RPAR, NULL);

            case ',': {
                sc->comma = 1;
                sc->commaNL = 0;
                return makeToken(sc, TOK_COMMA, NULL);
            }
            case ';':
                return makeToken(sc, TOK_SEMICOL, NULL);
            case ':':
                return makeToken(sc, TOK_COLON, NULL);
            case '*':
                return makeToken(sc, TOK_MUL, NULL);
            }

            return lexicalError(sc, "");

        // cases for general states
        case ST_UNDERSCORE:
            // check if the standalone underscore is followed by valid
            // identifier characters
            if (isalnum(character) || character == '_') {
                bufAdd(sc, character);
                state = ST_ID;
            } else {
                return syntacticError(sc, "a standalone underscore is not an identifier");
            }
            break;

        case ST_ID:
            // accumulate identifier characters or check for interpolated string
            // prefix 's"'
            if (isalnum(character) || character == '_') {
                bufAdd(sc, character);
            } else if (sc->bufLen == 1 && sc->buf[0] == 's' &&
                       character == '"') {
                state = ST_INTERP_OPEN;
            } else {
                ungetc(character, stdin); // Push back non-identifier character

                TokenType type = findKeyword(sc->buf);
                if (type == TOK_ID) {
                    return makeToken(sc, TOK_ID, bufCopy(sc));
                }
                return makeToken(sc, type, NULL);
            }
            break;

        case ST_LESS:
            if (character == '=') {
                return makeToken(sc, TOK_LESS_EQ, NULL);
            }
            ungetc(character, stdin);
            return makeToken(sc, TOK_LESS, NULL);

        case ST_GREATER:
            if (character == '=') {
                return makeToken(sc, TOK_GREATER_EQ, NULL);
            }
            ungetc(character, stdin);
            return makeToken(sc, TOK_GREATER, NULL);

        case ST_ASSIGN:
            if (character == '=') {
                return makeToken(sc, TOK_EQ, NULL);
            }
            ungetc(character, stdin);
            return makeToken(sc, TOK_ASSIGN, NULL);

        case ST_NEQ:
            if (character == '=') {
                return makeToken(sc, TOK_NEQ, NULL);
            }
            ungetc(character, stdin);
            return makeToken(sc, TOK_NOT, NULL);

        case ST_AMP:
            if (character == '&') {
                return makeToken(sc, TOK_AND, NULL);
            }
            return lexicalError(sc, "a standalone '&' character is not supported (expected '&&')");

        case ST_AT:
            // validate main function annotation (@main)
            if (isalnum(character) || character == '_') {
                bufAdd(sc, character);
            } else {
                ungetc(character, stdin);
                if (strcmp(sc->buf, "main") == 0) {
                    return makeToken(sc, TOK_AT_MAIN, NULL);
                }
                return syntacticError(sc, "@ must be immediately followed by main");
            }
            break;

        case ST_PLUS:
            // error in scala because two operators next to each other, like +-
            // would be joined into one operator
            if (character == '+' || character == '-') {
                return lexicalError(sc, "'+' and '-' must be separated by a space");
            }
            ungetc(character, stdin);
            return makeToken(sc, TOK_PLUS, NULL);

        case ST_MINUS:
            if (character == '+' || character == '-') {
                return lexicalError(sc, "'+' and '-' must be separated by a space");
            }
            ungetc(character, stdin);
            return makeToken(sc, TOK_MINUS, NULL);

        // cases for comment states
        case ST_SLASH:
            if (character == '/') {
                state = ST_LINE_COMMENT;
            } else if (character == '*') {
                commentDepth = 1;
                commentNL = 0;
                state = ST_BLOCK_COMMENT;
            } else {
                ungetc(character, stdin);
                return makeToken(sc, TOK_DIV, NULL);
            }
            break;

        case ST_PIPE:
            if (character == '|') {
                return makeToken(sc, TOK_OR, NULL);
            }
            return lexicalError(sc, "standalone '|' character is not supported (expected '||')");

        case ST_LINE_COMMENT:
            // consume characters until NL or EOF
            if (character == '\n' || character == EOF) {
                if (character != EOF) {
                    ungetc(character, stdin);
                }
                state = ST_START;
            }
            break;

        case ST_BLOCK_COMMENT:
            if (character == EOF) {
                return lexicalError(sc, "unterminated block comment");
            } else if (character == '\n') {
                sc->line++;
                commentNL = 1;
            } else if (character == '/') {
                state = ST_BLOCK_SLASH;
            } else if (character == '*') {
                state = ST_BLOCK_STAR;
            }
            break;

        case ST_BLOCK_SLASH:
            if (character == '*') {
                commentDepth++;
                state = ST_BLOCK_COMMENT;
            } else {
                ungetc(character, stdin);
                state = ST_BLOCK_COMMENT;
            }
            break;

        case ST_BLOCK_STAR:
            if (character == '/') {
                commentDepth--;
                if (commentDepth == 0) {
                    if (commentNL == 1 && sc->parenDepth == 0) {
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
            return lexicalError(sc, "a token cannot follow a multi-line comment");

        // cases for numeric literal states
        case ST_START_DOT:
            if (isdigit(character)) {
                bufReset(sc);
                bufAdd(sc, '.');
                bufAdd(sc, character);
                state = ST_DECIMAL;
            } else {
                ungetc(character, stdin);
                return makeToken(sc, TOK_DOT, NULL);
            }
            break;

        case ST_ZERO:
            if (isdigit(character)) {
                return lexicalError(sc, "a non-zero literal must not begin with a zero");
            } else if (character == '.') {
                bufAdd(sc, character);
                state = ST_DOT;
            } else if (character == 'e' || character == 'E') {
                bufAdd(sc, character);
                state = ST_EXP;
            } else if (character == 'd' || character == 'D') {
                state = ST_SUFFIX;
            } else {
                return endNumber(sc, TOK_INT, character);
            }
            break;

        case ST_INT:
            if (isdigit(character)) {
                bufAdd(sc, character);
            } else if (character == '_') {
                state = ST_INT_US;
            } else if (character == '.') {
                bufAdd(sc, character);
                state = ST_DOT;
            } else if (character == 'e' || character == 'E') {
                bufAdd(sc, character);
                state = ST_EXP;
            } else if (character == 'd' || character == 'D') {
                state = ST_SUFFIX;
            } else {
                return endNumber(sc, TOK_INT, character);
            }
            break;

        case ST_INT_US:
            if (isdigit(character)) {
                bufAdd(sc, character);
                state = ST_INT;
            } else if (character != '_') {
                return lexicalError(sc, "the underscode must be between figures");
            }
            break;

        case ST_DOT:
            if (isdigit(character)) {
                bufAdd(sc, character);
                state = ST_DECIMAL;
            } else {
                ungetc(character, stdin);
                sc->bufLen--;
                sc->buf[sc->bufLen] = '\0';
                sc->dotAfterNumber = 1;
                return makeToken(sc, TOK_INT, bufCopy(sc));
            }
            break;

        case ST_DECIMAL:
            if (isdigit(character)) {
                bufAdd(sc, character);
            } else if (character == '_') {
                state = ST_DECIMAL_US;
            } else if (character == 'e' || character == 'E') {
                bufAdd(sc, character);
                state = ST_EXP;
            } else if (character == 'd' || character == 'D') {
                state = ST_SUFFIX;
            } else {
                return endNumber(sc, TOK_DOUBLE, character);
            }
            break;

        case ST_DECIMAL_US:
            if (isdigit(character)) {
                bufAdd(sc, character);
                state = ST_DECIMAL;
            } else if (character != '_') {
                return lexicalError(sc, "the underscode must be between figures");
            }
            break;

        case ST_EXP:
            if (character == '+' || character == '-') {
                bufAdd(sc, character);
                state = ST_EXP_SIGN;
            } else if (isdigit(character)) {
                bufAdd(sc, character);
                state = ST_EXP_DIGITS;
            } else {
                return lexicalError(sc, "the exponent must contain a digit");
            }
            break;

        case ST_EXP_SIGN:
            if (isdigit(character)) {
                bufAdd(sc, character);
                state = ST_EXP_DIGITS;
            } else {
                return lexicalError(sc, "the exponent must contain a digit");
            }
            break;

        case ST_EXP_DIGITS:
            if (isdigit(character)) {
                bufAdd(sc, character);
            } else if (character == '_') {
                state = ST_EXP_US;
            } else if (character == 'd' || character == 'D') {
                state = ST_SUFFIX;
            } else {
                return endNumber(sc, TOK_DOUBLE, character);
            }
            break;

        case ST_EXP_US:
            if (isdigit(character)) {
                bufAdd(sc, character);
                state = ST_EXP_DIGITS;
            } else if (character != '_') {
                return lexicalError(sc, "the underscore must be between figures");
            }
            break;

        case ST_SUFFIX:
            return endNumber(sc, TOK_DOUBLE, character);

        // cases for string states
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
                return makeToken(sc, TOK_STRING, bufCopy(sc));
            } else if (character == '\\') {
                state = ST_ESCAPE;
            } else if (character == '\n' || character == EOF) {
                return lexicalError(sc, "unfinished string");
            } else {
                bufAdd(sc, character);
            }
            break;

        case ST_ESCAPE:
            if (character == 'n') {
                bufAdd(sc, '\n');
            } else if (character == 'r') {
                bufAdd(sc, '\r');
            } else if (character == 't') {
                bufAdd(sc, '\t');
            } else if (character == '\\') {
                bufAdd(sc, '\\');
            } else if (character == '"') {
                bufAdd(sc, '"');
            } else {
                return lexicalError(sc, "invalid escape sequence");
            }
            state = ST_STRING;
            break;

        case ST_EMPTY_STRING:
            if (character == '"') {
                state = ST_MUL_LINE_STRING;
            } else {
                ungetc(character, stdin);
                return makeToken(sc, TOK_STRING, bufCopy(sc));
            }
            break;

        case ST_MUL_LINE_STRING:
            // consume characters until the end of the line or file
            if (character == '"') {
                state = ST_MUL_LINE_QUOTE1;
            } else if (character == EOF) {
                return lexicalError(sc, "unterminated multi-row string");
            } else {
                if (character == '\n') {
                    sc->line++;
                }
                bufAdd(sc, character);
            }
            break;

        case ST_MUL_LINE_QUOTE1:
            if (character == '"') {
                state = ST_MUL_LINE_QUOTE2;
            } else {
                bufAdd(sc, '"');
                ungetc(character, stdin);
                state = ST_MUL_LINE_STRING;
            }
            break;

        case ST_MUL_LINE_QUOTE2:
            if (character == '"') {
                state = ST_MUL_LINE_QUOTE3;
            } else {
                bufAdd(sc, '"');
                bufAdd(sc, '"');
                ungetc(character, stdin);
                state = ST_MUL_LINE_STRING;
            }
            break;

        case ST_MUL_LINE_QUOTE3:
            if (character == '"') {
                bufAdd(sc, '"');
            } else {
                ungetc(character, stdin);
                return makeToken(sc, TOK_STRING, bufCopy(sc));
            }
            break;

        case ST_LINE_START:
            // measure leading indentation at the start of a logical line
            if (character == ' ') {
                indentCount++;
            } else if (character == '\t') {
                sc->lineTab = 1;
            } else if (character == '\r') {

            } else {
                ungetc(character, stdin);
                sc->lineIndent = indentCount;
                state = ST_START;
            }
            break;

        case ST_PAREN_LINE:
            // count spaces on continuation lines inside parentheses
            if (character == ' ') {
                indentCount++;
            } else if (character == '\t' || character == '\r') {
                // bílý znak
            } else if (character == '\n') {
                sc->line++;
                indentCount = 0;
            } else {
                if (character != EOF && indentCount <= sc->lineIndent) {
                    return syntacticError(sc, "the code inside the parentheses must be indented further than the start of the command");
                }
                ungetc(character, stdin);
                state = ST_START;
            }
            break;

        // cases for interpolated string states
        case ST_INTERP_OPEN:
            if (character == '"') {
                state = ST_INTERP_OPEN2;
            } else {
                ungetc(character, stdin);
                pushInterp(sc, 0);
                return makeToken(sc, TOK_INTERP_START, NULL);
            }
            break;

        case ST_INTERP_OPEN2:
            if (character == '"') {
                pushInterp(sc, 1);
                return makeToken(sc, TOK_INTERP_START, NULL);
            }
            ungetc(character, stdin);
            sc->interpQueued = makeToken(sc, TOK_INTERP_END, NULL);
            sc->hasInterpQueued = 1;
            return makeToken(sc, TOK_INTERP_START, NULL);

        case ST_INTERP_TEXT:
            // parse text segments and handle variable interpolations within
            // strings
            if (character == EOF) {
                return lexicalError(sc, "unterminated interpolated string");
            } else if (character == '$') {
                state = ST_INTERP_DOLLAR;
            } else if (sc->interpKind[sc->interpDepth - 1] == 0) {
                // one-line string
                if (character == '"') {
                    return endInterpolation(sc);
                } else if (character == '\\') {
                    state = ST_INTERP_ESCAPE;
                } else if (character == '\n') {
                    return lexicalError(sc, "unterminated interpolated string");
                } else {
                    bufAdd(sc, character);
                }
            } else {
                // multi-line string, everything is literal
                if (character == '"') {
                    state = ST_INTERP_QUOTE1;
                } else {
                    if (character == '\n') {
                        sc->line++;
                    }
                    bufAdd(sc, character);
                }
            }
            break;

        case ST_INTERP_ESCAPE:
            if (character == 'n') {
                bufAdd(sc, '\n');
            } else if (character == 'r') {
                bufAdd(sc, '\r');
            } else if (character == 't') {
                bufAdd(sc, '\t');
            } else if (character == '\\') {
                bufAdd(sc, '\\');
            } else if (character == '"') {
                bufAdd(sc, '"');
            } else {
                return lexicalError(sc, "invalid escape sequence");
            }
            state = ST_INTERP_TEXT;
            break;

        case ST_INTERP_DOLLAR:
            // handle escaped dollar signs ($$) or expression interpolations
            // (${...})
            if (character == '$') {
                bufAdd(sc, '$');
                state = ST_INTERP_TEXT;
            } else if (character == '{') {
                return startExpression(sc);
            } else if (isalpha(character) || character == '_') {
                if (sc->bufLen == 0) {
                    bufAdd(sc, character);
                    state = ST_INTERP_ID;
                } else {
                    // first return the text before $, then continue with the
                    // identifier
                    char *text = bufCopy(sc);
                    bufReset(sc);
                    bufAdd(sc, character);
                    sc->resumeId = 1;
                    return makeToken(sc, TOK_STRING, text);
                }
            } else {
                return lexicalError(sc, "after $ expected $, { or identifier");
            }
            break;

        case ST_INTERP_ID:
            if (isalnum(character) || character == '_') {
                bufAdd(sc, character);
            } else {
                ungetc(character, stdin);
                return makeToken(sc, TOK_ID, bufCopy(sc));
            }
            break;

        case ST_INTERP_QUOTE1:
            if (character == '"') {
                state = ST_INTERP_QUOTE2;
            } else {
                bufAdd(sc, '"');
                ungetc(character, stdin);
                state = ST_INTERP_TEXT;
            }
            break;

        case ST_INTERP_QUOTE2:
            if (character == '"') {
                state = ST_INTERP_QUOTE3;
            } else {
                bufAdd(sc, '"');
                bufAdd(sc, '"');
                ungetc(character, stdin);
                state = ST_INTERP_TEXT;
            }
            break;

        case ST_INTERP_QUOTE3:
            if (character == '"') {
                bufAdd(sc, '"');
            } else {
                ungetc(character, stdin);
                return endInterpolation(sc);
            }
            break;
        }
    }
}

// AI(cloude): generate function for inicialization of scanner
Scanner *scannerInit(void) {
    Scanner *sc = calloc(1, sizeof *sc);
    if (sc == NULL) {
        exit(99);
    }
    sc->line = 1;
    sc->atLineStart = 1;
    bufReset(sc);
    return sc;
}

// AI(cloude): generate function for realeasing scanner
//  releases the lexer and its buffer
void scannerFree(Scanner *sc) {
    if (sc == NULL) {
        return;
    }
    free(sc->buf);
    free(sc);
}

// Returns the next token, including TOK_INDENT and TOK_DEDENT
Token getToken(Scanner *sc) {
    sc->error = 0;
    while (1) {
        if (sc->sendDedents > 0) {
            sc->sendDedents--;
            return makeToken(sc, TOK_DEDENT, NULL);
        }

        if (sc->hasQueued == 1) {
            sc->hasQueued = 0;
            return sc->queued;
        }

        Token token = readToken(sc);
        if (sc->error != 0)
        {
            return token;
        }

        if (token.type == TOK_SEMICOL) {
            if (sc->afterSemicolon == 1) {
                continue; // repeated ';' is dropped
            }
            sc->afterSemicolon = 1;
        } else {
            sc->afterSemicolon = 0;
        }

        if (token.type == TOK_NL) {
            if (sc->lineHasToken == 0) {
                continue;
            }
            sc->lineHasToken = 0;
            return token;
        }

        if (token.type == TOK_EOF) {
            sc->sendDedents = sc->indentTop;
            sc->indentTop = 0;
            sc->queued = token;
            sc->hasQueued = 1;
            if (sc->lineHasToken == 1) {
                // the last line has no LF
                sc->lineHasToken = 0;
                return makeToken(sc, TOK_NL, NULL);
            }
            continue;
        }

        if (sc->lineHasToken == 0) {
            // first token of a logical line: compare the indentation
            sc->lineHasToken = 1;
            if (sc->lineTab == 1) {
                return lexicalError(sc, "tab character in indentation");
            }
            if (sc->lineIndent > sc->indentStack[sc->indentTop]) {
                if (sc->indentTop + 1 >= MAX_INDENT) {
                    exit(99);
                }
                sc->indentTop++;
                sc->indentStack[sc->indentTop] = sc->lineIndent;
                sc->queued = token;
                sc->hasQueued = 1;
                return makeToken(sc, TOK_INDENT, NULL);
            }
            if (sc->lineIndent < sc->indentStack[sc->indentTop]) {
                while (sc->indentTop > 0 &&
                       sc->indentStack[sc->indentTop] > sc->lineIndent) {
                    sc->indentTop--;
                    sc->sendDedents++;
                }
                if (sc->indentStack[sc->indentTop] != sc->lineIndent) {
                    return lexicalError(sc, "inconsistent indentation");
                }
                sc->queued = token;
                sc->hasQueued = 1;
                continue;
            }
        }
        return token;
    }
}
/*** End of file scanner.c ***/
