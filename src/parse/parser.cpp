#include "parser.h"
namespace pptok {
#include "parsertok.h"

TokenInfo pptokinfo = {
  tokenCount,
  sectionCount,
  tokenaction,
  tokenstr,
  isws,
  stateCount,
  transitions,
  transitionOffset,
  tokens
};
}

// parser : parsepart +
// parsepart : (typedefrule|production|associativityrule|precedencerule|disallowrule) SEMI
// typedefrule : TYPEDEF ID nonterminal+
// productiondecl : REJECTABLE? production action?
// production: nonterminal COLON symbol+ (VERTICAL symbol+)+
// nonterminal : START | ID
// symbol : ID | ERROR
// associativityrule : ['LEFT-ASSOCIATIVE'|'RIGHT-ASSOCIATIVE'|'NON-ASSOCIATIVE'] productiondescriptors
// precedencerule : PRECEDENCE precedenceparts
// precedenceparts : precedencepart ['>' precedencepart]+
// precedencepart : productiondescriptors
// productiondescriptors : productiondescriptors productiondescriptor
// productiondescriptor : '[' nonterminal COLON (symbol|'.' symbol)+ ']'
// productions : ('[' nonterminal COLON (symbol)+ ']')+
// disallowrule : DISALLOW productiondescriptors (ARROW productiondescriptors '*'?)+ NOTARROW productions

ProductionDescriptor::ProductionDescriptor(int nt, Vector<int> symbols) : m_nt(nt), m_symbols(symbols) {}

bool ProductionDescriptor::hasSameProductionAs(const ProductionDescriptor &rhs) const {
  if( m_nt != rhs.m_nt )
    return false;
  Vector<int>::const_iterator curlhs = m_symbols.begin(), endlhs = m_symbols.end(), currhs = rhs.m_symbols.begin(), endrhs = rhs.m_symbols.end();
  while( curlhs != endlhs && currhs != endrhs ) {
    if( *curlhs == pptok::DOT )
      ++curlhs;
    if( *currhs == pptok::DOT )
      ++currhs;
    if( *curlhs != *currhs )
      return false;
    ++curlhs;
    ++currhs;
  }
  if( curlhs != endlhs || currhs != endrhs )
    return false;
  return true;
}

bool ProductionDescriptor::addDotsFrom(const ProductionDescriptor &rhs) {
  if( ! hasSameProductionAs(rhs) )
    return false;
  Vector<int>::iterator curlhs = m_symbols.begin(), endlhs = m_symbols.end();
  Vector<int>::const_iterator currhs = rhs.m_symbols.begin(), endrhs = rhs.m_symbols.end();
  while( curlhs != endlhs && currhs != endrhs ) {
    if( *curlhs == pptok::DOT && *currhs == pptok::DOT ) {
      ++curlhs;
      ++curlhs;
      ++currhs;
      ++currhs;
    }
    else if( *curlhs == pptok::DOT ) {
      ++curlhs;
      ++curlhs;
      ++currhs;
    }
    else if( *currhs == pptok::DOT ) {
      curlhs = m_symbols.insert(curlhs,pptok::DOT);
      endlhs = m_symbols.end();
      ++curlhs;
      ++curlhs;
      ++currhs;
      ++currhs;
    } else {
      ++curlhs;
      ++currhs;
    }
  }
  return true;
}

bool ProductionDescriptor::operator<(const ProductionDescriptor &rhs) const {
  if( m_nt < rhs.m_nt )
    return true;
  else if( rhs.m_nt < m_nt )
    return false;
  if( m_symbols < rhs.m_symbols )
    return true;
  return false;
}

ProductionDescriptor *ProductionDescriptor::clone() const {
  ProductionDescriptor *ret = new ProductionDescriptor();
  ret->m_nt = m_nt;
  ret->m_symbols = m_symbols;
  return ret;
}

Production::Production(bool rejectable, int nt, const Vector<int> &symbols, String action) : m_rejectable(rejectable), m_nt(nt), m_symbols(symbols), m_action(action) {}

Production *Production::clone() {
  return new Production(m_rejectable, m_nt, m_symbols, m_action);
}

static void error(Tokenizer &toks, const String &err) {
  throw ParserErrorWithLineCol(toks.line(),toks.col(),err);
}

void ParserDef::addProduction(Tokenizer &toks, Production *p) {
  if( p->m_nt == pptok::START ) {
    if( m_startProduction )
      error(toks,"<start> production can be defined only once");
    m_startProduction = p;
  }
  m_productions.push_back(p);
}

bool ParserDef::addProduction(Production *p) {
  if( p->m_nt == pptok::START ) {
    if( m_startProduction )
      return false;
    m_startProduction = p;
  }
  m_productions.push_back(p);
  return true;
}

SymbolType ParserDef::getSymbolType(int tok) const {
  if( m_tokdefs.find(tok) == m_tokdefs.end() )
    return SymbolTypeUnknown;
  return m_tokdefs[tok].m_symboltype;
}

int ParserDef::findSymbolId(const String &s) const {
  Map<String,int>::const_iterator iter = m_tokens.find(s);
  if( iter == m_tokens.end() )
    return -1;
  return iter->second;
}

int ParserDef::addSymbolId(const String &s, SymbolType stype, int basetokid) {
  Map<String,int>::iterator iter = m_tokens.find(s);
  if( iter != m_tokens.end() )
    return -1;
  String name(s);
  int tokid = m_nextsymbolid++;
  m_tokens[s] = tokid;
  m_tokdefs[tokid] = SymbolDef(tokid, name, stype, basetokid);
  return tokid;
 } 

int ParserDef::findOrAddSymbolId(Tokenizer &toks, const String &s, SymbolType stype, int basetokid) {
  Map<String,int>::iterator iter = m_tokens.find(s);
  if( iter != m_tokens.end() ) {
    int tokid = iter->second;
    SymbolDef &symboldef = m_tokdefs[tokid];
    if( stype == SymbolTypeUnknown ) {
      // is ok
    } else if( symboldef.m_symboltype == SymbolTypeUnknown && stype != SymbolTypeUnknown ) {
      // now we know
      symboldef.m_symboltype = stype;
    } else if( symboldef.m_symboltype != stype ) {
      if( symboldef.m_symboltype == SymbolTypeNonterminal )
        error(toks,String("Attempted to turn Nonterminal ")+symboldef.m_name+"into a Terminal");
      else if( symboldef.m_symboltype == SymbolTypeTerminal )
        error(toks,String("Attempted to turn Terminal ")+symboldef.m_name+"into a NonTerminal");
      symboldef.m_symboltype = stype;
    }
    return tokid;
  }
  String name(s);
  int tokid = m_nextsymbolid++;
  m_tokens[s] = tokid;
  m_tokdefs[tokid] = SymbolDef(tokid, name, stype, basetokid);
  return tokid;
}

int ParserDef::getStartNt() const {
  return pptok::START;
}

int ParserDef::getBaseTokId(int s) const {
  Map<int,SymbolDef>::const_iterator iter = m_tokdefs.find(s);
  while( iter != m_tokdefs.end() ) {
    if( iter->second.m_basetokid == -1 )
      return s;
    s = iter->second.m_basetokid;
    iter = m_tokdefs.find(s);
  }
  return -1;
}

Vector<Production*> ParserDef::productionsAt(const Production *p, int idx, const Vector<Production*> *pproductions) const {
  Vector<Production*> productions;
  if( ! pproductions )
    pproductions = &m_productions;
  if( idx < p->m_symbols.size() ) {
    int symbol = p->m_symbols[idx];
    if( getSymbolType(symbol) == SymbolTypeNonterminal ) {
      for( Vector<Production*>::const_iterator cur = pproductions->begin(), end = pproductions->end(); cur != end; ++cur ) {
        Production *p = *cur;
        if( p->m_nt != symbol )
          continue;
        productions.push_back(p);
      }
    }
  }
  return productions;
}

ProductionDescriptor UnDottedProductionDescriptor(const ProductionDescriptor &pd) {
  ProductionDescriptor pdundotted;
  pdundotted.m_nt = pd.m_nt;
  for( int i = 0; i < pd.m_symbols.size(); ++i ) {
    int sym = pd.m_symbols[i];
    if( sym == pptok::DOT )
      continue;
    pdundotted.m_symbols.push_back(sym);
  }
  return pdundotted;
}

ProductionDescriptors *ProductionDescriptors::UnDottedProductionDescriptors() const {
  ProductionDescriptors *pdsundotted = new ProductionDescriptors();
  for( ProductionDescriptors::const_iterator cur = begin(), enditer = end(); cur != enditer; ++cur ) {
    ProductionDescriptor pd = UnDottedProductionDescriptor(*cur);
    pdsundotted->insert(pd);
  }
  return pdsundotted;
}

ProductionDescriptor DottedProductionDescriptor(const ProductionDescriptor &pd, int nt, Assoc assoc) {
  ProductionDescriptor pddotted;
  pddotted.m_nt = pd.m_nt;
  int firstnt = -1;
  int lastnt = -1;
  for( int i = 0; i < pd.m_symbols.size(); ++i ) {
    int sym = pd.m_symbols[i];
    if( sym == nt ) {
      if( firstnt == -1 )
        firstnt = i;
      lastnt = i;
    }
  }
  for( int i = 0; i < pd.m_symbols.size(); ++i ) {
    int sym = pd.m_symbols[i];
    if( sym == nt && ( assoc == AssocNon || (assoc == AssocLeft && i != firstnt) || (assoc == AssocRight && i != lastnt) ) )
      pddotted.m_symbols.push_back(pptok::DOT);
    pddotted.m_symbols.push_back(sym);
  }
  return pddotted;
}

ProductionDescriptors *ProductionDescriptors::DottedProductionDescriptors(int nt, Assoc assoc) const {
  ProductionDescriptors *pdsdotted = new ProductionDescriptors();
  for( ProductionDescriptors::const_iterator cur = begin(), enditer = end(); cur != enditer; ++cur ) {
    ProductionDescriptor pd = DottedProductionDescriptor(*cur,nt,assoc);
    pdsdotted->insert(pd);
  }
  return pdsdotted;
}

// No rule may nest into another rule in the same group of productions, including itself.
// We'll assume the rules are valid because they were validated by the parser.
// If something truly disasterous happens, return false.
void ParserDef::expandAssocRules() {
  while( m_assocrules.size() ) {
    AssociativeRule *rule = m_assocrules.back();
    int nt = rule->m_pds->begin()->m_nt;
    DisallowRule *dr = new DisallowRule();
    dr->m_disallowed = rule->m_pds->clone();
    dr->m_lead = rule->m_pds->DottedProductionDescriptors(nt, rule->m_assoc);
    m_disallowrules.push_back(dr);
    delete rule;
    m_assocrules.pop_back();
  }
}

// No rule may appear in a rule of higher precedence.
// We'll assume the rules are valid because they were validated by the parser.
// If something truly disasterous happens, return false.
void ParserDef::expandPrecRules() {
  ProductionDescriptors descriptors;
  while( m_precrules.size() ) {
    PrecedenceRule *precrule = m_precrules.back();
    int nt = -1;
    for( PrecedenceRule::iterator cur = precrule->begin()+(precrule->size()-1), begin = precrule->begin(), last = precrule->begin()+(precrule->size()-1); cur >= begin; --cur ) {
      ProductionDescriptors *pds = *cur;
      if( nt != -1 ) {
        DisallowRule *dr = new DisallowRule();
        dr->m_lead = pds->DottedProductionDescriptors(nt,AssocNon);
        dr->m_disallowed = descriptors.clone();
        dr->consolidateRules();
        m_disallowrules.push_back(dr);
      }
      nt = pds->begin()->m_nt;
      descriptors.insert(pds->begin(), pds->end());
    }
    delete precrule;
    m_precrules.pop_back();
  }
}

// Combine whatever rules can be combined.
void ParserDef::combineRules() {
  for( int i = 0; i < m_disallowrules.size(); ++i )
    m_disallowrules[i]->consolidateRules();

  // Trifrucate intersections
  for( int i = 0; i < m_disallowrules.size(); ++i ) {
    DisallowRule *lhs = m_disallowrules[i];
    for( int j = i+1; j < m_disallowrules.size(); ++j ) {
      DisallowRule *rhs = m_disallowrules[j];
      if( lhs->m_intermediates != rhs->m_intermediates )
        continue;
      ProductionDescriptors overlap;
      lhs->m_lead->trifrucate(*rhs->m_lead,overlap);
      if( overlap.size() ) {
        DisallowRule *dr = new DisallowRule();
        dr->m_lead = overlap.clone();
        dr->m_disallowed = lhs->m_disallowed->clone();
        dr->m_disallowed->insert(rhs->m_disallowed->begin(),rhs->m_disallowed->end());
        dr->m_disallowed->consolidateRules();
        m_disallowrules.push_back(dr);
      }
      if( rhs->m_lead->size() == 0 ) {
        m_disallowrules.erase(m_disallowrules.begin()+j);
        --j;
      }
      if( lhs->m_lead->size() == 0 ) {
        m_disallowrules.erase(m_disallowrules.begin()+i);
        --i;
        j = m_disallowrules.size()-1;
      }
    }
  }

  // Simplify anything left
  for( int i = 0; i < m_disallowrules.size(); ++i ) {
    DisallowRule *lhs = m_disallowrules[i];
    for( int j = i+1; j < m_disallowrules.size(); ++j ) {
      DisallowRule *rhs = m_disallowrules[j];
      if( lhs->m_intermediates != rhs->m_intermediates )
        continue;
      if( *lhs->m_lead == *rhs->m_lead ) {
        lhs->m_disallowed->insert(rhs->m_disallowed->begin(),rhs->m_disallowed->end());
        lhs->consolidateRules();
        delete rhs;
        m_disallowrules.erase(m_disallowrules.begin()+j);
        --i;
        j = m_disallowrules.size()-1;
      } else if( *lhs->m_disallowed == *rhs->m_disallowed ) {
        lhs->m_lead->insert(rhs->m_lead->begin(),rhs->m_lead->end());
        lhs->consolidateRules();
        delete rhs;
        m_disallowrules.erase(m_disallowrules.begin()+j);
        --i;
        j = m_disallowrules.size()-1;
      }
    }
  }
}

bool ProductionDescriptor::matchesProduction(const Production *p, const ParserDef &parser, bool useBaseTokId) const {
  // '.' is ignored for matching purposes.
  if( m_nt != p->m_nt )
    return false;
  Vector<int>::const_iterator curs = p->m_symbols.begin(), ends = p->m_symbols.end(), curwc = m_symbols.begin(), endwc = m_symbols.end();
  while( curs != ends && curwc != endwc ) {
    if( *curwc == pptok::DOT )
      ++curwc;
    if( parser.getBaseTokId(*curs) != *curwc )
      return false;
    ++curs;
    ++curwc;
  }
  if( curs != ends || curwc != endwc )
    return false;
  return true;
}

bool ProductionDescriptor::matchesProductionAndPosition(const Production *p, int pos, const ParserDef &parser, bool useBaseTokId) const {
  int matchPos = 0;
  bool matchedPos = false;
  if( m_nt != parser.getBaseTokId(p->m_nt) )
    return false;
  Vector<int>::const_iterator curs = p->m_symbols.begin(), ends = p->m_symbols.end(), curwc = m_symbols.begin(), endwc = m_symbols.end();
  while( curs != ends && curwc != endwc ) {
    if( *curwc == pptok::DOT ) {
      ++curwc;
      if( matchPos == pos )
        matchedPos = true;
    }
    if( parser.getBaseTokId(*curs) != *curwc )
      return false;
    ++curs;
    ++curwc;
    ++matchPos;
  }
  if( curs != ends || curwc != endwc )
    return false;
  return matchedPos;
}

static int ParseNonterminal(Tokenizer &toks, ParserDef &parser) {
  if( toks.peek() == pptok::START ) {
    toks.discard();
    return pptok::START;
  } else if( toks.peek() == pptok::ID ) {
    String s = toks.tokstr();
    toks.discard();
    return parser.findOrAddSymbolId(toks, s, SymbolTypeNonterminal, -1);
  }
  return -1;
}

static int ParseSymbol(Tokenizer &toks, ParserDef &parser) {
  if( toks.peek() == pptok::ID ) {
    String s= toks.tokstr();
    toks.discard();
    return parser.findOrAddSymbolId(toks, s, SymbolTypeUnknown, -1);
  }
  return -1;
}

static Vector<int> ParseSymbols(Tokenizer &toks, ParserDef &parser) {
  Vector<int> symbols;
  int s = ParseSymbol(toks,parser);
  if( s == -1 )
    error(toks,"expected symbol");
  symbols.push_back(s);
  while( (s = ParseSymbol(toks,parser)) != -1 )
    symbols.push_back(s);
  return symbols;
}

static String ParseAction(Tokenizer &toks, ParserDef &parser) {
  String s;
  if( toks.peek() != pptok::LBRACE )
    return s;
  toks.discard();
  while( toks.peek() != -1 && toks.peek() != pptok::RBRACE ) {
    if( toks.peek() == pptok::LBRACE )
      s += ParseAction(toks,parser);
    s += toks.tokstr();
    toks.discard();
  }
  if( toks.peek() != pptok::RBRACE )
    error(toks,"Missing '}'");
  toks.discard();
  return s;
}

static Vector<Production*> ParseProduction(Tokenizer &toks, ParserDef &parser) {
  bool rejectable = false;
  if( toks.peek() == pptok::REJECTABLE ) {
    rejectable = true;
    toks.discard();
  }
  int nt = ParseNonterminal(toks,parser);
  if( nt == -1 )
    error(toks,"Expected nonterminal");
  if( toks.peek() != pptok::COLON )
    error(toks,"Exptected ':'");
  toks.discard();
  Vector<Production*> productions;
  while(true) {
    Vector<int> symbols = ParseSymbols(toks,parser);
    String action;
    if( toks.peek() == pptok::LBRACE )
      action = ParseAction(toks,parser);
    char buf[64];
    sprintf(buf,"Production-%d",parser.m_productions.size()+1);
    String productionName = buf;
    Production *p = new Production(rejectable,nt,symbols,action);
    productions.push_back(p);
    if( toks.peek() != pptok::VSEP )
      break;
    toks.discard();
  }
  return productions;
}

static int ParseSymbolOrDottedSymbol(Tokenizer &toks, ParserDef &parser, bool  &dotted) {
  int symbol = -1;
  dotted = false;
  if(  toks.peek() == pptok::DOT ) {
    toks.discard();
    dotted = true;
    symbol = ParseSymbol(toks,parser);
  } else
    symbol = ParseSymbol(toks,parser);
  return symbol;
}

static ProductionDescriptor *ParseProductionDescriptor(Tokenizer &toks, ParserDef &parser) {
  if( toks.peek() != pptok::LBRKT )
    error(toks,"Expected '[' to start production descriptor");
  toks.discard();
  int nt = ParseNonterminal(toks,parser);
  if( nt == -1 )
    error(toks,"expected nonterminal");
  if( toks.peek() != pptok::COLON )
    error(toks,"Expected ':' in production descriptor");
  toks.discard();
  bool dotted = false;
  int symbol = ParseSymbolOrDottedSymbol(toks,parser,dotted);
  if( symbol == -1 )
    error(toks,"expected symbol or '.' symbol ");
  Vector<int> symbols;
  while( symbol != -1 ) {
    if( dotted )
      symbols.push_back(pptok::DOT);
    symbols.push_back(symbol);
    symbol = ParseSymbolOrDottedSymbol(toks,parser,dotted);
  }
  if( toks.peek() != pptok::RBRKT )
    error(toks,"Expected ']' to end production descriptor");
  toks.discard();
  return new ProductionDescriptor(nt, symbols);
}

static void ParseTypedefRule(Tokenizer &toks, ParserDef &parser) {
  if( toks.peek() != pptok::TYPEDEF )
    return;
  toks.discard();
  if( toks.peek() != pptok::ID )
    error(toks,"expected typedef");
  String toktype = toks.tokstr();
  toks.discard();
  int nt = ParseNonterminal(toks,parser);
  while( nt != -1 ) {
    SymbolDef &symboldef = parser.m_tokdefs[nt];
    if( symboldef.m_symboltype != SymbolTypeNonterminal )
      error(toks, String("TYPEDEF may only be applied to nonterminals, attempted to TYPEDEF ")+symboldef.m_name);
    if( symboldef.m_semantictype.length() != 0 )
      error(toks, String("TYPEDEF can only be assigned once per type.  Attempated to reassign TYPEDEF of ")+symboldef.m_name);
    parser.m_tokdefs[nt].m_semantictype = toktype;
    nt = ParseNonterminal(toks,parser);
  }
}

static ProductionDescriptors *ParseProductionDescriptors(Tokenizer &toks, ParserDef &parser) {
  ProductionDescriptor *pd = ParseProductionDescriptor(toks,parser);
  ProductionDescriptors *pds = new ProductionDescriptors();
  pds->insert(*pd);
  delete pd;
  while( toks.peek() == pptok::LBRKT ) {
    pd = ParseProductionDescriptor(toks,parser);
    pds->insert(*pd);
    delete pd;
  }
  pds->consolidateRules();
  return pds;
}

static ProductionDescriptors *ParseProductions(Tokenizer &toks, ParserDef &parser);

// Combine the dots from two production descriptors that decribe the same production.
void ProductionDescriptors::consolidateRules() {
  for( iterator lhs = begin(); lhs != end(); ++lhs ) {
    for( iterator rhs = lhs+1; rhs != end(); ++rhs ) {
      if( lhs->addDotsFrom(*rhs) ) {
        rhs = erase(rhs);
        --rhs;
      }
    }
  }
}

ProductionDescriptors *ProductionDescriptors::clone() const {
  ProductionDescriptors *ret = new ProductionDescriptors();
  *ret = *this;
  return ret;
}

static void checkProductionDescriptorsSameNonterminal(Tokenizer &toks, ProductionDescriptors *p, int lastnt, const char *name) {
  // Check all parts have the same nonterminal.
  int nt = p->begin()->m_nt;
  if( lastnt != -1 && nt != lastnt ) {
    String err = "The dotted nonterminals of a ";
    err += name;
    err += " must be the same as the previous ";
    err += name;
    err += "'s nonterminal";
    error(toks,err);
  }
  int dottednt = -1;
  for( ProductionDescriptors::iterator cur = p->begin(), end = p->end(); cur != end; ++cur ) {
    ProductionDescriptor &pd = *cur;
    if( nt != pd.m_nt ) {
      String err = "All production descriptors in an ";
      err += name;
      err += " must have the same nonterminal";
      error(toks,err);
    }
  }
}

static int checkProductionDescriptorsSameNonterminalAndDotsAgree(Tokenizer &toks, ProductionDescriptors *p, int lastnt, const char *name) {
  checkProductionDescriptorsSameNonterminal(toks,p,lastnt,name);
  // Check all parts have the same nonterminal.
  int dottednt = -1;
  for( ProductionDescriptors::iterator cur = p->begin(), end = p->end(); cur != end; ++cur ) {
    const ProductionDescriptor &pd = *cur;
    // Check that all dotted nonterminals are the same (and that there is at least 1)
    int dotcnt = 0;
    for( Vector<int>::const_iterator cursym = pd.m_symbols.begin(), endsym = pd.m_symbols.end(); cursym != endsym; ++cursym ) {
      if( *cursym == pptok::DOT ) {
        ++dotcnt;
        ++cursym;
        if( dottednt == -1 ) {
          dottednt = *cursym;
        } else if( *cursym != dottednt ) {
          String err = "All dots in a ";
          err += name;
          err += " must have the same nonterminal";
          error(toks,err);
        }
      }
    }
    if( dotcnt == 0 ) {
      String err = "Each part of a ";
      err += name;
      err += " must have at least one dot";
      error(toks,err);
    }
  }
  return dottednt;
}

static AssociativeRule *ParseAssociativeRule(Tokenizer &toks, ParserDef &parser) {
  if( toks.peek() != pptok::LEFTASSOC && toks.peek() != pptok::RIGHTASSOC && toks.peek() != pptok::NONASSOC )
    error(toks,"expected LEFT-ASSOCIATIVE or RIGHT-ASSOCITIVE or NON-ASSOCIATIVE");
  int iassoc = toks.peek();
  Assoc assoc;
  if( iassoc == pptok::LEFTASSOC )
    assoc = AssocLeft;
  else if( iassoc == pptok::RIGHTASSOC )
    assoc = AssocRight;
  if( iassoc == pptok::NONASSOC )
    assoc = AssocNon;
  toks.discard();
  ProductionDescriptors *p = ParseProductions(toks,parser);
  checkProductionDescriptorsSameNonterminal(toks,p,-1,"associative rule");
  return new AssociativeRule(assoc,p);
}

static PrecedenceRule *ParsePrecedenceRule(Tokenizer &toks, ParserDef &parser) {
  if( toks.peek() != pptok::PRECEDENCE )
    return 0;
  toks.discard();
  PrecedenceRule *rule = new PrecedenceRule();
  ProductionDescriptors *part = ParseProductions(toks,parser);
  checkProductionDescriptorsSameNonterminal(toks,part,-1,"precedence rule group");
  rule->push_back(part);
  while( toks.peek() == pptok::GT ) {
    toks.discard();
    ProductionDescriptors *part = ParseProductions(toks,parser);
    checkProductionDescriptorsSameNonterminal(toks,part,-1,"precedence rule group");
    rule->push_back(part);
  }
  return rule;
}

static DisallowProductionDescriptors *ParseDisallowProductionDescriptors(Tokenizer &toks, ParserDef &parser) {
  ProductionDescriptors *psd = ParseProductionDescriptors(toks,parser);
  bool star = false;
  if( toks.peek() == pptok::STAR ) {
    star = true;
    toks.discard();
  }
  DisallowProductionDescriptors *ret = new DisallowProductionDescriptors();
  ret->m_descriptors = psd;
  ret->m_star = star;
  return ret;
}

static ProductionDescriptors *ParseProductions(Tokenizer &toks, ParserDef &parser) {
  if( toks.peek() != pptok::LBRKT )
    error(toks,"Expected '[' to start production description");
  ProductionDescriptors *ret = new ProductionDescriptors();
  while( toks.peek() == pptok::LBRKT ) {
    toks.discard();
    int nt = ParseNonterminal(toks,parser);
    if( nt == -1 )
      error(toks,"expected nonterminal");
    if( toks.peek() != pptok::COLON )
      error(toks,"Expected ':' in production descriptor");
    toks.discard();
    int symbol = ParseSymbol(toks,parser);
    if( symbol == -1 )
      error(toks,"expected symbol");
    Vector<int> symbols;
    while( symbol != -1 ) {
      symbols.push_back(symbol);
      symbol = ParseSymbol(toks,parser);
    }
    if( toks.peek() != pptok::RBRKT )
      error(toks,"Expected ']' to end production description");
    toks.discard();
    ret->insert(ProductionDescriptor(nt,symbols));
  }
  ret->consolidateRules();
  return ret;
}

static DisallowRule *ParseDisallowRule(Tokenizer &toks, ParserDef &parser) {
  if( toks.peek() != pptok::DISALLOW )
    return 0;
  toks.discard();
  DisallowRule *rule = new DisallowRule();
  ProductionDescriptors *pd = ParseProductionDescriptors(toks,parser);
  int lastnt = checkProductionDescriptorsSameNonterminalAndDotsAgree(toks,pd,-1,"disallow rule group");
  rule->m_lead = pd;
  if( toks.peek() != pptok::ARROW && toks.peek() != pptok::NOTARROW )
    error(toks,"expected -> or -/-> in disallow rule");
  toks.discard();
  DisallowProductionDescriptors *dpd = ParseDisallowProductionDescriptors(toks,parser);
  lastnt = checkProductionDescriptorsSameNonterminalAndDotsAgree(toks,dpd->m_descriptors,lastnt,"disallow rule group");
  // when using stars, dots must be self consistent
  if( dpd->m_star && dpd->m_descriptors->begin()->m_nt != lastnt )
    error(toks,"When using '*' with a disallow group, the productions must dot their own nonterminals");
  rule->m_intermediates.push_back(dpd);
  while( toks.peek() == pptok::ARROW ) {
    toks.discard();
    dpd = ParseDisallowProductionDescriptors(toks,parser);
    lastnt = checkProductionDescriptorsSameNonterminalAndDotsAgree(toks,dpd->m_descriptors,lastnt,"disallow rule group");
    // when using stars, dots must be self consistent
    if( dpd->m_star && dpd->m_descriptors->begin()->m_nt != lastnt )
      error(toks,"When using '*' with a disallow group, the productions must dot their own nonterminals");
    rule->m_intermediates.push_back(dpd);
  }
  if( toks.peek() != pptok::NOTARROW )
    error(toks,"expected -/-> in disallow rule");
  toks.discard();
  pd = ParseProductions(toks,parser);
  checkProductionDescriptorsSameNonterminal(toks,pd,lastnt,"disallow rule group");
  rule->m_disallowed = pd;
  return rule;
}

static bool ParseParsePart(Tokenizer &toks, ParserDef &parser) {
  if( toks.peek() == pptok::TYPEDEF ) {
    ParseTypedefRule(toks,parser);
  } else if( toks.peek() == pptok::LEFTASSOC || toks.peek() == pptok::RIGHTASSOC || toks.peek() == pptok::NONASSOC ) {
    AssociativeRule *ar = ParseAssociativeRule(toks,parser);
    parser.m_assocrules.push_back(ar);
  } else if( toks.peek() == pptok::PRECEDENCE ) {
    PrecedenceRule *p = ParsePrecedenceRule(toks,parser);
    parser.m_precrules.push_back(p);
  } else if( toks.peek() == pptok::DISALLOW ) {
    DisallowRule *d = ParseDisallowRule(toks,parser);
    parser.m_disallowrules.push_back(d);
  } else if( toks.peek() == pptok::REJECTABLE  || toks.peek() == pptok::START || toks.peek() == pptok::ID ) {
    Vector<Production*> productions = ParseProduction(toks,parser);
    for( Vector<Production*>::iterator cur = productions.begin(), end = productions.end(); cur != end; ++cur )
      parser.addProduction(toks,*cur);
  } else
    return false;
  if( toks.peek() != pptok::SEMI )
    error(toks,"Expected ';'");
  toks.discard();
  return true;
}

static void ParseStart(Tokenizer &toks, ParserDef &parser) {
  if( ! ParseParsePart(toks,parser) )
    error(toks,"expected productin, precedence rule, or disallow rule");
  while( true ) {
    if( ! ParseParsePart(toks,parser) )
      break;
  }
}

void ParseParser(TokBuf *tokbuf, ParserDef &parser, FILE *vout, int verbosity) {
  Tokenizer toks(tokbuf,&pptok::pptokinfo);
  for( int i = 0, n = pptok::pptokinfo.m_tokenCount; i < n; ++i )
    parser.findOrAddSymbolId(toks, pptok::pptokinfo.m_tokenstr[i], SymbolTypeTerminal, -1);
  ParseStart(toks,parser);
  if( toks.peek() != -1 )
    error(toks,"Expected EOF");
  for( Map<int,SymbolDef>::iterator curdef = parser.m_tokdefs.begin(), enddef = parser.m_tokdefs.end(); curdef != enddef; ++curdef )
    if( curdef->second.m_symboltype == SymbolTypeUnknown )
      curdef->second.m_symboltype = SymbolTypeTerminal;
}

void ParserDef::print(FILE *out) const {
  Map<int,String> tokens;
  for( Map<String,int>::const_iterator tok = m_tokens.begin(); tok != m_tokens.end(); ++tok )
    tokens[tok->second] = tok->first;
  for( Vector<Production*>::const_iterator p = m_productions.begin(); p != m_productions.end(); ++p ) {
    (*p)->print(out, tokens);
    fputs(" ;\n",out);
  }
  for( Vector<PrecedenceRule*>::const_iterator r = m_precrules.begin(); r != m_precrules.end(); ++r ) {
    (*r)->print(out, tokens);
    fputs(" ;\n", out);
  }
  for( Vector<AssociativeRule*>::const_iterator r = m_assocrules.begin(); r != m_assocrules.end(); ++r ) {
    (*r)->print(out, tokens);
    fputs(" ;\n", out);
  }
  for( Vector<DisallowRule*>::const_iterator r = m_disallowrules.begin(); r != m_disallowrules.end(); ++r ) {
    (*r)->print(out, tokens);
    fputs(" ;\n", out);
  }
}

void Production::print(FILE *out, const Map<int,String> &tokens, int idx) const {
  fputs(tokens[m_nt].c_str(), out);
  fputs(" :", out);
  for( int i = 0; i < m_symbols.size(); ++i ) {
    if( idx == i )
      fputc('.',out);
    else
      fputc(' ',out);
    fputs(tokens[m_symbols[i]].c_str(),out);
  }
  if( idx == m_symbols.size() )
    fputc('.',out);
}

void PrecedenceRule::print(FILE *out, const Map<int,String> &tokens) const {
  bool first = true;
  fputs("PRECEDENCE ",out);
  for( const_iterator cur = begin(); cur != end(); ++cur ) {
    if( first )
      first = false;
    else
      fputs(" > ",out);
    (*cur)->print(out,tokens);
  }
}

bool ProductionDescriptor::dotIntersection(const ProductionDescriptor &rhs, Vector<int> &lhsdot, Vector<int> &rhsdot, Vector<int> &dots) const {
  Vector<int>::const_iterator beginlhs = m_symbols.begin(), curlhs = m_symbols.begin(), endlhs = m_symbols.end();
  Vector<int>::const_iterator beginrhs = rhs.m_symbols.begin(), currhs = rhs.m_symbols.begin(), endrhs = rhs.m_symbols.end();
  int symidx = 0;
  while( curlhs != endlhs && currhs != endrhs ) {
    if( *curlhs == pptok::DOT && *currhs == pptok::DOT ) {
      lhsdot.push_back(curlhs-beginlhs);
      rhsdot.push_back(currhs-beginrhs);
      dots.push_back(symidx);
      ++curlhs;
      ++currhs;
      ++symidx;
    }
    else if( *curlhs == pptok::DOT ) {
      ++curlhs;
    }
    else if( *currhs == pptok::DOT ) {
      ++currhs;
    } else if( *curlhs == *currhs ) {
      ++curlhs;
      ++currhs;
      ++symidx;
    } else {
      return false;
    }
  }
  return dots.size() != 0;
}

void ProductionDescriptor::insert(const Vector<int> &dots, int sym) {
  for( int i = dots.size()-1; i >= 0; --i )
    m_symbols.insert(m_symbols.begin()+dots[i],sym);
}

void ProductionDescriptor::remove(const Vector<int> &dots) {
  for( int i = dots.size()-1; i >= 0; --i )
    m_symbols.erase(m_symbols.begin()+dots[i]);
}

void ProductionDescriptors::trifrucate(ProductionDescriptors &rhs, ProductionDescriptors &overlap) {
  Vector<int> dots;
  for( iterator c = begin(); c != end(); ++c ) {
    for( iterator c2 = rhs.begin(); c != end() && c2 != rhs.end(); ++c2 ) {
      Vector<int> lhsdots, rhsdots, dots;
      c->dotIntersection(*c2,lhsdots,rhsdots,dots);
      if( ! dots.size() )
        continue;
      ProductionDescriptor pd = UnDottedProductionDescriptor(*c);
      pd.insert(dots,pptok::DOT);
      overlap.insert(pd);
      c2->remove(rhsdots);
      if( c2->countOf(pptok::DOT) == 0 ) {
        c2 = rhs.erase(c2); 
        --c2;
      }
      c->remove(lhsdots);
      if( c->countOf(pptok::DOT) == 0  ) {
        c = erase(c);
        --c;
        c2 = rhs.end()-1;
      }
    }
  }
}

void ProductionDescriptors::print(FILE *out, const Map<int,String> &tokens) const {
  for( const_iterator cur = begin(); cur != end(); ++cur )
    cur->print(out,tokens);
}

void ProductionDescriptor::print(FILE *out, const Map<int,String> &tokens) const {
  fputc('[',out);
  fputs(tokens[m_nt].c_str(), out);
  fputs(" :", out);
  for( int i = 0; i < m_symbols.size(); ++i ) {
    if( m_symbols[i] == pptok::DOT ) {
      fputc('.',out);
      ++i;
    } else {
      fputc(' ',out);
    }
    fputs(tokens[m_symbols[i]].c_str(),out);
  }
  fputc(']',out);
}

void AssociativeRule::print(FILE *out, const Map<int,String> &tokens) const {
  switch(m_assoc) {
  case AssocLeft:
    fputs("LEFT-ASSOCIATIVE ", out);
    break;
  case AssocRight:
    fputs("RIGHT-ASSOCIATIVE ", out);
    break;
  case AssocNon:
    fputs("NON-ASSOCIATIVE ", out);
    break;
  }
  m_pds->print(out,tokens);
}

void DisallowRule::consolidateRules() {
  if( m_lead )
    m_lead->consolidateRules();
  if( m_disallowed )
    m_disallowed->consolidateRules();
  for( int i = 0, n = m_intermediates.size(); i < n; ++i )
    m_intermediates[i]->m_descriptors->consolidateRules();
}

void DisallowRule::print(FILE *out, const Map<int,String> &tokens) const {
  fputs("DISALLOW ",out);
  m_lead->print(out,tokens);
  for( Vector<DisallowProductionDescriptors*>::const_iterator cur = m_intermediates.begin(); cur != m_intermediates.end(); ++cur ) {
    fputs(" --> ",out);
    (*cur)->print(out,tokens);
  }
  fputs(" -/-> ",out);
  m_disallowed->print(out,tokens);
}

void DisallowProductionDescriptors::print(FILE *out, const Map<int,String> &tokens) const {
  m_descriptors->print(out,tokens);
  if( m_star )
    fputc('*',out);
}

void state_t::print(FILE *out, const Map<int,String> &tokens) const {
  for( state_t::const_iterator curps = begin(), endps = end(); curps != endps; ++curps ) {
    curps->m_p->print(out,tokens,curps->m_idx);
    fputs("\n",out);
  
  }
}
