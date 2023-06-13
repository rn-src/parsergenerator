#include <stdio.h>
#include "../../src/tok/tinytemplates.h"
#include "../../src/tok/tok.h"
#include "parserL.h"
TokenInfo tokinfo = {
  tokenCount,
  sectionCount,
  tokenaction,
  tokenstr,
  isws,
  stateCount,
  transitions,
  transitionOffset,
  tokens
};
#include "../../src/parse/lrparse.h"
#include "parserG.h"
ParseInfo lrinfo = {
  nstates,
  actions,
  actionstart,
  PROD_0,
  nproductions,
  productions,
  productionstart,
  START,
  nonterminals
};

int main() {
  FileTokBuf tokbuf(stdin, "<stdin>");
  TokBufTokenizer toks(&tokbuf,&tokinfo);
  Parser<extra_t,stack_t> parser(&parserlrinfo,reduce,0,0);
  if( parser.parse(&toks) )
    fprintf(stderr, "%f\n", parser.m_extra);
  else
    fprintf(stderr,"parser error at input:%d(%d)\n", tokbuf.line(),tokbuf.col());
  return 0;
}
