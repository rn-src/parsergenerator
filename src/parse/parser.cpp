#include "parser.h"
namespace pptok {
#include "parsertok.h"

TokenInfo pptokinfo = {
  tokenCount,
  sectionCount,
  tokenaction,
  tokenstr,
  isws,
  ranges,
  stateCount,
  transitions,
  transitionOffset,
  tokens
};
}

// parser : parsepart +
// parsepart : (production|precedencerule|disallowrule) SEMI
// productiondecl : REJECTABLE? production action?
// production: nonterminal COLON symbol+
// nonterminal : START | ID
// symbol : ID | ERROR
// precedencerule : PRECEDENCE precedenceparts
// precedenceparts : precedencepart [['='|'>'] precedencepart]+
// precedencepart : productiondescriptor associativity?
// associativity LEFTASSOC | RIGHTASSOC | NONASSOC | DISALLOW '@' productiondescriptor
// productiondescriptor : '[' (nonterminal|'*') COLON (symbol|'*'|'...'|'.' symbol)+ ']'
// disallowrule : DISALLOW productiondescriptor ('@' productiondescriptor)+ ('@' '...' productiondescriptor)?

ProductionDescriptor::ProductionDescriptor(int nt, Vector<int> symbols) : m_nt(nt), m_symbols(symbols) {}

Production::Production(bool rejectable, int nt, const vector<int> &symbols, String action) : m_rejectable(rejectable), m_nt(nt), m_symbols(symbols), m_action(action) {}

void ParserDef::setNextSymbolId(int nextsymbolid) {
  m_nextsymbolid = nextsymbolid;
}

int ParserDef::findOrAddSymbol(const char *s) {
  map<string,int>::iterator iter = m_tokens.find(string(s));
  if( iter != m_tokens.end() )
    return iter->second;
  int id = m_nextsymbolid++;
  m_tokens[string(s)] = id;
  return id;
}

void error(Tokenizer &toks, const char *err) {
  throw ParserError(toks.line(),toks.col(),err);
}

static int ParseNonterminal(Tokenizer &toks, ParserDef &parser) {
  if( toks.peek() == pptok::START || toks.peek() == pptok::ID )
    return parser.findOrAddSymbol(toks.tokstr());
  return -1;
}

static int ParseSymbol(Tokenizer &toks, ParserDef &parser) {
  if( toks.peek() == pptok::ID )
    return parser.findOrAddSymbol(toks.tokstr());
  return -1;
}

static vector<int> ParseSymbols(Tokenizer &toks, ParserDef &parser) {
  vector<int> symbols;
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

static Production *ParseProduction(Tokenizer &toks, ParserDef &parser) {
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
  vector<int> symbols = ParseSymbols(toks,parser);
  String action;
  if( toks.peek() == pptok::LBRACE )
    action = ParseAction(toks,parser);
  return new Production(rejectable,nt,symbols,action);
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
  int nt = ParseNtOrStar(toks,parser);
  if( nt == -1 )
    error(toks,"expected nonterminal or '*'");
  if( toks.peek() != pptok::COLON )
    error(toks,"Expected ':' in production descriptor");
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
  if( toks.peek() != pptok::LBRKT )
    error(toks,"Expected ']' to end production descriptor");
  return new ProductionDescriptor(nt, symbols);
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
  if( toks.peek() == pptok::PRECEDENCE ) {
    PrecedenceRule *p = ParsePrecedenceRule(toks,parser);
    parser.m_precedencerules.push_back(p);
  } else if( toks.peek() == pptok::DISALLOW ) {
    DisallowRule *d = ParseDisallowRule(toks,parser);
    parser.m_disallowrules.push_back(d);
  } else if( toks.peek() == pptok::REJECTABLE  || toks.peek() == pptok::START || toks.peek() == pptok::ID ) {
    Production *p = ParseProduction(toks,parser);
    parser.m_productions.push_back(p);
  } else
    return false;
  if( toks.peek() != pptok::SEMI )
    error(toks,"Expected ';'");
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
  parser.setNextSymbolId(pptok::pptokinfo.m_tokenCount);
  Tokenizer toks(tokbuf,&pptok::pptokinfo);
  ParseStart(toks,parser);
}
