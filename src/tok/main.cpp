#include "tokenizer.h"
#include <iostream>
#include <fstream>

int main(int argc, char *argv[])
{
  OutputLanguage language = LanguageJs;
  for( int i = 1; i < argc; ++i ) {
    if( argv[i][0] == '-' ) {
      if( strcmp(argv[i],"--test") == 0 ) {
        extern void test();
        test();
        return 0;
      } else if( strcmp(argv[i],"--js") == 0 ) {
        language = LanguageJs;
      } else if( strcmp(argv[i],"--c") == 0 ) {
        language = LanguageC;
      }
    }
  }
  try {
    TokStream s(&cin);
    Nfa *dfa = ParseTokenizerFile(s);
    OutputTokenizerSource(cout,*dfa,language);
  } catch(ParseException pe) {
    cout << "Parse Error <stdin>(" << pe.m_line << ':' << pe.m_col << ") : " << pe.m_err << endl;
  }
  return 0;
}
