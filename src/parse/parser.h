#ifndef __parser_h
#define __parser_h

#include "../tok/tok.h"
#include "../tok/tinytemplates.h"
#define EOF_TOK (-1)
#define MARKER_DOT (999999)

using namespace std;
class ParserDef;
class Production;

enum SymbolType {
  SymbolTypeUnknown,
  SymbolTypeTerminal,
  SymbolTypeNonterminal,
  SymbolTypeNonterminalProduction
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

enum ActionType {
  ActionTypeSrc,
  ActionTypeDollarDollar,
  ActionTypeDollarNumber
};
 
struct ActionItem {
  ActionType m_actiontype;
  String m_str;
  int m_dollarnum;
};

class Production {
public:
  bool m_rejectable;
  int m_nt;
  int m_pid;
  Vector<int> m_symbols;
  Vector<ActionItem> m_action;
  int m_lineno;
  String m_filename;

  Production(bool rejectable, int nt, const Vector<int> &symbols, const Vector<ActionItem> &action, int lineno, String filename);
  Production *clone();
  void print(FILE *out, const Map<int,SymbolDef> &tokens, int pos=-1, int forbidstate=0) const;
  bool operator<(const Production &rhs) const {
    if( m_pid < rhs.m_pid )
      return true;
    if( m_pid > rhs.m_pid )
      return false;
    if( m_pid != -1 && m_pid == rhs.m_pid )
      return false;
    if( m_nt < rhs.m_nt )
      return true;
    if( m_nt > rhs.m_nt )
      return false;
    if( m_symbols < rhs.m_symbols )
      return true;
    return false;
  }  

  // for use in qsort
  static int cmpprdid(const void *lhs, const void *rhs);
};

class ProductionState {
public:
  Production *m_p;
  int m_pos;
  int m_forbidstate;
  ProductionState(Production *p, int pos, int forbidstate) : m_p(p), m_pos(pos), m_forbidstate(forbidstate) {}
  ProductionState(const ProductionState &rhs) : m_p(rhs.m_p), m_pos(rhs.m_pos), m_forbidstate(rhs.m_forbidstate) {}
  bool operator<(const ProductionState &rhs) const {
    if( *m_p < *rhs.m_p )
      return true;
    else if( *rhs.m_p < *m_p )
      return false;
    if( m_forbidstate < rhs.m_forbidstate )
      return true;
    else if( rhs.m_forbidstate < m_forbidstate )
      return false;
    if( m_pos < rhs.m_pos )
      return true;
    return false;
  }
  int symbol() const {
    if( m_pos >= m_p->m_symbols.size() )
      return -1;
    return m_p->m_symbols[m_pos];
  }
};

enum Assoc {
  AssocLeft,
  AssocRight,
  AssocNon
};


class ProductionDescriptor {
protected:
  Vector<int> m_symbols;
  int m_dots;
  int m_dotcnt;
  ProductionDescriptor(int nt, const Vector<int> &symbols, int dots, int dotcnt);
  bool hasDotAt(int pos) const;
public:
  int m_nt;
  ProductionDescriptor() {}
  ProductionDescriptor(int nt, const Vector<int> &symbols);
  bool matchesProduction(const Production *p) const;
  bool matchesProductionAndPosition(const Production *p, int pos) const;
  // Check if, ignoring dots, the productions are the same
  bool hasSameProductionAs(const ProductionDescriptor &rhs) const;
  // Add dots from rhs to this descriptor.  Fail if not same production.
  bool addDotsFrom(const ProductionDescriptor &rhs);
  void addDots(int dots);
  void removeDots(int dots);
  int appearancesOf(int sym, int &at) const;
  bool operator<(const ProductionDescriptor &rhs) const;
  ProductionDescriptor *clone() const;
  ProductionDescriptor UnDottedProductionDescriptor() const;
  ProductionDescriptor DottedProductionDescriptor(int nt, Assoc assoc) const;
  Vector<int> symbolsAtDots() const;
  bool operator==(const ProductionDescriptor &rhs) const {
    return m_nt == rhs.m_nt && m_dots == rhs.m_dots && m_dotcnt == rhs.m_dotcnt && m_symbols == rhs.m_symbols;
  }
  bool operator!=(const ProductionDescriptor &rhs) const {
    return ! operator==(rhs);
  }
  void print(FILE *out, const Map<int,SymbolDef> &tokens) const;
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
  bool matchesProduction(const Production *lhs) const {
    for( const_iterator c = begin(), e = end(); c != e; ++c )
     if( c->matchesProduction(lhs) )
       return true;
    return false;
  }
  bool matchesProductionAndPosition(const Production *lhs, int pos) const {
    for( const_iterator c = begin(), e = end(); c != e; ++c )
     if( c->matchesProductionAndPosition(lhs,pos) )
       return true;
    return false;
  }
  void trifrucate(ProductionDescriptors &rhs, ProductionDescriptors &overlap);
  void print(FILE *out, const Map<int,SymbolDef> &tokens) const;
};

class PrecedenceRule : public Vector<ProductionDescriptors*> {
public:
  void print(FILE *out, const Map<int,SymbolDef> &tokens) const;
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
  void print(FILE *out, const Map<int,SymbolDef> &tokens) const;
};

class DisallowRule {
public:
  ProductionDescriptors *m_lead;
  Vector<DisallowProductionDescriptors*> m_intermediates;
  ProductionDescriptors *m_disallowed;
  void consolidateRules();
  void print(FILE *out, const Map<int,SymbolDef> &tokens) const;
};

class AssociativeRule {
public:
  Assoc m_assoc;
  ProductionDescriptors *m_pds;
  AssociativeRule(Assoc assoc, ProductionDescriptors *pds) : m_assoc(assoc), m_pds(pds) {}
  void print(FILE *out, const Map<int,SymbolDef> &tokens) const;
};

class ForbidDescriptor {
public:
  String m_name;
  ProductionDescriptors *m_positions;
  ProductionDescriptors *m_forbidden;

  ForbidDescriptor();
  ForbidDescriptor(const String &name, ProductionDescriptors *positions, ProductionDescriptors *forbidden);
  ForbidDescriptor(const ForbidDescriptor &rhs);
  bool forbids(const Production *positionProduction, int pos, const Production *expandProduction) const;
  bool operator<(const ForbidDescriptor &rhs) const;
  void print(FILE *out, const Map<int,SymbolDef> &tokens) const;
};

typedef Set<ForbidDescriptor> ForbidDescriptors;

typedef Map<int,ForbidDescriptors> StateToForbids;

class ForbidSub {
public:
  const ProductionDescriptors *m_lhs, *m_rhs;
  ForbidSub();
  ForbidSub(const ProductionDescriptors *lhs, const ProductionDescriptors *rhs);
  ForbidSub(const ForbidSub &rhs);
  bool operator<(const ForbidSub &rhs) const;
  bool matches(const Production *lhs, int pos, const Production *rhs) const;
  void print(FILE *out, const Map<int,SymbolDef> &tokens) const;
};

class ForbidAutomata {
  int m_nextstate;
  int m_nxtnameidx;
  int m_q0;
  StateToForbids m_statetoforbids;
  Map< int,Set<int> > m_emptytransitions;
  Map< int,Map< ForbidSub,Set<int> > > m_transitions;
public:
  ForbidAutomata();
  int newstate();
  int nstates() const;
  void addEmptyTransition(int s0, int s1);
  void addTransition(int s0, int s1, const ProductionDescriptors *subLhs, const ProductionDescriptors *subRhs);
  // Find *the* state after the current state, assuming this is a deterministic automata
  int nextState(const Production *curp, int pos, int stateno, const Production *p) const;
  bool isForbidden(const Production *p, int pos, int forbidstate, const Production *ptest) const;
  void addRule(const DisallowRule *rule);
  void closure(Set<int> &multistate) const;
  void ForbidsFromStates(const Set<int> &stateset, ForbidDescriptors &forbids) const;
  void SymbolsFromStates(const Set<int> &stateset, Set<ForbidSub> &symbols) const;
  void toDeterministicForbidAutomata(ForbidAutomata &out) const;
  void print(FILE *out, const Map<int,SymbolDef> &tokens) const;
};

typedef  Pair<Production*,int>  productionandforbidstate_t;

class ProductionsAtKey {
public:
  const Production *m_p;
  int m_pos;
  int m_forbidstate;
  bool operator<(const ProductionsAtKey &rhs) const {
    if( m_p < rhs.m_p )
      return true;
    else if( m_p > rhs.m_p )
      return false;
    if( m_pos < rhs.m_pos )
      return true;
    else if( m_pos > rhs.m_pos )
      return false;
    if( m_forbidstate < rhs.m_forbidstate )
      return true;
     return false;
  }
  bool operator==(const ProductionsAtKey &rhs) const {
    return m_p == rhs.m_p && m_pos == rhs.m_pos && m_forbidstate == rhs.m_forbidstate;
  }
};

class ParserDef {
  int m_nextsymbolid;
  Production *m_startProduction;
  FILE *m_vout;
  int m_verbosity;
public:
  ParserDef(FILE *vout, int verbosity) : m_nextsymbolid(0), m_startProduction(0), m_vout(vout), m_verbosity(verbosity) {}
  Map<String,int> m_tokens;
  Map<int,SymbolDef> m_tokdefs;
  Vector<Production*> m_productions;
  Vector<DisallowRule*> m_disallowrules;
  Vector<AssociativeRule*> m_assocrules;
  Vector<PrecedenceRule*> m_precrules;
  ForbidAutomata m_forbid;
  void addProduction(Tokenizer &toks, Production *p);
  bool addProduction(Production *p);
  int findSymbolId(const String &s) const;
  int addSymbolId(const String &s, SymbolType stype);
  int findOrAddSymbolId(Tokenizer &toks, const String &s, SymbolType stype);
  int getStartNt() const;
  int getExtraNt() const;
  Production *getStartProduction() const { return m_startProduction; }
  const Vector<productionandforbidstate_t> &productionsAt(const Production *p, int pos, int forbidstate, Map< ProductionsAtKey,Vector<productionandforbidstate_t> > &productionsAtResults) const;
  void computeForbidAutomata();
  SymbolType getSymbolType(int tok) const;
  void print(FILE *out) const;
  void addDisallowRule(DisallowRule *rule);
private:
  void expandAssocRules();
  void expandPrecRules();
  void combineRules();
};

class ParserError {
public:
  String m_err;
  ParserError(const String &err) : m_err(err) {}
};

class ParserErrorWithLineCol : public ParserError {
public:
  int m_line, m_col;
  String m_filename;
  ParserErrorWithLineCol(int line, int col, String filename, const String &err) : ParserError(err), m_line(line), m_col(col), m_filename(filename) {}
};

class state_t : public Set<ProductionState> {
public:
  void print(FILE *out, const Map<int,SymbolDef> &tokens) const;
};

typedef Map<int,Set<int> > shifttosymbols_t;
typedef Map<int,shifttosymbols_t> shiftmap_t;
typedef Map<Production*,Set<int> > reducebysymbols_t;
typedef Map<int,reducebysymbols_t> reducemap_t;
typedef Pair<int,int> symbolandforbidstate_t;

class ParserSolution {
public:
  Map< symbolandforbidstate_t, Set<int> > m_firsts;
  Map< symbolandforbidstate_t, Set<int> > m_follows;
  Vector<state_t> m_states;
  shiftmap_t m_shifts;
  reducemap_t m_reductions;
};

enum OutputLanguage {
  LanguageJs,
  LanguageC
};

class LanguageOutputOptions {
public:
  LanguageOutputOptions() : min_nt_value(0), do_pound_line(true) {
  }
  int min_nt_value;
  bool do_pound_line;
};

void ParseParser(TokBuf *tokbuf, ParserDef &parser, FILE *vout, int verbosity);
void SolveParser(ParserDef &parser, ParserSolution &solution, FILE *vout, int verbosity);
void OutputParserSolution(FILE *out, const ParserDef &parser, const ParserSolution &solution, OutputLanguage language, LanguageOutputOptions &options);

#endif /* __parser_h */

