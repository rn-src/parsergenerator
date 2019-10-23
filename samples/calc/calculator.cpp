#include "../tok/tinytemplates.h"
#include "../tok/tok.h"
#include <stdio.h>
#include "calculatorL.h"
TokenInfo calctokinfo = {
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
#include "../parse/lrparse.h"
#include "calculatorG.h"
ParseInfo calclrinfo = {
  nstates,
  actions,
  actionstart
};

int main() {
  FileTokBuf tokbuf(stdin, "<stdin>");
  TokBufTokenizer toks(&tokbuf,&calctokinfo);
  Parser<extra_t,stack_t> parser(&calclrinfo,reduce);
  if( parser.parse(&toks) )
    fprintf(stderr, "%f\n", parser.m_extra);
  else
    fprintf(stderr,"parser error at input:%d(%d)\n", tokbuf.line(),tokbuf.col());
  return 0;
}
