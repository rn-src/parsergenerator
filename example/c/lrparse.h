#ifndef __lrparse_h
#define __lrparse_h
#include "tokenizer.h"

#define ACTION_SHIFT (0)
#define ACTION_REDUCE (1)
#define ACTION_STOP (2)

typedef bool (*reducefnct)(void *extra, int productionidx, void *inputs, void *ntout, const char **err);

struct parseinfo {
  const int nstates;
  const int *actions;
  const int *actionstart;
  const int prod0;
  const int parse_error;
  const int nproductions;
  const int *productions;
  const int *productionstart;
  const int start;
  const char **nonterminals;
  const size_t itemsize;
  reducefnct reduce;
};
typedef struct parseinfo parseinfo;

struct writer;
typedef void (*writerprint)(struct writer *w, const char *s, ...);
typedef void (*writerputs)(struct writer *w, const char *s);
struct writer {
  writerprint printf;
  writerputs  puts;
};
typedef struct writer writer;

Token *parse(tokenizer *tokenizer, parseinfo *parseinfo, void *extra, int verbosity, writer *out);
writer *stderr_writer();

#endif /* __lrparse_h */

