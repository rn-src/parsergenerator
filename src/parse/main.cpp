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
  ParserSolution solution;
  if( ! NormalizeParser(parser) )
    return 0;
  if( ! SolveParser(parser, solution) )
    return 0;
  OutputParserSolution(stdout, parser, solution, LanguageC);
  return 0;
}
