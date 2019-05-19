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
  for( int i = 1; i < argc; ++i ) {
    if( argv[i][0] == '-' )
      continue;
    const char *fname = argv[i];
    FILE *fin = fopen(fname,"r");
    if( !fin ) {
      fprintf(stderr, "Unable to open %s for reading\n", fname);
      continue;
    }
    
    char *foutname = (char*)malloc(strlen(fname)+3);
    strcpy(foutname,fname);
    char *lastdot = strrchr(foutname,'.');
    while( *lastdot ) {
      lastdot[0] = lastdot[1];
      ++lastdot;
    }
    strcat(foutname,".h");
    FILE *fout = fopen(foutname,"w");
    if( !fin ) {
      fprintf(stderr, "Unable to open %s for writing\n", foutname);
      fclose(fin);
      continue;
    }
    try {
      FILE *vout = stderr;
      ParserDef parser(vout,verbosity);
      FileTokBuf tokbuf(fin, fname);
      ParseParser(&tokbuf,parser,vout,verbosity);
      ParserSolution solution;
      SolveParser(parser, solution, vout, verbosity);
      OutputParserSolution(fout, parser, solution, LanguageC);
    } catch(ParserErrorWithLineCol &pe) {
      fprintf(stderr, "%s(%d:%d) : %s\n", pe.m_filename.c_str(), pe.m_line, pe.m_col, pe.m_err.c_str());
    } catch(ParserError &pe) {
      fputs(pe.m_err.c_str(),stderr);
    }
    fclose(fin);
    fclose(fout);
  }
  return 0;
}
