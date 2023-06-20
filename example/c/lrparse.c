#include "lrparse.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#define INITIAL_VEC_CAPACITY (8)
#define TOKEN_SIZE (16)
#define TOKENS_PER_BLOCK (64)

void writer_printf(writer *w, const char *format, ...) {
  va_list args;
  va_start(args,format);
  vfprintf(stderr, format, args);
  va_end(args);
}

void writer_puts(writer *w, const char *s) {
  fputs(s, stderr);
}

writer stderrwriter = {writer_printf,writer_puts};

writer *stderr_writer() {
  return &stderrwriter;
}

struct vectok {
  void *values;
  size_t itemsize;
  size_t size;
  size_t capacity;
};
typedef struct vectok vectok;

void vectok_init(vectok *v, int itemsize) {
  v->itemsize = itemsize;
  v->values = malloc(v->itemsize*INITIAL_VEC_CAPACITY);
  v->size = 0;
  v->capacity = INITIAL_VEC_CAPACITY;
}

void vectok_destroy(vectok *v) {
  if( v->values )
    free(v->values);
}

Token *vectok_item(vectok *v, size_t i) {
  return (Token*)((char*)v->values+v->itemsize*i);
}

void vectok_push(vectok *v, Token *t, size_t copysize) {
  if( v->size == v->capacity ) {
    v->values = realloc(v->values,v->size*v->itemsize*2);
    v->capacity *= 2;
  }
  if( t )
    memcpy(vectok_item(v,v->size++),t,copysize);
  else
    memset(vectok_item(v,v->size++),0,v->itemsize);
}

Token *vectok_back(vectok *v) {
  return (Token*)((char*)v->values+v->itemsize*(v->size-1));
}

struct memnode {
  char *buf;
  size_t tokens;
  struct memnode *next;
};
typedef struct memnode memnode;

struct tokenbank {
  memnode *first;
  memnode *last;
};
typedef struct tokenbank tokenbank;

static memnode *create_memnode(size_t tokens) {
  memnode *node = (memnode*)malloc(sizeof(memnode));
  node->buf = (char*)malloc(tokens*TOKEN_SIZE);
  memset(node->buf,0,tokens*TOKEN_SIZE);
  node->tokens = tokens;
  node->next = 0;
  return node;
}

void tokenbank_init(tokenbank *tokbank) {
  tokbank->first = tokbank->last = 0;
}

void tokenbank_destroy(tokenbank *tokbank) {
  memnode *cur = tokbank->first;
  while( cur ) {
    memnode *next = cur->next;
    free(cur->buf);
    free(cur);
    cur = next;
  }
}

const char *tokenbank_strdup(tokenbank *tokbank, const char *str) {
  if( ! str )
    return 0;
  memnode *node = tokbank->first;
  size_t len = strlen(str)+2;
  size_t blks = (len/TOKEN_SIZE)+((len%TOKEN_SIZE)?1:0);
  while( node ) {
    size_t cnt = 0;
    for( size_t i = 0, n = node->tokens; i < n; ++i ) {
      if( ! node->buf[i*TOKEN_SIZE] && ! node->buf[i*TOKEN_SIZE+1] )
        ++cnt;
      else
        cnt = 0;
      if( cnt == blks ) {
        char *dst = node->buf+(i-cnt)*TOKEN_SIZE;
        strcpy(dst,str);
        dst[len-1]=1;
        return dst;
      }
    }
    node = node->next;
  }
  size_t reqblks = blks < TOKENS_PER_BLOCK ? TOKENS_PER_BLOCK : blks; 
  node = create_memnode(reqblks);
  if( tokbank->last ) {
    tokbank->last->next = node;
    tokbank->last = node;
  } else
    tokbank->first = tokbank->last = node;
  char *dst = node->buf;
  strcpy(dst,str);
  dst[len-1]=1;
  return dst;
}

void tokenbank_release(tokenbank *tokbank, const char *str) {
  if( ! str || ! *str )
    return;
  memset((char*)str,0,strlen(str)+1);
}

bool findsymbol(int tok, const int *symbols, int nsymbols) {
  for( int i = 0; i < nsymbols; ++i )
    if( symbols[i] == tok )
      return true;
  return false;
}

bool parse(tokenizer *toks, parseinfo *parseinfo, void *extra, int verbosity, writer *writer) {
  vecint states;
  vectok values;
  vecint states_inputqueue;
  vectok values_inputqueue;
  tokenbank tokbank;
  bool ret = true;
  vecint_init(&states);
  vectok_init(&values, parseinfo->itemsize);
  vecint_init(&states_inputqueue);
  vectok_init(&values_inputqueue, parseinfo->itemsize);
  tokenbank_init(&tokbank);
  vecint_push(&states,0);
  vectok_push(&values,0,0);
  int tok = -1;
  int inputnum = 0;
  const char *err = 0;
  while( states.size > 0 ) {
    if( verbosity ) {
      writer->puts(writer,"lrstates = { ");
      for( int i = 0; i < states.size; ++i )
        writer->printf(writer,"%d ",states.values[i]);
      writer->puts(writer,"}\n");
    }
      int stateno = vecint_back(&states);
      if( ! states_inputqueue.size ) {
        inputnum += 1;
        int nxt = tokenizer_peek(toks);
        Token t = {nxt, tokenbank_strdup(&tokbank,toks->tokstr), toks->pos};
        tokenizer_discard(toks);
        vecint_push(&states_inputqueue,nxt);
        vectok_push(&values_inputqueue,&t,sizeof(Token));
      }
      tok = vecint_back(&states_inputqueue);
      if( verbosity ) {
        const Token *t = vectok_back(&values_inputqueue);
        if( t->s )
          writer->printf(writer, "input (# %d) = %d = \"%s\" - %s(%d:%d)\n", inputnum, tok, tokenizer_tokid2str(toks,tok),t->s,t->pos.filename,t->pos.line,t->pos.col);
      }
      const int *firstaction = parseinfo->actions+parseinfo->actionstart[stateno];
      const int *lastaction = parseinfo->actions+parseinfo->actionstart[stateno+1];
      while( firstaction != lastaction ) {
        const int *nextaction;
        int action = firstaction[0];
        bool didaction = false;
        if( action == ACTION_SHIFT ) {
          int shiftto = firstaction[1];
          int nsymbols = firstaction[2];
          nextaction = firstaction+3+nsymbols;
          if( findsymbol(tok,firstaction+3,nsymbols) ) {
            if( verbosity )
              writer->printf(writer,"shift to %d\n", shiftto);
            vecint_push(&states,shiftto);
            vectok_push(&values,vectok_back(&values_inputqueue),parseinfo->itemsize);
            states_inputqueue.size -= 1;
            values_inputqueue.size -= 1;
            didaction = true;
          }
        } else if( action == ACTION_REDUCE || action == ACTION_STOP ) {
          int reduceby = firstaction[1];
          int reducecount = firstaction[2];
          int nsymbols = firstaction[3];
          nextaction = firstaction+4+nsymbols;
          if( findsymbol(tok,firstaction+4,nsymbols) ) {
            Token *inputs = vectok_item(&values,values.size-reducecount);
            if( verbosity ) {
              int reducebyp = reduceby-parseinfo->prod0;
              writer->printf(writer,"reduce %d states by rule %d [", reducecount, reducebyp+1);
              const int *pproduction = parseinfo->productions+parseinfo->productionstart[reducebyp];
              int pcount = parseinfo->productionstart[reducebyp+1]-parseinfo->productionstart[reducebyp];
              bool first = true;
              while( pcount-- ) {
                int s = *pproduction++;
                writer->puts(writer, " ");
                writer->puts(writer,s >= parseinfo->start ? parseinfo->nonterminals[s-parseinfo->start] : toks->tokstr);
                if( first ) {
                  first = false;
                  writer->puts(writer," :");
                }
              }
              writer->puts(writer," ] ? ... ");
            }
            err = 0;
#ifdef WIN32
            char *v = (char*)_alloca(parseinfo->itemsize);
#else
	    char *v = char[parseinfo->itemsize];
#endif
            Token *output = (Token*)v;
            memset(output,0,parseinfo->itemsize);
            output->tok = reduceby;
            output->pos = vectok_back(&values_inputqueue)->pos;
            if( parseinfo->reduce(extra,reduceby,inputs,output,&err) ) {
              if( verbosity )
                writer->puts(writer,"YES\n");
              for( int ir = 0; ir < reducecount; ++ir )
                tokenbank_release(&tokbank,inputs[ir].s);
              states.size -= reducecount;
              values.size -= reducecount;
              vecint_push(&states_inputqueue,reduceby);
              vectok_push(&values_inputqueue,output,parseinfo->itemsize);
              didaction = true;
            } else {
              if( verbosity )
                writer->puts(writer,"NO\n");
            }
          }
          if( action == ACTION_STOP && didaction ) {
            if( verbosity )
              writer->puts(writer,"ACCEPT\n");
            ret = true;
            goto cleanup;
          }
        }
        if( didaction )
          break;
        firstaction = nextaction;
      }
      if( firstaction == lastaction ) {
        if( tok == parseinfo->parse_error ) {
          // Input is error and no handler, keep popping states until we find a handler or run out.
          if( verbosity )
            writer->printf(writer,"during error handling, popping lr state %d\n", vecint_back(&states));
          states.size--;
          if( states.size == 0 ) {
            if( verbosity )
              writer->puts(writer,"NO ERROR HANDLER AVAILABLE, FAIL\n");
            ret = false;
            goto cleanup;
          }
        }
        else {
          const Token *t = vectok_back(&values_inputqueue);
          // report the error for verbose output, but add error to the input, hoping for a handler, and carry on
          if( verbosity ) {
            tokpos *pos = &toks->pos;
            writer->printf(writer,"%s(%d:%d) : parse error : ", t->pos.filename, t->pos.line, t->pos.col);
            if( err )
              writer->puts(writer,err);
            else
              writer->printf(writer," parse error : input (# %d) = %d = \"%s\"", inputnum, tok, tokenizer_tokid2str(toks,tok));
            while( pos ) {
              writer->printf(writer,"  at %s(%d:%d)\n", pos->filename, pos->line, pos->col);
            }
          }
          Token t_err = {parseinfo->parse_error,0,t->pos};
          vecint_push(&states_inputqueue,parseinfo->parse_error);
          vectok_push(&values_inputqueue,&t_err,sizeof(Token));
        }
      }
    }
cleanup:
  vecint_destroy(&states);
  vectok_destroy(&values);
  vecint_destroy(&states_inputqueue);
  vectok_destroy(&values_inputqueue);
  tokenbank_destroy(&tokbank);
  return ret;
}

