#include "lrparse.h"
#include "common.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#define INITIAL_VEC_CAPACITY (8)
#define TOKEN_SIZE (16)
#define TOKENS_PER_BLOCK (64)

static char *tokdup(const char *s) {
  if( ! s )
    return 0;
  char *dup = (char*)mrealloc(0,0,strlen(s)+1);
  strcpy(dup,s);
  return dup;
}

static void tokfree(char *s) {
  mrealloc(s,0,0);
}

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
  v->values = 0;
  v->size = 0;
  v->capacity = 0;
}

void vectok_destroy(vectok *v) {
  mrealloc(v->values,0,0);
}

Token *vectok_item(vectok *v, size_t i) {
  return (Token*)((char*)v->values+v->itemsize*i);
}

void vectok_push(vectok *v, Token *t, size_t copysize) {
  if( v->size == v->capacity ) {
    size_t newcapacity = v->capacity?v->capacity*2:INITIAL_VEC_CAPACITY;
    v->values = mrealloc(v->values,v->capacity*v->itemsize,newcapacity*v->itemsize);
    v->capacity = newcapacity;
  }
  memset(vectok_item(v,v->size),0,v->itemsize);
  if( t )
    memcpy(vectok_item(v,v->size),t,copysize);
  v->size++;
}

Token *vectok_back(vectok *v) {
  return (Token*)((char*)v->values+v->itemsize*(v->size-1));
}

bool findmatchingaction(intiter *iiactions, int tok, int actionvalues[3]) {
  int action, a1 = 0, a2 = 0;
  while( ! intiter_end(iiactions) ) {
    action = actionvalues[0] = intiter_next(iiactions);
    if( action == ACTION_SHIFT ) {
      a1 = intiter_next(iiactions); // shift-to
    } else if( action == ACTION_REDUCE || action == ACTION_STOP ) {
      a1 = intiter_next(iiactions); // reduce-by
      a2 = intiter_next(iiactions); // reduce-count
    }
    int nsymbols = intiter_next(iiactions);
    while( nsymbols-- ) {
      int s = intiter_next(iiactions);
      if( s == tok ) {
        if( nsymbols )
          intiter_skip(iiactions,nsymbols);
        actionvalues[0] = action;
        actionvalues[1] = a1;
        actionvalues[2] = a2;
        return true;
      }
    }
  }
  actionvalues[0] = -1;
  return false;
}

struct parsecontext {
  tokenizer *toks;
  parseinfo *parseinfo;
  void *extra;
  int verbosity;
  int inputnum;
  writer *writer;
  vecint states;
  vectok values;
  vecint states_inputqueue;
  vectok values_inputqueue;
  intiter iiactions, iitokstate, iitoksection;
};
typedef struct parsecontext parsecontext;

void parsecontext_init(parsecontext *ctx, tokenizer *toks, parseinfo *parseinfo, void *extra, int verbosity, writer *writer) {
  ctx->toks = toks;
  ctx->parseinfo = parseinfo;
  ctx->extra = extra;
  ctx->verbosity = verbosity;
  ctx->inputnum = 0;
  ctx->writer = writer;
  vecint_init(&ctx->states);
  vectok_init(&ctx->values, parseinfo->itemsize);
  vecint_init(&ctx->states_inputqueue);
  vectok_init(&ctx->values_inputqueue, parseinfo->itemsize);
  vecint_push(&ctx->states,0);
  vectok_push(&ctx->values,0,0);
  intiter_init(&ctx->iiactions,parseinfo->actions_format,parseinfo->actions,parseinfo->actionstart,parseinfo->actionstateindex_count,parseinfo->actionstartindex,DECODEDELTA);
  intiter_init(&ctx->iitokstate,toks->info->stateinfo_format,toks->info->stateinfo,toks->info->stateinfo_offset,toks->info->stateinfoindex_count,toks->info->stateinfoindex,DECODEDELTA);
  intiter_init(&ctx->iitoksection,ENC_8BIT,toks->info->sectioninfo,toks->info->sectioninfo_offset,0,0,0);  
}

void parsecontext_destroy(parsecontext *ctx) {
  vecint_destroy(&ctx->states);
  vectok_destroy(&ctx->values);
  vecint_destroy(&ctx->states_inputqueue);
  vectok_destroy(&ctx->values_inputqueue);
  intiter_destroy(&ctx->iiactions);
  intiter_destroy(&ctx->iitokstate);
  intiter_destroy(&ctx->iitoksection);
}

static void printstatestack(vecint *states, writer *writer) {
  writer->puts(writer,"lrstates = { ");
  for( int i = 0; i < states->size; ++i )
    writer->printf(writer,"%d ",states->values[i]);
  writer->puts(writer,"}\n");
}

static void printreduceaction(tokenizer *toks,
              parseinfo *parseinfo,
              int reduceby,
              int reducecount,
              writer *writer) {
  int reducebyp = reduceby-parseinfo->prod0;
  writer->printf(writer,"reduce %d states by rule %d [", reducecount, reducebyp+1);
  intiter ii;
  // no decodedelta on these
  intiter_init(&ii,ENC_8BIT,parseinfo->productions,parseinfo->productionstart,0,0,0);
  intiter_seek(&ii,reducebyp,0);
  bool first = true;
  while( ! intiter_end(&ii) ) {
    int s = intiter_next(&ii);
    writer->printf(writer, " %s", s >= parseinfo->start ? parseinfo->nonterminals[s-parseinfo->start] : tokenizer_tokid2str(toks,s));
    if( first ) {
      first = false;
      writer->puts(writer," :");
    }
  }
  writer->puts(writer," ] ? ... ");
  intiter_destroy(&ii);
}

static void printtoken(tokenizer *toks, int inputnum, int tok, const Token *t, writer *writer) {
  if( t->s )
    writer->printf(writer, "input (# %d) = %d %s = \"%s\" - %s(%d:%d)\n", inputnum, tok, tokenizer_tokid2str(toks,tok),t->s,t->pos.filename,t->pos.line,t->pos.col);
}

bool doaction(parsecontext *ctx,
              const char **err, int actionvalues[3]) {
  int action = actionvalues[0];
  if( action == ACTION_SHIFT ) {
    int shiftto = actionvalues[1];
    if( ctx->verbosity ) ctx->writer->printf(ctx->writer,"shift to %d\n", shiftto);
    vecint_push(&ctx->states,shiftto);
    vectok_push(&ctx->values,vectok_back(&ctx->values_inputqueue),ctx->parseinfo->itemsize);
    ctx->states_inputqueue.size -= 1;
    ctx->values_inputqueue.size -= 1;
    return true;
  } else if( action == ACTION_REDUCE || action == ACTION_STOP ) {
    int reduceby = actionvalues[1];
    int reducecount = actionvalues[2];
    if( ctx->verbosity ) printreduceaction(ctx->toks,ctx->parseinfo,reduceby,reducecount,ctx->writer);
    size_t inputpos = ctx->values.size-reducecount;
    vectok_push(&ctx->values,0,0);
    Token *inputs = vectok_item(&ctx->values,inputpos);
    Token *tmpout = vectok_back(&ctx->values);
    tmpout->tok = reduceby;
    tmpout->pos = vectok_back(&ctx->values_inputqueue)->pos;
    if( ctx->parseinfo->reduce(ctx->extra,reduceby,inputs,tmpout,err) ) {
      if( ctx->verbosity ) ctx->writer->puts(ctx->writer,"YES\n");
      for( int ir = 0; ir < reducecount; ++ir )
        tokfree((char*)vectok_item(&ctx->values,inputpos+ir)->s);
      vecint_push(&ctx->states_inputqueue,reduceby);
      vectok_push(&ctx->values_inputqueue,tmpout,ctx->parseinfo->itemsize);
      ctx->states.size = inputpos;
      ctx->values.size = inputpos;
      if( action == ACTION_STOP && ctx->verbosity ) ctx->writer->puts(ctx->writer,"ACCEPT\n");
      return true;
    } else {
      if( ctx->verbosity ) ctx->writer->puts(ctx->writer,"NO\n");
      ctx->values.size--;
      return false;
    }
  }
  return false;
}

static bool handleerror(parsecontext *ctx, int tok, const char *err) {
  tokpos tpos = vectok_back(&ctx->values_inputqueue)->pos;
  if( tok == ctx->parseinfo->parse_error ) {
    // Input is error and no handler, keep popping states until we find a handler or run out.
    if( ctx->verbosity )
      ctx->writer->printf(ctx->writer,"during error handling, popping lr state %d\n", vecint_back(&ctx->states));
    if( ctx->states.size > 1 )
      ctx->states.size -= 1;
    else {
      if( ctx->verbosity ) {
        tokpos *pos = &ctx->toks->pos;
        ctx->writer->printf(ctx->writer,"%s(%d:%d) : parse error : ", tpos.filename, tpos.line, tpos.col);
        if( err )
          ctx->writer->puts(ctx->writer,err);
        else
          ctx->writer->printf(ctx->writer," parse error : input (# %d) = %d = \"%s\"", ctx->inputnum, tok, tokenizer_tokid2str(ctx->toks,tok));
        ctx->writer->printf(ctx->writer,"  at %s(%d:%d)\n", pos->filename, pos->line, pos->col);
      }
      return false;
    }
  } else {
    Token t_err = {ctx->parseinfo->parse_error,0,tpos};
    vecint_push(&ctx->states_inputqueue,ctx->parseinfo->parse_error);
    vectok_push(&ctx->values_inputqueue,&t_err,sizeof(Token));
  }
  return true;
}

static int curstate(parsecontext *ctx) {
  if( ctx->verbosity ) printstatestack(&ctx->states, ctx->writer);
  return vecint_back(&ctx->states);
}

static int nexttoken(parsecontext *ctx) {
  if( ! ctx->states_inputqueue.size ) {
    ctx->inputnum++;
    int nxt = tokenizer_peek(ctx->toks,&ctx->iitokstate,&ctx->iitoksection);
    Token t;
    memset(&t, 0, sizeof(t));
    t.tok = nxt;
    t.s = tokdup(ctx->toks->tokstr);
    t.pos = ctx->toks->pos;
    tokenizer_discard(ctx->toks);
    vecint_push(&ctx->states_inputqueue,nxt);
    vectok_push(&ctx->values_inputqueue,&t,sizeof(Token));
  }
  int tok = vecint_back(&ctx->states_inputqueue);
  if( ctx->verbosity ) printtoken(ctx->toks, ctx->inputnum, tok, vectok_back(&ctx->values_inputqueue), ctx->writer);
  return tok;
}

Token *parse(tokenizer *toks, parseinfo *parseinfo, void *extra, int verbosity, writer *writer) {
  parsecontext ctx;
  const char *err = 0;
  Token *ret = 0;
  parsecontext_init(&ctx, toks, parseinfo, extra, verbosity, writer);
  int tok = -1;
  while( ctx.states.size > 0 ) {
    int stateno = curstate(&ctx);
    tok = nexttoken(&ctx);
    intiter_seek(&ctx.iiactions,stateno,0);
    int actionvalues[3];
    while( findmatchingaction(&ctx.iiactions,tok,actionvalues) ) {
      if( doaction(&ctx, &err, actionvalues) )
        break;
    }
    if( actionvalues[0] != -1 ) {
      if( actionvalues[0] != ACTION_STOP )
        continue;
      ret = (Token*)mrealloc(0,0,parseinfo->itemsize);
      memcpy(ret,vectok_back(&ctx.values_inputqueue),parseinfo->itemsize);
      break;
    }
    if( ! handleerror(&ctx, tok, err) ) {
      break;
    }
  }
  parsecontext_destroy(&ctx);
  return ret;
}

