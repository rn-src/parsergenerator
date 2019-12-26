#include "parser.h"
#include <stdio.h>

const char *getarg(int argc, char *argv[], const char *arg) {
  for( int i = 1; i < argc; ++i )
    if( strncmp(argv[i],arg,strlen(arg)) == 0 )
      return argv[i];
  return 0;
}

char *makeFOutName(const char *fname) {
  char *foutname = (char*)malloc(strlen(fname) + 3);
  strcpy(foutname, fname);
  char *lastdot = strrchr(foutname, '.');
  while (*lastdot) {
    lastdot[0] = lastdot[1];
    ++lastdot;
  }
  strcat(foutname, ".h");
  return foutname;
}

void parseArgs(int argc, char *argv[], int *pverbosity, int *ptimed, LanguageOutputOptions *options, const char **pfname) {
  int verbosity = 0;
  int timed = 0;
  const char *arg;
  if (getarg(argc, argv, "-vvv"))
    verbosity = 3;
  else if (getarg(argc, argv, "-vv"))
    verbosity = 2;
  else if (getarg(argc, argv, "-v"))
    verbosity = 1;
  if ((arg = getarg(argc, argv, "--minnt=")))
    options->min_nt_value = atoi(arg + 8);
  if ((arg = getarg(argc, argv, "--no-pound-line")))
    options->do_pound_line = false;
  if (getarg(argc, argv, "--timed"))
    timed = 1;
  if (getarg(argc, argv, "--lr"))
    options->m_parserType = ParserType_LR;
  if (getarg(argc, argv, "--ll"))
    options->m_parserType = ParserType_LL;
  for (int i = 1; i < argc; ++i) {
    if (argv[i][0] == '-')
      continue;
    *pfname = argv[i];
    break;
  }
  *pverbosity = verbosity;
  *ptimed = timed;
}

int main(int argc, char *argv[]) {
  int verbosity = 0;
  int timed = 0;
  const char *fname = 0;
  LanguageOutputOptions options = { 0, true, ParserType_LR };

  parseArgs(argc, argv, &verbosity, &timed, &options, &fname);

  if (!fname) {
    fputs("usage : parser [-v[v[v]]] [--minnt=int] [--no-pound-line] [--timed] filename\n"
          "   -v          : verbosity, 1 through 3 times\n"
          "--minnt        : minimum nonterminal value, if the default output is too low\n"
          "--lr           : output an LR parser\n"
          "--ll           : output an LL parser\n"
          "-no-pound-line : turn off #line directives in the output\n", stderr);
    return -1;
  }

  FILE *fin = 0;
  FILE *fout = 0;
  char *foutname = 0;
  FILE *vout = stdout;
  ParserDef parser;
  FileTokBuf tokbuf;
  LRParserSolution solution;
  LLParserSolution llsolution;
  ParseError *pe = 0;
  int ret = 0;

  Scope_Push();
  Scope_SetJmp(ret);
  if (!ret) {
    fin = fopen(fname, "r");
    if (!fin) {
      fprintf(stderr, "Unable to open %s for reading\n", fname);
      Scope_Pop();
      return -1;
    }
    Push_Destroy(fin, fclose);
    foutname = makeFOutName(fname);
    Push_Destroy(foutname, free);
    ParserDef_init(&parser,vout,verbosity,true);
    FileTokBuf_init(&tokbuf,fin,fname,true);
    LRParserSolution_init(&solution, true);
    LLParserSolution_init(&llsolution, true);
    ParseParser(&tokbuf.m_tokbuf,&parser,vout,verbosity);
    int n = 0;

    if (options.m_parserType == ParserType_LR)
      n = LR_SolveParser(&parser, &solution, vout, verbosity, timed);
    else if (options.m_parserType == ParserType_LL)
      n = LL_SolveParser(&parser, &llsolution, vout, verbosity, timed);

    if( n != 0 ) {
      Scope_Pop();
      return n;
    }
    fout = fopen(foutname,"w");
    if( !fout ) {
      fprintf(stderr, "Unable to open %s for writing\n", foutname);
      Scope_Pop();
      return -1;
    }
    Push_Destroy(fout, fclose);

    if (options.m_parserType == ParserType_LR)
      OutputLRParserSolution(fout, &parser, &solution, &options);
    else if (options.m_parserType == ParserType_LL)
      OutputLLParserSolution(fout, &parser, &llsolution, &options);

  } else if( getParseError(&pe) ) {
    if( pe->line != -1 )
      fprintf(stderr, "%s(%d:%d) : %s\n", String_Chars(&pe->file), pe->line, pe->col, String_Chars(&pe->err));
    else
      fputs(String_Chars(&pe->err),stderr);
    Scope_Pop();
    return -1;
  } else {
    fputs("Unknown error\n", stderr);
    Scope_Pop();
    return -1;
  }
  Scope_Pop();
  return 0;
}
