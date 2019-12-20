#include "parser.h"

extern void *zalloc(size_t len);

extern ElementOps ProductionDescriptorElement;
extern ElementOps ProductionDescriptorsElement;

static String *NameIndexToName(int idx, String *name) {
  if( idx == 0 )
    String_AssignChars(name,"A");
  else {
    char namestr[10];
    namestr[9] = 0;
    char *s = namestr+9; 
    while( idx ) {
      --s;
      *s = (idx%26)+'A';
      idx /= 26;
    }
    String_AssignChars(name,s);
  }
  return name;
}

void RestrictAutomata_init(RestrictAutomata *This, bool onstack) {
  This->m_nextstate = 0;
  This->m_nxtnameidx = 0;
  SetAny_init(&This->m_startStates, getIntElement(), false);
  SetAny_init(&This->m_endStates, getIntElement(), false);
  MapAny_init(&This->m_emptytransitions,getIntElement(),getSetAnyElement(),false);
  MapAny_init(&This->m_transitions,getIntElement(),getMapAnyElement(),false);
  MapAny_init(&This->m_statetorestricts, getIntElement(), &ProductionDescriptorsElement, false);
  if( onstack )
    Push_Destroy(This,RestrictAutomata_destroy);
}

void RestrictAutomata_destroy(RestrictAutomata *This) {
  SetAny_destroy(&This->m_startStates);
  SetAny_destroy(&This->m_endStates);
  MapAny_destroy(&This->m_emptytransitions);
  MapAny_destroy(&This->m_transitions);
  MapAny_destroy(&This->m_statetorestricts);
}

void RestrictAutomata_Assign(RestrictAutomata *lhs, const RestrictAutomata *rhs) {
  MapAny_Assign(&lhs->m_statetorestricts,&rhs->m_statetorestricts);
  MapAny_Assign(&lhs->m_emptytransitions,&rhs->m_emptytransitions);
  MapAny_Assign(&lhs->m_transitions,&rhs->m_transitions);
  lhs->m_nextstate = rhs->m_nextstate;
  lhs->m_nxtnameidx = rhs->m_nxtnameidx;
  SetAny_Assign(&lhs->m_startStates,&rhs->m_startStates);
  SetAny_Assign(&lhs->m_endStates, &rhs->m_endStates);
}

int RestrictAutomata_newstate(RestrictAutomata *This) {
  return This->m_nextstate++;
}

int RestrictAutomata_nstates(const RestrictAutomata *This) {
  return This->m_nextstate;
}

void RestrictAutomata_addStartState(RestrictAutomata *This, int stateNo) {
  SetAny_insert(&This->m_startStates, &stateNo, 0);
}

void RestrictAutomata_addEndState(RestrictAutomata *This, int stateNo) {
  SetAny_insert(&This->m_endStates, &stateNo, 0);
}

void RestrictAutomata_addEmptyTransition(RestrictAutomata *This, int s0, int s1) {
  if (!MapAny_contains(&This->m_emptytransitions, &s0)) {
    SetAny tmp;
    Scope_Push();
    SetAny_init(&tmp, getIntElement(), true);
    MapAny_insert(&This->m_emptytransitions, &s0, &tmp);
    Scope_Pop();
  }
  SetAny *emptytransitions = &MapAny_findT(&This->m_emptytransitions, &s0, SetAny);
  SetAny_insert(emptytransitions,&s1,0);
}

void RestrictAutomata_addTransition(RestrictAutomata *This, int s0, int s1, const ProductionDescriptor *pd) {
  if (!MapAny_contains(&This->m_transitions, &s0)) {
    MapAny tmp;
    Scope_Push();
    MapAny_init(&tmp, &ProductionDescriptorElement, getSetAnyElement(), true);
    MapAny_insert(&This->m_transitions, &s0, &tmp);
    Scope_Pop();
  }
  MapAny *s0map = &MapAny_findT(&This->m_transitions, &s0, MapAny);
  if (!MapAny_contains(s0map, pd)) {
    SetAny nextstates;
    Scope_Push();
    SetAny_init(&nextstates, getIntElement(), true);
    MapAny_insert(s0map, pd, &nextstates);
    Scope_Pop();
  }
  SetAny *pdnexts = &MapAny_findT(s0map, pd, SetAny);
  SetAny_insert(pdnexts, &s1, 0);
}

void RestrictAutomata_addRestrictions(RestrictAutomata *This, int stateNo, const ProductionDescriptors *restricted) {
  if (!MapAny_contains(&This->m_statetorestricts, &stateNo))
    MapAny_insert(&This->m_statetorestricts, &stateNo, restricted);
  else {
    ProductionDescriptors *pds = &MapAny_findT(&This->m_statetorestricts, &stateNo, ProductionDescriptors);
    ProductionDescriptors_insertMany(pds, ProductionDescriptors_ptrConst(restricted), ProductionDescriptors_size(restricted));
  }
}

// Find *the* state after the current state, assuming this is a deterministic automata
int RestrictAutomata_nextState(const RestrictAutomata *This, const Production *curp, int pos, int restrictstateno, const Production *ptest) {
  if( RestrictAutomata_isRestricted(This,curp,pos,restrictstateno,ptest) )
    return -1;
  if( MapAny_findConst(&This->m_transitions,&restrictstateno) ) {
    const MapAny /*< ProductionDescriptor,Set<int> >*/ *t = &MapAny_findConstT(&This->m_transitions,&restrictstateno,MapAny);
    for( int cursub = 0, endsub = MapAny_size(t); cursub != endsub; ++cursub ) {
      const ProductionDescriptor *pd = 0;
      const SetAny *nextstates = 0;
      MapAny_getByIndexConst(t,cursub,&pd,&nextstates);
      if( ProductionDescriptor_matchesProductionAndPosition(pd, curp, pos) && ProductionDescriptor_matchesProduction(pd, ptest) )
        return SetAny_getByIndexConstT(nextstates,0,int);
    }
  }
  // Every other transition goes to 0.
  return 0;
}

bool RestrictAutomata_isRestricted(const RestrictAutomata *This, const Production *curp, int pos, int restrictstateno, const Production *ptest) {
  if( pos < 0 || pos >= VectorAny_size(&curp->m_symbols) || ptest->m_nt != VectorAny_ArrayOpConstT(&curp->m_symbols,pos,int) )
    return true;
  if( restrictstateno < 0 || restrictstateno >= This->m_nextstate )
    return true;
  const ProductionDescriptors *restricts = &MapAny_findConstT(&This->m_statetorestricts,&restrictstateno, ProductionDescriptors);
  if( restricts )
    return false;
  for (int curfb = 0, endfb = ProductionDescriptors_size(restricts); curfb != endfb; ++curfb)
    if( ProductionDescriptors_matchesProductionAndPosition(restricts, curp, pos) && ProductionDescriptors_matchesProduction(restricts, ptest)  )
      return true;
  return false;
}

int RestrictRegex_addToNfa(ProductionRegex *This, RestrictAutomata *nrestrict, int startState) {
  if (This->m_t == RxType_Production) {
    int endState = RestrictAutomata_newstate(nrestrict);
    RestrictAutomata_addTransition(nrestrict, startState, endState, This->m_pd);
    return endState;
  }
  else if (This->m_t == RxType_BinaryOp) {
    if (This->m_op == BinaryOp_Or) {
      int endlhs = RestrictRegex_addToNfa(This->m_lhs, nrestrict, startState);
      int endrhs = RestrictRegex_addToNfa(This->m_rhs, nrestrict, startState);
      int endState = RestrictAutomata_newstate(nrestrict);
      RestrictAutomata_addEmptyTransition(nrestrict, endlhs, endState);
      RestrictAutomata_addEmptyTransition(nrestrict, endrhs, endState);
      return endState;
    }
    else if (This->m_op == BinaryOp_Concat) {
      int endlhs = RestrictRegex_addToNfa(This->m_lhs, nrestrict, startState);
      int endrhs = RestrictRegex_addToNfa(This->m_rhs, nrestrict, endlhs);
      return endrhs;
    }
  }
  else if (This->m_t == RxType_Many) {
    int prevState = RestrictAutomata_newstate(nrestrict);
    RestrictAutomata_addEmptyTransition(nrestrict, startState, prevState);
    int endState = RestrictAutomata_newstate(nrestrict);
    int loopState = prevState;
    if (This->m_low == 0) {
      loopState = prevState;
      prevState = RestrictRegex_addToNfa(This->m_lhs, nrestrict, prevState);
      RestrictAutomata_addEmptyTransition(nrestrict, loopState, prevState);
    }
    else {
      for (int i = 0; i < This->m_low; ++i) {
        loopState = prevState;
        prevState = RestrictRegex_addToNfa(This->m_lhs, nrestrict, prevState);
      }
    }
    if (This->m_high == -1) {
      // endless loop back
      RestrictAutomata_addEmptyTransition(nrestrict, prevState, loopState);
    }
    else {
      for (int i = This->m_low; i < This->m_high; ++i) {
        RestrictAutomata_addEmptyTransition(nrestrict, prevState, endState);
        prevState = RestrictRegex_addToNfa(This->m_lhs, nrestrict, prevState);
      }
    }
    RestrictAutomata_addEmptyTransition(nrestrict, prevState, endState);
    return endState;
  }
  return -1;
}

void RestrictAutomata_closure(const RestrictAutomata *This, SetAny /*<int>*/ *multistate) {
  int lastsize = 0;
  SetAny /*<int>*/ snapshot;
  Scope_Push();
  SetAny_init(&snapshot,getIntElement(),true);
  do {
    lastsize = SetAny_size(multistate);
    SetAny_Assign(&snapshot,multistate);
    for( int curstate = 0, endstate = SetAny_size(&snapshot); curstate != endstate; ++curstate ) {
      int stateno = SetAny_getByIndexConstT(&snapshot,curstate,int);
      if( MapAny_contains(&This->m_emptytransitions,&stateno) ) {
        const SetAny /*<int>*/ *nextstates = &MapAny_findConstT(&This->m_emptytransitions,&stateno,SetAny);
        SetAny_insertMany(multistate,SetAny_ptrConst(nextstates), SetAny_size(nextstates));
      }
    }
  } while( SetAny_size(multistate) > lastsize );
  Scope_Pop();
}

void RestrictAutomata_RestrictsFromStates(const RestrictAutomata *This, const SetAny /*<int>*/ *stateset, ProductionDescriptors *restricts) {
  for( int curstate = 0, endstate = SetAny_size(stateset); curstate != endstate; ++curstate ) {
    int state = SetAny_getByIndexConstT(stateset,curstate,int);
    if( MapAny_contains(&This->m_statetorestricts,&state) ) {
      const ProductionDescriptors *staterestricts = &MapAny_findConstT(&This->m_statetorestricts,&state, ProductionDescriptors);
      ProductionDescriptors_insertMany(restricts, ProductionDescriptors_ptrConst(staterestricts), ProductionDescriptors_size(staterestricts));
    }
  }
}

void RestrictAutomata_SymbolsFromStates(const RestrictAutomata *This, const SetAny /*<int>*/ *stateset, ProductionDescriptors *symbols) {
  for( int curstate = 0, endstate = SetAny_size(stateset); curstate != endstate; ++curstate ) {
    int state = SetAny_getByIndexConstT(stateset,curstate,int);
    if( MapAny_contains(&This->m_transitions,&state) ) {
      const MapAny /*<ProductionDescriptor,Set<int> >*/ *statetransitions = &MapAny_findConstT(&This->m_transitions,&state,MapAny);
      for( int curtrans = 0, endtrans = MapAny_size(statetransitions); curtrans != endtrans; ++curtrans ) {
        const ProductionDescriptor *pd = 0;
        SetAny /*<int>*/ *nextstates = 0;
        MapAny_getByIndexConst(statetransitions,curtrans,&pd,&nextstates);
        ProductionDescriptors_insert(symbols, pd, 0);
      }
    }
  }
}

// Make a DFA from the NFA.  ALSO, make sure to add the implied transition to the start state.
void RestrictAutomata_toDeterministicRestrictAutomata(const RestrictAutomata *This, RestrictAutomata *out) {
  VectorAny /*< Set<int> >*/ statesets;
  MapAny /*< Set<int>, int >*/ stateset2state;
  SetAny /*<int>*/ initstate;
  Scope_Push();
  VectorAny_init(&statesets,getSetAnyElement(),true);
  MapAny_init(&stateset2state,getSetAnyElement(),getIntElement(),true);
  SetAny_init(&initstate,getIntElement(),true);

  SetAny_insertMany(&initstate,SetAny_ptrConst(&This->m_startStates),SetAny_size(&This->m_startStates));
  int q0 = RestrictAutomata_newstate(out);
  RestrictAutomata_addStartState(out, q0);
  RestrictAutomata_closure(This,&initstate);
  MapAny_insert(&stateset2state,&initstate,&q0);
  VectorAny_push_back(&statesets,&initstate);
  for( int i = 0; i < VectorAny_size(&statesets); ++i ) {
    SetAny /*<int>*/ *stateset = &VectorAny_ArrayOpT(&statesets,i,SetAny);
    ProductionDescriptors restricts;
    ProductionDescriptors symbols;
    Scope_Push();
    ProductionDescriptors_init(&restricts,true);
    ProductionDescriptors_init(&symbols,true);
    RestrictAutomata_RestrictsFromStates(This,stateset,&restricts);
    RestrictAutomata_SymbolsFromStates(This,stateset,&symbols);
    MapAny_insert(&out->m_statetorestricts,&i,&restricts);
    
    for( int cursym = 0, endsym = ProductionDescriptors_size(&symbols); cursym != endsym; ++cursym ) {
      SetAny /*<int>*/ nextstate;
      Scope_Push();
      SetAny_init(&nextstate,getIntElement(),true);
      const ProductionDescriptor *sub = &ProductionDescriptors_getByIndexConst(&symbols,cursym);
      for( int curstate = 0, endstate = SetAny_size(stateset); curstate != endstate; ++curstate ) {
        int state = SetAny_getByIndexConstT(stateset,curstate,int);
        if( MapAny_contains(&This->m_transitions,&state) ) {
          const MapAny /*<ProductionDescriptor,Set<int> >*/ *statetransitions = &MapAny_findConstT(&This->m_transitions,&state,MapAny);
          if( MapAny_contains(statetransitions,sub) ) {
            const SetAny /*<int>*/ *nxtstates = &MapAny_findConstT(statetransitions,sub,SetAny);
            SetAny_insertMany(&nextstate,SetAny_ptrConst(nxtstates),SetAny_size(nxtstates));
          }
        }
      }
      SetAny_insert(&nextstate, &q0, 0); // all states should also transition to the initial state
      RestrictAutomata_closure(This,&nextstate);
      int nextStateNo = -1;
      if( ! MapAny_contains(&stateset2state,&nextstate) ) {
        nextStateNo = RestrictAutomata_newstate(out);
        MapAny_insert(&stateset2state,&nextstate,&nextStateNo);
        VectorAny_push_back(&statesets,&nextstate);
      } else
        nextStateNo = MapAny_findConstT(&stateset2state,&nextstate,int);
      RestrictAutomata_addTransition(out,i,nextStateNo,sub);
      Scope_Pop();
    } 
    Scope_Pop();
  }
  Scope_Pop();
}

void RestrictAutomata_print(const RestrictAutomata *This, FILE *out, const MapAny /*<int,SymbolDef>*/ *tokens) {
  fprintf(out, "%d states\n", This->m_nextstate);
  fprintf(out, "start state is %d\n", SetAny_getByIndexConstT(&This->m_startStates,0,int));
  for( int i = 0; i < This->m_nextstate; ++i ) {
    fprintf(out, "Restrict State %d:\n", i);
    if( MapAny_contains(&This->m_statetorestricts,&i) ) {
      const ProductionDescriptors *restrict = &MapAny_findConstT(&This->m_statetorestricts,&i, ProductionDescriptors);
      fputs("  Restricts:\n", out);
      for( int cur = 0, end = ProductionDescriptors_size(restrict); cur != end; ++cur ) {
        fputs("    ", out);
        ProductionDescriptor_print(&ProductionDescriptors_getByIndexConst(restrict,cur),out,tokens);
        fputs("\n",out);
      }
    }
    if( MapAny_contains(&This->m_emptytransitions,&i) ) {
      const SetAny /*<int>*/ *empties = &MapAny_findConstT(&This->m_emptytransitions,&i,SetAny);
      for( int cur = 0, end = SetAny_size(empties); cur != end; ++cur )
        fprintf(out, "epsilon transition to %d\n", SetAny_getByIndexConstT(empties,cur,int));
    }
    if( MapAny_contains(&This->m_transitions,&i) ) {
      const MapAny /*< ProductionDescriptor, Set<int> >*/ *trans = &MapAny_findConstT(&This->m_transitions,&i,MapAny);
      for( int cur = 0, end = MapAny_size(trans); cur != end; ++cur ) {
        const ProductionDescriptor *pd = 0;
        const SetAny /*<int>*/ *intset = 0;
        MapAny_getByIndexConst(trans,cur,&pd,&intset);
        fputs("  on input ", out);
        ProductionDescriptor_print(pd,out,tokens);
        fputs("\n", out);
        for( int cur2 = 0, end2 = SetAny_size(intset); cur2 != end2; ++cur2 )
          fprintf(out, "    -> move to state %d\n", SetAny_getByIndexConstT(intset,cur2,int));
      }
    }
  }
}

void StringToInt_2_IntToString(const MapAny /*<String,int>*/ *src, MapAny /*<int,String>*/ *tokens) {
  for( int tok = 0; tok != MapAny_size(src); ++tok ) {
    const String *first = 0;
    const int *second = 0;
    MapAny_getByIndexConst(src,tok,&first,&second);
    String_AssignString(&MapAny_findT(tokens,second,String),first);
  }
}

void ParserDef_computeRestrictAutomata(ParserDef *This) {
  RestrictAutomata nrestrict, restrict;
  Scope_Push();
  RestrictAutomata_init(&nrestrict, true);
  RestrictAutomata_init(&restrict, true);

  // Expand and combine the rules
  ParserDef_expandAssocRules(This);
  ParserDef_expandPrecRules(This);
  if( This->m_verbosity > 2 ) {
    fputs("final restrict rules:\n", This->m_vout);
    for( int i = 0; i < VectorAny_size(&This->m_restrictrules); ++i ) {
      RestrictRule_print(VectorAny_ArrayOpT(&This->m_restrictrules,i,RestrictRule*),This->m_vout,&This->m_tokdefs);
      fputs("\n",This->m_vout);
    }
  }
  // Turn the regular expressions into a nondeterministic restrict automata
  int startState = RestrictAutomata_newstate(&nrestrict);
  RestrictAutomata_addStartState(&nrestrict, startState);
  for (int cur = 0, end = VectorAny_size(&This->m_restrictrules); cur != end; ++cur) {
    RestrictRule *rr = VectorAny_ArrayOpT(&This->m_restrictrules, cur, RestrictRule*);
    int endState = RestrictRegex_addToNfa(rr->m_rx, &nrestrict, startState);
    RestrictAutomata_addEndState(&nrestrict,endState);
    RestrictAutomata_addRestrictions(&nrestrict, endState, rr->m_restricted);
  }

  if( This->m_verbosity > 2 ) {
    fputs("final non-deterministic restrict automata:\n", This->m_vout);
    RestrictAutomata_print(&nrestrict,This->m_vout,&This->m_tokdefs);
  }
  // Make the automata deterministic.
  RestrictAutomata_toDeterministicRestrictAutomata(&nrestrict,&restrict);
  RestrictAutomata_Assign(&This->m_restrict,&restrict);
  if( This->m_verbosity > 2 ) {
    fputs("final deterministic restrict automata:\n", This->m_vout);
      RestrictAutomata_print(&This->m_restrict,This->m_vout,&This->m_tokdefs);
  }

  Scope_Pop();
}

