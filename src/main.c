#include "parser.h"
#include "tokenizer.h"
#include <stdio.h>
#include <string.h>

const char *getarg(int argc, char *argv[], const char *arg) {
  for( int i = 1; i < argc; ++i )
    if( strncmp(argv[i],arg,strlen(arg)) == 0 )
      return argv[i];
  return 0;
}

char *makeFOutName(const char *fname, LanguageOutputOptions *options) {
  char *foutname = (char*)malloc(strlen(fname) + 3);
  strcpy(foutname, fname);
  char *lastdot = strrchr(foutname, '.');
  while (*lastdot) {
    lastdot[0] = lastdot[1];
    ++lastdot;
  }
  if( options->outputLanguage == OutputLanguage_Python )
    strcat(foutname, ".py");
  else
    strcat(foutname, ".h");
  return foutname;
}

void parseArgs(int argc, char *argv[], int *pverbosity, int *ptimed, LanguageOutputOptions *options, const char **pfname) {
  int verbosity = 0;
  int timed = 0;
  const char *arg;
  if (getarg(argc, argv, "--help")) {
    *pfname = 0;
    return;
  }
  if (getarg(argc, argv, "-vvv"))
    verbosity = 3;
  else if (getarg(argc, argv, "-vv"))
    verbosity = 2;
  else if (getarg(argc, argv, "-v"))
    verbosity = 1;
  if ((arg = getarg(argc, argv, "--lexer=")))
    options->lexerName = arg + 8;
  if ((arg = getarg(argc, argv, "--minnt=")))
    options->min_nt_value = atoi(arg + 8);
  if ((arg = getarg(argc, argv, "--no-pound-line")))
    options->do_pound_line = false;
  if (getarg(argc, argv, "--timed"))
    timed = 1;
  if (getarg(argc, argv, "--c"))
    options->outputLanguage = OutputLanguage_C;
  if (getarg(argc, argv, "--py"))
    options->outputLanguage = OutputLanguage_Python;
  for (int i = 1; i < argc; ++i) {
    if (strncmp(argv[i], "--import=",9) == 0)
      LanguageOutputOptions_import(options,argv[i]+9,0);
    if (strncmp(argv[i],"--as=",5) == 0)
      LanguageOutputOptions_import(options,0,argv[i]+5);
  }
  for (int i = 1; i < argc; ++i) {
    if (argv[i][0] == '-')
      continue;
    *pfname = argv[i];
    break;
  }
  options->name = "";
  if( *pfname ) {
    char *name = (char*)malloc(strlen(*pfname)+1);
    strcpy(name,*pfname);
    char *lastdot = strrchr(name,'.');
    while( *lastdot ) {
      lastdot[0] = lastdot[1];
      ++lastdot;
    }
    options->name = name;
  }

  // get the lexer name, if not provided
  if (!options->lexerName && *pfname) {
    const char *fname = *pfname;
    char* foutname = (char*)malloc(strlen(fname) + 3);
    strcpy(foutname, fname);
    const char* startfname = strrchr(foutname, '/');
    if ( startfname )
      startfname++;
    if( ! startfname )
      startfname = strrchr(foutname,'\\');
    if (startfname)
      startfname++;
    if( ! startfname )
      startfname = fname;
    char *lexerName = (char*)malloc(strlen(startfname)+4);
    strcpy(lexerName,startfname);
    char* lastdot = strrchr(lexerName, '.');
    if( *lastdot )
      *lastdot = 0;
    int len = strlen(lexerName);
    lexerName[len] = 'L';
    lexerName[len+1] = 0;
    options->lexerName = lexerName;
  }
  *pverbosity = verbosity;
  *ptimed = timed;
}

int parse_main(int argc, char *argv[]) {
  int verbosity = 0;
  int timed = 0;
  const char *fname = 0;
  LanguageOutputOptions options;
  memset(&options,0,sizeof(options));
  options.do_pound_line = true;
  options.outputLanguage = OutputLanguage_C;

  parseArgs(argc, argv, &verbosity, &timed, &options, &fname);

  if (!fname) {
    fputs("usage : parser [-v[v[v]]] [--minnt=int] [--no-pound-line] [--timed] filename\n"
          "   -v          : verbosity, 1 through 3 times\n"
          "--minnt        : minimum nonterminal value, if the default output is too low\n"
          "--lr           : output an LR parser\n"
          "--c            : output C\n"
          "--py           : output python\n"
          "--lexer        : lexer name, if different from the parser (used in python)\n"
          "--import       : module to import (python) or include (C), can be repeated to import multple modules\n"
          "--as           : as clause on module to import (python), applies to most recent import\n"
          "-no-pound-line : turn off #line directives in the output (applies to C)\n", stderr);
    return -1;
  }

  FILE *fin = 0;
  FILE *fout = 0;
  char *foutname = 0;
  FILE *vout = stdout;
  ParserDef parser;
  FileTokBuf tokbuf;
  LRParserSolution solution;
  const ParseError *pe = 0;
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
    Push_Destroy(fin, (vpstack_destroyer)fclose);
    foutname = makeFOutName(fname,&options);
    Push_Destroy(foutname, free);
    ParserDef_init(&parser,vout,verbosity,true);
    FileTokBuf_init(&tokbuf,fin,fname,true);
    LRParserSolution_init(&solution, true);
    ParseParser(&tokbuf.m_tokbuf,&parser,vout,verbosity);
    int n = 0;

    n = LR_SolveParser(&parser, &solution, vout, verbosity, timed);

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
    Push_Destroy(fout, (vpstack_destroyer)fclose);

    OutputLRParserSolution(fout, &parser, &solution, &options);
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

int tok_main(int argc, char *argv[])
{
  const char *prefix = 0;
  bool py = false;
  bool minimal = false;
  for( int i = 1; i < argc; ++i ) {
    if( argv[i][0] == '-' ) {
      if (strncmp(argv[i],"--prefix=",9) == 0) {
        prefix = argv[i] + 9;
      }
      if (strncmp(argv[i],"--minimal",10) == 0) {
        minimal = true;
      }
      if (strncmp(argv[i], "--py", 4) == 0) {
        py = true;
      }
    }
  }
  const char *fname = 0;
  for( int i = 1; i < argc; ++i ) {
    if( argv[i][0] == '-' )
      continue;
    fname = argv[i];
    break;
  }

  if (!fname) {
    fputs("usage : tokenizer [--prefix] [--minimal] [--py] filename\n"
          "--prefix       : prefix for output values\n"
          "--minimal      : skip the structure declaration at the bottom (C only)\n"
          "--py           : output python (default C)\n", stderr);
    return -1;
  }

  FILE *fin = fopen(fname,"r");
  if( !fin ) {
    fprintf(stderr, "Unable to open %s for reading\n", fname);
    return 1;
  }
    
  FILE *fout = 0;
  char *foutname = (char*)malloc(strlen(fname)+3);
  char *name = (char*)malloc(strlen(fname)+3);
  strcpy(name,fname);
  char *lastdot = strrchr(name,'.');
  while( *lastdot ) {
    lastdot[0] = lastdot[1];
    ++lastdot;
  }
  strcpy(foutname,name);
  if( py )
    strcat(foutname,".py");
  else
   strcat(foutname,".h");

  ParseError *pe;
  TokStream s;
  Scope_Push();
  TokStream_init(&s,fin,fname,true);
  int ret = 0;
  Scope_SetJmp(ret);
  if( ! ret ) {
    Nfa *dfa = ParseTokenizerFile(&s);
    fout = fopen(foutname,"w");
    if( ! fout ) {
      fprintf(stderr, "Unable to open %s for writing\n", foutname);
      fclose(fin);
      goto cleanup;
    }
    LanguageOutputOptions options;
    memset(&options,0,sizeof(options));
    options.name = name;
    options.prefix = prefix;
    options.outputLanguage = py ? OutputLanguage_Python : OutputLanguage_C;
    options.minimal = minimal;
    OutputTokenizerSource(fout,dfa,&options);
  } else if( getParseError((const ParseError**)&pe) ) {
    fprintf(stderr, "Parse Error %s(%d:%d) : %s\n", fname, pe->line, pe->col, String_Chars(&pe->err));
    clearParseError();
  } else {
    fputs("Unknown Error\n", stderr);
  }
cleanup:
  if( fin )
    fclose(fin);
  if( fout )
    fclose(fout);
  Scope_Pop();
  return 0;
}

int main(int argc, char *argv[]) {
  if( argc > 1 ) {
    if( strcmp(argv[1],"parser")==0)
      return parse_main(argc-1,argv+1);
    else if( strcmp(argv[1],"tokenizer")==0)
      return tok_main(argc-1,argv+1);
  }
  fputs("usage: parser [parser|tokenizer]\n", stdout);
  return 1;
}
