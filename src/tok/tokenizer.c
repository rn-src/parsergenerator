#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <stdlib.h>
#include "tokenizer.h"

ElementOps ActionElement = {sizeof(Action), false, false, 0, 0, 0, 0, 0, 0};
ElementOps TransitionElement = {sizeof(Transition), false, false, (elementInit)Transition_init, 0, (elementLessThan)Transition_LessThan, (elementEqual)Transition_Equal, 0, 0};
ElementOps CharSetElement = {sizeof(CharSet), false, false, (elementInit)CharSet_init, (elementDestroy)CharSet_destroy, (elementLessThan)CharSet_LessThan, (elementEqual)CharSet_Equal, (elementAssign)CharSet_Assign, 0};
ElementOps CharRangeElement = {sizeof(CharRange), false, false, 0, 0, (elementLessThan)CharRange_LessThan, 0, 0, 0};
ElementOps TokenElement = {sizeof(Token), false, false, (elementInit)Token_init, (elementDestroy)Token_destroy, (elementLessThan)Token_LessThan, (elementEqual)Token_Equal, (elementAssign)Token_Assign, 0};

// rx : simplerx | rx rx | rx '+' | rx '*' | rx '?' | rx '|' rx | '(' rx ')' | rx '{' number '}' | rx '{' number ',' '}' | rx '{' number ',' number '}' | rx '{' ',' number '}'
// simplerx : /[0-9A-Za-z \t]/ | '\\[trnv]' | /\\[.]/ | /(\\x|\\u)[[:xdigit:]]+/ | /./ | charset
// charset : '[' charsetelements ']' | '[^' charsetelements ']'
// charsetelements : charsetelements charsetelement
// charsetelement : char '-' char | char | namedcharset
// namedcharset : '[:' charsetname ':]'
// charsetname : 'alpha' | 'digit' | 'xdigit' | 'alnum' | 'white' | 'space'
// char : /[0-9A-Za-z]/ | /\\[trnv]/ | /\\./ | /\\[xu][[:xdigit:]]+/

static int xvalue(char c) {
  if(c >= '0' && c <= '9')
    return c-'0';
  if(c >= 'a' && c <= 'f')
    return c-'a'+10;
  if(c >= 'A' || c <= 'F')
    return c-'A'+10;
  return -1;
}

void Token_init(Token *This, bool onstack) {
  This->m_token = -1;
  This->m_isws = false;
  String_init(&This->m_name, false);
  MapAny_init(&This->m_actions, getIntElement(), &ActionElement, false);
  if( onstack )
    Push_Destroy(This,(vpstack_destroyer)Token_destroy);
}

void Token_destroy(Token *This) {
  String_clear(&This->m_name);
  MapAny_destroy(&This->m_actions);
}

bool Token_LessThan(const Token *lhs, const Token *rhs) {
  return lhs->m_token < rhs->m_token;
}

bool Token_Equal(const Token *lhs, const Token *rhs) {
  return lhs->m_token == rhs->m_token;
}

void Token_Assign(Token *lhs, const Token *rhs) {
  lhs->m_token = rhs->m_token;
  lhs->m_isws = rhs->m_isws;
  String_AssignString(&lhs->m_name,&rhs->m_name);
  MapAny_Assign(&lhs->m_actions,&rhs->m_actions);
}

static int compareTokens(const void *lhs, const void *rhs) {
  if( Token_LessThan(lhs,rhs) )
    return -1;
  if( Token_LessThan(rhs,lhs) )
    return 1;
  return 0;
}

bool TokStream_fill(TokStream *This) {
  if( feof(This->m_in) )
    return false;
  if( This->m_buffill == This->m_buflen ) {
    if( This->m_buflen == 0 ) {
      int newlen = 256;
      char *newbuf = (char*)malloc(newlen+1);
      memset(newbuf,0,newlen+1);
      This->m_buf = newbuf;
      This->m_next = This->m_buf;
      This->m_buflen = newlen;
    } else {
      size_t newlen = This->m_buflen*2;
      char *newbuf = (char*)malloc(newlen+1);
      memset(newbuf,0,newlen+1);
      for( int i = 0; i < This->m_buffill; ++i ) {
        int oldpos = (This->m_bufpos+i)%This->m_buflen;
        newbuf[i] = This->m_buf[oldpos];
      }
      free(This->m_buf);
      This->m_buf = newbuf;
      This->m_next = This->m_buf;
      This->m_buflen = newlen;
      This->m_bufpos = 0;
    }
  }
  size_t totalread = 0;
  while( This->m_buffill < This->m_buflen ) {
    int fillpos = (This->m_bufpos+This->m_buffill)%This->m_buflen;
    int len = (This->m_bufpos > fillpos) ? (This->m_bufpos-fillpos) : This->m_buflen-fillpos;
    size_t nread = fread(This->m_buf+fillpos,1,len,This->m_in);
    totalread += nread;
    This->m_buffill += nread;
    if( nread < len )
      break;
  }
  if( totalread == 0 )
    return false;
  return true;
}

void TokStream_init(TokStream *This, FILE *in, bool onstack) {
  This->m_buf = 0;
  This->m_next = 0;
  This->m_buffill = 0;
  This->m_buflen = 0;
  This->m_bufpos = 0;
  This->m_in = in;
  This->m_pos = 0;
  This->m_line = This->m_col = 1;
  if( onstack )
    Push_Destroy(This,(vpstack_destroyer)TokStream_destroy);
}
void TokStream_destroy(TokStream *This) {
  if( This->m_buf )
    free(This->m_buf);
}

int TokStream_peekc(TokStream *This, int n) {
  while( n >= This->m_buffill ) {
    if( ! TokStream_fill(This) )
      return -1;
  }
  return This->m_buf[(This->m_bufpos+n)%This->m_buflen];
}

bool TokStream_peekstr(TokStream *This, const char *s) {
  int slen = (int)strlen(s);
  TokStream_peekc(This,slen-1);
  for( int i = 0; i < slen; ++i )
    if( TokStream_peekc(This,i) != s[i] )
      return false;
  return true;
}

int TokStream_readc(TokStream *This) {
  if( This->m_buffill == 0 )
    if( ! TokStream_fill(This) )
      return -1;
  int c = This->m_next[0];
  This->m_bufpos = (This->m_bufpos+1)%This->m_buflen;
  This->m_next = This->m_buf+This->m_bufpos;
  --This->m_buffill;
  if( c == '\n' ) {
    ++This->m_line;
    This->m_col = 1;
  } else
    ++This->m_col;
  return c;
}

void TokStream_discard(TokStream *This, int n) {
  for( int i = 0; i < n; ++i )
    TokStream_readc(This);
}

int TokStream_line(TokStream *This) {
  return This->m_line;
}

int TokStream_col(TokStream *This) {
  return This->m_col;
}

static ParseError g_err;
bool g_errInit = false;
bool g_errSet = false;

void setParseError(int line, int col, const String *err) {
  if( ! g_errInit ) {
    String_init(&g_err.err,false);
    g_errInit = true;
  }
  clearParseError();
  g_err.line = line;
  g_err.col = col;
  String_AssignString(&g_err.err,err);
  g_errSet = true;
  Scope_LongJmp(1);
}

bool getParseError(const ParseError **err) {
  if( ! g_errSet )
    return false;
  *err = &g_err;
  return true;
}

void clearParseError() {
  if( g_errInit )
    String_clear(&g_err.err);
  g_errSet = false;
}

static void error(TokStream *s, const char *err) {
  String errstr;
  Scope_Push();
  String_init(&errstr,true);
  String_AssignChars(&errstr,err);
  setParseError(TokStream_line(s),TokStream_col(s),&errstr);
  Scope_Pop();
}

static void errorString(TokStream *s, const String *err) {
  setParseError(TokStream_line(s),TokStream_col(s),err);
}

bool CharRange_LessThan(const CharRange *lhs, const CharRange *rhs) {
  if( lhs->m_low < rhs->m_low )
    return true;
  if( lhs->m_low > rhs->m_low )
    return false;
  if( lhs->m_high < rhs->m_high )
    return true;
  if( lhs->m_high > rhs->m_high )
    return false;
  return false;
}

CharRange *CharRange_SetRange(CharRange *This, int low, int high) {
  This->m_low = low;
  This->m_high = high;
  return This;
}

bool CharRange_ContainsChar(const CharRange *This, int i) {
  if( i >= This->m_low && i <= This->m_high )
    return true;
  return false;
}

bool CharRange_ContainsCharRange(const CharRange *This, const CharRange *rhs) {
  if( rhs->m_low >= This->m_low && rhs->m_high <= This->m_high )
    return true;
  return false;
}

bool CharRange_OverlapsRange(const CharRange *This, int low, int high) {
  if( low > This->m_high || high < This->m_low )
    return false;
  return true;
}

void CharSet_init(CharSet *This, bool onstack) {
  VectorAny_init(&This->m_ranges,&CharRangeElement, false);
  if( onstack )
    Push_Destroy(This,(vpstack_destroyer)CharSet_destroy);
}

void CharSet_destroy(CharSet *This) {
  VectorAny_destroy(&This->m_ranges);
}

bool CharSet_LessThan(const CharSet *lhs, const CharSet *rhs) {
  return VectorAny_LessThan(&lhs->m_ranges,&rhs->m_ranges);
}

bool CharSet_Equal(const CharSet *lhs, const CharSet *rhs) {
  return VectorAny_Equal(&lhs->m_ranges,&rhs->m_ranges);
}

void CharSet_Assign(CharSet *lhs, const CharSet *rhs) {
  VectorAny_Assign(&lhs->m_ranges,&rhs->m_ranges);
}

int CharSet_find(const CharSet *This, int c, bool *found) {
  int low = 0, high = VectorAny_size(&This->m_ranges)-1;
  while( low <= high ) {
    int mid = (low+high)/2;
    const CharRange *range = (const CharRange*)VectorAny_ArrayOpConst(&This->m_ranges,mid);
    if( CharRange_ContainsChar(range,c) ) {
      *found = true;
      return mid;
    }
    if( c < range->m_low )
      high = mid-1;
    else if( c > range->m_high )
      low = mid+1;
    else {
      *found = true;
      return mid;
    }
  }
  *found = false;
  return low;
}

bool CharSet_ContainsCharRange(const CharSet *This, const CharRange *range) {
  bool found = false;
  int i = CharSet_find(This,range->m_low,&found);
  if( ! found )
    return false;
  return CharRange_ContainsCharRange((const CharRange*)VectorAny_ArrayOpConst(&This->m_ranges,i),range);
}

void CharSet_clear(CharSet *This) {
  VectorAny_clear(&This->m_ranges);
}

bool CharSet_empty(const CharSet *This) {
  return VectorAny_empty(&This->m_ranges);
}

void CharSet_addChar(CharSet *This, int i) {
  CharSet_addCharRange(This,i,i);
}

void CharSet_addCharRange(CharSet *This, int low, int high) {
  bool found;
  CharRange range;
  int i = CharSet_find(This,low,&found);
  if( found ) {
    if( low < VectorAny_ArrayOpT(&This->m_ranges,i,CharRange).m_low )
      VectorAny_ArrayOpT(&This->m_ranges,i,CharRange).m_low = low;
    if( high > VectorAny_ArrayOpT(&This->m_ranges,i,CharRange).m_high )
      VectorAny_ArrayOpT(&This->m_ranges,i,CharRange).m_high = high;
  } else {
    VectorAny_insert(&This->m_ranges,i,CharRange_SetRange(&range,low,high));
  }
  while( i < VectorAny_size(&This->m_ranges)-1 && (VectorAny_ArrayOpT(&This->m_ranges,i+1,CharRange).m_low-VectorAny_ArrayOpT(&This->m_ranges,i,CharRange).m_high <= 1) ) {
    VectorAny_ArrayOpT(&This->m_ranges,i,CharRange).m_high = VectorAny_ArrayOpT(&This->m_ranges,i+1,CharRange).m_high;
    VectorAny_erase(&This->m_ranges,i+1);
  }
  if( i > 0 ) {
    while( i < VectorAny_size(&This->m_ranges) && (VectorAny_ArrayOpT(&This->m_ranges,i,CharRange).m_low-VectorAny_ArrayOpT(&This->m_ranges,i-1,CharRange).m_high <= 1) ) {
      VectorAny_ArrayOpT(&This->m_ranges,i-1,CharRange).m_high = VectorAny_ArrayOpT(&This->m_ranges,i,CharRange).m_high;
      VectorAny_erase(&This->m_ranges,i);
    }
  }
}
void CharSet_addCharSet(CharSet *This, const CharSet *rhs) {
  for( int cur = 0, end = VectorAny_size(&rhs->m_ranges); cur != end; ++cur ) {
    const CharRange *curRange = &VectorAny_ArrayOpConstT(&rhs->m_ranges,cur,CharRange);
    CharSet_addCharRange(This, curRange->m_low, curRange->m_high);
  }
}
//#define splitTEST
void CharSet_splitRange(CharSet *This, int low, int high) {
#ifdef splitTEST
  Vector<CharRange> rangesBefore = m_ranges;
  splitReal(low,high,-1);
  bool overlap = false;
  CharRange last(-1,-1);
  for( int i = 0, n = m_ranges.size(); i < n; ++i ) {
    if( m_ranges[i].m_low <= last.m_high ) {
      overlap = true;
      m_ranges = rangesBefore;
      splitReal(low,high,-1);
      break;
    }
    last = m_ranges[i];
  }
#else
  CharSet_splitRangeRecursive(This,low,high,-1);
#endif
}

int CharSet_splitRangeRecursive(CharSet *This, int low, int high, int i) {
  CharRange range;
  if( i == -1 ) {
    bool found = false;
    i = CharSet_find(This,low,&found);
  }
  if( i == VectorAny_size(&This->m_ranges) || ! CharRange_OverlapsRange(&VectorAny_ArrayOpT(&This->m_ranges,i,CharRange),low,high) ) {
    VectorAny_insert(&This->m_ranges,i,CharRange_SetRange(&range,low,high));
    return 1;
  }

  int adjust = 0;
  range = VectorAny_ArrayOpT(&This->m_ranges,i,CharRange);

  if( low < range.m_low ) {
    int tmp = CharSet_splitRangeRecursive(This,low,range.m_low-1,i);
    i += tmp;
    adjust += tmp;
  } else if( low > range.m_low ) {
    VectorAny_ArrayOpT(&This->m_ranges,i,CharRange).m_low = low;
    int tmp = CharSet_splitRangeRecursive(This,range.m_low,low-1,i);
    i += tmp;
    adjust += tmp;
  }

  if( high < range.m_high ) {
    VectorAny_ArrayOpT(&This->m_ranges,i,CharRange).m_high = high;
    CharSet_splitRangeRecursive(This,high+1,range.m_high,i+1);
  } else if( high > range.m_high ) {
    CharSet_splitRangeRecursive(This,range.m_high+1,high,i+1);
  }

  return adjust;
}

void CharSet_splitCharSet(CharSet *This, const CharSet *rhs) {
  for( int cur = 0, end = VectorAny_size(&rhs->m_ranges); cur != end; ++cur ) {
    const CharRange *curRange = &VectorAny_ArrayOpConstT(&rhs->m_ranges,cur,CharRange);
    CharSet_splitRange(This, curRange->m_low, curRange->m_high);
  }
}

void CharSet_negate(CharSet *This) {
  VectorAny /*<CharRange>*/ ranges;
  CharRange range;
  Scope_Push();
  VectorAny_init(&ranges,&CharRangeElement,true);
  int prev = 0;
  for( int cur = 0, end = VectorAny_size(&This->m_ranges); cur != end; ++cur ) {
    CharRange *curRange = &VectorAny_ArrayOpT(&This->m_ranges,cur,CharRange);
    if( prev < curRange->m_low )
      VectorAny_push_back(&ranges,CharRange_SetRange(&range,prev,curRange->m_low-1));
    prev = curRange->m_high+1;
  }
  if( prev < INT_MAX )
    VectorAny_push_back(&ranges,CharRange_SetRange(&range,prev,INT_MAX));
  VectorAny_Assign(&This->m_ranges,&ranges);
  Scope_Pop();
}

int CharSet_size(const CharSet *This) {
  return VectorAny_size(&This->m_ranges);
}

const CharRange *CharSet_getRange(const CharSet *This, int i) {
  return &VectorAny_ArrayOpConstT(&This->m_ranges,i,CharRange);
}

void Transition_init(Transition *This, bool onstack) {
  This->m_from = This->m_to = 0;
}

Transition *Transition_SetFromTo(Transition *This, int from, int to) {
  This->m_from = from;
  This->m_to = to;
  return This;
}

bool Transition_LessThan(const Transition *lhs, const Transition *rhs) {
  if( lhs->m_from < rhs->m_from )
    return true;
  if( lhs->m_from == rhs->m_from &&  lhs->m_to < rhs->m_to)
    return true;
  return false;
}

bool Transition_Equal(const Transition *lhs, const Transition *rhs) {
  return lhs->m_from == rhs->m_from && lhs->m_to == rhs->m_to;
}

void Nfa_destroy(Nfa *This);

void Nfa_init(Nfa *This, bool onstack) {
  This->m_nextState = 0;
  This->m_sections = 0;
  SetAny_init(&This->m_startStates, getIntElement(),false);
  SetAny_init(&This->m_endStates, getIntElement(),false);
  MapAny_init(&This->m_transitions, &TransitionElement, &CharSetElement,false);
  SetAny_init(&This->m_emptytransitions, &TransitionElement,false);
  MapAny_init(&This->m_state2token, getIntElement(), getIntElement(),false);
  MapAny_init(&This->m_tokendefs, getIntElement(), &TokenElement,false);
  if(onstack)
    Push_Destroy(This,(vpstack_destroyer)Nfa_destroy);
}

void Nfa_destroy(Nfa *This) {
  SetAny_destroy(&This->m_startStates);
  SetAny_destroy(&This->m_endStates);
  MapAny_destroy(&This->m_transitions);
  SetAny_destroy(&This->m_emptytransitions);
  MapAny_destroy(&This->m_state2token);
  MapAny_destroy(&This->m_tokendefs);
}

void Nfa_addNfa(Nfa *This, const Nfa *nfa) {
  int states = This->m_nextState;
  This->m_nextState += Nfa_stateCount(nfa);
  for( int cur = 0, end = SetAny_size(&nfa->m_startStates); cur != end; ++cur ) {
    int curStartState = SetAny_getByIndexConstT(&nfa->m_startStates,cur,int);
    int adjustedState = curStartState+states;
    SetAny_insert(&This->m_startStates,&adjustedState,0);
  }
  for( int cur = 0, end = SetAny_size(&nfa->m_endStates); cur != end; ++cur ) {
    int sumState = SetAny_getByIndexConstT(&nfa->m_endStates,cur,int)+states;
    SetAny_insert(&This->m_endStates,&sumState,0);
  }
  for( int cur = 0, end = MapAny_size(&nfa->m_transitions); cur != end; ++cur ) {
    const Transition *key = 0;
    const CharSet *value = 0;
    MapAny_getByIndexConst(&nfa->m_transitions,cur,(const void**)&key,(const void**)&value);
    Transition nextKey;
    MapAny_insert(&This->m_transitions,Transition_SetFromTo(&nextKey,key->m_from+states,key->m_to+states),value);
  }
  for( int cur = 0, end = SetAny_size(&nfa->m_emptytransitions); cur != end; ++cur ) {
    const Transition *key = &SetAny_getByIndexConstT(&nfa->m_emptytransitions,cur,Transition);
    Transition nextKey;
    SetAny_insert(&This->m_emptytransitions,Transition_SetFromTo(&nextKey,key->m_from+states,key->m_to+states),0);
  }
  for( int cur = 0, end = MapAny_size(&nfa->m_state2token); cur != end; ++cur ) {
    const int *key = 0, *value = 0;
    MapAny_getByIndexConst(&nfa->m_state2token,cur,(const void**)&key,(const void**)&value);
    int updatedState = *key+states;
    MapAny_insert(&This->m_state2token, &updatedState, value);
  }
  for( int cur = 0, end = MapAny_size(&nfa->m_tokendefs); cur != end; ++cur ) {
    int *key = 0;
    Token *value = 0;
    MapAny_getByIndexConst(&nfa->m_tokendefs,cur,(const void**)&key,(const void**)&value);
    Nfa_setTokenDef(This,value);
  }
}

int Nfa_stateCount(const Nfa *This) { return This->m_nextState; }
int Nfa_addState(Nfa *This) { return This->m_nextState++; }
void Nfa_addStartState(Nfa *This, int startstate) { SetAny_insert(&This->m_startStates,&startstate,0); }
void Nfa_addEndState(Nfa *This, int endstate) { SetAny_insert(&This->m_endStates,&endstate,0); }

void Nfa_addTransition(Nfa *This, int from, int to, int symbol) {
  Transition t;
  CharSet *value = &MapAny_findT(&This->m_transitions,Transition_SetFromTo(&t,from,to),CharSet);
  if( ! value ) {
    Scope_Push();
    CharSet charset;
    CharSet_init(&charset,true);
    CharSet_addChar(&charset,symbol);
    MapAny_insert(&This->m_transitions,&t,&charset);
    Scope_Pop();
  } else
    CharSet_addChar(value,symbol);
}

void Nfa_addTransitionCharRange(Nfa *This, int from, int to, const CharRange *range) {
  Scope_Push();
  CharSet charset;
  Transition t;
  CharSet *value = &MapAny_findT(&This->m_transitions,Transition_SetFromTo(&t,from,to),CharSet);
  CharSet_init(&charset,true);
  if( ! value ) {
    CharSet_clear(&charset);
    CharSet_addCharRange(&charset,range->m_low,range->m_high);
    MapAny_insert(&This->m_transitions,&t,&charset);
  } else
    CharSet_addCharRange(value,range->m_low,range->m_high);
  Scope_Pop();
}

void Nfa_addTransitionCharSet(Nfa *This, int from, int to, const CharSet *charset) {
  Transition t;
  CharSet *value = &MapAny_findT(&This->m_transitions,Transition_SetFromTo(&t,from,to),CharSet);
  if( ! value )
    MapAny_insert(&This->m_transitions,&t,charset);
  else
    CharSet_addCharSet(value,charset);
}

void Nfa_addEmptyTransition(Nfa *This, int from, int to) {
  Transition t;
  SetAny_insert(&This->m_emptytransitions,Transition_SetFromTo(&t,from,to),0);
}

bool Nfa_hasTokenDef(const Nfa *This, int token)  {
  return MapAny_findConst(&This->m_tokendefs,&token) != 0;
}

void Nfa_setTokenDef(Nfa *This, const Token *tok) {
  MapAny_insert(&This->m_tokendefs,&tok->m_token,tok);
}

bool Nfa_setTokenAction(Nfa *This, int token, int section, TokenAction action, int actionarg) {
  Token *tok = &MapAny_findT(&This->m_tokendefs,&token,Token);
  if( ! tok )
    return false;
  if( action == ActionNone )
    return true;
  Action tokaction;
  tokaction.m_action = action;
  tokaction.m_actionarg = actionarg;
  MapAny_insert(&tok->m_actions,&section,&tokaction);
  return true;
}

int Nfa_getSections(const Nfa *This) {
  return This->m_sections;
}

void Nfa_setSections(Nfa *This, int sections) {
  This->m_sections = sections;
}

const Token *Nfa_getTokenDef(const Nfa *This, int token) {
  return &MapAny_findConstT(&This->m_tokendefs,&token,Token);
}

void Nfa_getTokenDefs(const Nfa *This, VectorAny /*<Token>*/ *tokendefs) {
  const int *key = 0;
  const Token *tok = 0;
  for( int cur = 0, end = MapAny_size(&This->m_tokendefs); cur< end; ++cur ) {
    MapAny_getByIndexConst(&This->m_tokendefs,cur,(const void**)&key,(const void**)&tok);
    VectorAny_push_back(tokendefs,tok);
  }
  qsort(tokendefs->m_p,tokendefs->m_size,tokendefs->m_ops->elementSize,compareTokens);
}

bool Nfa_stateHasToken(const Nfa *This, int state) {
  return MapAny_findConst(&This->m_state2token,&state) != 0;
}

int Nfa_getStateToken(const Nfa *This, int state) {
  const int *value = &MapAny_findConstT(&This->m_state2token,&state,int);
  if( ! value )
    return -1;
  return *value;
}

void Nfa_setStateToken(Nfa *This, int state, int token) {
  int *pstate = &MapAny_findT(&This->m_state2token,&state,int);
  if( pstate )
    *pstate = token;
  else
    MapAny_insert(&This->m_state2token,&state,&token);
}

void Nfa_clear(Nfa *This) {
  This->m_nextState = 0;
  SetAny_clear(&This->m_startStates);
  SetAny_clear(&This->m_endStates);
  MapAny_clear(&This->m_transitions);
  SetAny_clear(&This->m_emptytransitions);
  MapAny_clear(&This->m_state2token);
  MapAny_clear(&This->m_tokendefs);
  This->m_sections = 1;
}

void Nfa_closure(const Nfa *This, const MapAny /*<int,Set<int>>*/ *emptytransitions, SetAny /*<int>*/ *states) {
  Scope_Push();
  SetAny /*<int>*/ nextstates;
  VectorAny /*<int>*/ vecstates;
  SetAny_init(&nextstates, getIntElement(), true);
  VectorAny_init(&vecstates, getIntElement(),true);
  SetAny_Assign(&nextstates, states);
  VectorAny_insertMany(&vecstates,VectorAny_size(&vecstates),nextstates.m_values.m_p,nextstates.m_values.m_size);
  for( int i = 0; i < VectorAny_size(&vecstates); ++i )
  {
    int stateno = VectorAny_ArrayOpConstT(&vecstates, i, int);
    const SetAny /*<int>*/ *intset = &MapAny_findConstT(emptytransitions,&stateno,SetAny);
    if( ! intset )
      continue;
    for( int cur = 0, end = SetAny_size(intset); cur != end; ++cur ) {
      const int *curstate = &SetAny_getByIndexConstT(intset,cur,int);
      if( SetAny_contains(&nextstates,curstate) )
        continue;
      SetAny_insert(&nextstates,curstate,0);
      VectorAny_push_back(&vecstates,curstate);
    }
  }
  SetAny_Assign(states,&nextstates);
  Scope_Pop();
}

void Nfa_follow(const Nfa *This, const CharRange *range, const SetAny /*<int>*/ *states, SetAny /*<int>*/ *nextstates ) {
  const Transition *key = 0;
  const CharSet *value = 0;
  SetAny_clear(nextstates);
  for( int cur = 0, end = MapAny_size(&This->m_transitions); cur != end; ++cur ) {
    MapAny_getByIndexConst(&This->m_transitions,cur,(const void**)&key,(const void**)&value);
    if( ! SetAny_findConst(states,&key->m_from) || ! CharSet_ContainsCharRange(value,range) )
      continue;
    SetAny_insert(nextstates,&key->m_to,0);
  }
}

void Nfa_stateTransitions(const Nfa *This, const SetAny /*<int>*/ *states, CharSet *transitions) {
  CharSet_clear(transitions);
  const Transition *key = 0;
  const CharSet *value = 0;
  for( int cur = 0, end = MapAny_size(&This->m_transitions); cur != end; ++cur ) {
    MapAny_getByIndexConst(&This->m_transitions,cur,(const void**)&key,(const void**)&value);
    const int *pstate = &SetAny_findConstT(states,&key->m_from,int);
    if( ! pstate )
      continue;
    CharSet_splitCharSet(transitions,value);
  }
}

bool Nfa_hasEndState(const Nfa *This, const SetAny /*<int>*/ *states) {
  for( int cur = 0, end = SetAny_size(states); cur != end; ++cur ) {
    int state = SetAny_getByIndexConstT(states,cur,int);
    if( SetAny_findConst(&This->m_endStates,&state) )
      return true;
  }
  return false;
}

int Nfa_lowToken(const Nfa *This, const SetAny /*<int>*/ *states) {
  int finaltok = -1;
  for( int cur = 0, end = SetAny_size(states); cur != end; ++cur ) {
    int state = SetAny_getByIndexConstT(states,cur,int);
    int tok = Nfa_getStateToken(This, state);
    if( tok == -1 )
      continue;
    else if( finaltok == -1 || tok < finaltok )
      finaltok = tok;
  }
  return finaltok;
}

void Nfa_toDfa(const Nfa *This, Nfa *dfa) {
  VectorAny /*< <Set<int>,int> >*/ newstates;
  MapAny /*< <Set<int>,int> >*/ state2num;
  SetAny /*<int>*/ nextstate;
  MapAny /*<int,Set<int> >*/ emptytransitions;
  CharSet transitions;
  SetAny /*<int>*/ state;
  SetAny setEmpty;

  Scope_Push();
  VectorAny_init(&newstates, getSetAnyElement(), true);
  MapAny_init(&state2num, getSetAnyElement(), getIntElement(), true);
  SetAny_init(&nextstate, getIntElement(), true);
  MapAny_init(&emptytransitions, getIntElement(), getSetAnyElement(), true);
  CharSet_init(&transitions, true);
  SetAny_init(&state,getIntElement(), true);
  SetAny_init(&setEmpty,getIntElement(),true);

  Nfa_clear(dfa);
  Nfa_setSections(dfa, This->m_sections);

  const Transition *curTransition = 0;
  const CharRange *curRange = 0;
  for( int cur = 0, end = SetAny_size(&This->m_emptytransitions); cur != end; ++cur ) {
    curTransition = &SetAny_getByIndexConstT(&This->m_emptytransitions,cur,Transition);
    if( ! MapAny_find(&emptytransitions,&curTransition->m_from) ) {
      SetAny_clear(&setEmpty);
      MapAny_insert(&emptytransitions, &curTransition->m_from, &setEmpty);
    }
    SetAny_insert(&MapAny_findT(&emptytransitions,&curTransition->m_from,SetAny), &curTransition->m_to,0);
  }
  SetAny_Assign(&nextstate,&This->m_startStates);
  Nfa_closure(This, &emptytransitions, &nextstate);
  int initStateNo = 0;
  VectorAny_push_back(&newstates, &nextstate);
  MapAny_insert(&state2num, &nextstate, &initStateNo);
  Nfa_addStartState(dfa, Nfa_addState(dfa));
  for( int stateno = 0; stateno < VectorAny_size(&newstates); ++stateno ) {
    SetAny_clear(&state);
    const SetAny *pstate = &VectorAny_ArrayOpConstT(&newstates, stateno, SetAny);
    SetAny_Assign(&state,pstate);
    CharSet_clear(&transitions);
    Nfa_stateTransitions(This, &state, &transitions);
    for( int cur = 0, end = CharSet_size(&transitions); cur != end; ++cur ) {
      curRange = CharSet_getRange(&transitions,cur);
      Nfa_follow(This, curRange, &state, &nextstate);
      Nfa_closure(This, &emptytransitions, &nextstate);
      if( SetAny_size(&nextstate) == 0 )
        continue;
      const int *pFoundState = &MapAny_findConstT(&state2num, &nextstate, int);
      int nextstateno = -1;
      if( ! pFoundState ) {
        nextstateno = Nfa_addState(dfa);
        VectorAny_push_back(&newstates, &nextstate);
        MapAny_insert(&state2num,&nextstate,&nextstateno);
        if( Nfa_hasEndState(This,&nextstate) )
          Nfa_addEndState(dfa,nextstateno);
        int token = Nfa_lowToken(This,&nextstate);
        if( token != -1 ) {
          if( ! Nfa_hasTokenDef(dfa,token) ) {
            const Token *tokendef = Nfa_getTokenDef(This,token);
            Nfa_setTokenDef(dfa,tokendef);
          }
          Nfa_setStateToken(dfa,nextstateno,token);
        }
      } else {
        nextstateno = *pFoundState;
      }   
      Nfa_addTransitionCharRange(dfa,stateno,nextstateno,curRange);
    }
  }
  Scope_Pop();
}

void Rx_destroy(Rx *This);

void Rx_init(Rx *This, bool onstack) {
  This->m_t = RxType_None;
  This->m_op = BinaryOp_None;
  This->m_lhs = This->m_rhs = 0;
  This->m_low = This->m_high = -1;
  CharSet_init(&This->m_charset, false);
  if( onstack )
    Push_Destroy(This,(vpstack_destroyer)Rx_destroy);
}

void Rx_destroy(Rx *This) {
  CharSet_destroy(&This->m_charset);
  if( This->m_lhs ) {
    Rx_destroy(This->m_lhs);
    This->m_lhs = 0;
  }
  if( This->m_rhs ) {
    Rx_destroy(This->m_rhs);
    This->m_rhs = 0;
  }
}

void Rx_init_FromChar(Rx *This, char c, bool onstack) {
  Rx_init(This,onstack);
  This->m_t = RxType_CharSet;
  CharSet_addChar(&This->m_charset,c);
}

void Rx_init_FromCharSet(Rx *This, const CharSet *charset, bool onstack) {
  Rx_init(This,onstack);
  This->m_t = RxType_CharSet;
  CharSet_Assign(&This->m_charset,charset);
}

void Rx_init_Binary(Rx *This, BinaryOp op,Rx *lhs, Rx *rhs, bool onstack) {
  Rx_init(This,onstack);
  This->m_t = RxType_BinaryOp;
  This->m_op = op;
  This->m_lhs = lhs;
  This->m_rhs = rhs;
}

void Rx_init_Many(Rx *This, Rx *rx, int count, bool onstack) {
  Rx_init(This,onstack);
  This->m_t = RxType_Many;
  This->m_lhs = rx;
  This->m_low = This->m_high = count;
  if( This->m_low < 0 )
    This->m_low = 0;
  if( This->m_high < 0 )
    This->m_high = -1;
}

void Rx_init_ManyRange(Rx *This, Rx *rx, int low, int high, bool onstack) {
  Rx_init(This,onstack);
  This->m_t = RxType_Many;
  This->m_lhs = rx;
  This->m_low = low;
  This->m_high = high;
  if( This->m_low < 0 )
    This->m_low = 0;
  if( This->m_high < 0 )
    This->m_high = -1;
}

Rx *Rx_clone(const Rx *This) {
  Rx *rx = (Rx*)malloc(sizeof(Rx));
  Rx_init(rx,false);
  rx->m_t = This->m_t;
  rx->m_op = This->m_op;
  CharSet_Assign(&rx->m_charset,&This->m_charset);
  rx->m_lhs = This->m_lhs ? Rx_clone(This->m_lhs) : 0;
  rx->m_rhs = This->m_rhs ? Rx_clone(This->m_rhs) : 0;
  rx->m_low = This->m_low;
  rx->m_high = This->m_high;
  return rx;
}

int Rx_addToNfa(const Rx *This, Nfa *nfa, int startState) {
  if( This->m_t == RxType_CharSet ) {
    int endState = Nfa_addState(nfa);
    Nfa_addTransitionCharSet(nfa,startState,endState,&This->m_charset);
    return endState;
  } else if( This->m_t == RxType_BinaryOp ) {
    if( This->m_op == BinaryOp_Or ) {
      int endlhs = Rx_addToNfa(This->m_lhs,nfa,startState);
      int endrhs = Rx_addToNfa(This->m_rhs,nfa,startState);
      int endState = Nfa_addState(nfa);
      Nfa_addEmptyTransition(nfa,endlhs,endState);
      Nfa_addEmptyTransition(nfa,endrhs,endState);
      return endState;
    } else if( This->m_op == BinaryOp_Concat ) {
      int endlhs = Rx_addToNfa(This->m_lhs, nfa, startState);
      int endrhs = Rx_addToNfa(This->m_rhs, nfa, endlhs);
      return endrhs;
    }
  } else if( This->m_t == RxType_Many ) {
    int prevState = Nfa_addState(nfa);
    Nfa_addEmptyTransition(nfa,startState,prevState);
    int endState = Nfa_addState(nfa);
    int loopState = prevState;
    if( This->m_low == 0 ) {
      loopState = prevState;
      prevState = Rx_addToNfa(This->m_lhs, nfa, prevState);
      Nfa_addEmptyTransition(nfa,loopState,prevState);
    } else {
      for( int i = 0; i < This->m_low; ++i ) {
        loopState = prevState;
        prevState = Rx_addToNfa(This->m_lhs, nfa, prevState);
      }
    }
    if( This->m_high == -1 ) {
      // endless loop back
      Nfa_addEmptyTransition(nfa,prevState,loopState);
    } else {
      for( int i = This->m_low; i < This->m_high; ++i ) {
        Nfa_addEmptyTransition(nfa,prevState,endState);
        prevState = Rx_addToNfa(This->m_lhs, nfa, prevState);
      }
    }
    Nfa_addEmptyTransition(nfa,prevState,endState);
    return endState;
  }
  return -1;
}

Nfa *Rx_toNfa(const Rx *This) {
  Nfa *nfa = (Nfa*)malloc(sizeof(Nfa));
  Nfa_init(nfa,false);
  int endState = Rx_addToNfa(This, nfa, Nfa_addState(nfa));
  Nfa_addEndState(nfa,endState);
  return nfa;
}

static Rx *rxbinary(BinaryOp op, Rx *lhs, Rx *rhs) {
  if( ! lhs && ! rhs )
    return 0;
  if( ! lhs )
    return rhs;
  if( ! rhs )
    return lhs;
  Rx *rx = (Rx*)malloc(sizeof(Rx));
  Rx_init_Binary(rx,op,lhs,rhs,false);
  return rx;
}

static Rx *rxmany(Rx *rx, int low, int high) {
  Rx *ret = (Rx*)malloc(sizeof(Rx));
  Rx_init_ManyRange(ret,rx,low,high,false);
  return ret;
}

static Rx *rxchar(int c) {
  Rx *rx = (Rx*)malloc(sizeof(Rx));
  Rx_init_FromChar(rx,c,false);
  return rx;
}

static Rx *rxcharset(const CharSet *charset) {
  if( ! charset )
    return 0;
  Rx *rx = (Rx*)malloc(sizeof(Rx));
  Rx_init_FromCharSet(rx,charset,false);
  return rx;
}

static Rx *rxany() {
  CharSet charset;
  Scope_Push();
  CharSet_init(&charset,true);
  CharSet_negate(&charset);
  Rx *rx = (Rx*)malloc(sizeof(Rx));
  Rx_init_FromCharSet(rx,&charset,false);
  Scope_Pop();
  return rx;
}

static int ParseNumber(TokStream *s, int start, int end) {
  if( start == end )
    return -1;
  int i = 0;
  while( start < end ) {
    int c = TokStream_peekc(s,start++);
    i *= 10;
    i += c-'0';
  }
  return i;
}

static Rx *ParseRx(TokStream *s, MapAny /*<String,Rx*>*/ *subs);

static bool ParseSub(TokStream *s, String *sub) {
  if( TokStream_peekc(s,0) != '{' )
    return false;
  if( ! isalpha(TokStream_peekc(s,1)) )
    return false;
  int n = 1;
  while( isalnum(TokStream_peekc(s,n+1)) )
    ++n;
  if( TokStream_peekc(s,n+1) != '}' )
    return false;
  TokStream_discard(s,1);
  char *sout = (char*)calloc(n+1,1);
  for( int i = 0; i < n; ++i )
    sout[i] = TokStream_readc(s);
  String_AssignChars(sub,sout);
  TokStream_discard(s,1);
  return true;
}

static bool ParseRepeat(TokStream *s, int *low, int *high) {
  TokStream_peekc(s,0);
  if( TokStream_peekc(s,0) != '{' )
    return false;
  int startn1 = 1;
  int endn1 = 1;
  while( isdigit(TokStream_peekc(s,endn1)) )
    ++endn1;
  if( TokStream_peekc(s,endn1) == ',' ) {
    int startn2 = endn1+1;
    int endn2 = startn2;
    while( isdigit(TokStream_peekc(s,endn2)) )
      ++endn2;
    if( TokStream_peekc(s,endn2) != '}' )
      return false;
    if( endn1 == startn1 && endn2 == startn2 )
      return false;
    *low = ParseNumber(s,startn1,endn1);
    *high = ParseNumber(s,startn2,endn2);
    TokStream_discard(s,endn2+1);
    return true;
  } else if( TokStream_peekc(s,endn1) == '}' ) {
    if( endn1 == startn1 )
      return false;
    *low = *high = ParseNumber(s,startn1,endn1);
    TokStream_discard(s,endn1+1);
    return true;
  }
  return false;
}

static int ParseChar(TokStream *s) {
  int c = TokStream_peekc(s,0);
  if( c == '[' || c == ']' )
    return -1;
  else if( c == '\\' ) {
    TokStream_discard(s,1);
    c = TokStream_peekc(s,0);
    if( c == 'n' ) {
      TokStream_discard(s,1);
      return '\n';
    } else if( c == 't' ) {
      TokStream_discard(s,1);
      return '\t';
    } else if( c == 'r' ) {
      TokStream_discard(s,1);
      return '\r';
    } else if( c == 'n' ) {
      TokStream_discard(s,1);
      return '\n';
    } else if( c == 'v' ) {
      TokStream_discard(s,1);
      return '\v';
    } else if( c == 'x' || c == 'u' ) {
      int ltr = c;
      TokStream_discard(s,1);
      c = TokStream_peekc(s,0);
      if( isxdigit(c) ) {
        ltr = 0;
        while( isxdigit(c) ) {
          ltr *= 16;
          ltr += xvalue(c);
          TokStream_discard(s,1);
          c = TokStream_peekc(s,0);
        }
        return ltr;
      } else {
        return ltr;
      }
    } else {
      TokStream_discard(s,1);
      return c;
    }
  } else {
    TokStream_discard(s,1);
    return c;
  }
  return -1;
}

static Rx *ParseCharSet(TokStream *s) {
  CharSet charset;
  Scope_Push();
  CharSet_init(&charset,true);
  int c = TokStream_peekc(s,0);
  if( c != '[' ) {
    Scope_Pop();
    return 0;
  }
  TokStream_discard(s,1);
  c = TokStream_peekc(s,0);
  bool negate = (c == '^');
  if( negate ) {
    TokStream_discard(s,1);
    c = TokStream_peekc(s,0);
  }
  while( c != -1 ) {
    if( c == ']' ) {
      TokStream_discard(s,1);
      break;
    } else if( c == '[' ) {
      if( TokStream_peekstr(s,"[:alpha:]") ) {
        TokStream_discard(s,9);
        CharSet_addCharRange(&charset, 'A', 'Z');
        CharSet_addCharRange(&charset,'a','z');
      } else if( TokStream_peekstr(s,"[:digit:]") ) {
        TokStream_discard(s,9);
        CharSet_addCharRange(&charset,'0','9');
      } else if( TokStream_peekstr(s,"[:xdigit:]") ) {
        TokStream_discard(s,10);
        CharSet_addCharRange(&charset,'0','9');
        CharSet_addCharRange(&charset, 'A', 'F');
        CharSet_addCharRange(&charset,'a','f');
      } else if( TokStream_peekstr(s,"[:alnum:]") ) {
        TokStream_discard(s,9);
        CharSet_addCharRange(&charset,'0','9');
        CharSet_addCharRange(&charset, 'A', 'Z');
        CharSet_addCharRange(&charset,'a','z');
      } else if( TokStream_peekstr(s,"[:space:]") ) {
        TokStream_discard(s,9);
        CharSet_addChar(&charset,'\t');
        CharSet_addChar(&charset, ' ');
      } else if( TokStream_peekstr(s,"[:white:]") ) {
        TokStream_discard(s,9);
        CharSet_addChar(&charset,'\t');
        CharSet_addChar(&charset, '\n');
        CharSet_addChar(&charset,'\r');
        CharSet_addChar(&charset, ' ');
      } else {
        CharSet_addChar(&charset,c);
        TokStream_discard(s,1);
      }
      c = TokStream_peekc(s,0);
    } else {
      int ch = ParseChar(s);
      if( ch != -1 ) {
        c = TokStream_peekc(s,0);
        if( c == '-' ) {
          TokStream_discard(s,1);
          c = TokStream_peekc(s,0);
          int ch2 = ParseChar(s);
          if( ch2 != -1 ) {
            c = TokStream_peekc(s,0);
            CharSet_addCharRange(&charset,ch,ch2);
          } else
            CharSet_addChar(&charset,ch);
        }
        else
          CharSet_addChar(&charset,ch);
      }
    }
  }
  if( negate )
    CharSet_negate(&charset);
  Rx *rx = rxcharset(&charset);
  Scope_Pop();
  return rx;
}

static Rx *ParseRxSimple(TokStream *s, MapAny /*<String,Rx*>*/ *subs) {
  int c = TokStream_peekc(s,0);

  if( c == -1 || strchr("/+*?",c) )
    return 0;

  if( c == '{' ) {
    Scope_Push();
    String sub;
    String tmp;
    String_init(&sub,true);
    String_init(&tmp,true);
    if( ! ParseSub(s,&sub)  ) {
      Scope_Pop();
      return 0;
    }
    const Rx **rx = &MapAny_findConstT(subs,&sub,Rx*);
    if( ! rx ) {
      String_AssignChars(&tmp,"unknown substitution ");
      String_AddStringInPlace(&tmp,&sub);
      errorString(s,&tmp);
    }
    Scope_Pop();
    return Rx_clone(*rx);
  }
  else if( c == '.' ) {
    TokStream_discard(s,1);
    return rxany();
  } else if( c == '\\' ) {
    TokStream_discard(s,1);
    c = TokStream_peekc(s,0);
    if( c == 'n' ) {
      TokStream_discard(s,1);
      return rxchar('\n');
    } else if( c == 't' ) {
      TokStream_discard(s,1);
      return rxchar('\t');
    } else if( c == 'r' ) {
      TokStream_discard(s,1);
      return rxchar('\r');
    } else if( c == 'n' ) {
      TokStream_discard(s,1);
      return rxchar('\n');
    } else if( c == 'v' ) {
      TokStream_discard(s,1);
      return rxchar('\v');
    } else if( c == 'x' || c == 'u' ) {
      int ltr = c;
      TokStream_discard(s,1);
      c = TokStream_peekc(s,0);
      if( isxdigit(c) ) {
        ltr = 0;
        while( isxdigit(c) ) {
          ltr *= 16;
          ltr += xvalue(c);
          TokStream_discard(s,1);
          c = TokStream_peekc(s,0);
        }
        return rxchar(ltr);
      } else {
        return rxchar(ltr);
      }
    } else {
      TokStream_discard(s,1);
      return rxchar(c);
    }
  } else if( c == '[' ) {
    return ParseCharSet(s);
  }
  else if( c == '(' ) {
   TokStream_discard(s,1);
   Rx *r = ParseRx(s,subs);
   c = TokStream_peekc(s,0);
   if( c != ')' )
    error(s,"unbalanced '('");
   TokStream_discard(s,1);
   return r;
  } else if( c == ')' ) {
   return 0;
  } else  {
    TokStream_discard(s,1);
    return rxchar(c);
  }

  error(s,"parse error");
  return 0;
}

static Rx *ParseRxMany(TokStream *s, MapAny /*<String,Rx*>*/ *subs) {
  Rx* r = 0;
  int low, high;
  r = ParseRxSimple(s,subs);
  if( ! r )
    return 0;
  int c = TokStream_peekc(s,0);

  while( c != -1 ) {
    if( c == '+' ) {
      r = rxmany(r,1,-1);
      TokStream_discard(s,1);
      c = TokStream_peekc(s,0);
    } else if( c == '*' ) {
      r = rxmany(r,0,-1);
      TokStream_discard(s,1);
      c = TokStream_peekc(s,0);
    } else if( c == '?' ) {
      r = rxmany(r,0,1);
      TokStream_discard(s,1);
      c = TokStream_peekc(s,0);
    } else if( ParseRepeat(s,&low,&high) ) {
      r = rxmany(r,low,high);
      c = TokStream_peekc(s,0);
    } else {
      break;
    }
  }
  return r;
}

static Rx *ParseRxConcat(TokStream *s, MapAny /*<String,Rx*>*/ *subs) {
  Rx *lhs = ParseRxMany(s,subs);
  if( ! lhs )
    return 0;
  int c = TokStream_peekc(s,0);
  while( c != -1 && c != '/' && c != '|' ) {
    Rx *rhs = ParseRxMany(s,subs);
    if( ! rhs )
      break;
    lhs = rxbinary(BinaryOp_Concat, lhs, rhs);
    c = TokStream_peekc(s,0);
  }
  return lhs;
}

static Rx *ParseRxOr(TokStream *s, MapAny /*<String,Rx*>*/ *subs) {
  Rx *lhs = ParseRxConcat(s,subs);
  if( ! lhs )
    return 0;
  int c = TokStream_peekc(s,0);
  while( c == '|' ) {
    TokStream_discard(s,1);
    Rx *rhs = ParseRxConcat(s,subs);
    lhs = rxbinary(BinaryOp_Or,lhs,rhs);
    c = TokStream_peekc(s,0);
  }
  return lhs;
}

static Rx *ParseRx(TokStream *s, MapAny /*<String,Rx*>*/ *subs) {
 return ParseRxOr(s, subs);
}

static void ParseSymbol(TokStream *s, String *symbol) {
  int n = 0;
  int c = TokStream_peekc(s,n);
  if( ! isalpha(c) && c != '_' )
    return;
  c = TokStream_peekc(s,++n);
  while( isalnum(c) || c == '_' )
    c = TokStream_peekc(s,++n);
  char *str = (char*)malloc(n+1);
  for( int i = 0; i < n; ++i )
    str[i] = TokStream_peekc(s,i);
  str[n] = 0;
  String_AssignChars(symbol,str);
  free(str);
  TokStream_discard(s,n);
  return;
}

static void ParseWs(TokStream *s) {
  int c = TokStream_peekc(s,0);
  while(c == '#' || isspace(c) ) {
    if (c == '#') {
      while (c != -1 && c != '\n') {
        TokStream_discard(s, 1);
        c = TokStream_peekc(s, 0);
      }
    }
    else {
      TokStream_discard(s, 1);
      c = TokStream_peekc(s, 0);
    }
  }
}

static Rx *ParseRegex(TokStream *s, MapAny /*<String,Rx*>*/ *subs) {
  int c = TokStream_peekc(s,0);
  if( c != '/' )
    error(s,"No regex");
  TokStream_discard(s,1);
  Rx *rx = ParseRx(s,subs);
  if( ! rx )
    error(s,"empty regex");
  c = TokStream_peekc(s,0);
  if( c != '/' ) {
    Rx_destroy(rx);
    free(rx);
    if( c == ')' )
      error(s,"unbalanced ')'");
    else
      error(s,"No trailing '/' on regex");
    return 0;
  }
  TokStream_discard(s,1);
  return rx;
}

static void AddDefaultSection(SetAny /*<String>*/ *sections, MapAny /*<String,int>*/ *sectionNumbers) {
  String strdefault;
  int zero = 0;
  Scope_Push();
  String_init(&strdefault,true);
  String_AssignChars(&strdefault,"default");
  SetAny_insert(sections,&strdefault,0);
  MapAny_insert(sectionNumbers,&strdefault,&zero);
  Scope_Pop();
}

Nfa *ParseTokenizerFile(TokStream *s) {
  SetAny /*<String>*/ sections;
  MapAny /*<String,int>*/ sectionNumbers;
  MapAny /*<String,int>*/ tokens;
  Nfa nfa;
  MapAny /*<String,Rx*>*/ subs;
  SetAny /*<int>*/ wstoks;
  String label;
  String symbol;
  Token token;

  Scope_Push();
  SetAny_init(&sections,getStringElement(),true);
  MapAny_init(&sectionNumbers,getStringElement(),getIntElement(),true);
  MapAny_init(&tokens,getStringElement(),getIntElement(),true);
  Nfa_init(&nfa,true);
  MapAny_init(&subs,getStringElement(),getPointerElement(),true);
  SetAny_init(&wstoks,getIntElement(),true);
  String_init(&label,true);
  String_init(&symbol,true);
  Token_init(&token,true);

  AddDefaultSection(&sections,&sectionNumbers);
  int curSection = 0;
  int nextSectionNumber = 1;
  int startState = Nfa_addState(&nfa);
  Nfa_addStartState(&nfa,startState);
  int startTokState = Nfa_addState(&nfa);
  Nfa_addTransition(&nfa,startState,startTokState,curSection);
  ParseWs(s);
  int c = TokStream_peekc(s,0);
  int nexttokid = 0;
  while( c != -1 ) {
    bool isws = false;
    bool issub = false;
    TokenAction action = ActionNone;
    int actionarg = -1;
    if( c == '~' ) {
      isws = true;
      TokStream_discard(s,1);
    } else if( c == '=' ) {
      issub = true;
      TokStream_discard(s,1);
    } else if( c == '-' ) {
      TokStream_discard(s,1);
      ParseWs(s);
      if( ! TokStream_peekstr(s,"section") )
        error(s,"No 'section'");
      TokStream_discard(s,7);
      ParseWs(s);
      String_AssignChars(&label,"");
      ParseSymbol(s,&label);
      if( String_length(&label) == 0 )
        error(s,"No section label");
      if( ! MapAny_find(&sectionNumbers,&label) ) {
        MapAny_insert(&sectionNumbers,&label,&nextSectionNumber);
        nextSectionNumber++;
      }
      if( ! SetAny_contains(&sections,&label) ) {
        SetAny_insert(&sections,&label,0);
      } else
        error(s,"the section name is already used");
      if( Nfa_stateCount(&nfa) == 0 )
        error(s,"sections may not be empty");
      curSection = MapAny_findConstT(&sectionNumbers,&label,int);
      startTokState = Nfa_addState(&nfa);
      Nfa_addTransition(&nfa,startState,startTokState,curSection);
      ParseWs(s);
      c = TokStream_peekc(s,0);
      continue;
    } else if( TokStream_peekstr(s,"push") ) {
      action = ActionPush;
      TokStream_discard(s,4);
      ParseWs(s);
      String_AssignChars(&label,"");
      ParseSymbol(s,&label);
      if( String_length(&label) == 0 )
        error(s,"no 'push' label");
      if( ! MapAny_contains(&sectionNumbers,&label) ) {
        MapAny_insert(&sectionNumbers,&label,&nextSectionNumber);
        nextSectionNumber++;
      }
      actionarg = MapAny_findConstT(&sectionNumbers,&label,int);
    } else if( TokStream_peekstr(s,"pop") ) {
      action = ActionPop;
      TokStream_discard(s,3);
    } else if( TokStream_peekstr(s,"goto") ) {
      action = ActionGoto;
      TokStream_discard(s,4);
      ParseWs(s);
      String_AssignChars(&label,"");
      ParseSymbol(s,&label);
      if( String_length(&label) == 0 )
        error(s,"no 'goto' label");
      if( ! MapAny_contains(&sectionNumbers,&label) ) {
        MapAny_insert(&sectionNumbers,&label,&nextSectionNumber);
        nextSectionNumber++;
      }
      actionarg = MapAny_findConstT(&sectionNumbers,&label,int);
    }
    ParseWs(s);
    c = TokStream_peekc(s,0);
    String_AssignChars(&symbol,"");
    ParseSymbol(s,&symbol);
    if( String_length(&symbol) == 0 )
      error(s,"No symbol");
    ParseWs(s);
    c = TokStream_peekc(s,0);
    Rx *rx = ParseRegex(s,&subs);
    if( ! rx )
      error(s,"No regex");
    ParseWs(s);
    c = TokStream_peekc(s,0);
    if( issub ) {
      Rx **rxtmp = &MapAny_findT(&subs,&symbol,Rx*);
      if( rxtmp ) {
        Rx_destroy(*rxtmp);
        free(*rxtmp);
        MapAny_erase(&subs,&symbol);
      }
      //fprintf(stderr, "Adding substitution %s %d\n", symbol.c_str(), s.line());
      //fflush(stderr);
      MapAny_insert(&subs,&symbol,&rx);
    } else {
      int tokid = -1;
      if( ! MapAny_find(&tokens,&symbol) ) {
        tokid = nexttokid++;
        MapAny_insert(&tokens,&symbol,&tokid);
        token.m_token = tokid;
        token.m_isws = isws;
        String_AssignString(&token.m_name,&symbol);
        Nfa_setTokenDef(&nfa,&token);
      } else {
        tokid = MapAny_findConstT(&tokens,&symbol,int);
      }
      Nfa_setTokenAction(&nfa,tokid,curSection,action,actionarg);
      int endState = Rx_addToNfa(rx, &nfa, startTokState);
      Nfa_addEndState(&nfa,endState);
      Nfa_setStateToken(&nfa,endState,tokid);
    }
  }
  Nfa_setSections(&nfa,SetAny_size(&sections));
  Nfa *dfa = (Nfa*)malloc(sizeof(Nfa));
  Nfa_init(dfa,false);
  Nfa_toDfa(&nfa,dfa);

  Scope_Pop();
  return dfa;
}

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
  const char *prefix;
};

void CLanguageOutputter_outDecl(const LanguageOutputter *This, FILE *out, const char *type, const char *name) { fputs(type, out); fputc(' ', out); fputs(This->prefix, out); fputs("_", out); fputs(name, out); }
void CLanguageOutputter_outArrayDecl(const LanguageOutputter *This, FILE *out, const char *type, const char *name) { fputs(type,out); fputc(' ',out);  fputs(This->prefix, out); fputs("_", out); fputs(name,out); fputs("[]",out); }
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
LanguageOutputter CLanguageOutputter = {CLanguageOutputter_outDecl, CLanguageOutputter_outArrayDecl, CLanguageOutputter_outStartArray, CLanguageOutputter_outEndArray, CLanguageOutputter_outEndStmt, CLanguageOutputter_outNull, CLanguageOutputter_outBool, CLanguageOutputter_outStr, CLanguageOutputter_outChar, CLanguageOutputter_outInt};

void PyLanguageOutputter_outDecl(const LanguageOutputter* This, FILE* out, const char* type, const char* name) { fputs(type, out); fputc(' ', out); fputs(This->prefix, out); fputs("_", out); fputs(name, out); }
void PyLanguageOutputter_outArrayDecl(const LanguageOutputter* This, FILE* out, const char* type, const char* name) { fputs(type, out); fputc(' ', out);  fputs(This->prefix, out); fputs("_", out); fputs(name, out); fputs("[]", out); }
void PyLanguageOutputter_outStartArray(const LanguageOutputter* This, FILE* out) { fputc('{', out); }
void PyLanguageOutputter_outEndArray(const LanguageOutputter* This, FILE* out) { fputc('}', out); }
void PyLanguageOutputter_outEndStmt(const LanguageOutputter* This, FILE* out) { fputc(';', out); }
void PyLanguageOutputter_outNull(const LanguageOutputter* This, FILE* out) { fputc('0', out); }
void PyLanguageOutputter_outBool(const LanguageOutputter* This, FILE* out, bool b) { fputs((b ? "true" : "false"), out); }
void PyLanguageOutputter_outStr(const LanguageOutputter* This, FILE* out, const char* str) { fputc('"', out); fputs(str, out); fputc('"', out); }
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
LanguageOutputter PyLanguageOutputter = { PyLanguageOutputter_outDecl, PyLanguageOutputter_outArrayDecl, PyLanguageOutputter_outStartArray, PyLanguageOutputter_outEndArray, PyLanguageOutputter_outEndStmt, PyLanguageOutputter_outNull, PyLanguageOutputter_outBool, PyLanguageOutputter_outStr, PyLanguageOutputter_outChar, PyLanguageOutputter_outInt };

static void OutputDfaSource(FILE *out, const Nfa *dfa, const LanguageOutputter *lang) {
  bool first = true;
  // Going to assume there are no gaps in toking numbering
  VectorAny /*<Token>*/ tokens;
  MapAny /*<int,int>*/ transitioncounts;
  Scope_Push();
  VectorAny_init(&tokens,&TokenElement,true);
  MapAny_init(&transitioncounts,getIntElement(),getIntElement(),true);
  Nfa_getTokenDefs(dfa,&tokens);

  for( int cur = 0, end = VectorAny_size(&tokens); cur != end; ++cur ) {
    const Token *curToken = &VectorAny_ArrayOpConstT(&tokens,cur,Token);
    lang->outDecl(lang,out,"static const int",String_Chars(&curToken->m_name));
    fputs(" = ",out);
    lang->outInt(lang,out,curToken->m_token);
    lang->outEndStmt(lang,out);
    fputc('\n',out);
  }

  lang->outDecl(lang,out,"static const int","tokenCount");
  fputs(" = ",out);
  lang->outInt(lang,out,VectorAny_size(&tokens));
  lang->outEndStmt(lang,out);
  fputc('\n',out);

  lang->outDecl(lang,out,"static const int","sectionCount");
  fputs(" = ",out);
  lang->outInt(lang,out,Nfa_getSections(dfa));
  lang->outEndStmt(lang,out);
  fputc('\n',out);

  lang->outArrayDecl(lang,out, "static const int", "tokenaction");
  fputs(" = ",out);
  lang->outStartArray(lang,out);
  first = true;
  for( int cur = 0, end = VectorAny_size(&tokens); cur != end; ++cur ) {
    const Token *token = &VectorAny_ArrayOpConstT(&tokens,cur,Token);
    for( int i = 0, n = Nfa_getSections(dfa); i < n; ++i ) {
      if( first )
        first = false;
      else
        fputc(',',out);
      const Action *action = &MapAny_findConstT(&token->m_actions,&i,Action);
      if( ! action ) {
        fputs("0,-1",out);
      } else {
        lang->outInt(lang,out,action->m_action);
        fputc(',',out);
        lang->outInt(lang,out,action->m_actionarg);
      }
    }
  }
  lang->outEndArray(lang,out);
  lang->outEndStmt(lang,out);
  fputc('\n',out);

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

  lang->outArrayDecl(lang,out, "static const bool","isws");
  fputs(" = ",out);
  lang->outStartArray(lang,out);
  first = true;
  for( int cur = 0, end = VectorAny_size(&tokens); cur != end; ++cur ) {
    if( first )
      first = false;
    else
      fputc(',',out);
    const Token *token = &VectorAny_ArrayOpConstT(&tokens,cur,Token);
    lang->outBool(lang,out,token->m_isws);
  }
  lang->outEndArray(lang,out);
  lang->outEndStmt(lang,out);
  fputc('\n',out);

  lang->outDecl(lang,out,"static const int","stateCount");
  fputs(" = ",out);
  lang->outInt(lang,out,Nfa_stateCount(dfa));
  lang->outEndStmt(lang,out);
  fputc('\n',out);

  lang->outArrayDecl(lang,out,"static const int","transitions");
  fputs(" = ",out);
  lang->outStartArray(lang,out);
  first = true;
  MapAny_clear(&transitioncounts);
  for( int cur = 0, end = MapAny_size(&dfa->m_transitions); cur < end; ++cur ) {
    const Transition *transition = 0;
    const CharSet *ranges = 0;
    MapAny_getByIndexConst(&dfa->m_transitions,cur,(const void**)&transition,(const void**)&ranges);
    if( ! MapAny_find(&transitioncounts,&transition->m_from) ) {
      int zero = 0;
      MapAny_insert(&transitioncounts,&transition->m_from,&zero);
    }
    MapAny_findT(&transitioncounts,&transition->m_from,int) += 2+2*CharSet_size(ranges);
    if( first )
      first = false;
    else
      fputc(',',out);
    lang->outInt(lang,out,transition->m_to);
    fputc(',',out);
    lang->outInt(lang,out,CharSet_size(ranges));
    for( int cur = 0, end = CharSet_size(ranges); cur != end; ++cur ) {
      const CharRange *curRange = CharSet_getRange(ranges,cur);
      fputc(',',out);
      lang->outChar(lang,out,curRange->m_low);
      fputc(',',out);
      lang->outChar(lang,out,curRange->m_high);
    }
  }
  lang->outEndArray(lang,out);
  lang->outEndStmt(lang,out);
  fputc('\n',out);

  int offset = 0;
  lang->outArrayDecl(lang,out,"static const int","transitionOffset");
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

  first = true;
  lang->outArrayDecl(lang,out,"static const int","tokens");
  fputs(" = ",out);
  lang->outStartArray(lang,out);
  for( int i = 0, n = Nfa_stateCount(dfa); i < n; ++i ) {
    if( first )
      first = false;
    else
      fputc(',',out);
    int tok = Nfa_getStateToken(dfa,i);
    lang->outInt(lang,out,tok);
  }
  lang->outEndArray(lang,out);
  lang->outEndStmt(lang,out);
  fputc('\n',out);
  Scope_Pop();
}

void OutputTokenizerSource(FILE *out, const Nfa *dfa, const char *prefix, bool py) {
  LanguageOutputter *outputer = py ? &PyLanguageOutputter : &CLanguageOutputter;
  outputer->prefix = prefix;
  OutputDfaSource(out,dfa,outputer);
  fputc('\n',out);
}
