#include "tokenizer.h"
#include <string.h>

int main(int argc, char *argv[])
{
  const char *prefix = "prefix";
  for( int i = 1; i < argc; ++i ) {
    if( argv[i][0] == '-' ) {
      if (strncmp(argv[i],"--prefix=",9) == 0) {
        prefix = argv[i] + 9;
      }
    }
  }
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

    ParseError *pe;
    TokStream s;
    Scope_Push();
    TokStream_init(&s,fin,true);
    int ret = 0;
    Scope_SetJmp(ret);
    if( ! ret ) {
      Nfa *dfa = ParseTokenizerFile(&s);
      fout = fopen(foutname,"w");
      if( ! fout ) {
        fprintf(stderr, "Unable to open %s for writing\n", foutname);
        fclose(fin);
        continue;
      }
      OutputTokenizerSource(fout,dfa,prefix);
    } else if( getParseError(&pe) ) {
      fprintf(stderr, "Parse Error %s(%d:%d) : %s\n", fname, pe->line, pe->col, String_Chars(&pe->err));
      clearParseError();
    } else {
      fputs("Unknown Error\n", stderr);
    }
    fclose(fin);
    if( fout )
      fclose(fout);
    Scope_Pop();
  }
  return 0;
}
