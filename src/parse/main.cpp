#include "parser.h"
#include <stdio.h>

bool hasarg(int argc, char *argv[], const char *arg) {
  for( int i = 1; i < argc; ++i )
    if( strcmp(argv[i],arg) == 0 )
      return true;
  return false;
}

int main(int argc, char *argv[]) {
  bool bVerbose = hasarg(argc,argv,"--verbose");
  ParserDef parser;
  try {
    TokBuf tokbuf(stdin);
    ParseParser(&tokbuf,parser);
    ParserSolution solution;
    NormalizeParser(parser, bVerbose);
    SolveParser(parser, solution, bVerbose);
    OutputParserSolution(stdout, parser, solution, LanguageC);
  } catch(ParserErrorWithLineCol &pe) {
    fprintf(stderr, "<stdin>(%d:%d) : %s", pe.m_line, pe.m_col, pe.m_err.c_str());
  } catch(ParserError &pe) {
    fputs(pe.m_err.c_str(),stderr);
  }
  return 0;
}
