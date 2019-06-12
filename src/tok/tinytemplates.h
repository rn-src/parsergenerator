#ifndef __tinytemplates_h
#define __tinytemplates_h
// Some templates so the executable size can shrink

#include <new>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

class String {
protected:
  struct strkernel {
    char *m_s;
    int m_refs;
    int m_len;
  } *m_p;
  void decref() {
    if( m_p && --(m_p->m_refs) == 0 ) {
      free(m_p->m_s);
      delete m_p;
    }
  }
public:
  String() : m_p(0) {}
  String(const char *s, int len=-1) : m_p(0) {
    if( s && *s ) {
      m_p = new strkernel();
      if( len < 0 || len > strlen(s) )
        len = strlen(s);
      m_p->m_len = len;
      m_p->m_s = (char*)malloc(m_p->m_len+1);
      strncpy(m_p->m_s,s,len);
      m_p->m_s[len] = 0;
      m_p->m_refs = 1;
    }
  }
  String(const String &s) : m_p(0) {
    if( s.m_p ) {
      m_p = s.m_p;
      m_p->m_refs++;
    }
  }
  String &operator=(const String &s) {
    if( s.m_p )
      s.m_p->m_refs++;
    decref();
    m_p = s.m_p;
    return *this;
  }
  String &operator=(const char *s) {
    if( m_p && m_p->m_refs == 1 ) {
      m_p->m_len = strlen(s);
      m_p->m_s = (char*)realloc(m_p->m_s,m_p->m_len+1);
      strcpy(m_p->m_s,s);
    } else {
      decref();
      m_p = 0;
      if( s && *s) {
        m_p = new strkernel();
        m_p->m_len = strlen(s);
        m_p->m_s = (char*)malloc(m_p->m_len+1);
        strcpy(m_p->m_s,s);
        m_p->m_refs = 1;
      }
    }
    return *this;
  }
  ~String() {
    decref();
  }
  String &operator+=(const char *s) {
    if( ! s || ! *s )
      return *this;
    if( m_p && m_p->m_refs == 1 ) {
      m_p->m_len += strlen(s);
      m_p->m_s = (char*)realloc(m_p->m_s,m_p->m_len+1);
      strcat(m_p->m_s,s);
    } else {
      strkernel *newp = new strkernel();
      newp->m_len = length()+strlen(s);
      newp->m_s = (char*)malloc(newp->m_len+1);
      *newp->m_s = 0;
      if( m_p && m_p->m_s )
        strcat(newp->m_s,m_p->m_s);
      strcat(newp->m_s,s);
      newp->m_refs = 1;
      decref();
      m_p = newp;
    }
    return *this;
  }
  String &operator+=(char c) {
    if( ! c )
      return *this;
    char cc[2];
    cc[0] = c;
    cc[1] = 0;
    *this += cc;
    return *this;
  }
  String &operator+=(const String &rhs) {
    *this += rhs.c_str();
    return *this;
  }
  String operator+(const char *s) const {
    String ret =*this;
    ret += s;
    return ret;
  }
  String operator+(char c) const {
    String ret =*this;
    ret += c;
    return ret;
  }
  String operator+(const String &rhs) const {
    String ret =*this;
    ret += rhs;
    return ret;
  }
  int length() const {
    if( m_p )
      return m_p->m_len;
    return 0;
  }
  const char *c_str() const {
    if( m_p )
      return m_p->m_s;
    return 0;
  }
  bool operator==(const String &rhs) const {
    if( length() != rhs.length() )
      return false;
    if( length() == 0 )
      return true;
    const char *s1 = m_p->m_s, *s2 = rhs.m_p->m_s;
    while( *s1 && *s2 ) {
      if( *s1 != *s2 )
        return false;
      ++s1;
      ++s2;
    }
    if( *s1 != *s2 )
      return false;
    return true;
  }
  bool operator!=(const String &rhs) const {
    return ! operator==(rhs);
  }
  bool operator<(const String &rhs) const {
    if( ! m_p || ! rhs.m_p ) {
      if( ! m_p && rhs.m_p != 0 )
        return true;
      return false;
    }
    const char *s1 = m_p->m_s, *s2 = rhs.m_p->m_s;
    while( *s1 && *s2 ) {
      if( *s1 < *s2 )
        return true;
      else if( *s1 > *s2 )
        return false;
      ++s1;
      ++s2;
    }
    if( ! *s1 && ! *s2 )
      return false;
    else if( ! *s1 )
      return true;
    return false;
  }
  String substr(int first, int cnt) {
    size_t len = length();
    if( first >= len || cnt <= 0 )
      return String("");
    if( cnt > len )
      cnt = len;
    return String(c_str()+first,cnt);
  }
  String slice(int first, int last) {
    size_t len = length();
    if( first < 0 )
      first += len;
    if( last < 0 )
      last += len;
    return substr(first,last-first);
  }
  String &ReplaceAll(const char *s, const char *with) {
    if( ! s || ! *s )
      return *this;
    int len0 = strlen(s);
    int len1 = strlen(with);
    const char *src = c_str();
    const char *nxt = 0;
    int finallen = 0;
    while( (nxt = strstr(src,s)) != 0) {
      finallen += nxt-src;
      finallen += len1;
      src = nxt+len0;
    }
    finallen += strlen(src);
    char *buf = (char*)malloc(finallen+1);
    char *dst = buf;
    src = c_str();
    while( (nxt = strstr(src,s)) != 0) {
      int leadlen = nxt-src;
      strncpy(dst,src,leadlen);
      dst += leadlen;
      if( len1 ) {
        strncpy(dst,with,len1);
        dst += len1;
      }
      src = nxt+len0;
    }
    int taillen = strlen(src);
    if( taillen ) {
      strncpy(dst,src,taillen);
      dst += taillen;
    }
    *dst = 0;
    *this = buf;
    free(buf);
    return *this;
  }
  String &ReplaceAt(int at, int len, const char *with) {
    if( at < 0 )
      at = length()+at;
    if( at < 0 || at > length() )
      return *this;
    int finallen = at+strlen(with)+(length()-len-at);
    char *buf = (char*)malloc(finallen+1);
    char *dst = buf;
    if( at > 0 ) {
      strncpy(dst,c_str(),at);
      dst += at;
    }
    if( strlen(with) > 0 ) {
      strncpy(dst,with,strlen(with));
      dst += strlen(with);
    }
    int taillen = length()-len-at;
    if( taillen > 0 ) {
      strncpy(dst,c_str()+at+len,taillen);
      dst += taillen;
    }
    *dst = 0;
    *this = buf;
    free(buf);
    return *this;
  }
  static String vFormatString(const char *fmt, va_list args) {
    // TODO : premeasure length instead of a fixed length
    char buf[3200];
    vsprintf(buf,fmt,args);
    return String(buf);
  }
  static String FormatString(const char *fmt, ...) {
    String s;
    va_list args;
    va_start(args,fmt);
    s = vFormatString(fmt, args);
    va_end(args);
    return s;
  }
};

template<class T>
class Vector {
  T *m_p;
  size_t m_size;
  size_t m_capacity;

  void capacity(size_t mincap) {
    if( mincap <= m_capacity )
      return;
    if( m_capacity == 0 )
      m_capacity = 8;
    while( m_capacity < mincap ) {
      m_capacity *= 2;
    }
    if( ! m_p )
      m_p = (T*)malloc(sizeof(T)*m_capacity);
    else
      m_p = (T*)realloc(m_p,sizeof(T)*m_capacity);
  }
public:
  Vector() : m_p(0), m_size(0), m_capacity(0) {}
  Vector(const Vector<T> &rhs) : m_p(0), m_size(0), m_capacity(0) {
    insert(end(),rhs.begin(),rhs.end());
  }
  Vector<T> &operator=(const Vector<T> &rhs) {
    clear();
    insert(end(),rhs.begin(),rhs.end());
    return *this;
  }
  typedef const T *const_iterator;
  typedef T *iterator;
  T &front() { return *m_p; }
  iterator begin() { return m_p; }
  iterator end() { return m_p+m_size; }
  const_iterator begin() const { return m_p; }
  const_iterator end() const { return m_p+m_size; }
  bool empty() const { return m_size == 0; }
  T &operator[](int i) {
    if( i < 0 )
      i+= m_size;
    return m_p[i];
  }
  const T &operator[](int i) const {
    if( i < 0 )
      i+= m_size;
    return m_p[i];
  }
  void clear() {
    for( int i = 0; i < m_size; ++i )
      m_p[i].~T();
    m_size = 0;
  }
  iterator push_back(const T &value) {
    capacity(m_size+1);
    iterator dst = end();
    new(dst) T(value);
    ++m_size;
    return dst;
  }
  void resize(size_t newsize, const T &def) {
    if( newsize < m_size ) {
      for( int i = newsize; i < m_size; ++i )
        m_p[i].~T();
      m_size = newsize;
    } else if( newsize > m_size ) {
      capacity(newsize);
      for( int i = m_size; i < newsize; ++i )
        new(m_p+i) T(def);
      m_size = newsize;
    }
  }
  void resize(size_t newsize) {
    if( newsize < m_size ) {
      for( int i = newsize; i < m_size; ++i )
        m_p[i].~T();
      m_size = newsize;
    } else if( newsize > m_size ) {
      capacity(newsize);
      for( int i = m_size; i < newsize; ++i )
        new(m_p+i) T();
      m_size = newsize;
    }
  }
  void pop_back() {
    if( m_size == 0 )
      return;
    erase(m_p+(m_size-1));
  }
  iterator erase(iterator at) {
    at->~T();
    int following = end()-at;
    if( following > 0 )
      memmove(at,at+1,following*sizeof(T));
    --m_size;
    return at;
  }
  iterator erase(iterator first, iterator last) {
    // DEBUG ME!
    int cnt = last-first;
    for( iterator cur = first; cur != last; ++cur )
      cur->~T();
    int following = end()-last;
    if( following > 0 )
      memmove(first,last,following*sizeof(T));
    m_size -= cnt;
    return first;
  }
  iterator insert(iterator at, const T &value) {
    int idx = at-m_p;
    capacity(m_size+1);
    at = m_p+idx;
    int following = end()-at;
    if( following )
      memmove(at+1,at,following*sizeof(T));
    new(at) T(value);
    ++m_size;
    return at;
  }
  template <class T2>
  iterator insert(iterator at, T2 first, T2 last) {
    int idx = at-m_p;
    int xtra = last-first;
    capacity(m_size+xtra);
    at = m_p+idx;
    int following = end()-at;
    if( following )
      memmove(at+xtra,at,following*sizeof(T));
    while( first != last )
      new(at++) T(*first++);
    m_size += xtra;
    return m_p+idx;
  }
  int size() const { return m_size; }
  T &back() { return m_p[m_size-1]; }
  bool operator!=(const Vector<T> &rhs) const {
    if( size() != rhs.size() )
      return true;
    for( int i = 0, n = size(); i < n; ++i )
      if( operator[](i) != rhs[i] )
        return true;
    return false;
  }
  bool operator==(const Vector<T> &rhs) const {
    return ! operator!=(rhs);
  }
  bool operator<(const Vector<T> &rhs) const {
    int len = size()<rhs.size()?size():rhs.size();
    for( int i = 0, n = len; i < n; ++i ) {
      if( operator[](i) < rhs[i] )
        return true;
      else if( rhs[i] < operator[](i) )
        return false;
    }
    if( size() < rhs.size() )
      return true;
    return false;
  }
};

template<class T, class T2>
class Pair {
public:
  Pair() {}
  Pair(const T &f, const T2 &s) : first(f), second(s) {}
  T  first;
  T2 second;
  bool operator<(const Pair<T,T2> &rhs) const {
    if( first < rhs.first )
      return true;
    else if( rhs.first < first )
      return false;
    return second < rhs.second;
  }
};

template<class T>
class Set {
  Vector<T> m_values;
public:
  typedef typename Vector<T>::const_iterator const_iterator;
  typedef typename Vector<T>::iterator iterator;

  iterator begin() { return m_values.begin(); }
  iterator end() { return m_values.end(); }
  const_iterator begin() const { return m_values.begin(); }
  const_iterator end() const { return m_values.end(); }
  bool empty() const { return m_values.empty(); }
  void clear() { m_values.clear(); }
  int size() const { return m_values.size(); }
private:
  int findIndex(const T &value, bool &found) const {
    if( m_values.size() == 0 ) {
      found = false;
      return 0;
    }
    const_iterator low = begin(), high = end()-1;
    while( low <= high ) {
      const_iterator mid = low+(high-low)/2;
      if( value < *mid )
        high = mid-1;
      else if( *mid < value )
        low = mid+1;
      else {
        found = true;
        return mid-begin();
      }
    }
    found = false;
    return low-begin();
  }

public:
  iterator find(const T &value) {
    bool found = false;
    int idx = findIndex(value,found);
    if( found )
      return begin()+idx;
    return end();
  }
  const_iterator find(const T &value) const {
    bool found = false;
    int idx = findIndex(value,found);
    if( found )
      return begin()+idx;
    return end();
  }
  bool contains(const  T &value) const {
    bool found = false;
    int idx = findIndex(value,found);
    return found;
  }
  iterator insert(const T &value) {
    bool found =false;
    int idx = findIndex(value,found);
    if( found )
      return begin()+idx;
    return m_values.insert(begin()+idx,value);
  }
  iterator erase(iterator iter) {
    return erase(*iter);
  }
  template <class Iter>
  void erase(Iter first, Iter last) {
    while( first != last ) {
      erase(*first);
      ++first;
    }
  }
  iterator erase(const T &value) {
    iterator iter = find(value);
    if( iter == end() )
      return iter;
    return m_values.erase(iter);
  }
  template<class Iter>
  void insert(Iter first, Iter last) {
    while( first != last ) {
      insert(*first++);
    }
  }
  bool operator==(const Set<T> &rhs) const {
    if( size() != rhs.size() )
      return false;
    const_iterator rhscur = rhs.begin();
    for( const_iterator cur = begin(), last = end(); cur != last; ++cur, ++rhscur )
      if( *cur < *rhscur || *rhscur < *cur )
        return false;
    return true;
  }
  bool operator!=(const Set<T> &rhs) const {
    return !operator==(rhs);
  }
  bool operator<(const Set<T> &rhs) const {
    return m_values < rhs.m_values;
  }
};

template<class T, class T2>
class Map {
private:
  Vector< Pair<T,T2> > m_entries;

public:
  typedef typename Vector< Pair<T,T2> >::iterator iterator;
  typedef typename Vector< Pair<T,T2> >::const_iterator const_iterator;

  iterator begin() { return m_entries.begin(); }
  iterator end() { return m_entries.end(); }
  const_iterator begin() const { return m_entries.begin(); }
  const_iterator end() const { return m_entries.end(); }
  void clear() { m_entries.clear(); }

private:
  int findIdx(const T &key, bool &found) const {
    if( m_entries.size() == 0 ) {
      found = false;
      return 0;
    }
    const_iterator low = begin(), high = end()-1;
    while( low <= high ) {
      const_iterator mid = low+(high-low)/2;
      if( key < mid->first )
        high = mid-1;
      else if( mid->first < key )
        low = mid+1;
      else {
        found = true;
        return mid-begin();
      }
    }
    found = false;
    return low-begin();
  }
public:
  int size() const { return m_entries.size(); }
  iterator find(const T &key) {
    bool found = false;
    int idx = findIdx(key,found);
    if( found )
      return begin()+idx;
    return end();
  }
  const_iterator find(const T &key) const {
    bool found = false;
    int idx = findIdx(key,found);
    if( found )
      return begin()+idx;
    return end();
  }
  bool contains(const T &key) const {
    bool found =false;
    int idx = findIdx(key,found);
    return found;
  }
  iterator erase(const T &key) {
    iterator iter = find(key);
    if( iter != end() )
      iter = m_entries.erase(iter);
    return iter;
  }
  T2 &operator[](const T &key) {
    bool found = false;
    int idx = findIdx(key,found);
    if( found )
      return m_entries[idx].second;
    return m_entries.insert(begin()+idx,Pair<T,T2>(key,T2()))->second;
  }
  const T2 &operator[](const T &key) const {
    bool found = false;
    int idx = findIdx(key,found);
    if( found )
      return m_entries[idx].second;
    // uh oh. boom
    return m_entries.end()->second;
  }
};

#endif
