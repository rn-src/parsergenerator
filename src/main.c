#include "parser.h"
#include "tokenizer.h"
#include <stdio.h>
#include <string.h>

static void *memalloc(size_t size) {
  return malloc(size);
}

static const char *getarg(int argc, char *argv[], const char *arg) {
  for( int i = 1; i < argc; ++i )
    if( strncmp(argv[i],arg,strlen(arg)) == 0 )
      return argv[i];
  return 0;
}

static bool getboolarg(int argc, char *argv[], const char *arg, bool def) {
  size_t len = strlen(arg);
  const char *v = getarg(argc,argv,arg);
  if( ! v )
    return def;
  if( v[len] == '=' ) {
    v += len+1;
    if( *v == 't' || *v == 'T' || *v == 'y' || *v == 'Y' || *v == '1' )
      return true;
    return false;
  }
  if( v[len] == 0 )
    return true;
  return def;
}

static char *makeFOutName(const char *fname, LanguageOutputOptions *options) {
  char *foutname = (char*)memalloc(strlen(fname) + 3);
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

static void parseArgs(int argc, char *argv[], int *pverbosity, int *ptimed, LanguageOutputOptions *options, const char **pfname) {
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
  arg = getarg(argc, argv, "--no-pound-line");
  if(arg)
    options->do_pound_line = false;
  if (getarg(argc, argv, "--timed"))
    timed = 1;
  if (getarg(argc, argv, "--py")) {
    options->outputLanguage = OutputLanguage_Python;
    options->encode = false;
    options->compress = false;
  }
  options->encode = getboolarg(argc,argv,"--encode", options->encode);
  options->compress = getboolarg(argc,argv,"--compress", options->compress);
  options->show_uncompressed_data = getboolarg(argc,argv,"--show-uncompressed", options->show_uncompressed_data);
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
    char *name = (char*)memalloc(strlen(*pfname)+1);
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
    char* foutname = (char*)memalloc(strlen(fname) + 3);
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
    char *lexerName = (char*)memalloc(strlen(startfname)+4);
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

static int parse_main(int argc, char *argv[]) {
  int verbosity = 0;
  int timed = 0;
  const char *fname = 0;
  LanguageOutputOptions options;
  memset(&options,0,sizeof(options));
  options.do_pound_line = true;
  options.outputLanguage = OutputLanguage_C;
  options.encode = true;
  options.compress = true;

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
          "-no-pound-line : turn off #line directives in the output (applies to C)\n"
          "--encode       : set to true/false, emit values 8bit encoded instead of full integers\n"
          "                 default true for C, false for Python.\n"
          "                 presently only C parser implements.\n"
          "--compress     : set to true/false, allow emitting compressed state data\n"
          "                 will fall-back to 8-bit encoded output if space is not saved\n"
          "                 default true for C, false for Python\n"
          "                 only C parser implements.\n"
          "--show-uncompressed\n"
          "               : output uncompressed data as a line comment\n"
          , stderr);
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
      ret = -1;
      goto end;
    }
    Push_Destroy(fin, (vpstack_destroyer)fclose);
    foutname = makeFOutName(fname,&options);
    Push_Destroy(foutname, free);
    ParserDef_init(&parser,vout,verbosity,true);
    FileTokBuf_init(&tokbuf,fin,fname,true);
    LRParserSolution_init(&solution, true);
    ParseParser(&tokbuf.m_tokbuf,&parser,vout,verbosity);
    ret = LR_SolveParser(&parser, &solution, vout, verbosity, timed);
    if( ret != 0 )
      goto end;
    fout = fopen(foutname,"w");
    if( !fout ) {
      fprintf(stderr, "Unable to open %s for writing\n", foutname);
      ret = -1;


      goto end;
    }
    Push_Destroy(fout, (vpstack_destroyer)fclose);

    OutputLRParserSolution(fout, &parser, &solution, &options);
  } else if( getParseError(&pe) ) {
    if( pe->line != -1 )
      fprintf(stderr, "%s(%d:%d) : %s\n", String_Chars(&pe->file), pe->line, pe->col, String_Chars(&pe->err));
    else
      fputs(String_Chars(&pe->err),stderr);
    ret = -1;
  } else {
    fputs("Unknown error\n", stderr);
    ret = -1;
  }
end:
  Scope_Pop();
  return ret;
}

static int tok_main(int argc, char *argv[])
{
  LanguageOutputOptions options;
  bool verbose = false;
  memset(&options,0,sizeof(options));
  options.outputLanguage = OutputLanguage_C;
  options.encode = true;
  options.compress = true;

  const char *arg = 0;
  if(getarg(argc,argv,"-v"))
    verbose = true;
  if(getarg(argc,argv,"--py")) {
    options.outputLanguage = OutputLanguage_Python;
    options.encode = false;
    options.compress = false;
  }
  options.encode = getboolarg(argc,argv,"--encode", options.encode);
  options.compress = getboolarg(argc,argv,"--compress", options.compress);
  options.show_uncompressed_data = getboolarg(argc,argv,"--show-uncompressed", options.show_uncompressed_data);
  const char *fname = 0;
  for( int i = 1; i < argc; ++i ) {
    if( argv[i][0] == '-' )
      continue;
    fname = argv[i];
    break;
  }

  if (!fname) {
    fputs("usage : tokenizer [--py] filename\n"
          "-v             : verbose\n"
          "--py           : output python (default C)\n"
          "--encode       : set to true/false, emit values 8bit encoded instead of full integers\n"
          "                 default true for C, false for Python.\n"
          "                 presently only C tokenizer implements.\n"
          "--compress     : set to true/false, allow emitting compressed state data\n"
          "                 will fall-back to 8-bit encoded output if space is not saved\n"
          "                 default true for C, false for Python\n"
          "                 only C tokenizer implements.\n"
          , stderr);
    return -1;
  }

  FILE *fin = fopen(fname,"r");
  if( !fin ) {
    fprintf(stderr, "Unable to open %s for reading\n", fname);
    return 1;
  }
  Scope_Push();
  Push_Destroy(fin, (vpstack_destroyer)fclose);
  FILE *fout = 0;
  char *foutname = (char*)memalloc(strlen(fname)+3);
  char *name = (char*)memalloc(strlen(fname)+3);
  strcpy(name,fname);
  char *lastdot = strrchr(name,'.');
  while( *lastdot ) {
    lastdot[0] = lastdot[1];
    ++lastdot;
  }
  options.name = name;
  strcpy(foutname,name);
  if( options.outputLanguage == OutputLanguage_Python )
    strcat(foutname,".py");
  else
   strcat(foutname,".h");

  ParseError *pe = 0;
  TokStream s;
  TokStream_init(&s,fin,fname,true);
  int ret = 0;
  Scope_SetJmp(ret);
  if( ! ret ) {
    Nfa *dfa = ParseTokenizerFile(&s,verbose);
    fout = fopen(foutname,"w");
    if( ! fout ) {
      fprintf(stderr, "Unable to open %s for writing\n", foutname);
      goto cleanup;
    }
    Push_Destroy(fout, (vpstack_destroyer)fclose);
    OutputTokenizerSource(fout,dfa,&options);
  } else if( getParseError((const ParseError**)&pe) ) {
    fprintf(stderr, "Parse Error %s(%d:%d) : %s\n", fname, pe->line, pe->col, String_Chars(&pe->err));
    clearParseError();
  } else {
    fputs("Unknown Error\n", stderr);
  }
cleanup:
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
