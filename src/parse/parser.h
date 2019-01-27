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
  int m_symbolid;

  Production(int symbolid, bool rejectable, int nt, const Vector<int> &symbols, String action);
};

enum Associativity {
  AssocNull,
  AssocLeft,
  AssocRight,
  AssocNon,
  AssocByDisallow
};

class ProductionDescriptor;

class PrecedencePart {
public:
  int m_localprecedence;
  ProductionDescriptor *m_descriptor;
  Associativity m_assoc;
  ProductionDescriptor *m_disallowed;
};

class ProductionStateItem {
public:
  Production *m_p;
  int m_idx;
  ProductionStateItem(Production *p, int idx) : m_p(p), m_idx(idx) {}
  ProductionStateItem(const ProductionStateItem &rhs) : m_p(rhs.m_p), m_idx(rhs.m_idx) {}
  bool operator<(const ProductionStateItem &rhs) const {
    if( m_p->m_nt < rhs.m_p->m_nt )
      return true;
    else if( rhs.m_p->m_nt < m_p->m_nt )
      return false;
    if( m_idx < rhs.m_idx )
      return true;
    return false;
  }
};

class ProductionState {
public:
  Vector<ProductionStateItem> m_items;
  bool operator<(const ProductionState &rhs) const {
    return m_items < rhs.m_items;
  }
  ProductionState() {}
  ProductionState(const ProductionState &rhs) : m_items(rhs.m_items) {}
  ProductionState(Production *p, int idx) {
    m_items.push_back(ProductionStateItem(p,idx));
  }
};

class ProductionDescriptor {
public:
  int m_nt;
  Vector<int> m_symbols;
  ProductionDescriptor(int nt, Vector<int> symbols);
  bool matchesProduction(Production *p) const;
  bool matchesProductionStateItem(const ProductionStateItem &rhs) const;
};

class PrecedenceRule {
public:
  Vector<PrecedencePart*> m_parts;
  bool isRejectedPlacement(const ProductionState &ps, Production *p) const;
};

class DisallowRule {
public:
  ProductionDescriptor *m_disallowed;
  Vector<ProductionDescriptor*> m_ats;
  ProductionDescriptor *m_finalat;

  bool isRejectedPlacement(const ProductionState &ps, Production *p) const;
};

enum SymbolType {
  SymbolTypeUnknown,
  SymbolTypeTerminal,
  SymbolTypeNonterminal,
  SymbolTypeProduction
};

class SymbolDef {
public:
  int m_tokid;
  String m_name;
  SymbolType m_symboltype;
  String m_semantictype;
  Production *m_p;
  SymbolDef() : m_tokid(-1), m_symboltype(SymbolTypeUnknown), m_p(0) {}
  SymbolDef(int tokid, const String &name, SymbolType symboltype) : m_tokid(tokid), m_name(name), m_symboltype(symboltype), m_p(0) {}
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
  Production *getStartProduction() const { return m_startProduction; }
  Vector<Production*> productionsAt(const ProductionState &ps) const;
  bool isRejectedPlacement(const ProductionState &ps, Production *p) const;
  bool isPartialReject(const ProductionState &ps) const;
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