#include "parser.h"

extern void *zalloc(size_t len);

extern ElementOps ProductionDescriptorElement;
extern ElementOps ProductionDescriptorsElement;
extern ElementOps RestrictionElement;

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

void Restriction_init(Restriction *This, bool onstack) {
  ProductionDescriptor_init(&This->m_restricted, false);
  ProductionDescriptor_init(&This->m_at, false);
  if (onstack)
    Push_Destroy(This, Restriction_destroy);
}

void Restriction_destroy(Restriction *This) {
  ProductionDescriptor_destroy(&This->m_restricted);
  ProductionDescriptor_destroy(&This->m_at);
}

bool Restriction_LessThan(const Restriction *lhs, const Restriction *rhs) {
  if (ProductionDescriptor_LessThan(&lhs->m_restricted, &rhs->m_restricted))
    return true;
  if (ProductionDescriptor_LessThan(&rhs->m_restricted, &lhs->m_restricted) )
    return false;
  if (ProductionDescriptor_LessThan(&lhs->m_at, &rhs->m_at))
    return true;
  return false;
}

bool Restriction_Equal(const Restriction *lhs, const Restriction *rhs) {
  return ProductionDescriptor_Equal(&lhs->m_restricted, &rhs->m_restricted) && ProductionDescriptor_Equal(&lhs->m_at, &rhs->m_at);
}

void Restriction_Assign(Restriction *lhs, const Restriction *rhs) {
  ProductionDescriptor_Assign(&lhs->m_restricted, &rhs->m_restricted);
  ProductionDescriptor_Assign(&lhs->m_at, &rhs->m_at);
}

void Restriction_clear(Restriction *This) {
  ProductionDescriptor_clear(&This->m_restricted);
  ProductionDescriptor_clear(&This->m_at);
}

void Restriction_print(const Restriction *This, FILE *out, const MapAny /*<int,SymbolDef>*/ *tokens) {
  ProductionDescriptor_print(&This->m_restricted, out, tokens);
  if (This->m_at.m_nt != -1) {
    fputs(" @ ", out);
    ProductionDescriptor_print(&This->m_at, out, tokens);
  }
}

void RestrictAutomata_init(RestrictAutomata *This, bool onstack) {
  This->m_nextstate = 0;
  This->m_startState = -1;
  SetAny_init(&This->m_endStates, getIntElement(), false);
  MapAny_init(&This->m_emptytransitions,getIntElement(),getSetAnyElement(),false);
  MapAny_init(&This->m_transitions,getIntElement(),getMapAnyElement(),false);
  MapAny_init(&This->m_statetorestrictions, getIntElement(), getSetAnyElement(), false);
  if( onstack )
    Push_Destroy(This,RestrictAutomata_destroy);
}

void RestrictAutomata_destroy(RestrictAutomata *This) {
  SetAny_destroy(&This->m_endStates);
  MapAny_destroy(&This->m_emptytransitions);
  MapAny_destroy(&This->m_transitions);
  MapAny_destroy(&This->m_statetorestrictions);
}

void RestrictAutomata_Assign(RestrictAutomata *lhs, const RestrictAutomata *rhs) {
  MapAny_Assign(&lhs->m_statetorestrictions,&rhs->m_statetorestrictions);
  MapAny_Assign(&lhs->m_emptytransitions,&rhs->m_emptytransitions);
  MapAny_Assign(&lhs->m_transitions,&rhs->m_transitions);
  lhs->m_nextstate = rhs->m_nextstate;
  lhs->m_startState = rhs->m_startState;
  SetAny_Assign(&lhs->m_endStates, &rhs->m_endStates);
}

void RestrictAutomata_clear(RestrictAutomata *This) {
  This->m_nextstate = 0;
  This->m_startState = -1;
  SetAny_clear(&This->m_endStates);
  MapAny_clear(&This->m_emptytransitions);
  MapAny_clear(&This->m_transitions);
  MapAny_clear(&This->m_statetorestrictions);
}

int RestrictAutomata_newstate(RestrictAutomata *This) {
  return This->m_nextstate++;
}

int RestrictAutomata_nstates(const RestrictAutomata *This) {
  return This->m_nextstate;
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

void RestrictAutomata_addTransition(RestrictAutomata *This, int s0, int s1, const Restriction *restriction) {
  if (!MapAny_contains(&This->m_transitions, &s0)) {
    MapAny tmp;
    Scope_Push();
    MapAny_init(&tmp, &RestrictionElement, getSetAnyElement(), true);
    MapAny_insert(&This->m_transitions, &s0, &tmp);
    Scope_Pop();
  }
  MapAny *s0map = &MapAny_findT(&This->m_transitions, &s0, MapAny);
  if (!MapAny_contains(s0map, restriction)) {
    SetAny nextstates;
    Scope_Push();
    SetAny_init(&nextstates, getIntElement(), true);
    MapAny_insert(s0map, restriction, &nextstates);
    Scope_Pop();
  }
  SetAny *pdnexts = &MapAny_findT(s0map, restriction, SetAny);
  SetAny_insert(pdnexts, &s1, 0);
}

void RestrictAutomata_addMultiStateTransition(RestrictAutomata *This, int s0, const SetAny /*<int>*/ *s1, const Restriction *restriction) {
  if (!MapAny_contains(&This->m_transitions, &s0)) {
    MapAny tmp;
    Scope_Push();
    MapAny_init(&tmp, &RestrictionElement, getSetAnyElement(), true);
    MapAny_insert(&This->m_transitions, &s0, &tmp);
    Scope_Pop();
  }
  MapAny *s0map = &MapAny_findT(&This->m_transitions, &s0, MapAny);
  if (!MapAny_contains(s0map, restriction)) {
    SetAny nextstates;
    Scope_Push();
    SetAny_init(&nextstates, getIntElement(), true);
    MapAny_insert(s0map, restriction, &nextstates);
    Scope_Pop();
  }
  SetAny *pdnexts = &MapAny_findT(s0map, restriction, SetAny);
  SetAny_insertMany(pdnexts, SetAny_ptrConst(s1), SetAny_size(s1));
}

void RestrictAutomata_addRestriction(RestrictAutomata *This, int stateNo, const Restriction *restriction) {
  if (!MapAny_contains(&This->m_statetorestrictions, &stateNo)) {
    SetAny restrictions;
    Scope_Push();
    SetAny_init(&restrictions, &RestrictionElement, true);
    MapAny_insert(&This->m_statetorestrictions, &stateNo, &restrictions);
    Scope_Pop();
  }
  SetAny *restrictions = &MapAny_findT(&This->m_statetorestrictions, &stateNo, SetAny);
  SetAny_insert(restrictions, restriction, 0);
}

// Find *the* state after the current state, assuming this is a deterministic automata
int RestrictAutomata_nextState(const RestrictAutomata *This, const Production *curp, int pos, int restrictstateno, const Production *ptest) {
  if( RestrictAutomata_isRestricted(This,curp,pos,restrictstateno,ptest) )
    return -1;
  if( MapAny_findConst(&This->m_transitions,&restrictstateno) ) {
    const MapAny /*< Restriction,Set<int> >*/ *t = &MapAny_findConstT(&This->m_transitions,&restrictstateno,MapAny);
    for( int cursub = 0, endsub = MapAny_size(t); cursub != endsub; ++cursub ) {
      const Restriction *symbol = 0;
      const SetAny *nextstates = 0;
      MapAny_getByIndexConst(t,cursub,&symbol,&nextstates);
      if( ProductionDescriptor_matchesProductionAndPosition(&symbol->m_at, curp, pos) && ProductionDescriptor_matchesProduction(&symbol->m_restricted, ptest) )
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
  const SetAny /*<Restriction>*/ *restrictions = &MapAny_findConstT(&This->m_statetorestrictions,&restrictstateno, SetAny);
  if( ! restrictions )
    return false;
  for (int curfb = 0, endfb = SetAny_size(restrictions); curfb != endfb; ++curfb) {
    const Restriction *restriction = &SetAny_getByIndexConstT(restrictions, curfb, Restriction);
    if( ProductionDescriptor_matchesProductionAndPosition(&restriction->m_at, curp, pos) && ProductionDescriptor_matchesProduction(&restriction->m_restricted, ptest) )
      return true;
  }
  return false;
}

int RestrictRegex_addToNfa(ProductionRegex *This, RestrictAutomata *nrestrict, int startState) {
  if (This->m_t == RxType_Production) {
    int endState = RestrictAutomata_newstate(nrestrict);
    Restriction restriction;
    Scope_Push();
    Restriction_init(&restriction, true);
    ProductionDescriptor_Assign(&restriction.m_restricted,This->m_pd);
    RestrictAutomata_addTransition(nrestrict, startState, endState, &restriction);
    Scope_Pop();
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
    int endState = startState;
    int preLhsState = startState;
    if (This->m_low <= 0) {
      endState = RestrictRegex_addToNfa(This->m_lhs, nrestrict, endState);
      // Jump ahead to the end, lhs is optional
      RestrictAutomata_addEmptyTransition(nrestrict, preLhsState, endState);
    } else {
      // do the mandatory number of lhs repeats
      for (int i = 0; i < This->m_low; ++i) {
        preLhsState = endState;
        endState = RestrictRegex_addToNfa(This->m_lhs, nrestrict, preLhsState);
      }
    }
    if (This->m_high == -1) {
      // endless loop back
      RestrictAutomata_addEmptyTransition(nrestrict, endState, preLhsState);
    }
    else {
      for (int i = This->m_low; i < This->m_high; ++i) {
        RestrictAutomata_addEmptyTransition(nrestrict, preLhsState, endState);
        preLhsState = endState;
        endState = RestrictRegex_addToNfa(This->m_lhs, nrestrict, preLhsState);
      }
    }
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

void RestrictAutomata_RestrictsFromStates(const RestrictAutomata *This, const SetAny /*<int>*/ *stateset, SetAny /*<Restiction*>*/ *restrictions) {
  for( int curstate = 0, endstate = SetAny_size(stateset); curstate != endstate; ++curstate ) {
    int state = SetAny_getByIndexConstT(stateset,curstate,int);
    if( MapAny_contains(&This->m_statetorestrictions,&state) ) {
      const SetAny /*<Restriction*>*/ *staterestrictions = &MapAny_findConstT(&This->m_statetorestrictions,&state, SetAny);
      SetAny_insertMany(restrictions, SetAny_ptrConst(staterestrictions), SetAny_size(staterestrictions));
    }
  }
}

void RestrictAutomata_SymbolsFromStates(const RestrictAutomata *This, const SetAny /*<int>*/ *stateset, SetAny /*<Restriction>*/ *symbols) {
  for( int curstate = 0, endstate = SetAny_size(stateset); curstate != endstate; ++curstate ) {
    int state = SetAny_getByIndexConstT(stateset,curstate,int);
    if( MapAny_contains(&This->m_transitions,&state) ) {
      const MapAny /*<Restriction,Set<int> >*/ *statetransitions = &MapAny_findConstT(&This->m_transitions,&state,MapAny);
      for( int curtrans = 0, endtrans = MapAny_size(statetransitions); curtrans != endtrans; ++curtrans ) {
        const Restriction *restriction = 0;
        SetAny /*<int>*/ *nextstates = 0;
        MapAny_getByIndexConst(statetransitions,curtrans,&restriction,&nextstates);
        SetAny_insert(symbols, restriction, 0);
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

  RestrictAutomata_clear(out);
  SetAny_insert(&initstate,&This->m_startState,0);
  int q0 = RestrictAutomata_newstate(out);
  out->m_startState = q0;
  RestrictAutomata_closure(This,&initstate);
  MapAny_insert(&stateset2state,&initstate,&q0);
  VectorAny_push_back(&statesets,&initstate);
  for( int i = 0; i < VectorAny_size(&statesets); ++i ) {
    SetAny /*<int>*/ *stateset = &VectorAny_ArrayOpT(&statesets,i,SetAny);
    SetAny /*Restriction>*/ restrictions;
    SetAny /*Restriction>*/ symbols;
    Scope_Push();
    SetAny_init(&restrictions,&RestrictionElement,true);
    SetAny_init(&symbols,&RestrictionElement,true);
    RestrictAutomata_RestrictsFromStates(This,stateset,&restrictions);
    RestrictAutomata_SymbolsFromStates(This,stateset,&symbols);
    MapAny_insert(&out->m_statetorestrictions,&i,&restrictions);
    
    for( int cursym = 0, endsym = SetAny_size(&symbols); cursym != endsym; ++cursym ) {
      SetAny /*<int>*/ nextstate;
      Scope_Push();
      SetAny_init(&nextstate,getIntElement(),true);
      const Restriction *sub = &SetAny_getByIndexConstT(&symbols,cursym,Restriction);
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
      RestrictAutomata_closure(This,&nextstate);
      int nextStateNo = -1;
      if( ! MapAny_contains(&stateset2state,&nextstate) ) {
        nextStateNo = RestrictAutomata_newstate(out);
        MapAny_insert(&stateset2state,&nextstate,&nextStateNo);
        VectorAny_push_back(&statesets,&nextstate);
        if( SetAny_intersects(&nextstate, &This->m_endStates) )
          RestrictAutomata_addEndState(out,nextStateNo);
      } else
        nextStateNo = MapAny_findConstT(&stateset2state,&nextstate,int);
      RestrictAutomata_addTransition(out,i,nextStateNo,sub);
      Scope_Pop();
    } 
    Scope_Pop();
  }
  Scope_Pop();
}

static int getState(RestrictAutomata *This, MapAny /*<int,int>*/ *state2state, int state) {
  if (!MapAny_contains(state2state, &state)) {
    int toState = RestrictAutomata_newstate(This);
    MapAny_insert(state2state,&state,&toState);
    return toState;
  }
  return MapAny_findT(state2state,&state,int);
}

void RestrictAutomata_Compact(const RestrictAutomata *This, RestrictAutomata *restrictAutomataOut) {
  MapAny /*<int,int>*/ state2state;

  Scope_Push();
  MapAny_init(&state2state,getIntElement(), getIntElement(),true);

  RestrictAutomata_clear(restrictAutomataOut);
  restrictAutomataOut->m_startState = getState(restrictAutomataOut,&state2state, This->m_startState);
  for (int i = 0, n = SetAny_size(&This->m_endStates); i < n; ++i) {
    int endState = SetAny_getByIndexConstT(&This->m_endStates,i,int);
    int endState2 = getState(restrictAutomataOut,&state2state,endState);
    SetAny_insert(&restrictAutomataOut->m_endStates,&endState2,0);
  }
  for (int i = 0, n = MapAny_size(&This->m_emptytransitions); i < n; ++i) {
    const int *fromState = 0;
    const SetAny *toStates = 0;
    MapAny_getByIndexConst(&This->m_emptytransitions, i, &fromState, &toStates);
    int fromState2 = getState(restrictAutomataOut, &state2state, *fromState);
    for (int j = 0, m = SetAny_size(toStates); j < m; ++j) {
      const int toState = SetAny_getByIndexConstT(toStates, j, int);
      int toState2 = getState(restrictAutomataOut, &state2state, toState);
      RestrictAutomata_addEmptyTransition(restrictAutomataOut,fromState2,toState2);
    }
  }
  for (int i = 0, n = MapAny_size(&This->m_transitions); i < n; ++i) {
    const int *fromState = 0;
    const MapAny *restrictiontostates = 0;
    MapAny_getByIndexConst(&This->m_transitions, i, &fromState, &restrictiontostates);
    int fromState2 = getState(restrictAutomataOut, &state2state, *fromState);
    for (int j = 0, m = MapAny_size(restrictiontostates); j < m; ++j) {
      const Restriction *symbol = 0;
      const SetAny *toStates = 0;
      MapAny_getByIndexConst(restrictiontostates, j, &symbol, &toStates);
      for (int j = 0, m = SetAny_size(toStates); j < m; ++j) {
        const int toState = SetAny_getByIndexConstT(toStates, j, int);
        int toState2 = getState(restrictAutomataOut, &state2state, toState);
        RestrictAutomata_addTransition(restrictAutomataOut, fromState2, toState2, symbol);
      }
    }
  }
  for (int i = 0, n = MapAny_size(&This->m_statetorestrictions); i < n; ++i) {
    const int *fromState = 0;
    const SetAny *restrictions = 0;
    MapAny_getByIndexConst(&This->m_statetorestrictions, i, &fromState, &restrictions);
    int fromState2 = getState(restrictAutomataOut, &state2state, *fromState);
    MapAny_insert(&restrictAutomataOut->m_statetorestrictions,&fromState2,restrictions);
  }
  Scope_Pop();
}

// For each transition symbol A along S0->S1 where S1 is an end state, add restriction -/->A to S0
// remove transitions used during the above step
void RestrictAutomata_RestrictionFixups(const RestrictAutomata *This, RestrictAutomata *restrictAutomataOut) {
  SetAny /*<Restriction>*/ toSymbols;
  SetAny /*<int>*/ toStatesEnd;
  SetAny /*<int>*/ toStatesNoEnd;
  RestrictAutomata restrictAutomataTmp;
  Restriction r;

  Scope_Push();
  SetAny_init(&toSymbols, &RestrictionElement, true);
  SetAny_init(&toStatesEnd, getIntElement(), true);
  SetAny_init(&toStatesNoEnd, getIntElement(), true);
  Restriction_init(&r, true);
  RestrictAutomata_init(&restrictAutomataTmp, true);

  restrictAutomataTmp.m_startState = This->m_startState;
  restrictAutomataTmp.m_nextstate = This->m_nextstate;
  MapAny_Assign(&restrictAutomataTmp.m_emptytransitions, &This->m_emptytransitions);

  for (int curtrns = 0, endtrns = MapAny_size(&This->m_transitions); curtrns < endtrns; ++curtrns) {
    const int *pfrom = 0;
    const MapAny /*<Restriction,Set<int>>*/ *restrictiontostatesmap = 0;
    MapAny_getByIndexConst(&This->m_transitions, curtrns, &pfrom, &restrictiontostatesmap);
    int fromState = *pfrom;

    for (int curres = 0, endres = MapAny_size(restrictiontostatesmap); curres < endres; ++curres) {
      const Restriction *tSymbol = 0;
      const SetAny /*<int>*/ *toStates = 0;
      MapAny_getByIndexConst(restrictiontostatesmap, curres, &tSymbol, &toStates);
      SetAny_intersection(&toStatesEnd, toStates, &This->m_endStates);
      SetAny_Assign(&toStatesNoEnd, toStates);
      if( SetAny_size(&toStatesEnd) > 0 ) {
        SetAny_insertMany(&restrictAutomataTmp.m_endStates, SetAny_ptr(&toStatesEnd), SetAny_size(&toStatesEnd));
        for (int cursym = 0, endsym = SetAny_size(&toStatesEnd); cursym < endsym; ++cursym) {
          int toState = SetAny_getByIndexConstT(&toStatesEnd, cursym, int);
          Restriction_clear(&r);
          ProductionDescriptor_UnDottedProductionDescriptor(&tSymbol->m_restricted, &r.m_restricted);
          RestrictAutomata_addTransition(&restrictAutomataTmp, fromState, toState, &r);
        }
      }
      if( SetAny_size(&toStatesNoEnd) > 0 )
        RestrictAutomata_addMultiStateTransition(&restrictAutomataTmp, fromState, &toStatesNoEnd, tSymbol);
    }
  }

  RestrictAutomata_Compact(&restrictAutomataTmp, restrictAutomataOut);

  Scope_Pop();
}

// For each transition symbol A along S0->S1 where S1 has restriction R, add restriction R@A (rx AR) to S0.
// For each transition symbol A along S0->S1 where S1 has transition symbol B along S1->S2, add transition symbol A@B (rx BA) to S0->S1.
// remove transitions used during the above steps
void RestrictAutomata_PlacementFixups(const RestrictAutomata *This, RestrictAutomata *restrictAutomataOut) {
  SetAny /*<Restriction>*/ tosymbols;
  Restriction r;
  RestrictAutomata restrictAutomataTmp;

  Scope_Push();
  SetAny_init(&tosymbols,&RestrictionElement,true);
  Restriction_init(&r, true);
  RestrictAutomata_init(&restrictAutomataTmp,true);

  restrictAutomataTmp.m_startState = This->m_startState;
  restrictAutomataTmp.m_nextstate = This->m_nextstate;
  SetAny_Assign(&restrictAutomataTmp.m_endStates, &This->m_endStates);
  MapAny_Assign(&restrictAutomataTmp.m_emptytransitions, &This->m_emptytransitions);

  for (int curtrns = 0, endtrns = MapAny_size(&This->m_transitions); curtrns < endtrns; ++curtrns) {
    const int *pfrom = 0;
    const MapAny /*<Restriction,Set<int>>*/ *restrictiontostatesmap = 0;
    MapAny_getByIndexConst(&This->m_transitions, curtrns, &pfrom, &restrictiontostatesmap);
    int fromState = *pfrom;

    for (int curres = 0, endres = MapAny_size(restrictiontostatesmap); curres < endres; ++curres) {
      const Restriction *tSymbol = 0;
      const SetAny /*<int>*/ *toStates = 0;
      MapAny_getByIndexConst(restrictiontostatesmap, curres, &tSymbol, &toStates);

      // For each transition symbol A along S0->S1 where S1 has transition symbol B along S1->S2, add transition symbol A@B (rx BA) to S0->S1.
      ProductionDescriptor_Assign(&r.m_at, &tSymbol->m_restricted);
      RestrictAutomata_SymbolsFromStates(This, toStates, &tosymbols);
      for (int cursym = 0, endsym = SetAny_size(&tosymbols); cursym < endsym; ++cursym) {
        const Restriction *tosymbol = &SetAny_getByIndexConstT(&tosymbols, cursym, Restriction);
        ProductionDescriptor_UnDottedProductionDescriptor(&tosymbol->m_restricted, &r.m_restricted);
        RestrictAutomata_addMultiStateTransition(&restrictAutomataTmp, fromState, toStates,&r);
      }

      // For each transition symbol A along S0->S1 where S1 has restriction R, add restriction R@A (rx AR) to S0.
      ProductionDescriptor_Assign(&r.m_at, &tSymbol->m_restricted);
      for( int curtostate = 0, endtostate = SetAny_size(toStates); curtostate != endtostate; ++curtostate ) {
        int toState = SetAny_getByIndexConstT(toStates,curtostate,int);
        const SetAny /*<Restriction>*/ *toRestrictions = &MapAny_findConstT(&This->m_statetorestrictions, &toState, SetAny);
        for (int ridx = 0, endridx = SetAny_size(toRestrictions); ridx < endridx; ++ridx) {
          const Restriction *toRestriction = &SetAny_getByIndexConstT(toRestrictions, ridx, Restriction);
          ProductionDescriptor_UnDottedProductionDescriptor(&toRestriction->m_restricted, &r.m_restricted);
          RestrictAutomata_addRestriction(&restrictAutomataTmp,fromState,&r);
          RestrictAutomata_addEndState(&restrictAutomataTmp,fromState);
        }
      }
    }
  }

  RestrictAutomata_Compact(&restrictAutomataTmp, restrictAutomataOut);

  Scope_Pop();
}

void RestrictAutomata_AddStartStateEpsilons(RestrictAutomata *This) {
  for (int curstate = 0, endstate = RestrictAutomata_nstates(This); curstate < endstate; ++curstate)
    if( curstate != This->m_startState )
      RestrictAutomata_addEmptyTransition(This, curstate, This->m_startState);
}

void RestrictAutomata_print(const RestrictAutomata *This, FILE *out, const MapAny /*<int,SymbolDef>*/ *tokens) {
  fprintf(out, "%d states\n", This->m_nextstate);
  fprintf(out, "start state is %d\n", This->m_startState);
  for( int i = 0; i < This->m_nextstate; ++i ) {
    fprintf(out, "Restrict State %d:\n", i);
    if( MapAny_contains(&This->m_emptytransitions,&i) ) {
      const SetAny /*<int>*/ *empties = &MapAny_findConstT(&This->m_emptytransitions,&i,SetAny);
      for( int cur = 0, end = SetAny_size(empties); cur != end; ++cur )
        fprintf(out, "epsilon transition to %d\n", SetAny_getByIndexConstT(empties,cur,int));
    }
    if( MapAny_contains(&This->m_transitions,&i) ) {
      const MapAny /*< Restriction, Set<int> >*/ *trans = &MapAny_findConstT(&This->m_transitions,&i,MapAny);
      for( int cur = 0, end = MapAny_size(trans); cur != end; ++cur ) {
        const Restriction *restriction = 0;
        const SetAny /*<int>*/ *intset = 0;
        MapAny_getByIndexConst(trans,cur,&restriction,&intset);
        fputs("  on input ", out);
        Restriction_print(restriction,out,tokens);
        fputs("\n", out);
        for( int cur2 = 0, end2 = SetAny_size(intset); cur2 != end2; ++cur2 )
          fprintf(out, "    -> move to state %d\n", SetAny_getByIndexConstT(intset,cur2,int));
      }
    }
    if (MapAny_contains(&This->m_statetorestrictions, &i)) {
      const SetAny /*<Restriction>*/ *restrictions = &MapAny_findConstT(&This->m_statetorestrictions, &i, SetAny);
      fputs("  Restrictions:\n", out);
      for (int cur = 0, end = SetAny_size(restrictions); cur != end; ++cur) {
        fputs("    ", out);
        const Restriction *restriction = &SetAny_getByIndexConstT(restrictions, cur, Restriction);
        Restriction_print(restriction, out, tokens);
        fputs("\n", out);
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
  RestrictAutomata restrictA, restrictB;
  Restriction restriction;

  Scope_Push();
  RestrictAutomata_init(&restrictA, true);
  RestrictAutomata_init(&restrictB, true);
  Restriction_init(&restriction, true);

  // Expand rules
  ParserDef_expandAssocRules(This);
  ParserDef_expandPrecRules(This);
  if( This->m_verbosity > 2 ) {
    fputs("final restrict rules:\n", This->m_vout);
    for( int i = 0; i < VectorAny_size(&This->m_restrictrules); ++i ) {
      RestrictRule_print(VectorAny_ArrayOpT(&This->m_restrictrules,i,RestrictRule*),This->m_vout,&This->m_tokdefs);
      fputs("\n",This->m_vout);
    }
  }
  // Turn the regular expressions into a nondeterministic automata
  int startState = RestrictAutomata_newstate(&restrictA);
  restrictA.m_startState = startState;
  for (int cur = 0, end = VectorAny_size(&This->m_restrictrules); cur != end; ++cur) {
    RestrictRule *rr = VectorAny_ArrayOpT(&This->m_restrictrules, cur, RestrictRule*);
    int endState = RestrictRegex_addToNfa(rr->m_rx, &restrictA, startState);
    RestrictAutomata_addEndState(&restrictA, endState);
  }
  if (This->m_verbosity > 2) {
    fputs("non-deterministic restrict automata (wo/restrictions):\n", This->m_vout);
    RestrictAutomata_print(&restrictA, This->m_vout, &This->m_tokdefs);
  }
  // The state preceeding an endstates gets the restriction.
  RestrictAutomata_RestrictionFixups(&restrictA, &restrictB);
  if( This->m_verbosity > 2 ) {
    fputs("non-deterministic restrict automata (w/restrictions):\n", This->m_vout);
    RestrictAutomata_print(&restrictB,This->m_vout,&This->m_tokdefs);
  }
  // Make the automata deterministic.
  RestrictAutomata_toDeterministicRestrictAutomata(&restrictB,&restrictA);
  if (This->m_verbosity > 2) {
    fputs("deterministic restrict automata (wo/placement-fixups):\n", This->m_vout);
    RestrictAutomata_print(&restrictA, This->m_vout, &This->m_tokdefs);
  }
  // Add the placement fixups (may add epsilons)
  RestrictAutomata_PlacementFixups(&restrictA,&restrictB);
  if (This->m_verbosity > 2) {
    fputs("deterministic restrict automata (w/placement-fixups):\n", This->m_vout);
    RestrictAutomata_print(&restrictB, This->m_vout, &This->m_tokdefs);
  }
  // Add the epsilons to transition the start state
  RestrictAutomata_AddStartStateEpsilons(&restrictB);
  if (This->m_verbosity > 2) {
    fputs("non-deterministic restrict automata (with start-state epsilons):\n", This->m_vout);
    RestrictAutomata_print(&restrictB, This->m_vout, &This->m_tokdefs);
  }
  // Make the automata deterministic again.
  RestrictAutomata_toDeterministicRestrictAutomata(&restrictB, &restrictA);
  // Finished
  RestrictAutomata_Assign(&This->m_restrict, &restrictA);
  if( This->m_verbosity > 2 ) {
    fputs("deterministic restrict automata (final):\n", This->m_vout);
      RestrictAutomata_print(&This->m_restrict,This->m_vout,&This->m_tokdefs);
  }

  Scope_Pop();
}

