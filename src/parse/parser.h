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

class ProductionState {
public:
  Production *m_p;
  int m_idx;
  ProductionState(Production *p, int idx) : m_p(p), m_idx(idx) {}
  ProductionState(const ProductionState &rhs) : m_p(rhs.m_p), m_idx(rhs.m_idx) {}
  bool operator<(const ProductionState &rhs) const {
    if( m_p->m_nt < rhs.m_p->m_nt )
      return true;
    else if( rhs.m_p->m_nt < m_p->m_nt )
      return false;
    if( m_idx < rhs.m_idx )
      return true;
    return false;
  }
  int symbol() const {
    if( m_idx >= m_p->m_symbols.size() )
      return -1;
    return m_p->m_symbols[m_idx];
  }
};

class ProductionDescriptor {
public:
  int m_nt;
  Vector<int> m_symbols;
  ProductionDescriptor(int nt, Vector<int> symbols);
  bool matchesProduction(Production *p) const;
  bool matchesProductionState(const ProductionState &rhs) const;
};

typedef Vector<ProductionDescriptor*> ProductionSetDescriptor;
typedef Vector<ProductionSetDescriptor*> PrecedenceRule;

class DisallowProductionSetDescriptor {
public:
  ProductionSetDescriptor *m_descriptor;
  bool m_star;
};

class DisallowRule {
public:
  Vector<DisallowProductionSetDescriptor*> m_parts;
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
  Vector<DisallowRule*> m_disallowrules;
  void addProduction(Tokenizer &toks, Production *p);
  void addLeftAssociativeForbids(ProductionSetDescriptor *p);
  void addRightAssociativeForbids(ProductionSetDescriptor *p);
  void addPrecedenceRuleForbids(const PrecedenceRule *rule);
  int findOrAddSymbolId(Tokenizer &toks, const String &s, SymbolType stype);
  int getStartNt() { return m_startnt; }
  Production *getStartProduction() const { return m_startProduction; }
  Vector<Production*> productionsAt(const ProductionState &ps) const;
};

class ParserError {
public:
  int m_line, m_col;
  String m_err;
  ParserError(int line, int col, const String &err) : m_line(line), m_col(col), m_err(err) {}
};

typedef Set<ProductionState> state_t;

class ParserSolution {
public:
  Vector<state_t> m_states;
  // statesrc,statedst -> symbols
  Map< Pair<int,int>,Set<int> > m_shifts;
  // statesrc,ruleno -> symbols
  Map< Pair<int,int>,Set<int> > m_reductions;
};

enum OutputLanguage {
  LanguageJs,
  LanguageC
};

void ParseParser(TokBuf *tokbuf, ParserDef &parser);
bool NormalizeParser(const ParserDef &parserIn, ParserDef &parserOut);
bool SolveParser(const ParserDef &parser, ParserSolution &solution);
void OutputParserSolution(FILE *out, const ParserDef &parser, const ParserSolution &solution, OutputLanguage language);

#endif /* __parser_h */