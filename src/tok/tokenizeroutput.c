#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "tokenizer.h"

struct LanguageOutputter;
typedef struct LanguageOutputter LanguageOutputter;

struct LanguageOutputter {
  void (*outTop)(const LanguageOutputter *This, FILE *out);
  void (*outBottom)(const LanguageOutputter *This, FILE *out, bool hassections);
  void (*outDecl)(const LanguageOutputter *This, FILE *out, const char *type, const char *name);
  void (*outIntDecl)(const LanguageOutputter *This, FILE *out, const char *name, int i);
  void (*outArrayDecl)(const LanguageOutputter *This, FILE *out, const char *type, const char *name);
  void (*outStartArray)(const LanguageOutputter *This, FILE *out);
  void (*outEndArray)(const LanguageOutputter *This, FILE *out);
  void (*outEndStmt)(const LanguageOutputter *This, FILE *out);
  void (*outNull)(const LanguageOutputter *This, FILE *out);
  void (*outBool)(const LanguageOutputter *This, FILE *out, bool b);
  void (*outStr)(const LanguageOutputter *This, FILE *out, const char *str);
  void (*outChar)(const LanguageOutputter *This, FILE *out, int c);
  void (*outInt)(const LanguageOutputter *This, FILE *out, int i);
  int  (*encodeuint)(const LanguageOutputter *lang, FILE *out, unsigned int i);
  const char *prefix;
  const char *name;
  bool minimal;
};

void CLanguageOutputter_outTop(const LanguageOutputter* This, FILE* out) {
  if( This->minimal )
    return;
  fprintf(out, "#ifndef __%s_h\n", This->name);
  fprintf(out, "#define __%s_h\n", This->name);
  fputs("#include \"tokenizer.h\"\n\n", out);
}
void CLanguageOutputter_outBottom(const LanguageOutputter* This, FILE* out, bool hassections) {
  if( This->minimal )
    return;
  fprintf(out, "struct tokinfo %stkinfo = {\n", This->prefix);
  fprintf(out, "  %stokenCount,\n", This->prefix);
  fprintf(out, "  %ssectionCount,\n", This->prefix);
  if( hassections ) {
    fprintf(out, "  %ssectioninfo,\n", This->prefix);
    fprintf(out, "  %ssectioninfo_offset,\n", This->prefix);
  } else {
    fputs("  0,\n  0,\n", out);
  }
  fprintf(out, "  %stokenstr,\n", This->prefix);
  fprintf(out, "  %sisws,\n", This->prefix);
  fprintf(out, "  %sstateCount,\n", This->prefix);
  fprintf(out, "  %sstateinfo,\n", This->prefix);
  fprintf(out, "  %sstateinfo_offset,\n", This->prefix);
  fputs("};\n", out);
  fprintf(out, "#endif // __%s_h\n", This->name);
}
void CLanguageOutputter_outDecl(const LanguageOutputter *This, FILE *out, const char *type, const char *name) {
  fprintf(out, "%s %s%s", type, This->prefix, name);
}
void CLanguageOutputter_outIntDecl(const LanguageOutputter *This, FILE *out, const char *name, int i) {
  fprintf(out, "#define %s (%d)\n", name, i);
}
void CLanguageOutputter_outArrayDecl(const LanguageOutputter *This, FILE *out, const char *type, const char *name) {
  fprintf(out, "%s %s%s[]", type, This->prefix, name);
}
void CLanguageOutputter_outStartArray(const LanguageOutputter *This, FILE *out) { fputc('{',out); }
void CLanguageOutputter_outEndArray(const LanguageOutputter *This, FILE *out) { fputc('}',out); }
void CLanguageOutputter_outEndStmt(const LanguageOutputter *This, FILE *out) { fputc(';',out); }
void CLanguageOutputter_outNull(const LanguageOutputter *This, FILE *out) { fputc('0',out); }
void CLanguageOutputter_outBool(const LanguageOutputter *This, FILE *out, bool b) { fputs((b ? "true" : "false"),out); }
void CLanguageOutputter_outStr(const LanguageOutputter *This, FILE *out, const char *str)  {
  fprintf(out, "\"%s\"", str);
}
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

static int modup(int lhs, int rhs) {
  return lhs/rhs+((lhs%rhs)?1:0);
}

static unsigned int lshift(unsigned int i, unsigned int b) {
  // workaround bit-shift quirks
  if( b==0 )
    return i;
  else if( b >= 32 )
    return 0;
  return i<<b;
}

static unsigned int rshift(unsigned int i, unsigned int b) {
  // workaround bit-shift quirks
  if( b==0 )
    return i;
  else if( b >= 32 )
    return 0;
  return i>>b;
}

// A simple encoding scheme for unsigned 4 byte integers.
// In practice, almost all integers where we use this end
// up being 1 byte encodings, but we have to handle up to
// 4 byte integers, so we save 75% typically.
// First bytes contains...
// 1 byte encoding 0xxxxxxx
// 2 byte encoding 10xxxxxx xxxxxxxx
// 3 byte encoding 110xxxxx xxxxxxxx xxxxxxxx
// 4 byte encoding 1110xxxx xxxxxxxx xxxxxxxx xxxxxxxx
// 5 byte encoding 111110xx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx
// Note: first byte of 5 byte encoding will be 11111000 due to
// all 32 bits being consumed in the final 4 bytes.
static int CLanguageOutputter_encodeuint(const LanguageOutputter *lang, FILE *out, unsigned int i) {
  if( i == 0 ) {
    lang->outInt(lang,out,0);
    return 1;
  }
  unsigned int nbits = 0;
  unsigned char c = 0;
  while( (lshift(0xffffffff,nbits)&i) != 0 ) ++nbits;
  unsigned int bytes = modup(nbits,7);
  unsigned int mask = 0;
  unsigned int first = 8-bytes;
  for( unsigned int b = bytes; b > 0; --b ) {
    if( b == bytes ) {
      mask = 0xff & ~lshift(0xff,first);
      c = 0xff & lshift(0xff,first+1);
    } else {
      mask = 0xff;
      fputc(',',out);
    }
    c |= mask & rshift(i,(b-1)*8);
    lang->outInt(lang,out,c);
  } 
  return bytes;
}

LanguageOutputter CLanguageOutputter = {CLanguageOutputter_outTop, CLanguageOutputter_outBottom, CLanguageOutputter_outDecl, CLanguageOutputter_outIntDecl, CLanguageOutputter_outArrayDecl, CLanguageOutputter_outStartArray, CLanguageOutputter_outEndArray, CLanguageOutputter_outEndStmt, CLanguageOutputter_outNull, CLanguageOutputter_outBool, CLanguageOutputter_outStr, CLanguageOutputter_outChar, CLanguageOutputter_outInt, CLanguageOutputter_encodeuint};

static const char *pytype(const char *type) {
  if( strstr(type,"int") )
    return "int";
  if( strstr(type,"bool") )
    return "bool";
  if( strstr(type,"unsigned char") )
    return "int";
  if( strstr(type,"char") )
    return "str";
  return "Any";
}

void PyLanguageOutputter_outTop(const LanguageOutputter* This, FILE* out) { fputs("from typing import Sequence\n", out); }
void PyLanguageOutputter_outBottom(const LanguageOutputter* This, FILE* out, bool hassections) {}
void PyLanguageOutputter_outDecl(const LanguageOutputter* This, FILE* out, const char* type, const char* name) { fputs(name, out); fputs(": ",out); fputs(pytype(type), out); }
void PyLanguageOutputter_outIntDecl(const LanguageOutputter* This, FILE* out, const char* name, int i) {
  fprintf(out, "%s: int = %d\n", name, i);
}
void PyLanguageOutputter_outArrayDecl(const LanguageOutputter* This, FILE* out, const char* type, const char* name) { fputs(name, out); fputs(": Sequence[", out);  fputs(pytype(type), out); fputs("]", out); }
void PyLanguageOutputter_outStartArray(const LanguageOutputter* This, FILE* out) { fputc('[', out); }
void PyLanguageOutputter_outEndArray(const LanguageOutputter* This, FILE* out) { fputc(']', out); }
void PyLanguageOutputter_outEndStmt(const LanguageOutputter* This, FILE* out) {}
void PyLanguageOutputter_outNull(const LanguageOutputter* This, FILE* out) { fputc('0', out); }
void PyLanguageOutputter_outBool(const LanguageOutputter* This, FILE* out, bool b) { fputs((b ? "True" : "False"), out); }
void PyLanguageOutputter_outStr(const LanguageOutputter* This, FILE* out, const char* str) { fputc('\'', out); fputs(str, out); fputc('\'', out); }
void PyLanguageOutputter_outChar(const LanguageOutputter* This, FILE* out, int c) {
  if (c == '\r') {
    fputs("ord('\\r')", out);
  }
  else if (c == '\n') {
    fputs("ord('\\n')", out);
  }
  else if (c == '\v') {
    fputs("ord('\\v')", out);
  }
  else if (c == ' ') {
    fputs("ord(' ')", out);
  }
  else if (c == '\t') {
    fputs("ord('\\t')", out);
  }
  else if (c == '\\' || c == '\'') {
    fprintf(out, "ord('\\%c')", c);
  }
  else if (c <= 126 && isgraph(c)) {
    fprintf(out, "ord('%c')", c);
  }
  else
    fprintf(out, "%d", c);
}
void PyLanguageOutputter_outInt(const LanguageOutputter* This, FILE* out, int i) { fprintf(out, "%d", i); }

static int PyLanguageOutputter_encodeuint(const LanguageOutputter *lang, FILE *out, unsigned int i) {
  // don't bother encoding in python
  lang->outInt(lang,out,i);
  return 1;
}

LanguageOutputter PyLanguageOutputter = { PyLanguageOutputter_outTop, PyLanguageOutputter_outBottom, PyLanguageOutputter_outDecl, PyLanguageOutputter_outIntDecl, PyLanguageOutputter_outArrayDecl, PyLanguageOutputter_outStartArray, PyLanguageOutputter_outEndArray, PyLanguageOutputter_outEndStmt, PyLanguageOutputter_outNull, PyLanguageOutputter_outBool, PyLanguageOutputter_outStr, PyLanguageOutputter_outChar, PyLanguageOutputter_outInt, PyLanguageOutputter_encodeuint};

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
      cnt += lang->encodeuint(lang,out,token->m_token);
      fputc(',',out);
      cnt += lang->encodeuint(lang,out,action->m_action);
      fputc(',',out);
      cnt += lang->encodeuint(lang,out,action->m_actionarg);
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
  lang->outArrayDecl(lang,out,"static const int","sectioninfo_offset");
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
  if( first )
    first = false;
  else
    fputc(',',out);
  lang->outInt(lang,out,secoffset);
  lang->outEndArray(lang,out);
  lang->outEndStmt(lang,out);
  fputc('\n',out);
  Scope_Pop();
  return true;
}

static void OutputDfaSource(FILE *out, const Nfa *dfa, const LanguageOutputter *lang) {
  bool first = true;
  // Going to assume there are no gaps in toking numbering
  VectorAny /*<Token>*/ tokens;
  VectorAny /*<int>*/ wsflags;
  MapAny /*<int,int>*/ transitioncounts;
  Scope_Push();
  VectorAny_init(&tokens,getTokenElement(),true);
  VectorAny_init(&wsflags,getIntElement(),true);
  MapAny_init(&transitioncounts,getIntElement(),getIntElement(),true);
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
  int nwsflags = modup(ntokens,8);
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

  lang->outArrayDecl(lang,out,"static const unsigned char","stateinfo");
  fputs(" = ",out);
  lang->outStartArray(lang,out);
  first = true;
  MapAny_clear(&transitioncounts);
  // there typically aren't long runs of repeats, and values are usually
  // small, so we use a simple encoding scheme to save space.
  int lastfrom = -1;
  for( int cur = 0, end = MapAny_size(&dfa->m_transitions); cur < end; ++cur ) {
    const Transition *transition = 0;
    const CharSet *ranges = 0;
    int cnt = 0;
    MapAny_getByIndexConst(&dfa->m_transitions,cur,(const void**)&transition,(const void**)&ranges);
    if( first )
      first = false;
    else
      fputc(',',out);
    if( lastfrom != transition->m_from ) {
      while( ++lastfrom < transition->m_from ) {
        if( MapAny_find(&transitioncounts,&lastfrom) )
          continue;
        int tokfrm = Nfa_getStateToken(dfa,lastfrom);
        if( tokfrm != -1 ) {
          int tokcnt = lang->encodeuint(lang,out,tokfrm+1);
          fputc(',',out);
          MapAny_insert(&transitioncounts,&lastfrom,&tokcnt);
        }
      }
      int tok = Nfa_getStateToken(dfa,transition->m_from);
      // -1 is quite common, add one to shrink encoding
      cnt += lang->encodeuint(lang,out,tok+1);
      fputc(',',out);
    }
    cnt += lang->encodeuint(lang,out,transition->m_to);
    fputc(',',out);
    cnt += lang->encodeuint(lang,out,CharSet_size(ranges));
    // range values increase, so subtracting the last value helps encoding
    int last = 0;
    for( int cur = 0, end = CharSet_size(ranges); cur != end; ++cur ) {
      const CharRange *curRange = CharSet_getRange(ranges,cur);
      fputc(',',out);
      cnt += lang->encodeuint(lang,out,curRange->m_low-last);
      last = curRange->m_low;
      fputc(',',out);
      cnt += lang->encodeuint(lang,out,curRange->m_high-last);
      last = curRange->m_high;
    }
    // keeping count of how many values are in each stateinfo
    if( ! MapAny_find(&transitioncounts,&transition->m_from) )
      MapAny_insert(&transitioncounts,&transition->m_from,&cnt);
    else
      MapAny_findT(&transitioncounts,&transition->m_from,int) += cnt;
  }
  lang->outEndArray(lang,out);
  lang->outEndStmt(lang,out);
  fputc('\n',out);

  // stateinfo is variable length, this fixed size index lets us
  // jump right to the data.  It is possible to recreate this
  // from the stateinfo, and maybe we will one day, but for now
  // prefer to just write this.
  int offset = 0;
  lang->outArrayDecl(lang,out,"static const int","stateinfo_offset");
  fputs(" = ",out);
  lang->outStartArray(lang,out);
  first = true;
  for( int i = 0, n = Nfa_stateCount(dfa); i < n; ++i ) {
    if( first )
      first = false;
    else
      fputc(',',out);
    lang->outInt(lang,out,offset);
    if( MapAny_findConst(&transitioncounts,&i) )
      offset += MapAny_findConstT(&transitioncounts,&i,int);
  }
  if( first )
    first = false;
  else
    fputc(',',out);
  lang->outInt(lang,out,offset);
  lang->outEndArray(lang,out);
  lang->outEndStmt(lang,out);
  fputc('\n',out);

  fputs("\n",out);
  lang->outBottom(lang,out,hassections);
  Scope_Pop();
}

void OutputTokenizerSource(FILE *out, const Nfa *dfa, const char *name, const char *prefix, bool py, bool minimal) {
  LanguageOutputter *outputer = py ? &PyLanguageOutputter : &CLanguageOutputter;
  outputer->name = name;
  outputer->prefix = prefix?prefix:"";
  outputer->minimal = minimal;
  OutputDfaSource(out,dfa,outputer);
  fputc('\n',out);
}
