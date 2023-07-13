#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "tokenizer.h"

void outBottom(const LanguageOutputter* This, FILE* out, bool hassections) {
  if( This->options->minimal )
    return;
  if( This->options->outputLanguage == OutputLanguage_C ) {
    const char *prefix = This->options->prefix ? This->options->prefix : "";
    fprintf(out, "struct tokinfo %stkinfo = {\n", prefix);
    fprintf(out, "  %stokenCount,\n", prefix);
    fprintf(out, "  %ssectionCount,\n", prefix);
    if( hassections ) {
      fprintf(out, "  %ssectioninfo,\n", prefix);
      fprintf(out, "  %ssectioninfo_offset,\n", prefix);
    } else {
      fputs("  0,\n  0,\n", out);
    }
    fprintf(out, "  %stokenstr,\n", prefix);
    fprintf(out, "  %sisws,\n", prefix);
    fprintf(out, "  %sstateCount,\n", prefix);
    fprintf(out, "  %sstateinfo_format,\n", prefix);
    fprintf(out, "  %sstateinfo,\n", prefix);
    fprintf(out, "  %sstateinfo_offset,\n", prefix);
    fputs("};\n", out);
  }
  This->outBottom(This,out);
}

static bool OutputSections(FILE *out, const Nfa *dfa, const LanguageOutputter *lang, VectorAny /*<Token>*/ *tokens) {
  bool hasactions = false;
  for( int i = 0, n = Nfa_getSections(dfa); i < n; ++i ) {
    for( int cur = 0, end = VectorAny_size(tokens); cur != end; ++cur ) {
      const Token *token = &VectorAny_ArrayOpConstT(tokens,cur,Token);
      const Action *action = &MapAny_findConstT(&token->m_actions,&i,Action);
      if( ! action )
        continue;
      hasactions = true;
      break;
    }
    if( hasactions )
      break;
  }
  if( ! hasactions )
    return false;
  MapAny /*<int,int>*/ sectioncounts;
  Scope_Push();
  MapAny_init(&sectioncounts,getIntElement(),getIntElement(),true);
  lang->outArrayDecl(lang,out, "static const unsigned char", "sectioninfo");
  fputs(" = ",out);
  lang->outStartArray(lang,out);
  bool first = true;
  for( int i = 0, n = Nfa_getSections(dfa); i < n; ++i ) {
    for( int cur = 0, end = VectorAny_size(tokens); cur != end; ++cur ) {
      const Token *token = &VectorAny_ArrayOpConstT(tokens,cur,Token);
      const Action *action = &MapAny_findConstT(&token->m_actions,&i,Action);
      if( ! action )
        continue;
      int cnt = 0;
      if( first )
        first = false;
      else
        fputc(',',out);
      cnt += encodeuint(lang,out,token->m_token);
      fputc(',',out);
      cnt += encodeuint(lang,out,action->m_action);
      fputc(',',out);
      cnt += encodeuint(lang,out,action->m_actionarg);
      if( ! MapAny_find(&sectioncounts,&i) ) {
        int zero = 0;
        MapAny_insert(&sectioncounts,&i,&zero);
      }
      MapAny_findT(&sectioncounts,&i,int) += cnt;
    }
  }
  lang->outEndArray(lang,out);
  lang->outEndStmt(lang,out);
  fputc('\n',out);

  int secoffset = 0;
  lang->outArrayDecl(lang,out,"static const unsigned short","sectioninfo_offset");
  fputs(" = ",out);
  lang->outStartArray(lang,out);
  first = true;
  for( int i = 0, n = Nfa_getSections(dfa); i < n; ++i ) {
    if( first )
      first = false;
    else
      fputc(',',out);
    lang->outInt(lang,out,secoffset);
    if( MapAny_findConst(&sectioncounts,&i) )
      secoffset += MapAny_findConstT(&sectioncounts,&i,int);
  }
  if( ! first )
    fputc(',',out);
  lang->outInt(lang,out,secoffset);
  lang->outEndArray(lang,out);
  lang->outEndStmt(lang,out);
  fputc('\n',out);
  Scope_Pop();
  return true;
}

static void OutputStateInfo(FILE *out, const Nfa *dfa, const LanguageOutputter *lang) {
  VectorAny /*<int>*/ stateinfo;
  VectorAny /*<int>*/ stateinfocounts;
  Scope_Push();
  VectorAny_init(&stateinfocounts,getIntElement(),true);
  VectorAny_init(&stateinfo,getIntElement(),true);

  // there typically aren't long runs of repeats, and values are usually
  // small, so we use a simple encoding scheme to save space.
  int lastfrom = -1;
  for( int cur = 0, end = MapAny_size(&dfa->m_transitions); cur < end; ++cur ) {
    const Transition *transition = 0;
    const CharSet *ranges = 0;
    int cnt = 0;
    MapAny_getByIndexConst(&dfa->m_transitions,cur,(const void**)&transition,(const void**)&ranges);
    if( lastfrom != transition->m_from ) {
      while( ++lastfrom < transition->m_from ) {
        int tokfrm = Nfa_getStateToken(dfa,lastfrom);
        int cnttmp = 0;
        if( tokfrm != -1 ) {
          VectorAny_push_back(&stateinfo,&tokfrm);
          ++cnttmp;
        }
        VectorAny_push_back(&stateinfocounts,&cnttmp);
      }
      // -1 is quite common, add one to shrink encoding
      int tok = Nfa_getStateToken(dfa,transition->m_from);
      VectorAny_push_back(&stateinfo,&tok);
      ++cnt;
    }
    VectorAny_push_back(&stateinfo,&transition->m_to);
    int sz = CharSet_size(ranges);
    VectorAny_push_back(&stateinfo,&sz);
    // range values increase, so subtracting the last value helps encoding
    int last = 0;
    for( int cur = 0, end = CharSet_size(ranges); cur != end; ++cur ) {
      const CharRange *curRange = CharSet_getRange(ranges,cur);
      int tmp = curRange->m_low-last;
      VectorAny_push_back(&stateinfo,&tmp);
      last = curRange->m_low;
      tmp = curRange->m_high-last;
      VectorAny_push_back(&stateinfo,&tmp);
      last = curRange->m_high;
    }
    // keeping count of how many values are in each stateinfo
    cnt += 2+2*CharSet_size(ranges);
    if( VectorAny_size(&stateinfocounts) == transition->m_from )
      VectorAny_push_back(&stateinfocounts,&cnt);
    else
      VectorAny_ArrayOpT(&stateinfocounts,transition->m_from,int) += cnt;
  }

  WriteIndexedArray(lang, out,
      lang->options->encode,
      lang->options->compress,
      lang->options->allow_full_compression,
      &stateinfo,
      "static const unsigned char",
      "stateinfo",
      &stateinfocounts,
      "static const unsigned short",
      "stateinfo_offset");
  
  Scope_Pop();
}

static void OutputDfaSource(FILE *out, const Nfa *dfa, const LanguageOutputter *lang) {
  bool first = true;
  // Going to assume there are no gaps in toking numbering
  VectorAny /*<Token>*/ tokens;
  VectorAny /*<int>*/ wsflags;
  Scope_Push();
  VectorAny_init(&tokens,getTokenElement(),true);
  VectorAny_init(&wsflags,getIntElement(),true);
  Nfa_getTokenDefs(dfa,&tokens);

  lang->outTop(lang,out);

  for( int cur = 0, end = VectorAny_size(&tokens); cur != end; ++cur ) {
    const Token *curToken = &VectorAny_ArrayOpConstT(&tokens,cur,Token);
    lang->outIntDecl(lang,out,String_Chars(&curToken->m_name),curToken->m_token);
  }

  lang->outIntDecl(lang,out,"tokenCount",VectorAny_size(&tokens));
  lang->outIntDecl(lang,out,"sectionCount",Nfa_getSections(dfa));

  bool hassections = OutputSections(out, dfa, lang, &tokens);

  lang->outArrayDecl(lang,out, "static const char*", "tokenstr");
  fputs(" = ",out);
  lang->outStartArray(lang,out);
  first = true;
  for( int cur = 0, end = VectorAny_size(&tokens); cur != end; ++cur ) {
    if( first )
      first = false;
    else
      fputc(',',out);
    const Token *token = &VectorAny_ArrayOpConstT(&tokens,cur,Token);
    lang->outStr(lang,out,String_Chars(&token->m_name));
  }
  lang->outEndArray(lang,out);
  lang->outEndStmt(lang,out);
  fputc('\n',out);

  int zero = 0;
  int ntokens = VectorAny_size(&tokens);
  int nwsflags = ntokens/8+((ntokens%8)?1:0);
  for( int i = 0, end = nwsflags; i < end; ++i )
    VectorAny_push_back(&wsflags,&zero);
  for( int i = 0, end = ntokens; i < end; ++i ) {
    int flags = VectorAny_ArrayOpConstT(&wsflags,i/8,int);
    if( VectorAny_ArrayOpConstT(&tokens,i,Token).m_isws ) {
      flags |= 1<<(i%8);
      VectorAny_set(&wsflags,i/8,&flags);
    }
  }
  // Why waste a whole byte for a bit?
  lang->outArrayDecl(lang,out, "static const unsigned char","isws");
  fputs(" = ",out);
  lang->outStartArray(lang,out);
  first = true;
  for( int i = 0, end = nwsflags; i < end; ++i ) {
      if( first )
        first = false;
      else
        fputc(',',out);
      int flags = VectorAny_ArrayOpConstT(&wsflags,i,int);
      lang->outInt(lang,out,flags);
  }
  lang->outEndArray(lang,out);
  lang->outEndStmt(lang,out);
  fputc('\n',out);

  lang->outIntDecl(lang,out,"stateCount",Nfa_stateCount(dfa));

  OutputStateInfo(out, dfa, lang);

  fputs("\n",out);
  outBottom(lang,out,hassections);
  Scope_Pop();
}

void OutputTokenizerSource(FILE *out, const Nfa *dfa, LanguageOutputOptions *options) {
  LanguageOutputter outputter;
  LanguageOutputter_init(&outputter,options);
  if( options->outputLanguage == OutputLanguage_C )
    LanguageOutputOptions_import(options, "tokenizer", 0);
  if( options->outputLanguage == OutputLanguage_Python ) {
    LanguageOutputOptions_import(options, "typing", 0);
    LanguageOutputOptions_importitem(options, "Sequence", 0);
  }
  OutputDfaSource(out,dfa,&outputter);
  fputc('\n',out);
}
