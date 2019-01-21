#ifndef __parser_h
#define __parser_h

#include "../tok/tok.h"
#include "../tok/tinytemplates.h"

using namespace std;

class Production {
public:
  bool m_rejectable;
  int m_nt;
  Vector<int> m_symbols;
  String m_action;

  Production(bool rejectable, int nt, const Vector<int> &symbols, String action);
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

enum SymbolType {
  SymbolTypeUnknown,
  SymbolTypeTerminal,
  SymbolTypeNonterminal,
};

class SymbolDef {
public:
  int m_tokid;
  String m_name;
  SymbolType m_symboltype;
  String m_semantictype;
  SymbolDef() : m_tokid(-1), m_symboltype(SymbolTypeUnknown) {}
  SymbolDef(int tokid, const String &name, SymbolType symboltype) : m_tokid(tokid), m_name(name), m_symboltype(symboltype) {}
};

class ParserDef {
  int m_nextsymbolid;
  int m_startnt;
  Production *m_startProduction;
public:
  ParserDef() : m_nextsymbolid(0), m_startnt(-1), m_startProduction(0) {}
  Map<String,int> m_tokens;
  Map<int,SymbolDef> m_tokdefs;
  Vector<Production*> m_productions;
  Vector<PrecedenceRule*> m_precedencerules;
  Vector<DisallowRule*> m_disallowrules;
  void addProduction(Tokenizer &toks, Production *p);
  int findOrAddSymbolId(Tokenizer &toks, const String &s, SymbolType stype);
  int getStartNt() { return m_startnt; }
  Production *getStartProduction() { return m_startProduction; }
};

class ParserError {
public:
  int m_line, m_col;
  String m_err;
  ParserError(int line, int col, const String &err) : m_line(line), m_col(col), m_err(err) {}
};

void ParseParser(TokBuf *tokbuf, ParserDef &parser);
bool SolveParser(ParserDef &parser);
void OutputParser(ParserDef &parser);

#endif /* __parser_h */