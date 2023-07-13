#ifndef __tokenizer_h
#define __tokenizer_h
#include <stdbool.h>
#include <stddef.h>

void *mrealloc(void *prev, size_t prevsize, size_t newsize);

struct tokinfo {
  int tokenCount;
  int sectionCount;
  const unsigned char *sectioninfo;
  const unsigned short *sectioninfo_offset;
  const char **tokenstr;
  const unsigned char *isws;
  int stateCount;
  const unsigned char stateinfo_format;
  const unsigned char *stateinfo;
  const unsigned short *stateinfo_offset;
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
size_t charbuf_putc_utf8(charbuf *buf, int c);
void charbuf_destroy(charbuf *buf);

struct reader;
typedef size_t (*readfnct)(struct reader *r, size_t n, void *buf);
struct reader {
  const char *name;
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
  size_t bufoffset, bufend;
  tokpos pos;
  toksize size;
  vecint secstack;
};
typedef struct tokenizer tokenizer;

void tokenizer_init(tokenizer *tokenizer, tokinfo *info, reader *rdr);
void tokenizer_destroy(tokenizer *tokenizer);
int tokenizer_peek(tokenizer *tokenizer);
void tokenizer_discard(tokenizer *tokenizer);
const char *tokenizer_tokid2str(tokenizer *tokenizer, int tokid);

#endif /* __tokenizer_h */
