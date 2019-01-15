#include <string.h>
#include <set>
#include <map>
#include <vector>
#include <string>
#include <limits.h>
#include <ctype.h>
#include <iostream>
#include <stdlib.h>
#include <algorithm>
using namespace std;
#include "tokenizer.h"

// rx : simplerx | rx rx | rx '+' | rx '*' | rx '?' | rx '|' rx | '(' rx ')' | rx '{' number '}' | rx '{' number ',' '}' | rx '{' number ',' number '}' | rx '{' ',' number '}'
// simplerx : /[0-9A-Za-z \t]/ | '\\[trnv]' | /\\[.]/ | /(\\x|\\u)[[:xdigit:]]+/ | /./ | charset
// charset : '[' charsetelements ']' | '[~' charsetelements ']'
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

bool TokStream::addc() {
  if( m_in->eof() )
    return false;
  char c;
  if( m_in->read(&c,1).eof() )
    return false;
  if( m_buffill == m_buflen ) {
    int newlen = 0;
    if( m_buflen == 0 ) {
      newlen = 1024;
      m_buf = (char*)malloc(newlen);
      memset(m_buf,0,newlen);
    } else {
      newlen = m_buflen*2;
      m_buf = (char*)realloc(m_buf,newlen);
      memset(m_buf+m_buflen,0,newlen-m_buflen);
      for( int i = 0; i < m_buffill; ++i ) {
        int oldpos = (m_bufpos+i)%m_buflen;
        int newpos = (m_bufpos+i)%newlen;
        if( oldpos == newpos ) continue;
        m_buf[newpos] = m_buf[oldpos];
        m_buf[oldpos] = 0;
      }
    }
    m_buflen = newlen;
  }
  m_buf[(m_bufpos+m_buffill)%m_buflen] = c;
  ++m_buffill;
  return true;
}

TokStream::TokStream(istream *in) {
  m_buf = 0;
  m_buffill = 0;
  m_buflen = 0;
  m_bufpos = 0;
  m_in = in;
  m_pos = 0;
  m_line = m_col = 1;
}
TokStream::~TokStream() {
  if( m_buf )
    free(m_buf);
}
int TokStream::peekc(int n) {
  while( n >= m_buffill ) {
    if( ! addc() )
      return -1;
  }
  return m_buf[(m_bufpos+n)%m_buflen];
}
bool TokStream::peekstr(const char *s) {
  int slen = strlen(s);
  peekc(slen-1);
  char *ss = m_buf+m_bufpos;
  for( int i = 0; i < slen; ++i )
    if( peekc(i) != s[i] )
      return false;
  return true;
}
int TokStream::readc() {
  if( m_buffill == 0 )
    if( ! addc() )
      return -1;
  int c = m_buf[m_bufpos];
  m_bufpos = (m_bufpos+1)%m_buflen;
  --m_buffill;
  if( c == '\n' ) {
    ++m_line;
    m_col = 1;
  } else
    ++m_col;
  return c;
}
void TokStream::discard(int n) {
  for( int i = 0; i < n; ++i )
    readc();
}
int TokStream::line() { return m_line; }
int TokStream::col() { return m_col; }

static void error(TokStream &s, const char *err) {
  throw ParseException(s.line(),s.col(),err);
}

CharRange::CharRange(int low, int high) : m_low(low), m_high(high) {}
bool CharRange::contains(int i) const {
  if( i >= m_low && i <= m_high )
    return true;
  return false;
}
bool CharRange::contains(const CharRange &rhs) const {
  if( rhs.m_low >= m_low && rhs.m_high <= m_high )
    return true;
  return false;
}
bool CharRange::overlaps(int low, int high) const {
  if( low > m_high || high < m_low )
    return false;
  return true;
}

CharIterator::CharIterator(vector<CharRange>::const_iterator cur, vector<CharRange>::const_iterator end, int curchar) {
  m_cur = cur;
  m_end = end;
  m_curchar = curchar;
}
int CharIterator::operator*() const {
  return m_curchar;
}
CharIterator CharIterator::operator++() {
  if( m_cur != m_end ) {
    if( m_curchar == m_cur->m_high ) {
      ++m_cur;
      if( m_cur != m_end )
        m_curchar = m_cur->m_low;
      else
        m_curchar = -1;
    }
    else
      ++m_curchar;
  }
  return *this;
}
CharIterator CharIterator::operator++(int) {
  CharIterator ret = *this;
  operator++();
  return ret;
}
bool CharIterator::operator==(const CharIterator &rhs) const {
  if( m_cur == rhs.m_cur && m_end == rhs.m_end && m_curchar == rhs.m_curchar )
    return true;
  return false;
}
bool CharIterator::operator!=(const CharIterator &rhs) const {
  if( m_cur != rhs.m_cur || m_end != rhs.m_end || m_curchar != rhs.m_curchar )
    return true;
  return false;
}

int CharSet::find(int c, bool &found) const {
  int low = 0, high = m_ranges.size()-1;
  while( low <= high ) {
    int mid = (low+high)/2;
    const CharRange &range = m_ranges[mid];
    if( range.contains(c) ) {
      found = true;
      return mid;
    }
    if( c < range.m_low )
      high = mid-1;
    else if( c > range.m_high )
      low = mid+1;
    else {
      found = true;
      return mid;
    }
  }
  found = false;
  return low;
}

bool CharSet::contains(const CharRange &range) const {
  bool found = false;
  int i = find(range.m_low,found);
  if( ! found )
    return false;
  return m_ranges[i].contains(range);
}

CharSet::iterator CharSet::begin() const {
  return m_ranges.begin();
}
CharSet::iterator CharSet::end() const {
  return m_ranges.end();
}
CharSet::char_iterator CharSet::begin_char() const {
  if( m_ranges.empty() )
    return char_iterator(m_ranges.end(), m_ranges.end(), -1);
  return char_iterator(m_ranges.begin(), m_ranges.end(), m_ranges.begin()->m_low);
}

CharSet::char_iterator CharSet::end_char() const {
  return char_iterator(m_ranges.end(), m_ranges.end(), -1);
}
void CharSet::clear() {
  m_ranges.clear();
}
bool CharSet::empty() const {
  return m_ranges.empty();
}
void CharSet::addChar(int i) {
  addCharRange(i,i);
}
void CharSet::addCharRange(int low, int high) {
  bool found;
  int i = find(low,found);
  if( found ) {
    if( low < m_ranges[i].m_low )
      m_ranges[i].m_low = low;
    if( high > m_ranges[i].m_high )
      m_ranges[i].m_high = high;
  } else {
    m_ranges.insert(m_ranges.begin()+i,CharRange(low,high));
  }
  while( i < m_ranges.size()-1 && (m_ranges[i+1].m_low-m_ranges[i].m_high <= 1) ) {
    m_ranges[i].m_high = m_ranges[i+1].m_high;
    m_ranges.erase(m_ranges.begin()+i+1);
  }
  if( i > 0 ) {
    while( i < m_ranges.size() && (m_ranges[i].m_low-m_ranges[i-1].m_high <= 1) ) {
      m_ranges[i-1].m_high = m_ranges[i].m_high;
      m_ranges.erase(m_ranges.begin()+i);
    }
  }
}
void CharSet::addCharset(const CharSet &rhs) {
  for( vector<CharRange>::const_iterator cur = rhs.m_ranges.begin(), end = rhs.m_ranges.end(); cur != end; ++cur )
    addCharRange(cur->m_low, cur->m_high);
}
//#define splitTEST
void CharSet::split(int low, int high) {
#ifdef splitTEST
  vector<CharRange> rangesBefore = m_ranges;
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
  splitRec(low,high,-1);
#endif
}

int CharSet::splitRec(int low, int high, int i) {
  if( i == -1 ) {
    bool found = false;
    i = find(low,found);
  }
  if( i == m_ranges.size() || ! m_ranges[i].overlaps(low,high) ) {
    m_ranges.insert(m_ranges.begin()+i,CharRange(low,high));
    return 1;
  }

  int adjust = 0;
  CharRange range = m_ranges[i];

  if( low < range.m_low ) {
    int tmp = splitRec(low,range.m_low-1,i);
    i += tmp;
    adjust += tmp;
  } else if( low > range.m_low ) {
    m_ranges[i].m_low = low;
    int tmp = splitRec(range.m_low,low-1,i);
    i += tmp;
    adjust += tmp;
  }

  if( high < range.m_high ) {
    m_ranges[i].m_high = high;
    splitRec(high+1,range.m_high,i+1);
  } else if( high > range.m_high ) {
    splitRec(range.m_high+1,high,i+1);
  }

  return adjust;
}
void CharSet::split(const CharSet &rhs) {
  for( vector<CharRange>::const_iterator cur = rhs.m_ranges.begin(), end = rhs.m_ranges.end(); cur != end; ++cur )
    split(cur->m_low, cur->m_high);
}
void CharSet::negate() {
  vector<CharRange> ranges;
  int prev = 0;
  for( vector<CharRange>::iterator cur = m_ranges.begin(), end = m_ranges.end(); cur != end; ++cur ) {
    if( prev < cur->m_low )
      ranges.push_back(CharRange(prev,cur->m_low-1));
    prev = cur->m_high+1;
  }
  if( prev < INT_MAX )
    ranges.push_back(CharRange(prev,INT_MAX));
  m_ranges = ranges;
}
bool CharSet::operator!() const {
  if( ! m_ranges.size() )
    return true;
  return false;
}

Transition::Transition(int from, int to) : m_from(from), m_to(to) {}

bool Transition::operator<(const Transition &rhs) const {
  if( m_from < rhs.m_from )
    return true;
  if( m_from == rhs.m_from &&  m_to < rhs.m_to)
    return true;
  return false;
}

Nfa::Nfa() : m_nextState(0) {
}
void Nfa::addNfa(const Nfa &nfa) {
  int states = m_nextState;
  m_nextState += nfa.stateCount();
  for( set<int>::const_iterator cur = nfa.m_startStates.begin(), end = nfa.m_startStates.end(); cur != end; ++cur )
    m_startStates.insert(*cur+states);
  for( set<int>::const_iterator cur = nfa.m_endStates.begin(), end = nfa.m_endStates.end(); cur != end; ++cur )
    m_endStates.insert(*cur+states);
  for( map<Transition,CharSet>::const_iterator cur = nfa.m_transitions.begin(), end = nfa.m_transitions.end(); cur != end; ++cur )
    m_transitions[Transition(cur->first.m_from+states,cur->first.m_to+states)] = cur->second;
  for( set<Transition>::const_iterator cur = nfa.m_emptytransitions.begin(), end = nfa.m_emptytransitions.end(); cur != end; ++cur )
    m_emptytransitions.insert(Transition(cur->m_from+states,cur->m_to+states));
  for( map<int,int>::const_iterator cur = nfa.m_token2state.begin(), end = nfa.m_token2state.end(); cur != end; ++cur )
    m_token2state[cur->first+states] = cur->second;
  for( map<int,Token>::const_iterator cur = nfa.m_tokendefs.begin(), end = nfa.m_tokendefs.end(); cur != end; ++cur )
    setTokenDef(cur->second.m_token,cur->second.m_isws,cur->second.m_name);
}
int Nfa::stateCount() const { return m_nextState; }
int Nfa::addState() { return m_nextState++; }
void Nfa::addStartState(int startstate) { m_startStates.insert(startstate); }
void Nfa::addEndState(int endstate) { m_endStates.insert(endstate); }
void Nfa::addTransition(int from, int to, int symbol) {
  map<Transition,CharSet>::iterator iter = m_transitions.find(Transition(from,to));
  if( iter == m_transitions.end() ) {
    CharSet charset;
    charset.addChar(symbol);
    m_transitions[Transition(from,to)] = charset;
  } else
    iter->second.addChar(symbol);
}
void Nfa::addTransition(int from, int to, const CharRange &range) {
  map<Transition,CharSet>::iterator iter = m_transitions.find(Transition(from,to));
  if( iter == m_transitions.end() ) {
    CharSet charset;
    charset.addCharRange(range.m_low,range.m_high);
    m_transitions[Transition(from,to)] = charset;
  } else
    iter->second.addCharRange(range.m_low,range.m_high);
}
void Nfa::addTransition(int from, int to, const CharSet &charset) {
  map<Transition,CharSet>::iterator iter = m_transitions.find(Transition(from,to));
  if( iter == m_transitions.end() )
    m_transitions[Transition(from,to)] = charset;
  else
    iter->second.addCharset(charset);
}
void Nfa::addEmptyTransition(int from, int to) { m_emptytransitions.insert(Transition(from,to)); }
bool Nfa::hasTokenDef(int token)  const {
  return (m_tokendefs.find(token) != m_tokendefs.end());
}
void Nfa::setTokenDef(int token, bool isws, const string name) {
  if( m_tokendefs.find(token) == m_tokendefs.end() ) {
    m_tokendefs[token] = Token(token,isws,name);
  } else {
    m_tokendefs[token].m_isws = isws;
    m_tokendefs[token].m_name = name;
  }
}
Token Nfa::getTokenDef(int token) const {
  if( m_tokendefs.find(token) == m_tokendefs.end() )
    return Token();
  return m_tokendefs.find(token)->second;
}
vector<Token> Nfa::getTokenDefs() const {
  vector<Token> tokendefs;
  for( map<int,Token>::const_iterator cur = m_tokendefs.begin(), end = m_tokendefs.end(); cur != end; ++cur ) {
    tokendefs.push_back(cur->second);
  }
  sort(tokendefs.begin(), tokendefs.end());
  return tokendefs;
}
bool Nfa::stateHasToken(int state) const {
  return (m_token2state.find(state) != m_token2state.end());
}
int Nfa::getStateToken(int state) const {
  map<int,int>::const_iterator iter = m_token2state.find(state);
  if( iter != m_token2state.end() )
    return iter->second;
  return -1;
}
void Nfa::setStateToken(int state, int token) {
  map<int,int>::iterator iter = m_token2state.find(state);
  if( iter == m_token2state.end() )
    m_token2state[state] = token;
  else
    iter->second = token;
}
void Nfa::clear() {
  m_nextState = 0;
  m_startStates.clear();
  m_endStates.clear();
  m_transitions.clear();
  m_emptytransitions.clear();
  m_token2state.clear();
  m_tokendefs.clear();
}
void Nfa::closure(const map<int,set<int> > &emptytransitions, set<int> &states) const {
  set<int> nextstates = states;
  vector<int> vecstates;
  vecstates.insert(vecstates.end(),nextstates.begin(),nextstates.end());
  for( int i = 0; i < vecstates.size(); ++i )
  {
    map<int,set<int> >::const_iterator e = emptytransitions.find(vecstates[i]);
    if( e == emptytransitions.end() )
      continue;
    const set<int> &intset = e->second;
    for( set<int>::const_iterator cur = intset.begin(), end = intset.end(); cur != end; ++cur ) {
      if( nextstates.find(*cur) != nextstates.end() )
        continue;
      nextstates.insert(*cur);
      vecstates.push_back(*cur);
    }
  }
  states = nextstates;
}
void Nfa::follow( const CharRange &range, const set<int> &states, set<int> &nextstates ) const {
  nextstates.clear();
  for( map<Transition,CharSet>::const_iterator cur = m_transitions.begin(), end = m_transitions.end(); cur != end; ++cur ) {
    if( states.find(cur->first.m_from) == states.end() || ! cur->second.contains(range) )
      continue;
    nextstates.insert(cur->first.m_to);
  }
}
void Nfa::stateTransitions(const set<int> &states, CharSet &transitions) const {
  transitions.clear();
  for( map<Transition,CharSet>::const_iterator cur = m_transitions.begin(), end = m_transitions.end(); cur != end; ++cur ) {
    set<int>::const_iterator iter = states.find(cur->first.m_from);
    if( iter == states.end() )
      continue;
    transitions.split(cur->second);
  }
}
bool Nfa::hasEndState(const set<int> &states) const {
  for( set<int>::const_iterator cur = states.begin(), end = states.end(); cur != end; ++cur ) {
    if( m_endStates.find(*cur) != m_endStates.end() )
      return true;
  }
  return false;
}
int Nfa::lowToken(const set<int> &states) const {
  int finaltok = -1;
  for( set<int>::const_iterator cur = states.begin(), end = states.end(); cur != end; ++cur ) {
    int tok = getStateToken(*cur);
    if( tok == -1 )
      continue;
    else if( finaltok == -1 || tok < finaltok )
      finaltok = tok;
  }
  return finaltok;
}
void Nfa::toDfa(Nfa &dfa) const {
  dfa.clear();
  vector< set<int> > newstates;
  set<int> nextstate;
  map<int,set<int> > emptytransitions;
  for( set<Transition>::const_iterator cur = m_emptytransitions.begin(), end = m_emptytransitions.end(); cur != end; ++cur ) {
    if( emptytransitions.find(cur->m_from) == emptytransitions.end() )
      emptytransitions[cur->m_from] = set<int>();
    emptytransitions[cur->m_from].insert(cur->m_to);
  }
  nextstate = m_startStates;
  closure(emptytransitions, nextstate);
  newstates.push_back(nextstate);
  dfa.addStartState(dfa.addState());
  for( int stateno = 0; stateno < newstates.size(); ++stateno ) {
    const set<int> state = newstates[stateno];
    CharSet transitions;
    stateTransitions(state, transitions);
    for( CharSet::iterator cur = transitions.begin(), end = transitions.end(); cur != end; ++cur ) {
      follow(*cur, state, nextstate);
      closure(emptytransitions, nextstate);
      if(nextstate.size() == 0 )
        continue;
      vector< set<int> >::iterator iter = find(newstates.begin(), newstates.end(), nextstate);
      int nextstateno = -1;
      if( iter == newstates.end() ) {
        newstates.push_back(nextstate);
        nextstateno = dfa.addState();
        if( hasEndState(nextstate) )
          dfa.addEndState(nextstateno);
        int token = lowToken(nextstate);
        if( token != -1 ) {
          if( ! dfa.hasTokenDef(token) ) {
            Token tokendef = getTokenDef(token);
            dfa.setTokenDef(token, tokendef.m_isws, tokendef.m_name);
          }
          dfa.setStateToken(nextstateno,token);
        }
      } else {
        nextstateno = iter-newstates.begin();
      }
      dfa.addTransition(stateno,nextstateno,*cur);
    }
  }
}

void Rx::init() {
  m_t = RxType_None;
  m_op = BinaryOp_None;
  m_lhs = m_rhs = 0;
  m_low = m_high = -1;
}

Rx::Rx() {
  init();
}

Rx::Rx(char c) {
  init();
  m_t = RxType_CharSet;
  m_charset.addChar(c);
}

Rx::Rx(const CharSet &charset) {
  init();
  m_t = RxType_CharSet;
  m_charset = charset;
}

Rx::Rx(BinaryOp op,Rx *lhs, Rx *rhs) {
  init();
  m_t = RxType_BinaryOp;
  m_op = op;
  m_lhs = lhs;
  m_rhs = rhs;
}

Rx::Rx(Rx *rx, int count) {
  init();
  m_t = RxType_Many;
  m_lhs = rx;
  m_low = m_high = count;
  if( m_low < 0 )
    m_low = 0;
  if( m_high < 0 )
    m_high = -1;
}

Rx::Rx(Rx *rx, int low, int high) {
  init();
  m_t = RxType_Many;
  m_lhs = rx;
  m_low = low;
  m_high = high;
  if( m_low < 0 )
    m_low = 0;
  if( m_high < 0 )
    m_high = -1;
}

Rx *Rx::clone() {
  Rx *rx = new Rx();
  rx->m_t = m_t;
  rx->m_op = m_op;
  rx->m_charset = m_charset;
  rx->m_lhs = m_lhs ? m_lhs->clone() : 0;
  rx->m_rhs = m_rhs ? m_rhs->clone() : 0;
  rx->m_low = m_low;
  rx->m_high = m_high;
  return rx;
}

int Rx::addToNfa(Nfa &nfa, int startState) {
  if( m_t == RxType_CharSet ) {
    int endState = nfa.addState();
    nfa.addTransition(startState,endState,m_charset);
    return endState;
  } else if( m_t == RxType_BinaryOp ) {
    if( m_op == BinaryOp_Or ) {
      int endlhs = m_lhs->addToNfa(nfa, startState);
      int endrhs = m_rhs->addToNfa(nfa, startState);
      int endState = nfa.addState();
      nfa.addEmptyTransition(endlhs,endState);
      nfa.addEmptyTransition(endrhs,endState);
      return endState;
    } else if( m_op == BinaryOp_Concat ) {
      int endlhs = m_lhs->addToNfa(nfa, startState);
      int endrhs = m_rhs->addToNfa(nfa, endlhs);
      return endrhs;
    }
  } else if( m_t == RxType_Many ) {
    int prevState = nfa.addState();
    nfa.addEmptyTransition(startState,prevState);
    int endState = nfa.addState();
    int loopState = prevState;
    if( m_low == 0 ) {
      loopState = prevState;
      prevState = m_lhs->addToNfa(nfa, prevState);
      nfa.addEmptyTransition(loopState,prevState);
    } else {
      for( int i = 0; i < m_low; ++i ) {
        loopState = prevState;
        prevState = m_lhs->addToNfa(nfa, prevState);
      }
    }
    if( m_high == -1 ) {
      // endless loop back
      nfa.addEmptyTransition(prevState,loopState);
    } else {
      for( int i = m_low; i < m_high; ++i ) {
        nfa.addEmptyTransition(prevState,endState);
        prevState = m_lhs->addToNfa(nfa, prevState);
      }
    }
    nfa.addEmptyTransition(prevState,endState);
    return endState;
  }
  return -1;
}

Nfa *Rx::toNfa() {
  Nfa *nfa = new Nfa();
  int endState = addToNfa(*nfa, nfa->addState());
  nfa->addEndState(endState);
  return nfa;
}

static Rx *rxbinary(BinaryOp op, Rx *lhs, Rx *rhs) {
  if( ! lhs && ! rhs )
    return 0;
  if( ! lhs )
    return rhs;
  if( ! rhs )
    return lhs;
  return new Rx(op, lhs, rhs);
}

static Rx *rxmany(Rx *rx, int low, int high) {
  return new Rx(rx,low,high);
}

static Rx *rxchar(int c) {
  return new Rx(c);
}

static Rx *rxcharset(const CharSet &charset) {
  if( ! charset )
    return 0;
  return new Rx(charset);
}

static Rx *rxany() {
  CharSet charset;
  charset.negate();
  return new Rx(charset);
}

static int ParseNumber(TokStream &s, int start, int end) {
  if( start == end )
    return -1;
  int i = 0;
  while( start < end ) {
    int c = s.peekc(start++);
    i *= 10;
    i += c-'0';
  }
  return i;
}

static Rx *ParseRx(TokStream &s, map<string,Rx*> &subs);

static bool ParseSub(TokStream &s, string &sub) {
  if( s.peekc() != '{' )
    return false;
  if( ! isalpha(s.peekc(1)) )
    return false;
  int i = 1;
  while( isalnum(s.peekc(i+1)) )
    ++i;
  if( s.peekc(i+1) != '}' )
    return false;
  s.discard();
  sub.clear();
  while( i > 0 ) {
    sub += s.readc();
    --i;
  }
  s.discard();
  return true;
}

static bool ParseRepeat(TokStream &s, int &low, int &high) {
  int c = s.peekc();
  if( s.peekc() != '{' )
    return false;
  int startn1 = 1;
  int endn1 = 1;
  while( isdigit(s.peekc(endn1)) )
    ++endn1;
  if( s.peekc(endn1) == ',' ) {
    int startn2 = endn1+1;
    int endn2 = startn2;
    while( isdigit(s.peekc(endn2)) )
      ++endn2;
    if( s.peekc(endn2) != '}' )
      return false;
    if( endn1 == startn1 && endn2 == startn2 )
      return false;
    low = ParseNumber(s,startn1,endn1);
    high = ParseNumber(s,startn2,endn2);
    s.discard(endn2+1);
    return true;
  } else if( s.peekc(endn1) == '}' ) {
    if( endn1 == startn1 )
      return false;
    low = high = ParseNumber(s,startn1,endn1);
    s.discard(endn1+1);
    return true;
  }
  return false;
}

static int ParseChar(TokStream &s) {
  int c = s.peekc();
  if( c == '[' || c == ']' )
    return -1;
  else if( c == '\\' ) {
    s.discard();
    c = s.peekc();
    if( c == 'n' ) {
      s.discard();
      return '\n';
    } else if( c == 't' ) {
      s.discard();
      return '\t';
    } else if( c == 'r' ) {
      s.discard();
      return '\r';
    } else if( c == 'n' ) {
      s.discard();
      return '\n';
    } else if( c == 'v' ) {
      s.discard();
      return '\v';
    } else if( c == 'x' || c == 'u' ) {
      int ltr = c;
      s.discard();
      c = s.peekc();
      if( isxdigit(c) ) {
        ltr = 0;
        while( isxdigit(c) ) {
          ltr *= 16;
          ltr += xvalue(c);
          s.discard();
          c = s.peekc();
        }
        return ltr;
      } else {
        return ltr;
      }
    } else {
      s.discard();
      return c;
    }
  } else {
    s.discard();
    return c;
  }
  return -1;
}

static Rx *ParseCharSet(TokStream &s) {
  CharSet charset;
  int c = s.peekc();
  if( c != '[' )
    return 0;
  s.discard();
  c = s.peekc();
  bool negate = (c == '~');
  if( negate ) {
    s.discard();
    c = s.peekc();
  }
  while( c != -1 ) {
    if( c == ']' ) {
      s.discard();
      break;
    } else if( c == '[' ) {
      if( s.peekstr("[:alpha:]") ) {
        s.discard(9);
        charset.addCharRange('a','z');
        charset.addCharRange('A','Z');
      } else if( s.peekstr("[:digit:]") ) {
        s.discard(9);
        charset.addCharRange('0','9');
      } else if( s.peekstr("[:xdigit:]") ) {
        s.discard(10);
        charset.addCharRange('0','9');
        charset.addCharRange('a','f');
        charset.addCharRange('A','F');
      } else if( s.peekstr("[:alnum:]") ) {
        s.discard(9);
        charset.addCharRange('0','9');
        charset.addCharRange('a','z');
        charset.addCharRange('A','Z');
      } else if( s.peekstr("[:space:]") ) {
        s.discard(9);
        charset.addChar(' ');
        charset.addChar('\t');
      } else if( s.peekstr("[:white:]") ) {
        s.discard(9);
        charset.addChar(' ');
        charset.addChar('\t');
        charset.addChar('\r');
        charset.addChar('\n');
      } else {
        charset.addChar(c);
        s.discard();
      }
      c = s.peekc();
    } else {
      int ch = ParseChar(s);
      if( ch != -1 ) {
        c = s.peekc();
        if( c == '-' ) {
          s.discard();
          c = s.peekc();
          int ch2 = ParseChar(s);
          if( ch2 != -1 ) {
            c = s.peekc();
            charset.addCharRange(ch,ch2);
          } else
            charset.addChar(ch);
        }
        else
          charset.addChar(ch);
      }
    }
  }
  if( negate )
    charset.negate();
  return rxcharset(charset);
}

static Rx *ParseRxSimple(TokStream &s, map<string,Rx*> &subs) {
  int c = s.peekc();

  if( c == -1 || strchr("/+*?",c) )
    return 0;

  if( c == '{' ) {
    string sub;
    if( ! ParseSub(s,sub)  )
      return 0;
    if( subs.find(sub) == subs.end() )
      error(s,"unknown substitution");
    return subs[sub]->clone();
  }
  else if( c == '.' ) {
    s.discard();
    return rxany();
  } else if( c == '\\' ) {
    s.discard();
    c = s.peekc();
    if( c == 'n' ) {
      s.discard();
      return rxchar('\n');
    } else if( c == 't' ) {
      s.discard();
      return rxchar('\t');
    } else if( c == 'r' ) {
      s.discard();
      return rxchar('\r');
    } else if( c == 'n' ) {
      s.discard();
      return rxchar('\n');
    } else if( c == 'v' ) {
      s.discard();
      return rxchar('\v');
    } else if( c == 'x' || c == 'u' ) {
      int ltr = c;
      s.discard();
      c = s.peekc();
      if( isxdigit(c) ) {
        ltr = 0;
        while( isxdigit(c) ) {
          ltr *= 16;
          ltr += xvalue(c);
          s.discard();
          c = s.peekc();
        }
        return rxchar(ltr);
      } else {
        return rxchar(ltr);
      }
    } else {
      s.discard();
      return rxchar(c);
    }
  } else if( c == '[' ) {
    return ParseCharSet(s);
  }
  else if( c == '(' ) {
   s.discard();
   Rx *r = ParseRx(s,subs);
   c = s.peekc();
   if( c != ')' )
    error(s,"unbalanced '('");
   s.discard();
   return r;
  } else if( c == ')' ) {
   return 0;
  } else  {
    s.discard();
    return rxchar(c);
  }

  error(s,"parse error");
  return 0;
}

static Rx *ParseRxMany(TokStream &s, map<string,Rx*> &subs) {
  Rx* r = 0;
  int low, high;
  r = ParseRxSimple(s,subs);
  if( ! r )
    return 0;
  int c = s.peekc();

  while( c != -1 ) {
    if( c == '+' ) {
      r = rxmany(r,1,-1);
      s.discard();
      c = s.peekc();
    } else if( c == '*' ) {
      r = rxmany(r,0,-1);
      s.discard();
      c = s.peekc();
    } else if( c == '?' ) {
      r = rxmany(r,0,1);
      s.discard();
      c = s.peekc();
    } else if( ParseRepeat(s,low,high) ) {
      r = rxmany(r,low,high);
      c = s.peekc();
    } else {
      break;
    }
  }
  return r;
}

static Rx *ParseRxConcat(TokStream &s, map<string,Rx*> &subs) {
  Rx *lhs = ParseRxMany(s,subs);
  if( ! lhs )
    return 0;
  int c = s.peekc();
  while( c != -1 && c != '/' ) {
    Rx *rhs = ParseRxMany(s,subs);
    if( ! rhs )
      break;
    lhs = rxbinary(BinaryOp_Concat, lhs, rhs);
    c = s.peekc();
  }
  return lhs;
}

static Rx *ParseRxOr(TokStream &s, map<string,Rx*> &subs) {
  Rx *lhs = ParseRxConcat(s,subs);
  if( ! lhs )
    return 0;
  int c = s.peekc();
  while( c == '|' ) {
    s.discard();
    Rx *rhs = ParseRxConcat(s,subs);
    lhs = rxbinary(BinaryOp_Or,lhs,rhs);
    c = s.peekc();
  }
  return lhs;
}

static Rx *ParseRx(TokStream &s, map<string,Rx*> &subs) {
 return ParseRxOr(s, subs);
}

static string ParseSymbol(TokStream &s) {
  string symbol;
  int c = s.peekc();
  if( ! isalpha(c) )
    return symbol;
  symbol += c;
  s.discard();
  c = s.peekc();
  while( isalnum(c) ) {
    symbol += c;
    s.discard();
    c = s.peekc();
  }
  return symbol;
}

static void ParseWs(TokStream &s) {
  int c = s.peekc();
  while( isspace(c) ) {
    s.discard();
    c = s.peekc();
  }
}

static Rx *ParseRegex(TokStream &s, map<string,Rx*> &subs) {
  int c = s.peekc();
  if( c != '/' )
    error(s,"No regex");
  s.discard();
  Rx *rx = ParseRx(s,subs);
  if( ! rx )
    error(s,"empty regex");
  c = s.peekc();
  if( c != '/' ) {
    delete rx;
    if( c == ')' )
      error(s,"unbalanced ')'");
    else
      error(s,"No trailing '/' on regex");
  }
  s.discard();
  return rx;
}

Nfa *ParseTokenizerFile(TokStream &s) {
  set<string> sections;
  map<string,int> sectionNumbers;
  sectionNumbers["default"] = 0;
  int curSection = 0;
  int nextSectionNumber = 1;
  Nfa nfa;
  int startTokState = nfa.addState();
  nfa.addStartState(startTokState);
  int startState = nfa.addState();
  nfa.addTransition(startTokState,startState,curSection);
  map<string,Rx*> subs;
  set<int> wstoks;
  ParseWs(s);
  int c = s.peekc();
  int tokid = 0;
  while( c != -1 ) {
    bool isws = false;
    bool issub = false;
    if( c == '~' ) {
      isws = true;
      s.discard();
    } else if( c == '=' ) {
      issub = true;
      s.discard();
    } else if( c == '-' ) {
      s.discard();
      ParseWs(s);
      string label = ParseSymbol(s);
      if( ! label.length() )
        error(s,"No section label");
      if( sectionNumbers.find(label) == sectionNumbers.end() )
        sectionNumbers[label] = nextSectionNumber++;
      if( sections.find(label) == sections.end() )
        sections.insert(label);
      else
        error(s,"the section name is already used");
      if( nfa.stateCount() == 0 )
        error(s,"sections may not be empty");
      curSection = sectionNumbers[label];
      startTokState = nfa.addState();
      nfa.addStartState(startTokState);
      startState = nfa.addState();
      nfa.addTransition(startTokState,startState,curSection);
    }
    ParseWs(s);
    c = s.peekc();
    string symbol = ParseSymbol(s);
    if( ! symbol.length() )
      error(s,"No symbol");
    ParseWs(s);
    c = s.peekc();
    Rx *rx = ParseRegex(s,subs);
    if( ! rx )
      error(s,"No regex");
    ParseWs(s);
    c = s.peekc();
    if( issub ) {
      if( subs.find(symbol) != subs.end() ) {
        delete subs[symbol];
        subs.erase(symbol);
      }
      subs[symbol] = rx;
    } else {
      int endState = rx->addToNfa(nfa, startState);
      nfa.setTokenDef(tokid,isws,symbol);
      nfa.addEndState(endState);
      nfa.setStateToken(endState,tokid);
      ++tokid;
    }
  }
  Nfa *dfa = new Nfa();
  nfa.toDfa(*dfa);
  return dfa;
}

class LanguageOutputter {
public:
  virtual ~LanguageOutputter() {}
  virtual void outDecl(ostream &out, const char *type, const char *name) const = 0;
  virtual void outArrayDecl(ostream &out, const char *type, const char *name) const = 0;
  virtual void outStartArray(ostream &out) const = 0;
  virtual void outEndArray(ostream &out) const = 0;
  virtual void outEndStmt(ostream &out) const = 0;
  virtual void outNull(ostream &out) const = 0;
  virtual void outBool(ostream &out, bool b) const = 0;
  virtual void outStr(ostream &out, const string &str) const = 0;
  virtual void outChar(ostream &out, int c) const = 0;
};

class CLanguageOutputter : public LanguageOutputter {
public:
  virtual void outDecl(ostream &out, const char *type, const char *name) const { out << type << ' ' << name; }
  virtual void outArrayDecl(ostream &out, const char *type, const char *name) const { out << type << ' ' << name << "[]"; }
  virtual void outStartArray(ostream &out) const { out << '{'; }
  virtual void outEndArray(ostream &out) const  { out << '}'; }
  virtual void outEndStmt(ostream &out) const  { out << ';'; }
  virtual void outNull(ostream &out) const  { out << '0'; }
  virtual void outBool(ostream &out, bool b) const  { out << (b ? "true" : "false"); }
  virtual void outStr(ostream &out, const string &str) const  {
    string tmp = str;
    //tmp.replaceall("\\","\\\\");
    //tmp.replaceall(tmp.begin(),"\"","\\\"");
    out << '"' << tmp << '"';
  }
  virtual void outChar(ostream &out, int c) const  {
    if(c == '\r' ) {
      out << "'\\r'";
    } else if(c == '\n' ) {
      out << "'\\n'";
    } else if(c == '\v' ) {
      out << "'\\v'";
    } else if(c == ' ' ) {
      out << "' '";
    } else if(c == '\t' ) {
      out << "'\\t'";
    } else if(c == '\\' || c == '\'' ) {
      out << "\'\\" << (char)c << '\''; 
    } else if( isgraph(c) ) {
      out << '\'' << (char)c << '\''; 
    } else
      out << c;
  }
};

class JsLanguageOutputter : public LanguageOutputter {
public:
  virtual void outDecl(ostream &out, const char *type, const char *name) const { out << "var " << name; }
  virtual void outArrayDecl(ostream &out, const char *type, const char *name) const { out << "var " << name; }
  virtual void outStartArray(ostream &out) const { out << '['; }
  virtual void outEndArray(ostream &out) const { out << ']'; }
  virtual void outEndStmt(ostream &out)  const { out << ';'; }
  virtual void outNull(ostream &out) const  { out << 'null'; }
  virtual void outBool(ostream &out, bool b) const  { out << (b ? "true" : "false"); }
  virtual void outStr(ostream &out, const string &str) const  { out << '"' << str << '"'; }
  virtual void outChar(ostream &out, int c) const  { out << c; }
};

LanguageOutputter *getLanguageOutputter(OutputLanguage language) {
  if( language == LanguageC )
    return new CLanguageOutputter();
  else if( language == LanguageJs )
    return new  JsLanguageOutputter();
  return 0;
}

static void OutputDfaSource(ostream &out, const Nfa &dfa, const LanguageOutputter &lang) {
  // Going to assume there are no gaps in toking numbering
  vector<Token> tokens = dfa.getTokenDefs();

  lang.outDecl(out,"const int","tokenCount");
  out << " = " << tokens.size();
  lang.outEndStmt(out);
  out << endl;

  for( vector<Token>::iterator cur = tokens.begin(), end = tokens.end(); cur != end; ++cur ) {
    lang.outDecl(out,"const int",cur->m_name.c_str());
    out << " = " << cur->m_token;
    lang.outEndStmt(out);
    out << endl;
  }

  lang.outArrayDecl(out, "const char*", "tokenstr");
  out << " = ";
  lang.outStartArray(out);
  bool first = true;
  for( vector<Token>::iterator cur = tokens.begin(), end = tokens.end(); cur != end; ++cur ) {
    if( first )
      first = false;
    else
      out << ',';
    lang.outStr(out,cur->m_name);
  }
  lang.outEndArray(out);
  lang.outEndStmt(out);
  out << endl;

  lang.outArrayDecl(out, "int","isws");
  out << " = ";
  lang.outStartArray(out);
  first = true;
  for( vector<Token>::iterator cur = tokens.begin(), end = tokens.end(); cur != end; ++cur ) {
    if( first )
      first = false;
    else
      out << ',';
    lang.outBool(out,cur->m_isws);
  }
  lang.outEndArray(out);
  lang.outEndStmt(out);
  out << endl;

  CharSet ranges;
  set<int> allstates;
  for( int i = 0, n = dfa.stateCount(); i < n; ++i )
    allstates.insert(i);
  dfa.stateTransitions(allstates,ranges);
  lang.outArrayDecl(out,"int","ranges");
  out << " = ";
  lang.outStartArray(out);
  first = true;
  for( CharSet::iterator cur = ranges.begin(), end = ranges.end(); cur != end; ++cur ) {
    if( first )
      first = false;
    else
      out << ',';
    lang.outChar(out,cur->m_low);
    out << ',';
    lang.outChar(out,cur->m_high);
  }
  lang.outEndArray(out);
  lang.outEndStmt(out);
  out << endl;

  lang.outDecl(out,"const int","stateCount");
  out << " = " << dfa.stateCount();
  lang.outEndStmt(out);
  out << endl;

  lang.outArrayDecl(out,"int","transitions");
  out << " = ";
  lang.outStartArray(out);
  first = true;
  vector<int> transitioncounts;
  for( int i = 0, n = dfa.stateCount(); i < n; ++i ) {
    vector<Transition> transitions;
    set<int> state, nextstate;
    state.insert(i);
    for( CharSet::iterator cur = ranges.begin(), end = ranges.end(); cur != end; ++cur ) {
      dfa.follow(*cur,state,nextstate);
      if( ! nextstate.size() )
        continue;
      transitions.push_back(Transition(cur-ranges.begin(),*nextstate.begin()));
    }
    transitioncounts.push_back(transitions.size());
    for( int i = 0; i < transitions.size(); ++i ) {
      if( first )
        first = false;
      else
        out << ',';
      out << transitions[i].m_from << ',' << transitions[i].m_to;
    }
  }
  lang.outEndArray(out);
  lang.outEndStmt(out);
  out << endl;

  int offset = 0;
  lang.outArrayDecl(out,"int","transitionOffset");
  out << " = ";
  lang.outStartArray(out);
  first = true;
  for( int i = 0, n = transitioncounts.size(); i < n; ++i ) {
    if( first )
      first = false;
    else
      out << ',';
    out << offset;
    offset += transitioncounts[i];
  }
  if( first )
    first = false;
  else
    out << ',';
  out << offset;
  lang.outEndArray(out);
  lang.outEndStmt(out);
  out << endl;

  first = true;
  lang.outArrayDecl(out,"int","tokens");
  out << " = ";
  lang.outStartArray(out);
  for( int i = 0, n = dfa.stateCount(); i < n; ++i ) {
    if( first )
      first = false;
    else
      out << ',';
    int tok = dfa.getStateToken(i);
    out << tok;
  }
  lang.outEndArray(out);
  lang.outEndStmt(out);
  out << endl;
}

void OutputTokenizerSource(ostream &out, const Nfa &dfa, OutputLanguage language) {
  LanguageOutputter *outputer = getLanguageOutputter(language);
  OutputDfaSource(cout,dfa,*outputer);
  out << endl;
  delete outputer;
}
