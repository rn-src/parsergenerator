#include "parser.h"
#include <stdio.h>

bool hasarg(int argc, char *argv[], const char *arg) {
  for( int i = 1; i < argc; ++i )
    if( strcmp(argv[i],arg) == 0 )
      return true;
  return false;
}

int main(int argc, char *argv[]) {
  int verbosity = 0;
  if( hasarg(argc,argv,"-vvv") )
    verbosity = 3;
  if( hasarg(argc,argv,"-vv") )
    verbosity = 2;
  else if( hasarg(argc,argv,"-v") )
    verbosity = 1;
  try {
    FILE *vout = stderr;
    ParserDef parser(vout,verbosity);
    TokBuf tokbuf(stdin);
    ParseParser(&tokbuf,parser,vout,verbosity);
    ParserSolution solution;
    SolveParser(parser, solution, vout, verbosity);
    OutputParserSolution(stdout, parser, solution, LanguageC);
  } catch(ParserErrorWithLineCol &pe) {
    fprintf(stderr, "<stdin>(%d:%d) : %s\n", pe.m_line, pe.m_col, pe.m_err.c_str());
  } catch(ParserError &pe) {
    fputs(pe.m_err.c_str(),stderr);
  }
  return 0;
}
