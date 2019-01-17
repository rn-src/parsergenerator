#include "parser.h"
#include <vector>
#include <strstream>
using namespace std;
namespace pptok {
#include "parsertok.h"
}

TokenInfo pptokinfo = {
  pptok::tokenCount,
  pptok::sectionCount,
  pptok::tokenaction,
  pptok::tokenstr,
  pptok::isws,
  pptok::ranges,
  pptok::stateCount,
  pptok::transitions,
  pptok::transitionOffset,
  pptok::tokens
};

using namespace std;
// parser : parerpart +
// parserpart : (production|precedencerule|disallowrule) SEMICOLON
// productiondecl : REJECTABLE? production action?
// production: nonterminal COLON symbol+
// nonterminal : START | ID
// symbol : ID | ERROR
// precedencerule : PRECEDENCE precedenceparts
// precedenceparts : precedenceparts [['='|'>'] precedencepart]+
// precedencepart : productiondescriptor associativity?
// associativity LEFTASSOC | RIGHTASSOC | NONASSOC | DISALLOW '@' productiondescriptor
// productiondescriptor : '[' [nonterminal|'*']? COLON [symbol|'*'|'...'|'.' symbol]+ ']'
// disallowrule : DISALLOW productiondescriptor '@' productiondescriptor ['...' '@' productiondescriptor]

Production::Production(int nt, const vector<int> &symbols, String action) : m_nt(nt), m_symbols(symbols), m_action(action) {}

int ParserDef::findOrAddSymbol(const char *s) {
  map<string,int>::iterator iter = m_tokens.find(string(s));
  if( iter != m_tokens.end() )
    return iter->second;
  int id = m_tokens.size();
  m_tokens[string(s)] = id;
  return id;
}

void error(Tokenizer &toks, const char *err) {
  // TODO
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

static String ParseSrc(Tokenizer &toks, ParserDef &parser) {
  String s;
  if( toks.peek() != pptok::LBRACE )
    return s;
  toks.discard();
  while( toks.peek() != -1 && toks.peek() != pptok::RBRACE ) {
    if( toks.peek() == pptok::LBRACE )
      s += ParseSrc(toks,parser);
    s += toks.tokstr();
    toks.discard();
  }
  if( toks.peek() != pptok::RBRACE )
    error(toks,"Missing '}'");
  toks.discard();
  return s;
}

static Production *ParseProduction(Tokenizer &toks, ParserDef &parser) {
  int nt = ParseNonterminal(toks,parser);
  if( nt == -1 )
    error(toks,"Expected nonterminal");
  if( toks.peek() != pptok::COLON )
    error(toks,"Exptected ':'");
  vector<int> symbols = ParseSymbols(toks,parser);
  String action;
  if( toks.peek() == pptok::LBRACE )
    action = ParseSrc(toks,parser);
  if( toks.peek() != pptok::SEMI )
    error(toks,"Expected ';'");
  return new Production(nt,symbols,action);
}

void ParseParser(TokBuf *tokbuf, ParserDef &parser) {
  Tokenizer toks(tokbuf,&pptokinfo);
  Production *p = ParseProduction(toks,parser);
  parser.m_productions.push_back(p);
  while( p = ParseProduction(toks,parser) )
    parser.m_productions.push_back(p);
}
