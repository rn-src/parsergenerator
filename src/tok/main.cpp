#include "tokenizer.h"
#include <string.h>

int main(int argc, char *argv[])
{
  OutputLanguage language = LanguageC;
  for( int i = 1; i < argc; ++i ) {
    if( argv[i][0] == '-' ) {
      if( strcmp(argv[i],"--js") == 0 ) {
        language = LanguageJs;
      } else if( strcmp(argv[i],"--c") == 0 ) {
        language = LanguageC;
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

    try {
      TokStream s(fin);
      Nfa *dfa = ParseTokenizerFile(s);
      FILE *fout = fopen(foutname,"w");
      if( ! fout ) {
        fprintf(stderr, "Unable to open %s for writing\n", foutname);
        fclose(fin);
        continue;
      }
      OutputTokenizerSource(fout,*dfa,language);
    } catch(ParseException pe) {
      fprintf(stderr, "Parse Error %s(%d:%d) : %s\n", fname, pe.m_line, pe.m_col, pe.m_err.c_str());
    }
    fclose(fin);
    if( fout )
      fclose(fout);
  }
  return 0;
}
