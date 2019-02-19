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
  Production *clone();
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
  ProductionDescriptor() {}
  ProductionDescriptor(int nt, Vector<int> symbols);
  bool matchesProduction(const Production *p, bool useBaseTokId) const;
  // Check if, ignoring dots, the productions are the same
  bool hasSameProductionAs(const ProductionDescriptor &rhs) const;
  // Add dots from rhs to this descriptor.  Fail if not same production.
  bool addDotsFrom(const ProductionDescriptor &rhs);
  bool operator<(const ProductionDescriptor &rhs) const;
  ProductionDescriptor *clone() const;
  bool operator==(const ProductionDescriptor &rhs) const {
    return m_nt == rhs.m_nt && m_symbols == rhs.m_symbols;
  }
  bool operator!=(const ProductionDescriptor &rhs) const {
    return ! operator==(rhs);
  }
};

class ProductionDescriptors {
public:
  Vector<ProductionDescriptor*> m_descriptors;
  typedef Vector<ProductionDescriptor*>::iterator iterator;
  typedef Vector<ProductionDescriptor*>::const_iterator const_iterator;
  iterator begin() { return m_descriptors.begin(); }
  const_iterator begin() const { return m_descriptors.begin(); }
  iterator end() { return m_descriptors.end(); }
  const_iterator end() const { return m_descriptors.end(); }
  void push_back(ProductionDescriptor *p) { m_descriptors.push_back(p); }
  void consolidateRules();
  ProductionDescriptors *clone() const;
  bool operator==(const ProductionDescriptors &rhs) const {
    if( m_descriptors.size() != rhs.m_descriptors.size() )
      return false;
    for( int i = 0, n = m_descriptors.size(); i < n; ++i )
      if( *m_descriptors[i] != *rhs.m_descriptors[i] )
        return false;
    return true;
  }
  bool operator!=(const ProductionDescriptors &rhs) const {
    return ! operator==(rhs);
  }
  bool matchesProduction(const Production *lhs, bool useBaseTokId) const {
    for( const_iterator c = begin(), e = end(); c != e; ++c )
     if( (*c)->matchesProduction(lhs,useBaseTokId) )
       return true;
    return false;
  }
};

typedef Vector<ProductionDescriptors*> PrecedenceRule;

class DisallowProductionDescriptors {
public:
  ProductionDescriptors *m_descriptors;
  bool m_star;
  bool operator==(const DisallowProductionDescriptors &rhs) const {
    return m_star == rhs.m_star && *m_descriptors == *rhs.m_descriptors;
  }
  bool operator!=(const DisallowProductionDescriptors &rhs) const {
    return ! operator==(rhs);
  }
};

class DisallowRule {
public:
  ProductionDescriptors *m_lead;
  Vector<DisallowProductionDescriptors*> m_intermediates;
  ProductionDescriptors *m_disallowed;
  bool canCombineWith(const DisallowRule &rhs) const;
  bool combineWith(const DisallowRule &rhs);
};

enum SymbolType {
  SymbolTypeUnknown,
  SymbolTypeTerminal,
  SymbolTypeNonterminal
};

class SymbolDef {
public:
  int m_tokid;
  int m_basetokid;
  String m_name;
  SymbolType m_symboltype;
  String m_semantictype;
  Production *m_p;
  SymbolDef() : m_tokid(-1), m_basetokid(-1), m_symboltype(SymbolTypeUnknown), m_p(0) {}
  SymbolDef(int tokid, const String &name, SymbolType symboltype, int basetokid) : m_tokid(tokid), m_basetokid(basetokid), m_name(name), m_symboltype(symboltype), m_p(0) {}
};

enum Assoc {
  AssocLeft,
  AssocRight,
  AssocNon
};

class AssociativeRule {
public:
  Assoc m_assoc;
  ProductionDescriptors *m_p;
  AssociativeRule(Assoc assoc, ProductionDescriptors *p) : m_assoc(assoc), m_p(p) {}
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
  Vector<AssociativeRule*> m_assocrules;
  Vector<PrecedenceRule*> m_precrules;
  void addProduction(Tokenizer &toks, Production *p);
  bool addProduction(Production *p);
  int findSymbolId(const String &s) const;
  int addSymbolId(const String &s, SymbolType stype, int basetokid);
  int findOrAddSymbolId(Tokenizer &toks, const String &s, SymbolType stype, int basetokid);
  int getStartNt() { return m_startnt; }
  int getBaseTokId(int s) const;
  Production *getStartProduction() const { return m_startProduction; }
  Vector<Production*> productionsAt(const Production *p, int idx) const;
  bool expandAssocRules(String &err);
  bool expandPrecRules(String &err);
  bool combineRules(String &err);
  SymbolType getSymbolType(int tok) const;
};

class ParserError {
public:
  String m_err;
  ParserError(const String &err) : m_err(err) {}
};

class ParserErrorWithLineCol : public ParserError {
public:
  int m_line, m_col;
  ParserErrorWithLineCol(int line, int col, const String &err) : ParserError(err), m_line(line), m_col(col) {}
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
void NormalizeParser(ParserDef &parser);
void SolveParser(const ParserDef &parser, ParserSolution &solution);
void OutputParserSolution(FILE *out, const ParserDef &parser, const ParserSolution &solution, OutputLanguage language);

#endif /* __parser_h */
