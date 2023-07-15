#include "common.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define INITIAL_INTVEC_CAPACITY (8)

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

static int decodeuintsize(const unsigned char *c) {
  int leadbits = 0;
  while( leadbits <= 8 && (0x80>>leadbits)&*c ) ++leadbits;
  return leadbits+1;
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

bool intiter_init(intiter *ii, int format, const unsigned char *values, const unsigned short *offsets, int offsets_count, const unsigned short *indexes, int decodedelta) {
  vecint_init(&ii->stack);
  if( format != ENC_8BIT && format != CMP_ENC_8BIT ) {
    fprintf(stderr, "Encoding format %d is not supported\n", format);
    return false;
  }
  ii->format = format;
  ii->values = values;
  ii->offsets = offsets;
  ii->offsets_count = offsets_count;
  ii->indexes = indexes;
  ii->pos = ii->endpos = ii->values;
  ii->n = 0;
  ii->startindex = 0;
  ii->offset = 0;
  ii->decodedelta = decodedelta;
  return true;
}

static int intiter_findblk(intiter *ii, size_t pos) {
  int low = 0, high = ii->offsets_count-1;
  while( low < high ) {
    int mid = low+high/2;
    if( pos < ii->offsets[mid] ) {
      high = mid;
    } else if( pos >= ii->offsets[mid+1] ) {
      low = mid+1;
    } else {
      return mid;
    }
  }
  return low;
}

void intiter_skip(intiter *ii, size_t count) {
  if( ii->format == ENC_8BIT ) {
    size_t i = 0;
    const unsigned char *pos = ii->pos, *endpos = ii->endpos;
    while( i < count && pos < endpos ) {
      ++i;
      pos += decodeuintsize(pos);
    }
    ii->pos = pos;
    ii->offset += i;
    ii->n -= i;
    ii->pos = pos;
  } else {
    while( count > 0 && ii->n > 0 ) {
      // advance compressed block at a time, until we either
      // arrive at the seek destionion or hit a partial block
      while( count > 0 && ii->n > 0 && *ii->pos == CMP_MARKER) {
        int pos = decodeuint(ii->pos+1,&ii->pos);
        int n = decodeuint(ii->pos,&ii->pos);
        if( count >= n ) {
          // the place we want to seek to is afer this block,
          // so skip without reading, pushing, popping
          count -= n;
          ii->n -= n;
          ii->offset += n;
        } else {
          // the place we want to seek to is within this block
          // we'll have to push as if we read it.
          vecint_push(&ii->stack,ii->pos-ii->values);
          vecint_push(&ii->stack,ii->n-n);
          // find the block for the position, and the compressed position
          // then adjust the seek to compensate for any extra
          int startblk = intiter_findblk(ii,pos);
          while( ii->indexes[startblk] == CMP_OFF_UNDEF )
            --startblk;
          int extra = pos-ii->offsets[startblk];
          if( extra ) {
            n += extra;
            count += extra;
          }
          ii ->n = n;
          ii->pos = ii->values+ii->indexes[startblk];
        }
      }

      // advance a character at a time until we arrive at the seek
      // destination or hit another compressed block
      size_t i = 0;
      const unsigned char *pos = ii->pos;
      while( i < count && ii->n > 0 && *pos != CMP_MARKER ) {
        ++i;
        pos += decodeuintsize(pos);
      }
      ii->pos = pos;
      ii->offset += i;
      ii->n -= i;
      count -= i;

      // if we exhausted the current block, pop the stack
      while( ii->stack.size > 0 && ii->n == 0 ) {
        int n = vecint_back(&ii->stack);
        --ii->stack.size;
        int pos = vecint_back(&ii->stack);
        --ii->stack.size;
        ii->n = n;
        ii->pos = ii->values+pos;
      }
    }
  }
}

void intiter_seek(intiter *ii, size_t startindex, size_t offset) {
  if( ii->format == ENC_8BIT ) {
    ii->pos = ii->values+ii->offsets[startindex];
    ii->endpos = ii->values+ii->offsets[startindex+1]; // exact
    ii->n = ii->offsets[startindex+1]-ii->offsets[startindex]; // upper bound
  } else if( ii->format == CMP_ENC_8BIT ) {
    size_t pos = ii->offsets[startindex];
    ii->stack.size = 0;
    unsigned short extraskip = 0;
    int startblk = startindex;
    int endblk = startindex+1;
    while( ii->indexes[startblk] == CMP_OFF_UNDEF )
      --startblk;
    while( ii->indexes[endblk] == CMP_OFF_UNDEF )
      ++endblk;
    ii->pos = ii->values+ii->indexes[startblk];
    ii->offset = ii->offsets[startblk];
    ii->endpos = ii->values+ii->indexes[endblk]; // upper bound
    ii->n = ii->offsets[startindex+1]-ii->offsets[startblk]; // exact
    if( startblk < startindex )
      offset = offset + ii->offsets[startindex] - ii->offsets[startblk];
  }
  if( offset > 0 )
    intiter_skip(ii,offset);
}

void intiter_destroy(intiter *ii) {
  vecint_destroy(&ii->stack);
}

int intiter_next(intiter *ii) {
  if( ii->pos == ii->endpos || ii->n == 0 )
    return -1;
  if( ii->format == ENC_8BIT ) {
    --ii->n;
    ++ii->offset;
    return decodeuint(ii->pos,&ii->pos)-ii->decodedelta;
  }
  if( ii->format == CMP_ENC_8BIT ) {
    // if we encounter a compressed block, we must seek within it
    while( ii->n > 0 && *ii->pos == CMP_MARKER) {
      int pos = decodeuint(ii->pos+1,&ii->pos);
      int n = decodeuint(ii->pos,&ii->pos);
      vecint_push(&ii->stack,ii->pos-ii->values);
      vecint_push(&ii->stack,ii->n-n);
      // find the block for the position, and the compressed position
      // then seek to compensate for any extra
      int startblk = intiter_findblk(ii,pos);
      while( ii->indexes[startblk] == CMP_OFF_UNDEF )
        --startblk;
      int extra = pos-ii->offsets[startblk];
      if( extra )
        n += extra;
      ii ->n = n;
      ii->pos = ii->values+ii->indexes[startblk];
      if( extra )
        intiter_skip(ii,extra);
    }
    // arrived, get the character
    int i = decodeuint(ii->pos,&ii->pos);
    ++ii->offset;
    --ii->n;
    // if we exhausted the data, pop
    while( ii->stack.size > 0 && ii->n == 0 ) {
      int n = vecint_back(&ii->stack);
      --ii->stack.size;
      int pos = vecint_back(&ii->stack);
      --ii->stack.size;
      ii->n = n;
      ii->pos = ii->values+pos;
    }
    return i-ii->decodedelta;
  }
  return -1;
}

bool intiter_end(intiter *ii) {
  // sometimes endpos is correct and n is too big, other times n is correct and endpos is too far
  return ii->pos == ii->endpos || ii->n == 0;
}
