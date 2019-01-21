#include "parser.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
  ParserDef parser;
  try {
    TokBuf tokbuf(stdin);
    ParseParser(&tokbuf,parser);
  } catch(ParserError &pe) {
    fprintf(stderr, "<stdin>(%d:%d) : %s", pe.m_line, pe.m_col, pe.m_err.c_str());
  }
  if( ! SolveParser(parser) )
    return 0;
  OutputParser(parser);
  return 0;
}
