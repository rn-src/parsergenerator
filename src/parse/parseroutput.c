#include "parser.h"
#include <ctype.h>
#include "parsertokL.h"

extern void *zalloc(size_t len);

struct LanguageOutputter;
typedef struct LanguageOutputter LanguageOutputter;

struct LanguageOutputter {
  void (*outDecl)(const LanguageOutputter *This, FILE *out, const char *type, const char *name);
  void (*outArrayDecl)(const LanguageOutputter *This, FILE *out, const char *type, const char *name);
  void (*outStartArray)(const LanguageOutputter *This, FILE *out);
  void (*outEndArray)(const LanguageOutputter *This, FILE *out);
  void (*outEndStmt)(const LanguageOutputter *This, FILE *out);
  void (*outNull)(const LanguageOutputter *This, FILE *out);
  void (*outBool)(const LanguageOutputter *This, FILE *out, bool b);
  void (*outStr)(const LanguageOutputter *This, FILE *out, const char *str);
  void (*outChar)(const LanguageOutputter *This, FILE *out, int c);
  void (*outInt)(const LanguageOutputter *This, FILE *out, int i);
  void (*outFunctionStart)(const LanguageOutputter *This, FILE *out, const char *rettype, const char *name);
  void (*outStartParameters)(const LanguageOutputter *This, FILE *out);
  void (*outEndParameters)(const LanguageOutputter *This, FILE *out);
  void (*outStartFunctionCode)(const LanguageOutputter *This, FILE *out);
  void (*outEndFunctionCode)(const LanguageOutputter *This, FILE *out);
};

void CLanguageOutputter_outDecl(const LanguageOutputter *This, FILE *out, const char *type, const char *name) { fputs(type,out); fputc(' ',out); fputs(name,out); }
void CLanguageOutputter_outArrayDecl(const LanguageOutputter *This, FILE *out, const char *type, const char *name) { fputs(type,out); fputc(' ',out); fputs(name,out); fputs("[]",out); }
void CLanguageOutputter_outStartArray(const LanguageOutputter *This, FILE *out) { fputc('{',out); }
void CLanguageOutputter_outEndArray(const LanguageOutputter *This, FILE *out) { fputc('}',out); }
void CLanguageOutputter_outEndStmt(const LanguageOutputter *This, FILE *out) { fputc(';',out); }
void CLanguageOutputter_outNull(const LanguageOutputter *This, FILE *out) { fputc('0',out); }
void CLanguageOutputter_outBool(const LanguageOutputter *This, FILE *out, bool b) { fputs((b ? "true" : "false"),out); }
void CLanguageOutputter_outStr(const LanguageOutputter *This, FILE *out, const char *str)  { fputc('"',out); fputs(str,out); fputc('"',out); }
void CLanguageOutputter_outChar(const LanguageOutputter *This, FILE *out, int c)  {
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
void CLanguageOutputter_outInt(const LanguageOutputter *This, FILE *out, int i)  { fprintf(out,"%d",i); }
void CLanguageOutputter_outFunctionStart(const LanguageOutputter *This, FILE *out, const char *rettype, const char *name) {
    fputs("function ", out);
    fputs(name, out);
}
void CLanguageOutputter_outStartParameters(const LanguageOutputter *This, FILE *out) {
  fputs("(", out);
}
void CLanguageOutputter_outEndParameters(const LanguageOutputter *This, FILE *out) {
  fputs(")", out);
}
void CLanguageOutputter_outStartFunctionCode(const LanguageOutputter *This, FILE *out) {
  fputs("{", out);
}
void CLanguageOutputter_outEndFunctionCode(const LanguageOutputter *This, FILE *out) {
  fputs("}", out);
}
LanguageOutputter CLanguageOutputter = {CLanguageOutputter_outDecl, CLanguageOutputter_outArrayDecl, CLanguageOutputter_outStartArray, CLanguageOutputter_outEndArray, CLanguageOutputter_outEndStmt, CLanguageOutputter_outNull, CLanguageOutputter_outBool, CLanguageOutputter_outStr, CLanguageOutputter_outChar, CLanguageOutputter_outInt, CLanguageOutputter_outFunctionStart, CLanguageOutputter_outStartParameters, CLanguageOutputter_outEndParameters, CLanguageOutputter_outStartFunctionCode, CLanguageOutputter_outEndFunctionCode};

static void OutputLRParser(FILE *out, const ParserDef *parser, const LRParserSolution *solution, const LanguageOutputter *lang, const LanguageOutputOptions *outputOptions) {
  bool first = true;
  MapAny /*<int,int>*/ pid2idx;
  MapAny /*<int,int>*/ nt2idx;
  VectorAny /*<int>*/ pidxtopoffset;
  VectorAny /*<int>*/ sidxtoaoffset;
  int poffset = 0;
  int terminals = 0;
  int nonterminals = 0;

  Scope_Push();
  MapAny_init(&pid2idx,getIntElement(),getIntElement(),true);
  MapAny_init(&nt2idx,getIntElement(),getIntElement(),true);
  VectorAny_init(&pidxtopoffset,getIntElement(),true);
  VectorAny_init(&sidxtoaoffset,getIntElement(),true);

  for( int curtok = 0, endtok = MapAny_size(&parser->m_tokdefs); curtok != endtok; ++curtok ) {
    const int *tokid = 0;
    const SymbolDef *tok = 0;
    MapAny_getByIndexConst(&parser->m_tokdefs,curtok,&tokid,&tok);
    if( tok->m_symboltype == SymbolTypeTerminal )
      ++terminals;
    else if( tok->m_symboltype == SymbolTypeNonterminal ) {
      MapAny_insert(&nt2idx,&tok->m_tokid,&nonterminals);
      ++nonterminals;
    }
  }
  int firstnt = outputOptions->min_nt_value>(terminals+1)?outputOptions->min_nt_value:(terminals+1);
  int firstproduction = firstnt+nonterminals;
  for( int curtok = 0, endtok = MapAny_size(&parser->m_tokdefs); curtok != endtok; ++curtok ) {
    const int *tokid = 0;
    const SymbolDef *tok = 0;
    MapAny_getByIndexConst(&parser->m_tokdefs,curtok,&tokid,&tok);
    if( tok->m_symboltype == SymbolTypeNonterminal ) {
      lang->outDecl(lang,out,"const int",String_Chars(&tok->m_name));
      fputs(" = ",out);
      lang->outInt(lang,out,firstnt+MapAny_findT(&nt2idx,&tok->m_tokid,int));
      lang->outEndStmt(lang,out);
      fputc('\n',out);
    }
  }

  for( int i = 0; i < VectorAny_size(&parser->m_productions); ++i ) {
    char buf[64];
    MapAny_insert(&pid2idx,&VectorAny_ArrayOpConstT(&parser->m_productions,i,Production*)->m_pid,&i);
    sprintf(buf,"PROD_%d",i);
    lang->outDecl(lang,out,"const int",buf);
    fputs(" = ",out);
    lang->outInt(lang,out,firstproduction+i);
    lang->outEndStmt(lang,out);
    fputc('\n',out);
  }

  lang->outDecl(lang,out,"const int","nstates");
  fputs(" = ",out);
  lang->outInt(lang,out,VectorAny_size(&solution->m_states));
  lang->outEndStmt(lang,out);
  fputc('\n',out);

  lang->outArrayDecl(lang,out,"static const int", "actions");
  fputs(" = ",out);
  lang->outStartArray(lang,out);
  first = true;
  int actionidx = 0;
  for( int i = 0; i < VectorAny_size(&solution->m_states); ++i ) {
    fprintf(out,"\n/* state %d */ ", i);
    VectorAny_push_back(&sidxtoaoffset,&actionidx);
    const shifttosymbols_t *shifttosymbols = &MapAny_findConstT(&solution->m_shifts,&i,shifttosymbols_t);
    if( shifttosymbols ) {
      for( int curshift = 0, endshift = MapAny_size(shifttosymbols); curshift != endshift; ++curshift ) {
        const int *tokid = 0;
        const SetAny /*<int>*/ *curShiftSymbols = 0;
        MapAny_getByIndexConst(shifttosymbols,curshift,&tokid,&curShiftSymbols);
        if( SetAny_size(curShiftSymbols) == 0 )
          continue;
        if( first )
          first = false;
        else
          fputc(',',out);
        // action=shift
        fputs("ACTION_SHIFT",out);
        // shift to
        fputc(',',out);
        lang->outInt(lang,out,*tokid);
        // symbols
        fputc(',',out);
        lang->outInt(lang,out,SetAny_size(curShiftSymbols));
        for( int curs = 0, ends = SetAny_size(curShiftSymbols); curs != ends; ++curs) {
          int s = SetAny_getByIndexConstT(curShiftSymbols,curs,int);
          fputc(',',out);
          if( ParserDef_getSymbolType(parser,s) == SymbolTypeTerminal )
            fputs(String_Chars(&MapAny_findConstT(&parser->m_tokdefs,&s,SymbolDef).m_name),out);
          else
            fprintf(out,"PROD_%d",MapAny_findT(&pid2idx,&s,int));
        }
        actionidx += SetAny_size(curShiftSymbols)+3;
      }
    }
    // typedef MapAny /*<Production*,Set<int> >*/ reducebysymbols_t;
    // typedef MapAny /*<int,reducebysymbols_t>*/ reducemap_t;
    const reducebysymbols_t *reducebysymbols = &MapAny_findConstT(&solution->m_reductions,&i,reducebysymbols_t);
    if( reducebysymbols ) {
      VectorAny /*<Production*>*/ reduces;
      Scope_Push();
      VectorAny_init(&reduces,getPointerElement(),true);
      for( int curreduce = 0, endreduce = MapAny_size(reducebysymbols); curreduce != endreduce; ++curreduce ) {
        const Production **reduceby = 0;
        const SetAny /*<int>*/ *reducesymbols = 0;
        MapAny_getByIndexConst(reducebysymbols,curreduce,(const void**)&reduceby,&reducesymbols);
        VectorAny_push_back(&reduces,reduceby);
      }
      if( VectorAny_size(&reduces) > 1 )
        qsort(VectorAny_ptr(&reduces),VectorAny_size(&reduces),sizeof(Production*),Production_CompareId);
      for( int curp = 0, endp = VectorAny_size(&reduces); curp != endp; ++curp ) {
        Production *p = VectorAny_ArrayOpT(&reduces,curp,Production*);
        const SetAny /*<int>*/ *syms = &MapAny_findConstT(reducebysymbols,&p,SetAny);
        if( SetAny_size(syms) == 0 )
          continue;
        if( first )
          first = false;
        else
          fputc(',',out);
        // action=reduce
        if( p->m_nt == ParserDef_getStartNt(parser) )
          fputs("ACTION_STOP",out);
        else
          fputs("ACTION_REDUCE",out);
        // reduce by
        fputc(',',out);
        fprintf(out,"PROD_%d",MapAny_findT(&pid2idx,&p->m_pid,int));
        // reduce count
        fputc(',',out);
        lang->outInt(lang,out,VectorAny_size(&p->m_symbols));
        // symbols
        fputc(',',out);
        lang->outInt(lang,out,SetAny_size(syms));
        for( int curs = 0, ends = SetAny_size(syms); curs != ends; ++curs ) {
          int s = SetAny_getByIndexConstT(syms,curs,int);
          fputc(',',out);
          if( s == -1 )
            lang->outInt(lang,out,-1);
          else if( ParserDef_getSymbolType(parser,s) == SymbolTypeTerminal )
            fputs(String_Chars(&MapAny_findConstT(&parser->m_tokdefs,&s,SymbolDef).m_name),out);
          else
            fprintf(out,"PROD_%d",MapAny_findT(&pid2idx,&s,int));
        }
        actionidx += SetAny_size(syms)+4;
      }
      Scope_Pop();
    }
  }
  VectorAny_push_back(&sidxtoaoffset,&actionidx);

  lang->outEndArray(lang,out);
  lang->outEndStmt(lang,out);
  fputc('\n',out);

  lang->outArrayDecl(lang,out,"static const int", "actionstart");
  fputs(" = ",out);
  lang->outStartArray(lang,out);
  first = true;
  for( int i = 0; i < VectorAny_size(&sidxtoaoffset); ++i ) {
    if( first )
      first = false;
    else
      fputc(',',out);
    lang->outInt(lang,out,VectorAny_ArrayOpT(&sidxtoaoffset,i,int));
  }
  lang->outEndArray(lang,out);
  lang->outEndStmt(lang,out);
  fputc('\n',out);

  lang->outDecl(lang,out,"const int","nproductions");
  fputs(" = ",out);
  lang->outInt(lang,out,VectorAny_size(&parser->m_productions));
  lang->outEndStmt(lang,out);
  fputc('\n',out);


  lang->outArrayDecl(lang,out,"static const int", "productions");
  fputs(" = ",out);
  lang->outStartArray(lang,out);
  first = true;
  for( int i = 0; i < VectorAny_size(&parser->m_productions); ++i ) {
    const Production *p = VectorAny_ArrayOpConstT(&parser->m_productions,i,Production*);
    VectorAny_push_back(&pidxtopoffset,&poffset);
    if( first )
      first = false;
    else
      fputc(',',out);
    fputs(String_Chars(&MapAny_findConstT(&parser->m_tokdefs,&p->m_nt,SymbolDef).m_name),out);
    for( int sidx = 0; sidx < VectorAny_size(&p->m_symbols); ++sidx ) {
      fputc(',',out);
      fputs(String_Chars(&MapAny_findConstT(&parser->m_tokdefs,&VectorAny_ArrayOpConstT(&p->m_symbols,sidx,int),SymbolDef).m_name),out);
    }
    poffset += VectorAny_size(&p->m_symbols)+1;
  }
  VectorAny_push_back(&pidxtopoffset,&poffset);
  lang->outEndArray(lang,out);
  lang->outEndStmt(lang,out);
  fputc('\n',out);
  lang->outArrayDecl(lang,out,"static const int", "productionstart");
  fputs(" = ",out);
  lang->outStartArray(lang,out);
  first = true;
  for( int i = 0; i < VectorAny_size(&pidxtopoffset); ++i ) {
    if( first )
      first = false;
    else
      fputc(',',out);
    lang->outInt(lang,out,VectorAny_ArrayOpT(&pidxtopoffset,i,int));
  }
  lang->outEndArray(lang,out);
  lang->outEndStmt(lang,out);
  fputc('\n',out);

  lang->outArrayDecl(lang,out,"static const char*", "nonterminals");
  fputs(" = ",out);
  lang->outStartArray(lang,out);
  first = true;
  for( int curtok = 0, endtok = MapAny_size(&parser->m_tokdefs); curtok != endtok; ++curtok ) {
    const int *tokid = 0;
    const SymbolDef *tokdef = 0;
    MapAny_getByIndexConst(&parser->m_tokdefs,curtok,&tokid,&tokdef);
    if( tokdef->m_symboltype != SymbolTypeNonterminal )
      continue;
    if( first )
      first = false;
    else
      fputc(',',out);
    lang->outStr(lang,out,String_Chars(&tokdef->m_name));
  }
  lang->outEndArray(lang,out);
  lang->outEndStmt(lang,out);
  fputc('\n',out);
  int extraNt = ParserDef_getExtraNt(parser);
  if( ! MapAny_findConst(&parser->m_tokdefs,&extraNt) )
    fputs("typedef int extra_t;\n", out);
  else
    fprintf(out, "typedef %s extra_t;\n", String_Chars(&MapAny_findConstT(&parser->m_tokdefs,&extraNt,SymbolDef).m_semantictype));

  MapAny /*<String,String>*/ tfields;
  String token_str;
  String tok_str;
  MapAny_init(&tfields,getStringElement(),getStringElement(),true);
  String_init(&token_str,true);
  String_AssignChars(&token_str,"Token");
  String_init(&tok_str,true);
  String_AssignChars(&tok_str,"tok");
  MapAny_insert(&tfields,&token_str,&tok_str);
  fputs("struct stack_t {\nstack_t() {}\n", out);
  lang->outDecl(lang,out,"Token","tok");
  fputs(";\n",out);
  for( int cursymbol = 0, endsymbol = MapAny_size(&parser->m_tokdefs); cursymbol != endsymbol; ++cursymbol ) {
    // Map<int,SymbolDef>::const_iterator
    const int *tokid = 0;
    const SymbolDef *symdef = 0;
    MapAny_getByIndexConst(&parser->m_tokdefs,cursymbol,&tokid,&symdef);
    if( ! String_length(&symdef->m_semantictype) || symdef->m_tokid == ParserDef_getStartNt(parser) || symdef->m_tokid == ParserDef_getExtraNt(parser) )
      continue;
    if( ! MapAny_find(&tfields,&symdef->m_semantictype) ) {
      String fld;
      Scope_Push();
      String_init(&fld,true);
      int i = MapAny_size(&tfields);
      while( i > 0 ) {
        String_AddCharInPlace(&fld,'a'+(i%26));
        i /= 26;
      } 
      MapAny_insert(&tfields,&symdef->m_semantictype,&fld);
      lang->outDecl(lang,out,String_Chars(&symdef->m_semantictype),String_Chars(&fld));
      fputs(";\n",out);
      Scope_Pop();
    }
  }
  fputs("};\n",out);

  lang->outFunctionStart(lang,out,"static bool", "reduce");
  lang->outStartParameters(lang,out);
  lang->outDecl(lang,out, "extra_t&", "extra");
  fputs(", ", out);
  lang->outDecl(lang,out, "int", "productionidx");
  fputs(", ", out);
  lang->outDecl(lang,out, "stack_t*", "inputs");
  fputs(", ", out);
  lang->outDecl(lang,out, "stack_t&", "output");
  fputs(", ", out);
  lang->outDecl(lang,out, "const char**", "err");
  lang->outEndParameters(lang,out);
  lang->outStartFunctionCode(lang,out);
  fputc('\n',out);
  fputs("switch(productionidx) {\n", out);
  for( int curp = 0, endp = VectorAny_size(&parser->m_productions); curp != endp; ++curp ) {
    const Production *p = VectorAny_ArrayOpConstT(&parser->m_productions,curp,Production*);
    if( VectorAny_size(&p->m_action) == 0 )
      continue;
    fprintf(out,"case PROD_%d:\n", MapAny_findConstT(&pid2idx,&p->m_pid,int));
    String s;
    Scope_Push();
    String_init(&s,true);
    String_AssignString(&s,&p->m_filename);
    String_ReplaceAll(&s,"\\","\\\\");
    if( outputOptions->do_pound_line )
      fprintf(out, "#line %d \"%s\"\n", p->m_lineno, String_Chars(&s));
    for( int curaction = 0, endaction = VectorAny_size(&p->m_action); curaction != endaction; ++curaction ) {
      const ActionItem *item = &VectorAny_ArrayOpConstT(&p->m_action,curaction,ActionItem);
      if( item->m_actiontype == ActionTypeSrc )
        fputs(String_Chars(&item->m_str), out);
      else if( item->m_actiontype == ActionTypeDollarDollar ) {
        const String *semtype = &MapAny_findConstT(&parser->m_tokdefs,&p->m_nt,SymbolDef).m_semantictype;
        const String *fld = &MapAny_findConstT(&tfields,&semtype,String);
        fprintf(out,"output.%s",String_Chars(fld));
      } else if( item->m_actiontype == ActionTypeDollarNumber ) {
        SymbolType stype = ParserDef_getSymbolType(parser,VectorAny_ArrayOpConstT(&p->m_symbols,item->m_dollarnum-1,int));
        String semtype;
        Scope_Push();
        String_init(&semtype,true);
        if( stype == SymbolTypeTerminal )
          String_AssignChars(&semtype,"Token");
        else if( stype == SymbolTypeNonterminal )
          String_AssignString(&semtype,&MapAny_findConstT(&parser->m_tokdefs,&VectorAny_ArrayOpConstT(&p->m_symbols,item->m_dollarnum-1,int),SymbolDef).m_semantictype);
        const String *fld = &MapAny_findConstT(&tfields,&semtype,String);
        fprintf(out, "inputs[%d].%s",item->m_dollarnum-1,String_Chars(fld));
        Scope_Pop();
      }
    }
    fputs("\nbreak;\n", out);
    Scope_Pop();
  }
  fputs("default:\n", out);
  fputs("  break;\n", out);
  fputs("} /* end switch */\n", out);
  fputs("return true",out);
  lang->outEndStmt(lang,out);
  fputc('\n',out);
  lang->outEndFunctionCode(lang,out);
  Scope_Pop();
}

void OutputLRParserSolution(FILE *out, const ParserDef *parser, const LRParserSolution *solution, LanguageOutputOptions *options) {
  LanguageOutputter *outputer = &CLanguageOutputter;
  OutputLRParser(out,parser,solution,outputer,options);
}

void OutputLLParserSolution(FILE *out, const ParserDef *parser, const LLParserSolution *solution, LanguageOutputOptions *options) {
  String err;
  Scope_Push();
  String_init(&err,true);
  String_AssignChars(&err, "LL Parser Output not implemented");
  setParseError(-1,-1,&err);
  Scope_Pop();
}
