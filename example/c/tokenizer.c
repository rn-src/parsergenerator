#include "tokenizer.h"
#include <stdlib.h>
#include <string.h>
#define INITIAL_INTVEC_CAPACITY (8)
#define INITIAL_TOKSTR_CAPACITY (8)
#define BUF_BLOCK_SIZE (1<<12)

void vecint_init(vecint *v) {
  v->values = (int*)malloc(sizeof(int)*INITIAL_INTVEC_CAPACITY);
  v->size = 0;
  v->capacity = INITIAL_INTVEC_CAPACITY;
}

void vecint_destroy(vecint *v) {
  if( v->values )
    free(v->values);
}

void vecint_push(vecint *v, int i) {
  if( v->size == v->capacity ) {
    v->values = (int*)realloc(v->values,v->size*sizeof(int)*2);
    v->capacity *= 2;
  }
  v->values[v->size++] = i;
}

int vecint_back(vecint *v) {
  return v->values[v->size-1];
}

void charbuf_init(charbuf *buf, size_t initial_capacity) {
  buf->buf = (char*)malloc(initial_capacity);
  buf->size = 0;
  buf->capacity = initial_capacity;
}

void charbuf_clear(charbuf *buf) {
  buf->size = 0;
}

void charbuf_set_capacity(charbuf *buf, size_t new_capacity) {
  buf->buf = realloc(buf->buf,new_capacity);
  buf->capacity = new_capacity;
  if( buf->size < new_capacity )
    buf->size = new_capacity;
}

int charbuf_putc_utf8(charbuf *buf, int c) {
  if( buf->size == buf->capacity )
    charbuf_set_capacity(buf,buf->capacity?buf->capacity*2:32);
  buf->buf[buf->size++] = (char)c;
  return 1;
}

void charbuf_destroy(charbuf *buf) {
  if( buf->buf )
    free(buf->buf);
}

void tokenizer_init(tokenizer *tokenizer, tokinfo *info) {
  tokenizer->info = info;
  tokenizer->tok = -1;
  tokenizer->tokstr = 0;
  tokenizer->pos.filename = 0;
  tokenizer->pos.line = 1;
  tokenizer->pos.col = 1;
  charbuf_init(&tokenizer->tokstrbuf,INITIAL_TOKSTR_CAPACITY);
  charbuf_init(&tokenizer->readbuf,BUF_BLOCK_SIZE);
  vecint_init(&tokenizer->secstack);
  vecint_push(&tokenizer->secstack,0);
}

void tokenizer_destroy(tokenizer *tokenizer) {
  charbuf_destroy(&tokenizer->tokstrbuf);
  charbuf_destroy(&tokenizer->readbuf);
  vecint_destroy(&tokenizer->secstack);
}

static int nextstate(tokinfo *info, int state, int c) {
  const int *cur = info->transitions+info->transitionOffset[state];
  const int *end = info->transitions+info->transitionOffset[state+1];
  while( cur != end ) {
    int to = *cur++;
    int cnt = *cur++;
    for( int i = 0; i < cnt; ++i ) {
      int low = *cur++;
      int high = *cur++;
      if( c >=low && c <= high )
        return to;
    }
  }
  return -1;
}

static int peekc(tokenizer *tokenizer, int byteoffset, int *bytesread) {
  while( byteoffset >= tokenizer->readbuf.size ) {
    if( tokenizer->eof )
      return -1;
    if( tokenizer->readoffset >= BUF_BLOCK_SIZE ) {
      memmove(tokenizer->readbuf.buf,tokenizer->readbuf.buf+BUF_BLOCK_SIZE,tokenizer->readbuf.size-BUF_BLOCK_SIZE);
      tokenizer->readoffset -= BUF_BLOCK_SIZE;
      tokenizer->readbuf.size -= BUF_BLOCK_SIZE;
    } else {
      charbuf_set_capacity(&tokenizer->readbuf,tokenizer->readbuf.capacity+BUF_BLOCK_SIZE);
    }
    int read = tokenizer->rdr->read(tokenizer->rdr,BUF_BLOCK_SIZE,tokenizer->readbuf.buf+tokenizer->readbuf.size);
    if( read <= 0 )
      tokenizer->eof = true;
    else
      tokenizer->readbuf.size += read;
  }
  *bytesread = 1;
  return tokenizer->readbuf.buf[tokenizer->readoffset];
}

int tokenizer_peek(tokenizer *tokenizer) {
  if( tokenizer->tok != -1 )
    return tokenizer->tok;
  bool getanothertok = true;
  while( getanothertok ) {
    getanothertok = false;
    int cursection = vecint_back(&tokenizer->secstack);
    int state = nextstate(tokenizer->info,0,cursection);
    int tok = -1, bytes = 0;
    toksize cursize = {0,0,0,0};
    toksize toksize = {0,0,0,0};
    charbuf *buf = &tokenizer->tokstrbuf;
    charbuf_clear(buf);
    while( state != -1 ) {
      int t = tokenizer->info->tokens[state];
      if( t != -1 ) {
        tok = t;
        toksize = cursize;
      }
      int c = peekc(tokenizer,cursize.bytes,&bytes);
      cursize.bytes += charbuf_putc_utf8(buf,c);
      ++cursize.len;
      cursize.bytes += bytes;
      if( c == '\n' ) {
        cursize.lines += 1;
        cursize.cols = 0;
      } else
        ++cursize.cols;
      state = nextstate(tokenizer->info,state,c);
    }
    if( tok == -1 )
      break;
    tokenizer->size = toksize;
    tokenizer->tok = tok;
    tokenizer->tokstr = buf->buf;
    const int *actions = tokenizer->info->tokenaction+(tok*tokenizer->info->sectionCount*2)+cursection*2;
    int action = actions[0];
    int actionarg = actions[1];
    if( action == 1 )
      vecint_push(&tokenizer->secstack,actionarg);
    else if( action == 2 )
      tokenizer->secstack.size--;
    else if( action == 3 )
      tokenizer->secstack.values[tokenizer->secstack.size-1] = actionarg;
    if( tokenizer->info->isws[tokenizer->tok] ) {
      getanothertok = true;
    }
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


