#ifndef __tests_h
#define __tests_h
#ifndef __cplusplus
#include <stdbool.h>
#endif
#ifdef __cplusplus
extern "C" {
#endif

typedef bool (*TestCb)(void *vpcb);

int addTest(const char *category, const char *name, TestCb test, void *vpcb);
bool runTests(const char *categories, const char *name);
void listTests(const char *categories, const char *name);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
