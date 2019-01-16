#ifndef __parser_h
#define __parser_h

#include <string>
#include <vector>
#include <map>
#include "../tok/tok.h"

using namespace std;

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

void ParseParser(TokBuf *tokbuf, ParserDef &parser);

#endif /* __parser_h */