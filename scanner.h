//Project IFJ26
//author: xoprsae00
//last change: 18.10.2026

#ifndef SCANNER_H
#define SCANNER_H

typedef enum {
    TOK_EOF, TOK_EOL, TOK_INDENT, TOK_DEDENT,
    TOK_ID, TOK_KEYWORD,
    TOK_INT, TOK_DOUBLE, TOK_STRING,

    //tokens for operators
    TOK_PLUS, TOK_MINUS, TOK_MUL, TOK_DIV,
    TOK_LESS, TOK_LESS_EQ, TOK_GREATER, TOK_GREATER_EQ,
    TOK_EQ, TOK_NEQ, TOK_ASSIGN,

    //tokens for separators
    TOK_LPAR, TOK_RPAR, TOK_COMMA, TOK_COLON, TOK_SEMICOL,
    TOK_DOT, TOK_AT,

    //tokens for extentions
    TOK_AND, TOK_OR, TOK_NOT, TOK_PLUSPLUS
}TokenType;

typedef enum {
    KW_DEF, KW_DO, KW_DOUBLE, KW_ELSE, KW_IF, KW_IMPORT,
    KW_INT, KW_RETURN, KW_STRING, KW_THEN, KW_UNIT, KW_VAL,
    KW_VAR, KW_WHILE
}Keyword;

//TO DO: vyřešit provedení struktury, předává "5"
typedef struct {
    TokenType type;
    char *str; //identifikátor/řetězec
    Keyword kw;
    int line;
}Token;

Token getToken(void);


#endif
