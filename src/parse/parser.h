#ifndef __parser_h
#define __parser_h

#include <string>
#include <vector>
#include <map>
#include "../tok/tok.h"
#include "../tok/tinytemplates.h"

using namespace std;

class Production {
public:
  bool m_rejectable;
  int m_nt;
  vector<int> m_symbols;
  String m_action;

  Production(bool rejectable, int nt, const vector<int> &symbols, String action);
};

enum Associativity {
  AssocNull,
  AssocLeft,
  AssocRight,
  AssocNon,
  AssocByDisallow
};

class ProductionDescriptor {
public:
  int m_nt;
  Vector<int> m_symbols;
  ProductionDescriptor(int nt, Vector<int> symbols);
};

class PrecedencePart {
public:
  int m_localprecedence;
  ProductionDescriptor *m_descriptor;
  Associativity m_assoc;
  ProductionDescriptor *m_disallowed;
};

class PrecedenceRule {
public:
  Vector<PrecedencePart*> m_parts;
};

class DisallowRule {
public:
  ProductionDescriptor *m_disallowed;
  Vector<ProductionDescriptor*> m_ats;
  ProductionDescriptor *m_finalat;
};

class ParserDef {
  int m_nextsymbolid;
  map<string,int> m_tokens;
public:
  void setNextSymbolId(int nextsymbolid);
  vector<Production*> m_productions;
  vector<PrecedenceRule*> m_precedencerules;
  vector<DisallowRule*> m_disallowrules;
  int findOrAddSymbol(const char *s);
};

class ParserError {
public:
  int m_line, m_col;
  String m_err;
  ParserError(int line, int col, const char *err) : m_line(line), m_col(col), m_err(err) {}
};

void ParseParser(TokBuf *tokbuf, ParserDef &parser);
void OutputParser(ParserDef &parser);

#endif /* __parser_h */