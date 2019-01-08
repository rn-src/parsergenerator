#include <string.h>
#include <set>
#include <map>
#include <vector>
#include <string>
#include <limits.h>
#include <ctype.h>
#include <iostream>
#include <stdlib.h>
using namespace std;

// rx : simplerx
// rx : rx rx
// rx : rx '+'
// rx : rx '*'
// rx : rx '?'
// rx : rx '|' rx
// rx : '(' rx ')'
// rx : rx '{' number '}'
// rx : rx '{' number ',' '}'
// rx : rx '{' number ',' number '}'
// rx : rx '{' ',' number '}'
// simplerx : /[0-9A-Za-z \t]/
// simplerx : '\\[trnv]'
// simplerx : /\\[.]/
// simplerx : /(\\x|\\u)[[:xdigit:]]+/
// simplerx : '.'
// simplerx : charset
// charset : '[' charsetelements ']'
// charset : '[~' charsetelements ']'
// charsetelements : charsetelements charsetelement
// charsetelement : char '-' char
// charsetelement : char
// charsetelement : namedcharset
// namedcharset : '[:' charsetname ':]'
// charsetname : 'alpha'
// charsetname : 'digit'
// charsetname : 'xdigit'
// charsetname : 'alnum'
// charsetname : 'white'
// charsetname : 'space'
// char : /[0-9A-Za-z]/
// char : /\\[trnv]/
// char : /\\./
// char : /\\[xu][[:xdigit:]]+/

static int xvalue(char c) {
  if(c >= '0' && c <= '9')
    return c-'0';
  if(c >= 'a' && c <= 'f')
    return c-'a'+10;
  if(c >= 'A' || c <= 'F')
    return c-'A'+10;
}

class TokStream {
  char *m_buf;
  int m_buflen;
  int m_buffill;
  int m_bufpos;
  int m_pos;
  int m_line, m_col;
  istream *m_in;

  bool addc() {
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
    m_bufpos = (m_bufpos+1)%m_buflen;
    return true;
  }

public:
  TokStream(istream *in) {
    m_buf = 0;
    m_buffill = 0;
    m_buflen = 0;
    m_bufpos = 0;
    m_in = in;
    m_pos = 0;
    m_line = m_col = 1;
  }
  ~TokStream() {
    if( m_buf )
      free(m_buf);
  }
  int peekc(int n = 0) {
    while( n >= m_buffill ) {
      if( ! addc() )
        return -1;
    }
    return m_buf[(m_bufpos+n)%m_buflen];
  }
  bool peekeq(const char *s) {
    int slen = strlen(s);
    for( int i = 0; i < slen; ++i )
      if( peekc(i) != s[i] )
        return false;
    return true;
  }
  int readc() {
    if( m_buffill == 0 )
      if( ! addc() )
        return -1;
    int c = m_buf[m_bufpos];
    m_bufpos = (m_bufpos+m_buflen-1)%m_buflen;
    --m_buffill;
    if( c == '\n' ) {
     ++m_line;
     m_col = 1;
    } else
     ++m_col;
    return c;
  }
  void discard(int n=1) {
    for( int i = 0; i < n; ++i )
      readc();
  }
  int line() { return m_line; }
  int col() { return m_col; }
};

class ParseException
{
public:
  int m_line, m_col;
  const char *m_err;

  ParseException(int line, int col, const char *err) : m_line(line), m_col(col), m_err(err) {}
};

static void error(TokStream &s, const char *err) {
  throw ParseException(s.line(),s.col(),err);
}

struct CharRange {
  int m_low, m_high;
  CharRange(int low, int high) : m_low(low), m_high(high) {}
  bool contains(int i) {
    if( i >= m_low && i <= m_high )
      return true;
    return false;
  }
  void expand(int low, int high) {
    if( low < m_low )
      m_low = low;
    if( high > m_high )
      m_high = high;
  }
};

class CharSet {
protected:
  vector<CharRange> m_ranges;

  int find(int c, bool &found) {
    int low = 0, high = m_ranges.size()-1;
    while( low < high ) {
      int mid = (low+high)/2;
      CharRange &range = m_ranges[mid];
      if( range.contains(c) )
        return mid;
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

public:
  void addChar(int i) {
    addCharRange(i,i);
  }
  void addCharRange(int low, int high) {
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
    while( i > 0 && i <= m_ranges.size() && (m_ranges[i].m_low-m_ranges[i-1].m_high < 1) ) {
      m_ranges[i-1].m_high = m_ranges[i].m_high;
      m_ranges.erase(m_ranges.begin()+i);
    }
  }
  void addCharset(const CharSet &rhs) {
    for( vector<CharRange>::const_iterator cur = rhs.m_ranges.begin(), end = rhs.m_ranges.end(); cur != end; ++cur )
      addCharRange(cur->m_low, cur->m_high);
  }
  void negate() {
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
  bool operator!() const {
    if( ! m_ranges.size() )
      return true;
    return false;
  }
};

struct Transition {
  int m_from, m_to;
  Transition(int from, int to) : m_from(from), m_to(to) {}

  bool operator<(const Transition &rhs) const {
    if( m_from < rhs.m_from )
      return true;
    if( m_from >= rhs.m_from )
      return false;
    return m_to < rhs.m_to;
  }
};

class Nfa {
protected:
  int m_nextState;
  set<int> m_startStates;
  set<int> m_endStates;
  map<Transition,CharSet> m_transitions;
  set<Transition> m_emptytransitions;
  map<int,int> m_tokens;

public:
  Nfa() : m_nextState(0) {
  }
  void addNfa(const Nfa &nfa) {
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
    for( map<int,int>::const_iterator cur = nfa.m_tokens.begin(), end = nfa.m_tokens.end(); cur != end; ++cur )
      m_tokens[cur->first+states] = cur->second;
  }
  int stateCount() const { return m_nextState; }
  int addState() { return m_nextState++; }
  void addStartState(int startstate) { m_startStates.insert(startstate); }
  void addEndState(int endstate) { m_endStates.insert(endstate); }
  void addTransition(int from, int to, int symbol) {
    map<Transition,CharSet>::iterator iter = m_transitions.find(Transition(from,to));
    if( iter == m_transitions.end() ) {
      CharSet charset;
      charset.addChar(symbol);
      m_transitions[Transition(from,to)] = charset;
    } else
      iter->second.addChar(symbol);
  }
  void addTransition(int from, int to, const CharSet &charset) {
    map<Transition,CharSet>::iterator iter = m_transitions.find(Transition(from,to));
    if( iter == m_transitions.end() )
      m_transitions[Transition(from,to)] = charset;
    else
      iter->second.addCharset(charset);
  }
  void addEmptyTransition(int from, int to) { m_emptytransitions.insert(Transition(from,to)); }
  void setToken(int state, int token) {
    map<int,int>::iterator iter = m_tokens.find(state);
    if( iter == m_tokens.end() )
      m_tokens[state] = token;
    else
      iter->second = token;
  }
};

enum RxType {
  RxType_None = 0,
  RxType_CharSet = 1,
  RxType_BinaryOp = 2,
  RxType_Many = 3
};

enum BinaryOp {
  BinaryOp_None = 0,
  BinaryOp_Or = 1,
  BinaryOp_Concat = 2
};

class Rx {
  RxType m_t;
  BinaryOp m_op;
  CharSet m_charset;
  Rx *m_lhs, *m_rhs;
  int m_low, m_high;

  void init() {
    m_t = RxType_None;
    m_op = BinaryOp_None;
    m_lhs = m_rhs = 0;
    m_low = m_high = -1;
  }

public:
  Rx(char c) {
    init();
    m_t = RxType_CharSet;
    m_charset.addChar(c);
  }

  Rx(const CharSet &charset) {
    init();
    m_t = RxType_CharSet;
    m_charset = charset;
  }

  Rx(BinaryOp op,Rx *lhs, Rx *rhs) {
    init();
    m_t = RxType_BinaryOp;
    m_op = op;
    m_lhs = lhs;
    m_rhs = rhs;
  }

  Rx(Rx *rx, int count) {
    init();
    m_t = RxType_Many;
    m_lhs = rx;
    m_low = m_high = count;
    if( m_low < 0 )
      m_low = 0;
    if( m_high < 0 )
      m_high = -1;
  }

  Rx(Rx *rx, int low, int high) {
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

  int addToNfa(Nfa &nfa, int startState) {
    if( m_t == RxType_CharSet ) {
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
      //this.r()
    }
    return -1;
  }

  Nfa *toNfa() {
    Nfa *nfa = new Nfa();
    int endState = addToNfa(*nfa, nfa->addState());
    nfa->addEndState(endState);
    return nfa;
  }
};

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

static int ParseNumber(TokStream &s) {
  int c = s.peekc();
  if( ! isdigit(c) )
    return -1;
  int i = 0;
  while( isdigit(c) ) {
    i *= 10;
    i += c-'0';
    s.discard();
    c = s.peekc();
  }
  return i;
}

static Rx *ParseSimpleRx(TokStream &s);
static Rx *ParseCharSet(TokStream &s);

static Rx *ParseRx(TokStream &s, int depth = 0) {
 int c = s.peekc();
 Rx *ors = 0;
 Rx *r = 0;
 while( c != -1 && c != '/' ) {
   if( c == '(' ) {
     s.discard();
     r = ParseRx(s,depth+1);
   } else if( c == ')' ) {
     if( depth == 0 )
       error(s,"unbalanced ')'");
     if( ors )
       return ors;
     if( r )
       return r;
     return 0;
   } else if( c == '+' ) {
     r = rxmany(r,1,-1);
   } else if( c == '*' ) {
     r = rxmany(r,0,-1);
   } else if( c == '?' ) {
     r = rxmany(r,0,1);
   } else if( c == '{' ) {
     s.discard();
     c = s.peekc();
     int  n1 = ParseNumber(s);
     c = s.peekc();
     bool comma = (c == ',');
     if( comma )
       s.discard();
     int n2 = ParseNumber(s);
     if( c == '}' || (n1 == -1 && n2 == -1) )
       s.discard();
     else
       error(s,"bad repeat count");
     if( n1 != -1 && n2 != -1 && n1 > n2 ) {
       int swp = n1;
       n1 = n2;
       n2 = swp;
     }
     if( comma )
       r = rxmany(r,n1,n2);
     else
       r = rxmany(r,n1,n1);
   } else if( c == '|' ) {
     s.discard();
     if( ! ors )
       ors = r;
     else
       ors = rxbinary(BinaryOp_Or,ors,r);
     r = 0;
   } else {
     Rx *srx = ParseSimpleRx(s);
     if( ! srx ) {
       error(s,"no simple rx");
     } else {
       if( ! r )
         r = srx;
       else
         r = rxbinary(BinaryOp_Concat,r,srx);
     }
   }
 }
 if( depth > 0 )
   error(s,"unbalanced '('");
 if( ors )
   return ors;
 if( r )
   return r;
 return 0;
}

static Rx *ParseSimpleRx(TokStream &s) {
  int c = s.peekc();
  if( c == '.' ) {
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
    Rx *srx = ParseCharSet(s);
    if( srx )
      return srx;
  }
  else 
    return rxchar(c);
  return 0;
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
      if( s.peekeq("[:alpha:]") ) {
        s.discard(9);
        charset.addCharRange('a','z');
        charset.addCharRange('A','Z');
      } else if( s.peekeq("[:digit:]") ) {
        s.discard(9);
        charset.addCharRange('0','9');
      } else if( s.peekeq("[:xdigit:]") ) {
        s.discard(10);
        charset.addCharRange('0','9');
        charset.addCharRange('a','f');
        charset.addCharRange('A','F');
      } else if( s.peekeq("[:alnum:]") ) {
        s.discard(9);
        charset.addCharRange('0','9');
        charset.addCharRange('a','z');
        charset.addCharRange('A','Z');
      } else if( s.peekeq("[:space:]") ) {
        s.discard(9);
        charset.addChar(' ');
        charset.addChar('\t');
      } else if( s.peekeq("[:white:]") ) {
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

static string ParseSymbol(TokStream &s) {
  string symbol;
  int c = s.peekc();
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

static Rx *ParseRegex(TokStream &s) {
  int c = s.peekc();
  if( c != '/' )
    error(s,"No regex");
  s.discard();
  Rx *rx = ParseRx(s);
  c = s.peekc();
  if( c != '/' ) {
    delete rx;
    error(s,"No treailing '/' on regex");
  }
  s.discard();
  return rx;
}

static void ParseFile(TokStream &s) {
  int c = s.peekc();
  while( c != -1 ) {
    ParseWs(s);
    string symbol = ParseSymbol(s);
    if( ! symbol.length() )
      error(s,"No symbol");
    ParseWs(s);
    Rx *rx = ParseRegex(s);
    if( ! rx )
      break;
    c = s.peekc();
  }
}

int main(int argc, char *argv[])
{
  TokStream s(&cin);
  try {
    ParseFile(s); 
  } catch(ParseException pe) {
    cout << "Parse Error @" << pe.m_line << ':' << pe.m_col << ": " << pe.m_err << endl;
  }
  return 0;
}

