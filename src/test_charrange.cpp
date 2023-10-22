#include "tokenizer.h"
#include "tests.h"

#define TEST_CONTAINSCHARRANGE(lhs,rhs,expected) if( ! test_containscharrange(lhs,rhs,expected) ) return false;
#define TEST_OVERLAPSRANGE(lhs,low,high,expected) if( ! test_overlapsrange(lhs,low,high,expected) ) return false;
#define TEST_LESSTHAN(lhs,rhs,expected) if( ! test_lessthan(lhs,rhs,expected) ) return false;
#define TEST_CONTAINSCHAR(cr,c,expected) if( ! test_containschar(cr,c,expected) ) return false;

static bool test_containschar(const CharRange *cr, int c, bool expected) {
  if( CharRange_ContainsChar(cr,c) != expected ) {
    fprintf(stdout, "%d in %d-%d expected %s", c, cr->m_low, cr->m_high, expected ? "true" : "false");
    return false;
  }
  return true;
}

static bool test_containscharrange(const CharRange *lhs, const CharRange *rhs, bool expected) {
  if(  CharRange_ContainsCharRange(lhs,rhs) != expected ) {
    fprintf(stdout, "expected %s got %s", expected ? "true" : "false", expected ? "false" : "true");
    return false;
  }
  return true;
}

static bool test_overlapsrange(const CharRange *lhs, int low, int high, bool expected) {
  if(  CharRange_OverlapsRange(lhs,low,high) != expected ) {
    fprintf(stdout, "expected %d-%d to return %s, got %s", low, high, expected ? "true" : "false", expected ? "false" : "true");
    return false;
  }
  return true;
}

static bool test_lessthan(const CharRange *lhs, const CharRange *rhs, bool expected) {
  if(  CharRange_LessThan(lhs,rhs) != expected ) {
    fprintf(stdout, "expected %d-%d < %d-%d -> %s, got %s", lhs->m_low, lhs->m_high, rhs->m_low, rhs->m_high, expected ? "true" : "false", expected ? "false" : "true");
    return false;
  }
  return true;
}

static bool charrange_setrange(void *vp) {
  CharRange cr;
  CharRange *pcr = CharRange_SetRange(&cr,'A','Z');
  if( pcr != &cr ) {
    fputs("wrong charrange pointer", stdout);
    return false;
  }
  if( cr.m_low != 'A' || cr.m_high != 'Z' ) {
    fputs("wrong charrange", stdout);
    return false;
  }
  return true;
}

static bool charrange_containschar(void *vp) {
  CharRange cr;
  CharRange_SetRange(&cr,'A','Z');
  TEST_CONTAINSCHAR(&cr,'C',true)
  TEST_CONTAINSCHAR(&cr,'c',false)
  return true;
}

static bool charrange_containscharrange(void *vp) {
  CharRange cr;
  CharRange cr2;
  CharRange cr3;
  CharRange cr4;
  CharRange_SetRange(&cr,'A','Z');
  CharRange_SetRange(&cr2,'A','C');
  CharRange_SetRange(&cr3,'a','c');
  CharRange_SetRange(&cr4,'A'-3,'B');
  TEST_CONTAINSCHARRANGE(&cr,&cr2,true)
  TEST_CONTAINSCHARRANGE(&cr,&cr3,false)
  TEST_CONTAINSCHARRANGE(&cr,&cr4,false)
  return true;
}

static bool charrange_overlapsrange(void *vp) {
bool CharRange_OverlapsRange(const CharRange *This, int low, int high);
  CharRange cr;
  CharRange *pcr = CharRange_SetRange(&cr,'F','X');
  TEST_OVERLAPSRANGE(&cr,'A','F', true)
  TEST_OVERLAPSRANGE(&cr,'A','J', true)
  TEST_OVERLAPSRANGE(&cr,'G','I', true)
  TEST_OVERLAPSRANGE(&cr,'A','Z', true)
  TEST_OVERLAPSRANGE(&cr,'M','Z', true)
  TEST_OVERLAPSRANGE(&cr,'A','E', false)
  return true;
}

static bool charrange_lessthan(void *vp) {
bool CharRange_LessThan(const CharRange *lhs, const CharRange *rhs);
  CharRange cr;
  CharRange cr2;
  CharRange cr3;
  CharRange cr4;
  CharRange cr5;
  CharRange_SetRange(&cr,'L','X');
  CharRange_SetRange(&cr2,'B','Z');
  CharRange_SetRange(&cr3,'M','N');
  CharRange_SetRange(&cr4,'M','Z');
  CharRange_SetRange(&cr5,'A','C');
  TEST_LESSTHAN(&cr,&cr,false)
  TEST_LESSTHAN(&cr2,&cr,true)
  TEST_LESSTHAN(&cr3,&cr,false)
  TEST_LESSTHAN(&cr4,&cr,false)
  TEST_LESSTHAN(&cr5,&cr,true)
  return true;
}

// using C++ to allow auto registration
static int g_charrange_setrange = addTest("charrange","setrange",charrange_setrange,0);
static int g_charrange_containschar = addTest("charrange","containschar",charrange_containschar,0);
static int g_charrange_containscharrange = addTest("charrange","containscharrange",charrange_containscharrange,0);
static int g_charrange_overlapsrange = addTest("charrange","overlapsrange",charrange_overlapsrange,0);
static int g_charrange_lessthan = addTest("charrange","lessthan",charrange_lessthan,0);
