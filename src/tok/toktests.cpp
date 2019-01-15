#include "tokenizer.h"
#include <ostream>
#include <iostream>
#include <fstream>

ostream &operator<<(ostream &out, const CharRange &charrange) {
  out << '[';
  if( isalnum(charrange.m_low) )
    out << "'" << (char)charrange.m_low << "'";
  else
    out << charrange.m_low;
  out << '-';
  if( isalnum(charrange.m_high) )
    out << "'" << (char)charrange.m_high << "'";
  else
    out << charrange.m_high;
  out << ']';
  return out;
}
ostream &operator<<(ostream &out, const CharSet &charset) {
  out << "CharSet(";
  for( CharSet::iterator cur = charset.begin(), end = charset.end(); cur != end; ++cur )
    out << *cur << ',';
  out << ")";
  return out;
}

void test() {
  CharSet charset;
  charset.addCharRange('A','Z');
  charset.addCharRange('a','z');
  charset.addCharRange('0','9');
  cout << charset << endl;
  charset.clear();
  charset.addCharRange('b','y');
  charset.negate();
  cout << charset << endl;
  charset.clear();
  charset.addCharRange('A','M');
  charset.addCharRange('N','Z');
  cout << charset << endl;
  charset.clear();
  charset.addCharRange('N','Z');
  charset.addCharRange('A','M');
  cout << charset << endl;
  charset.clear();
  charset.addCharRange('N','Z');
  charset.addCharRange('A','C');
  charset.addCharRange('D','M');
  cout << charset << endl;
  charset.clear();
  charset.addCharRange('N','Z');
  charset.addCharRange('A','C');
  charset.addCharRange('B','X');
  cout << charset << endl;
  charset.clear();
  charset.addCharRange('A','Z');
  charset.split('M','Z');
  charset.split('A','E');
  charset.split('F','L');
  charset.split('a','g');
  charset.split('a','g');
  charset.split('a','k');
  charset.split('p','t');
  charset.split('m','q');
  cout << charset << endl;

  fstream in;
  in.open("toktest.txt", ios_base::in);
  TokStream s(&in);
  Nfa *dfa = ParseTokenizerFile(s);
  delete dfa;
}
