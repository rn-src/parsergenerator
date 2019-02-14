#include "parser.h"

class ForbidAutomata {
  int m_nextstate;
public:
  ForbidAutomata() : m_nextstate(0) {}
  int newstate() { return m_nextstate++; }
  int followDeterministic(int in, int symbol) {
    // TODO
  }
  void ToDeterministicForbidAutomata(ForbidAutomata &out) {
    // TODO
  }
};

bool NormalizeParser(ParserDef &parser) {
  ForbidAutomata nforbid, forbid;
  int q0 = nforbid.newstate();
  String err;
  // Expand and combine the rules
  if( ! parser.expandAssocRules(err) || ! parser.expandPrecRules(err) || ! parser.combineRules(err) ) {
    fputs(err.c_str(),stderr);
    return false;
  }
  // Turn the rules into a nondeterministic forbid automata.
  // Make the automata deterministic.
  nforbid.ToDeterministicForbidAutomata(forbid);
  // add the initial production/state
  Production *S = parser.getStartProduction();
  Set< Pair<Production*,int> > nodes, prevnodes;
  nodes.insert( Pair<Production*,int>(S,0) );
  // expand each production/state until closure
  while( nodes.size() > 0 ) {
    Pair<Production*,int> k = *nodes.begin();
    //nodes.remove(k);
    prevnodes.insert(k);
    //forbid.followDeterministic(s,k->second);
    //parserIn.m_disallowrules[i]
  }
  // done
  return true;
}
