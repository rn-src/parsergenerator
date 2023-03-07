#include "parser.h"
#include <ctype.h>
#include "parsertokL.h"

extern void *zalloc(size_t len);

struct LanguageOutputter;
typedef struct LanguageOutputter LanguageOutputter;

struct LanguageOutputter {
  void (*outTop)(const LanguageOutputter* This, const LanguageOutputOptions* outputOptions, FILE* out);
  void (*outTypeDecl)(const LanguageOutputter* This, FILE* out, const char* type, const char* name);
  void (*outDecl)(const LanguageOutputter *This, FILE *out, const char *type, const char *name);
  void (*outOptionalDecl)(const LanguageOutputter* This, FILE* out, const char* type, const char* name);
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
  void (*outStartLineComment)(const LanguageOutputter* This, FILE* out);
};

void CLanguageOutputter_outTop(const LanguageOutputter* This, const LanguageOutputOptions* outputOptions, FILE* out) {
  const char** extraImports = outputOptions->m_extraImports;
  while( *extraImports ) {
    fprintf(out, "#include \"%s\"\n", *extraImports);
    ++extraImports;
  }
}
void CLanguageOutputter_outTypeDecl(const LanguageOutputter* This, FILE* out, const char* type, const char* name) {
  fputs("typedef ", out); fputs(type, out); fputc(' ', out); fputs(name, out);
}
void CLanguageOutputter_outDecl(const LanguageOutputter *This, FILE *out, const char *type, const char *name) { fputs(type,out); fputc(' ',out); fputs(name,out); }
void CLanguageOutputter_outOptionalDecl(const LanguageOutputter* This, FILE* out, const char* type, const char* name) { fputs(type, out); fputc(' ', out); fputs(name, out); }
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
void CLanguageOutputter_outStartLineComment(const LanguageOutputter* This, FILE* out) {
  fputs("//", out);
}
LanguageOutputter CLanguageOutputter = {CLanguageOutputter_outTop, CLanguageOutputter_outTypeDecl, CLanguageOutputter_outDecl, CLanguageOutputter_outTypeDecl, CLanguageOutputter_outArrayDecl, CLanguageOutputter_outStartArray, CLanguageOutputter_outEndArray, CLanguageOutputter_outEndStmt, CLanguageOutputter_outNull, CLanguageOutputter_outBool, CLanguageOutputter_outStr, CLanguageOutputter_outChar, CLanguageOutputter_outInt, CLanguageOutputter_outFunctionStart, CLanguageOutputter_outStartParameters, CLanguageOutputter_outEndParameters, CLanguageOutputter_outStartFunctionCode, CLanguageOutputter_outEndFunctionCode, CLanguageOutputter_outStartLineComment };

static const char* pytype(const char* type) {
  if (strncmp(type, "static", 6) == 0)
    type += 6;
  while (*type == ' ')
    ++type;
  if( strncmp(type,"const",5) == 0)
    type += 5;
  while( *type == ' ')
    ++type;
  if( strncmp("char*",type,5) == 0)
    return "str";
  return type;
}

void PyLanguageOutputter_outTop(const LanguageOutputter* This, const LanguageOutputOptions* outputOptions, FILE* out) {
  fputs("from typing import Any,Sequence,Optional,Callable\n", out);
  // in C we can depend on the preprocessor, in python we must import
  fputs("from lrparse import ACTION_SHIFT,ACTION_REDUCE,ACTION_STOP\n", out);
  fputs("from tok import Token\n", out);
  const char** extraImports = outputOptions->m_extraImports;
  while (*extraImports) {
    fprintf(out, "import %s\n", *extraImports);
    ++extraImports;
  }
}
void PyLanguageOutputter_outDecl(const LanguageOutputter* This, FILE* out, const char* type, const char* name) {
  fputs(name,out);
  fputs(": ",out);
  fputs(pytype(type), out);
}
void PyLanguageOutputter_outOptionalDecl(const LanguageOutputter* This, FILE* out, const char* type, const char* name) {
  fputs(name, out);
  fputs(": Optional[", out);
  fputs(pytype(type), out);
  fputs("]", out);
}
void PyLanguageOutputter_outTypeDecl(const LanguageOutputter* This, FILE* out, const char* type, const char* name) {
  fputs(name, out);
  fputs(" = ", out);
  fputs(pytype(type), out);
}
void PyLanguageOutputter_outArrayDecl(const LanguageOutputter* This, FILE* out, const char* type, const char* name) {
  fputs(name,out);
  fputs(": ", out);
  fputs("Sequence[",out);
  fputs(pytype(type), out);
  fputs("]",out);
}
void PyLanguageOutputter_outStartArray(const LanguageOutputter* This, FILE* out) {
  fputc('(', out);
}
void PyLanguageOutputter_outEndArray(const LanguageOutputter* This, FILE* out) {
  fputc(')', out);
}
void PyLanguageOutputter_outEndStmt(const LanguageOutputter* This, FILE* out) {
}
void PyLanguageOutputter_outNull(const LanguageOutputter* This, FILE* out) {
  fputs("None", out);
}
void PyLanguageOutputter_outBool(const LanguageOutputter* This, FILE* out, bool b) {
  fputs((b ? "True" : "False"), out);
}
void PyLanguageOutputter_outStr(const LanguageOutputter* This, FILE* out, const char* str) {
  fputc('\'', out); fputs(str, out); fputc('\'', out);
}
void PyLanguageOutputter_outChar(const LanguageOutputter* This, FILE* out, int c) {
  if (c == '\r') {
    fputs("'\\r'", out);
  }
  else if (c == '\n') {
    fputs("'\\n'", out);
  }
  else if (c == '\v') {
    fputs("'\\v'", out);
  }
  else if (c == ' ') {
    fputs("' '", out);
  }
  else if (c == '\t') {
    fputs("'\\t'", out);
  }
  else if (c == '\\' || c == '\'') {
    fprintf(out, "'\\%c'", c);
  }
  else if (c <= 126 && isgraph(c)) {
    fprintf(out, "'%c'", c);
  }
  else
    fprintf(out, "%d", c);
}
void PyLanguageOutputter_outInt(const LanguageOutputter* This, FILE* out, int i) { fprintf(out, "%d", i); }
void PyLanguageOutputter_outFunctionStart(const LanguageOutputter* This, FILE* out, const char* rettype, const char* name) {
  fputs("def ", out);
  fputs(name, out);
}
void PyLanguageOutputter_outStartParameters(const LanguageOutputter* This, FILE* out) {
  fputs("(", out);
}
void PyLanguageOutputter_outEndParameters(const LanguageOutputter* This, FILE* out) {
  fputs(")", out);
}
void PyLanguageOutputter_outStartFunctionCode(const LanguageOutputter* This, FILE* out) {
  fputs(":", out);
}
void PyLanguageOutputter_outEndFunctionCode(const LanguageOutputter* This, FILE* out) {}
void PyLanguageOutputter_outStartLineComment(const LanguageOutputter* This, FILE* out) {
  fputs("#", out);
}
LanguageOutputter PyLanguageOutputter = { PyLanguageOutputter_outTop, PyLanguageOutputter_outTypeDecl, PyLanguageOutputter_outDecl, PyLanguageOutputter_outOptionalDecl, PyLanguageOutputter_outArrayDecl, PyLanguageOutputter_outStartArray, PyLanguageOutputter_outEndArray, PyLanguageOutputter_outEndStmt, PyLanguageOutputter_outNull, PyLanguageOutputter_outBool, PyLanguageOutputter_outStr, PyLanguageOutputter_outChar, PyLanguageOutputter_outInt, PyLanguageOutputter_outFunctionStart, PyLanguageOutputter_outStartParameters, PyLanguageOutputter_outEndParameters, PyLanguageOutputter_outStartFunctionCode, PyLanguageOutputter_outEndFunctionCode, PyLanguageOutputter_outStartLineComment };

static void PrintExtraType(FILE *out, const ParserDef *parser, const LanguageOutputter *lang, const LanguageOutputOptions *outputOptions) {
  int extraNt = ParserDef_getExtraNt(parser);
  if( ! MapAny_contains(&parser->m_tokdefs, &extraNt) || String_length(&MapAny_findConstT(&parser->m_tokdefs, &extraNt, SymbolDef).m_semantictype) == 0 )
    lang->outTypeDecl(lang, out, "int", "extra_t");
  else
    lang->outTypeDecl(lang, out, String_Chars(&MapAny_findConstT(&parser->m_tokdefs, &extraNt, SymbolDef).m_semantictype), "extra_t");
  lang->outEndStmt(lang, out);
  fputc('\n', out);
}

static void PrintSymbolType(FILE *out, const ParserDef *parser, const LanguageOutputter *lang, const LanguageOutputOptions *outputOptions, MapAny /*<String,String>*/ *tfields) {
  String token_str;
  String tok_str;

  Scope_Push();
  String_init(&token_str, true);
  String_init(&tok_str, true);

  String_AssignChars(&token_str, "Token");
  String_AssignChars(&tok_str, "self.tok");
  MapAny_insert(tfields, &token_str, &tok_str);
  if( outputOptions->m_outputLanguage == OutputLanguage_Python ) {
    fputs("class stack_t:\n", out);
    fputs("  __slots__ = ('tok'", out);
    for (int cursymbol = 0, endsymbol = MapAny_size(&parser->m_tokdefs); cursymbol != endsymbol; ++cursymbol) {
      // Map<int,SymbolDef>::const_iterator
      const int* tokid = 0;
      const SymbolDef* symdef = 0;
      MapAny_getByIndexConst(&parser->m_tokdefs, cursymbol, (const void**)&tokid, (const void**)&symdef);
      if (!String_length(&symdef->m_semantictype) || symdef->m_tokid == ParserDef_getStartNt(parser) || symdef->m_tokid == ParserDef_getExtraNt(parser))
        continue;
      if (!MapAny_find(tfields, &symdef->m_semantictype)) {
        String fld;
        Scope_Push();
        String_init(&fld, true);
        int i = MapAny_size(tfields);
        while (i > 0) {
          String_AddCharInPlace(&fld, 'a' + (i % 26));
          i /= 26;
        }
        fputc(',', out);
        lang->outStr(lang, out, String_Chars(&fld));
        Scope_Pop();
      }
    }
    fputs(")\n", out);
    fputs("  def __init__(self) -> None:\n", out);
  } else {
    fputs("struct stack_t;\ntypedef struct stack_t stack_t;\n", out);
    fputs("struct stack_t {\n", out);
  }
  if (outputOptions->m_outputLanguage == OutputLanguage_Python)
    fputs("    self.",out);
  lang->outDecl(lang, out, "Token", "tok");
  lang->outEndStmt(lang,out);
  fputc('\n',out);
  for (int cursymbol = 0, endsymbol = MapAny_size(&parser->m_tokdefs); cursymbol != endsymbol; ++cursymbol) {
    // Map<int,SymbolDef>::const_iterator
    const int *tokid = 0;
    const SymbolDef *symdef = 0;
    MapAny_getByIndexConst(&parser->m_tokdefs, cursymbol, (const void**)&tokid, (const void**)&symdef);
    if (!String_length(&symdef->m_semantictype) || symdef->m_tokid == ParserDef_getStartNt(parser) || symdef->m_tokid == ParserDef_getExtraNt(parser))
      continue;
    if (!MapAny_find(tfields, &symdef->m_semantictype)) {
      String fld;
      Scope_Push();
      String_init(&fld, true);
      int i = MapAny_size(tfields);
      while (i > 0) {
        String_AddCharInPlace(&fld, 'a' + (i % 26));
        i /= 26;
      }
      MapAny_insert(tfields, &symdef->m_semantictype, &fld);
      if (outputOptions->m_outputLanguage == OutputLanguage_Python)
        fputs("    self.", out);
      lang->outDecl(lang, out, String_Chars(&symdef->m_semantictype), String_Chars(&fld));
      lang->outEndStmt(lang,out);
      Scope_Pop();
    }
  }
  if (outputOptions->m_outputLanguage != OutputLanguage_Python)
    fputc('}',out);
  lang->outEndStmt(lang,out);
  fputs("\n", out);

  Scope_Pop();
}

static void AssignTokenValues(const ParserDef *parser, MapAny /*<int,int>*/ *pid2idx, MapAny /*<int,int>*/ *nt2idx, int *pterminals, int *pnonterminals) {
  int terminals = 0, nonterminals = 0;
  for (int curtok = 0, endtok = MapAny_size(&parser->m_tokdefs); curtok != endtok; ++curtok) {
    const int *tokid = 0;
    const SymbolDef *tok = 0;
    MapAny_getByIndexConst(&parser->m_tokdefs, curtok, (const void**)&tokid, (const void**)&tok);
    if (tok->m_symboltype == SymbolTypeTerminal)
      ++terminals;
    else if (tok->m_symboltype == SymbolTypeNonterminal) {
      MapAny_insert(nt2idx, &tok->m_tokid, &nonterminals);
      ++nonterminals;
    }
  }
  *pterminals = terminals;
  *pnonterminals = nonterminals;
}

static void PrintTokenConstants(FILE *out, const ParserDef *parser, const LanguageOutputter *lang, const LanguageOutputOptions *outputOptions, MapAny /*<int,int>*/ *pid2idx, MapAny /*<int,int>*/ *nt2idx, int terminals, int nonterminals) {
  int firstnt = outputOptions->min_nt_value>(terminals + 1) ? outputOptions->min_nt_value : (terminals + 1);
  int firstproduction = firstnt + nonterminals;
  if( outputOptions->m_outputLanguage == OutputLanguage_Python ) {
    bool first = true;
    fprintf(out, "from %s import ", outputOptions->m_lexerName);
    for (int curtok = 0, endtok = MapAny_size(&parser->m_tokdefs); curtok != endtok; ++curtok) {
      const int* tokid = 0;
      const SymbolDef* tok = 0;
      MapAny_getByIndexConst(&parser->m_tokdefs, curtok, (const void**)&tokid, (const void**)&tok);
      if (tok->m_symboltype == SymbolTypeTerminal) {
        if (first) {
          first = false;
        } else {
          fputs(",", out);
        }
        fputs(String_Chars(&tok->m_name), out);
      }
    }
    fputc('\n', out);
  }
  for (int curtok = 0, endtok = MapAny_size(&parser->m_tokdefs); curtok != endtok; ++curtok) {
    const int *tokid = 0;
    const SymbolDef *tok = 0;
    MapAny_getByIndexConst(&parser->m_tokdefs, curtok, (const void**)&tokid, (const void**)&tok);
    if (tok->m_symboltype == SymbolTypeNonterminal) {
      lang->outDecl(lang, out, "const int", String_Chars(&tok->m_name));
      fputs(" = ", out);
      lang->outInt(lang, out, firstnt + MapAny_findT(nt2idx, &tok->m_tokid, int));
      lang->outEndStmt(lang, out);
      fputc('\n', out);
    }
  }

  for (int i = 0; i < VectorAny_size(&parser->m_productions); ++i) {
    char buf[64];
    MapAny_insert(pid2idx, &VectorAny_ArrayOpConstT(&parser->m_productions, i, Production*)->m_pid, &i);
    sprintf(buf, "PROD_%d", i);
    lang->outDecl(lang, out, "const int", buf);
    fputs(" = ", out);
    lang->outInt(lang, out, firstproduction + i);
    lang->outEndStmt(lang, out);
    fputc('\n', out);
  }
}

static void WriteSemanticAction(const Production *p, FILE *out, const ParserDef *parser, const LanguageOutputter *lang, const LanguageOutputOptions *outputOptions, MapAny /*<String,String>*/ *tfields) {
  if( ! VectorAny_size(&p->m_action) ) {
    if( outputOptions->m_outputLanguage == OutputLanguage_Python )
      fputs("pass",out);
    return;
  }
  String s;
  Scope_Push();
  String_init(&s, true);
  String_AssignString(&s, &p->m_filename);
  String_ReplaceAll(&s, "\\", "\\\\");
  if (outputOptions->do_pound_line && outputOptions->m_outputLanguage == OutputLanguage_C )
    fprintf(out, "#line %d \"%s\"\n", p->m_lineno, String_Chars(&s));
  for (int curaction = 0, endaction = VectorAny_size(&p->m_action); curaction != endaction; ++curaction) {
    const ActionItem *item = &VectorAny_ArrayOpConstT(&p->m_action, curaction, ActionItem);
    if (item->m_actiontype == ActionTypeSrc) {
      // skip the enclosing curly braces if python
      if (outputOptions->m_outputLanguage == OutputLanguage_Python && (curaction == 0 || curaction == endaction-1) )
        continue;
      fputs(String_Chars(&item->m_str), out);
    }
    else if (item->m_actiontype == ActionTypeDollarDollar) {
      const String *semtype = &MapAny_findConstT(&parser->m_tokdefs, &p->m_nt, SymbolDef).m_semantictype;
      const String *fld = &MapAny_findConstT(tfields, semtype, String);
      if( outputOptions->m_parserType == ParserType_LR )
        fprintf(out, "output.%s", String_Chars(fld));
      else if( outputOptions->m_parserType == ParserType_LL )
        fprintf(out, "(output->%s)", String_Chars(fld));
    }
    else if (item->m_actiontype == ActionTypeDollarNumber) {
      SymbolType stype = ParserDef_getSymbolType(parser, VectorAny_ArrayOpConstT(&p->m_symbols, item->m_dollarnum - 1, int));
      String semtype;
      Scope_Push();
      String_init(&semtype, true);
      if (stype == SymbolTypeTerminal)
        String_AssignChars(&semtype, "Token");
      else if (stype == SymbolTypeNonterminal) {
        int sym = VectorAny_ArrayOpConstT(&p->m_symbols, item->m_dollarnum - 1, int);
        const SymbolDef *symboldef = &MapAny_findConstT(&parser->m_tokdefs, &sym, SymbolDef);
        String_AssignString(&semtype,&symboldef->m_semantictype);
      }
      const String *fld = &MapAny_findConstT(tfields, &semtype, String);
      if (outputOptions->m_parserType == ParserType_LR)
        fprintf(out, "inputs[%d].%s", item->m_dollarnum - 1, String_Chars(fld));
      else if (outputOptions->m_parserType == ParserType_LL)
        fprintf(out, "(v%d->%s)", item->m_dollarnum - 1, String_Chars(fld));
      Scope_Pop();
    }
  }
  if (outputOptions->do_pound_line && outputOptions->m_outputLanguage == OutputLanguage_C)
    fputs("#line\n", out);
  Scope_Pop();
}

static void OutputLRParser(FILE *out, const ParserDef *parser, const LRParserSolution *solution, const LanguageOutputter *lang, const LanguageOutputOptions *outputOptions) {
  bool first = true;
  MapAny /*<int,int>*/ pid2idx;
  MapAny /*<int,int>*/ nt2idx;
  VectorAny /*<int>*/ pidxtopoffset;
  VectorAny /*<int>*/ sidxtoaoffset;
  MapAny /*<String,String>*/ tfields;
  int poffset = 0;
  int terminals = 0;
  int nonterminals = 0;

  Scope_Push();
  MapAny_init(&pid2idx,getIntElement(),getIntElement(),true);
  MapAny_init(&nt2idx,getIntElement(),getIntElement(),true);
  VectorAny_init(&pidxtopoffset,getIntElement(),true);
  VectorAny_init(&sidxtoaoffset,getIntElement(),true);
  MapAny_init(&tfields, getStringElement(), getStringElement(), true);

  lang->outTop(lang, outputOptions, out);
  AssignTokenValues(parser, &pid2idx, &nt2idx, &terminals, &nonterminals);
  PrintTokenConstants(out, parser, lang, outputOptions, &pid2idx, &nt2idx, terminals, nonterminals);

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
    fputs("\n",out);
    lang->outStartLineComment(lang,out);
    fprintf(out," state %d \n", i);
    VectorAny_push_back(&sidxtoaoffset,&actionidx);
    const shifttosymbols_t *shifttosymbols = &MapAny_findConstT(&solution->m_shifts,&i,shifttosymbols_t);
    if( shifttosymbols ) {
      for( int curshift = 0, endshift = MapAny_size(shifttosymbols); curshift != endshift; ++curshift ) {
        const int *tokid = 0;
        const SetAny /*<int>*/ *curShiftSymbols = 0;
        MapAny_getByIndexConst(shifttosymbols,curshift,(const void**)&tokid,(const void**)&curShiftSymbols);
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
        MapAny_getByIndexConst(reducebysymbols,curreduce,(const void**)&reduceby,(const void**)&reducesymbols);
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
    MapAny_getByIndexConst(&parser->m_tokdefs,curtok,(const void**)&tokid,(const void**)&tokdef);
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

  PrintExtraType(out, parser, lang, outputOptions);
  PrintSymbolType(out,parser,lang,outputOptions,&tfields);

  lang->outFunctionStart(lang,out,"static bool", "reduce");
  lang->outStartParameters(lang,out);
  if( outputOptions->m_outputLanguage == OutputLanguage_Python ) {
    lang->outDecl(lang, out, "extra_t", "extra");
    fputs(", ", out);
    lang->outDecl(lang, out, "int", "productionidx");
    fputs(", ", out);
    lang->outArrayDecl(lang, out, "stack_t", "inputs");
    fputs(", ", out);
    lang->outDecl(lang, out, "stack_t", "output");
    fputs(", err: Callable[[str],None]", out);
  } else {
    fputs(", ", out);
    lang->outDecl(lang, out, "stack_t&", "output");
    fputs(", ", out);
    lang->outDecl(lang, out, "const char**", "err");
  }
  lang->outEndParameters(lang, out);
  if (outputOptions->m_outputLanguage == OutputLanguage_Python) {
    fputs(" -> bool", out);
  }
  lang->outStartFunctionCode(lang, out);
  fputc('\n',out);
  if (outputOptions->m_outputLanguage == OutputLanguage_Python) {
    fputs("  match productionidx:\n", out);
  } else {
    fputs("  switch(productionidx) {\n", out);
  }
  for( int curp = 0, endp = VectorAny_size(&parser->m_productions); curp != endp; ++curp ) {
    const Production *p = VectorAny_ArrayOpConstT(&parser->m_productions,curp,Production*);
    if( VectorAny_size(&p->m_action) == 0 )
      continue;
    fprintf(out,"    case PROD_%d:\n      ", MapAny_findConstT(&pid2idx,&p->m_pid,int));
    WriteSemanticAction(p, out, parser, lang, outputOptions, &tfields);
    fputc('\n',out);
  }
  if (outputOptions->m_outputLanguage == OutputLanguage_Python) {
    fputs("    case _:\n      pass\n", out);
  }
  else {
    fputs("default:\n", out);
    fputs("  break;\n", out);
    fputs("} /* end switch */\n", out);
  }
  fputs("  return ",out);
  lang->outBool(lang,out,true);
  lang->outEndStmt(lang,out);
  fputc('\n', out);
  lang->outEndFunctionCode(lang,out);
  fputc('\n', out);
  Scope_Pop();
}

void OutputLRParserSolution(FILE *out, const ParserDef *parser, const LRParserSolution *solution, LanguageOutputOptions *options) {
  LanguageOutputter *outputer = options->m_outputLanguage == OutputLanguage_Python ? &PyLanguageOutputter : &CLanguageOutputter;
  OutputLRParser(out,parser,solution,outputer,options);
}

const char *SymbolName(const ParserDef *parser, int tok) {
  const SymbolDef *def = &MapAny_findConstT(&parser->m_tokdefs, &tok, SymbolDef);
  return String_Chars(&def->m_name);
}

const char *ProductionSymbolName(const ParserDef *parser, const Production *p, int index) {
  int tok = VectorAny_ArrayOpConstT(&p->m_symbols,index,int);
  return SymbolName(parser,tok);
}

#if 0
static void PrintParseProductions(FILE *out, const ParserDef *parser, const LLParserSolution *solution, LanguageOutputter *lang, LanguageOutputOptions *outputOptions, VectorAny /*<Production*>*/ *productions, int low, int high, int index) {
  int symbol = VectorAny_ArrayOpConstT(&VectorAny_ArrayOpConstT(&productions, low, Production*)->m_symbols,index);
  productionandrestrictstate_t prs;
  prs.production = p;
  prs.restrictstate = 0;
  const SetAny /*<int>*/ *peeks = &MapAny_findConstT(&solution->m_firstsAndFollows.m_firsts, &prs, SetAny);
  fputs("  if( ", out);
  for (int curpeek = 0, endpeek = SetAny_size(peeks); curpeek < endpeek; ++curpeek) {
    if (curpeek != 0)
      fputs(" || ", out);
    int tok = SetAny_getByIndexConstT(peeks, curpeek, int);
    fprintf(out, "tokenizer->peek(tokenizer) == %s", SymbolName(parser, tok));
  }
  fputs(" ) {\n", out);
  for (int curp = low+1; curp < high; ++curp ) {
    const Production *p = VectorAny_ArrayOpConstT(&productions, curp, Production*);
    int prodsym = -1;
    if(index < VectorAny_size(&p->m_symbols)) {
      prodsym = VectorAny_ArrayOpConstT(&p->m_symbols, index);
    if( prodsym != symbol ) {
    }
  }

  const Production *acceptProduction = 0;
      continue;
    if (bFirst)
      bFirst = false;
    else
      fputs(" else ", out);
    for (int i = 0, n = VectorAny_size(&p->m_symbols); i < n; ++i) {
      int tok = VectorAny_ArrayOpConstT(&p->m_symbols, i, int);
      if (ParserDef_getSymbolType(parser, tok) == SymbolTypeTerminal)
        fprintf(out, "    stack_t v%d = readtoken(%s);\n", i, ProductionSymbolName(parser, p, i));
      else
        fprintf(out, "    stack_t v%d;\n", i);
      fprintf(out, "    parse_%s(tokenizer,restrictState,&v%d);\n", ProductionSymbolName(parser, p, i), i);
    }
    WriteSemanticAction(p, out, parser, lang, outputOptions, &tfields);
    fputs("  }", out);
  }
}
#endif

static void OutputLLParser(FILE *out, const ParserDef *parser, const LLParserSolution *solution, LanguageOutputter *lang, LanguageOutputOptions *outputOptions) {
  MapAny /*<int,int>*/ pid2idx;
  MapAny /*<int,int>*/ nt2idx;
  MapAny /*<String,String>*/ tfields;
  VectorAny /*<Production*>*/ productions, normal_productions, empty_productions, leftrec_productions;
  SetAny /*<int>*/ symbols;
  int terminals = 0;
  int nonterminals = 0;

  Scope_Push();
  MapAny_init(&pid2idx, getIntElement(), getIntElement(), true);
  MapAny_init(&nt2idx, getIntElement(), getIntElement(), true);
  MapAny_init(&tfields, getStringElement(), getStringElement(), true);
  VectorAny_init(&productions,getPointerElement(),true);
  VectorAny_init(&normal_productions, getPointerElement(), true);
  VectorAny_init(&empty_productions, getPointerElement(), true);
  VectorAny_init(&leftrec_productions, getPointerElement(), true);
  SetAny_init(&symbols, getIntElement(), true);

  fputs("#include \"tok.h\"\n\n",out);
  AssignTokenValues(parser, &pid2idx, &nt2idx, &terminals, &nonterminals);
  PrintTokenConstants(out, parser, lang, outputOptions, &pid2idx, &nt2idx, terminals, nonterminals);
  PrintExtraType(out, parser, lang, outputOptions);
  PrintSymbolType(out, parser, lang, outputOptions, &tfields);
  fputs("\n", out);

  for (int curtok = 0, endtok = MapAny_size(&parser->m_tokdefs); curtok < endtok; ++curtok) {
    const int *tok = 0;
    const SymbolDef *tokdef = 0;
    MapAny_getByIndexConst(&parser->m_tokdefs,curtok,(const void**)&tok,(const void**)&tokdef);
    if( tokdef->m_symboltype != SymbolTypeNonterminal )
      continue;
    ParserDef_getProductionsOfNt(parser,tokdef->m_tokid,&productions);
    VectorAny_clear(&normal_productions);
    VectorAny_clear(&empty_productions);
    VectorAny_clear(&leftrec_productions);
    if( VectorAny_size(&productions) == 0 )
      continue;
    for (int curp = 0, endp = VectorAny_size(&productions); curp < endp; ++curp) {
      const Production *p = VectorAny_ArrayOpConstT(&productions, curp, Production*);
      if (VectorAny_size(&p->m_symbols) == 0 )
        VectorAny_push_back(&empty_productions,&p);
      else if( VectorAny_ArrayOpConstT(&p->m_symbols, 0, int) == tokdef->m_tokid )
        VectorAny_push_back(&leftrec_productions, &p);
      else
        VectorAny_push_back(&normal_productions, &p);
    }
    fprintf(out, "void parse_%s(Tokenizer *tokenizer, int restrictState, stack_t *out) {\n", String_Chars(&tokdef->m_name));
    //PrintParseProductions(out,&normal_productions,0,&symbols);
    // parse empty productions
    fputs(" else {\n", out);
    if( VectorAny_size(&empty_productions) ) {
      const Production *p = VectorAny_ArrayOpConstT(&empty_productions, 0, Production*);
      WriteSemanticAction(p, out, parser, lang, outputOptions, &tfields);
    }
    else {
      fprintf(out, "    error(\"Unable to parse '%s'\");\n", String_Chars(&tokdef->m_name));
    }
    fputs("  }", out);
    // parse left-recursive productions
    if( VectorAny_size(&leftrec_productions) ) {
      fputs("\n  while( out ) {\n", out);
      //PrintParseProductions(out,&leftrec_productions,1,&symbols);
  #if 0
      for (int curp = 0, endp = VectorAny_size(&leftrec_productions); curp < endp; ++curp) {
        const Production *p = VectorAny_ArrayOpConstT(&leftrec_productions, curp, Production*);
        if (bFirst)
          bFirst = false;
        else
          fputs(" else ", out);
        fprintf(out, "if( peek() == %s ) {\n    v0 = out;\n", ProductionSymbolName(parser, p, 1));
        for (int i = 1, n = VectorAny_size(&p->m_symbols); i < n; ++i) {
          int tok = VectorAny_ArrayOpConstT(&p->m_symbols, i, int);
          if (ParserDef_getSymbolType(parser, tok) == SymbolTypeTerminal)
            fprintf(out, "      v%d = readtoken(%s);\n", i, ProductionSymbolName(parser, p, i));
          else {
            fprintf(out, "      stack_t v%d;\n", i);
            fprintf(out, "      parse_%s(tokenizer,restrictState,&v%d);\n", ProductionSymbolName(parser, p, i), i);
          }
        }
        fputs("      out = ...\n", out);
        fputs("}", out);
      }
#endif
      fputs("  }", out);
    }
    fprintf(out, "\n} /* end parse_%s */\n\n", String_Chars(&tokdef->m_name));
  }
  Scope_Pop();
}

void OutputLLParserSolution(FILE *out, const ParserDef *parser, const LLParserSolution *solution, LanguageOutputOptions *options) {
  LanguageOutputter *outputer = &CLanguageOutputter;
  OutputLLParser(out, parser, solution, outputer, options);
}
