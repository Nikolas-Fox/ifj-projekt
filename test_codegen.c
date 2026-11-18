/**
 * AI generated (Claude Sonnet 5.5)
 * Prompt: "Generate unit tests for codegen.
 * It is not yet linked to other modules, necessary declarations are duplicated in codegen.h (from ast, symtable etc).
 * See codegen.h and project specification, make the tests adhere to specified rules for the language.
 * Generate ast in the tests and run generateProgram on them."
 * 
 * test_codegen.c - unit tests for the IFJ26 code generator
 *
 * Every test builds an AST by hand (as the semantic analyser would hand it
 * over: types filled in, scopes numbered), runs generateProgram() on it in a
 * forked child and checks the result.
 *
 *   1. STRUCTURE (always):  the child must finish with exit code 0 and its
 *      output must start with the ".IFJcode26" header.
 *   2. BEHAVIOUR (when the ic26int interpreter is available): the generated
 *      code is executed with a given stdin and the stdout + exit code are
 *      compared with the values the IFJ26 specification dictates.
 *      Interpreter: $IC26INT, or "ic26int" from PATH.
 *
 * Build:  gcc -std=gnu11 -Wall -Wextra -g test_codegen.c codegen.c -o test_codegen
 * Run:    ./test_codegen            (all tests)
 *         ./test_codegen fib        (only tests whose name contains "fib")
 *
 * ASSUMPTIONS about the AST contract (adjust the helpers below if codegen
 * expects something else):
 *  - generateProgram() writes IFJcode26 to stdout.
 *  - Literal `value` holds the decoded content: strings WITHOUT quotes and with
 *    escape sequences already resolved (real '\n', '\\' characters); Int/Double
 *    literals hold the lexeme text ("42", "1.5", "1e3", ".5"); Unit literal
 *    has value NULL and type TYPE_UNIT.
 *  - `scope` numbers are unique within the whole program (not depth).
 *    A function's parameters carry the scope id of the function body.
 *  - Node `type` is the type of the value (declaration: the variable's type,
 *    func: return type, call: return type, comparison: TYPE_BOOL). Operands are
 *    NOT wrapped in conversion nodes: codegen must insert Int->Double itself
 *    from the operand/target types.
 *  - A function body / if branch / while body is a linked list (->next).
 *    A function's `group` is an AST_GROUP (items = AST_PARAM list), groupCount=1.
 *    A call's `group` is an AST_GROUP (items = argument list), groupCount=1.
 *  - A bare `return` is AST_RETURN with expression == NULL.
 *  - A statement that is just a literal () is an AST_LITERAL (unit).
 */
#define _GNU_SOURCE
#include "codegen.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/* ------------------------------------------------------------------ */
/*  AST construction helpers                                          */
/* ------------------------------------------------------------------ */
#define TI TYPE_INT
#define TD TYPE_DOUBLE
#define TS TYPE_STRING
#define TU TYPE_UNIT
#define TB TYPE_BOOL

static int scope_counter = 0;
static int ns(void) { return ++scope_counter; }

static AstNode *mk(AstClass c, DataType t)
{
    AstNode *n = calloc(1, sizeof *n);
    if (!n) { perror("calloc"); _exit(97); }
    n->class = c;
    n->type = t;
    return n;
}

static AstNode *tail(AstNode *n)
{
    while (n && n->next) n = n->next;
    return n;
}

/* chain nodes (or chains) into one linked list; terminated by NULL */
static AstNode *seq(AstNode *first, ...)
{
    va_list ap;
    va_start(ap, first);
    AstNode *cur = tail(first), *n;
    while ((n = va_arg(ap, AstNode *)) != NULL) {
        cur->next = n;
        cur = tail(n);
    }
    va_end(ap);
    return first;
}
#define SEQ(...) seq(__VA_ARGS__, (AstNode *)NULL)

static AstNode *I(const char *v)  { AstNode *n = mk(AST_LITERAL, TI); n->data.value = strdup(v); return n; }
static AstNode *Dl(const char *v) { AstNode *n = mk(AST_LITERAL, TD); n->data.value = strdup(v); return n; }
static AstNode *St(const char *v) { AstNode *n = mk(AST_LITERAL, TS); n->data.value = strdup(v); return n; }
static AstNode *U(void)           { AstNode *n = mk(AST_LITERAL, TU); n->data.value = NULL; return n; }

static AstNode *R(const char *name, int scope, DataType t)
{
    AstNode *n = mk(AST_ID, t);
    n->data.id.name = strdup(name);
    n->data.id.scope = scope;
    return n;
}

static AstNode *B(TokenType op, AstNode *l, AstNode *r, DataType t)
{
    AstNode *n = mk(AST_BINARY, t);
    n->data.binary.op = op;
    n->data.binary.left = l;
    n->data.binary.right = r;
    return n;
}
#define CMP(op, l, r) B(op, l, r, TB)

static AstNode *Neg(AstNode *e)
{
    AstNode *n = mk(AST_UNARY, e->type);
    n->data.unary.op = TOK_MINUS;
    n->data.unary.operand = e;
    return n;
}

static AstNode *Grp(AstNode *items)
{
    AstNode *g = mk(AST_GROUP, TU);
    g->data.group.items = items;
    for (AstNode *p = items; p; p = p->next) g->data.group.count++;
    return g;
}

static AstNode *Call(const char *name, DataType t, AstNode *args)
{
    AstNode *n = mk(AST_CALL, t);
    n->data.call.name = strdup(name);
    n->data.call.group = Grp(args);
    n->data.call.groupCount = 1;
    return n;
}

static AstNode *Decl(const char *name, bool isVar, DataType t, AstNode *e, int scope)
{
    AstNode *n = mk(AST_DECLARATION, t);
    n->data.declaration.name = strdup(name);
    n->data.declaration.isVar = isVar;
    n->data.declaration.expression = e;
    n->data.declaration.scope = scope;
    return n;
}

static AstNode *Asg(const char *name, int scope, DataType t, AstNode *e)
{
    AstNode *n = mk(AST_ASSIGN, TU);
    n->data.assign.destination = R(name, scope, t);
    n->data.assign.expression = e;
    return n;
}

static AstNode *If(AstNode *c, AstNode *th, AstNode *el)
{
    AstNode *n = mk(AST_IF, TU);
    n->data.ifStat.condition = c;
    n->data.ifStat.thenBody = th;
    n->data.ifStat.elseBody = el;
    return n;
}

static AstNode *While(AstNode *c, AstNode *body)
{
    AstNode *n = mk(AST_WHILE, TU);
    n->data.whileStat.condition = c;
    n->data.whileStat.body = body;
    return n;
}

static AstNode *Ret(AstNode *e, DataType t)
{
    AstNode *n = mk(AST_RETURN, t);
    n->data.returnStat.expression = e;
    return n;
}

static AstNode *Param(const char *name, int scope, DataType t)
{
    AstNode *n = mk(AST_PARAM, t);
    n->data.param.name = strdup(name);
    n->data.param.scope = scope;
    return n;
}

static AstNode *Func(const char *name, DataType ret, AstNode *params, AstNode *body, bool isMain)
{
    AstNode *n = mk(AST_FUNC, ret);
    n->data.func.name = strdup(name);
    n->data.func.group = Grp(params);
    n->data.func.groupCount = 1;
    n->data.func.body = body;
    n->data.func.isMain = isMain;
    return n;
}
#define Main(body) Func("main", TU, NULL, body, true)

static AstNode *Prog(AstNode *funcs)
{
    AstNode *n = mk(AST_WHOLE, TU);
    n->data.whole.func = funcs;
    return n;
}

/* shortcuts for statements */
static AstNode *Pr(AstNode *arg) { return Call("print", TU, arg); }
static AstNode *Nl(void)         { return Pr(St("\n")); }

static AstNode *TF(AstNode *cond) { return If(cond, Pr(St("T")), Pr(St("F"))); }

/* val <tag> = expr ; print it (Double via toStr) ; print "\n" */
static AstNode *Show(const char *tag, int sc, DataType t, AstNode *e)
{
    AstNode *d = Decl(tag, false, t, e, sc);
    if (t == TD) {
        char name2[64];
        snprintf(name2, sizeof name2, "%s_s", tag);
        AstNode *d2 = Decl(name2, false, TS, Call("toStr", TS, R(tag, sc, TD)), sc);
        return SEQ(d, d2, Pr(R(name2, sc, TS)), Nl());
    }
    return SEQ(d, Pr(R(tag, sc, t)), Nl());
}

/* ------------------------------------------------------------------ */
/*  Test programs                                                     */
/* ------------------------------------------------------------------ */
typedef AstNode *(*Builder)(void);

/* ---- trivial / printing ---- */
static AstNode *b_empty(void) { return Prog(Main(U())); }

static AstNode *b_print_literals(void)
{
    return Prog(Main(SEQ(Pr(St("hello")), Nl(), Pr(I("42")), Nl(), Pr(U()))));
}

static AstNode *b_print_double(void)
{
    int s = ns();
    return Prog(Main(SEQ(Decl("d", false, TD, Dl("1.5"), s), Pr(R("d", s, TD)))));
}

static AstNode *b_tostr_int(void)
{
    int s = ns();
    return Prog(Main(SEQ(Decl("n", false, TI, I("42"), s),
                         Decl("t", false, TS, Call("toStr", TS, R("n", s, TI)), s),
                         Pr(R("t", s, TS)))));
}

static AstNode *b_string_escapes(void)
{
    /* spaces, '#', backslash, quote, tab, newline must survive IFJcode26 escaping */
    return Prog(Main(SEQ(Pr(St("a b#c\\d\"q\"\tx")), Nl(), Pr(St("line1\nline2")))));
}

/* ---- arithmetic ---- */
static AstNode *b_int_arith(void)
{
    int s = ns();
    return Prog(Main(SEQ(
        Show("a", s, TI, B(TOK_PLUS, I("2"), B(TOK_MUL, I("3"), I("4"), TI), TI)),   /* 14 */
        Show("b", s, TI, B(TOK_DIV, I("7"), I("2"), TI)),                            /* 3  */
        Show("c", s, TI, B(TOK_DIV, Neg(I("7")), I("2"), TI)),                       /* -3: truncation to zero */
        Show("d", s, TI, B(TOK_DIV, I("7"), Neg(I("2")), TI)),                       /* -3 */
        Show("e", s, TI, B(TOK_MINUS, I("7"), I("10"), TI)),                         /* -3 */
        Show("f", s, TI, Neg(R("a", s, TI))),                                        /* -14 */
        Show("g", s, TI, B(TOK_DIV, Neg(I("6")), I("3"), TI)),                       /* -2 */
        Show("h", s, TI, B(TOK_MINUS, B(TOK_MINUS, I("10"), I("3"), TI), I("2"), TI))/* 5: left assoc. */
    )));
}

static AstNode *b_int_overflow(void)
{
    /* Int is 32-bit with Scala wrap-around semantics (IFJcode int is 64-bit!) */
    int s = ns();
    return Prog(Main(SEQ(
        Show("a", s, TI, B(TOK_PLUS, I("2147483647"), I("1"), TI)),
        Show("b", s, TI, B(TOK_MUL, I("2147483647"), I("2"), TI)),
        Show("c", s, TI, B(TOK_MINUS, B(TOK_MINUS, I("0"), I("2147483647"), TI), I("2"), TI)),
        Show("d", s, TI, B(TOK_MUL, I("65536"), I("65536"), TI)),
        Show("e", s, TI, B(TOK_MUL, I("46341"), I("46341"), TI))
    )));
}

static AstNode *b_double_arith(void)
{
    int s = ns();
    return Prog(Main(SEQ(
        Show("a", s, TD, B(TOK_PLUS, Dl("1.5"), Dl("2.25"), TD)),
        Show("b", s, TD, B(TOK_PLUS, I("1"), Dl("2.5"), TD)),               /* Int -> Double */
        Show("c", s, TD, B(TOK_DIV, I("5"), Dl("2.0"), TD)),
        Show("d", s, TD, B(TOK_DIV, Dl("7.0"), I("2"), TD)),
        Show("e", s, TD, B(TOK_MUL, I("2"), Dl("1.5"), TD)),
        Show("f", s, TD, B(TOK_MINUS, I("10"), Dl("0.25"), TD)),
        Show("g", s, TD, Neg(Dl("1.5"))),
        Show("h", s, TD, B(TOK_PLUS, B(TOK_DIV, I("7"), I("2"), TI), Dl("0.5"), TD)) /* 7/2 stays integer: 3 + 0.5 */
    )));
}

static AstNode *b_double_literals(void)
{
    int s = ns();
    return Prog(Main(SEQ(
        Show("a", s, TD, Dl(".5")),
        Show("b", s, TD, Dl("1e3")),
        Show("c", s, TD, Dl("2.5E-1")),
        Show("d", s, TD, Dl("1e+2")),
        Show("e", s, TD, Dl("0.75")),
        Show("f", s, TD, Dl("12.0"))
    )));
}

/* ---- implicit Int -> Double ---- */
static AstNode *b_conv_decl(void)
{
    int s = ns();
    return Prog(Main(Show("d", s, TD, I("3"))));              /* val d: Double = 3 */
}

static AstNode *b_conv_assign(void)
{
    int s = ns();
    return Prog(Main(SEQ(Decl("d", true, TD, I("1"), s),      /* var d: Double = 1 */
                         Asg("d", s, TD, I("2")),             /* d = 2 */
                         Show("e", s, TD, R("d", s, TD)))));
}

static AstNode *b_conv_arg(void)
{
    int sf = ns(), sm = ns();
    AstNode *twice = Func("twice", TD, Param("x", sf, TD),
                          B(TOK_MUL, R("x", sf, TD), I("2"), TD), false);
    AstNode *m = Main(Show("r", sm, TD, Call("twice", TD, I("3"))));   /* Int arg to Double param */
    return Prog(SEQ(twice, m));
}

static AstNode *b_conv_return(void)
{
    int sm = ns();
    AstNode *three = Func("three", TD, NULL, I("3"), false);          /* def three(): Double = 3 */
    AstNode *m = Main(Show("r", sm, TD, Call("three", TD, NULL)));
    return Prog(SEQ(m, three));
}

/* ---- strings & builtins ---- */
static AstNode *b_string_ops(void)
{
    int s = ns();
    return Prog(Main(SEQ(
        Show("a", s, TS, B(TOK_PLUS, St("ab"), St("cd"), TS)),
        Show("b", s, TI, Call("strLength", TI, St("hello"))),
        Show("c", s, TS, Call("subStr", TS, SEQ(St("abcdef"), I("1"), I("4")))),
        Show("d", s, TS, Call("subStr", TS, SEQ(St("abc"), I("0"), I("0")))),   /* empty result */
        Show("e", s, TS, Call("subStr", TS, SEQ(St("abc"), I("1"), I("3")))),
        Show("f", s, TI, Call("ord", TI, SEQ(St("A"), I("0")))),
        Show("g", s, TI, Call("ord", TI, SEQ(St("abc"), I("5")))),              /* out of range -> -1 */
        Show("h", s, TI, Call("ord", TI, SEQ(St(""), I("0")))),                 /* empty -> -1 */
        Show("i", s, TS, Call("chr", TS, I("66"))),
        Show("j", s, TI, Call("strLength", TI, St(""))),
        Show("k", s, TS, Call("subStr", TS, SEQ(St("abc"), I("0"), I("3"))))
    )));
}

static AstNode *b_toint(void)
{
    int s = ns();
    return Prog(Main(SEQ(
        Decl("d", false, TD, Dl("3.9"), s),
        Show("a", s, TI, Call("toInt", TI, R("d", s, TD))),
        Decl("n", false, TD, Neg(Dl("3.9")), s),
        Show("b", s, TI, Call("toInt", TI, R("n", s, TD)))
    )));
}

static AstNode *b_readline(void)
{
    int s = ns();
    return Prog(Main(SEQ(
        Decl("l1", false, TS, Call("readLine", TS, NULL), s), Pr(R("l1", s, TS)),
        Decl("l2", false, TS, Call("readLine", TS, NULL), s), Pr(R("l2", s, TS)),
        Decl("l3", false, TS, Call("readLine", TS, NULL), s), Pr(R("l3", s, TS))   /* EOF -> "" */
    )));
}

static AstNode *b_readdouble(void)
{
    int s = ns();
    return Prog(Main(SEQ(Decl("d", false, TD, Call("readDouble", TD, NULL), s),
                         Show("e", s, TD, R("d", s, TD)))));
}

static AstNode *b_readint_edge(void)
{
    int s = ns();
    return Prog(Main(SEQ(
        Show("a", s, TI, Call("readInt", TI, NULL)),      /* "-12"          -> -12 */
        Show("b", s, TI, Call("readInt", TI, NULL)),      /* " 5"           -> 0 (no whitespace skipping) */
        Show("c", s, TI, Call("readInt", TI, NULL))       /* "99999999999"  -> 0 (out of Int range) */
    )));
}

/* ---- relational operators / if ---- */
static AstNode *b_relational(void)
{
    return Prog(Main(SEQ(
        TF(CMP(TOK_LESS,       I("1"), I("2"))),            /* T */
        TF(CMP(TOK_LESS,       I("2"), I("1"))),            /* F */
        TF(CMP(TOK_LESS_EQ,    I("2"), I("2"))),            /* T */
        TF(CMP(TOK_GREATER,    I("3"), I("2"))),            /* T */
        TF(CMP(TOK_GREATER_EQ, I("2"), I("3"))),            /* F */
        TF(CMP(TOK_EQ,         I("1"), I("1"))),            /* T */
        TF(CMP(TOK_NEQ,        I("1"), I("1"))),            /* F */
        TF(CMP(TOK_EQ,         I("1"), Dl("1.0"))),         /* T  (Int vs Double) */
        TF(CMP(TOK_LESS,       Dl("1.5"), I("2"))),         /* T */
        TF(CMP(TOK_LESS,       St("abc"), St("abd"))),      /* T  (lexicographic) */
        TF(CMP(TOK_GREATER,    St("b"), St("abc"))),        /* T */
        TF(CMP(TOK_EQ,         St("a"), St("a"))),          /* T */
        TF(CMP(TOK_NEQ,        St("a"), St("a"))),          /* F */
        TF(CMP(TOK_NEQ,        Dl("2.5"), Dl("2.5"))),      /* F */
        TF(CMP(TOK_GREATER_EQ, I("2"), Dl("2.0"))),         /* T */
        TF(CMP(TOK_LESS,       St(""), St("a")))            /* T */
    )));
}

static AstNode *b_if_scopes(void)
{
    int s0 = ns(), s1 = ns(), s2 = ns(), s3 = ns(), s4 = ns(), s5 = ns();
    return Prog(Main(SEQ(
        Decl("x", false, TI, I("1"), s0),
        If(CMP(TOK_LESS, I("1"), I("2")),
           SEQ(Decl("x", false, TI, I("2"), s1), Pr(R("x", s1, TI))),
           SEQ(Decl("x", false, TI, I("3"), s2), Pr(R("x", s2, TI)))),
        If(CMP(TOK_GREATER, I("1"), I("2")),
           SEQ(Decl("x", false, TI, I("8"), s3), Pr(R("x", s3, TI))),
           SEQ(Decl("x", false, TI, I("9"), s4), Pr(R("x", s4, TI)))),
        If(CMP(TOK_LESS, I("1"), I("2")),                              /* shadowing with a different type */
           SEQ(Decl("x", false, TS, St("s"), s5), Pr(R("x", s5, TS))),
           U()),
        Pr(R("x", s0, TI)), Nl()
    )));
}

static AstNode *b_outer_assign(void)
{
    int s0 = ns();
    return Prog(Main(SEQ(
        Decl("c", true, TI, I("0"), s0),
        If(CMP(TOK_LESS, I("1"), I("2")),
           Asg("c", s0, TI, B(TOK_PLUS, R("c", s0, TI), I("5"), TI)),
           U()),
        Show("r", s0, TI, R("c", s0, TI))
    )));
}

static AstNode *b_if_unit_func(void)
{
    int sp = ns();
    AstNode *p = Func("p", TU, Param("x", sp, TI),
                      If(CMP(TOK_LESS, R("x", sp, TI), I("5")), Pr(St("small")), Pr(St("big"))),
                      false);
    return Prog(SEQ(Main(SEQ(Call("p", TU, I("3")), Call("p", TU, I("9")))), p));
}

/* ---- while ---- */
static AstNode *b_factorial(void)
{
    int sf = ns(), sm = ns();
    AstNode *fact = Func("factorial", TI, Param("n", sf, TI),
        SEQ(Decl("result", true, TI, I("1"), sf),
            Decl("i", true, TI, R("n", sf, TI), sf),
            While(CMP(TOK_GREATER, R("i", sf, TI), I("1")),
                  SEQ(Asg("result", sf, TI, B(TOK_MUL, R("result", sf, TI), R("i", sf, TI), TI)),
                      Asg("i", sf, TI, B(TOK_MINUS, R("i", sf, TI), I("1"), TI)))),
            R("result", sf, TI)),
        false);
    AstNode *m = Func("runFoo", TU, NULL,
        SEQ(Decl("n", false, TI, Call("readInt", TI, NULL), sm),
            Decl("result", false, TI, Call("factorial", TI, R("n", sm, TI)), sm),
            Pr(R("result", sm, TI)),
            Pr(St("\n"))),
        true);
    return Prog(SEQ(fact, m));
}

static AstNode *b_while_decl(void)
{
    int sm = ns(), sw = ns();
    return Prog(Main(SEQ(
        Decl("i", true, TI, I("0"), sm),
        While(CMP(TOK_LESS, R("i", sm, TI), I("3")),
              SEQ(Decl("sq", false, TI, B(TOK_MUL, R("i", sm, TI), R("i", sm, TI), TI), sw),  /* re-declared each iteration */
                  Pr(R("sq", sw, TI)),
                  Asg("i", sm, TI, B(TOK_PLUS, R("i", sm, TI), I("1"), TI)))),
        Nl()
    )));
}

static AstNode *b_nested_loops(void)
{
    int sm = ns(), so = ns(), si = ns(), st = ns();
    AstNode *inner = While(CMP(TOK_LESS, R("j", so, TI), I("4")),
        SEQ(If(CMP(TOK_LESS, R("i", sm, TI), R("j", so, TI)),
               Asg("cnt", sm, TI, B(TOK_PLUS, R("cnt", sm, TI), I("1"), TI)),
               U()),
            Asg("j", so, TI, B(TOK_PLUS, R("j", so, TI), I("1"), TI))));
    (void)si; (void)st;
    return Prog(Main(SEQ(
        Decl("i", true, TI, I("0"), sm),
        Decl("cnt", true, TI, I("0"), sm),
        While(CMP(TOK_LESS, R("i", sm, TI), I("4")),
              SEQ(Decl("j", true, TI, I("0"), so),
                  inner,
                  Asg("i", sm, TI, B(TOK_PLUS, R("i", sm, TI), I("1"), TI)))),
        Show("r", sm, TI, R("cnt", sm, TI))                       /* pairs i<j in 0..3 = 6 */
    )));
}

static AstNode *b_while_zero(void)
{
    int sm = ns();
    return Prog(Main(SEQ(
        Decl("i", true, TI, I("5"), sm),
        While(CMP(TOK_LESS, R("i", sm, TI), I("3")), Pr(St("never"))),
        Pr(St("done"))
    )));
}

/* ---- functions ---- */
static AstNode *b_fib(void)
{
    int sf = ns(), sm = ns();
    AstNode *fib = Func("fib", TI, Param("n", sf, TI),
        SEQ(If(CMP(TOK_LESS, R("n", sf, TI), I("2")), Ret(R("n", sf, TI), TI), U()),
            Decl("m1", false, TI, B(TOK_MINUS, R("n", sf, TI), I("1"), TI), sf),
            Decl("a", false, TI, Call("fib", TI, R("m1", sf, TI)), sf),
            Decl("m2", false, TI, B(TOK_MINUS, R("n", sf, TI), I("2"), TI), sf),
            Decl("b", false, TI, Call("fib", TI, R("m2", sf, TI)), sf),
            B(TOK_PLUS, R("a", sf, TI), R("b", sf, TI), TI)),
        false);
    return Prog(SEQ(Main(Show("r", sm, TI, Call("fib", TI, I("10")))), fib));   /* 55 */
}

static AstNode *b_deep_recursion(void)
{
    int sf = ns(), sm = ns();
    AstNode *sum = Func("sum", TI, Param("n", sf, TI),
        SEQ(If(CMP(TOK_EQ, R("n", sf, TI), I("0")), Ret(I("0"), TI), U()),
            Decl("m", false, TI, B(TOK_MINUS, R("n", sf, TI), I("1"), TI), sf),
            Decl("r", false, TI, Call("sum", TI, R("m", sf, TI)), sf),
            B(TOK_PLUS, R("n", sf, TI), R("r", sf, TI), TI)),
        false);
    return Prog(SEQ(sum, Main(Show("r", sm, TI, Call("sum", TI, I("1000"))))));   /* 500500 */
}

static AstNode *b_mutual_recursion(void)
{
    int se = ns(), so = ns(), sm = ns();
    AstNode *even = Func("isEven", TI, Param("n", se, TI),
        SEQ(If(CMP(TOK_EQ, R("n", se, TI), I("0")), Ret(I("1"), TI), U()),
            Decl("m", false, TI, B(TOK_MINUS, R("n", se, TI), I("1"), TI), se),
            Ret(Call("isOdd", TI, R("m", se, TI)), TI)),
        false);
    AstNode *odd = Func("isOdd", TI, Param("n", so, TI),
        SEQ(If(CMP(TOK_EQ, R("n", so, TI), I("0")), Ret(I("0"), TI), U()),
            Decl("m", false, TI, B(TOK_MINUS, R("n", so, TI), I("1"), TI), so),
            Ret(Call("isEven", TI, R("m", so, TI)), TI)),
        false);
    AstNode *m = Main(SEQ(                                     /* main defined BEFORE the callees */
        Show("a", sm, TI, Call("isEven", TI, I("10"))),
        Show("b", sm, TI, Call("isEven", TI, I("7"))),
        Show("c", sm, TI, Call("isOdd", TI, I("7")))));
    return Prog(SEQ(m, even, odd));
}

static AstNode *b_return_in_loop(void)
{
    int sf = ns(), sm = ns();
    AstNode *find = Func("find", TI, Param("limit", sf, TI),
        SEQ(Decl("i", true, TI, I("0"), sf),
            While(CMP(TOK_LESS, R("i", sf, TI), R("limit", sf, TI)),
                  SEQ(If(CMP(TOK_GREATER, B(TOK_MUL, R("i", sf, TI), R("i", sf, TI), TI), I("20")),
                         Ret(R("i", sf, TI), TI), U()),
                      Asg("i", sf, TI, B(TOK_PLUS, R("i", sf, TI), I("1"), TI)))),
            Ret(Neg(I("1")), TI)),
        false);
    AstNode *m = Main(SEQ(Show("a", sm, TI, Call("find", TI, I("10"))),    /* 5  */
                          Show("b", sm, TI, Call("find", TI, I("3")))));   /* -1 */
    return Prog(SEQ(m, find));
}

static AstNode *b_unit_funcs(void)
{
    int sm = ns();
    AstNode *hello = Func("hello", TU, NULL, Pr(St("hi")), false);
    AstNode *drop = Func("drop", TU, NULL, I("5"), false);          /* value discarded, returns () */
    AstNode *m = Main(SEQ(Call("hello", TU, NULL),
                          Call("hello", TU, NULL),
                          Decl("u", false, TU, Call("drop", TU, NULL), sm),
                          Pr(R("u", sm, TU))));
    return Prog(SEQ(m, hello, drop));
}

static AstNode *b_bare_return(void)
{
    int sm = ns(), se = ns();
    AstNode *early = Func("early", TU, Param("x", se, TI),
        SEQ(If(CMP(TOK_LESS, R("x", se, TI), I("0")), Ret(NULL, TU), U()),
            Pr(St("pos"))),
        false);
    AstNode *m = Main(SEQ(Decl("neg", false, TI, Neg(I("1")), sm),
                          Call("early", TU, R("neg", sm, TI)),
                          Call("early", TU, I("5"))));
    return Prog(SEQ(m, early));
}

static AstNode *b_param_order(void)
{
    int sa = ns(), sb = ns(), sm = ns();
    AstNode *sub = Func("sub", TI, SEQ(Param("a", sa, TI), Param("b", sa, TI)),
                        B(TOK_MINUS, R("a", sa, TI), R("b", sa, TI), TI), false);
    AstNode *cat3 = Func("cat3", TS, SEQ(Param("a", sb, TS), Param("b", sb, TS), Param("c", sb, TS)),
                         B(TOK_PLUS, B(TOK_PLUS, R("a", sb, TS), R("b", sb, TS), TS), R("c", sb, TS), TS),
                         false);
    AstNode *m = Main(SEQ(
        Show("a", sm, TI, Call("sub", TI, SEQ(I("10"), I("3")))),
        Show("b", sm, TI, Call("sub", TI, SEQ(I("3"), I("10")))),
        Decl("c", false, TS, Call("cat3", TS, SEQ(St("x"), St("y"), St("z"))), sm),
        Pr(R("c", sm, TS))));
    return Prog(SEQ(m, sub, cat3));
}

static AstNode *b_names_across_funcs(void)
{
    int sf = ns(), sg = ns(), sm = ns();
    AstNode *f = Func("f", TI, Param("x", sf, TI),
        SEQ(Decl("y", false, TI, B(TOK_PLUS, R("x", sf, TI), I("1"), TI), sf), R("y", sf, TI)), false);
    AstNode *g = Func("g", TI, Param("x", sg, TI),
        SEQ(Decl("y", false, TI, B(TOK_MUL, R("x", sg, TI), I("2"), TI), sg), R("y", sg, TI)), false);
    AstNode *m = Main(SEQ(
        Decl("y", false, TI, Call("f", TI, I("1")), sm), Pr(R("y", sm, TI)), Nl(),
        Show("z", sm, TI, Call("g", TI, R("y", sm, TI)))));
    return Prog(SEQ(g, m, f));                                      /* "2\n4\n" */
}

static AstNode *b_implicit_return(void)
{
    int sf = ns(), ss = ns(), sm = ns();
    AstNode *five = Func("five", TI, NULL,
        SEQ(Decl("a", false, TI, I("2"), sf), B(TOK_PLUS, R("a", sf, TI), I("3"), TI)), false);
    /* if is the last statement and ALL branches end with return */
    AstNode *sign = Func("sign", TI, Param("x", ss, TI),
        If(CMP(TOK_LESS, R("x", ss, TI), I("0")),
           Ret(Neg(I("1")), TI),
           If(CMP(TOK_EQ, R("x", ss, TI), I("0")), Ret(I("0"), TI), Ret(I("1"), TI))),
        false);
    AstNode *m = Main(SEQ(
        Show("a", sm, TI, Call("five", TI, NULL)),
        Decl("neg", false, TI, Neg(I("3")), sm),
        Show("b", sm, TI, Call("sign", TI, R("neg", sm, TI))),
        Show("c", sm, TI, Call("sign", TI, I("0"))),
        Show("d", sm, TI, Call("sign", TI, I("7")))));
    return Prog(SEQ(five, sign, m));
}

static AstNode *b_call_discard(void)
{
    int sn = ns(), sm = ns();
    AstNode *noisy = Func("noisy", TI, NULL, SEQ(Pr(St("x")), I("1")), false);
    AstNode *m = Main(SEQ(
        Call("noisy", TI, NULL),                                    /* result discarded */
        Decl("v", true, TI, I("0"), sm),
        Asg("v", sm, TI, Call("noisy", TI, NULL)),                  /* call as RHS of assignment */
        Pr(R("v", sm, TI))));
    (void)sn;
    return Prog(SEQ(noisy, m));                                     /* "xx1" */
}

static AstNode *b_main_name(void)
{
    AstNode *helper = Func("helper", TU, NULL, Pr(St("h")), false);
    AstNode *run = Func("runFoo", TU, NULL, SEQ(Call("helper", TU, NULL), Pr(St("m"))), true);
    AstNode *unused = Func("unused", TU, NULL, Pr(St("NEVER")), false);
    return Prog(SEQ(helper, run, unused));                          /* main is not first, not called "main" */
}

static AstNode *b_labels_across_funcs(void)
{
    int sa = ns(), sb = ns(), sm = ns();
    AstNode *a = Func("a", TU, NULL, SEQ(
        Decl("i", true, TI, I("0"), sa),
        While(CMP(TOK_LESS, R("i", sa, TI), I("2")),
              SEQ(If(CMP(TOK_LESS, R("i", sa, TI), I("1")), Pr(St("a")), Pr(St("b"))),
                  Asg("i", sa, TI, B(TOK_PLUS, R("i", sa, TI), I("1"), TI))))), false);
    AstNode *b = Func("b", TU, NULL, SEQ(
        Decl("i", true, TI, I("0"), sb),
        While(CMP(TOK_LESS, R("i", sb, TI), I("2")),
              SEQ(If(CMP(TOK_LESS, R("i", sb, TI), I("1")), Pr(St("c")), Pr(St("d"))),
                  Asg("i", sb, TI, B(TOK_PLUS, R("i", sb, TI), I("1"), TI))))), false);
    (void)sm;
    return Prog(SEQ(Main(SEQ(Call("a", TU, NULL), Call("b", TU, NULL))), a, b));
}

static AstNode *b_reassign(void)
{
    int s = ns();
    return Prog(Main(SEQ(
        Decl("s", true, TS, St("a"), s),
        Asg("s", s, TS, B(TOK_PLUS, R("s", s, TS), St("b"), TS)),
        Asg("s", s, TS, B(TOK_PLUS, R("s", s, TS), St("c"), TS)),
        Pr(R("s", s, TS)), Nl(),
        Decl("d", true, TD, Dl("1.5"), s),
        Asg("d", s, TD, B(TOK_PLUS, R("d", s, TD), I("1"), TD)),
        Show("e", s, TD, R("d", s, TD))
    )));
}

/* ---- runtime errors ---- */
static AstNode *b_div_zero(void)
{
    int s = ns();
    return Prog(Main(SEQ(Pr(St("a")),
                         Decl("z", false, TI, I("0"), s),
                         Decl("r", false, TI, B(TOK_DIV, I("1"), R("z", s, TI), TI), s),
                         Pr(R("r", s, TI)))));
}

#define SUBSTR_ERR(fn, str, A, Bx)                                                      \
    static AstNode *fn(void)                                                            \
    {                                                                                   \
        int s = ns();                                                                   \
        return Prog(Main(SEQ(Pr(St("a")),                                               \
                             Decl("m1", false, TI, Neg(I("1")), s),                     \
                             Decl("r", false, TS, Call("subStr", TS, SEQ(St(str), A, Bx)), s), \
                             Pr(R("r", s, TS)))));                                      \
    }
SUBSTR_ERR(b_substr_start_gt_end, "abc", I("2"), I("1"))
SUBSTR_ERR(b_substr_start_eq_len, "abc", I("3"), I("3"))
SUBSTR_ERR(b_substr_start_neg,    "abc", R("m1", s, TI), I("2"))
SUBSTR_ERR(b_substr_end_neg,      "abc", I("0"), R("m1", s, TI))
SUBSTR_ERR(b_substr_end_gt_len,   "abc", I("0"), I("4"))
SUBSTR_ERR(b_substr_empty,        "",    I("0"), I("0"))

static AstNode *b_chr_range(void)
{
    int s = ns();
    return Prog(Main(SEQ(Pr(St("a")),
                         Decl("c", false, TI, I("200"), s),
                         Decl("r", false, TS, Call("chr", TS, R("c", s, TI)), s),
                         Pr(R("r", s, TS)))));
}

/* ------------------------------------------------------------------ */
/*  Test table                                                        */
/* ------------------------------------------------------------------ */
typedef struct {
    const char *name;
    Builder build;
    const char *input;      /* stdin for the interpreter */
    const char *expected;   /* expected stdout */
    int rc;                 /* expected interpreter exit code */
} Case;

static const Case CASES[] = {
    {"empty_main",             b_empty,             "", "", 0},
    {"print_literals",         b_print_literals,    "", "hello\n42\n()", 0},
    {"print_double_hex",       b_print_double,      "", "0x1.8p+0", 0},
    {"tostr_int",              b_tostr_int,         "", "42", 0},
    {"string_escapes",         b_string_escapes,    "", "a b#c\\d\"q\"\tx\nline1\nline2", 0},

    {"int_arith",              b_int_arith,         "", "14\n3\n-3\n-3\n-3\n-14\n-2\n5\n", 0},
    {"int_overflow_wraps",     b_int_overflow,      "", "-2147483648\n-2\n2147483647\n0\n-2147479015\n", 0},
    {"double_arith",           b_double_arith,      "", "3.75\n3.50\n2.50\n3.50\n3.00\n9.75\n-1.50\n3.50\n", 0},
    {"double_literal_forms",   b_double_literals,   "", "0.50\n1000.00\n0.25\n100.00\n0.75\n12.00\n", 0},

    {"conv_decl",              b_conv_decl,         "", "3.00\n", 0},
    {"conv_assign",            b_conv_assign,       "", "2.00\n", 0},
    {"conv_call_arg",          b_conv_arg,          "", "6.00\n", 0},
    {"conv_return",            b_conv_return,       "", "3.00\n", 0},

    {"string_builtins",        b_string_ops,        "", "abcd\n5\nbcd\n\nbc\n65\n-1\n-1\nB\n0\nabc\n", 0},
    {"toInt_truncates",        b_toint,             "", "3\n-3\n", 0},
    {"readLine_keeps_LF",      b_readline,          "hello\nworld\n", "hello\nworld\n", 0},
    {"readDouble",             b_readdouble,        "2.5\n", "2.50\n", 0},
    {"readInt_edge_cases",     b_readint_edge,      "-12\n 5\n99999999999\n", "-12\n0\n0\n", 0},

    {"relational_ops",         b_relational,        "", "TFTTFTFTTTTTFFTT", 0},
    {"if_scopes_shadowing",    b_if_scopes,         "", "29s1\n", 0},
    {"assign_outer_in_if",     b_outer_assign,      "", "5\n", 0},
    {"if_unit_function",       b_if_unit_func,      "", "smallbig", 0},

    {"factorial_5",            b_factorial,         "5\n", "120\n", 0},
    {"factorial_0",            b_factorial,         "0\n", "1\n", 0},
    {"factorial_bad_input",    b_factorial,         "abc\n", "1\n", 0},
    {"factorial_10",           b_factorial,         "10\n", "3628800\n", 0},
    {"while_decl_in_body",     b_while_decl,        "", "014\n", 0},
    {"nested_loops_labels",    b_nested_loops,      "", "6\n", 0},
    {"while_zero_iterations",  b_while_zero,        "", "done", 0},

    {"fib_recursion",          b_fib,               "", "55\n", 0},
    {"deep_recursion",         b_deep_recursion,    "", "500500\n", 0},
    {"mutual_recursion",       b_mutual_recursion,  "", "1\n0\n1\n", 0},
    {"return_in_loop",         b_return_in_loop,    "", "5\n-1\n", 0},
    {"unit_functions",         b_unit_funcs,        "", "hihi()", 0},
    {"bare_return",            b_bare_return,       "", "pos", 0},
    {"param_order",            b_param_order,       "", "7\n-7\nxyz", 0},
    {"same_names_in_funcs",    b_names_across_funcs,"", "2\n4\n", 0},
    {"implicit_return_values", b_implicit_return,   "", "5\n-1\n0\n1\n", 0},
    {"call_result_discarded",  b_call_discard,      "", "xx1", 0},
    {"main_name_and_position", b_main_name,         "", "hm", 0},
    {"labels_across_funcs",    b_labels_across_funcs,"", "abcd", 0},
    {"var_reassign",           b_reassign,          "", "abc\n2.50\n", 0},

    {"err_int_div_by_zero",    b_div_zero,          "", "a", 57},
    {"err_substr_start_gt_end",b_substr_start_gt_end,"", "a", 40},
    {"err_substr_start_eq_len",b_substr_start_eq_len,"", "a", 40},
    {"err_substr_start_neg",   b_substr_start_neg,  "", "a", 40},
    {"err_substr_end_neg",     b_substr_end_neg,    "", "a", 40},
    {"err_substr_end_gt_len",  b_substr_end_gt_len, "", "a", 40},
    {"err_substr_empty_str",   b_substr_empty,      "", "a", 40},
    {"err_chr_out_of_range",   b_chr_range,         "", "a", 58},
};

/* ------------------------------------------------------------------ */
/*  Runner                                                            */
/* ------------------------------------------------------------------ */
static char tmpdir[64];
static const char *interp = "ic26int";
static int have_interp = 0;

static void show_escaped(const char *s, size_t n)
{
    putchar('"');
    for (size_t i = 0; i < n; i++) {
        unsigned char c = (unsigned char)s[i];
        if (c == '\n') fputs("\\n", stdout);
        else if (c == '\t') fputs("\\t", stdout);
        else if (c == '\\') fputs("\\\\", stdout);
        else if (c == '"') fputs("\\\"", stdout);
        else if (c < 32 || c > 126) printf("\\x%02x", c);
        else putchar(c);
    }
    putchar('"');
}

static char *slurp(const char *path, size_t *len)
{
    FILE *f = fopen(path, "rb");
    if (!f) { *len = 0; return calloc(1, 1); }
    size_t cap = 4096, n = 0;
    char *buf = malloc(cap);
    size_t r;
    while ((r = fread(buf + n, 1, cap - n - 1, f)) > 0) {
        n += r;
        if (n + 1 >= cap) { cap *= 2; buf = realloc(buf, cap); }
    }
    fclose(f);
    buf[n] = '\0';
    *len = n;
    return buf;
}

/* returns 1 = pass, 0 = fail */
static int run_case(const Case *c, int *behaviour_checked)
{
    char code[256], in[256], out[256];
    snprintf(code, sizeof code, "%s/%s.code", tmpdir, c->name);
    snprintf(in,   sizeof in,   "%s/%s.in",   tmpdir, c->name);
    snprintf(out,  sizeof out,  "%s/%s.out",  tmpdir, c->name);

    fflush(stdout);
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 0; }
    if (pid == 0) {                                   /* child: run the generator */
        alarm(10);
        if (!freopen(code, "w", stdout)) _exit(98);
        scope_counter = 0;
        AstNode *ast = c->build();
        generateProgram(ast);
        fflush(stdout);
        _exit(0);
    }
    int st;
    waitpid(pid, &st, 0);
    if (WIFSIGNALED(st)) {
        printf("  codegen crashed with signal %d (%s)\n", WTERMSIG(st), strsignal(WTERMSIG(st)));
        return 0;
    }
    if (WEXITSTATUS(st) != 0) {
        printf("  codegen exited with code %d (expected 0)\n", WEXITSTATUS(st));
        return 0;
    }

    size_t clen;
    char *generated = slurp(code, &clen);
    int ok = 1;
    if (clen < 10 || strncmp(generated, ".IFJcode26", 10) != 0) {
        printf("  output does not start with the .IFJcode26 header\n");
        ok = 0;
    }
    free(generated);
    if (!ok || !have_interp) return ok;

    FILE *f = fopen(in, "w");
    if (!f) { perror("fopen"); return 0; }
    fputs(c->input, f);
    fclose(f);

    char cmd[1024];
    snprintf(cmd, sizeof cmd, "timeout 10 %s %s < %s > %s 2>%s/%s.err", interp, code, in, out, tmpdir, c->name);
    int sr = system(cmd);
    int rc = WIFEXITED(sr) ? WEXITSTATUS(sr) : -1;
    *behaviour_checked = 1;

    size_t olen;
    char *actual = slurp(out, &olen);
    size_t elen = strlen(c->expected);
    if (olen != elen || memcmp(actual, c->expected, elen) != 0) {
        printf("  stdout mismatch\n    expected: "); show_escaped(c->expected, elen);
        printf("\n    actual:   "); show_escaped(actual, olen); printf("\n");
        ok = 0;
    }
    if (rc != c->rc) {
        printf("  interpreter exit code %d, expected %d%s\n", rc, c->rc, rc == 124 ? " (timeout)" : "");
        ok = 0;
    }
    free(actual);
    return ok;
}

int main(int argc, char **argv)
{
    const char *filter = NULL;

    snprintf(tmpdir, sizeof tmpdir, "/tmp/codegen_tests_XXXXXX");
    if (!mkdtemp(tmpdir)) { perror("mkdtemp"); return 2; }

    if (getenv("IC26INT")) interp = getenv("IC26INT");
    /* arguments: an executable file = path to the interpreter, anything else = name filter */
    for (int i = 1; i < argc; i++) {
        if (access(argv[i], X_OK) == 0) interp = argv[i];
        else filter = argv[i];
    }
    char probe[256];
    snprintf(probe, sizeof probe, "command -v %s >/dev/null 2>&1", interp);
    have_interp = (system(probe) == 0);
    printf("Interpreter '%s': %s\n", interp,
           have_interp ? "found - running behavioural checks"
                       : "NOT found - only structural checks (set IC26INT to enable behavioural tests)");
    printf("Generated code and interpreter output are kept in %s\n\n", tmpdir);

    int total = 0, passed = 0, behaviour = 0;
    for (size_t i = 0; i < sizeof CASES / sizeof CASES[0]; i++) {
        const Case *c = &CASES[i];
        if (filter && !strstr(c->name, filter)) continue;
        total++;
        int checked = 0;
        int ok = run_case(c, &checked);
        behaviour += checked;
        passed += ok;
        printf("[%s] %s\n", ok ? "PASS" : "FAIL", c->name);
    }

    printf("\n%d/%d tests passed (%d with behavioural check)\n", passed, total, behaviour);
    return passed == total ? 0 : 1;
}
