//Project IFJ26
//author: xoprsae00
//last change: 18.10.2026

#ifndef SCANNER_H
#define SCANNER_H

typedef enum {
    TOK_EOF, TOK_EOL, TOK_INDENT, TOK_DEDENT,
    TOK_IDENT, TOK_KEYWORD,
    TOK_INT, TOK_DOUBLE, TOK_STRING,

    //tokens for operators
    TOK_PLUS, TOK_MINUS, TOK_MUL, TOK_DIV,
    TOK_LESS, TOK_LESS_EQ, TOK_GRATER, TOK_GRATER_EQ,
    TOK_EQ, TOK_NEQ, TOK_ASSIGN,

    //tokens for separators
    TOK_LPAR, TOK_RPAR, TOK_COMMA, TOK_COLON, TOK_SEMICOL,
    TOK_DOT, TOK_AT,

    //tokens for extentions
    TOK_AND, TOK_OR, TOK_NOT, TOK_PLUSPLUS
}TokenType;


#endif
