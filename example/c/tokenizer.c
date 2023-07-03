#include "tokenizer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#define INITIAL_INTVEC_CAPACITY (8)
#define INITIAL_TOKSTR_CAPACITY (8)
#define BUF_BLOCK_SIZE (1<<12)
#define REPLACEMENT_CHARACTER (0xfffd)

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
    free(v->values);
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

static void charbuf_ensure_additional_capacity(charbuf *buf, size_t additional) {
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
  if (c >= 0 && c <= 0x0077) {
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
    free(buf->buf);
}

void tokenizer_init(tokenizer *tokenizer, tokinfo *info, reader *rdr) {
  memset(tokenizer,0,sizeof(struct tokenizer));
  tokenizer->info = info;
  tokenizer->tok = -1;
  tokenizer->pos.filename = rdr->name;
  tokenizer->pos.line = 1;
  tokenizer->pos.col = 1;
  tokenizer->eof = false;
  charbuf_init(&tokenizer->tokstrbuf,INITIAL_TOKSTR_CAPACITY);
  charbuf_init(&tokenizer->readbuf,0);
  vecint_init(&tokenizer->secstack);
  vecint_push(&tokenizer->secstack,0);
  tokenizer->rdr = rdr;
}

void tokenizer_destroy(tokenizer *tokenizer) {
  charbuf_destroy(&tokenizer->tokstrbuf);
  charbuf_destroy(&tokenizer->readbuf);
  vecint_destroy(&tokenizer->secstack);
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

static int tokstate(tokinfo *info, int state) {
  const unsigned char *cur = info->stateinfo+info->stateinfo_offset[state];
  const unsigned char *end = info->stateinfo+info->stateinfo_offset[state+1];
  if( cur < end )
    return decodeuint(cur,&cur)-1;
  return -1;
}

static int nextstate(tokinfo *info, int state, int c) {
  const unsigned char *cur = info->stateinfo+info->stateinfo_offset[state];
  const unsigned char *end = info->stateinfo+info->stateinfo_offset[state+1];
  if( cur < end )
    decodeuint(cur,&cur); // tok
  while( cur != end ) {
    int to = decodeuint(cur,&cur);
    int cnt = decodeuint(cur,&cur);
    int last = 0;
    for( int i = 0; i < cnt; ++i ) {
      int low = decodeuint(cur,&cur)+last;
      last = low;
      int high = decodeuint(cur,&cur)+last;
      last = high;
      if( c >=low && c <= high )
        return to;
    }
  }
  return -1;
}

// count of how many in the circular buffer
static int tok_nreadable(tokenizer* tokenizer, size_t offset) {
  size_t nreadable = 0;
  if(tokenizer->bufoffset <= tokenizer->bufend)
    nreadable = tokenizer->bufend - tokenizer->bufoffset;
  else
    nreadable = tokenizer->readbuf.size - tokenizer->bufoffset + tokenizer->bufend;
  if( nreadable < offset )
    return 0;
  return nreadable - offset;
}

static int peekc(tokenizer *tokenizer, size_t offset, size_t *bytes) {
  int nreadable = tok_nreadable(tokenizer,offset);
  // make sure we can see ahead 4, the size of the largest utf-8 encoding
  if( ! tokenizer->eof && nreadable < offset+4 ) {
    if( tokenizer->readbuf.size - nreadable < BUF_BLOCK_SIZE ) {
      // expand the circular buffer
      charbuf_ensure_additional_capacity(&tokenizer->readbuf, BUF_BLOCK_SIZE);
      if( tokenizer->bufoffset > tokenizer->bufend ) {
        // this is the case where we have to make room in the middle
        if( tokenizer->readbuf.size > tokenizer->bufoffset )
          memmove(tokenizer->readbuf.buf+tokenizer->readbuf.size, tokenizer->readbuf.buf+tokenizer->bufoffset, tokenizer->readbuf.size-tokenizer->bufoffset);
        tokenizer->bufoffset += BUF_BLOCK_SIZE;
      }
      tokenizer->readbuf.size = tokenizer->readbuf.capacity;
    }
    // we don't have to worry about wrap-around here, because we always read this many (or less for eof) at a time.
    size_t nread = tokenizer->rdr->read(tokenizer->rdr, BUF_BLOCK_SIZE, tokenizer->readbuf.buf+tokenizer->bufend%tokenizer->readbuf.size);
    if( nread < BUF_BLOCK_SIZE )
      tokenizer->eof = true;
    if( nread > 0 )
      tokenizer->bufend = (tokenizer->bufend+nread)%tokenizer->readbuf.size;
    nreadable += nread;
  }
  if(nreadable == 0)
    return -1;
  char *buf = tokenizer->readbuf.buf + tokenizer->bufoffset + offset;
  // utf-8 decoding
  if ((buf[0] & 0x80) == 0) {
    *bytes = 1;
    return buf[0];
  }
  if((buf[0] & 0xe0) == 0xc0) {
    if(nreadable < 2 ) goto errn;
    if( (buf[1]&0xc0) != 0x80) goto err1;
    *bytes = 2;
    return ((buf[0] & 0x1f) << 6) | (buf[1] & 0x3f);
  }
  if ((buf[0] & 0xf0) == 0xe0) {
    if (nreadable < 3) goto errn;
    if ((buf[1] & 0xc0) != 0x80 || (buf[2] & 0xc0) != 0x80) goto err1;
    *bytes= 3;
    return ((buf[0] & 0x1f) << 12) | ((buf[1] & 0x3f) << 6) | (buf[2] & 0x3f);
  }
  if((buf[0] & 0xf8) == 0xf0) {
    if (nreadable < 4) goto errn;
    if((buf[1] & 0xc0) != 0x80 || (buf[2] & 0xc0) != 0x80 || (buf[3] & 0xc0) != 0x80) goto err1;
    *bytes = 4;
    return ((buf[0] & 0x1f) << 18) | ((buf[1] & 0x3f) << 12) | ((buf[2] & 0x3f) << 6) | (buf[3] & 0x3f);
  }
err1:
  *bytes = 1;
  return REPLACEMENT_CHARACTER;
errn: // not enough bytes to make full character
  *bytes = nreadable;
  return REPLACEMENT_CHARACTER;
}

static void chomp(tokenizer* tokenizer, size_t n) {
  tokenizer->bufoffset = (tokenizer->bufoffset + n) % tokenizer->readbuf.size;
}

int tokenizer_peek(tokenizer *tokenizer) {
  if( tokenizer->tok != -1 )
    return tokenizer->tok;
  bool getanothertok = true;
  while( getanothertok ) {
    getanothertok = false;
    int cursection = vecint_back(&tokenizer->secstack);
    int tok = -1;
    int t = -1;
    int state = nextstate(tokenizer->info,0,cursection);
    size_t offset = 0, used = 0;
    toksize cursize = {0,0,0,0};
    toksize toksize = {0,0,0,0};
    while( state != -1 ) {
      t = tokstate(tokenizer->info,state);
      if( t != -1 ) {
        tok = t;
        toksize = cursize;
      }
      int c = peekc(tokenizer, offset, &used);
      if( c == -1 )
        break;
      offset += used;
      ++cursize.len;
      if( c == '\n' ) {
        cursize.lines += 1;
        cursize.cols = 0;
      } else
        ++cursize.cols;
      state = nextstate(tokenizer->info, state, c);
    }
    if( tok == -1 ) {
      tokenizer->tok = -1;
      break;
    }
    offset = 0;
    charbuf_clear(&tokenizer->tokstrbuf);
    for (size_t n = 0; n < toksize.len; ++n) {
      int c = peekc(tokenizer, offset, &used);
      offset += used;
      toksize.bytes += charbuf_putc_utf8(&tokenizer->tokstrbuf,c);
    }
    chomp(tokenizer,offset);
    charbuf_putc_utf8(&tokenizer->tokstrbuf, 0);
    if( tokenizer->info->sectioninfo ) {
      const unsigned char *startsection = tokenizer->info->sectioninfo+tokenizer->info->sectioninfo_offset[cursection];
      const unsigned char *endsection = tokenizer->info->sectioninfo+tokenizer->info->sectioninfo_offset[cursection+1];
      while( startsection < endsection ) {
        int actiontok = decodeuint(startsection,&startsection);
        int action = decodeuint(startsection,&startsection);
        int actionarg = decodeuint(startsection,&startsection);
        if( actiontok == tok ) {
          if( action == 1 )
            vecint_push(&tokenizer->secstack,actionarg);
          else if( action == 2 )
            tokenizer->secstack.size--;
          else if( action == 3 )
            tokenizer->secstack.values[tokenizer->secstack.size-1] = actionarg;
          break;
        }
      }
    }
    if( tokenizer->info->isws[tok/8]&(1<<(tok%8)) ) {
      getanothertok = true;
      continue;
    }
    tokenizer->size = toksize;
    tokenizer->tok = tok;
    tokenizer->tokstr = tokenizer->tokstrbuf.buf;
  }
  return tokenizer->tok;
}

void tokenizer_discard(tokenizer *tokenizer) {
  if( tokenizer->tok == -1 )
    return;
  if( tokenizer->size.lines > 0 ) {
    tokenizer->pos.line += tokenizer->size.lines;
    tokenizer->pos.col = tokenizer->size.cols;
  } else {
    tokenizer->pos.col += tokenizer->size.cols;
  }
  tokenizer->tok = -1;
}

const char *tokenizer_tokid2str(tokenizer *tokenizer, int tokid) {
  if( tokid < 0 || tokid >= tokenizer->info->tokenCount )
    return "?";
  return tokenizer->info->tokenstr[tokid];
}
