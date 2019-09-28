#include "parser.h"
#include <stdio.h>

const char *getarg(int argc, char *argv[], const char *arg) {
  for( int i = 1; i < argc; ++i )
    if( strncmp(argv[i],arg,strlen(arg)) == 0 )
      return argv[i];
  return 0;
}

int main(int argc, char *argv[]) {
  int verbosity = 0;
  int min_nt_value = 0;
  const char *arg;
  if( getarg(argc,argv,"-vvv") )
    verbosity = 3;
  if( getarg(argc,argv,"-vv") )
    verbosity = 2;
  else if( getarg(argc,argv,"-v") )
    verbosity = 1;
  if( (arg = getarg(argc,argv,"-minnt=")) )
    min_nt_value=atoi(arg+7);
  for( int i = 1; i < argc; ++i ) {
    if( argv[i][0] == '-' )
      continue;
    const char *fname = argv[i];
    FILE *fin = fopen(fname,"r");
    if( !fin ) {
      fprintf(stderr, "Unable to open %s for reading\n", fname);
      continue;
    }
    
    FILE *fout = 0;
    char *foutname = (char*)malloc(strlen(fname)+3);
    strcpy(foutname,fname);
    char *lastdot = strrchr(foutname,'.');
    while( *lastdot ) {
      lastdot[0] = lastdot[1];
      ++lastdot;
    }
    strcat(foutname,".h");

    try {
      FILE *vout = stderr;
      ParserDef parser(vout,verbosity);
      FileTokBuf tokbuf(fin, fname);
      ParseParser(&tokbuf,parser,vout,verbosity);
      ParserSolution solution;
      SolveParser(parser, solution, vout, verbosity);
      fout = fopen(foutname,"w");
      if( !fout ) {
        fprintf(stderr, "Unable to open %s for writing\n", foutname);
        fclose(fin);
        continue;
      }
      OutputParserSolution(fout, parser, solution, LanguageC, min_nt_value);
    } catch(ParserErrorWithLineCol &pe) {
      fprintf(stderr, "%s(%d:%d) : %s\n", pe.m_filename.c_str(), pe.m_line, pe.m_col, pe.m_err.c_str());
    } catch(ParserError &pe) {
      fputs(pe.m_err.c_str(),stderr);
    }
    fclose(fin);
    if( fout )
      fclose(fout);
  }
  return 0;
}
