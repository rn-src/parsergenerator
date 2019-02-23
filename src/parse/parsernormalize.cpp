#include "parser.h"

static void error(const String &err) {
  throw ParserError(err);
}

class gnode {
public:
  Production *m_p;
  int m_stateNo;
  gnode(Production *p, int stateNo) : m_p(p), m_stateNo(stateNo) {}
  gnode() : m_p(0), m_stateNo(0) {}

  bool operator<(const gnode &rhs) const {
    if( m_p < rhs.m_p )
      return true;
    else if( rhs.m_p < m_p )
      return false;
    if( m_stateNo < rhs.m_stateNo )
      return true;
    else if( rhs.m_stateNo < m_stateNo )
      return false;
    return false;
  }

  bool operator==(const gnode &rhs) const {
    return m_p == rhs.m_p && m_stateNo == rhs.m_stateNo;
  }
  void print(FILE *out, const Map<int,String> &tokens) const {
    fprintf(out,"{state=%d,[",m_stateNo);
    m_p->print(out,tokens);
    fputs("]}",out);
  }
};

class ForbidDescriptor {
public:
  String m_name;
  ProductionDescriptors *m_positions;
  ProductionDescriptors *m_forbidden;

  ForbidDescriptor() : m_positions(0), m_forbidden(0) {}
  ForbidDescriptor(const String &name, ProductionDescriptors *positions, ProductionDescriptors *forbidden) : m_name(name), m_positions(positions), m_forbidden(forbidden) {}
  ForbidDescriptor(const ForbidDescriptor &rhs) {
    m_name = rhs.m_name;
    m_positions = rhs.m_positions;
    m_forbidden = rhs.m_forbidden;
  }

  bool forbids(const Production *positionProduction, int pos, const Production *expandProduction) const {
    return m_positions->matchesProductionAndPosition(positionProduction,pos,true) && m_forbidden->matchesProduction(expandProduction,true);
  }
  bool operator<(const ForbidDescriptor &rhs) const {
    if( m_forbidden < rhs.m_forbidden )
      return true;
    return false;
  }
};

typedef Set<ForbidDescriptor> ForbidDescriptors;

typedef Map<int,ForbidDescriptors> StateToForbids;

class ForbidSub {
public:
  const ProductionDescriptors *m_lhs, *m_rhs;
  ForbidSub() : m_lhs(0), m_rhs(0) {}
  ForbidSub(const ProductionDescriptors *lhs, const ProductionDescriptors *rhs) : m_lhs(lhs), m_rhs(rhs) {}
  ForbidSub(const ForbidSub &rhs) : m_lhs(rhs.m_lhs), m_rhs(rhs.m_rhs) {}

  bool operator<(const ForbidSub &rhs) const {
    if( m_lhs < rhs.m_lhs )
      return true;
    if( rhs.m_lhs < m_lhs )
      return false;
    if( m_rhs < rhs.m_rhs )
      return true;
    return false;
  }

  bool matches(const Production *lhs, const Production *rhs) const {
    return m_lhs->matchesProduction(lhs,true) && m_rhs->matchesProduction(rhs,true);
  }
};

static String NameIndexToName(int idx) {
  String name;
  if( idx == 0 )
    name = "A";
  else {
    char namestr[10];
    namestr[9] = 0;
    char *s = namestr+9; 
    while( idx ) {
      --s;
      *s = (idx%26)+'A';
      idx /= 26;
    }
    name = s;
  }
  return name;
}

class ForbidAutomata {
  int m_nextstate;
  int m_nxtnameidx;
  ParserDef &m_parser;
  int m_q0;
  StateToForbids m_statetoforbids;
  Map< int,Set<int> > m_emptytransitions;
  Map< int,Map< ForbidSub,Set<int> > > m_transitions;
public:
  int newstate() { return m_nextstate++; }

  ForbidAutomata(ParserDef &parser) : m_nextstate(0), m_parser(parser), m_nxtnameidx(0) {
    m_q0 = newstate();
  }

  void addEmptyTransition(int s0, int s1) {
    m_emptytransitions[s0].insert(s1);
  }

  void addTransition(int s0, int s1, const ProductionDescriptors *subLhs, const ProductionDescriptors *subRhs) {
    m_transitions[s0][ForbidSub(subLhs,subRhs)].insert(s1);
  }

  // Find *the* state after the current state, assuming this is a deterministic automata
  int nextState(const gnode &g, Production *p) {
    if( m_transitions.find(g.m_stateNo) == m_transitions.end() )
      return 0; // "other" -> 0
    const Map< ForbidSub,Set<int> > &t = m_transitions[g.m_stateNo];
    for( Map<ForbidSub,Set<int> >::const_iterator cursub = t.begin(), endsub = t.end(); cursub != endsub; ++cursub ) {
      if( cursub->first.matches(g.m_p,p) ) {
        if( cursub->second.size() == 0 )
          return 0; // "other" -> 0
        return *cursub->second.begin();
      }
    }
    return 0; // "other" -> 0
  }

  void addRule(const DisallowRule *rule) {
    ProductionDescriptors *nextDesc = 0;
    ProductionDescriptors *disallowed = rule->m_disallowed;
    int qNext = -1;
    for( int i = rule->m_intermediates.size()-1; i >= 0; --i ) {
      int qCur = newstate();
      DisallowProductionDescriptors *ddesc = rule->m_intermediates[i];
      ProductionDescriptors *desc = ddesc->m_descriptors;
      if( nextDesc )
        addTransition(qCur,qNext,desc->clone(),nextDesc->UnDottedProductionDescriptors());
      if( disallowed ) {
        m_statetoforbids[qCur].insert(ForbidDescriptor(NameIndexToName(m_nxtnameidx++),desc->clone(),disallowed->clone()));
        if( ! ddesc->m_star )
          disallowed = 0;
      }
      if( ddesc->m_star )
        addTransition(qCur,qCur,desc->clone(),desc->UnDottedProductionDescriptors());
      qNext = qCur;
      nextDesc = desc;
    }
    int qFirst = newstate();
    ProductionDescriptors *desc = rule->m_lead;
    if( nextDesc )
      addTransition(qFirst,qNext,desc->clone(),nextDesc->UnDottedProductionDescriptors());
    if( disallowed )
      m_statetoforbids[qFirst].insert(ForbidDescriptor(NameIndexToName(m_nxtnameidx++),desc->clone(),disallowed->clone()));
    addEmptyTransition(m_q0,qFirst);
  }

  void expandNode(const gnode &k, Set<gnode> &nodes, const Set<gnode> &processednodes, Map<int,String> &tokens, FILE *out) {
    if( out ) {
      fputs("Considering ",out);
      k.print(out,tokens);
      fputs("\n",out);
    }
    // These are the forbids that apply to the current state.
    ForbidDescriptors &forbids = m_statetoforbids[k.m_stateNo];
    // Look at all of the nonterminals of the production.
    for( int i = 0, n = k.m_p->m_symbols.size(); i < n; ++i ) {
      int s = k.m_p->m_symbols[i];
      if( m_parser.getSymbolType(s) != SymbolTypeNonterminal )
        continue;
      Set<String> forbidnames;
      // Get the list of candidate productions that might be at the current location
      Vector<Production*> productions = m_parser.productionsAt(k.m_p,i);
      // Weed out some of the candidates based on the forbids
      for( Vector<Production*>::iterator curp = productions.begin(); curp != productions.end(); ++curp ) {
        if( out ) {
          fputs(" Is [", out);
          (*curp)->print(out,tokens);
          fprintf(out,"] forbidden at %d --> ",i);
        }
        bool thisProductionForbidden = false;
        for( ForbidDescriptors::iterator curforbid = forbids.begin(), endforbid = forbids.end(); curforbid != endforbid; ++curforbid ) {
          if( curforbid->forbids(k.m_p,i,*curp) ) {
            forbidnames.insert(curforbid->m_name);
            thisProductionForbidden = true;
          }
        }
        if( out ) {
          fputs((thisProductionForbidden?"yes":"no"),out);
          fputc('\n',out);
        }
        if( thisProductionForbidden ) {
          curp = productions.erase(curp);
          --curp;
        }
      }
      if( forbidnames.size() > 0 ) {
        String sName = tokens[s];
        for( Set<String>::const_iterator c = forbidnames.begin(), e = forbidnames.end(); c != e; ++c) {
          sName += "_";
          sName += *c;
        }
        s = m_parser.findSymbolId(sName);
        if( s == -1 ) {
          int nexts = m_parser.addSymbolId(sName,SymbolTypeNonterminal,m_parser.getBaseTokId(s));
          s = nexts;
          k.m_p->m_symbols[i] = s;
          tokens[s] = sName;
          for( Vector<Production*>::iterator curp = productions.begin(), endp = productions.end(); curp != endp; ++curp ) {
            Production *pClone = (*curp)->clone();
            pClone->m_nt = s;
            m_parser.addProduction(pClone);
          }
        } else {
          k.m_p->m_symbols[i] = s;
        }
      }
      for( Vector<Production*>::iterator curp = productions.begin(), endp = productions.end(); curp != endp; ++curp ) {
        int nextStateNo = nextState(k,*curp);
        gnode nextk(*curp,nextStateNo);
        if( processednodes.find(nextk) == processednodes.end() ) {
          if( out ) {
            fputs("Adding ",out);
            nextk.print(out,tokens);
            fputs("\n",out);
          }
          nodes.insert(nextk);
        }
      }
    }
  }

  void closure(Set<int> &multistate) const {
    int lastsize = 0;
    Set<int> snapshot;
    do {
      lastsize = multistate.size();
      snapshot = multistate;
      for( Set<int>::const_iterator curstate = snapshot.begin(), endstate = snapshot.end(); curstate != endstate; ++curstate ) {
        if( m_emptytransitions.contains(*curstate) ) {
          const Set<int> &nextstates = m_emptytransitions[*curstate];
          multistate.insert(nextstates.begin(), nextstates.end());
        }
      }
    } while( multistate.size() > lastsize );
  }

  void ForbidsFromStates(const Set<int> &stateset, ForbidDescriptors &forbids) const {
    for( Set<int>::const_iterator curstate = stateset.begin(), endstate = stateset.end(); curstate != endstate; ++curstate ) {
      int state = *curstate;
      if( m_statetoforbids.contains(state) ) {
        const ForbidDescriptors &stateforbids = m_statetoforbids[state];
        forbids.insert(stateforbids.begin(), stateforbids.end());
      }
    }
  }

  void SymbolsFromStates(const Set<int> &stateset, Set<ForbidSub> &symbols) const {
    for( Set<int>::const_iterator curstate = stateset.begin(), endstate = stateset.end(); curstate != endstate; ++curstate ) {
      int state = *curstate;
      if( m_transitions.contains(state) ) {
        const Map<ForbidSub,Set<int> > &statetransitions = m_transitions[state];
        for( Map<ForbidSub,Set<int> >::const_iterator curtrans = statetransitions.begin(), endtrans = statetransitions.end(); curtrans != endtrans; ++curtrans )
          symbols.insert(curtrans->first);
      }
    }
  }

  void toDeterministicForbidAutomata(ForbidAutomata &out) {
    Vector< Set<int> > statesets;
    Map< Set<int>, int > stateset2state;
    Set<int> initstate;
    initstate.insert(m_q0);
    closure(initstate);
    stateset2state[initstate] = out.m_q0;
    statesets.push_back(initstate);
    for( int i = 0; i < statesets.size(); ++i ) {
      Set<int> &stateset = statesets[i];
      ForbidDescriptors forbids;
      Set<ForbidSub> symbols;
      ForbidsFromStates(stateset,forbids);
      SymbolsFromStates(stateset,symbols);
      out.m_statetoforbids[i] = forbids;
      
      for( Set<ForbidSub>::const_iterator cursym = symbols.begin(), endsym = symbols.end(); cursym != endsym; ++cursym ) {
        Set<int> nextstate;
        const ForbidSub &sub = *cursym;
        for( Set<int>::const_iterator curstate = stateset.begin(), endstate = stateset.end(); curstate != endstate; ++curstate ) {
          int state = *curstate;
          if( m_transitions.contains(state) ) {
            const Map<ForbidSub,Set<int> > &statetransitions = m_transitions[state];
            if( statetransitions.contains(sub) ) {
              const Set<int> &nxtstates = statetransitions[sub];
              nextstate.insert(nxtstates.begin(),nxtstates.end());
            }
          }
        }
        closure(nextstate);
        int nextStateNo = -1;
        if( ! stateset2state.contains(nextstate) ) {
          nextStateNo = out.newstate();
          stateset2state[nextstate] = nextStateNo;
          statesets.push_back(nextstate);
        } else
          nextStateNo = stateset2state[nextstate];
        // no need to add transition to 0, default is transition to 0
        if( nextStateNo != 0 )
          out.addTransition(i,nextStateNo,sub.m_lhs,sub.m_rhs);
      } 
    }
  }
};

void StringToInt_2_IntToString(const Map<String,int> &src, Map<int,String> &tokens) {
  for( Map<String,int>::const_iterator tok = src.begin(); tok != src.end(); ++tok )
    tokens[tok->second] = tok->first;
}

void NormalizeParser(ParserDef &parser) {
  parser.print(stdout);
  ForbidAutomata nforbid(parser), forbid(parser);
  // Expand and combine the rules
  parser.expandAssocRules();
  parser.expandPrecRules();
  parser.combineRules();
  // Turn the rules into a nondeterministic forbid automata
  for( Vector<DisallowRule*>::const_iterator cur = parser.m_disallowrules.begin(), end = parser.m_disallowrules.end(); cur != end; ++cur )
    nforbid.addRule(*cur);
  // Make the automata deterministic.
  nforbid.toDeterministicForbidAutomata(forbid);
  // add the initial production/state
  Production *S = parser.getStartProduction();
  Set<gnode> nodes, processednodes;
  nodes.insert(gnode(S,0));
  // expand each production/state until closure
  Map<int,String> tokens;
  StringToInt_2_IntToString(parser.m_tokens,tokens);
  FILE *out = stdout;
  while( nodes.size() ) {
    Set<gnode>::iterator curgnode = nodes.begin();
    gnode k = *curgnode;
    nodes.erase(curgnode);
    processednodes.insert(k);
    forbid.expandNode(k,nodes,processednodes,tokens,out);
  }
  parser.m_disallowrules.clear();
  parser.print(stdout);
}

