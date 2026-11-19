/* AI-generated (Claude)
 * Minimal test helpers shared by unit_hashtable.c and unit_symtable.c.
 * Must be the FIRST include in a test file (it defines _POSIX_C_SOURCE). */
#ifndef UNIT_UTILS_H
#define UNIT_UTILS_H

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static int g_checks = 0;
static int g_failed = 0;
static int g_skip_death = 0;  // set by --no-death

/* Records a failure and continues. */
#define CHECK(cond)                                                    \
    do {                                                               \
        g_checks++;                                                    \
        if (!(cond)) {                                                 \
            printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);   \
            g_failed++;                                                \
        }                                                              \
    } while (0)

/* Records a failure and leaves the current (void) test function. */
#define REQUIRE(cond)                                                  \
    do {                                                               \
        g_checks++;                                                    \
        if (!(cond)) {                                                 \
            printf("  FAIL %s:%d: %s (test aborted)\n", __FILE__,      \
                   __LINE__, #cond);                                   \
            g_failed++;                                                \
            return;                                                    \
        }                                                              \
    } while (0)

#define CHECK_STR(actual, expected)                                    \
    do {                                                               \
        g_checks++;                                                    \
        const char *a_ = (actual);                                     \
        if (a_ == NULL || strcmp(a_, (expected)) != 0) {               \
            printf("  FAIL %s:%d: \"%s\" != \"%s\"\n", __FILE__,       \
                   __LINE__, a_ ? a_ : "(null)", (expected));          \
            g_failed++;                                                \
        }                                                              \
    } while (0)

#define RUN(fn)                \
    do {                       \
        printf("%s\n", #fn);   \
        fn();                  \
    } while (0)

static int childStatusCode(int status) {
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    if (WIFSIGNALED(status)) {
        return 1000 + WTERMSIG(status);  // crashed, e.g. 1011 = SIGSEGV
    }
    return -1;
}

/* Runs stmt in a forked child and compares the child's exit code.
 * Used for code that is supposed to call exit(N) and for "must not crash". */
#define RUN_IN_CHILD(stmt, expected)                                   \
    do {                                                               \
        int status_ = 0;                                               \
        g_checks++;                                                    \
        fflush(NULL);                                                  \
        pid_t pid_ = fork();                                           \
        if (pid_ == 0) {                                               \
            if (freopen("/dev/null", "w", stderr) == NULL) {           \
                _exit(97);                                             \
            }                                                          \
            (void)(stmt);                                              \
            _exit(0);                                                  \
        }                                                              \
        waitpid(pid_, &status_, 0);                                    \
        if (childStatusCode(status_) != (expected)) {                  \
            printf("  FAIL %s:%d: child exit code %d, expected %d\n",  \
                   __FILE__, __LINE__, childStatusCode(status_),       \
                   (expected));                                        \
            g_failed++;                                                \
        }                                                              \
    } while (0)

/* stmt must terminate the process with exit(code). Skipped with --no-death. */
#define EXPECT_EXIT_CODE(stmt, code)       \
    do {                                   \
        if (!g_skip_death) {               \
            RUN_IN_CHILD(stmt, code);      \
        }                                  \
    } while (0)

/* stmt must finish normally (no crash, no exit with an error code). */
#define EXPECT_SURVIVES(stmt) RUN_IN_CHILD(stmt, 0)

#endif
