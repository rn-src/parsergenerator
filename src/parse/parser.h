#ifndef __parser_h
#define __parser_h

#include <string>
#include <vector>
#include <map>
using namespace std;

class ParseStream {
public:
  ParseStream();
  int peek();
  void discard();
  const char *tokstr();
};

class Production {
public:
  int m_nt;
  vector<int> m_symbols;
  const char *m_action;

  Production(int nt, const vector<int> &symbols, const char *action);
};

class ParserDef {
  map<string,int> m_tokens;
public:
  vector<Production*> m_productions;
  int findOrAddSymbol(const char *s);
};

#endif /* __parser_h */

