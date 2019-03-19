#ifndef __parser_h
#define __parser_h

#include "../tok/tok.h"
#include "../tok/tinytemplates.h"
#define EOF_TOK (-1)

using namespace std;
class ParserDef;

class Production {
public:
  bool m_rejectable;
  int m_nt;
  Vector<int> m_symbols;
  String m_action;

  Production(bool rejectable, int nt, const Vector<int> &symbols, String action);
  Production *clone();
  void print(FILE *out, const Map<int,String> &tokens, int idx=-1) const;
};

class ProductionState {
public:
  Production *m_p;
  int m_idx;
  ProductionState(Production *p, int idx) : m_p(p), m_idx(idx) {}
  ProductionState(const ProductionState &rhs) : m_p(rhs.m_p), m_idx(rhs.m_idx) {}
  bool operator<(const ProductionState &rhs) const {
    if( m_p < rhs.m_p )
      return true;
    else if( rhs.m_p < m_p )
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
  bool matchesProduction(const Production *p, const ParserDef &parser, bool useBaseTokId) const;
  bool matchesProductionAndPosition(const Production *p, int pos, const ParserDef &parser, bool useBaseTokId) const;
  // Check if, ignoring dots, the productions are the same
  bool hasSameProductionAs(const ProductionDescriptor &rhs) const;
  // Add dots from rhs to this descriptor.  Fail if not same production.
  bool addDotsFrom(const ProductionDescriptor &rhs);
  bool dotIntersection(const ProductionDescriptor &rhs, Vector<int> &lhsdot, Vector<int> &rhsdot, Vector<int> &dots) const;
  void insert(const Vector<int> &dots, int sym);
  void remove(const Vector<int> &dots);
  int countOf(int sym) const {
    int cnt = 0;
    for( Vector<int>::const_iterator c = m_symbols.begin(), e = m_symbols.end(); c != e; ++c )
      if( *c == sym )
        ++cnt;
    return cnt;
  }
  bool operator<(const ProductionDescriptor &rhs) const;
  ProductionDescriptor *clone() const;
  bool operator==(const ProductionDescriptor &rhs) const {
    return m_nt == rhs.m_nt && m_symbols == rhs.m_symbols;
  }
  bool operator!=(const ProductionDescriptor &rhs) const {
    return ! operator==(rhs);
  }
  void print(FILE *out, const Map<int,String> &tokens) const;
};

enum Assoc {
  AssocLeft,
  AssocRight,
  AssocNon
};

class ProductionDescriptors : public Set<ProductionDescriptor> {
public:
  void consolidateRules();
  ProductionDescriptors *clone() const;
  ProductionDescriptors *DottedProductionDescriptors(int nt, Assoc assoc) const;
  ProductionDescriptors *UnDottedProductionDescriptors() const;

  bool operator==(const ProductionDescriptors &rhs) const {
    return Set<ProductionDescriptor>::operator==(rhs);
  }
  bool operator!=(const ProductionDescriptors &rhs) const {
    return ! operator==(rhs);
  }
  bool matchesProduction(const Production *lhs, const ParserDef &parser, bool useBaseTokId) const {
    for( const_iterator c = begin(), e = end(); c != e; ++c )
     if( c->matchesProduction(lhs,parser,useBaseTokId) )
       return true;
    return false;
  }
  bool matchesProductionAndPosition(const Production *lhs, int pos, const ParserDef &parser, bool useBaseTokId) const {
    for( const_iterator c = begin(), e = end(); c != e; ++c )
     if( c->matchesProductionAndPosition(lhs,pos,parser,useBaseTokId) )
       return true;
    return false;
  }
  void trifrucate(ProductionDescriptors &rhs, ProductionDescriptors &overlap);
  void print(FILE *out, const Map<int,String> &tokens) const;
};

class PrecedenceRule : public Vector<ProductionDescriptors*> {
public:
  void print(FILE *out, const Map<int,String> &tokens) const;
};

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
  void print(FILE *out, const Map<int,String> &tokens) const;
};

class DisallowRule {
public:
  ProductionDescriptors *m_lead;
  Vector<DisallowProductionDescriptors*> m_intermediates;
  ProductionDescriptors *m_disallowed;
  void consolidateRules();
  void print(FILE *out, const Map<int,String> &tokens) const;
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

class AssociativeRule {
public:
  Assoc m_assoc;
  ProductionDescriptors *m_pds;
  AssociativeRule(Assoc assoc, ProductionDescriptors *pds) : m_assoc(assoc), m_pds(pds) {}
  void print(FILE *out, const Map<int,String> &tokens) const;
};

class ParserDef {
  int m_nextsymbolid;
  Production *m_startProduction;
public:
  ParserDef() : m_nextsymbolid(0), m_startProduction(0) {}
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
  int getStartNt() const;
  int getBaseTokId(int s) const;
  Production *getStartProduction() const { return m_startProduction; }
  Vector<Production*> productionsAt(const Production *p, int idx, const Vector<Production*> *pproductions = 0) const;
  void expandAssocRules();
  void expandPrecRules();
  void combineRules();
  SymbolType getSymbolType(int tok) const;
  void print(FILE *out) const;
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

class state_t : public Set<ProductionState> {
public:
  void print(FILE *out, const Map<int,String> &tokens) const;
};

typedef Map<int,Set<int> > shifttosymbols_t;
typedef Map<int,shifttosymbols_t> shiftmap_t;
typedef Map<Production*,Set<int> > reducebysymbols_t;
typedef Map<int,reducebysymbols_t> reducemap_t;

class ParserSolution {
public:
  Map<int, Set<int> > m_firsts;
  Map<int, Set<int> > m_follows;
  Vector<state_t> m_states;
  shiftmap_t m_shifts;
  reducemap_t m_reductions;
};

enum OutputLanguage {
  LanguageJs,
  LanguageC
};

void ParseParser(TokBuf *tokbuf, ParserDef &parser, FILE *vout, int verbosity);
void NormalizeParser(ParserDef &parser, FILE *vout, int verbosity);
void SolveParser(const ParserDef &parser, ParserSolution &solution, FILE *vout, int verbosity);
void OutputParserSolution(FILE *out, const ParserDef &parser, const ParserSolution &solution, OutputLanguage language);

#endif /* __parser_h */
