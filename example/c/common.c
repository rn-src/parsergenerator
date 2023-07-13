#include "common.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define INITIAL_INTVEC_CAPACITY (8)
#define COMPRESSMARKER (0)

void *mrealloc(void *prev, size_t prevsize, size_t newsize) {
  if( ! newsize ) {
    if( prev )
      free(prev);
    return 0;
  }
  if( prev ) {
    void *vpnew = realloc(prev,newsize);
    if( ! vpnew )
      return prev;
    if( newsize > prevsize )
      memset((char*)vpnew+prevsize,0,newsize-prevsize);
    return vpnew;
  }
  void *vpnew = malloc(newsize);
  if( vpnew )
    memset(vpnew,0,newsize);
  return vpnew;
}

void vecint_init(vecint *v) {
  v->values = 0;
  v->size = 0;
  v->capacity = 0;
}

void vecint_destroy(vecint *v) {
  if( v->values )
    mrealloc(v->values,0,0);
}

void vecint_push(vecint *v, int i) {
  if( v->size == v->capacity ) {
    size_t newcapacity = v->capacity?v->capacity*2:INITIAL_INTVEC_CAPACITY;
    v->values = (int*)mrealloc(v->values,v->capacity*sizeof(int),newcapacity*sizeof(int));
    v->capacity = newcapacity;
  }
  v->values[v->size++] = i;
}

int vecint_back(vecint *v) {
  return v->values[v->size-1];
}

void charbuf_init(charbuf *buf, size_t initial_capacity) {
  buf->buf = (char*)mrealloc(0,0,initial_capacity);
  buf->size = 0;
  buf->capacity = initial_capacity;
}

void charbuf_clear(charbuf *buf) {
  buf->size = 0;
}

void charbuf_ensure_additional_capacity(charbuf *buf, size_t additional) {
  size_t mincapacity = buf->size+additional;
  if (mincapacity <= buf->capacity)
    return;
  size_t newcapacity = buf->capacity?buf->capacity:1;
  while( newcapacity < mincapacity ) newcapacity*=2;
  buf->buf = (char*)mrealloc(buf->buf,buf->capacity,newcapacity);
  buf->capacity = newcapacity;
}

size_t charbuf_putc_utf8(charbuf *buf, int c) {
  charbuf_ensure_additional_capacity(buf, 4);
  char *dst = buf->buf+buf->size;
  if( c < 0 || c >= 0x0010ffff)
    c = REPLACEMENT_CHARACTER;
  if (c >= 0 && c <= 0x7f) {
    *dst++ = c;
  } else {
    if (c >= 0x0080 && c <= 0x07ff) {
      *dst++ = 0xc0 | ((c >> 6) & 0x1f);
    } else {
      if (c >= 0x0800 && c <= 0xffff) {
        *dst++ = ((c >> 12) & 0x0f);
      }
      else {
        *dst++ = 0xf8 | ((c >> 18) & 0x07);
        *dst++ = 0x80 | ((c >> 12) & 0x3f);
      }
      *dst++ = 0x80 | ((c >> 6) & 0x3f);
    }
    *dst++ = 0x80 | (c & 0x3f);
  }
  size_t n = dst-buf->buf;
  buf->size += n;
  return n;
}

void charbuf_destroy(charbuf *buf) {
  if( buf->buf )
    mrealloc(buf->buf,0,0);
}

static unsigned int decodeuint(const unsigned char *c, const unsigned char **pnextc) {
  int leadbits = 0;
  while( leadbits <= 8 && (0x80>>leadbits)&*c ) ++leadbits;
  if( leadbits == 0 ) {
    *pnextc = c+1;
    return *c;
  }
  unsigned int i = (0xff<<(8-leadbits)) & *c++;
  while( leadbits-- ) {
    i <<= 8;
    i |= *c++;
  }
  *pnextc = c;
  return i;
}

bool intiter_init(intiter *ii, int format, const unsigned char *values, const unsigned short *offsets, int decodedelta) {
  if( ii->format == SECTIONAL_COMPRESSION )
    vecint_init(&ii->stack);
  if( format != ENCODED_8BIT && format != SECTIONAL_COMPRESSION ) {
    fprintf(stderr, "Encoding format %d is not supported\n", format);
    return false;
  }
  ii->format = format;
  ii->values = values;
  ii->offsets = offsets;
  ii->pos = ii->values;
  ii->n = 0;
  ii->startindex = 0;
  ii->offset = 0;
  ii->decodedelta = decodedelta;
  return true;
}

void intiter_seek(intiter *ii, int startindex, int offset) {
  if( ii->format == SECTIONAL_COMPRESSION )
    ii->stack.size = 0;
  ii->pos = ii->values+ii->offsets[startindex];
  ii->n = ii->offsets[startindex+1]-ii->offsets[startindex];
  if( offset > ii->n )
    offset = ii->n;
  while( offset > 0 ) {
    --offset;
    intiter_next(ii);
  }
}

void intiter_destroy(intiter *ii) {
  if( ii->format == SECTIONAL_COMPRESSION )
    vecint_destroy(&ii->stack);
}

int intiter_next(intiter *ii) {
  if( ii->n == 0 )
    return -1;
  if( ii->format == ENCODED_8BIT ) {
    --ii->n;
    ++ii->offset;
    return decodeuint(ii->pos,&ii->pos)-ii->decodedelta;
  }
  if( ii->format == SECTIONAL_COMPRESSION ) {
    int i = decodeuint(ii->pos,&ii->pos);
    while( i == COMPRESSMARKER ) {
      int off = decodeuint(ii->pos,&ii->pos);
      int n = decodeuint(ii->pos,&ii->pos);
      vecint_push(&ii->stack,ii->pos-ii->values);
      vecint_push(&ii->stack,ii->n-n);
      ii->pos = ii->values+off;
      ii->n = n;
      i = decodeuint(ii->pos,&ii->pos);
    }
    ++ii->offset;
    --ii->n;
    while( ii->n == 0 && ii->stack.size > 0 ) {
     int n = vecint_back(&ii->stack);
     ii->stack.size -= 1;
     int pos = vecint_back(&ii->stack);
     ii->stack.size -= 1;
     ii->pos = ii->values+pos;
    }
    return i-ii->decodedelta;
  }
  return -1;
}

bool intiter_end(intiter *ii) {
  return ii->n == 0;
}
