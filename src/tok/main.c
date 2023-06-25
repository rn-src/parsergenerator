#include "tokenizer.h"
#include <string.h>

int main(int argc, char *argv[])
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
      OutputTokenizerSource(fout,dfa,name,prefix,py,minimal);
    } else if( getParseError((const ParseError**)&pe) ) {
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
