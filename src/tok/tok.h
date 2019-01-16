#ifndef __tok_h
#define __tok_h

#include <istream>
using namespace std;

class TokBuf {
  char *m_buf;
  int m_buflen, m_buffill, m_bufpos, m_pos, m_line, m_col;
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
    return true;
  }

public:
  TokBuf(istream *in) {
    m_buf = 0;
    m_buffill = 0;
    m_buflen = 0;
    m_bufpos = 0;
    m_in = in;
    m_pos = 0;
    m_line = m_col = 1;
  }
  ~TokBuf() {
    if( m_buf )
      free(m_buf);
  }
  int peekc(int n) {
    while( n >= m_buffill ) {
      if( ! addc() )
        return -1;
    }
    return m_buf[(m_bufpos+n)%m_buflen];
  }
  void discard(int n) {
    for( int i = 0; i < n; ++i ) {
      if( m_buffill == 0 )
        if( ! addc() )
          return;
      int c = m_buf[m_bufpos];
      m_bufpos = (m_bufpos+1)%m_buflen;
      --m_buffill;
      if( c == '\n' ) {
        ++m_line;
        m_col = 1;
      } else
        ++m_col;
    }
  }
  int line() { return m_line; }
  int col()  { return m_col; }
};

struct TokenInfo {
  const int m_tokenCount;
  const int m_sectionCount;
  const int *m_tokenaction;
  const char **m_tokenstr;
  const bool *m_isws;
  const int *m_ranges;
  const int m_stateCount;
  const int *m_transitions;
  const int *m_transitionOffset;
  const int *m_tokens;
};

class Tokenizer {
  TokBuf *m_tokbuf;
  TokenInfo *m_tokinfo;
  int m_tok;
  int m_tokLen;
  char *m_s;
  int m_slen;
  vector<int> m_stack;

  int getTok(int state) {
    return m_tokinfo->m_tokens[state];
  }
  bool isWs(int tok) {
    return m_tokinfo->m_isws[tok];
  }
  int nextState(int state, int c) {
    int offset = m_tokinfo->m_transitionOffset[state];
    int count = m_tokinfo->m_transitionOffset[state+1]-offset;
    const int *firstTransition = m_tokinfo->m_transitions+(offset*2);
    for( int i = 0; i < count; ++i ) {
      int transition = firstTransition[i*2];
      int low = m_tokinfo->m_transitions[transition*2];
      int high = m_tokinfo->m_transitions[transition*2+1];
      if( c < low )
        continue;
      if( c > high )
        break;
      if( c >= low && c <= high )
        return firstTransition[i*2+1];
    }
    return -1;
  }
  void push(int tokset) {
    m_stack.push_back(tokset);
  }
  void pop() {
    m_stack.pop_back();
  }
  void gotots(int tokset) {
    m_stack.back() = tokset;
  }
public:
  Tokenizer(TokBuf *tokbuf, TokenInfo *tokinfo) : m_tokbuf(tokbuf), m_tokinfo(tokinfo) {
    m_tok = -1;
    m_tokLen = 0;
    m_s = 0;
    m_slen = 0;
    m_stack.push_back(0);
  }
  int peek() {
    if( m_tok != -1 )
      return m_tok;
    do
    {
      int curSection = m_stack.back();
      if( m_tok != -1 ) {
        m_tokbuf->discard(m_tokLen);
        m_tok = -1;
        m_tokLen = 0;
      }
      int state = nextState(0,curSection);;
      int tok = -1;
      int tokLen = -1;
      int len = 0;
      while( state != -1 ) {
        int t = getTok(state);
        if( t != -1 ) {
          tok = t;
          tokLen = len;
        }
        int c = m_tokbuf->peekc(len);
        state = nextState(state,c);
        ++len;
      }
      m_tok = tok;
      m_tokLen = tokLen;
      if( m_tok != -1 ) {
        const int *tokActions = m_tokinfo->m_tokenaction + (m_tok*m_tokinfo->m_sectionCount*2);
        int tokenaction = tokActions[curSection*2];
        int actionarg = m_tokinfo->m_tokenaction[curSection*2+1];
        if( tokenaction == 1 )
          push(actionarg);
        else if( tokenaction == 2 )
          pop();
        else if( tokenaction == 3 )
          gotots(actionarg);
      }
    } while( m_tok != -1 && isWs(m_tok) );
    return m_tok;
  }
  void discard() {
    if( m_tok == -1 )
      peek();
    if( m_tok != -1 ) {
      m_tokbuf->discard(m_tokLen);
      m_tok = -1;
      m_tokLen = 0;
    }
  }
  int length() {
    return m_tokLen;
  }
  char *tokstr() {
    if( m_tok == -1 )
      return 0;
    if( m_slen < m_tokLen+1 ) {
      m_s = (char*)realloc(m_s,m_tokLen+1);
      m_slen = m_tokLen+1;
    }
    *m_s = 0;
    if( m_tok != -1 ) {
      for( int i = 0, n = m_tokLen; i < n; ++i )
        m_s[i] = m_tokbuf->peekc(i);
      m_s[m_tokLen] = 0;
    }
    return m_s;
  }
};


#endif /* __tok_h */