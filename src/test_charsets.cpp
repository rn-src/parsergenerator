#include "tokenizer.h"
#include "tests.h"

#define TEST_CHARSET_SIZE(expected) if( ! test_charset_size(&cs,expected) ) return false;
#define TEST_CHARSET_RANGE(index,low,high) if( ! test_charset_range(&cs,index,low,high) ) return false;
#define TEST_CHARSET_EMPTY(expected) if( ! test_charset_empty(&cs,expected) ) return false;
#define TEST_CHARSET_CONTAINS(low,high,expected) if( ! test_charset_contains(&cs,low,high,expected) ) return false;
#define TEST_VECTORANY_SIZE(vec,expected) if( ! test_vectorany_size(vec,expected) ) return false;
#define TEST_VECTORINT_HAS(vec,index,expected) if( ! test_vectorint_has(vec,index,expected) ) return false;
#define TEST_MAPANY_SIZE(map,expected) if( ! test_mapany_size(map,expected) ) return false;

static bool test_vectorany_size(const VectorAny *vec, int expected) {
  if( VectorAny_size(vec) != expected ) {
    fprintf(stdout, "expected vec size %d, got %d", expected, VectorAny_size(vec));
    return false;
  }
  return true;
}

static bool test_vectorint_has(const VectorAny *vec, int index, int expected) {
  if( VectorAny_ArrayOpConstT(vec,index,int) != expected ) {
    fprintf(stdout, "expected vec[%d]=%d, got %d", index, expected, VectorAny_ArrayOpConstT(vec,index,int));
    return false;
  }
  return true;
}

static bool test_mapany_size(const MapAny *map, int expected) {
  if( MapAny_size(map) != expected ) {
    fprintf(stdout, "expected map size %d, got %d", expected, MapAny_size(map));
    return false;
  }
  return true;
}

static bool test_charset_size(const CharSet *cs, int expected) {
  if( CharSet_size(cs) != expected ) {
    fprintf(stdout, "expected charset size %d, got %d", expected, CharSet_size(cs));
    return false;
  }
  return true;
}

static bool test_charset_range(CharSet *cs, int index, uint32_t low, uint32_t high) {
  const CharRange *rng = CharSet_getRange(cs,index);
  if( ! rng ) {
    fprintf(stdout, "got null cs[%d]", index);
    return false;
  }
  if( rng->m_low != low || rng->m_high != high ) {
    fprintf(stdout, "expected range %d-%d, got %d-%d", low, high, rng->m_low, rng->m_high);
    return false;
  }
  return true;
}

static bool test_charset_empty(CharSet *cs, bool expected) {
  if( CharSet_empty(cs) != expected ) {
    fprintf(stdout, "expected empty %s, got %s", expected ? "true" : "false", CharSet_empty(cs) ? "true" : "false");
    return false;
  }
  return true;
}

static bool test_charset_contains(CharSet *cs, uint32_t low, uint32_t high, bool expected) {
  CharRange cr = {low,high};
  if( CharSet_ContainsCharRange(cs,&cr) != expected ) {
    fprintf(stdout, "expected contains %u-%u -> %s, got %s", low, high, expected ? "true" : "false", CharSet_ContainsCharRange(cs,&cr) ? "true" : "false");
    return false;
  }
  return true;
}

static bool charset_init(void *vp) {
  CharSet cs;
  Scope_Push();
  CharSet_init(&cs,true);
  TEST_CHARSET_SIZE(0)
  Scope_Pop();
  return true;
}

static bool charset_assign(void *vp) {
  CharSet cs1;
  CharSet cs;
  Scope_Push();
  CharSet_init(&cs1,true);
  CharSet_init(&cs,true);
  CharSet_addCharRange(&cs1,'A','Z');
  CharSet_addCharRange(&cs1,'a','z');
  CharSet_Assign(&cs,&cs1);
  TEST_CHARSET_SIZE(2)
  TEST_CHARSET_RANGE(0,'A','Z')
  TEST_CHARSET_RANGE(1,'a','z')
  Scope_Pop();
  return true;
}

static bool charset_max(void *vp) {
  CharSet cs;
  Scope_Push();
  CharSet_init(&cs,true);
  CharSet_negate(&cs);
  TEST_CHARSET_SIZE(1)
  TEST_CHARSET_RANGE(0,0,UNICODE_MAX)
  Scope_Pop();
  return true;
}

static bool charset_insert(void *vp) {
  CharSet cs;
  Scope_Push();
  CharSet_init(&cs,true);
  CharSet_addCharRange(&cs,'A','Z');
  CharSet_addCharRange(&cs,'a','z');
  TEST_CHARSET_SIZE(2)
  TEST_CHARSET_RANGE(0,'A','Z')
  TEST_CHARSET_RANGE(1,'a','z')
  Scope_Pop();
  return true;
}

static bool charset_insert_char(void *vp) {
  CharSet cs;
  Scope_Push();
  CharSet_init(&cs,true);
  CharSet_addChar(&cs,'A');
  TEST_CHARSET_SIZE(1)
  TEST_CHARSET_RANGE(0,'A','A')
  Scope_Pop();
  return true;
}

static bool charset_insert_charset(void *vp) {
  CharSet cs1;
  CharSet cs;
  Scope_Push();
  CharSet_init(&cs1,true);
  CharSet_init(&cs,true);
  CharSet_addCharRange(&cs,'A','F');
  CharSet_addCharRange(&cs,'N','V');
  CharSet_addCharRange(&cs1,'G','M');
  CharSet_addCharRange(&cs1,'W','Z');
  CharSet_addCharSet(&cs,&cs1);
  TEST_CHARSET_SIZE(1)
  TEST_CHARSET_RANGE(0,'A','Z')
  Scope_Pop();
  return true;
}

static bool charset_insert_reverse(void *vp) {
  CharSet cs;
  Scope_Push();
  CharSet_init(&cs,true);
  CharSet_addCharRange(&cs,'a','z');
  CharSet_addCharRange(&cs,'A','Z');
  TEST_CHARSET_SIZE(2)
  TEST_CHARSET_RANGE(0,'A','Z')
  TEST_CHARSET_RANGE(1,'a','z')
  Scope_Pop();
  return true;
}

static bool charset_insert_overlap(void *vp) {
  CharSet cs;
  Scope_Push();
  CharSet_init(&cs,true);
  CharSet_addCharRange(&cs,'A','F');
  CharSet_addCharRange(&cs,'B','Z');
  TEST_CHARSET_SIZE(1)
  TEST_CHARSET_RANGE(0,'A','Z')
  Scope_Pop();
  return true;
}

static bool charset_insert_adjacent(void *vp) {
  CharSet cs;
  Scope_Push();
  CharSet_init(&cs,true);
  CharSet_addCharRange(&cs,'A','F');
  CharSet_addCharRange(&cs,'G','Z');
  TEST_CHARSET_SIZE(1)
  TEST_CHARSET_RANGE(0,'A','Z')
  Scope_Pop();
  return true;
}

static bool charset_negate(void *vp) {
  CharSet cs;
  Scope_Push();
  CharSet_init(&cs,true);
  CharSet_addCharRange(&cs,'a','z');
  CharSet_addCharRange(&cs,'A','Z');
  CharSet_negate(&cs);
  TEST_CHARSET_SIZE(3)
  TEST_CHARSET_RANGE(0,0,'A'-1)
  TEST_CHARSET_RANGE(1,'Z'+1,'a'-1)
  TEST_CHARSET_RANGE(2,'z'+1,UNICODE_MAX)
  Scope_Pop();
  return true;
}

static bool charset_empty(void *vp) {
  CharSet cs;
  Scope_Push();
  CharSet_init(&cs,true);
  TEST_CHARSET_EMPTY(true)
  CharSet_addCharRange(&cs,'A','Z');
  TEST_CHARSET_EMPTY(false)
  Scope_Pop();
  return true;
}

static bool charset_clear(void *vp) {
  CharSet cs;
  Scope_Push();
  CharSet_init(&cs,true);
  TEST_CHARSET_EMPTY(true)
  CharSet_addCharRange(&cs,'A','Z');
  TEST_CHARSET_EMPTY(false)
  CharSet_clear(&cs);
  TEST_CHARSET_EMPTY(true)
  Scope_Pop();
  return true;
}

static bool charset_lessthan(void *vp) {
  CharSet cs1;
  CharSet cs;
  Scope_Push();
  CharSet_init(&cs1,true);
  CharSet_init(&cs,true);
  CharSet_addCharRange(&cs,'A','Z');
  CharSet_addCharRange(&cs1,'a','z');
  if( ! CharSet_LessThan(&cs,&cs1) ) {
    fputs("exptected less than true, got false", stdout);
    return false;
  }
  if( CharSet_LessThan(&cs1,&cs) ) {
    fputs("exptected less than false, got true", stdout);
    return false;
  }
  Scope_Pop();
  return true;
}

static bool charset_equal(void *vp) {
  CharSet cs1;
  CharSet cs;
  Scope_Push();
  CharSet_init(&cs1,true);
  CharSet_init(&cs,true);
  CharSet_addCharRange(&cs,'A','Z');
  CharSet_addCharRange(&cs1,'A','Z');
  if( ! CharSet_Equal(&cs,&cs1) ) {
    fputs("exptected equal true, got false", stdout);
    return false;
  }
  CharSet_addCharRange(&cs1,'a','z');
  if( CharSet_Equal(&cs,&cs1) ) {
    fputs("exptected equal false, got true", stdout);
    return false;
  }
  Scope_Pop();
  return true;
}

static bool charset_contains(void *vp) {
  CharSet cs;
  Scope_Push();
  CharSet_init(&cs,true);
  CharSet_addCharRange(&cs,'A','Z');
  TEST_CHARSET_CONTAINS('A','Z',true);
  TEST_CHARSET_CONTAINS('a','z',false);
  Scope_Pop();
  return true;
}

static void cbadd(const CharSet* cs, VectorAny /*<int>*/* charset_indexes, MapAny /*<CharSet>*/* allcharsets) {
  MapAny_insert(allcharsets,cs,charset_indexes);
}

static const VectorAny *find_vector_by_charset(MapAny *map, int ch) {
  CharSet cs;
  Scope_Push();
  CharSet_init(&cs,true);
  CharSet_addChar(&cs,ch);
  const VectorAny *vec = &MapAny_findConstT(map,&cs,VectorAny);
  if( ! vec )
    fprintf(stdout, "unable to find vector by charset %c", ch);
  Scope_Pop();
  return vec;
}

static bool charset_combo_breaker(void *vp) {
  CharSet cs;
  CharSet cs1;
  CharSet cs2;
  CharSet cs3;
  VectorAny /*CharSet*/ charsets;
  MapAny /* <CharSet,Vector<int> >*/ allcharsets;
  Scope_Push();
  CharSet_init(&cs,true);
  CharSet_init(&cs1,true);
  CharSet_init(&cs2,true);
  CharSet_init(&cs3,true);
  VectorAny_init(&charsets,getCharSetElement(),true);
  MapAny_init(&allcharsets,getCharSetElement(),getVectorAnyElement(),true);

  CharSet_addCharRange(&cs1,'A','A');
  CharSet_addCharRange(&cs1,'B','B');
  CharSet_addCharRange(&cs1,'C','C');
  CharSet_addCharRange(&cs1,'E','E');
  VectorAny_push_back(&charsets,&cs1);

  CharSet_addCharRange(&cs2,'A','A');
  CharSet_addCharRange(&cs2,'B','B');
  CharSet_addCharRange(&cs2,'D','D');
  CharSet_addCharRange(&cs2,'F','F');
  VectorAny_push_back(&charsets,&cs2);

  CharSet_addCharRange(&cs3,'A','A');
  CharSet_addCharRange(&cs3,'C','C');
  CharSet_addCharRange(&cs3,'D','D');
  CharSet_addCharRange(&cs3,'G','G');
  VectorAny_push_back(&charsets,&cs3);

  CharSet_combo_breaker(&charsets,(ComboBreakerCB)cbadd,&allcharsets);
  TEST_MAPANY_SIZE(&allcharsets,7)
  const VectorAny *combo = 0;

  // {0,1,2} 'A'
  combo = find_vector_by_charset(&allcharsets,'A');
  if( ! combo )
    return false;
  TEST_VECTORANY_SIZE(combo,3)
  TEST_VECTORINT_HAS(combo,0,0)
  TEST_VECTORINT_HAS(combo,1,1)
  TEST_VECTORINT_HAS(combo,2,2)

  // {0,1} 'B'
  combo = find_vector_by_charset(&allcharsets,'B');
  if( ! combo )
    return false;
  TEST_VECTORANY_SIZE(combo,2)
  TEST_VECTORINT_HAS(combo,0,0)
  TEST_VECTORINT_HAS(combo,1,1)

  // {0,2} 'C'
  combo = find_vector_by_charset(&allcharsets,'C');
  if( ! combo )
    return false;
  TEST_VECTORANY_SIZE(combo,2)
  TEST_VECTORINT_HAS(combo,0,0)
  TEST_VECTORINT_HAS(combo,1,2)

  // {1,2} 'D'
  combo = find_vector_by_charset(&allcharsets,'D');
  if( ! combo )
    return false;
  TEST_VECTORANY_SIZE(combo,2)
  TEST_VECTORINT_HAS(combo,0,1)
  TEST_VECTORINT_HAS(combo,1,2)

  // {0} 'E'
  combo = find_vector_by_charset(&allcharsets,'E');
  if( ! combo )
    return false;
  TEST_VECTORANY_SIZE(combo,1)
  TEST_VECTORINT_HAS(combo,0,0)

  // {1} 'F'
  combo = find_vector_by_charset(&allcharsets,'F');
  if( ! combo )
    return false;
  TEST_VECTORANY_SIZE(combo,1)
  TEST_VECTORINT_HAS(combo,0,1)

  // {2} 'G'
  combo = find_vector_by_charset(&allcharsets,'G');
  if( ! combo )
    return false;
  TEST_VECTORANY_SIZE(combo,1)
  TEST_VECTORINT_HAS(combo,0,2)

  Scope_Pop();
  return true;
}

bool charset_case1(void *vp) {
  CharSet cs;
  Scope_Push();
  CharSet_init(&cs,true);
  CharSet_addCharRange(&cs,49,50);
  CharSet_addCharRange(&cs,57,57);
  CharSet_addCharRange(&cs,85,85);
  TEST_CHARSET_SIZE(3)
  TEST_CHARSET_RANGE(0,49,50)
  TEST_CHARSET_RANGE(1,57,57)
  TEST_CHARSET_RANGE(2,85,85)
  CharSet_addCharRange(&cs,48,8703);
  TEST_CHARSET_SIZE(1)
  TEST_CHARSET_RANGE(0,48,8703)
  Scope_Pop();
  return true;
}

bool charset_case2(void *vp) {
  CharSet cs;
  Scope_Push();
  CharSet_init(&cs,true);
  CharSet_addCharRange(&cs,50,50);
  CharSet_addCharRange(&cs,85,85);
  TEST_CHARSET_SIZE(2)
  TEST_CHARSET_RANGE(0,50,50)
  TEST_CHARSET_RANGE(1,85,85)
  CharSet_addCharRange(&cs,49,49);
  TEST_CHARSET_SIZE(2)
  TEST_CHARSET_RANGE(0,49,50)
  TEST_CHARSET_RANGE(1,85,85)
  Scope_Pop();
  return true;
}

// using C++ to allow auto registration
static int g_charset_init = addTest("charset","init",charset_init,0);
static int g_charset_assign = addTest("charset","assign",charset_assign,0);
static int g_charset_max = addTest("charset","max",charset_max,0);
static int g_charset_insert = addTest("charset","insert",charset_insert,0);
static int g_charset_insert_char = addTest("charset","insert_char",charset_insert_char,0);
static int g_charset_insert_charset = addTest("charset","insert_charset",charset_insert_charset,0);
static int g_charset_insert_reverse = addTest("charset","insert_reverse",charset_insert_reverse,0);
static int g_charset_insert_overlap = addTest("charset","insert_overlap",charset_insert_overlap,0);
static int g_charset_insert_adjacent = addTest("charset","insert_adjacent",charset_insert_adjacent,0);
static int g_charset_negate = addTest("charset","negate",charset_negate,0);
static int g_charset_empty = addTest("charset","empty",charset_empty,0);
static int g_charset_clear = addTest("charset","clear",charset_clear,0);
static int g_charset_lessthan = addTest("charset","lessthan",charset_lessthan,0);
static int g_charset_equal = addTest("charset","equal",charset_equal,0);
static int g_charset_contains = addTest("charset","contains",charset_contains,0);
static int g_charset_combo_breaker = addTest("charset","combo_breaker",charset_combo_breaker,0);
static int g_charset_case1 = addTest("charset","case1",charset_case1,0);
static int g_charset_case2 = addTest("charset","case2",charset_case2,0);
