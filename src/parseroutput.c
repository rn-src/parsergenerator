#include "parser.h"
#include <ctype.h>

static void outBottom(const LanguageOutputter *This, FILE *out) {
  if( This->options->outputLanguage == OutputLanguage_C ) {
    fputs("parseinfo prsinfo = {\n", out);
    fputs("  nstates,\n", out);
    fputs("  actions,\n", out);
    fputs("  actionstart,\n", out);
    fputs("  PROD_0,\n", out);
    fputs("  PARSE_ERROR,\n", out);
    fputs("  nproductions,\n", out);
    fputs("  productions,\n", out);
    fputs("  productionstart,\n", out);
    fputs("  START,\n", out);
    fputs("  nonterminals,\n", out);
    fputs("  sizeof(stack_t),\n", out);
    fputs("  (reducefnct)reduce,\n", out);
    fputs("};\n", out);
  }
  This->outBottom(This,out);
}

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
  String_AssignChars(&tok_str, "tok");
  MapAny_insert(tfields, &token_str, &tok_str);
  if( outputOptions->outputLanguage == OutputLanguage_Python ) {
    fputs("class stack_t:\n", out);
    fputs("  __slots__ = ('tok'", out);
    int slot = 0;
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
        int i = MapAny_size(tfields)+slot;
	++slot;
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
  if (outputOptions->outputLanguage == OutputLanguage_Python)
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
      if (outputOptions->outputLanguage == OutputLanguage_Python)
        fputs("    self.", out);
      lang->outDecl(lang, out, String_Chars(&symdef->m_semantictype), String_Chars(&fld));
      lang->outEndStmt(lang,out);
      fputc('\n',out);
      Scope_Pop();
    }
  }
  if (outputOptions->outputLanguage != OutputLanguage_Python)
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

static void ImportTokenConstants(FILE *out, const ParserDef *parser, const LanguageOutputter *lang, LanguageOutputOptions *outputOptions, MapAny /*<int,int>*/ *pid2idx, MapAny /*<int,int>*/ *nt2idx, int terminals, int nonterminals) {
  if( outputOptions->outputLanguage != OutputLanguage_Python )
    return;
  LanguageOutputOptions_import(outputOptions, outputOptions->lexerName, 0);
  for (int curtok = 0, endtok = MapAny_size(&parser->m_tokdefs); curtok != endtok; ++curtok) {
    const int* tokid = 0;
    const SymbolDef* tok = 0;
    MapAny_getByIndexConst(&parser->m_tokdefs, curtok, (const void**)&tokid, (const void**)&tok);
    if (tok->m_symboltype == SymbolTypeTerminal)
      LanguageOutputOptions_importitem(outputOptions, String_Chars(&tok->m_name), 0);
  }
}

static void PrintProductionConstants(FILE *out, const ParserDef *parser, const LanguageOutputter *lang, const LanguageOutputOptions *outputOptions, MapAny /*<int,int>*/ *pid2idx, MapAny /*<int,int>*/ *nt2idx, int terminals, int nonterminals) {
  int firstnt = outputOptions->min_nt_value>(terminals + 1) ? outputOptions->min_nt_value : (terminals + 1);
  int firstproduction = firstnt + nonterminals;
  for (int curtok = 0, endtok = MapAny_size(&parser->m_tokdefs); curtok != endtok; ++curtok) {
    const int *tokid = 0;
    const SymbolDef *tok = 0;
    MapAny_getByIndexConst(&parser->m_tokdefs, curtok, (const void**)&tokid, (const void**)&tok);
    if (tok->m_symboltype == SymbolTypeNonterminal)
      lang->outIntDecl(lang,out,String_Chars(&tok->m_name),firstnt + MapAny_findT(nt2idx, &tok->m_tokid, int));
  }

  for (int i = 0; i < VectorAny_size(&parser->m_productions); ++i) {
    char buf[64];
    MapAny_insert(pid2idx, &VectorAny_ArrayOpConstT(&parser->m_productions, i, Production*)->m_pid, &i);
    sprintf(buf, "PROD_%d", i);
    lang->outIntDecl(lang,out,buf,firstproduction+i);
  }
}

static void WriteSemanticAction(const Production *p, FILE *out, const ParserDef *parser, const LanguageOutputter *lang, const LanguageOutputOptions *outputOptions, MapAny /*<String,String>*/ *tfields) {
  if( ! VectorAny_size(&p->m_action) ) {
    if( outputOptions->outputLanguage == OutputLanguage_Python )
      fputs("pass",out);
    return;
  }
  String s;
  Scope_Push();
  String_init(&s, true);
  String_AssignString(&s, &p->m_filename);
  String_ReplaceAll(&s, "\\", "\\\\");
  if (outputOptions->do_pound_line && outputOptions->outputLanguage == OutputLanguage_C )
    fprintf(out, "#line %d \"%s\"\n", p->m_lineno, String_Chars(&s));
  for (int curaction = 0, endaction = VectorAny_size(&p->m_action); curaction != endaction; ++curaction) {
    const ActionItem *item = &VectorAny_ArrayOpConstT(&p->m_action, curaction, ActionItem);
    if (item->m_actiontype == ActionTypeSrc) {
      // skip the enclosing curly braces if python
      if (outputOptions->outputLanguage == OutputLanguage_Python && (curaction == 0 || curaction == endaction-1) )
        continue;
      fputs(String_Chars(&item->m_str), out);
    }
    else if (item->m_actiontype == ActionTypeDollarExtra) {
      if( outputOptions->outputLanguage == OutputLanguage_Python )
        fputs("extra", out);
      else
        fputs("(*extra)", out);
    }
    else if (item->m_actiontype == ActionTypeDollarDollar) {
      const String *semtype = &MapAny_findConstT(&parser->m_tokdefs, &p->m_nt, SymbolDef).m_semantictype;
      const String *fld = &MapAny_findConstT(tfields, semtype, String);
      if( outputOptions->outputLanguage == OutputLanguage_Python )
        fprintf(out, "output.%s", String_Chars(fld));
      else
        fprintf(out, "output->%s", String_Chars(fld));
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
      fprintf(out, "inputs[%d].%s", item->m_dollarnum - 1, String_Chars(fld));
      Scope_Pop();
    }
  }
  // Once upon a time, this would reset the line/file
  //if (outputOptions->do_pound_line && outputOptions->m_outputLanguage == OutputLanguage_C)
  //  fputs("\n#line\n", out);
  Scope_Pop();
}

static void OutputLRParser(FILE *out, const ParserDef *parser, const LRParserSolution *solution, const LanguageOutputter *lang, LanguageOutputOptions *outputOptions) {
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

  AssignTokenValues(parser, &pid2idx, &nt2idx, &terminals, &nonterminals);
  ImportTokenConstants(out, parser, lang, outputOptions, &pid2idx, &nt2idx, terminals, nonterminals);
  lang->outTop(lang, out);
  PrintProductionConstants(out, parser, lang, outputOptions, &pid2idx, &nt2idx, terminals, nonterminals);

  int firstnt = outputOptions->min_nt_value>(terminals + 1) ? outputOptions->min_nt_value : (terminals + 1);
  int firstproduction = firstnt + nonterminals;

  lang->outIntDecl(lang,out,"nstates",VectorAny_size(&solution->m_states));

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

  lang->outIntDecl(lang,out,"nproductions",VectorAny_size(&parser->m_productions));


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
  if( outputOptions->outputLanguage == OutputLanguage_Python ) {
    lang->outDecl(lang, out, "extra_t", "extra");
    fputs(", ", out);
    lang->outDecl(lang, out, "int", "productionidx");
    fputs(", ", out);
    lang->outArrayDecl(lang, out, "stack_t", "inputs");
    fputs(", ", out);
    lang->outDecl(lang, out, "stack_t", "output");
    fputs(", err: IO", out);
  } else {
    lang->outDecl(lang, out, "extra_t*", "extra");
    fputs(", ", out);
    lang->outDecl(lang, out, "int", "productionidx");
    fputs(", ", out);
    lang->outDecl(lang, out, "stack_t*", "inputs");
    fputs(", ", out);
    lang->outDecl(lang, out, "stack_t*", "output");
    fputs(", ", out);
    lang->outDecl(lang, out, "const char**", "err");
  }
  lang->outEndParameters(lang, out);
  if (outputOptions->outputLanguage == OutputLanguage_Python) {
    fputs(" -> bool", out);
  }
  lang->outStartFunctionCode(lang, out);
  fputc('\n',out);
  if (outputOptions->outputLanguage == OutputLanguage_Python) {
    fputs("  match productionidx:\n", out);
  } else {
    fputs("  switch(productionidx) {\n", out);
  }
  for( int curp = 0, endp = VectorAny_size(&parser->m_productions); curp != endp; ++curp ) {
    const Production *p = VectorAny_ArrayOpConstT(&parser->m_productions,curp,Production*);
    if( VectorAny_size(&p->m_action) == 0 )
      continue;
    if( outputOptions->outputLanguage == OutputLanguage_Python) {
      int prodIdx = MapAny_findConstT(&pid2idx,&p->m_pid,int);
      fprintf(out,"    case %d: # PROD_%d:\n      ", prodIdx+firstproduction, prodIdx);
    } else {
      fprintf(out,"    case PROD_%d:\n      ", MapAny_findConstT(&pid2idx,&p->m_pid,int));
    }
    WriteSemanticAction(p, out, parser, lang, outputOptions, &tfields);
    if( outputOptions->outputLanguage == OutputLanguage_C) {
      fputs("\n      break;\n", out);
    }
    fputc('\n',out);
  }
  if (outputOptions->outputLanguage == OutputLanguage_Python) {
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
  outBottom(lang,out);
  fputc('\n', out);
  Scope_Pop();
}

void OutputLRParserSolution(FILE *out, const ParserDef *parser, const LRParserSolution *solution, LanguageOutputOptions *options) {
  LanguageOutputter outputter;
  LanguageOutputter_init(&outputter,options);
  if( options->outputLanguage == OutputLanguage_C ) {
    LanguageOutputOptions_import(options,"lrparse",0);
    LanguageOutputOptions_import(options,options->lexerName,0);
  }
  if( options->outputLanguage == OutputLanguage_Python ) {
    LanguageOutputOptions_import(options,"typing",0);
    LanguageOutputOptions_importitem(options,"Any",0);
    LanguageOutputOptions_importitem(options,"Sequence",0);
    LanguageOutputOptions_importitem(options,"Optional",0);
    LanguageOutputOptions_importitem(options,"Callable",0);
    LanguageOutputOptions_importitem(options,"IO",0);
    LanguageOutputOptions_import(options,"lrparse",0);
    LanguageOutputOptions_importitem(options,"ACTION_SHIFT",0);
    LanguageOutputOptions_importitem(options,"ACTION_REDUCE",0);
    LanguageOutputOptions_importitem(options,"ACTION_STOP",0);
    LanguageOutputOptions_import(options,"tok",0);
    LanguageOutputOptions_importitem(options,"Token",0);
  }
  OutputLRParser(out,parser,solution,&outputter,options);
}

const char *SymbolName(const ParserDef *parser, int tok) {
  const SymbolDef *def = &MapAny_findConstT(&parser->m_tokdefs, &tok, SymbolDef);
  return String_Chars(&def->m_name);
}

const char *ProductionSymbolName(const ParserDef *parser, const Production *p, int index) {
  int tok = VectorAny_ArrayOpConstT(&p->m_symbols,index,int);
  return SymbolName(parser,tok);
}


