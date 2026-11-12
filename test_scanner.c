//AI generated with Gemini
// prompt: vygeneruj mi jednotkové testy pro tento soubor
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "scanner.h"

typedef struct {
    TokenType type;
    char *str;
    int line;
} TestToken;

static int collect_tokens(const char *input, TestToken *out_tokens, int max_tokens) {
    FILE *tmp = tmpfile();
    if (!tmp) return -1;
    fputs(input, tmp);
    rewind(tmp);

    FILE *old_stdin = stdin;
    stdin = tmp;

    int count = 0;
    while (count < max_tokens) {
        Token t = getToken();
        out_tokens[count].type = t.type;
        out_tokens[count].str = t.str;
        out_tokens[count].line = t.line;
        if (t.type == TOK_EOF) {
            count++;
            break;
        }
        count++;
    }

    stdin = old_stdin;
    fclose(tmp);
    return count;
}

void test_keywords_and_identifiers(void) {
    printf("Running test_keywords_and_identifiers...\n");
    const char *input = "def val var Int Double true false custom_id";
    TestToken tokens[32];
    int n = collect_tokens(input, tokens, 32);
    assert(n > 0);
    assert(tokens[0].type == TOK_KW_DEF);
    assert(tokens[1].type == TOK_KW_VAL);
    assert(tokens[2].type == TOK_KW_VAR);
    assert(tokens[3].type == TOK_KW_INT);
    assert(tokens[4].type == TOK_KW_DOUBLE);
    assert(tokens[5].type == TOK_KW_TRUE);
    assert(tokens[6].type == TOK_KW_FALSE);
    assert(tokens[7].type == TOK_ID);
    assert(strcmp(tokens[7].str, "custom_id") == 0);
    
    for(int i=0; i<n; i++) {
        if(tokens[i].str) free(tokens[i].str);
    }
    printf("test_keywords_and_identifiers PASSED.\n");
}

void test_operators(void) {
    printf("Running test_operators...\n");
    const char *input = "< <= > >= = == ! != && || + - * / @main";
    TestToken tokens[32];
    int n = collect_tokens(input, tokens, 32);
    assert(n > 0);
    assert(tokens[0].type == TOK_LESS);
    assert(tokens[1].type == TOK_LESS_EQ);
    assert(tokens[2].type == TOK_GREATER);
    assert(tokens[3].type == TOK_GREATER_EQ);
    assert(tokens[4].type == TOK_ASSIGN);
    assert(tokens[5].type == TOK_EQ);
    assert(tokens[6].type == TOK_NOT);
    assert(tokens[7].type == TOK_NEQ);
    assert(tokens[8].type == TOK_AND);
    assert(tokens[9].type == TOK_OR);
    assert(tokens[10].type == TOK_PLUS);
    assert(tokens[11].type == TOK_MINUS);
    assert(tokens[12].type == TOK_MUL);
    assert(tokens[13].type == TOK_DIV);
    assert(tokens[14].type == TOK_AT_MAIN);
    for(int i=0; i<n; i++) {
        if(tokens[i].str) free(tokens[i].str);
    }
    printf("test_operators PASSED.\n");
}

void test_numeric_literals(void) {
    printf("Running test_numeric_literals...\n");
    const char *input = "123 0 3.14 1e10 1_000 123d 1.5e-3";
    TestToken tokens[32];
    int n = collect_tokens(input, tokens, 32);
    assert(n > 0);
    assert(tokens[0].type == TOK_INT);
    assert(strcmp(tokens[0].str, "123") == 0);
    assert(tokens[1].type == TOK_INT);
    assert(strcmp(tokens[1].str, "0") == 0);
    assert(tokens[2].type == TOK_DOUBLE);
    assert(strcmp(tokens[2].str, "3.14") == 0);
    assert(tokens[3].type == TOK_DOUBLE);
    assert(strcmp(tokens[3].str, "1e10") == 0);
    assert(tokens[4].type == TOK_INT);
    assert(strcmp(tokens[4].str, "1000") == 0);
    assert(tokens[5].type == TOK_DOUBLE);
    assert(strcmp(tokens[5].str, "123") == 0);
    assert(tokens[6].type == TOK_DOUBLE);
    assert(strcmp(tokens[6].str, "1.5e-3") == 0);
    for(int i=0; i<n; i++) {
        if(tokens[i].str) free(tokens[i].str);
    }
    printf("test_numeric_literals PASSED.\n");
}

void test_strings(void) {
    printf("Running test_strings...\n");
    const char *input = "\"hello\\nworld\" \"\" \"\"\"multi\\nline\"\"\"";
    TestToken tokens[32];
    int n = collect_tokens(input, tokens, 32);
    assert(n > 0);
    assert(tokens[0].type == TOK_STRING);
    assert(strcmp(tokens[0].str, "hello\nworld") == 0);
    assert(tokens[1].type == TOK_STRING);
    assert(strcmp(tokens[1].str, "") == 0);
    assert(tokens[2].type == TOK_STRING);
    assert(strcmp(tokens[2].str, "multi\\nline") == 0); // Víceřádkový řetězec zachovává text doslovně
    for(int i=0; i<n; i++) {
        if(tokens[i].str) free(tokens[i].str);
    }
    printf("test_strings PASSED.\n");
}

void test_comments(void) {
    printf("Running test_comments...\n");
    const char *input = "x // comment\n y /* block comment /* nested */ */ z";
    TestToken tokens[32];
    int n = collect_tokens(input, tokens, 32);
    assert(n > 0);
    assert(tokens[0].type == TOK_ID);
    assert(strcmp(tokens[0].str, "x") == 0);
    
    int found_y = 0, found_z = 0;
    for(int i=0; i<n; i++) {
        if(tokens[i].type == TOK_ID) {
            if(strcmp(tokens[i].str, "y") == 0) found_y = 1;
            if(strcmp(tokens[i].str, "z") == 0) found_z = 1;
        }
        if(tokens[i].str) free(tokens[i].str);
    }
    assert(found_y);
    assert(found_z);
    printf("test_comments PASSED.\n");
}

void test_indentation(void) {
    printf("Running test_indentation...\n");
    const char *input = "def foo()\n  val x = 5\nval y = 10\n";
    TestToken tokens[64];
    int n = collect_tokens(input, tokens, 64);
    assert(n > 0);

    int found_indent = 0;
    int found_dedent = 0;
    for(int i=0; i<n; i++) {
        if(tokens[i].type == TOK_INDENT) found_indent = 1;
        if(tokens[i].type == TOK_DEDENT) found_dedent = 1;
        if(tokens[i].str) free(tokens[i].str);
    }

    assert(found_indent);
    assert(found_dedent);
    printf("test_indentation PASSED.\n");
}

int main(void) {
    printf("=== STARTING SCANNER UNIT TESTS ===\n");
    test_keywords_and_identifiers();
    test_operators();
    test_numeric_literals();
    test_strings();
    test_comments();
    test_indentation();
    printf("=== ALL UNIT TESTS PASSED SUCCESSFULLY ===\n");
    return 0;
}
