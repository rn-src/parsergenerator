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

ProductionDescriptor::ProductionDescriptor(int nt, const Vector<int> &symbols, int dots, int dotcnt) : m_nt(nt), m_symbols(symbols), m_dots(dots), m_dotcnt(dotcnt) {}

ProductionDescriptor::ProductionDescriptor(int nt, const Vector<int> &symbols) : m_nt(nt), m_dots(0), m_dotcnt(0)
{
	int j = 0;
    for( int i = 0, n = symbols.size(); i < n; ++i ) {
		int sym = symbols[i];
		if( sym == MARKER_DOT ) {
			if( (m_dots&(1<<j)) == 0 ) {
			  m_dots |= (1<<j);
			  ++m_dotcnt;
			}
		} else {
			m_symbols.push_back(sym);
			++j;
		}
    }
}

bool ProductionDescriptor::hasSameProductionAs(const ProductionDescriptor &rhs) const {
  return m_nt == rhs.m_nt && m_symbols == rhs.m_symbols;
}

int ProductionDescriptor::appearancesOf(int sym, int &at) const {
	if( sym == MARKER_DOT ) {
		at = m_dots;
		return m_dotcnt;
	}
	at = 0;
	int cnt = 0;
	for( int i = 0, n = m_symbols.size(); i < n; ++i ) {
		if( m_symbols[i] == sym ) {
		at |= (1<<i);
		++cnt;
		}
	}
	return cnt;
}

bool ProductionDescriptor::hasDotAt(int pos) const {
	return (m_dots&(1<<pos)) != 0;
}

bool ProductionDescriptor::addDotsFrom(const ProductionDescriptor &rhs) {
  if( ! hasSameProductionAs(rhs) )
    return false;
  addDots(rhs.m_dots);
  return true;
}

void ProductionDescriptor::addDots(int dots) {
  m_dots |= dots;
  m_dotcnt = 0;
  for( int i = 0, n = m_symbols.size(); i < n; ++i )
	  if( (m_dots & (1<<i)) != 0 )
		  ++m_dotcnt;
}

void ProductionDescriptor::removeDots(int dots) {
  m_dots &= ~dots;
  m_dotcnt = 0;
  for( int i = 0, n = m_symbols.size(); i < n; ++i )
	  if( (m_dots & (1<<i)) != 0 )
		  ++m_dotcnt;
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

Production::Production(bool rejectable, int nt, const Vector<int> &symbols, const Vector<ActionItem> &action, int lineno, String filename) : m_rejectable(rejectable), m_nt(nt), m_symbols(symbols), m_action(action), m_pid(-1), m_lineno(lineno), m_filename(filename) {}

Production *Production::clone() {
  return new Production(m_rejectable, m_nt, m_symbols, m_action, m_lineno, m_filename);
}

static void error(Tokenizer &toks, const String &err) {
  throw ParserErrorWithLineCol(toks.line(),toks.col(),toks.filename(),err);
}

void ParserDef::addProduction(Tokenizer &toks, Production *p) {
  if( p->m_nt == getStartNt() ) {
    if( m_startProduction )
      error(toks,"START production can be defined only once");
    m_startProduction = p;
  }
  m_productions.push_back(p);
  String s;
  s += "[";
  s += m_tokdefs[p->m_nt].m_name;
  s += " :";
  for( int i = 0; i < p->m_symbols.size(); ++i ) {
    s += " ";
    s += m_tokdefs[p->m_symbols[i]].m_name;
  }
  s += "]";
  p->m_pid = findOrAddSymbolId(toks,s,SymbolTypeNonterminalProduction);
  m_tokdefs[p->m_pid].m_p = p;
}

bool ParserDef::addProduction(Production *p) {
  if( p->m_nt == getStartNt() ) {
    if( m_startProduction )
      return false;
    m_startProduction = p;
  }
  m_productions.push_back(p);
  String s;
  s += "[";
  s += m_tokdefs[p->m_nt].m_name;
  s += " :";
  for( int i = 0; i < p->m_symbols.size(); ++i ) {
    s += " ";
    s += m_tokdefs[p->m_symbols[i]].m_name;
  }
  s += "]";
  p->m_pid = addSymbolId(s,SymbolTypeNonterminalProduction);
  m_tokdefs[p->m_pid].m_p = p;
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

int ParserDef::addSymbolId(const String &s, SymbolType stype) {
  Map<String,int>::iterator iter = m_tokens.find(s);
  if( iter != m_tokens.end() )
    return -1;
  String name(s);
  int tokid = m_nextsymbolid++;
  m_tokens[s] = tokid;
  m_tokdefs[tokid] = SymbolDef(tokid, name, stype);
  return tokid;
 } 

static const char *getSymbolTypeName(SymbolType stype) {
  switch( stype ){
  case SymbolTypeTerminal:
    return "Terminal";
  case SymbolTypeNonterminal:
    return "Nonterminal";
  case SymbolTypeNonterminalProduction:
    return "Nonterminal Production";
  default:
    break;
  }
  return "?";
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
      error(toks,String("Attempted to turn ")+getSymbolTypeName(symboldef.m_symboltype)+" "+symboldef.m_name+" into a "+getSymbolTypeName(stype));
    }
    return tokid;
  }
  String name(s);
  int tokid = m_nextsymbolid++;
  // Don't want that one.
  if( tokid == MARKER_DOT )
    ++tokid;
  m_tokens[s] = tokid;
  m_tokdefs[tokid] = SymbolDef(tokid, name, stype);
  return tokid;
}

int ParserDef::getStartNt() const {
  return 0;
}

int ParserDef::getExtraNt() const {
  return 1;
}

Vector<productionandforbidstate_t> ParserDef::productionsAt(const Production *p, int pos, int forbidstate) const {
  Vector<productionandforbidstate_t> productions;
  const Vector<Production*> *pproductions = &m_productions;
  if( pos < 0 || pos >= p->m_symbols.size() )
    return productions;
  int symbol = p->m_symbols[pos];
  if( getSymbolType(symbol) != SymbolTypeNonterminal )
    return productions;
  if( m_verbosity > 2 ) {
    p->print(m_vout,m_tokdefs,pos,forbidstate);
    fputs(" has...\n", m_vout);
  }
  for( Vector<Production*>::const_iterator cur = pproductions->begin(), end = pproductions->end(); cur != end; ++cur ) {
    Production *ptest = *cur;
    if( ptest->m_nt != symbol )
      continue;
    int nxtState = m_forbid.nextState(p,pos,forbidstate,ptest);
    if( nxtState == -1 ) {
      if( m_verbosity > 2 ) {
        fputs("  -/-> ", m_vout);
      }
    } else {
      if( m_verbosity > 2 ) {
        fputs("   --> ", m_vout);
      }
      productions.push_back(productionandforbidstate_t(ptest,nxtState));
    }
    if( m_verbosity > 2 ) {
      ptest->print(m_vout,m_tokdefs);
      fputs("\n", m_vout);
    }
  }
  return productions;
}

ProductionDescriptor ProductionDescriptor::UnDottedProductionDescriptor() const {
  return ProductionDescriptor(m_nt, m_symbols);
}

ProductionDescriptors *ProductionDescriptors::UnDottedProductionDescriptors() const {
  ProductionDescriptors *pdsundotted = new ProductionDescriptors();
  for( ProductionDescriptors::const_iterator cur = begin(), enditer = end(); cur != enditer; ++cur ) {
    ProductionDescriptor pd = cur->UnDottedProductionDescriptor();
    pdsundotted->insert(pd);
  }
  return pdsundotted;
}

ProductionDescriptor ProductionDescriptor::DottedProductionDescriptor(int nt, Assoc assoc) const {
  int ntat=0, ntcnt=0;
  int dots=0, dotcnt=0;
  ntcnt = appearancesOf(nt,ntat);
  if( assoc == AssocNon ) {
	  dots = ntat;
	  dotcnt = ntcnt;
  } else if( assoc == AssocLeft ) {
	  if( ntcnt ) {
		  int firstnt = 0;
		  while( (ntat&(1<<firstnt)) == 0 )
			  ++firstnt;
		  dots = ntat&(~(1<<firstnt));
		  dotcnt = ntcnt-1;
	  }
  } else if( assoc == AssocRight ) {
	  if( ntcnt ) {
		  int lastnt = 0;
		  while( (ntat>>lastnt) != 1 )
			  ++lastnt;
		  dots = ntat&(~(1<<lastnt));
		  dotcnt = ntcnt-1;
	  }
  }
  return ProductionDescriptor(m_nt,m_symbols,dots,dotcnt);
}

Vector<int> ProductionDescriptor::symbolsAtDots() const {
	Vector<int> symbols;
	for( int i = 0, n = m_symbols.size(); i < n; ++i ) {
		if( hasDotAt(i) )
			symbols.push_back(m_symbols[i]);
	}
	return symbols;
}

ProductionDescriptors *ProductionDescriptors::DottedProductionDescriptors(int nt, Assoc assoc) const {
  ProductionDescriptors *pdsdotted = new ProductionDescriptors();
  for( ProductionDescriptors::const_iterator cur = begin(), enditer = end(); cur != enditer; ++cur ) {
    ProductionDescriptor pd = cur->DottedProductionDescriptor(nt,assoc);
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
    addDisallowRule(dr);
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
        addDisallowRule(dr);
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
  static int combo = 1;
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
      if( m_verbosity > 2 ) {
        fprintf(m_vout, "combining (# %d)\n", combo++);
        lhs->print(m_vout,m_tokdefs);
        fputs(" and\n", m_vout);
        rhs->print(m_vout,m_tokdefs);
        fputs("\n", m_vout);
      }
      lhs->m_lead->trifrucate(*rhs->m_lead,overlap);
      if( overlap.size() ) {
        DisallowRule *dr = new DisallowRule();
        dr->m_lead = overlap.clone();
        dr->m_disallowed = lhs->m_disallowed->clone();
        dr->m_disallowed->insert(rhs->m_disallowed->begin(),rhs->m_disallowed->end());
        dr->m_disallowed->consolidateRules();
        addDisallowRule(dr);
      }
      if( rhs->m_lead->size() == 0 ) {
        m_disallowrules.erase(m_disallowrules.begin()+j);
        --j;
      } else {
        if( m_verbosity > 2 ) {
          fputs("retained\n", m_vout);
          rhs->print(m_vout,m_tokdefs);
          fputs("\n", m_vout);
        }
      }
      if( lhs->m_lead->size() == 0 ) {
        m_disallowrules.erase(m_disallowrules.begin()+i);
        --i;
        j = m_disallowrules.size()-1;
      } else {
        if( m_verbosity > 2 ) {
          fputs("retained\n", m_vout);
          lhs->print(m_vout,m_tokdefs);
          fputs("\n", m_vout);
        }
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

bool ProductionDescriptor::matchesProduction(const Production *p) const {
  // '.' is ignored for matching purposes.
  return m_nt == p->m_nt && m_symbols == p->m_symbols;
}

bool ProductionDescriptor::matchesProductionAndPosition(const Production *p, int pos) const {
  return matchesProduction(p) && hasDotAt(pos);
}

static int ParseNonterminal(Tokenizer &toks, ParserDef &parser) {
  if( toks.peek() == pptok::START ) {
    toks.discard();
    return parser.getStartNt();
  } else if( toks.peek() == pptok::ID ) {
    String s = toks.tokstr();
    toks.discard();
    return parser.findOrAddSymbolId(toks, s, SymbolTypeNonterminal);
  }
  return -1;
}

static int ParseNonterminalOrExtra(Tokenizer &toks, ParserDef &parser) {
  if( toks.peek() == pptok::START ) {
    toks.discard();
    return parser.getStartNt();
  } else if( toks.peek() == pptok::EXTRA ) {
    String s = toks.tokstr();
    toks.discard();
    return parser.getExtraNt();
  } else if( toks.peek() == pptok::ID ) {
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

static void ParseAction(Tokenizer &toks, ParserDef &parser, Vector<ActionItem> &action) {
  if( toks.peek() != pptok::LBRACE )
    return;
  else {
    ActionItem item;
    item.m_actiontype = ActionTypeSrc;
    item.m_str = toks.tokstr();
    action.push_back(item);
    toks.discard();
  }
  while( toks.peek() != -1 && toks.peek() != pptok::RBRACE ) {
    if( toks.peek() == pptok::LBRACE )
      ParseAction(toks,parser,action);
    if( toks.peek() == pptok::DOLLARDOLLAR ) {
      ActionItem item;
      item.m_actiontype = ActionTypeDollarDollar;
      action.push_back(item);
    } else if( toks.peek() == pptok::DOLLARNUMBER ) {
      ActionItem item;
      item.m_actiontype = ActionTypeDollarNumber;
      item.m_dollarnum = atoi(toks.tokstr()+1);
      action.push_back(item);
    } else {
      ActionItem item;
      item.m_actiontype = ActionTypeSrc;
      item.m_str = toks.tokstr();
      action.push_back(item);
    }
    toks.discard();
  }
  if( toks.peek() != pptok::RBRACE )
    error(toks,"Missing '}'");
  else {
    ActionItem item;
    item.m_actiontype = ActionTypeSrc;
    item.m_str = toks.tokstr();
    action.push_back(item);
    toks.discard();
  }
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
    error(toks,"Expected ':'");
  toks.discard();
  Vector<Production*> productions;
  while(true) {
    Vector<int> symbols = ParseSymbols(toks,parser);
    Vector<ActionItem> action;
    int lineno = toks.line();
    String filename = toks.filename();
    if( toks.peek() == pptok::LBRACE )
      ParseAction(toks,parser,action);
    char buf[64];
    sprintf(buf,"Production-%d",parser.m_productions.size()+1);
    String productionName = buf;
    Production *p = new Production(rejectable,nt,symbols,action,lineno,filename);
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
      symbols.push_back(MARKER_DOT);
    symbols.push_back(symbol);
    symbol = ParseSymbolOrDottedSymbol(toks,parser,dotted);
  }
  if( toks.peek() != pptok::RBRKT )
    error(toks,"Expected ']' to end production descriptor");
  toks.discard();
  return new ProductionDescriptor(nt, symbols);
}

static void ParseTypedefRule(Tokenizer &toks, ParserDef &parser) {
  if( toks.peek() != pptok::TYPEDEFTOK )
    return;
  toks.discard();
  if( toks.peek() != pptok::ID )
    error(toks,"expected id after typedef");
  String toktype = toks.tokstr();
  toks.discard();
  while( toks.peek() == pptok::STAR ) {
    toktype += toks.tokstr();
    toks.discard();
  }
  int nt = ParseNonterminalOrExtra(toks,parser);
  while( nt != -1 ) {
    SymbolDef &symboldef = parser.m_tokdefs[nt];
    if( symboldef.m_symboltype != SymbolTypeNonterminal )
      error(toks, String("TYPEDEF may only be applied to nonterminals, attempted to TYPEDEF ")+symboldef.m_name);
    if( symboldef.m_semantictype.length() != 0 )
      error(toks, String("TYPEDEF can only be assigned once per type.  Attempated to reassign TYPEDEF of ")+symboldef.m_name);
    parser.m_tokdefs[nt].m_semantictype = toktype;
    nt = ParseNonterminalOrExtra(toks,parser);
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
	Vector<int> symbolsAtDots = pd.symbolsAtDots();
	int dotcnt = symbolsAtDots.size();
    for( Vector<int>::const_iterator cursym = symbolsAtDots.begin(), endsym = symbolsAtDots.end(); cursym != endsym; ++cursym ) {
		if( dottednt == -1 )
			dottednt = *cursym;
		else if( *cursym != dottednt ) {
			String err = "All dots in a ";
			err += name;
			err += " must have the same nonterminal";
			error(toks,err);
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
  DisallowProductionDescriptors *dpd = 0;
  while( toks.peek() == pptok::ARROW ) {
    toks.discard();
    dpd = ParseDisallowProductionDescriptors(toks,parser);
    lastnt = checkProductionDescriptorsSameNonterminalAndDotsAgree(toks,dpd->m_descriptors,lastnt,"disallow rule group");
    // when using stars, dots must be self consistent
    if (dpd->m_star) {
      if (dpd->m_descriptors->begin()->m_nt != lastnt)
        error(toks,"When using '*' with a disallow group, the productions must dot their own nonterminals");
    }
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
  if( toks.peek() == pptok::TYPEDEFTOK ) {
    ParseTypedefRule(toks,parser);
  } else if( toks.peek() == pptok::LEFTASSOC || toks.peek() == pptok::RIGHTASSOC || toks.peek() == pptok::NONASSOC ) {
    AssociativeRule *ar = ParseAssociativeRule(toks,parser);
    parser.m_assocrules.push_back(ar);
  } else if( toks.peek() == pptok::PRECEDENCE ) {
    PrecedenceRule *p = ParsePrecedenceRule(toks,parser);
    parser.m_precrules.push_back(p);
  } else if( toks.peek() == pptok::DISALLOW ) {
    DisallowRule *d = ParseDisallowRule(toks,parser);
    parser.addDisallowRule(d);
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
    error(toks,"expected production, precedence rule, or disallow rule");
  while( true ) {
    if( ! ParseParsePart(toks,parser) )
      break;
  }
}

void ParseParser(TokBuf *tokbuf, ParserDef &parser, FILE *vout, int verbosity) {
  TokBufTokenizer toks(tokbuf,&pptok::pptokinfo);
  parser.findOrAddSymbolId(toks,"START",SymbolTypeNonterminal);
  parser.findOrAddSymbolId(toks,"EXTRA",SymbolTypeNonterminal);
  ParseStart(toks,parser);
  if( toks.peek() != -1 )
    error(toks,"Expected EOF");
  for( Map<int,SymbolDef>::iterator curdef = parser.m_tokdefs.begin(), enddef = parser.m_tokdefs.end(); curdef != enddef; ++curdef )
    if( curdef->second.m_symboltype == SymbolTypeUnknown )
      curdef->second.m_symboltype = SymbolTypeTerminal;
}

void ParserDef::print(FILE *out) const {
  for( Vector<Production*>::const_iterator p = m_productions.begin(); p != m_productions.end(); ++p ) {
    (*p)->print(out, m_tokdefs);
    fputs(" ;\n",out);
  }
  for( Vector<PrecedenceRule*>::const_iterator r = m_precrules.begin(); r != m_precrules.end(); ++r ) {
    (*r)->print(out, m_tokdefs);
    fputs(" ;\n", out);
  }
  for( Vector<AssociativeRule*>::const_iterator r = m_assocrules.begin(); r != m_assocrules.end(); ++r ) {
    (*r)->print(out, m_tokdefs);
    fputs(" ;\n", out);
  }
  for( Vector<DisallowRule*>::const_iterator r = m_disallowrules.begin(); r != m_disallowrules.end(); ++r ) {
    (*r)->print(out, m_tokdefs);
    fputs(" ;\n", out);
  }
}

void Production::print(FILE *out, const Map<int,SymbolDef> &tokens, int pos, int forbidstate) const {
  fputs(tokens[m_nt].m_name.c_str(), out);
  if( forbidstate != 0 )
    fprintf(out, "{%d}", forbidstate);
  fputs(" :", out);
  for( int i = 0; i < m_symbols.size(); ++i ) {
    if( pos == i )
      fputc('.',out);
    else
      fputc(' ',out);
    fputs(tokens[m_symbols[i]].m_name.c_str(),out);
  }
  if( pos == m_symbols.size() )
    fputc('.',out);
}

void PrecedenceRule::print(FILE *out, const Map<int,SymbolDef> &tokens) const {
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

void ProductionDescriptors::trifrucate(ProductionDescriptors &rhs, ProductionDescriptors &overlap) {
  for( iterator c = begin(); c != end(); ++c ) {
    for( iterator c2 = rhs.begin(); c != end() && c2 != rhs.end(); ++c2 ) {
	  if( ! c->hasSameProductionAs(*c2) )
	    continue;
	  // Find the overlap, remove the overlap from lhs and rhs, remove lhs or rhs if they are empty.
      int lhsdots=0,lhsdotcnt=0,rhsdots=0,rhsdotcnt=0;
	  int dots=0;
	  lhsdotcnt = c->appearancesOf(MARKER_DOT,lhsdots);
	  rhsdotcnt = c2->appearancesOf(MARKER_DOT,rhsdots);
	  dots = lhsdots&rhsdots;
      if( dots == 0 )
        continue;
      ProductionDescriptor pd = c->UnDottedProductionDescriptor();
	  pd.addDots(dots);
      overlap.insert(pd);
      c2->removeDots(dots);
	  if( c2->appearancesOf(MARKER_DOT,rhsdots) == 0 ) {
        c2 = rhs.erase(c2); 
        --c2;
      }
      c->removeDots(dots);
      if( c->appearancesOf(MARKER_DOT,lhsdots) == 0 ) {
        c = erase(c);
        --c;
        c2 = rhs.end()-1;
      }
    }
  }
}

void ProductionDescriptors::print(FILE *out, const Map<int,SymbolDef> &tokens) const {
  for( const_iterator cur = begin(); cur != end(); ++cur )
    cur->print(out,tokens);
}

void ProductionDescriptor::print(FILE *out, const Map<int,SymbolDef> &tokens) const {
  fputc('[',out);
  fputs(tokens[m_nt].m_name.c_str(), out);
  fputs(" :", out);
  for( int i = 0; i < m_symbols.size(); ++i ) {
    if( hasDotAt(i) ) {
      fputc('.',out);
    } else {
      fputc(' ',out);
    }
    fputs(tokens[m_symbols[i]].m_name.c_str(),out);
  }
  fputc(']',out);
}

void AssociativeRule::print(FILE *out, const Map<int,SymbolDef> &tokens) const {
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

void DisallowRule::print(FILE *out, const Map<int,SymbolDef> &tokens) const {
  fputs("DISALLOW ",out);
  m_lead->print(out,tokens);
  for( Vector<DisallowProductionDescriptors*>::const_iterator cur = m_intermediates.begin(); cur != m_intermediates.end(); ++cur ) {
    fputs(" --> ",out);
    (*cur)->print(out,tokens);
  }
  fputs(" -/-> ",out);
  m_disallowed->print(out,tokens);
}

void DisallowProductionDescriptors::print(FILE *out, const Map<int,SymbolDef> &tokens) const {
  m_descriptors->print(out,tokens);
  if( m_star )
    fputc('*',out);
}

void state_t::print(FILE *out, const Map<int,SymbolDef> &tokens) const {
  for( state_t::const_iterator curps = begin(), endps = end(); curps != endps; ++curps ) {
    curps->m_p->print(out,tokens,curps->m_pos,curps->m_forbidstate);
    fputs("\n",out);
  
  }
}

void ParserDef::addDisallowRule(DisallowRule *d) {
  static int i = 1;
  m_disallowrules.push_back(d);
  if( m_verbosity > 2 ) {
    fprintf(m_vout, "added (add # %d)\n", i++);
    d->print(m_vout,m_tokdefs);
    fputs("\n", m_vout);
  }
}