#include "tokenizer.h"
#include <string.h>

int main(int argc, char *argv[])
{
  OutputLanguage language = LanguageJs;
  for( int i = 1; i < argc; ++i ) {
    if( argv[i][0] == '-' ) {
      if( strcmp(argv[i],"--js") == 0 ) {
        language = LanguageJs;
      } else if( strcmp(argv[i],"--c") == 0 ) {
        language = LanguageC;
      }
    }
  }
  try {
    TokStream s(stdin);
    Nfa *dfa = ParseTokenizerFile(s);
    OutputTokenizerSource(stdout,*dfa,language);
  } catch(ParseException pe) {
    fprintf(stdout, "Parse Error <stdin>(%d:%d( : %s\n", pe.m_line, pe.m_col, pe.m_err);
  }
  return 0;
}
