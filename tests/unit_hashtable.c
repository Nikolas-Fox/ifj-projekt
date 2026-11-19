/* AI-generated (Claude). Prompt: "Generate me test for hashtable based on the
 * brief" (unit tests of hashtable.c ) */
#include "unit_utils.h"

#include "hashtable.h"

// ------------------------------------------------------------------ helpers

#define MAX_KEYS 3100
static char g_keys[MAX_KEYS][16]; // "k0", "k1", ... (must outlive the tables)
static int g_vals[MAX_KEYS];

static void initKeys(void) {
    for (int i = 0; i < MAX_KEYS; i++) {
        snprintf(g_keys[i], sizeof g_keys[i], "k%d", i);
    }
}

#define NCOLLIDE 6
static char g_collide[NCOLLIDE][16];

/* Fills g_collide with n distinct keys that share one hash for this size. */
static int findCollidingKeys(size_t size, int n) {
    size_t target = hashFunction("c0", size);
    int found = 0;
    for (int i = 0; i < 100000 && found < n; i++) {
        char buf[16];
        snprintf(buf, sizeof buf, "c%d", i);
        if (hashFunction(buf, size) == target) {
            strcpy(g_collide[found++], buf);
        }
    }
    return found;
}

static int g_visited;
static void *g_seen[16];

static void countCallback(void *data) {
    if (g_visited < 16) {
        g_seen[g_visited] = data;
    }
    g_visited++;
}

static int seenCount(void *p) {
    int c = 0;
    for (int i = 0; i < g_visited && i < 16; i++) {
        if (g_seen[i] == p) {
            c++;
        }
    }
    return c;
}

// ------------------------------------------------------------- hashFunction

static void test_hash_known_values(void) {
    // djb2: h = 5381; h = h * 33 + c
    CHECK(hashFunction("", 1000000) == 5381u);
    CHECK(hashFunction("a", 1000000) == 177670u); // 5381 * 33 + 97
}

static void test_hash_in_range(void) {
    const char *keys[] = {"", "a", "foo", "variable_name_42", "\xff\xfe\x80"};
    const size_t sizes[] = {1, 2, 7, 4093};
    for (size_t k = 0; k < sizeof keys / sizeof keys[0]; k++) {
        for (size_t z = 0; z < sizeof sizes / sizeof sizes[0]; z++) {
            CHECK(hashFunction(keys[k], sizes[z]) < sizes[z]);
        }
    }
}

static void test_hash_deterministic(void) {
    CHECK(hashFunction("hello", 4093) == hashFunction("hello", 4093));
}

static void test_hash_null_or_zero_size(void) {
    CHECK(hashFunction(NULL, 4093) == 0u);
    CHECK(hashFunction("abc", 0) == 0u);
}

// ------------------------------------------------------------------- htInit

static void test_init_zero_size(void) { CHECK(htInit(0, 0) == NULL); }

static void test_init_creates_empty_table(void) {
    HashTable *t = htInit(11, 42);
    REQUIRE(t != NULL);
    CHECK(t->size == 11u);
    CHECK(t->count_occupied == 0u);
    CHECK(t->scopeID == 42);
    CHECK(htScopeID(t) == 42);
    REQUIRE(t->tableItems != NULL);
    for (size_t i = 0; i < t->size; i++) {
        CHECK(t->tableItems[i].state == EMPTY);
    }
    htFree(t);
}

static void test_scope_id_and_rehash_preserves_it(void) {
    HashTable *t = htInit(5, 7);
    REQUIRE(t != NULL);
    CHECK(htScopeID(t) == 7);

    for (int i = 0; i < 5; i++) {
        CHECK(htInsert(t, g_keys[i], &g_vals[i]) != NULL);
    }
    CHECK(htScopeID(t) == 7);
    htFree(t);
}

static void test_free_null(void) { EXPECT_SURVIVES(htFree(NULL)); }

// ------------------------------------------------ insert / search / update

static void test_insert_then_search(void) {
    HashTable *t = htInit(HASH_T_SIZE, 1);
    REQUIRE(t != NULL);
    int x = 1;
    HashTItem *item = htInsert(t, "foo", &x);
    REQUIRE(item != NULL);
    CHECK(item->state == OCCUPIED);
    CHECK(t->count_occupied == 1u);

    HashTItem *found = htSearch(t, "foo");
    REQUIRE(found != NULL);
    CHECK(found == item);
    CHECK(found->data == &x);
    CHECK_STR(found->key, "foo");
    htFree(t);
}

static void test_search_missing(void) {
    HashTable *t = htInit(HASH_T_SIZE, 1);
    REQUIRE(t != NULL);
    CHECK(htSearch(t, "nothing") == NULL);
    htFree(t);
}

static void test_insert_existing_updates(void) {
    HashTable *t = htInit(HASH_T_SIZE, 1);
    REQUIRE(t != NULL);
    int a = 1, b = 2;
    HashTItem *first = htInsert(t, "k", &a);
    HashTItem *second = htInsert(t, "k", &b);
    REQUIRE(second != NULL);
    CHECK(first == second);
    CHECK(htSearch(t, "k")->data == &b);
    CHECK(t->count_occupied == 1u);
    htFree(t);
}

static void test_insert_null_data_allowed(void) {
    HashTable *t = htInit(HASH_T_SIZE, 1);
    REQUIRE(t != NULL);
    HashTItem *item = htInsert(t, "k", NULL);
    REQUIRE(item != NULL);
    CHECK(item->data == NULL);
    CHECK(htSearch(t, "k") != NULL);
    htFree(t);
}

static void test_null_arguments(void) {
    HashTable *t = htInit(HASH_T_SIZE, 1);
    REQUIRE(t != NULL);
    int x = 0;
    CHECK(htInsert(NULL, "k", &x) == NULL);
    CHECK(htInsert(t, NULL, &x) == NULL);
    CHECK(htSearch(NULL, "k") == NULL);
    CHECK(htSearch(t, NULL) == NULL);
    CHECK(!htDelete(NULL, "k"));
    CHECK(!htDelete(t, NULL));
    CHECK(t->count_occupied == 0u);
    htFree(t);
}

static void test_keys_compared_by_content(void) {
    HashTable *t = htInit(HASH_T_SIZE, 1);
    REQUIRE(t != NULL);
    int x = 7;
    htInsert(t, "same", &x);
    char other[] = "same"; // different pointer, same text
    HashTItem *found = htSearch(t, other);
    REQUIRE(found != NULL);
    CHECK(found->data == &x);
    htFree(t);
}

// ------------------------------------------------------------------- delete

static void test_delete_marks_slot(void) {
    HashTable *t = htInit(HASH_T_SIZE, 1);
    REQUIRE(t != NULL);
    int x = 1;
    htInsert(t, "foo", &x);
    HashTItem *slot = htSearch(t, "foo");
    REQUIRE(slot != NULL);

    CHECK(htDelete(t, "foo"));
    CHECK(slot->state == DELETED);
    CHECK(slot->key == NULL);
    CHECK(slot->data == NULL);
    CHECK(htSearch(t, "foo") == NULL);
    CHECK(t->count_occupied == 0u);
    htFree(t);
}

static void test_delete_missing(void) {
    HashTable *t = htInit(HASH_T_SIZE, 1);
    REQUIRE(t != NULL);
    CHECK(!htDelete(t, "ghost"));
    CHECK(t->count_occupied == 0u);
    htFree(t);
}

static void test_delete_twice(void) {
    HashTable *t = htInit(HASH_T_SIZE, 1);
    REQUIRE(t != NULL);
    int x = 1;
    htInsert(t, "foo", &x);
    CHECK(htDelete(t, "foo"));
    CHECK(!htDelete(t, "foo"));
    CHECK(t->count_occupied == 0u);
    htFree(t);
}

// --------------------------------------------------------------- collisions

static void test_colliding_keys_all_found(void) {
    const size_t size = 11;
    HashTable *t = htInit(size, 1);
    REQUIRE(t != NULL);
    REQUIRE(findCollidingKeys(size, 4) == 4);
    int v[4] = {0, 1, 2, 3};
    for (int i = 0; i < 4; i++) {
        CHECK(htInsert(t, g_collide[i], &v[i]) != NULL);
    }
    for (int i = 0; i < 4; i++) {
        HashTItem *f = htSearch(t, g_collide[i]);
        REQUIRE(f != NULL);
        CHECK(f->data == &v[i]);
    }
    CHECK(t->count_occupied == 4u);
    htFree(t);
}

static void test_search_probes_past_tombstone(void) {
    const size_t size = 11;
    HashTable *t = htInit(size, 1);
    REQUIRE(t != NULL);
    REQUIRE(findCollidingKeys(size, 3) == 3);
    int v[3] = {0, 1, 2};
    for (int i = 0; i < 3; i++) {
        htInsert(t, g_collide[i], &v[i]);
    }
    CHECK(htDelete(t, g_collide[1])); // tombstone in the middle of the chain
    HashTItem *f = htSearch(t, g_collide[2]);
    REQUIRE(f != NULL);
    CHECK(f->data == &v[2]);
    htFree(t);
}

static void test_insert_reuses_first_deleted_slot(void) {
    const size_t size = 11;
    HashTable *t = htInit(size, 1);
    REQUIRE(t != NULL);
    REQUIRE(findCollidingKeys(size, 4) == 4);
    int v[4] = {0, 1, 2, 3};
    for (int i = 0; i < 3; i++) {
        htInsert(t, g_collide[i], &v[i]);
    }
    HashTItem *slotOfSecond = htSearch(t, g_collide[1]);
    CHECK(htDelete(t, g_collide[1]));

    HashTItem *inserted = htInsert(t, g_collide[3], &v[3]);
    CHECK(inserted == slotOfSecond); // the tombstone was reused
    CHECK(htSearch(t, g_collide[2]) != NULL);
    CHECK(t->count_occupied == 3u);
    htFree(t);
}

static void test_no_duplicate_after_delete_and_reinsert(void) {
    // Regression: A, B collide; delete A; insert B again must UPDATE B,
    // not create a second copy in A's old slot.
    const size_t size = 11;
    HashTable *t = htInit(size, 1);
    REQUIRE(t != NULL);
    REQUIRE(findCollidingKeys(size, 2) == 2);
    int a = 1, b = 2, b2 = 3;
    htInsert(t, g_collide[0], &a);
    htInsert(t, g_collide[1], &b);
    CHECK(htDelete(t, g_collide[0]));

    CHECK(htInsert(t, g_collide[1], &b2) != NULL);
    CHECK(t->count_occupied == 1u);
    CHECK(htSearch(t, g_collide[1])->data == &b2);

    CHECK(htDelete(t, g_collide[1]));
    CHECK(htSearch(t, g_collide[1]) == NULL); // no resurrected old copy
    CHECK(t->count_occupied == 0u);
    htFree(t);
}

// --------------------------------------------------------------- full table

static void test_rehash_on_high_load(void) {
    HashTable *t = htInit(5, 1);
    REQUIRE(t != NULL);

    size_t oldSize = t->size;

    for (int i = 0; i < 5; i++) {
        CHECK(htInsert(t, g_keys[i], &g_vals[i]) != NULL);
    }

    CHECK(t->size > oldSize);
    CHECK(t->count_occupied == 5u);

    for (int i = 0; i < 5; i++) {
        CHECK(htSearch(t, g_keys[i]) != NULL);
        CHECK(htSearch(t, g_keys[i])->data == &g_vals[i]);
    }

    htFree(t);
}

static void test_full_table_with_tombstones_reuses_deleted(void) {
    // No EMPTY slot left, but DELETED slots must still be usable.
    HashTable *t = htInit(5, 1);
    REQUIRE(t != NULL);
    for (int i = 0; i < 5; i++) {
        CHECK(htInsert(t, g_keys[i], &g_vals[i]) != NULL);
    }
    CHECK(htDelete(t, g_keys[0]));
    CHECK(htDelete(t, g_keys[1]));

    CHECK(htInsert(t, g_keys[5], &g_vals[5]) != NULL);
    CHECK(htInsert(t, g_keys[6], &g_vals[6]) != NULL);
    CHECK(t->count_occupied == 5u);
    CHECK(htInsert(t, g_keys[7], &g_vals[7]) != NULL); // genuinely full again

    CHECK(htSearch(t, g_keys[5]) != NULL);
    CHECK(htSearch(t, g_keys[6]) != NULL);
    CHECK(htSearch(t, g_keys[0]) == NULL);
    CHECK(htSearch(t, g_keys[1]) == NULL);
    htFree(t);
}

static void test_size_one_table(void) {
    HashTable *t = htInit(1, 1);
    REQUIRE(t != NULL);
    int a = 1, b = 2;
    CHECK(htInsert(t, "a", &a) != NULL);
    CHECK(htInsert(t, "b", &b) != NULL);
    CHECK(htSearch(t, "a") != NULL);
    CHECK(htSearch(t, "b") != NULL);
    CHECK(htDelete(t, "a"));
    CHECK(htInsert(t, "b", &b) != NULL);
    htFree(t);
}

// --------------------------------------------------------------------- load

static void test_many_items(void) {
    const int n = 3000;
    HashTable *t = htInit(HASH_T_SIZE, 1);
    REQUIRE(t != NULL);
    for (int i = 0; i < n; i++) {
        REQUIRE(htInsert(t, g_keys[i], &g_vals[i]) != NULL);
    }
    CHECK(t->count_occupied == (size_t)n);
    for (int i = 0; i < n; i++) {
        HashTItem *f = htSearch(t, g_keys[i]);
        REQUIRE(f != NULL);
        CHECK(f->data == &g_vals[i]);
    }
    for (int i = 0; i < n; i++) {
        REQUIRE(htDelete(t, g_keys[i]));
    }
    CHECK(t->count_occupied == 0u);
    for (int i = 0; i < n; i++) {
        CHECK(htSearch(t, g_keys[i]) == NULL);
    }
    for (int i = 0; i < n; i++) {
        REQUIRE(htInsert(t, g_keys[i], &g_vals[i]) != NULL);
    }
    CHECK(t->count_occupied == (size_t)n);
    htFree(t);
}

// -------------------------------------------------------------- forEachData

static void test_foreach_visits_only_occupied(void) {
    HashTable *t = htInit(HASH_T_SIZE, 1);
    REQUIRE(t != NULL);
    int a = 1, b = 2, c = 3;
    htInsert(t, "a", &a);
    htInsert(t, "b", &b);
    htInsert(t, "c", &c);
    htDelete(t, "b");

    g_visited = 0;
    forEachData(t, countCallback);
    CHECK(g_visited == 2);
    CHECK(seenCount(&a) == 1);
    CHECK(seenCount(&c) == 1);
    CHECK(seenCount(&b) == 0);
    htFree(t);
}

static void test_foreach_empty_table(void) {
    HashTable *t = htInit(HASH_T_SIZE, 1);
    REQUIRE(t != NULL);
    g_visited = 0;
    forEachData(t, countCallback);
    CHECK(g_visited == 0);
    htFree(t);
}

static void test_foreach_uses_tables_own_size(void) {
    // Regression: must not assume HASH_T_SIZE.
    HashTable *t = htInit(3, 1);
    REQUIRE(t != NULL);
    int a = 1;
    htInsert(t, "a", &a);
    g_visited = 0;
    forEachData(t, countCallback);
    CHECK(g_visited == 1);
    htFree(t);
}

static void helper_foreach_null_callback(void) {
    HashTable *t = htInit(HASH_T_SIZE, 1);
    forEachData(t, NULL);
    htFree(t);
}

static void test_foreach_null_arguments(void) {
    EXPECT_SURVIVES(forEachData(NULL, countCallback));
    EXPECT_SURVIVES(helper_foreach_null_callback());
}

// --------------------------------------------------------------------- main

int main(int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "--no-death") == 0) {
        g_skip_death = 1;
    }
    initKeys();

    RUN(test_hash_known_values);
    RUN(test_hash_in_range);
    RUN(test_hash_deterministic);
    RUN(test_hash_null_or_zero_size);

    RUN(test_init_zero_size);
    RUN(test_init_creates_empty_table);
    RUN(test_scope_id_and_rehash_preserves_it);
    RUN(test_free_null);

    RUN(test_insert_then_search);
    RUN(test_search_missing);
    RUN(test_insert_existing_updates);
    RUN(test_insert_null_data_allowed);
    RUN(test_null_arguments);
    RUN(test_keys_compared_by_content);

    RUN(test_delete_marks_slot);
    RUN(test_delete_missing);
    RUN(test_delete_twice);

    RUN(test_colliding_keys_all_found);
    RUN(test_search_probes_past_tombstone);
    RUN(test_insert_reuses_first_deleted_slot);
    RUN(test_no_duplicate_after_delete_and_reinsert);

    RUN(test_rehash_on_high_load);
    RUN(test_full_table_with_tombstones_reuses_deleted);
    RUN(test_size_one_table);

    RUN(test_many_items);

    RUN(test_foreach_visits_only_occupied);
    RUN(test_foreach_empty_table);
    RUN(test_foreach_uses_tables_own_size);
    RUN(test_foreach_null_arguments);

    printf("\n%d checks, %d failed\n", g_checks, g_failed);
    return g_failed == 0 ? 0 : 1;
}