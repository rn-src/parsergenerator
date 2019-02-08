#include "parser.h"

#if 0
// What is a rejected placement?  A rejected placement means that the production is
// not allowed at the production state because of a rejection rule.  The rejection
// rule can either be explicit (including wildcard matches) or implicit because of
// a precedence rule.
bool ParserDef::isRejectedPlacement(const ProductionState &ps, Production *p) const {
  if( ps.m_items.size() == 0 )
    return false;
  const ProductionStateItem &psi = ps.m_items[0];
  for( Vector<PrecedenceRule*>::const_iterator cur = m_precedencerules.begin(), end = m_precedencerules.end(); cur != end; ++cur ) {
    if( (*cur)->isRejectedPlacement(ps,p) )
      return true;
  }
  for( Vector<DisallowRule*>::const_iterator cur = m_disallowrules.begin(), end = m_disallowrules.end(); cur != end; ++cur ) {
    if( (*cur)->isRejectedPlacement(ps,p) )
      return true;
  }
  return false;
}
 
// What is a partial reject? A partical reject means that there is a production that
// is not allowed at a production/index that is reachable, or more than one
// production/index from the list of prouduction/indexes supplied.  Since we are
// looking more than one deeper, predence rules are not a concern, they reach only
// one deep.
bool ParserDef::isPartialReject(const ProductionState &ps) const {
  for( Vector<DisallowRule*>::const_iterator cur = m_disallowrules.begin(), end = m_disallowrules.end(); cur != end; ++cur ) {
    if( (*cur)->isPartialReject(ps) )
      return true;
  }
  return false;
}

bool PrecedenceRule::isRejectedPlacement(const ProductionState &ps, Production *p) const {
  return false;
}

bool DisallowRule::isRejectedPlacement(const ProductionState &ps, Production *p) const {
  if( ps.m_items.size() < m_ats.size() || (m_ats.size() == ps.m_items.size() && m_finalat))
    return false;
  if( ! m_disallowed->matchesProduction(p) )
    return false;
  Vector<ProductionStateItem>::const_iterator highps = ps.m_items.end()-1;
  if( m_finalat && ! m_finalat->matchesProductionStateItem(*highps--) )
    return false;
  for( Vector<ProductionDescriptor*>::const_iterator lowat = m_ats.begin(), highat = m_ats.end()-1; highat >= lowat; --highat )
    if( ! (*highat)->matchesProductionStateItem(*highps--) )
      return false;
  return true;
}

bool DisallowRule::isPartialReject(const ProductionState &ps) const {
  if( ps.m_items.size() < m_ats.size() || (m_ats.size() == ps.m_items.size() && m_finalat))
    return false;
  if( m_finalat && ! m_finalat->matchesProductionStateItem(ps.m_items[ps.m_items.size()-1]) )
    return false;
  Vector<ProductionStateItem>::const_iterator lowps = ps.m_items.begin(), highps = ps.m_items.end()-1;
  if( m_finalat && ! m_finalat->matchesProductionStateItem(*highps--) )
    return false;
  for( Vector<ProductionDescriptor*>::const_iterator lowat = m_ats.begin(), highat = m_ats.end()-1; highps >= lowps; --highat )
    if( ! (*highat)->matchesProductionStateItem(*highps--) )
      return false;
  return true;
}

class ProductionState {
public:
  Vector<ProductionStateItem> m_items;
  bool operator<(const ProductionState &rhs) const {
    return m_items < rhs.m_items;
  }
  ProductionState() {}
  ProductionState(const ProductionState &rhs) : m_items(rhs.m_items) {}
  ProductionState(Production *p, int idx) {
    m_items.push_back(ProductionStateItem(p,idx));
  }
};

#endif

struct Kernel {
  int state;
};

// Given a CFG+F ParserDef parserIn, produce a CFG ParserDef parserOut
bool NormalizeParser(const ParserDef &parserIn, ParserDef &parserOut) {
  return false;
}
