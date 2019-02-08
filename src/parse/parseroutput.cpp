#include "parser.h"
#include <ctype.h>

namespace pptok {
#include "parsertok.h"
}

class LanguageOutputter {
public:
  virtual ~LanguageOutputter() {}
  virtual void outDecl(FILE *out, const char *type, const char *name) const = 0;
  virtual void outArrayDecl(FILE *out, const char *type, const char *name) const = 0;
  virtual void outStartArray(FILE *out) const = 0;
  virtual void outEndArray(FILE *out) const = 0;
  virtual void outEndStmt(FILE *out) const = 0;
  virtual void outNull(FILE *out) const = 0;
  virtual void outBool(FILE *out, bool b) const = 0;
  virtual void outStr(FILE *out, const char *str) const = 0;
  virtual void outChar(FILE *out, int c) const = 0;
  virtual void outInt(FILE *out, int i) const = 0;
};

class CLanguageOutputter : public LanguageOutputter {
public:
  virtual void outDecl(FILE *out, const char *type, const char *name) const { fputs(type,out); fputc(' ',out); fputs(name,out); }
  virtual void outArrayDecl(FILE *out, const char *type, const char *name) const { fputs(type,out); fputc(' ',out); fputs(name,out); fputs("[]",out); }
  virtual void outStartArray(FILE *out) const { fputc('{',out); }
  virtual void outEndArray(FILE *out) const  { fputc('}',out); }
  virtual void outEndStmt(FILE *out) const  { fputc(';',out); }
  virtual void outNull(FILE *out) const  { fputc('0',out); }
  virtual void outBool(FILE *out, bool b) const  { fputs((b ? "true" : "false"),out); }
  virtual void outStr(FILE *out, const char *str) const  {
    fputc('"',out); fputs(str,out); fputc('"',out);
  }
  virtual void outChar(FILE *out, int c) const  {
    if(c == '\r' ) {
      fputs("'\\r'",out);
    } else if(c == '\n' ) {
      fputs("'\\n'",out);
    } else if(c == '\v' ) {
      fputs("'\\v'",out);
    } else if(c == ' ' ) {
      fputs("' '",out);
    } else if(c == '\t' ) {
      fputs("'\\t'",out);
    } else if(c == '\\' || c == '\'' ) {
      fprintf(out,"'\\%c'",c);
    } else if( c <= 126 && isgraph(c) ) {
      fprintf(out,"'%c'",c);
    } else
      fprintf(out,"%d",c);
  }
  virtual void outInt(FILE *out, int i) const  { fprintf(out,"%d",i); }
};

class JsLanguageOutputter : public LanguageOutputter {
public:
  virtual void outDecl(FILE *out, const char *type, const char *name) const { fputs("var ",out); fputs(name,out); }
  virtual void outArrayDecl(FILE *out, const char *type, const char *name) const { fputs("var ",out); fputs(name,out); }
  virtual void outStartArray(FILE *out) const { fputc('[',out); }
  virtual void outEndArray(FILE *out) const { fputc(']',out); }
  virtual void outEndStmt(FILE *out)  const { fputc(';',out); }
  virtual void outNull(FILE *out) const  { fputs("null",out); }
  virtual void outBool(FILE *out, bool b) const  { fputs((b ? "true" : "false"),out); }
  virtual void outStr(FILE *out, const char *str) const  { fputc('"',out); fputs(str,out); fputc('"',out); }
  virtual void outChar(FILE *out, int c) const  { fprintf(out,"%d",c); }
  virtual void outInt(FILE *out, int i) const  { fprintf(out,"%d",i); }
};

LanguageOutputter *getLanguageOutputter(OutputLanguage language) {
  if( language == LanguageC )
    return new CLanguageOutputter();
  else if( language == LanguageJs )
    return new  JsLanguageOutputter();
  return 0;
}

static void OutputParser(FILE *out, const ParserDef &parser, const ParserSolution &solution, const LanguageOutputter &lang) {
  bool first = true;
  int i = 0;
  Vector<int> shiftOffsets, reduceOffsets;
  int symbolOffset = 0;

  first = true;
  lang.outArrayDecl(out,"static const int", "productionsizes");
  for( Vector<Production*>::const_iterator cur = parser.m_productions.begin(), end = parser.m_productions.end(); cur != end; ++cur ) {
    if( first )
      first = false;
    else
      fputc(',',out);
    lang.outInt(out,(*cur)->m_symbols.size());
  }
  lang.outEndArray(out);
  fputc('\n',out);

  first = true;
  lang.outArrayDecl(out,"static const int", "symbols");
  for( Map< Pair<int,int>,Set<int> >::const_iterator cur = solution.m_shifts.begin(), end = solution.m_shifts.end(); cur != end; ++cur ) {
    for( Set<int>::const_iterator cursymbol = cur->second.begin(), endsymbol = cur->second.end(); cursymbol != endsymbol; ++cursymbol ) {
      if( first )
        first = false;
      else
        fputc(',',out);
      lang.outInt(out,*cursymbol);
    }
    shiftOffsets.push_back(symbolOffset);
    symbolOffset += cur->second.size();
  }
  for( Map< Pair<int,int>,Set<int> >::const_iterator cur = solution.m_reductions.begin(), end = solution.m_reductions.end(); cur != end; ++cur ) {
    for( Set<int>::const_iterator cursymbol = cur->second.begin(), endsymbol = cur->second.end(); cursymbol != endsymbol; ++cursymbol ) {
      if( first )
        first = false;
      else
        fputc(',',out);
      lang.outInt(out,*cursymbol);
    }
    reduceOffsets.push_back(symbolOffset);
    symbolOffset += cur->second.size();
  }
  lang.outEndArray(out);
  fputc('\n',out);

  first = true;
  i = 0;
  lang.outArrayDecl(out,"static const int", "shifts");
  for( Map< Pair<int,int>,Set<int> >::const_iterator cur = solution.m_shifts.begin(), end = solution.m_shifts.end(); cur != end; ++cur ) {
    if( first )
      first = false;
    else
      fputc(',',out);
    fputc('\n',out);
    lang.outInt(out,cur->first.first);
    fputc(',',out);
    lang.outInt(out,cur->first.second);
    fputc(',',out);
    lang.outInt(out,shiftOffsets[i++]);
  }
  lang.outEndArray(out);
  fputc('\n',out);

  first = true;
  i = 0;
  lang.outArrayDecl(out,"static const int", "reductions");
  for( Map< Pair<int,int>,Set<int> >::const_iterator cur = solution.m_reductions.begin(), end = solution.m_reductions.end(); cur != end; ++cur ) {
    if( first )
      first = false;
    else
      fputc(',',out);
    fputc('\n',out);
    lang.outInt(out,cur->first.first);
    fputc(',',out);
    lang.outInt(out,cur->first.second);
    fputc(',',out);
    lang.outInt(out,reduceOffsets[i++]);
  }
  lang.outEndArray(out);
  fputc('\n',out);
}

void OutputParserSolution(FILE *out, const ParserDef &parser, const ParserSolution &solution, OutputLanguage language) {
  LanguageOutputter *outputer = getLanguageOutputter(language);
  OutputParser(out,parser,solution,*outputer);
}
