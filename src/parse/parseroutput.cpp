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
  virtual void outFunctionStart(FILE *out, const char *rettype, const char *name) const = 0;
  virtual void outStartParameters(FILE *out) const = 0;
  virtual void outEndParameters(FILE *out) const = 0;
  virtual void outStartFunctionCode(FILE *out) const = 0;
  virtual void outEndFunctionCode(FILE *out) const = 0;
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
  virtual void outFunctionStart(FILE *out, const char *rettype, const char *name) const {
    fputs(rettype, out);
    fputs(" ", out);
    fputs(name, out);
  }
  virtual void outStartParameters(FILE *out) const {
    fputs("(", out);
  }
  virtual void outEndParameters(FILE *out) const {
    fputs(")", out);
  }
  virtual void outStartFunctionCode(FILE *out) const {
    fputs("{", out);
  }
  virtual void outEndFunctionCode(FILE *out) const {
    fputs("}", out);
  }
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
  virtual void outFunctionStart(FILE *out, const char *rettype, const char *name) const {
    fputs("function ", out);
    fputs(name, out);
  }
  virtual void outStartParameters(FILE *out) const {
    fputs("(", out);
  }
  virtual void outEndParameters(FILE *out) const {
    fputs(")", out);
  }
  virtual void outStartFunctionCode(FILE *out) const {
    fputs("{", out);
  }
  virtual void outEndFunctionCode(FILE *out) const {
    fputs("}", out);
  }
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
  Map<int,int> pid2idx;
  Map<int,int> nt2idx;
  Vector<int> pidxtopoffset;
  Vector<int> sidxtoaoffset;
  int poffset = 0;
  int terminals = 0;
  int nonterminals = 0;

  for( Map<int,SymbolDef>::const_iterator curtok = parser.m_tokdefs.begin(), endtok = parser.m_tokdefs.end(); curtok != endtok; ++curtok ) {
    if( curtok->second.m_symboltype == SymbolTypeTerminal )
      ++terminals;
    else if( curtok->second.m_symboltype == SymbolTypeNonterminal ) {
      nt2idx[curtok->second.m_tokid] = nonterminals;
      ++nonterminals;
    }
  }
  int firstnt = terminals+10;
  int firstproduction = terminals+nonterminals+10;
  for( Map<int,SymbolDef>::const_iterator curtok = parser.m_tokdefs.begin(), endtok = parser.m_tokdefs.end(); curtok != endtok; ++curtok ) {
    if( curtok->second.m_symboltype == SymbolTypeNonterminal ) {
      lang.outDecl(out,"const int",curtok->second.m_name.c_str());
      fputs(" = ",out);
      lang.outInt(out,firstnt+nt2idx[curtok->second.m_tokid]);
      lang.outEndStmt(out);
      fputc('\n',out);
    }
  }

  for( int i = 0; i < parser.m_productions.size(); ++i ) {
    char buf[64];
    pid2idx[parser.m_productions[i]->m_pid] = i;
    sprintf(buf,"PROD_%d",i);
    lang.outDecl(out,"const int",buf);
    fputs(" = ",out);
    lang.outInt(out,firstproduction+i);
    lang.outEndStmt(out);
    fputc('\n',out);
  }

#if 0
  lang.outDecl(out,"const int","nproductions");
  fputs(" = ",out);
  lang.outInt(out,parser.m_productions.size());
  lang.outEndStmt(out);
  fputc('\n',out);

  lang.outArrayDecl(out,"static const int", "productions");
  fputs(" = ",out);
  lang.outStartArray(out);
  first = true;
  for( int i = 0; i < parser.m_productions.size(); ++i ) {
    Production *p = parser.m_productions[i];
    pidxtopoffset.push_back(poffset);
    if( first )
      first = false;
    else
      fputc(',',out);
    fputs(parser.m_tokdefs[p->m_nt].m_name.c_str(),out);
    for( int sidx = 0; sidx < p->m_symbols.size(); ++sidx ) {
      fputc(',',out);
      fputs(parser.m_tokdefs[p->m_symbols[sidx]].m_name.c_str(),out);
    }
    poffset += p->m_symbols.size()+1;
  }
  pidxtopoffset.push_back(poffset);
  lang.outEndArray(out);
  lang.outEndStmt(out);
  fputc('\n',out);

  lang.outArrayDecl(out,"static const int", "productionstart");
  fputs(" = ",out);
  lang.outStartArray(out);
  first = true;
  for( int i = 0; i < pidxtopoffset.size(); ++i ) {
    if( first )
      first = false;
    else
      fputc(',',out);
    lang.outInt(out,pidxtopoffset[i]);
  }
  lang.outEndArray(out);
  lang.outEndStmt(out);
  fputc('\n',out);
#endif

  lang.outDecl(out,"const int","nstates");
  fputs(" = ",out);
  lang.outInt(out,solution.m_states.size());
  lang.outEndStmt(out);
  fputc('\n',out);

  lang.outArrayDecl(out,"static const int", "actions");
  fputs(" = ",out);
  lang.outStartArray(out);
  first = true;
  int actionidx = 0;
  for( int i = 0; i < solution.m_states.size(); ++i ) {
    fprintf(out,"\n/* state %d */ ", i);
    sidxtoaoffset.push_back(actionidx);
    shiftmap_t::const_iterator shift = solution.m_shifts.find(i);
    if( shift != solution.m_shifts.end() ) {
      for( shifttosymbols_t::const_iterator curshift = shift->second.begin(), endshift = shift->second.end(); curshift != endshift; ++curshift ) {
        if( curshift->second.size() == 0 )
          continue;
        if( first )
          first = false;
        else
          fputc(',',out);
        // action=shift
        fputs("ACTION_SHIFT",out);
        // shift to
        fputc(',',out);
        lang.outInt(out,curshift->first);
        // symbols
        fputc(',',out);
        lang.outInt(out,curshift->second.size());
        for( Set<int>::const_iterator curs = curshift->second.begin(), ends = curshift->second.end(); curs != ends; ++curs) {
          int s = *curs;
          fputc(',',out);
          if( parser.getSymbolType(s) == SymbolTypeTerminal )
            fputs(parser.m_tokdefs[s].m_name.c_str(),out);
          else
            fprintf(out,"PROD_%d",pid2idx[s]);
        }
        actionidx += curshift->second.size()+3;
      }
    }
    reducemap_t::const_iterator reduce = solution.m_reductions.find(i);
    if( reduce != solution.m_reductions.end() ) {
      for( reducebysymbols_t::const_iterator curreduce = reduce->second.begin(), endreduce = reduce->second.end(); curreduce != endreduce; ++curreduce) {
        if( curreduce->second.size() == 0 )
          continue;
        if( first )
          first = false;
        else
          fputc(',',out);
        // action=reduce
        if( curreduce->first->m_nt == parser.getStartNt() )
          fputs("ACTION_STOP",out);
        else
          fputs("ACTION_REDUCE",out);
        // reduce by
        fputc(',',out);
        fprintf(out,"PROD_%d",pid2idx[curreduce->first->m_pid]);
        // reduce count
        fputc(',',out);
        lang.outInt(out,curreduce->first->m_symbols.size());
        // symbols
        fputc(',',out);
        lang.outInt(out,curreduce->second.size());
        for( Set<int>::const_iterator curs = curreduce->second.begin(), ends = curreduce->second.end(); curs != ends; ++curs ) {
          int s = *curs;
          fputc(',',out);
          if( s == -1 )
            lang.outInt(out,-1);
          else if( parser.getSymbolType(s) == SymbolTypeTerminal )
            fputs(parser.m_tokdefs[s].m_name.c_str(),out);
          else
            fprintf(out,"PROD_%d",pid2idx[s]);
        }
        actionidx += curreduce->second.size()+4;
      }
    }
  }
  sidxtoaoffset.push_back(actionidx);

  lang.outEndArray(out);
  lang.outEndStmt(out);
  fputc('\n',out);

  lang.outArrayDecl(out,"static const int", "actionstart");
  fputs(" = ",out);
  lang.outStartArray(out);
  first = true;
  for( int i = 0; i < sidxtoaoffset.size(); ++i ) {
    if( first )
      first = false;
    else
      fputc(',',out);
    lang.outInt(out,sidxtoaoffset[i]);
  }
  lang.outEndArray(out);
  lang.outEndStmt(out);
  fputc('\n',out);

  fputs("struct stack_t {\n", out);
  lang.outDecl(out,"String","str");
  fputs(";\n",out);
  for( Map<int,SymbolDef>::const_iterator cursymbol = parser.m_tokdefs.begin(), endsymbol = parser.m_tokdefs.end(); cursymbol != endsymbol; ++cursymbol ) {
    const SymbolDef &s = cursymbol->second;
    if( ! s.m_semantictype.length() )
      continue;
    String t = String("t")+s.m_semantictype;
    lang.outDecl(out,s.m_semantictype.c_str(),t.c_str());
    fputs(";\n",out);
  }
  fputs("};\n",out);

  lang.outFunctionStart(out,"static bool", "reduce");
  lang.outStartParameters(out);
  lang.outDecl(out, "int", "productionidx");
  fputs(", ", out);
  lang.outDecl(out, "stack_t*", "inputs");
  fputs(", ", out);
  lang.outDecl(out, "stack_t&", "output");
  lang.outEndParameters(out);
  lang.outStartFunctionCode(out);
  fputc('\n',out);
  fputs("switch(productionidx) {\n", out);
  for( Vector<Production*>::const_iterator curp = parser.m_productions.begin(), endp = parser.m_productions.end(); curp != endp; ++curp ) {
    const Production *p = *curp;
    if( p->m_action.size() == 0 )
      continue;
    fprintf(out,"case PROD_%d:", pid2idx[p->m_pid]);
    for( Vector<ActionItem>::const_iterator curaction = p->m_action.begin(), endaction = p->m_action.end(); curaction != endaction; ++curaction ) {
      const ActionItem &item = *curaction;
      if( item.m_actiontype == ActionTypeSrc )
        fputs(item.m_str.c_str(), out);
      else if( item.m_actiontype == ActionTypeDollarDollar ) {
        const String &stype = parser.m_tokdefs[p->m_nt].m_semantictype;
        fprintf(out,"output.t%s",stype.c_str());
      } else if( item.m_actiontype == ActionTypeDollarNumber ) {
        SymbolType stype = parser.getSymbolType(p->m_symbols[item.m_dollarnum-1]);
        if( stype == SymbolTypeTerminal )
          fprintf(out, "inputs[%d].str",item.m_dollarnum-1);
        else if( stype == SymbolTypeNonterminal ) {
          const String &stype = parser.m_tokdefs[p->m_symbols[item.m_dollarnum-1]].m_semantictype;
          fprintf(out, "inputs[%d].t%s",item.m_dollarnum-1,stype.c_str());
        }
      }
    }
    fputs("\nbreak;\n", out);
  }
  fputs("default:\n", out);
  fputs("  break;\n", out);
  fputs("} /* end switch */\n", out);
  fputs("return true",out);
  lang.outEndStmt(out);
  fputc('\n',out);
  lang.outEndFunctionCode(out);
}

void OutputParserSolution(FILE *out, const ParserDef &parser, const ParserSolution &solution, OutputLanguage language) {
  LanguageOutputter *outputer = getLanguageOutputter(language);
  OutputParser(out,parser,solution,*outputer);
}
