#ifndef __tokenizer_h
#define __tokenizer_h
#include <stdbool.h>
#include <stddef.h>

struct tokinfo {
  int tokenCount;
  int sectionCount;
  const int *tokenaction;
  const char **tokenstr;
  const bool *isws;
  int stateCount;
  const int *transitions;
  const int *transitionOffset;
  const int *tokens;
};
typedef struct tokinfo tokinfo;

struct tokpos {
  const char *filename;
  int line;
  int col;
};
typedef struct tokpos tokpos;

struct toksize {
  int lines;
  int cols;
  int len;
  int bytes;
};
typedef struct toksize toksize;

struct Token {
  int tok;
  const char *s;
  tokpos pos;
  toksize size;
};
typedef struct Token Token;

struct vecint {
  int *values;
  size_t size;
  size_t capacity;
};
typedef struct vecint vecint;

void vecint_init(vecint *v);
void vecint_destroy(vecint *v);
void vecint_push(vecint *v, int i);
int vecint_back(vecint *v);

struct charbuf {
  char *buf;
  size_t size;
  size_t capacity;
};
typedef struct charbuf charbuf;

void charbuf_init(charbuf *buf, size_t initial_capacity);
void charbuf_clear(charbuf *buf);
void charbuf_set_capacity(charbuf *buf, size_t new_capacity);
int charbuf_putc_utf8(charbuf *buf, int c);
void charbuf_destroy(charbuf *buf);

struct reader;
typedef size_t (*readfnct)(struct reader *r, size_t n, void *buf);
struct reader {
  readfnct read;
};
typedef struct reader reader;

struct tokenizer {
  tokinfo *info;
  int tok;
  char *tokstr;
  charbuf tokstrbuf;
  reader *rdr;
  charbuf readbuf;
  bool eof;
  size_t readoffset;
  tokpos pos;
  toksize size;
  vecint secstack;
};
typedef struct tokenizer tokenizer;

void tokenizer_init(tokenizer *tokenizer, tokinfo *info);
void tokenizer_destroy(tokenizer *tokenizer);
int tokenizer_peek(tokenizer *tokenizer);
void tokenizer_discard(tokenizer *tokenizer);
const char *tokenizer_tokid2str(tokenizer *tokenizer, int tokid);
const char *tokenizer_tokstr(tokenizer *tokenizer, int p);

#endif /* __tokenizer_h */
