#include "parser.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
  try {
    TokBuf tokbuf(stdin);
    ParserDef parser;
    ParseParser(&tokbuf,parser);
    OutputParser(parser);
  } catch(ParserError &pe) {
    fprintf(stderr, "<stdin>(%d:%d) : %s", pe.m_line, pe.m_col, pe.m_err.c_str());
  }
  return 0;
}
