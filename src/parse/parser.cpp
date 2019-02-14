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

Production::Production(int symbolid, bool rejectable, int nt, const Vector<int> &symbols, String action) : m_symbolid(symbolid), m_rejectable(rejectable), m_nt(nt), m_symbols(symbols), m_action(action) {}

void error(Tokenizer &toks, const String &err) {
  throw ParserError(toks.line(),toks.col(),err);
}

void ParserDef::addProduction(Tokenizer &toks, Production *p) {
  if( p->m_nt == m_startnt ) {
    if( m_startProduction )
      error(toks,"<start> production can be defined only once");
    m_startProduction = p;
  }
  m_productions.push_back(p);
}

int ParserDef::findOrAddSymbolId(Tokenizer &toks, const String &s, SymbolType stype) {
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
      else if( symboldef.m_symboltype == SymbolTypeProduction )
        error(toks,String("Attempted to turn Production ")+symboldef.m_name+"into something else");
      symboldef.m_symboltype = stype;
    }
    return tokid;
  }
  String name(s);
  int tokid = m_nextsymbolid++;
  m_tokens[s] = tokid;
  m_tokdefs[tokid] = SymbolDef(tokid, name, stype);
  if( strcmp(name.c_str(),"<start>") == 0 )
    m_startnt = tokid;
  return tokid;
}

Vector<Production*> ParserDef::productionsAt(const ProductionState &ps) const {
  Vector<Production*> productions;
  if( ps.m_idx < ps.m_p->m_symbols.size() ) {
    int symbol = ps.m_p->m_symbols[ps.m_idx];
    if( m_tokdefs.find(symbol)->second.m_symboltype == SymbolTypeNonterminal ) {
      for( Vector<Production*>::const_iterator cur = m_productions.begin(), end = m_productions.end(); cur != end; ++cur ) {
        Production *p = *cur;
        if( p->m_nt != symbol )
          continue;
        productions.push_back(p);
      }
    }
  } else if( ps.m_idx == ps.m_p->m_symbols.size() ) {
    // TODO get the follow productions?
  }
  return productions;
}

// No rule may nest into another rule in the same group of productions, including itself.
// We'll assume the rules are valid because they were validated by the parser.
// If something truly disasterous happens, return false.
bool ParserDef::expandAssocRules(String &err) {
  while( m_assocrules.size() ) {
    AssociativeRule *rule = m_assocrules.back();
    int nt = (*rule->m_p->begin())->m_nt;
    DisallowRule *dr = new DisallowRule();
    dr->m_disallowed = rule->m_p->clone();
    dr->m_lead = rule->m_p->clone();
    for( ProductionDescriptors::iterator cur = dr->m_lead->begin(), end = dr->m_lead->end(); cur != end; ++cur ) {
      ProductionDescriptor *pd = *cur;
      int lastidx = -1;
      int idx = -1;
      for( int i = 0; i < pd->m_symbols.size(); ++i ) {
        if( pd->m_symbols[i] != nt )
          continue;
        if( rule->m_assoc == AssocLeft ) {
          if( lastidx != -1 )
            idx = i;
        } else if( rule->m_assoc == AssocRight ) {
          idx = lastidx;
        } else if( rule->m_assoc == AssocNon ) {
          idx = i;
        }
        lastidx = i;
        if( idx != -1 ) {
          pd->m_symbols.insert(pd->m_symbols.begin()+idx,pptok::DOT);
          i++;
        }
      }
    }
    m_disallowrules.push_back(dr);
    delete rule;
    m_assocrules.pop_back();
  }
  return true;
}

// No rule may appear in a rule of higher precedence.
// We'll assume the rules are valid because they were validated by the parser.
// If something truly disasterous happens, return false.
bool ParserDef::expandPrecRules(String &err) {
  Vector<ProductionDescriptor*> descriptors;
  while( m_precrules.size() ) {
    PrecedenceRule *pr = m_precrules.back();
    int nt = (*(*pr->begin())->begin())->m_nt;
    descriptors.clear();
    for( PrecedenceRule::iterator cur = pr->begin()+(pr->size()-1), begin = pr->begin(), last = pr->begin()+(pr->size()-1); cur >= begin; --cur ) {
      if( cur != last ) {
        DisallowRule *dr = new DisallowRule();
        dr->m_lead = new ProductionDescriptors();
        for( ProductionDescriptors::iterator curdesc = (*cur)->begin(), enddesc = (*cur)->end(); curdesc != enddesc; ++curdesc ) {
          ProductionDescriptor *pd = new ProductionDescriptor();
          pd->m_nt = (*curdesc)->m_nt;
          for( Vector<int>::iterator cursym = (*curdesc)->m_symbols.begin(), endsym = (*curdesc)->m_symbols.end(); cursym != endsym; ++cursym ) {
            int s = *cursym;
            if( s == nt )
              pd->m_symbols.push_back(pptok::DOT);
            pd->m_symbols.push_back(s);
          }
          dr->m_lead->push_back(pd);
        }
        dr->m_lead->consolidateRules();
        dr->m_disallowed = new ProductionDescriptors();
        for( Vector<ProductionDescriptor*>::iterator curdesc = (*cur)->begin(), enddesc = (*cur)->end(); curdesc != enddesc; ++curdesc )
          dr->m_disallowed->push_back((*curdesc)->clone());
        dr->m_disallowed->consolidateRules();
        m_disallowrules.push_back(dr);
      }
      if( cur != begin )
        descriptors.insert(descriptors.end(),(*cur)->begin(), (*cur)->end());
    }
    m_precrules.pop_back();
  }
  return true;
}

// Combine whatever rules can be combined.
// If something truly disasterous happens, return false.
bool ParserDef::combineRules(String &err) {
  for( int i = 0; i < m_disallowrules.size(); ++i ) {
    DisallowRule *lhs = m_disallowrules[i];
    for( int j = i+1; j < m_disallowrules.size(); ++j ) {
      DisallowRule *rhs = m_disallowrules[j];
      if( ! lhs->combineWith(*rhs) )
        continue;
      delete rhs;
      m_disallowrules.erase(m_disallowrules.begin()+j);
      --j;
    }
  }
  return true;
}

bool ProductionDescriptor::matchesProduction(const Production *p) const {
  // '.' is ignored for matching purposes.
  if( m_nt != p->m_nt )
    return false;
  Vector<int>::const_iterator curs = p->m_symbols.begin(), ends = p->m_symbols.end(), curwc = m_symbols.begin(), endwc = m_symbols.end();
  while( curs != ends && curwc != endwc ) {
    if( *curwc == pptok::DOT )
      ++curwc;
    if( *curs != *curwc )
      return false;
    ++curs;
    ++curwc;
  }
  if( curs != ends || curwc != endwc )
    return false;
  return true;
}

static int ParseNonterminal(Tokenizer &toks, ParserDef &parser) {
  if( toks.peek() == pptok::START || toks.peek() == pptok::ID ) {
    String s = toks.tokstr();
    toks.discard();
    return parser.findOrAddSymbolId(toks, s, SymbolTypeNonterminal);
  }
  return -1;
}

static int ParseSymbol(Tokenizer &toks, ParserDef &parser) {
  if( toks.peek() == pptok::ID ) {
    String s= toks.tokstr();
    toks.discard();
    return parser.findOrAddSymbolId(toks, s, SymbolTypeUnknown);
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
    int productionId = parser.findOrAddSymbolId(toks,productionName,SymbolTypeProduction);
    Production *p = new Production(productionId,rejectable,nt,symbols,action);
    parser.m_tokdefs[productionId].m_p = p;
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
  ProductionDescriptors *pd = new ProductionDescriptors();
  ProductionDescriptor *descriptor = 0;
  while( (descriptor = ParseProductionDescriptor(toks,parser)) != 0 )
    pd->push_back(descriptor);
  pd->consolidateRules();
  return pd;
}

static ProductionDescriptors *ParseProductions(Tokenizer &toks, ParserDef &parser);

static int cmpdesc(const void *vplhs, const void *vprhs) {
  const ProductionDescriptor *lhs = (const ProductionDescriptor*)vplhs, *rhs = (const ProductionDescriptor*)vprhs;
  if( *lhs < *rhs )
     return -1;
  else if( *rhs < *lhs )
     return 1;
  return 0;
}

void ProductionDescriptors::consolidateRules() {
  for( int ilhs = 0; ilhs < m_descriptors.size(); ++ilhs ) {
    for( int irhs = ilhs+1; irhs < m_descriptors.size(); ++ilhs ) {
      if( m_descriptors[ilhs]->addDotsFrom(*m_descriptors[irhs]) ) {
        delete m_descriptors[irhs];
        m_descriptors.erase(m_descriptors.begin()+irhs);
        --irhs;
      }
    }
  }
  // Also sort them
  qsort(m_descriptors.begin(),m_descriptors.size(),sizeof(ProductionDescriptor*),cmpdesc);
}

ProductionDescriptors *ProductionDescriptors::clone() const {
  ProductionDescriptors *ret = new ProductionDescriptors();
  ret->m_descriptors.resize(m_descriptors.size());
  for( int i = 0, n = m_descriptors.size(); i < n; ++i )
    ret->m_descriptors[i] = m_descriptors[i]->clone();
  return ret;
}

static void checkProductionDescriptorsSameNonterminal(Tokenizer &toks, ProductionDescriptors *p, int lastnt, const char *name) {
  // Check all parts have the same nonterminal.
  int nt = (*p->begin())->m_nt;
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
    ProductionDescriptor *pd = *cur;
    if( nt != pd->m_nt ) {
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
    ProductionDescriptor *pd = *cur;
    // Check that all dotted nonterminals are the same (and that there is at least 1)
    int dotcnt = 0;
    for( Vector<int>::iterator cursym = pd->m_symbols.begin(), endsym = pd->m_symbols.end(); cursym != endsym; ++cursym ) {
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
  ProductionDescriptors *part = ParseProductionDescriptors(toks,parser);
  checkProductionDescriptorsSameNonterminalAndDotsAgree(toks,part,-1,"precedence rule group");
  while( toks.peek() == pptok::GT ) {
    toks.discard();
    ProductionDescriptors *part = ParseProductionDescriptors(toks,parser);
    checkProductionDescriptorsSameNonterminalAndDotsAgree(toks,part,-1,"precedence rule group");
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
    ret->push_back( new ProductionDescriptor(nt,symbols) );
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
  if( dpd->m_star && (*dpd->m_descriptors->begin())->m_nt != lastnt )
    error(toks,"When using '*' with a disallow group, the productions must dot their own nonterminals");
  rule->m_intermediates.push_back(dpd);
  while( toks.peek() == pptok::ARROW ) {
    toks.discard();
    dpd = ParseDisallowProductionDescriptors(toks,parser);
    lastnt = checkProductionDescriptorsSameNonterminalAndDotsAgree(toks,dpd->m_descriptors,lastnt,"disallow rule group");
    // when using stars, dots must be self consistent
    if( dpd->m_star && (*dpd->m_descriptors->begin())->m_nt != lastnt )
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

void ParseParser(TokBuf *tokbuf, ParserDef &parser) {
  Tokenizer toks(tokbuf,&pptok::pptokinfo);
  for( int i = 0, n = pptok::pptokinfo.m_tokenCount; i < n; ++i )
    parser.findOrAddSymbolId(toks, pptok::pptokinfo.m_tokenstr[i], SymbolTypeTerminal);
  ParseStart(toks,parser);
  if( toks.peek() != -1 )
    error(toks,"Expected EOF");
}

// Better think over this definition
bool DisallowRule::canCombineWith(const DisallowRule &rhs) const {
  if( m_intermediates.size() != rhs.m_intermediates.size() )
    return false;
  for( int i = 0, n = m_intermediates.size(); i < n; ++i )
    if( *m_intermediates[i] != *rhs.m_intermediates[i] )
      return false;
  return (*m_lead == *rhs.m_lead) || (*m_disallowed == *rhs.m_disallowed);
}

bool DisallowRule::combineWith(const DisallowRule &rhs) {
  if( m_intermediates.size() != rhs.m_intermediates.size() )
    return false;
  for( int i = 0, n = m_intermediates.size(); i < n; ++i )
    if( *m_intermediates[i] != *rhs.m_intermediates[i] )
      return false;
  if( *m_lead == *rhs.m_lead ) {
    if( *m_disallowed == *rhs.m_disallowed ) {
      m_disallowed->m_descriptors.insert(m_disallowed->m_descriptors.end(), rhs.m_disallowed->m_descriptors.begin(), rhs.m_disallowed->m_descriptors.end());
      m_disallowed->consolidateRules();
    }
    return true;
  } else if( *m_disallowed == *rhs.m_disallowed ) {
    if( *m_lead != *rhs.m_lead ) {
      m_lead->m_descriptors.insert(m_lead->m_descriptors.end(), rhs.m_lead->m_descriptors.begin(), rhs.m_lead->m_descriptors.end());
      m_lead->consolidateRules();
    }
    return true;
  }
  return false;
}
