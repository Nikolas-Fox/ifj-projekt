/******************************************************************************
* Project:		IFJ
* Package:		Lexical analysis
* File:			scanner.c
* Date:			18.10.2026
* Last change:	12.11.2026
* Author: 		Eliška Opršalová xoprsae00
*
* Description: implementation of lexer - implemented as a deterministic finite automat            
*
*******************************************************************************/
/**
* @file scanner.c
*
* @brief implementation of lexer
*
* @author Eliška Opršalová xoprsae00
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "scanner.h"

//auromat states
typedef enum {
    // general states
    ST_START, ST_UNDERSCORE, ST_ID,
    ST_LESS, ST_GREATER, ST_ASSIGN,
    ST_NEQ, ST_AT, ST_PLUS, ST_MINUS,
    ST_AMP, ST_PIPE, 
    
    // states for comments
    ST_SLASH, ST_LINE_COMMENT, ST_BLOCK_COMMENT,        
    ST_BLOCK_SLASH, ST_BLOCK_STAR, ST_AFTER_BLOCK,
      
    // states for numeric literals
    ST_START_DOT, ST_ZERO, ST_INT,
    ST_INT_US, ST_DOT, ST_DECIMAL,
    ST_DECIMAL_US, ST_EXP, ST_EXP_SIGN,
    ST_EXP_DIGITS,  ST_EXP_US, ST_SUFFIX,
    
    // states for string
    ST_STRING_START, ST_STRING, ST_ESCAPE, ST_EMPTY_STRING,
    ST_MUL_LINE_STRING, ST_MUL_LINE_QUOTE1, ST_MUL_LINE_QUOTE2,
    ST_MUL_LINE_QUOTE3, ST_LINE_START, ST_PAREN_LINE, // counts spaces on the next line inside parentheses
    
    // interpolated strings
    ST_INTERP_OPEN, ST_INTERP_OPEN2, ST_INTERP_TEXT,
    ST_INTERP_ESCAPE, ST_INTERP_DOLLAR, ST_INTERP_ID,
    ST_INTERP_QUOTE1, ST_INTERP_QUOTE2, ST_INTERP_QUOTE3
    
}States;

static int line = 1;

static int atLineStart = 1;     // 1 = starts a new logical line
static int parenDepth = 0;   
static int lineIndent = 0;    // number of spaces at the beginning of the current logical line
static int lineTab = 0;       // 1 = indentation contains a tab

static int dotAfterNumber = 0;

#define MAX_INTERP 32
static int interpDepth = 0;              // how many interpolated strings are open
static int interpKind[MAX_INTERP];       // 0 = s"...", 1 = s"""..."""
static int interpInExpr[MAX_INTERP];    // 1 = inside ${ ... }
static Token interpQueued;               // token waiting for the next call
static int hasInterpQueued = 0;
static int resumeId = 0;              // 1 = continue reading the identifier after $

static int comma = 0;  // 1 = the last token was a comma
static int commaNL = 0;  // 1 = a newline has appeared since that comma

// Buffer for the lexeme text
// AI (Claude): vygeneruj mi funkce pro dynamickou práci s buffrem
// functions slightly after generated
static char *buf = NULL;
static size_t bufLen = 0;
static size_t bufCap = 0;

// AI (Claude): same prompt
static void bufReset(void) {
    if (buf == NULL) {
        bufCap = 64;
        buf = malloc(bufCap);
        if (buf == NULL) {
            exit(99);    
        }
    }
    bufLen = 0;
    buf[0] = '\0';
}

//AI (Claude): same prompt
static void bufAdd(int c) {
    if (bufLen + 2 > bufCap) {
        size_t newCap = bufCap * 2;
        char *newBuf = realloc(buf, newCap);
        if (newBuf == NULL) exit(99);
        buf = newBuf;
        bufCap = newCap;
    }
    buf[bufLen++] = (char)c;
    buf[bufLen] = '\0';
}
 
//copy of the buffer for the token (the token owns its text)
//AI (Claude): same prompt
static char *bufCopy(void) {
    char *copy = malloc(bufLen + 1);
    if (!copy) exit(99);
    memcpy(copy, buf, bufLen + 1);
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
    {"String", TOK_KW_STRING}, {"Unit", TOK_KW_UNIT},   {"Boolean", TOK_KW_BOOLEAN},
    {"extension", TOK_KW_EXTENSION},
    
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

static Token makeToken(TokenType type, char *str) {
    Token tok = {0};
    tok.type = type;
    tok.str = str;
    tok.line = line;
    return tok;
}

static void lexError(const char *message) {
       fprintf(stderr, "Lexical error on line %d: %s\n", line, message);
       exit(1);
   }

// checks the character after a numeric literal, pushes back the excess character, and creates the token
static Token endNumber(TokenType type, int character) {
    if (isalnum(character) || character == '_') {
        lexError("invalid character after numeric literal");
    }
    ungetc(character, stdin);
    return makeToken(type, bufCopy());
}

// opens an interpolated string (kind 0 = one line, 1 = multi-line)
// INTERPOLATEDS extention AI (Claude): explained and partialy generated
// prompts:
// vysvětli mi jak funguje rozšíření INTERPOLATEDS
// navrni mi kostru, jak by to zhruba mělo vypadat
static void pushInterp(int kind) {
    if (interpDepth + 1 >= MAX_INTERP) {
        exit(99);
    }
    interpKind[interpDepth] = kind;
    interpInExpr[interpDepth] = 0;
    interpDepth++;
}

// closes an interpolated string: returns the pending text (if any) and then TOK_INTERP_END
static Token endInterpolation(void) {
    interpDepth--;
    Token end = makeToken(TOK_INTERP_END, NULL);
    if (bufLen == 0) {
        return end;
    }
    interpQueued = end;
    hasInterpQueued = 1;
    return makeToken(TOK_STRING, bufCopy());
}

// starts ${ ... }: returns the pending text (if any) and then TOK_INTERP_EXPR_START
static Token startExpression(void) {
    interpInExpr[interpDepth - 1] = 1;
    parenDepth++;  // a line break inside ${ } is only whitespace
    Token start = makeToken(TOK_INTERP_EXPR_START, NULL);
    if (bufLen == 0) {
        return start;
    }
    interpQueued = start;
    hasInterpQueued = 1;
    return makeToken(TOK_STRING, bufCopy());
}

// reads the next token from the input stream using a finite state machine
static Token readToken(void) {
    States state = ST_START;
    int commentDepth = 0;
    int commentNL = 0;
    int indentCount = 0;

    // handle any pre-queued interpolation tokens
    if (hasInterpQueued == 1) {
        hasInterpQueued = 0;
        return interpQueued;
    }

    // handle a pending dot token that follows a numeric literal
    if (dotAfterNumber == 1) {
        dotAfterNumber = 0;
        return makeToken(TOK_DOT, NULL);
    }

    // handle resumption states for identifiers, interpolation text, or line starts
    if (resumeId == 1) {
        resumeId = 0;
        state = ST_INTERP_ID;
    } else if (interpDepth > 0 && interpInExpr[interpDepth - 1] == 0) {
        bufReset();
        state = ST_INTERP_TEXT;
    } else if (atLineStart == 1) {
        atLineStart = 0;
        lineTab = 0;
        state = ST_LINE_START;
    }

    while(1) {
        int character = getchar();

        switch (state) {
            case ST_START:

                if(character == EOF) {
                    if (interpDepth > 0) {
                        lexError("unterminated interpolated string");
                    }
                    return makeToken(TOK_EOF, NULL);
                }  

                if (character == ' ' || character == '\t' || character == '\r') break;

                // handle NL, indentation checks, and line breaks inside parentheses
                if (character == '\n') {
                    if (interpDepth > 0 && interpKind[interpDepth - 1] == 0) {
                        lexError("unterminated interpolated string");
                    }
                    if (parenDepth > 0) {
                        if (comma == 1) {
                            commaNL = 1;
                        }
                        line++;
                        indentCount = 0;
                        state = ST_PAREN_LINE;
                        break;
                    }
                    Token token = makeToken(TOK_NL, NULL);
                    line++;
                    atLineStart = 1;
                    return token;
                }

                // check for invalid trailing commas before a closing parenthesis
                if (character == ')' && comma == 1 && commaNL == 0) {
                    fprintf(stderr, "trailing comma must be followed by a newline before ')'");
                    exit(2);
                }
                if (character != '/') {
                    comma = 0;
                    commaNL = 0;
}

                // identifier starting with a letter
                if (isalpha(character)) {
                    bufReset();
                    bufAdd(character);
                    state = ST_ID;
                    break;
                }
                // identifier starting with a underscore
                if (character == '_') {
                    bufReset();
                    bufAdd(character);
                    state = ST_UNDERSCORE;
                    break;
                }

                // operators that may have a second character
                if (character == '<') { state = ST_LESS; break; }
                if (character == '>') { state = ST_GREATER; break; }
                if (character == '=') { state = ST_ASSIGN; break; }
                if (character == '!') { state = ST_NEQ; break; }
                if (character == '/') { state = ST_SLASH; break; }
                if (character == '.') { state = ST_START_DOT; break; }
                if (character == '&') { state = ST_AMP; break; }
                if (character == '|') { state = ST_PIPE; break; }
                if (character == '@') { bufReset(); state = ST_AT; break; }
                if (character == '+') { state = ST_PLUS; break; }
                if (character == '-') { state = ST_MINUS; break; }

                // numeric literal
                if (isdigit(character)) {
                    bufReset();
                    bufAdd(character);
                    if (character == '0') {
                        state = ST_ZERO;
                    } else {
                        state = ST_INT;
                    }
                    break;
                }

                // string literal
                if (character == '"') {
                    bufReset();
                    state = ST_STRING_START;
                    break;
                }

                // end of an interpolated expression block inside a string
                if (character == '}' && interpDepth > 0 && interpInExpr[interpDepth - 1] == 1) {
                    interpInExpr[interpDepth - 1] = 0;
                    if (parenDepth > 0) { 
                        parenDepth--;
                    }
                    return makeToken(TOK_INTERP_EXPR_END, NULL);
                }

                //one character tokens
                switch (character) {
                    case '(':
                        parenDepth++;
                        return makeToken(TOK_LPAR, NULL);

                    case ')': 
                         if (parenDepth > 0) {
                            parenDepth--;
                        }
                        return makeToken(TOK_RPAR, NULL);

                    case ',': {
                        comma = 1;
                        commaNL = 0;
                        return makeToken(TOK_COMMA, NULL);
                    } 
                    case ';': return makeToken(TOK_SEMICOL, NULL);
                    case ':': return makeToken(TOK_COLON, NULL);
                    case '*': return makeToken(TOK_MUL, NULL);
                }
                
                lexError("lexical error");

                break;

            //cases for general states
            case ST_UNDERSCORE:
            // check if the standalone underscore is followed by valid identifier characters
                if (isalnum(character) || character == '_') {
                    bufAdd(character);
                    state = ST_ID;
                } 
                else {
                    lexError("A standalone underscore is not an identifier");
                }
                break;

            case ST_ID:
                // accumulate identifier characters or check for interpolated string prefix 's"'
                if (isalnum(character) || character == '_') {
                    bufAdd(character);
                } else if (bufLen == 1 && buf[0] == 's' && character == '"') {
                    state = ST_INTERP_OPEN; 
                }else {
                    ungetc(character, stdin);  // Push back non-identifier character

                    TokenType type = findKeyword(buf);
                    if (type == TOK_ID) {
                        return makeToken(TOK_ID, bufCopy());
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
                ungetc(character, stdin);
                return makeToken(TOK_NOT, NULL);

            case ST_AMP:
                if (character == '&') {
                    return makeToken(TOK_AND, NULL);
                }
                lexError("A standalone '&' character is not supported (expected '&&')");
                break;

            case ST_AT:
            // validate main function annotation (@main)
                if (isalnum(character) || character == '_') {
                    bufAdd(character);
                } else {
                    ungetc(character, stdin);
                    if (strcmp(buf, "main") == 0) {
                        return makeToken(TOK_AT_MAIN, NULL);
                    }
                    fprintf(stderr, "@ must be immediately followed by main");
                    exit(2);
                }
                break;

            case ST_PLUS:
                if (character == '+' || character == '-') {
                    lexError("'+' and '-' must be separated by a space");
                }
                ungetc(character, stdin);
                return makeToken(TOK_PLUS, NULL);

            case ST_MINUS:
                if (character == '+' || character == '-') {
                    lexError("'+' and '-' must be separated by a space");
                }
                ungetc(character, stdin);
                return makeToken(TOK_MINUS, NULL);
            
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
                    return makeToken(TOK_DIV, NULL);
                }
                break;

            case ST_PIPE:
                if (character == '|') {
                    return makeToken(TOK_OR, NULL);
                }
                lexError("A standalone '|' character is not supported (expected '||')");
                break;

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
                    lexError("unterminated block comment");
                } else if (character == '\n') {
                    line++;
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
                        if (commentNL == 1 && parenDepth == 0) {
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
                lexError("A token cannot follow a multi-line comment.");
                break;

            // cases for numeric literal states
            case ST_START_DOT:
                if (isdigit(character)) {
                    bufReset();
                    bufAdd('.');
                    bufAdd(character);
                    state = ST_DECIMAL;
                } else {
                    ungetc(character, stdin);
                    return makeToken(TOK_DOT, NULL);
                }
                break;

            case ST_ZERO:
                if (isdigit(character)) {
                    lexError("A non-zero literal must not begin with a zero.");
                } else if (character == '.') {
                    bufAdd(character);
                    state = ST_DOT;
                } else if (character == 'e' || character == 'E') {
                    bufAdd(character);
                    state = ST_EXP;
                } else if (character == 'd' || character == 'D') {
                    state = ST_SUFFIX;
                } else {
                    return endNumber(TOK_INT, character);
                }
                break;

            case ST_INT:
                if (isdigit(character)) {
                    bufAdd(character);
                } else if (character == '_') {
                    state = ST_INT_US;
                } else if (character == '.') {
                    bufAdd(character);
                    state = ST_DOT;
                } else if (character == 'e' || character == 'E') {
                    bufAdd(character);
                    state = ST_EXP;
                } else if (character == 'd' || character == 'D') {
                    state = ST_SUFFIX;
                } else {
                    return endNumber(TOK_INT, character);
                }
                break;

            case ST_INT_US:
                if (isdigit(character)) {
                    bufAdd(character);
                    state = ST_INT;
                } else if (character != '_') {
                    lexError("the underscore must be between figures");
                }
                break;

            case ST_DOT:
                if (isdigit(character)) {
                    bufAdd(character);
                    state = ST_DECIMAL;
                } else {
                    ungetc(character, stdin);
                bufLen--;
                buf[bufLen] = '\0';
                dotAfterNumber = 1;
                return makeToken(TOK_INT, bufCopy());
                }
                break;

            case ST_DECIMAL:
                if (isdigit(character)) {
                    bufAdd(character);
                } else if (character == '_') {
                    state = ST_DECIMAL_US;
                } else if (character == 'e' || character == 'E') {
                    bufAdd(character);
                    state = ST_EXP;
                } else if (character == 'd' || character == 'D') {
                    state = ST_SUFFIX;
                } else {
                    return endNumber(TOK_DOUBLE, character);
                }
                break;

            case ST_DECIMAL_US:
                if (isdigit(character)) {
                    bufAdd(character);
                    state = ST_DECIMAL;
                } else if (character != '_') {
                    lexError("the underscore must be between figures");
                }
                break;

            case ST_EXP:
                if (character == '+' || character == '-') {
                    bufAdd(character);
                    state = ST_EXP_SIGN;
                } else if (isdigit(character)) {
                    bufAdd(character);
                    state = ST_EXP_DIGITS;
                } else {
                    lexError("The exponent must contain a digit");
                }
                break;

            case ST_EXP_SIGN:
                if (isdigit(character)) {
                    bufAdd(character);
                    state = ST_EXP_DIGITS;
                } else {
                    lexError("The exponent must contain a digit");
                }
                break;

            case ST_EXP_DIGITS:
                if (isdigit(character)) {
                    bufAdd(character);
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
                    bufAdd(character);
                    state = ST_EXP_DIGITS;
                } else if (character != '_') {
                    lexError("the underscore must be between figures");
                }
                break;

            case ST_SUFFIX:
                return endNumber(TOK_DOUBLE, character);

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
                    return makeToken(TOK_STRING, bufCopy());
                } else if (character == '\\') {
                    state = ST_ESCAPE;
                } else if (character == '\n' || character == EOF) {
                    lexError("unfinished string");
                } else {
                    bufAdd(character);
                }
                break;

            case ST_ESCAPE:
                if (character == 'n') {
                    bufAdd('\n');
                } else if (character == 'r') {
                    bufAdd('\r');
                } else if (character == 't') {
                    bufAdd('\t');
                } else if (character == '\\') {
                    bufAdd('\\');
                } else if (character == '"') {
                    bufAdd('"');
                } else {
                    lexError("invalid escape sequence");
                }
                state = ST_STRING;
                break;
            
            case ST_EMPTY_STRING:
                if (character == '"') {
                    state = ST_MUL_LINE_STRING;
                } else {
                    ungetc(character, stdin);
                    return makeToken(TOK_STRING, bufCopy());
                }
                break;

            case ST_MUL_LINE_STRING:
            // consume characters until the end of the line or file
                if (character == '"') {
                    state = ST_MUL_LINE_QUOTE1;
                } else if (character == EOF) {
                    lexError("unterminated multi-row string");
                } else {
                    if (character == '\n') {
                        line++;
                    }
                    bufAdd(character);
                }
                break;

            case ST_MUL_LINE_QUOTE1:
                if (character == '"') {
                    state = ST_MUL_LINE_QUOTE2;
                } else {
                    bufAdd('"');
                    ungetc(character, stdin);
                    state = ST_MUL_LINE_STRING;
                }
                break;

            case ST_MUL_LINE_QUOTE2:
                if (character == '"') {
                    state = ST_MUL_LINE_QUOTE3;
                } else {
                    bufAdd('"');
                    bufAdd('"');
                    ungetc(character, stdin);
                    state = ST_MUL_LINE_STRING;
                }
                break;

            case ST_MUL_LINE_QUOTE3:
                if (character == '"') {
                    bufAdd('"');
                } else {
                    ungetc(character, stdin);
                    return makeToken(TOK_STRING, bufCopy());
                }
                break;

            case ST_LINE_START:
            // measure leading indentation at the start of a logical line
                if (character == ' ') {
                    indentCount++;
                } else if (character == '\t') {
                    lineTab = 1;
                } else if (character == '\r') {

                } else {
                    ungetc(character, stdin);
                    lineIndent = indentCount;
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
                    line++;
                    indentCount = 0;
                } else {
                    if (character != EOF && indentCount <= lineIndent) {
                        lexError("The code inside the parentheses must be indented further than the start of the command.");
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
                    pushInterp(0);
                    return makeToken(TOK_INTERP_START, NULL);
                }
                break;

            case ST_INTERP_OPEN2:
                if (character == '"') {
                    pushInterp(1);
                    return makeToken(TOK_INTERP_START, NULL);
                }
                ungetc(character, stdin);
                interpQueued = makeToken(TOK_INTERP_END, NULL);
                hasInterpQueued = 1;
                return makeToken(TOK_INTERP_START, NULL);
 
            case ST_INTERP_TEXT:
            // parse text segments and handle variable interpolations within strings
                if (character == EOF) {
                    lexError("unterminated interpolated string");
                } else if (character == '$') {
                    state = ST_INTERP_DOLLAR;
                } else if (interpKind[interpDepth - 1] == 0) {
                    // one-line string
                    if (character == '"') {
                        return endInterpolation();
                    } else if (character == '\\') {
                        state = ST_INTERP_ESCAPE;
                    } else if (character == '\n') {
                        lexError("unterminated interpolated string");
                    } else {
                        bufAdd(character);
                    }
                } else {
                    // multi-line string, everything is literal
                    if (character == '"') {
                        state = ST_INTERP_QUOTE1;
                    } else {
                        if (character == '\n') {
                            line++;
                        }
                        bufAdd(character);
                    }
                }
                break;

            case ST_INTERP_ESCAPE:
                if (character == 'n') {
                    bufAdd('\n');
                } else if (character == 'r') {
                    bufAdd('\r');
                } else if (character == 't') {
                    bufAdd('\t');
                } else if (character == '\\') {
                    bufAdd('\\');
                } else if (character == '"') {
                    bufAdd('"');
                } else {
                    lexError("invalid escape sequence");
                }
                state = ST_INTERP_TEXT;
                break;

            case ST_INTERP_DOLLAR:
            // handle escaped dollar signs ($$) or expression interpolations (${...})
                if (character == '$') {
                    bufAdd('$');
                    state = ST_INTERP_TEXT;
                } else if (character == '{') {
                    return startExpression();
                } else if (isalpha(character) || character == '_') {
                    if (bufLen == 0) {
                        bufAdd(character);
                        state = ST_INTERP_ID;
                    } else {
                        // first return the text before $, then continue with the identifier
                        char *text = bufCopy();
                        bufReset();
                        bufAdd(character);
                        resumeId = 1;
                        return makeToken(TOK_STRING, text);
                    }
                } else {
                    lexError("after $ expected $, { or identifier");
                }
                break;

            case ST_INTERP_ID:
                if (isalnum(character) || character == '_') {
                    bufAdd(character);
                } else {
                    ungetc(character, stdin);
                    return makeToken(TOK_ID, bufCopy());
                }
                break;
 
            case ST_INTERP_QUOTE1:
                if (character == '"') {
                    state = ST_INTERP_QUOTE2;
                } else {
                    bufAdd('"');
                    ungetc(character, stdin);
                    state = ST_INTERP_TEXT;
                }
                break;
 
            case ST_INTERP_QUOTE2:
                if (character == '"') {
                    state = ST_INTERP_QUOTE3;
                } else {
                    bufAdd('"');
                    bufAdd('"');
                    ungetc(character, stdin);
                    state = ST_INTERP_TEXT;
                }
                break;
 
            case ST_INTERP_QUOTE3:
                if (character == '"') {
                    bufAdd('"');
                } else {
                    ungetc(character, stdin);
                    return endInterpolation();
                }
                break;
            }
        }   
}

#define MAX_INDENT 128
static int indentStack[MAX_INDENT] = {0};
static int indentTop = 0;        // index of the stack top (0 = no indentation)
static int sendDedents = 0;   // how many DEDENT tokens are still to be returned
static int hasQueued = 0;        // 1 = a token is waiting in the queue
static Token queued;              // token waiting behind EOL / INDENT / DEDENT
static int lineHasToken = 0;    // has the current logical line already had a token?
static int afterSemicolon = 0;    // 1 = the previous token was ';'

// Returns the next token, including TOK_INDENT and TOK_DEDENT
Token getToken(void) {
    while (1) {     
        if (sendDedents > 0) {
            sendDedents--;
            return makeToken(TOK_DEDENT, NULL);
        }

        if (hasQueued == 1) {
            hasQueued = 0;
            return queued;
        }

        Token token = readToken();

        if (token.type == TOK_SEMICOL) {
            if (afterSemicolon == 1) {
                continue; // repeated ';' is dropped
            }
            afterSemicolon = 1;
        } else {
            afterSemicolon = 0;
        }

        if (token.type == TOK_NL) {
            if (lineHasToken == 0) {
                continue;
            }
            lineHasToken = 0;
            return token;
        }

        if (token.type == TOK_EOF) {
            sendDedents = indentTop;
            indentTop = 0;
            queued = token;
            hasQueued = 1;
            if (lineHasToken == 1) {
                // the last line has no LF
                lineHasToken = 0;
                return makeToken(TOK_NL, NULL);
            }
            continue;
        }

        if (lineHasToken == 0) {
            // first token of a logical line: compare the indentation
            lineHasToken = 1;
            if (lineTab == 1) {
                lexError("tab character in indentation");
            }
            if (lineIndent > indentStack[indentTop]) {
                if (indentTop + 1 >= MAX_INDENT) {
                    exit(99);
                }
                indentTop++;
                indentStack[indentTop] = lineIndent;
                queued = token;
                hasQueued = 1;
                return makeToken(TOK_INDENT, NULL);
            }
            if (lineIndent < indentStack[indentTop]) {
                while (indentTop > 0 && indentStack[indentTop] > lineIndent) {
                    indentTop--;
                    sendDedents++;
                }
                if (indentStack[indentTop] != lineIndent) {
                    lexError("inconsistent indentation");
                }
                queued = token;
                hasQueued = 1;
                continue;
            }
        }
        return token;
    }
}

/*** End of file scanner.c ***/