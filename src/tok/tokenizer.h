#ifndef __tokenizer_h
#define __tokenizer_h

#include <stdio.h>
#include "tinytemplates.h"

class TokStream {
  char *m_buf;
  int m_buflen, m_buffill, m_bufpos, m_pos, m_line, m_col;
  FILE *m_in;

  bool addc();

public:
  TokStream(FILE *in);
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
  CharRange();
  CharRange(int low, int high);
  bool contains(int i) const;
  bool contains(const CharRange &rhs) const;
  bool overlaps(int low, int high) const;
};

class CharSet {
protected:
  Vector<CharRange> m_ranges;

  int find(int c, bool &found) const;
  int splitRec(int low, int high, int i);
public:
  typedef Vector<CharRange>::const_iterator iterator;

  bool contains(const CharRange &range) const;
  iterator begin() const;
  iterator end() const;
  void clear();
  bool empty() const;
  void addChar(int i);
  void addCharRange(int low, int high);
  void addCharset(const CharSet &rhs);
  void split(int low, int high);
  void split(const CharSet &rhs);
  void negate();
  bool operator!() const;
  int size() const { return m_ranges.size(); }
};

class Transition {
public:
  int m_from, m_to;
  Transition(int from, int to);
  bool operator<(const Transition &rhs) const;
};

enum TokenAction {
  ActionNone,
  ActionPush,
  ActionPop,
  ActionGoto
};

class Action {
public:
  TokenAction m_action;
  int m_actionarg;
};

class Token {
public:
  int m_token;
  bool m_isws;
  String m_name;
  Map<int,Action> m_actions;  
  Token() : m_token(-1), m_isws(false) {}
  bool operator<(const Token &rhs) const { return m_token < rhs.m_token; }
};

class VectorToken {
  Token *m_p;
  int m_size;
  int m_capacity;
public:
  typedef const Token* const_iterator;
  typedef Token* iterator;
  iterator begin() { return m_p; }
  iterator end() { return m_p+m_size; }
  const_iterator begin() const { return m_p; }
  const_iterator end() const { return m_p+m_size; }
  bool empty() const { return m_size == 0; }
  Token &operator[](size_t i) { return m_p[i]; }
  const Token &operator[](size_t i) const { return m_p[i]; }
  void clear() { m_size = 0; }
  void push_back(const Token &value) {
    m_p[m_size++] = value;
  }
  int size() const { return m_size; }
};

class Nfa {
protected:
  int m_nextState;
  Set<int> m_startStates;
  Set<int> m_endStates;
  Map<Transition,CharSet> m_transitions;
  Set<Transition> m_emptytransitions;
  Map<int,int> m_state2token;
  Map<int,Token> m_tokendefs;
  int m_sections;

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
  Vector<Token> getTokenDefs() const;
  void setTokenDef(const Token &tok);
  bool setTokenAction(int token, int section, TokenAction action, int actionarg);
  int getSections() const;
  void setSections(int sections);
  void clear();
  void closure(const Map<int,Set<int>> &emptytransitions, Set<int> &states) const;
  void follow( const CharRange &range, const Set<int> &states, Set<int> &nextstates ) const;
  void stateTransitions(const Set<int> &states, CharSet &transitions) const;
  bool hasEndState(const Set<int> &states) const;
  int lowToken(const Set<int> &states) const;
  void toDfa(Nfa &dfa) const;

  typedef Map<Transition,CharSet>::const_iterator const_iterator;
  const_iterator begin() const { return m_transitions.begin(); }
  const_iterator end() const { return m_transitions.end(); }
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
void OutputTokenizerSource(FILE *out, const Nfa &dfa, OutputLanguage language);

#endif /* __tokenizer_h */
