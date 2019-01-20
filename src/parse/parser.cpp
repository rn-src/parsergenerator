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
// parsepart : (typedefrule|production|precedencerule|disallowrule) SEMI
// typedefrule : TYPEDEF ID nonterminal+
// productiondecl : REJECTABLE? production action?
// production: nonterminal COLON symbol+ (VERTICAL symbol+)+
// nonterminal : START | ID
// symbol : ID | ERROR
// precedencerule : PRECEDENCE precedenceparts
// precedenceparts : precedencepart [['='|'>'] precedencepart]+
// precedencepart : productiondescriptor associativity?
// associativity LEFTASSOC | RIGHTASSOC | NONASSOC | DISALLOW '@' productiondescriptor
// productiondescriptor : '[' (nonterminal|'*') COLON (symbol|'*'|'...'|'.' symbol)+ ']'
// disallowrule : DISALLOW productiondescriptor ('@' productiondescriptor)+ ('@' '...' productiondescriptor)?

ProductionDescriptor::ProductionDescriptor(int nt, Vector<int> symbols) : m_nt(nt), m_symbols(symbols) {}

Production::Production(bool rejectable, int nt, const Vector<int> &symbols, String action) : m_rejectable(rejectable), m_nt(nt), m_symbols(symbols), m_action(action) {}

void error(Tokenizer &toks, const String &err) {
  throw ParserError(toks.line(),toks.col(),err);
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
      symboldef.m_symboltype = stype;
    }
    return tokid;
  }
  String name(s);
  int tokid = m_nextsymbolid++;
  m_tokens[s] = tokid;
  m_tokdefs[tokid] = SymbolDef(tokid, name, stype);
  return tokid;
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

static Vector<Production*> ParseProductions(Tokenizer &toks, ParserDef &parser) {
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
    productions.push_back(new Production(rejectable,nt,symbols,action));
    if( toks.peek() != pptok::VSEP )
      break;
    toks.discard();
  }
  return productions;
}

static int ParseNtOrStar(Tokenizer &toks, ParserDef &parser) {
  int nt = -1;
  if( toks.peek() == pptok::STAR ) {
    nt = toks.peek();
    toks.discard();
  } else
    nt = ParseNonterminal(toks,parser);
  return nt;
}

static int ParseSymbolOrDottedSymbolOrWildcard(Tokenizer &toks, ParserDef &parser, bool  &dotted) {
  int symbol = -1;
  dotted = false;
  if( toks.peek() == pptok::STAR || toks.peek() == pptok::ELIPSIS ) {
    symbol = toks.peek();
    toks.discard();
  } else if(  toks.peek() == pptok::DOT ) {
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
  int nt = ParseNtOrStar(toks,parser);
  if( nt == -1 )
    error(toks,"expected nonterminal or '*'");
  if( toks.peek() != pptok::COLON )
    error(toks,"Expected ':' in production descriptor");
  toks.discard();
  bool dotted = false;
  int symbol = ParseSymbolOrDottedSymbolOrWildcard(toks,parser,dotted);
  if( symbol == -1 )
    error(toks,"expected symbol, '*', or '...'");
  Vector<int> symbols;
  while( symbol != -1 ) {
    if( dotted )
      symbols.push_back(pptok::DOT);
    symbols.push_back(symbol);
    symbol = ParseSymbolOrDottedSymbolOrWildcard(toks,parser,dotted);
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

static PrecedencePart *ParsePrecedencePart(Tokenizer &toks, ParserDef &parser) {
  PrecedencePart *prec = new PrecedencePart();
  ProductionDescriptor *descriptor = ParseProductionDescriptor(toks,parser);
  if( toks.peek() == pptok::LEFTASSOC ) {
    prec->m_assoc = AssocLeft;
    toks.discard();
  } else if( toks.peek() == pptok::RIGHTASSOC ) {
    prec->m_assoc = AssocRight;
    toks.discard();
  } else if( toks.peek() == pptok::NONASSOC ) {
    prec->m_assoc = AssocNon;
    toks.discard();
  } else if( toks.peek() == pptok::DISALLOW ) {
    prec->m_assoc = AssocByDisallow;
    toks.discard();
    if( toks.peek() != pptok::AT )
      error(toks,"Expected @ after DISALLOW");
    prec->m_disallowed = ParseProductionDescriptor(toks,parser);
  } else
    prec->m_assoc = AssocNull;
  return prec;
}

static PrecedenceRule *ParsePrecedenceRule(Tokenizer &toks, ParserDef &parser) {
  if( toks.peek() != pptok::PRECEDENCE )
    return 0;
  toks.discard();
  int localprecedence = 0;
  PrecedenceRule *rule = new PrecedenceRule();
  PrecedencePart *part = ParsePrecedencePart(toks,parser);
  while( toks.peek() == pptok::GT || toks.peek() == pptok::EQ ) {
    if( toks.peek() == pptok::GT )
      localprecedence++;
    toks.discard();
    PrecedencePart *part = ParsePrecedencePart(toks,parser);
    part->m_localprecedence = localprecedence;
    rule->m_parts.push_back(part);
  }
  return rule;
}

static DisallowRule *ParseDisallowRule(Tokenizer &toks, ParserDef &parser) {
  if( toks.peek() != pptok::DISALLOW )
    return 0;
  toks.discard();
  DisallowRule *rule = new DisallowRule();
  rule->m_disallowed = ParseProductionDescriptor(toks,parser);
  if( toks.peek() != pptok::AT )
    error(toks,"expected @ in disallow rule");
  toks.discard();
  rule->m_ats.push_back(ParseProductionDescriptor(toks,parser));
  while( toks.peek() == pptok::AT ) {
    toks.discard();
    if( toks.peek() == pptok::ELIPSIS ) {
      toks.discard();
      rule->m_finalat = ParseProductionDescriptor(toks,parser);
      break;
    } else {
      rule->m_ats.push_back(ParseProductionDescriptor(toks,parser));
    }
  }
  return rule;
}

static bool ParseParsePart(Tokenizer &toks, ParserDef &parser) {
  if( toks.peek() == pptok::TYPEDEF ) {
    ParseTypedefRule(toks,parser);
  } else if( toks.peek() == pptok::PRECEDENCE ) {
    PrecedenceRule *p = ParsePrecedenceRule(toks,parser);
    parser.m_precedencerules.push_back(p);
  } else if( toks.peek() == pptok::DISALLOW ) {
    DisallowRule *d = ParseDisallowRule(toks,parser);
    parser.m_disallowrules.push_back(d);
  } else if( toks.peek() == pptok::REJECTABLE  || toks.peek() == pptok::START || toks.peek() == pptok::ID ) {
    Vector<Production*> productions = ParseProductions(toks,parser);
    parser.m_productions.insert(parser.m_productions.end(), productions.begin(), productions.end());
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
