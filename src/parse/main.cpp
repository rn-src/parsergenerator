#include "parser.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
  ParserDef parser;
  try {
    TokBuf tokbuf(stdin);
    ParseParser(&tokbuf,parser);
    ParserSolution solution;
    NormalizeParser(parser);
    SolveParser(parser, solution);
    OutputParserSolution(stdout, parser, solution, LanguageC);
  } catch(ParserErrorWithLineCol &pe) {
    fprintf(stderr, "<stdin>(%d:%d) : %s", pe.m_line, pe.m_col, pe.m_err.c_str());
  } catch(ParserError &pe) {
    fputs(pe.m_err.c_str(),stderr);
  }
  return 0;
}
