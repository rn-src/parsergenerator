#include "parser.h"
#include "parsertok.h"
#include <vector>
using namespace std;
// parser : parerpart +
// parserpart : production | precedence | disallowrule
// production : [modifier +] nonterminal COLON symbol + [ACTION] SEMICOLON
// modifier : REJECTABLE | LABEL '=' ID
// nonterminal : START | ID
// symbol : ID
// precedence : PRECEDENCE precedenceparts
// precedenceparts : precedenceparts '>' precedencepart
// precedencepart : productiondescriptor [associativity]
// associativity : LEFTASSOC | RIGHTASSOC | NONASSOC
// disallowrule : DISALLOW productiondescriptor '@' productiondescriptor ["...@" productiondescriptor]

ParseStream::ParseStream() {
  // TODO
}

int ParseStream::peek() {
  // TODO
  return 0;
}

void ParseStream::discard() {
  // TODO
}

const char *ParseStream::tokstr() {
  // TODO
  return 0;
}

Production::Production(int nt, const vector<int> &symbols, const char *action) : m_nt(nt), m_symbols(symbols), m_action(action) {}

int ParserDef::findOrAddSymbol(const char *s) {
  map<string,int>::iterator iter = m_tokens.find(string(s));
  if( iter != m_tokens.end() )
    return iter->second;
  int id = m_tokens.size();
  m_tokens[string(s)] = id;
  return id;
}

void error(ParseStream &ps, const char *err) {
  // TODO
}

static int ParseNonterminal(ParseStream &ps, ParserDef &parser) {
  if( ps.peek() == START || ps.peek() == ID )
    return parser.findOrAddSymbol(ps.tokstr());
  return -1;
}

static int ParseSymbol(ParseStream &ps, ParserDef &parser) {
  if( ps.peek() == ID )
    return parser.findOrAddSymbol(ps.tokstr());
  return -1;
}

static vector<int> ParseSymbols(ParseStream &ps, ParserDef &parser) {
  vector<int> symbols;
  int s = ParseSymbol(ps,parser);
  if( s == -1 )
    error(ps,"expected symbol");
  symbols.push_back(s);
  while( (s = ParseSymbol(ps,parser)) != -1 )
    symbols.push_back(s);
  return symbols;
}

static Production *ParseProduction(ParseStream &ps, ParserDef &parser) {
  int nt = ParseNonterminal(ps,parser);
  if( nt == -1 )
    error(ps,"Expected nonterminal");
  if( ps.peek() != COLON )
    error(ps,"Exptected ':'");
  vector<int> symbols = ParseSymbols(ps,parser);
  const char *action = 0;
  if( ps.peek() == LBRACE ) {
    action = ps.tokstr();
    if( action )
      action = strdup(action);
  }
  if( ps.peek() != SEMI )
    error(ps,"Expected ';'");
  return new Production(nt,symbols,action);
}

void ParseParser(ParseStream &ps, ParserDef &parser) {
  Production *p = ParseProduction(ps,parser);
  parser.m_productions.push_back(p);
  while( p = ParseProduction(ps,parser) )
    parser.m_productions.push_back(p);
}
