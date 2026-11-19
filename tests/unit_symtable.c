/* AI-generated (Claude). Prompt: "Generate me tests for whole symtable based on
 * the brief" (unit tests of symtable.c in plain C, no framework) */
#include "unit_utils.h"

#include "hashtable.h"
#include "symtable.h"

// ------------------------------------------------------------------ lifecycle

static void test_init_global_scope_usable(void) {
    Symtable *s = symtableInit();
    REQUIRE(s != NULL);
    CHECK(symtableScopeID(s) == 0);
    CHECK(symtableSearch(s, "anything") == NULL);
    CHECK(symtableSearchCurrentScope(s, "anything") == NULL);
    symtableFree(s);
}

static void test_init_and_free(void) {
    Symtable *s = symtableInit();
    REQUIRE(s != NULL);
    symtableFree(s);
}

static void test_free_null(void) { EXPECT_SURVIVES(symtableFree(NULL)); }

static void test_push_scope(void) {
    CHECK(!symtablePushScope(NULL));
    Symtable *s = symtableInit();
    REQUIRE(s != NULL);
    CHECK(symtableScopeID(s) == 0);
    CHECK(symtablePushScope(s));
    CHECK(symtableScopeID(s) == 1);
    CHECK(symtablePushScope(s));
    CHECK(symtableScopeID(s) == 2);
    symtableFree(s);
}

static void test_scope_ids_unique_and_stored_in_symbols(void) {
    Symtable *s = symtableInit();
    REQUIRE(s != NULL);

    SymbolData *g = symtableInsertVar(s, "g", TYPE_INT, true);
    REQUIRE(g != NULL);
    CHECK(g->scopeID == 0);
    CHECK(symtableScopeID(s) == 0);

    REQUIRE(symtablePushScope(s));
    CHECK(symtableScopeID(s) == 1);
    SymbolData *v1 = symtableInsertVar(s, "a", TYPE_INT, true);
    REQUIRE(v1 != NULL);
    CHECK(v1->scopeID == 1);

    symtablePopScope(s);
    CHECK(symtableScopeID(s) == 0);

    REQUIRE(symtablePushScope(s));
    CHECK(symtableScopeID(s) == 2);
    SymbolData *v2 = symtableInsertVar(s, "a", TYPE_INT, true);
    REQUIRE(v2 != NULL);
    CHECK(v2->scopeID == 2);

    symtableFree(s);
}

static void test_pop_global_scope_is_ignored(void) {
    Symtable *s = symtableInit();
    REQUIRE(s != NULL);
    REQUIRE(symtableInsertVar(s, "g", TYPE_INT, true) != NULL);
    symtablePopScope(s); // top == 0 -> must do nothing
    symtablePopScope(s);
    CHECK(symtableScopeID(s) == 0);
    CHECK(symtableSearch(s, "g") != NULL);
    CHECK(symtableInsertVar(s, "h", TYPE_INT, true) != NULL); // still usable
    symtableFree(s);
}

static void test_pop_null(void) { EXPECT_SURVIVES(symtablePopScope(NULL)); }

// -------------------------------------------------------------------- inserts

static void test_insert_var_fields(void) {
    Symtable *s = symtableInit();
    REQUIRE(s != NULL);
    char buf[] = "counter";
    SymbolData *sym = symtableInsertVar(s, buf, TYPE_DOUBLE, false);
    REQUIRE(sym != NULL);
    CHECK(sym->sType == VAR);
    CHECK(sym->dtype == TYPE_DOUBLE);
    CHECK(!sym->isDefined);
    CHECK(sym->scopeID == 0);
    CHECK_STR(sym->symbolName, "counter");
    CHECK(sym->value.funcData.params == NULL);
    CHECK(sym->value.funcData.paramCount == 0u);
    CHECK(sym->value.funcData.listCount == 0u);
    symtableFree(s);
}

static void test_symbol_name_is_a_copy(void) {
    Symtable *s = symtableInit();
    REQUIRE(s != NULL);
    char buf[] = "counter";
    SymbolData *sym = symtableInsertVar(s, buf, TYPE_INT, true);
    REQUIRE(sym != NULL);
    CHECK(sym->symbolName != buf);
    buf[0] = 'X'; // the token buffer changes / dies later
    CHECK_STR(sym->symbolName, "counter");
    CHECK(symtableSearch(s, "counter") == sym);
    CHECK(symtableSearch(s, "Xounter") == NULL);
    symtableFree(s);
}

static void test_insert_val(void) {
    Symtable *s = symtableInit();
    REQUIRE(s != NULL);
    SymbolData *sym = symtableInsertVal(s, "pi", TYPE_DOUBLE, true);
    REQUIRE(sym != NULL);
    CHECK(sym->sType == VAL);
    CHECK(sym->dtype == TYPE_DOUBLE);
    CHECK(sym->isDefined);
    CHECK(sym->scopeID == 0);
    symtableFree(s);
}

static void test_insert_function(void) {
    Symtable *s = symtableInit();
    REQUIRE(s != NULL);
    SymbolData *f = symtableInsertFunction(s, "main", TYPE_UNIT, true);
    REQUIRE(f != NULL);
    CHECK(f->sType == FUNCTION);
    CHECK(f->dtype == TYPE_UNIT);
    CHECK(f->scopeID == 0);
    CHECK(f->value.funcData.params == NULL);
    CHECK(f->value.funcData.paramCount == 0u);
    CHECK(f->value.funcData.listCount == 0u);
    symtableFree(s);
}

static void test_insert_null_arguments(void) {
    Symtable *s = symtableInit();
    REQUIRE(s != NULL);
    CHECK(symtableInsertVar(NULL, "x", TYPE_INT, true) == NULL);
    CHECK(symtableInsertVar(s, NULL, TYPE_INT, true) == NULL);
    CHECK(symtableInsertVal(NULL, "x", TYPE_INT, true) == NULL);
    CHECK(symtableInsertVal(s, NULL, TYPE_INT, true) == NULL);
    CHECK(symtableInsertFunction(NULL, "f", TYPE_INT, true) == NULL);
    CHECK(symtableInsertFunction(s, NULL, TYPE_INT, true) == NULL);
    symtableFree(s);
}

static void test_function_defined_flag_via_pointer(void) {
    Symtable *s = symtableInit();
    REQUIRE(s != NULL);
    SymbolData *f = symtableInsertFunction(s, "later", TYPE_INT, false);
    REQUIRE(f != NULL);
    CHECK(!symtableSearch(s, "later")->isDefined);
    f->isDefined = true;
    CHECK(symtableSearch(s, "later")->isDefined);
    symtableFree(s);
}

// --------------------------------------------------------------------- search

static void test_search_returns_inserted_symbol(void) {
    Symtable *s = symtableInit();
    REQUIRE(s != NULL);
    SymbolData *sym = symtableInsertVar(s, "x", TYPE_INT, true);
    CHECK(symtableSearch(s, "x") == sym);
    CHECK(symtableSearchCurrentScope(s, "x") == sym);
    symtableFree(s);
}

static void test_search_missing_and_null(void) {
    Symtable *s = symtableInit();
    REQUIRE(s != NULL);
    CHECK(symtableSearch(s, "nope") == NULL);
    CHECK(symtableSearch(s, NULL) == NULL);
    CHECK(symtableSearch(NULL, "x") == NULL);
    CHECK(symtableSearchCurrentScope(s, "nope") == NULL);
    CHECK(symtableSearchCurrentScope(s, NULL) == NULL);
    CHECK(symtableSearchCurrentScope(NULL, "x") == NULL);
    symtableFree(s);
}

static void test_search_is_case_sensitive(void) {
    Symtable *s = symtableInit();
    REQUIRE(s != NULL);
    symtableInsertVar(s, "Value", TYPE_INT, true);
    CHECK(symtableSearch(s, "value") == NULL);
    CHECK(symtableSearch(s, "Value") != NULL);
    symtableFree(s);
}

static void test_outer_visible_current_scope_not(void) {
    Symtable *s = symtableInit();
    REQUIRE(s != NULL);
    SymbolData *outer = symtableInsertVar(s, "x", TYPE_INT, true);
    REQUIRE(symtablePushScope(s));
    CHECK(symtableSearch(s, "x") == outer);
    CHECK(symtableSearchCurrentScope(s, "x") == NULL);
    symtableFree(s);
}

static void test_shadowing_allowed(void) {
    Symtable *s = symtableInit();
    REQUIRE(s != NULL);
    SymbolData *outer = symtableInsertVar(s, "x", TYPE_INT, true);
    CHECK(outer->scopeID == 0);
    REQUIRE(symtablePushScope(s));
    SymbolData *inner = symtableInsertVal(s, "x", TYPE_STRING, true);
    REQUIRE(inner != NULL);
    CHECK(inner != outer);
    CHECK(inner->scopeID == 1);

    CHECK(symtableSearch(s, "x") == inner);
    CHECK(symtableSearchCurrentScope(s, "x") == inner);
    CHECK(symtableSearch(s, "x")->dtype == TYPE_STRING);

    symtablePopScope(s);
    CHECK(symtableSearch(s, "x") == outer);
    CHECK(symtableSearch(s, "x")->dtype == TYPE_INT);
    symtableFree(s);
}

static void test_innermost_scope_wins_in_three_levels(void) {
    Symtable *s = symtableInit();
    REQUIRE(s != NULL);
    SymbolData *a = symtableInsertVar(s, "v", TYPE_INT, true);
    symtablePushScope(s);
    symtablePushScope(s);
    CHECK(symtableSearch(s, "v") == a); // found two levels up
    SymbolData *c = symtableInsertVar(s, "v", TYPE_BOOL, true);
    CHECK(c->scopeID == 2);
    CHECK(symtableSearch(s, "v") == c);
    symtablePopScope(s);
    CHECK(symtableSearch(s, "v") == a);
    symtableFree(s);
}

// ------------------------------------------------------------- scope lifetime

static void test_pop_removes_inner_symbols(void) {
    Symtable *s = symtableInit();
    REQUIRE(s != NULL);
    symtablePushScope(s);
    REQUIRE(symtableInsertVar(s, "tmp", TYPE_INT, true) != NULL);
    CHECK(symtableSearch(s, "tmp") != NULL);
    symtablePopScope(s);
    CHECK(symtableSearch(s, "tmp") == NULL);
    symtableFree(s);
}

static void test_reused_scope_slot_starts_empty(void) {
    Symtable *s = symtableInit();
    REQUIRE(s != NULL);
    symtablePushScope(s);
    symtableInsertVar(s, "x", TYPE_INT, true);
    symtablePopScope(s);

    symtablePushScope(s); // reuses the same stack slot
    CHECK(symtableSearchCurrentScope(s, "x") == NULL);
    CHECK(symtableInsertVar(s, "x", TYPE_INT, true) != NULL); // no redefinition
    symtableFree(s);
}

static void test_same_name_in_sibling_scopes(void) {
    Symtable *s = symtableInit();
    REQUIRE(s != NULL);
    symtablePushScope(s);
    CHECK(symtableInsertVar(s, "i", TYPE_INT, true) != NULL);
    symtablePopScope(s);
    symtablePushScope(s);
    CHECK(symtableInsertVar(s, "i", TYPE_INT, true) != NULL);
    symtableFree(s);
}

static void test_deep_nesting_grows_stack(void) {
    // INITIAL_CAPACITY is 16 -> this forces several reallocs.
    const int depth = 100;
    char name[16];
    Symtable *s = symtableInit();
    REQUIRE(s != NULL);

    for (int i = 0; i < depth; i++) {
        REQUIRE(symtablePushScope(s));
        CHECK(symtableScopeID(s) == i + 1);
        snprintf(name, sizeof name, "v%d", i);
        REQUIRE(symtableInsertVar(s, name, TYPE_INT, true) != NULL);
    }
    for (int i = 0; i < depth; i++) {
        snprintf(name, sizeof name, "v%d", i);
        CHECK(symtableSearch(s, name) != NULL);
    }
    CHECK(symtableSearchCurrentScope(s, "v99") != NULL);
    CHECK(symtableSearchCurrentScope(s, "v98") == NULL);

    for (int i = depth - 1; i >= 0; i--) {
        snprintf(name, sizeof name, "v%d", i);
        CHECK(symtableSearch(s, name) != NULL);
        symtablePopScope(s);
        CHECK(symtableSearch(s, name) == NULL);
    }
    CHECK(symtableInsertVar(s, "after", TYPE_INT, true) !=
          NULL); // global still OK
    symtableFree(s);
}

static void test_many_symbols_in_one_scope(void) {
    enum { N = 1000 };
    static SymbolData *syms[N];
    char name[16];
    Symtable *s = symtableInit();
    REQUIRE(s != NULL);

    for (int i = 0; i < N; i++) {
        snprintf(name, sizeof name, "sym%d", i);
        syms[i] = symtableInsertVar(s, name, TYPE_INT, true);
        REQUIRE(syms[i] != NULL);
    }
    for (int i = 0; i < N; i++) {
        snprintf(name, sizeof name, "sym%d", i);
        CHECK(symtableSearch(s, name) == syms[i]); // pointers stay valid
    }
    symtableFree(s);
}

// ----------------------------------------------------------------- parameters

static void test_add_param_order_and_count(void) {
    Symtable *s = symtableInit();
    REQUIRE(s != NULL);
    SymbolData *f = symtableInsertFunction(s, "f", TYPE_INT, true);
    REQUIRE(f != NULL);
    REQUIRE(symtableStartParamList(f));
    CHECK(symtableAddParam(s, f, "a", TYPE_INT));
    CHECK(symtableAddParam(s, f, "b", TYPE_STRING));
    CHECK(symtableAddParam(s, f, "c", TYPE_DOUBLE));

    CHECK(f->value.funcData.paramCount == 3u);
    CHECK(f->value.funcData.listCount == 1u);
    FunctionParam *p = f->value.funcData.params;
    REQUIRE(p != NULL);
    CHECK_STR(p->paramName, "a");
    CHECK(p->dType == TYPE_INT);
    CHECK(p->listIndex == 0u);
    p = p->next;
    REQUIRE(p != NULL);
    CHECK_STR(p->paramName, "b");
    CHECK(p->dType == TYPE_STRING);
    CHECK(p->listIndex == 0u);
    p = p->next;
    REQUIRE(p != NULL);
    CHECK_STR(p->paramName, "c");
    CHECK(p->dType == TYPE_DOUBLE);
    CHECK(p->listIndex == 0u);
    CHECK(p->next == NULL);
    symtableFree(s);
}

static void test_param_name_is_a_copy(void) {
    Symtable *s = symtableInit();
    REQUIRE(s != NULL);
    SymbolData *f = symtableInsertFunction(s, "f", TYPE_UNIT, true);
    REQUIRE(f != NULL);
    REQUIRE(symtableStartParamList(f));
    char buf[] = "param";
    REQUIRE(symtableAddParam(s, f, buf, TYPE_INT));
    buf[0] = 'X';
    CHECK_STR(f->value.funcData.params->paramName, "param");
    symtableFree(s);
}

static void test_add_param_invalid_targets(void) {
    Symtable *s = symtableInit();
    REQUIRE(s != NULL);
    SymbolData *v = symtableInsertVar(s, "x", TYPE_INT, true);
    SymbolData *f = symtableInsertFunction(s, "f", TYPE_INT, true);
    REQUIRE(v != NULL && f != NULL);
    CHECK(!symtableStartParamList(v));
    CHECK(!symtableStartParamList(NULL));
    REQUIRE(symtableStartParamList(f));
    CHECK(!symtableAddParam(s, v, "p", TYPE_INT)); // not a function
    CHECK(!symtableAddParam(s, NULL, "p", TYPE_INT));
    CHECK(!symtableAddParam(s, f, NULL, TYPE_INT));
    CHECK(f->value.funcData.paramCount == 0u);
    CHECK(v->value.funcData.paramCount == 0u);
    symtableFree(s);
}

static void test_same_param_name_in_different_functions(void) {
    Symtable *s = symtableInit();
    REQUIRE(s != NULL);
    SymbolData *f = symtableInsertFunction(s, "f", TYPE_INT, true);
    SymbolData *g = symtableInsertFunction(s, "g", TYPE_INT, true);
    REQUIRE(f != NULL && g != NULL);
    REQUIRE(symtableStartParamList(f));
    REQUIRE(symtableStartParamList(g));
    CHECK(symtableAddParam(s, f, "n", TYPE_INT));
    CHECK(symtableAddParam(s, g, "n", TYPE_INT));
    symtableFree(s);
}

static void test_free_with_params_in_several_scopes(void) {
    // Content is verified by AddressSanitizer / Valgrind (no leak, no double
    // free).
    char fn[16], pn[16];
    Symtable *s = symtableInit();
    REQUIRE(s != NULL);
    for (int i = 0; i < 20; i++) {
        snprintf(fn, sizeof fn, "fn%d", i);
        SymbolData *f = symtableInsertFunction(s, fn, TYPE_INT, true);
        REQUIRE(f != NULL);
        REQUIRE(symtableStartParamList(f));
        for (int j = 0; j < 5; j++) {
            snprintf(pn, sizeof pn, "p%d", j);
            CHECK(symtableAddParam(s, f, pn, TYPE_INT));
        }
    }
    symtablePushScope(s);
    symtableInsertVar(s, "local", TYPE_STRING, true);
    symtablePushScope(s);
    symtableInsertVal(s, "deeper", TYPE_BOOL, true);
    symtableFree(s); // freed with scopes still open
}

// -------------------------------------------------- curried parameter lists

static void test_add_param_without_start_list_fails(void) {
    Symtable *s = symtableInit();
    REQUIRE(s != NULL);
    SymbolData *f = symtableInsertFunction(s, "f", TYPE_INT, true);
    REQUIRE(f != NULL);
    // listCount is 0 -> adding param before starting a list must fail
    CHECK(!symtableAddParam(s, f, "a", TYPE_INT));
    CHECK(f->value.funcData.paramCount == 0u);
    CHECK(f->value.funcData.listCount == 0u);
    symtableFree(s);
}

static void test_no_parens_vs_empty_parens(void) {
    Symtable *s = symtableInit();
    REQUIRE(s != NULL);

    // def answer: Int -> 0 parameter lists
    SymbolData *f_no_parens =
        symtableInsertFunction(s, "answer", TYPE_INT, true);
    REQUIRE(f_no_parens != NULL);
    CHECK(f_no_parens->value.funcData.listCount == 0u);
    CHECK(symtableParamListSize(f_no_parens, 0) == 0u);

    // def answerWithParens(): Int -> 1 empty parameter list
    SymbolData *f_empty_parens =
        symtableInsertFunction(s, "answerWithParens", TYPE_INT, true);
    REQUIRE(f_empty_parens != NULL);
    CHECK(symtableStartParamList(f_empty_parens));
    CHECK(f_empty_parens->value.funcData.listCount == 1u);
    CHECK(f_empty_parens->value.funcData.paramCount == 0u);
    CHECK(symtableParamListSize(f_empty_parens, 0) == 0u);
    CHECK(symtableGetParam(f_empty_parens, 0, 0) == NULL);

    // def twoEmpty()(): Int -> 2 empty parameter lists
    SymbolData *f_two_empty =
        symtableInsertFunction(s, "twoEmpty", TYPE_INT, true);
    REQUIRE(f_two_empty != NULL);
    CHECK(symtableStartParamList(f_two_empty));
    CHECK(symtableStartParamList(f_two_empty));
    CHECK(f_two_empty->value.funcData.listCount == 2u);
    CHECK(symtableParamListSize(f_two_empty, 0) == 0u);
    CHECK(symtableParamListSize(f_two_empty, 1) == 0u);

    symtableFree(s);
}

static void test_multiple_param_lists_currying(void) {
    Symtable *s = symtableInit();
    REQUIRE(s != NULL);

    // foo(a: Int, b: String)(c: Double)
    SymbolData *foo = symtableInsertFunction(s, "foo", TYPE_UNIT, true);
    REQUIRE(foo != NULL);

    // First list: (a, b)
    CHECK(symtableStartParamList(foo));
    CHECK(symtableAddParam(s, foo, "a", TYPE_INT));
    CHECK(symtableAddParam(s, foo, "b", TYPE_STRING));

    // Second list: (c)
    CHECK(symtableStartParamList(foo));
    CHECK(symtableAddParam(s, foo, "c", TYPE_DOUBLE));

    CHECK(foo->value.funcData.listCount == 2u);
    CHECK(foo->value.funcData.paramCount == 3u);

    CHECK(symtableParamListSize(foo, 0) == 2u);
    CHECK(symtableParamListSize(foo, 1) == 1u);
    CHECK(symtableParamListSize(foo, 2) == 0u); // out of bounds list

    const FunctionParam *p00 = symtableGetParam(foo, 0, 0);
    const FunctionParam *p01 = symtableGetParam(foo, 0, 1);
    const FunctionParam *p10 = symtableGetParam(foo, 1, 0);

    REQUIRE(p00 != NULL);
    CHECK_STR(p00->paramName, "a");
    CHECK(p00->dType == TYPE_INT);
    CHECK(p00->listIndex == 0u);

    REQUIRE(p01 != NULL);
    CHECK_STR(p01->paramName, "b");
    CHECK(p01->dType == TYPE_STRING);
    CHECK(p01->listIndex == 0u);

    REQUIRE(p10 != NULL);
    CHECK_STR(p10->paramName, "c");
    CHECK(p10->dType == TYPE_DOUBLE);
    CHECK(p10->listIndex == 1u);

    // Out-of-bounds parameter indices
    CHECK(symtableGetParam(foo, 0, 2) == NULL);
    CHECK(symtableGetParam(foo, 1, 1) == NULL);
    CHECK(symtableGetParam(foo, 2, 0) == NULL);

    symtableFree(s);
}

static void test_mixed_empty_and_nonempty_param_lists(void) {
    Symtable *s = symtableInit();
    REQUIRE(s != NULL);

    // foo()()()(a: Int)()
    SymbolData *foo = symtableInsertFunction(s, "foo", TYPE_INT, true);
    REQUIRE(foo != NULL);

    CHECK(symtableStartParamList(foo)); // list 0: ()
    CHECK(symtableStartParamList(foo)); // list 1: ()
    CHECK(symtableStartParamList(foo)); // list 2: ()
    CHECK(symtableStartParamList(foo)); // list 3: (a)
    CHECK(symtableAddParam(s, foo, "a", TYPE_INT));
    CHECK(symtableStartParamList(foo)); // list 4: ()

    CHECK(foo->value.funcData.listCount == 5u);
    CHECK(foo->value.funcData.paramCount == 1u);

    CHECK(symtableParamListSize(foo, 0) == 0u);
    CHECK(symtableParamListSize(foo, 1) == 0u);
    CHECK(symtableParamListSize(foo, 2) == 0u);
    CHECK(symtableParamListSize(foo, 3) == 1u);
    CHECK(symtableParamListSize(foo, 4) == 0u);

    const FunctionParam *pa = symtableGetParam(foo, 3, 0);
    REQUIRE(pa != NULL);
    CHECK_STR(pa->paramName, "a");
    CHECK(pa->listIndex == 3u);
    CHECK(symtableGetParam(foo, 0, 0) == NULL);
    CHECK(symtableGetParam(foo, 4, 0) == NULL);

    symtableFree(s);
}

static void test_param_list_invalid_queries(void) {
    Symtable *s = symtableInit();
    REQUIRE(s != NULL);
    SymbolData *v = symtableInsertVar(s, "x", TYPE_INT, true);
    REQUIRE(v != NULL);

    CHECK(symtableParamListSize(NULL, 0) == 0u);
    CHECK(symtableParamListSize(v, 0) == 0u);
    CHECK(symtableGetParam(NULL, 0, 0) == NULL);
    CHECK(symtableGetParam(v, 0, 0) == NULL);

    symtableFree(s);
}

static void test_builtins_param_lists(void) {
    Symtable *s = symtableInit();
    REQUIRE(s != NULL);

    // Builtin with 0 params -> must still have 1 empty param list: readInt()
    SymbolData *readInt = symtableSearch(s, "readInt");
    REQUIRE(readInt != NULL);
    CHECK(readInt->sType == FUNCTION);
    CHECK(readInt->value.funcData.listCount == 1u);
    CHECK(readInt->value.funcData.paramCount == 0u);
    CHECK(symtableParamListSize(readInt, 0) == 0u);

    // Builtin with 3 params -> subStr(value, start, end)
    SymbolData *subStr = symtableSearch(s, "subStr");
    REQUIRE(subStr != NULL);
    CHECK(subStr->sType == FUNCTION);
    CHECK(subStr->value.funcData.listCount == 1u);
    CHECK(subStr->value.funcData.paramCount == 3u);
    CHECK(symtableParamListSize(subStr, 0) == 3u);

    const FunctionParam *p1 = symtableGetParam(subStr, 0, 1);
    REQUIRE(p1 != NULL);
    CHECK_STR(p1->paramName, "start");
    CHECK(p1->dType == TYPE_INT);

    symtableFree(s);
}

// ---------------------------------------- exit codes (run in a child process)

static void helper_redef_same_scope(void) {
    Symtable *s = symtableInit();
    symtableInsertVar(s, "x", TYPE_INT, true);
    symtableInsertVar(s, "x", TYPE_INT, true);
    symtableFree(s);
}
static void test_redefinition_same_scope_exits_4(void) {
    EXPECT_EXIT_CODE(helper_redef_same_scope(), 4);
}

static void helper_val_and_var(void) {
    Symtable *s = symtableInit();
    symtableInsertVal(s, "x", TYPE_INT, true);
    symtableInsertVar(s, "x", TYPE_STRING, true);
    symtableFree(s);
}
static void test_val_and_var_same_name_exits_4(void) {
    EXPECT_EXIT_CODE(helper_val_and_var(), 4);
}

static void helper_func_redef(void) {
    Symtable *s = symtableInit();
    symtableInsertFunction(s, "f", TYPE_INT, true);
    symtableInsertFunction(s, "f", TYPE_INT, true);
    symtableFree(s);
}
static void test_function_redefinition_exits_4(void) {
    EXPECT_EXIT_CODE(helper_func_redef(), 4);
}

static void helper_func_and_var(void) {
    Symtable *s = symtableInit();
    symtableInsertFunction(s, "name", TYPE_INT, true);
    symtableInsertVar(s, "name", TYPE_INT, true);
    symtableFree(s);
}
static void test_function_and_variable_same_name_exits_4(void) {
    EXPECT_EXIT_CODE(helper_func_and_var(), 4);
}

static void helper_redef_inner(void) {
    Symtable *s = symtableInit();
    symtablePushScope(s);
    symtableInsertVar(s, "y", TYPE_INT, true);
    symtableInsertVal(s, "y", TYPE_INT, true);
    symtableFree(s);
}
static void test_redefinition_in_inner_scope_exits_4(void) {
    EXPECT_EXIT_CODE(helper_redef_inner(), 4);
}

static void helper_dup_param(void) {
    Symtable *s = symtableInit();
    SymbolData *f = symtableInsertFunction(s, "f", TYPE_INT, true);
    symtableStartParamList(f);
    symtableAddParam(s, f, "a", TYPE_INT);
    symtableAddParam(s, f, "b", TYPE_INT);
    symtableAddParam(s, f, "a", TYPE_STRING);
    symtableFree(s);
}
static void test_duplicate_parameter_exits_4(void) {
    EXPECT_EXIT_CODE(helper_dup_param(), 4);
}

static void helper_dup_param_across_lists(void) {
    Symtable *s = symtableInit();
    SymbolData *f = symtableInsertFunction(s, "f", TYPE_INT, true);
    symtableStartParamList(f);
    symtableAddParam(s, f, "a", TYPE_INT);
    symtableStartParamList(f);
    symtableAddParam(s, f, "a", TYPE_STRING);
    symtableFree(s);
}
static void test_duplicate_param_across_lists_exits_4(void) {
    EXPECT_EXIT_CODE(helper_dup_param_across_lists(), 4);
}

// ----------------------------------------------------------------------- main

int main(int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "--no-death") == 0) {
        g_skip_death = 1;
    }

    RUN(test_init_global_scope_usable);
    RUN(test_init_and_free);
    RUN(test_free_null);
    RUN(test_push_scope);
    RUN(test_scope_ids_unique_and_stored_in_symbols);
    RUN(test_pop_global_scope_is_ignored);
    RUN(test_pop_null);

    RUN(test_insert_var_fields);
    RUN(test_symbol_name_is_a_copy);
    RUN(test_insert_val);
    RUN(test_insert_function);
    RUN(test_insert_null_arguments);
    RUN(test_function_defined_flag_via_pointer);

    RUN(test_search_returns_inserted_symbol);
    RUN(test_search_missing_and_null);
    RUN(test_search_is_case_sensitive);
    RUN(test_outer_visible_current_scope_not);
    RUN(test_shadowing_allowed);
    RUN(test_innermost_scope_wins_in_three_levels);

    RUN(test_pop_removes_inner_symbols);
    RUN(test_reused_scope_slot_starts_empty);
    RUN(test_same_name_in_sibling_scopes);
    RUN(test_deep_nesting_grows_stack);
    RUN(test_many_symbols_in_one_scope);

    RUN(test_add_param_order_and_count);
    RUN(test_param_name_is_a_copy);
    RUN(test_add_param_invalid_targets);
    RUN(test_same_param_name_in_different_functions);
    RUN(test_free_with_params_in_several_scopes);

    RUN(test_add_param_without_start_list_fails);
    RUN(test_no_parens_vs_empty_parens);
    RUN(test_multiple_param_lists_currying);
    RUN(test_mixed_empty_and_nonempty_param_lists);
    RUN(test_param_list_invalid_queries);
    RUN(test_builtins_param_lists);

    RUN(test_redefinition_same_scope_exits_4);
    RUN(test_val_and_var_same_name_exits_4);
    RUN(test_function_redefinition_exits_4);
    RUN(test_function_and_variable_same_name_exits_4);
    RUN(test_redefinition_in_inner_scope_exits_4);
    RUN(test_duplicate_parameter_exits_4);
    RUN(test_duplicate_param_across_lists_exits_4);

    printf("\n%d checks, %d failed\n", g_checks, g_failed);
    return g_failed == 0 ? 0 : 1;
}