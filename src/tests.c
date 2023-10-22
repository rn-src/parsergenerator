// trivial test driver
#include "tests.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct TestLink;
typedef struct TestLink TestLink;
struct TestLink {
  const char *category;
  const char *name;
  TestCb test;
  void *vpcb;
  TestLink *next;
};

static TestLink *g_test_head = 0, *g_test_tail = 0;

int addTest(const char *category, const char *name, TestCb test, void *vpcb) {
  TestLink *link = (TestLink*)malloc(sizeof(TestLink));
  link->category = category;
  link->name = name;
  link->test = test;
  link->vpcb = vpcb;
  link->next = 0;
  if( ! g_test_head )
    g_test_head = link;
  if( g_test_tail )
    g_test_tail->next = link;
  g_test_tail = link;
  return 0;
}

static bool useTest(const char *categories, const char *name, TestLink *link) {
  if( categories && name==0 )
    return strcmp(categories,link->category) == 0;
  else if( categories && name )
    return strcmp(categories,link->category) == 0 && strcmp(name,link->name) == 0;
  else if( categories==0 && name )
    return strcmp(name,link->name) == 0;
  return true;
}

void listTests(const char *categories, const char *name) {
  TestLink *cur = g_test_head;
  fprintf(stdout, "tests %s::%s\n", categories ? categories : "*", name ? name : "*");
  while( cur ) {
    if( useTest(categories,name,cur) )
      fprintf(stdout, "%s::%s\n", cur->category, cur->name);
    cur = cur->next;
  }
}

bool runTests(const char *categories, const char *name) {
  TestLink *cur = g_test_head;
  int nrun = 0;
  int npass = 0;
  int nfail = 0;
  fprintf(stdout, "Running tests %s::%s\n", categories ? categories : "*", name ? name : "*");
  while( cur ) {
    if( useTest(categories,name,cur) ) {
      fprintf(stdout, "%s::%s ", cur->category, cur->name);
      ++nrun;
      if( cur->test(cur->vpcb) ) {
        fputs(" PASS\n", stdout);
        ++npass;
      } else {
        fputs(" FAIL\n", stdout);
        ++nfail;
      }
    }
    cur = cur->next;
  }
  fprintf(stdout, "Completed tests\n%d ran\n%d pass\n%d fail\n", nrun, npass, nfail);
  return nfail == 0;
}

static bool hasArg(int argc, char *argv[], const char *arg) {
  for( int i = 0; i < argc; ++i )
    if( strcmp(argv[i],arg) == 0 )
      return true;
  return false;
}

static const char *getArg(int argc, char *argv[], const char *arg) {
  size_t len = strlen(arg);
  for( int i = 0; i < argc; ++i )
    if( strncmp(argv[i],arg,len) == 0 ) {
      if( argv[i][len] == '=' )
        return argv[i]+len+1;
      else if( argv[i][len] == 0 && i+1 < argc )
        return argv[i+1];
    }
  return 0;
}

int main(int argc, char *argv[]) {
  bool list = hasArg(argc,argv,"--list");
  const char *categories = getArg(argc,argv,"--category");
  const char *name = getArg(argc,argv,"--name");
  if( list )
    listTests(categories,name);
  else {
    if( ! runTests(categories,name) )
      return 1;
  }
  return 0;
}

