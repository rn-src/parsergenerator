#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "tokenizer.h"

#define CHARSET_CHECKING_ON 1
#ifdef CHARSET_CHECKING_ON
#define CHARSET_CHECK(cs) CharSet_check(cs);
static void CharSet_check(const CharSet *cs) {
  const CharRange *r = (const CharRange*)cs->m_ranges.m_p;
  for( int i = 0, n = cs->m_ranges.m_size; i < n; ++i ) {
    if( r[i].m_low > r[i].m_high )
      fputs("uh-oh bad range\n", stdout);
    if( i < n-1 &&  r[i].m_high+1 >= r[i+1].m_low )
      fputs("uh-oh bad range\n", stdout);
  }
}
#else
#define CHARSET_CHECK(cs)
#endif

ElementOps ActionElement = {sizeof(Action), false, false, 0, 0, 0, 0, 0, 0};
ElementOps TransitionElement = {sizeof(Transition), false, false, (elementInit)Transition_init, 0, (elementLessThan)Transition_LessThan, (elementEqual)Transition_Equal, 0, 0};
ElementOps CharSetElement = {sizeof(CharSet), false, false, (elementInit)CharSet_init, (elementDestroy)CharSet_destroy, (elementLessThan)CharSet_LessThan, (elementEqual)CharSet_Equal, (elementAssign)CharSet_Assign, 0};
ElementOps CharRangeElement = {sizeof(CharRange), false, false, 0, 0, (elementLessThan)CharRange_LessThan, 0, 0, 0};
ElementOps TokenElement = {sizeof(Token), false, false, (elementInit)Token_init, (elementDestroy)Token_destroy, (elementLessThan)Token_LessThan, (elementEqual)Token_Equal, (elementAssign)Token_Assign, 0};

ElementOps *getTokenElement() {
  return &TokenElement;
}

ElementOps *getCharSetElement() {
  return &CharSetElement;
}

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
  if( Token_LessThan((const Token*)lhs,(const Token*)rhs) )
    return -1;
  if( Token_LessThan((const Token*)rhs,(const Token*)lhs) )
    return 1;
  return 0;
}

bool TokStream_fill(TokStream *This) {
  if( feof(This->m_in) )
    return false;
  if( This->m_buffill == This->m_buflen ) {
    if( This->m_buflen == 0 ) {
      size_t newlen = 256;
      char *newbuf = (char*)malloc(newlen+1);
      memset(newbuf,0,newlen+1);
      This->m_buf = newbuf;
      This->m_next = This->m_buf;
      This->m_buflen = newlen;
    } else {
      size_t newlen = This->m_buflen*2;
      char *newbuf = (char*)malloc(newlen+1);
      memset(newbuf,0,newlen+1);
      for( size_t i = 0; i < This->m_buffill; ++i ) {
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
    size_t fillpos = (This->m_bufpos+This->m_buffill)%This->m_buflen;
    size_t len = (This->m_bufpos > fillpos) ? (This->m_bufpos-fillpos) : This->m_buflen-fillpos;
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

void TokStream_init(TokStream *This, FILE *in, const char *file, bool onstack) {
  This->m_buf = 0;
  This->m_next = 0;
  This->m_buffill = 0;
  This->m_buflen = 0;
  This->m_bufpos = 0;
  This->m_in = in;
  This->m_pos = 0;
  This->m_line = This->m_col = 1;
  This->m_file = (char*)malloc(strlen(file)+1);
  strcpy(This->m_file,file);
  if( onstack )
    Push_Destroy(This,(vpstack_destroyer)TokStream_destroy);
}
void TokStream_destroy(TokStream *This) {
  if( This->m_buf )
    free(This->m_buf);
  free(This->m_file);
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

const char *TokStream_file(TokStream *This) {
  return This->m_file;
}

static void error(TokStream *s, const char *err) {
  String errstr;
  Scope_Push();
  String_init(&errstr,true);
  String_AssignChars(&errstr,err);
  setParseError(TokStream_line(s),TokStream_col(s),TokStream_file(s),&errstr);
  Scope_Pop();
}

static void errorString(TokStream *s, const String *err) {
  setParseError(TokStream_line(s),TokStream_col(s),TokStream_file(s),err);
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

CharRange *CharRange_SetRange(CharRange *This, uint32_t low, uint32_t high) {
  This->m_low = low;
  This->m_high = high;
  return This;
}

bool CharRange_ContainsChar(const CharRange *This, uint32_t i) {
  if( i >= This->m_low && i <= This->m_high )
    return true;
  return false;
}

bool CharRange_ContainsCharRange(const CharRange *This, const CharRange *rhs) {
  if( rhs->m_low >= This->m_low && rhs->m_high <= This->m_high )
    return true;
  return false;
}

bool CharRange_OverlapsRange(const CharRange *This, uint32_t low, uint32_t high) {
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
  CHARSET_CHECK(rhs)
  VectorAny_Assign(&lhs->m_ranges,&rhs->m_ranges);
  CHARSET_CHECK(lhs)
}

int CharSet_find(const CharSet *This, size_t c, bool *found) {
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

bool CharSet_has(const CharSet *This, uint32_t c) {
  bool found = false;
  CharSet_find(This,c,&found);
  return found;
}

void CharSet_clear(CharSet *This) {
  VectorAny_clear(&This->m_ranges);
}

bool CharSet_empty(const CharSet *This) {
  return VectorAny_empty(&This->m_ranges);
}

void CharSet_addChar(CharSet *This, uint32_t i) {
  CharSet_addCharRange(This,i,i);
  CHARSET_CHECK(This)
}

void CharSet_addCharRange(CharSet *This, uint32_t low, uint32_t high) {
  bool found;
  CharRange range;
  CharRange *ranges = &VectorAny_ArrayOpT(&This->m_ranges,0,CharRange);
  int i = CharSet_find(This,low,&found);
  if( ! found && i > 0 && ranges[i-1].m_high+1 == low ) {
    found = true;
    --i;
  }
  if( found ) {
    if( low < ranges[i].m_low )
      ranges[i].m_low = low;
    if( high > ranges[i].m_high )
      ranges[i].m_high = high;
  } else {
    VectorAny_insert(&This->m_ranges,i,CharRange_SetRange(&range,low,high));
    ranges = &VectorAny_ArrayOpT(&This->m_ranges,0,CharRange);
  }
  int ntoerase = 0;
  while( i+1+ntoerase < This->m_ranges.m_size && ranges[i].m_high+1 >= ranges[i+1+ntoerase].m_low ) {
    if( ranges[i+ntoerase+1].m_high > ranges[i].m_high )
      ranges[i].m_high = ranges[i+ntoerase+1].m_high; 
    ++ntoerase;
  }
  if( ntoerase > 0 )
    VectorAny_eraseRange(&This->m_ranges,i+1,i+1+ntoerase);
  CHARSET_CHECK(This)
}

void CharSet_addCharSet(CharSet *This, const CharSet *rhs) {
  CHARSET_CHECK(rhs)
  for( int cur = 0, end = VectorAny_size(&rhs->m_ranges); cur != end; ++cur ) {
    const CharRange *curRange = &VectorAny_ArrayOpConstT(&rhs->m_ranges,cur,CharRange);
    CharSet_addCharRange(This, curRange->m_low, curRange->m_high);
  }
  CHARSET_CHECK(This)
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
  if( prev < UNICODE_MAX )
    VectorAny_push_back(&ranges,CharRange_SetRange(&range,prev,UNICODE_MAX));
  VectorAny_Assign(&This->m_ranges,&ranges);
  Scope_Pop();
  CHARSET_CHECK(This)
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
  Action tokaction = {ActionNone};
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

void Nfa_closure(const MapAny /*<int,Set<int>>*/ *emptytransitions, SetAny /*<int>*/ *states) {
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

void Nfa_stateTransitions(const Nfa *This, const SetAny /*<int>*/ *states, VectorAny /*<int>*/ *transitions_to, VectorAny /*<CharSet>*/ *transition_symbols) {
  VectorAny_clear(transitions_to);
  VectorAny_clear(transition_symbols);
  const Transition *key = 0;
  const CharSet *value = 0;
  for( int cur = 0, end = MapAny_size(&This->m_transitions); cur != end; ++cur ) {
    MapAny_getByIndexConst(&This->m_transitions,cur,(const void**)&key,(const void**)&value);
    const int *pstate = &SetAny_findConstT(states,&key->m_from,int);
    if( ! pstate )
      continue;
    VectorAny_push_back(transitions_to,&key->m_to);
    VectorAny_push_back(transition_symbols, value);
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

void Nfa_print(const Nfa *nfa) {
  fputs("--- start nfa ---\n", stdout);
  for( int cur = 0, end = SetAny_size(&nfa->m_startStates); cur != end; ++cur ) {
    int s = SetAny_getByIndexConstT(&nfa->m_startStates,cur,int);
    fprintf(stdout, "%d is a start state\n", s);
  }
  for( int cur = 0, end = MapAny_size(&nfa->m_transitions); cur < end; ++cur ) {
    const Transition *transition = 0;
    const CharSet *ranges = 0;
    MapAny_getByIndexConst(&nfa->m_transitions,cur,(const void**)&transition,(const void**)&ranges);
    fprintf(stdout, "%d -> %d on ", transition->m_from, transition->m_to);
    for( int cur = 0, end = CharSet_size(ranges); cur != end; ++cur ) {
      const CharRange *curRange = CharSet_getRange(ranges,cur);
      if( curRange->m_low == curRange->m_high )
        fprintf(stdout,",%u", curRange->m_low);
      else
        fprintf(stdout,",%u-%u", curRange->m_low,curRange->m_high);
    }
    fputc('\n',stdout);
  }
  for( int cur = 0, end = SetAny_size(&nfa->m_emptytransitions); cur != end; ++cur ) {
    const Transition *transition = &SetAny_getByIndexConstT(&nfa->m_emptytransitions,cur,Transition);
    fprintf(stdout, "%d -> %d on <EPSILON>\n", transition->m_from, transition->m_to);
  }
  for( int cur = 0, end = SetAny_size(&nfa->m_endStates); cur != end; ++cur ) {
    int s = SetAny_getByIndexConstT(&nfa->m_endStates,cur,int);
    fprintf(stdout, "%d is an end state\n", s);
  }
  fputs("--- end nfa ---\n", stderr);
}

struct comboctx {
  const Nfa *nfa;
  Nfa *dfa;
  SetAny /*<int>*/ *nfastate;
  int dfastate;
  VectorAny /*< <Set<int>,int> >*/ *newstates;
  MapAny /*< <Set<int>,int> >*/ *state2num;
  SetAny /*<int>*/ *nextstate;
  MapAny /*<int,Set<int> >*/ *emptytransitions;
  VectorAny /*<int>*/ *transitions_to;
};
typedef struct comboctx comboctx;

static int addstate(comboctx *ctx) {
  int nextstateno = Nfa_addState(ctx->dfa);
  VectorAny_push_back(ctx->newstates, ctx->nextstate);
  MapAny_insert(ctx->state2num,ctx->nextstate,&nextstateno);
  if( Nfa_hasEndState(ctx->nfa,ctx->nextstate) )
    Nfa_addEndState(ctx->dfa,nextstateno);
  int token = Nfa_lowToken(ctx->nfa,ctx->nextstate);
  if( token != -1 ) {
    if( ! Nfa_hasTokenDef(ctx->dfa,token) ) {
      const Token *tokendef = Nfa_getTokenDef(ctx->nfa,token);
      if( tokendef )
        Nfa_setTokenDef(ctx->dfa,tokendef);
    }
    Nfa_setStateToken(ctx->dfa,nextstateno,token);
  }
  return nextstateno;
}

static void cb_add_state(const CharSet* cs, VectorAny /*<int>*/* charset_indexes, comboctx *ctx) {
  SetAny_clear(ctx->nextstate);
  for( int i = 0, iend = VectorAny_size(charset_indexes); i < iend; ++i ) {
    int idx = VectorAny_ArrayOpConstT(charset_indexes,i,int);
    int to = VectorAny_ArrayOpConstT(ctx->transitions_to,idx,int);
    SetAny_insert(ctx->nextstate,&to,0);
  }
  Nfa_closure(ctx->emptytransitions, ctx->nextstate);
  const int *pFoundState = &MapAny_findConstT(ctx->state2num, ctx->nextstate, int);
  int nextstateno = -1;
  if( ! pFoundState ) {
    nextstateno = addstate(ctx);
  } else {
    nextstateno = *pFoundState;
  }
  Nfa_addTransitionCharSet(ctx->dfa,ctx->dfastate,nextstateno,cs);
}

void Nfa_toDfa_NonMinimal(const Nfa *This, Nfa *dfa) {
  VectorAny /*< <Set<int>,int> >*/ newstates;
  MapAny /*< <Set<int>,int> >*/ state2num;
  SetAny /*<int>*/ nextstate;
  MapAny /*<int,Set<int> >*/ emptytransitions;
  VectorAny  /*<int>*/ transitions_to;
  VectorAny  /*<CharSet>*/ transition_symbols;
  SetAny /*<int>*/ state;
  SetAny setEmpty;
  VectorAny /*<int>*/transition_combo;

  Scope_Push();
  VectorAny_init(&newstates, getSetAnyElement(), true);
  MapAny_init(&state2num, getSetAnyElement(), getIntElement(), true);
  SetAny_init(&nextstate, getIntElement(), true);
  MapAny_init(&emptytransitions, getIntElement(), getSetAnyElement(), true);
  VectorAny_init(&transitions_to, getIntElement(),true);
  VectorAny_init(&transition_symbols, &CharSetElement,true);
  SetAny_init(&state,getIntElement(), true);
  SetAny_init(&setEmpty,getIntElement(),true);
  VectorAny_init(&transition_combo,getIntElement(),true);
  comboctx ctx = {0};
  ctx.nfa = This;
  ctx.dfa = dfa;
  ctx.nfastate = &state;
  ctx.dfastate = 0;
  ctx.newstates = &newstates;
  ctx.state2num = &state2num;
  ctx.nextstate = &nextstate;
  ctx.emptytransitions = &emptytransitions;
  ctx.transitions_to = &transitions_to;

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
  SetAny_Assign(ctx.nextstate,&This->m_startStates);
  Nfa_closure(&emptytransitions, ctx.nextstate);
  int initStateNo = addstate(&ctx);
  Nfa_addStartState(dfa, initStateNo);
  for( int stateno = 0; stateno < VectorAny_size(&newstates); ++stateno ) {
    const SetAny *pstate = &VectorAny_ArrayOpConstT(&newstates, stateno, SetAny);
    SetAny_Assign(ctx.nfastate,pstate);
    Nfa_stateTransitions(This, ctx.nfastate, ctx.transitions_to, &transition_symbols);
    ctx.dfastate = stateno;
    CharSet_combo_breaker(&transition_symbols, (ComboBreakerCB)cb_add_state, &ctx);
  }
  Scope_Pop();
}

static int state_num(int state, Nfa *This, bool reverse_numbers) {
  if( reverse_numbers )
    return This->m_nextState-1-state;
  return state;
}

void Nfa_Reverse(Nfa *This, bool reverse_numbers) {
  SetAny start_states, end_states;
  MapAny transitions;
  SetAny emptytransitions;
  MapAny state2token;
  Scope_Push();
  SetAny_init(&start_states,This->m_startStates.m_values.m_ops,true);
  SetAny_init(&end_states,This->m_endStates.m_values.m_ops,true);
  MapAny_init(&transitions,This->m_transitions.m_keys.m_values.m_ops,This->m_transitions.m_values.m_ops,true);
  SetAny_init(&emptytransitions,This->m_emptytransitions.m_values.m_ops,true);
  MapAny_init(&state2token,This->m_state2token.m_keys.m_values.m_ops,This->m_state2token.m_values.m_ops,true);
  for( int i = 0, n = SetAny_size(&This->m_startStates); i < n; ++i ) {
    int state = SetAny_getByIndexConstT(&This->m_startStates,i,int);
    state = state_num(state,This,reverse_numbers);
    SetAny_insert(&end_states, &state,0);
  }
  for( int i = 0, n = SetAny_size(&This->m_endStates); i < n; ++i ) {
    int state = SetAny_getByIndexConstT(&This->m_endStates,i,int);
    state = state_num(state,This,reverse_numbers);
    SetAny_insert(&start_states, &state,0);
  }
  SetAny_Assign(&This->m_startStates,&start_states);
  SetAny_Assign(&This->m_endStates,&end_states);
  for( int i = 0, n = MapAny_size(&This->m_transitions); i < n; ++i ) {
    Transition *t = 0;
    CharSet *cs = 0;
    MapAny_getByIndex(&This->m_transitions,i,(const void**)&t,(void**)&cs);
    Transition t_rev = {state_num(t->m_to,This,reverse_numbers),state_num(t->m_from,This,reverse_numbers)};
    MapAny_insert(&transitions,&t_rev,cs);
  }
  MapAny_Assign(&This->m_transitions,&transitions);
  for( int i = 0, n = SetAny_size(&This->m_emptytransitions); i < n; ++i ) {
    Transition *t = (Transition*)SetAny_getByIndexConst(&This->m_emptytransitions,i);
    Transition t_rev = {state_num(t->m_to,This,reverse_numbers),state_num(t->m_from,This,reverse_numbers)};
    SetAny_insert(&emptytransitions,&t_rev,0);
  }
  SetAny_Assign(&This->m_emptytransitions,&emptytransitions);
  for( int i = 0, n = MapAny_size(&This->m_state2token); i < n; ++i ) {
    const int *pstate = 0, *ptoken = 0;
    MapAny_getByIndex(&This->m_state2token,i,(const void**)&pstate,(void**)&ptoken);
    int state = state_num(*pstate,This,reverse_numbers);
    int token = *ptoken;
    MapAny_insert(&state2token,&state,&token);
  }
  MapAny_Assign(&This->m_state2token,&state2token);
  Scope_Pop();
}

void Nfa_toDfa(const Nfa *This, Nfa *dfa) {
  Nfa tmp;
  Scope_Push();
  Nfa_init(&tmp,true);
  Nfa_toDfa_NonMinimal(This,&tmp);
  // reverse it, do it again
  Nfa_Reverse(&tmp,false);
  Nfa_toDfa_NonMinimal(&tmp,dfa);
  // reverse it, done
  Nfa_Reverse(dfa,true);
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
  size_t n = 1;
  while( isalnum(TokStream_peekc(s,n+1)) )
    ++n;
  if( TokStream_peekc(s,n+1) != '}' )
    return false;
  TokStream_discard(s,1);
  char *sout = (char*)malloc(n+1);
  for( int i = 0; i < n; ++i )
    sout[i] = TokStream_readc(s);
  sout[n] = 0;
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

static int readHex(TokStream *s, int *pi) {
  int c = TokStream_peekc(s,0);
  int i = 0;
  int n = 0;
  while( isxdigit(c) ) {
    ++n;
    i *= 16;
    i += xvalue(c);
    c = TokStream_peekc(s,n);
  }
  if( i < 0 ) {
    char hex[10] = {0};
    for( int n0 = 0; n0 < n; ++n0 )
      hex[n0] = (char)TokStream_peekc(s,n0);
    hex[n] = 0;
    fprintf(stderr, "uh oh, negative hex %s -> %d\n", hex, i);
  }
  *pi = i;
  return n;
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
      TokStream_discard(s,1);
      int ltr = 0;
      if( readHex(s,&ltr) ) {
        return ltr;
      } else {
        return c;
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

  // you can have a newline mid-rx
  // ignore until the first non-whitespace on the next line
  // additionally, ignore comments introduced within these breaks
  while( c == '\r' || c == '\n' ) {
    while( c == ' ' || c == '\t' || c == '\r' || c == '\n' ) {
      TokStream_discard(s,1);
      c = TokStream_peekc(s,0);
    }
    // toss out the line comment
    if( c == '#' ) {
      TokStream_discard(s,1);
      c = TokStream_peekc(s,0);
      while( c != -1 && c != '\r' && c != '\n' ) {
        TokStream_discard(s,1);
        c = TokStream_peekc(s,0);
      }
    }
  }

  if( c == -1 || strchr("/+*?|",c) )
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
      int ltr = 0;
      TokStream_discard(s,1);
      if( readHex(s,&ltr) ) {
        return rxchar(ltr);
      } else {
        return rxchar(c);
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
  } else {
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

static bool addRxCharSet(Rx *lhs, Rx *rhs) {
  if( rhs->m_t != RxType_CharSet )
    return false;
  if( lhs->m_t == RxType_CharSet ) {
    CharSet_addCharSet(&lhs->m_charset,&rhs->m_charset);
    return true;
  }
  if( lhs->m_t == RxType_BinaryOp && lhs->m_op == BinaryOp_Or ) {
    if( addRxCharSet(lhs->m_lhs,rhs) || addRxCharSet(lhs->m_rhs,rhs) )
      return true;
  }
  return false;
}

static Rx *ParseRxOr(TokStream *s, MapAny /*<String,Rx*>*/ *subs) {
  Rx *lhs = ParseRxConcat(s,subs);
  if( ! lhs )
    return 0;
  int c = TokStream_peekc(s,0);
  while( c == '|' ) {
    TokStream_discard(s,1);
    Rx *rhs = ParseRxConcat(s,subs);
    // special case.  combining two single character regexes
    if( ! addRxCharSet(lhs,rhs) )
      lhs = rxbinary(BinaryOp_Or,lhs,rhs);
    c = TokStream_peekc(s,0);
  }
  return lhs;
}

static Rx *ParseRx(TokStream *s, MapAny /*<String,Rx*>*/ *subs) {
 return ParseRxOr(s, subs);
}

static void ParseSymbol(TokStream *s, String *symbol) {
  size_t n = 0;
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

Nfa *ParseTokenizerFile(TokStream *s, bool verbose) {
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

struct CharSetIter;
typedef struct CharSetIter CharSetIter;
struct CharSetIter {
  CharRange *cur;
  CharRange *end;
};
ElementOps CharSetIterElement = {sizeof(CharSetIter), false, false, 0, 0, 0, 0, 0, 0};

void CharSet_combo_breaker(const VectorAny /*<CharSet>*/ *charsets_initial,
                            ComboBreakerCB cb, void *vpcb) {
  VectorAny /*<CharSetIter>*/ charsets;
  VectorAny /*<CharSetIter>*/ charset_iters;
  VectorAny /*<int>*/ charset_indexes;
  CharSet charset;
  Scope_Push();
  VectorAny_init(&charsets,&CharSetElement,true);
  VectorAny_init(&charset_iters,&CharSetIterElement,true);
  VectorAny_init(&charset_indexes,getIntElement(),true);
  CharSet_init(&charset, true);
  VectorAny_Assign(&charsets,charsets_initial);
  // initialize the iterators
  for( int i = 0, n = VectorAny_size(&charsets); i < n; ++i ) {
    CharSet *cs = &VectorAny_ArrayOpT(&charsets,i,CharSet);
    CharSetIter iter = {0,0};
    iter.cur = &VectorAny_ArrayOpT(&cs->m_ranges,0,CharRange);
    iter.end = iter.cur+VectorAny_size(&cs->m_ranges);
    VectorAny_push_back(&charset_iters,&iter);
  }
  size_t niterators = VectorAny_size(&charset_iters);
  while( niterators ) {
    // get the current shortest match, the entire length of the first iter
    const CharRange *intersect_start = 0;
    const CharRange *intersect_end = 0;
    int chlow = 0;
    // get the list of active iterators, and shorten the match, add the iterator to the list
    VectorAny_clear(&charset_indexes);
    for( int i = 0, n = VectorAny_size(&charset_iters); i < n; ++i ) {
      CharSetIter *iter = &VectorAny_ArrayOpT(&charset_iters,i,CharSetIter);
      if( iter->cur == iter->end )
        continue;
      if( ! intersect_start ) {
        const CharSetIter *first = &VectorAny_ArrayOpConstT(&charset_iters,i,CharSetIter);
        intersect_start = first->cur;
        intersect_end = first->end;
        chlow = intersect_start->m_low;
      }
      if( chlow != iter->cur->m_low )
        continue;
      VectorAny_push_back(&charset_indexes, &i);
      const CharRange *intersect_cur = intersect_start;
      const CharRange *match_cur = iter->cur;
      const CharRange *match_end = iter->end;
      while( intersect_cur != intersect_end && match_cur != match_end && match_cur->m_low == intersect_cur->m_low && match_cur->m_high == intersect_cur->m_high ) {
        ++match_cur;
        ++intersect_cur;
      }
      if( intersect_cur != intersect_end ) {
        if( match_cur == match_end ) {
          // match is shorter
          intersect_start = iter->cur;
          intersect_end = iter->end;
        } else {
          if( intersect_cur->m_low == match_cur->m_low ) {
            // ended in overlap
            if( intersect_cur->m_high < match_cur->m_high ) {
              // intersect side is shorter
              intersect_end = intersect_cur+1;
            } else {
              // match side is shorter
              intersect_start = iter->cur;
              intersect_end = match_cur+1;
            }
          } else {
            // ended with non overlap
            intersect_end = intersect_cur;
          }
        }
      }
    }
    // Make the charset, do the callback
    CharSet_clear(&charset);
    int chhigh = intersect_end[-1].m_high;
    size_t nparts = intersect_end-intersect_start;
    VectorAny_insertMany(&charset.m_ranges,0,intersect_start,nparts);
    cb(&charset, &charset_indexes, vpcb);
    // update the iterators
    for( int i = 0, n = VectorAny_size(&charset_iters); i < n; ++i ) {
      CharSetIter *iter = &VectorAny_ArrayOpT(&charset_iters,i,CharSetIter);
      if( iter->cur == iter->end || chlow != iter->cur->m_low )
        continue;
      if( iter->cur[nparts-1].m_high != chhigh ) {
        // case where we nibble
        iter->cur += nparts-1;
        iter->cur->m_low = chhigh+1;
      } else {
        iter->cur += nparts;
      }
      // iterator exhausted, reduce the count
      if( iter->cur == iter->end )
        --niterators;
    }
  }
  Scope_Pop();
}
