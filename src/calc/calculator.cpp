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
  TokBuf tokbuf(stdin);
  Parser<stack_t> parser(&calctokinfo,&calclrinfo,reduce);
  if( ! parser.parse(&tokbuf) )
    fprintf(stderr,"parser error at input:%d(%d)\n", tokbuf.line(),tokbuf.col());
  return 0;
}
