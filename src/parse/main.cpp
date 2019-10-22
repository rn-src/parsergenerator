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
  int timed = 0;
  LanguageOutputOptions options;
  const char *arg;
  if( getarg(argc,argv,"-vvv") )
    verbosity = 3;
  else if( getarg(argc,argv,"-vv") )
    verbosity = 2;
  else if( getarg(argc,argv,"-v") )
    verbosity = 1;
  if( (arg = getarg(argc,argv,"--minnt=")) )
    options.min_nt_value=atoi(arg+8);
  if( (arg = getarg(argc,argv,"--no-pound-line")) )
    options.do_pound_line = false;
  if( getarg(argc,argv,"--timed") )
    timed = 1;
  const char *fname = 0;
  for( int i = 1; i < argc; ++i ) {
    if( argv[i][0] == '-' )
      continue;
    fname = argv[i];
    break;
  }
  FILE *fin = fopen(fname,"r");
  if( !fin ) {
    fprintf(stderr, "Unable to open %s for reading\n", fname);
    return 1;
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
    FILE *vout = stdout;
    ParserDef parser(vout,verbosity);
    FileTokBuf tokbuf(fin, fname);
    ParseParser(&tokbuf,parser,vout,verbosity);
    ParserSolution solution;
    int n = SolveParser(parser, solution, vout, verbosity, timed);
    if( n != 0 ) {
      fclose(fin);
      return n;
    }
    fout = fopen(foutname,"w");
    if( !fout ) {
      fprintf(stderr, "Unable to open %s for writing\n", foutname);
      fclose(fin);
      return -1;
    }
    OutputParserSolution(fout, parser, solution, LanguageC, options);
  } catch(ParserError &pe) {
    if( pe.m_line != 0 )
      fprintf(stderr, "%s(%d:%d) : %s\n", pe.m_filename.c_str(), pe.m_line, pe.m_col, pe.m_err.c_str());
    else
      fputs(pe.m_err.c_str(),stderr);
    fclose(fin);
    return -1;
  }
  fclose(fin);
  if( fout )
    fclose(fout);
  return 0;
}
