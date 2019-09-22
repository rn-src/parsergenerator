#ifndef __tok_h
#define __tok_h

#include <stdio.h>
#include <string.h>
#include "tinytemplates.h"

struct ParsePos {
  String filename;
  int line, col;
};

class TokBuf {
public:
  virtual ~TokBuf() {}
  virtual int peekc(int n) = 0;
  virtual void discard(int n) = 0;
  virtual int line() = 0;
  virtual int col()  = 0;
  virtual String filename() = 0;
  virtual void getParsePos(Vector<ParsePos> &fNames) = 0;
};

struct StackTokBufItem {
  TokBuf *tokbuf;
  int startline;
  int lineno;
  String filename;
};

class Token {
public:
  String m_s;
  String m_filename;
  int m_wslines, m_wscols, m_wslen;
  int m_line, m_col;
  int m_sym;

  Token() : m_wslines(0), m_wscols(0), m_wslen(0), m_line(-1), m_col(-1), m_sym(-1) {}

  void set(String s, String filename, int wslines, int wscols, int wslen, int line, int col, int sym) {
    m_s = s;
    m_filename = filename;
    m_wslines = wslines;
    m_wscols = wscols;
    m_wslen = wslen;
    m_line = line;
    m_col = col;
    m_sym = sym;
  }
  const char *c_str() const { return m_s.c_str(); }
};

class StackTokBuf : public TokBuf {
  Vector<StackTokBufItem> m_stack;
  Token m_endtok;
protected:
  virtual void startBuf(TokBuf *tokBuf) {}
  virtual void endBuf(TokBuf *tokBuf) {}
public:
  StackTokBuf() {}
  void push(TokBuf *tokbuf) {
    StackTokBufItem item;
    item.tokbuf = tokbuf;
    item.startline = -1;
    item.lineno = -1;
    m_stack.push_back(item);
    startBuf(tokbuf);
    // if empty file, pop immediately
    if( tokbuf->peekc(0) == -1 )
      discard(0);
  }
  virtual int peekc(int n) {
    if( m_stack.size() ) {
      StackTokBufItem &item = m_stack.back();
      return item.tokbuf->peekc(n);
    }
    return -1;
  }
  virtual void discard(int n) {
    if( m_stack.size() ) {
      StackTokBufItem &item = m_stack.back();
      item.tokbuf->discard(n);
      if( item.tokbuf->peekc(0) == -1 ) {
        TokBuf *tokbuf = m_stack.back().tokbuf;
        m_stack.pop_back();
        endBuf(tokbuf);
        if( m_stack.size() == 0 ) {
          m_endtok.m_filename = tokbuf->filename();
          m_endtok.m_line = tokbuf->line();
          m_endtok.m_col = tokbuf->col();
        }
        delete tokbuf;
      }
    }
  }
  virtual int line() {
    if( m_stack.size() ) {
      StackTokBufItem &item = m_stack.back();
      if( m_stack.back().lineno > 0 )
        return item.lineno + (item.tokbuf->line()-item.startline);
      return item.tokbuf->line();
    }
    return m_endtok.m_line;
  }
  virtual int col() {
    if( m_stack.size() ) {
      StackTokBufItem &item = m_stack.back();
      return item.tokbuf->col();
    }
    return m_endtok.m_col;
  }
  virtual String filename() {
    if( m_stack.size() ) {
      StackTokBufItem &item = m_stack.back();
      if( item.filename.length() )
        return item.filename;
      return item.tokbuf->filename();
    }
    return m_endtok.m_filename;
  }
  void setline(int lineno, String filename) {
    if( m_stack.size() ) {
      StackTokBufItem &item = m_stack.back();
      item.startline = item.tokbuf->line();
      item.lineno = lineno;
      item.filename = filename;
    }
  }
  virtual void getParsePos(Vector<ParsePos> &fPos) {
    for( int i = m_stack.size()-1; i >= 0; --i ) {
      m_stack[i].tokbuf->getParsePos(fPos);
    }
  }
};

class FileTokBuf : public TokBuf {
  char *m_buf;
  int m_buflen, m_buffill, m_bufpos, m_pos, m_line, m_col;
  FILE *m_in;
  String m_filename;

  bool addc() {
    if( feof(m_in) )
      return false;
    char c;
    if( fread(&c,1,1,m_in) != 1 )
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
  FileTokBuf(FILE *in, String fname) {
    m_buf = 0;
    m_buffill = 0;
    m_buflen = 0;
    m_bufpos = 0;
    m_in = in;
    m_pos = 0;
    m_line = m_col = 1;
    m_filename = fname;
  }
  ~FileTokBuf() {
    if( m_buf )
      free(m_buf);
  }
  virtual int peekc(int n) {
    while( n >= m_buffill ) {
      if( ! addc() )
        return -1;
    }
    return m_buf[(m_bufpos+n)%m_buflen];
  }
  virtual void discard(int n) {
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
  virtual int line() { return m_line; }
  virtual int col()  { return m_col; }
  virtual String filename() { return m_filename; }
  virtual void getParsePos(Vector<ParsePos> &fNames) {
    ParsePos pos;
    pos.filename = m_filename;
    pos.line = m_line;
    pos.col = m_col;
    fNames.push_back(pos);
  }
};

struct TokenInfo {
  const int m_tokenCount;
  const int m_sectionCount;
  const int *m_tokenaction;
  const char **m_tokenstr;
  const bool *m_isws;
  const int m_stateCount;
  const int *m_transitions;
  const int *m_transitionOffset;
  const int *m_tokens;
};

class Tokenizer {
public:
  virtual ~Tokenizer() {};
  virtual int peek() = 0;
  virtual void discard() = 0;
  virtual int length() = 0;
  virtual const char *tokstr() = 0;
  virtual int wslines() = 0;
  virtual int wscols() = 0;
  virtual int wslen() = 0;
  virtual int line() = 0;
  virtual int col() = 0;
  virtual String filename() = 0;
  virtual const char *tokstr(int tok) = 0;
  virtual void getParsePos(Vector<ParsePos> &parsepos) = 0;
};

class TokBufTokenizer : public Tokenizer {
  TokBuf *m_tokbuf;
  TokenInfo *m_tokinfo;
  int m_tok;
  int m_tokLen;
  char *m_s;
  int m_slen;
  int m_wslines, m_wscols, m_wslen;
  Vector<int> m_stack;

  int getTok(int state) {
    return m_tokinfo->m_tokens[state];
  }
  bool isWs(int tok) {
    return m_tokinfo->m_isws[tok];
  }
  int nextState(int state, int c) {
    const int *curTransition = m_tokinfo->m_transitions+m_tokinfo->m_transitionOffset[state];
    const int *endTransition = m_tokinfo->m_transitions+m_tokinfo->m_transitionOffset[state+1];
    while( curTransition != endTransition ) {
      int to = *curTransition++;
      int pairCount = *curTransition++;
      for( int i = 0; i < pairCount; ++i ) {
        int low = *curTransition++;
        int high = *curTransition++;
        if( c >= low && c <= high )
          return to;
      }
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
  TokBufTokenizer(TokBuf *tokbuf, TokenInfo *tokinfo) : m_tokbuf(tokbuf), m_tokinfo(tokinfo) {
    m_tok = -1;
    m_tokLen = 0;
    m_s = 0;
    m_slen = 0;
    m_wslines = 0;
    m_wscols = 0;
    m_wslen = 0;
    m_stack.push_back(0);
  }
  virtual int peek() {
    if( m_tok != -1 )
      return m_tok;
    do
    {
      int curSection = m_stack.back();
      if( m_tok != -1 ) {
        if( isWs(m_tok) ) {
          int tokstartline = m_tokbuf->line();
          int tokstartcol = m_tokbuf->col();
          m_tokbuf->discard(m_tokLen);
          int tokendline = m_tokbuf->line();
          int tokendcol = m_tokbuf->col();
          if( tokendline > tokstartline ) {
            m_wslines += (tokendline - tokstartline);
            m_wscols = tokendcol-1;
          } else {
            m_wscols += (tokendcol - tokstartcol);
          }
          m_wslen += m_tokLen;
        } else {
          m_tokbuf->discard(m_tokLen);
          m_wslines = 0;
          m_wscols = 0;
          m_wslen = 0;
        }
        m_tok = -1;
        m_tokLen = 0;
      }
      int state = nextState(0,curSection);
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
        const int *tokActions = m_tokinfo->m_tokenaction + (m_tok*m_tokinfo->m_sectionCount*2) + curSection*2;
        int tokenaction = tokActions[0];
        int actionarg = tokActions[1];
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
      m_wslines = 0;
      m_wscols = 0;
      m_wslen = 0;
    }
  }
  int length() {
    return m_tokLen;
  }
  const char *tokstr() {
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
  int wslines() {
    return m_wslines;
  }
  int wscols() {
    return m_wscols;
  }
  int wslen() {
    return m_wslen;
  }  
  int line() {
    return m_tokbuf->line();
  }
  int col() {
    return m_tokbuf->col();
  }
  String filename() {
    return m_tokbuf->filename();
  }
  const char *tokstr(int tok) {
    if( tok < 0 || tok >= m_tokinfo->m_tokenCount )
      return  "?";
    return m_tokinfo->m_tokenstr[tok];
  }
  void getParsePos(Vector<ParsePos> &posstack) {
    m_tokbuf->getParsePos(posstack);
  }
};

#endif /* __tok_h */
