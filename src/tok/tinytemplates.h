#ifndef __tinytemplates_h
#define __tinytemplates_h
// Some templates so the executable size can shrink

#include <new>

class String {
protected:
  struct strkernel {
    char *m_s;
    int m_refs;
  } *m_p;
  void decref() {
    extern void free(void*);
    if( m_p && --(m_p->m_refs) == 0 ) {
      free(m_p->m_s);
      delete m_p;
    }
  }
public:
  String() : m_p(0) {}
  String(const char *s) : m_p(0) {
    extern char *strdup(const char*);
    if( s ) {
      m_p = new strkernel();
      m_p->m_s = strdup(s);
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
  ~String() {
    decref();
  }
  String &operator+=(const String &rhs) {
    extern char *strcat(char*,const char*);
    if( rhs.m_p ) {
      size_t len = length()+rhs.length();
      char *s = (char*)malloc(len+1);
      *s = 0;
      if( m_p )
        strcat(s,m_p->m_s);
      strcat(s,m_p->m_s);
      decref();
      m_p = new strkernel();
      m_p->m_s = s;
      m_p->m_refs = 1;
    }
    return *this;
  }
  int length() const {
    extern size_t strlen(const char*);
    if( m_p )
      return strlen(m_p->m_s);
    return 0;
  }
  const char *c_str() {
    if( m_p )
      return m_p->m_s;
    return 0;
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
};

template<class T>
class Vector {
  T *m_p;
  int m_size;
  int m_capacity;

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
  typedef typename const T *const_iterator;
  typedef typename T *iterator;
  iterator begin() { return m_p; }
  iterator end() { return m_p+m_size; }
  const_iterator begin() const { return m_p; }
  const_iterator end() const { return m_p+m_size; }
  bool empty() const { return m_size == 0; }
  T &operator[](size_t i) { return m_p[i]; }
  const T &operator[](size_t i) const { return m_p[i]; }
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
};

template<class T, class T2>
class Pair {
public:
  Pair(const T &f, const T2 &s) : first(f), second(s) {}
  T  first;
  T2 second;
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
  iterator insert(const T &value) {
    bool found =false;
    int idx = findIndex(value,found);
    if( found )
      return begin()+idx;
    return m_values.insert(begin()+idx,value);
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
};

#endif
