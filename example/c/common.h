#ifndef __common_h
#define __common_h
#include <stdbool.h>

#define ENC_8BIT (2)
#define CMP_ENC_8BIT (3)
#define REPLACEMENT_CHARACTER (0xfffd)
#define DECODEDELTA (2)
#define CMP_MARKER (0)
#define CMP_OFF_UNDEF (0xffff)

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
void charbuf_ensure_additional_capacity(charbuf *buf, size_t additional);

struct intiter;
typedef struct intiter intiter;

struct intiter {
  vecint stack;
  int format;
  const unsigned char *values;
  const unsigned short *offsets;
  int offsets_count;
  const unsigned short *indexes;
  const unsigned char *pos, *endpos;
  int n, startindex, offset;
  int decodedelta;
};

bool intiter_init(intiter *ii, int format, const unsigned char *values, const unsigned short *offsets, int offsets_count, const unsigned short *indexes, int decodedelta);
void intiter_seek(intiter *ii, size_t startindex, size_t offset);
void intiter_destroy(intiter *ii);
int intiter_next(intiter *ii);
bool intiter_end(intiter *ii);

void *mrealloc(void *prev, size_t prevsize, size_t newsize);

#endif // __common_h
