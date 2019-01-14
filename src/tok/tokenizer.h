#ifndef __tokenizer_h
#define __tokenizer_h

#include <istream>
#include <vector>
#include <set>
#include <map>
using namespace std;

class TokStream {
  char *m_buf;
  int m_buflen, m_buffill, m_bufpos, m_pos, m_line, m_col;
  istream *m_in;

  bool addc();

public:
  TokStream(istream *in);
  ~TokStream();
  int peekc(int n = 0);
  bool peekstr(const char *s);
  int readc();
  void discard(int n=1);
  int line();
  int col();
};

class ParseException
{
public:
  int m_line, m_col;
  const char *m_err;

  ParseException(int line, int col, const char *err) : m_line(line), m_col(col), m_err(err) {}
};

class CharRange {
public:
  int m_low, m_high;
  CharRange(int low, int high);
  bool contains(int i) const;
  bool contains(const CharRange &rhs) const;
  bool overlaps(int low, int high) const;
};

class CharIterator {
  vector<CharRange>::const_iterator m_cur, m_end;
  int m_curchar;
public:
  CharIterator(vector<CharRange>::const_iterator cur, vector<CharRange>::const_iterator end, int curchar);
  int operator*() const;
  CharIterator operator++();
  CharIterator operator++(int);
  bool operator==(const CharIterator &rhs) const;
  bool operator!=(const CharIterator &rhs) const;
};

class CharSet {
protected:
  vector<CharRange> m_ranges;

  int find(int c, bool &found) const;
  int splitRec(int low, int high, int i);
public:
  typedef typename CharIterator char_iterator;
  typedef typename vector<CharRange>::const_iterator iterator;

  bool contains(const CharRange &range) const;
  iterator begin() const;
  iterator end() const;
  char_iterator begin_char() const;
  char_iterator end_char() const;
  void clear();
  bool empty() const;
  void addChar(int i);
  void addCharRange(int low, int high);
  void addCharset(const CharSet &rhs);
  void split(int low, int high);
  void split(const CharSet &rhs);
  void negate();
  bool operator!() const;
};

class Transition {
public:
  int m_from, m_to;
  Transition(int from, int to);
  bool operator<(const Transition &rhs) const;
};

class Token {
public:
  int m_token;
  bool m_isws;
  string m_name;
  Token() : m_token(-1), m_isws(false) {}
  Token(int token, bool isws, const string name) : m_token(token), m_isws(isws), m_name(name) {}
  bool operator<(const Token &rhs) const { return m_token < rhs.m_token; }
};

class Nfa {
protected:
  int m_nextState;
  set<int> m_startStates;
  set<int> m_endStates;
  map<Transition,CharSet> m_transitions;
  set<Transition> m_emptytransitions;
  map<int,int> m_token2state;
  map<int,Token> m_tokendefs;

public:
  Nfa();
  void addNfa(const Nfa &nfa);
  int stateCount() const;
  int addState();
  void addStartState(int startstate);
  void addEndState(int endstate);
  void addTransition(int from, int to, int symbol);
  void addTransition(int from, int to, const CharRange &range);
  void addTransition(int from, int to, const CharSet &charset);
  void addEmptyTransition(int from, int to);
  bool stateHasToken(int state) const;
  int getStateToken(int state) const;
  void setStateToken(int state, int token);
  bool hasTokenDef(int token) const;
  Token getTokenDef(int token) const;
  vector<Token> getTokenDefs() const;
  void setTokenDef(int token, bool isws, const string name);
  void clear();
  void closure(const map<int,set<int> > &emptytransitions, set<int> &states) const;
  void follow( const CharRange &range, const set<int> &states, set<int> &nextstates ) const;
  void stateTransitions(const set<int> &states, CharSet &transitions) const;
  bool hasEndState(const set<int> &states) const;
  int lowToken(const set<int> &states) const;
  void toDfa(Nfa &dfa) const;
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

  void init();

public:
  Rx();
  Rx(char c);
  Rx(const CharSet &charset);
  Rx(BinaryOp op,Rx *lhs, Rx *rhs);
  Rx(Rx *rx, int count);
  Rx(Rx *rx, int low, int high);
  Rx *clone();
  int addToNfa(Nfa &nfa, int startState);
  Nfa *toNfa();
};

enum OutputLanguage {
  LanguageJs,
  LanguageC
};

Nfa *ParseTokenizerFile(TokStream &s);
void OutputTokenizerSource(ostream &out, const Nfa &dfa, OutputLanguage language);

#endif /* __tokenizer_h */