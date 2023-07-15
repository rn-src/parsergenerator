#ifndef __lrparse_h
#define __lrparse_h
#include "tokenizer.h"

#define ACTION_SHIFT (0)
#define ACTION_REDUCE (1)
#define ACTION_STOP (2)

typedef bool (*reducefnct)(void *extra, int productionidx, void *inputs, void *ntout, const char **err);

struct parseinfo {
  const int nstates;
  const unsigned char actions_format;
  const unsigned char *actions;
  const unsigned short *actionstart;
  const unsigned short *actionstartindex;
  const int actionstateindex_count;
  const int prod0;
  const int parse_error;
  const int nproductions;
  const unsigned char *productions;
  const unsigned short *productionstart;
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

