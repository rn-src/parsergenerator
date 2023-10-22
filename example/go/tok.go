// Provided by parsergenerator - https://github.com/rn-src/parsergenerator - free to use under MIT license
package tok

const max_buffer_accum = 128
const buffer_trim_keep = 16

type TokenInfo struct {
  tokenCount int
  sectionCount int
  sectioninfo []int
  sectioninfo_offset []int
  tokenstr []string
  isws []bool
  stateCount int
  stateinfo []int
  stateinfo_offset []int
}

type ParsePos struct {
    filename string
    line, col int
}

type TokBuf interface {
  peekc(n int) int
  discard(n int)
  line() int
  col() int
  filename() string
  getparsepos() []ParsePos
}

type StackTokBufItem struct {
  tokbuf TokBuf
  filename string
  startline int
  lineno int
}

type Token struct {
  s string
  filename string
  wslines int
  wscols int
  wslen int
  line int
  col int
  sym int
}

type StackEventCallback func(TokBuf)

type StackTokBuf struct {
  stack []StackTokBufItem
  endtok Token
  onstartbuf StackEventCallback
  onendbuf StackEventCallback
}

func (self *StackTokBuf) push(tokbuf TokBuf) {
  item = StackTokBufItem(tokbuf)
  self.stack.append(item)
  if self.onstartbuf {
    self.onstartbuf(tokbuf)
  }
  // if empty file, pop immediately
  if tokbuf.peekc(0) == -1 {
    self.discard(0)
  }
}

func (self *StackTokBuf) peekc(n int) int {
  if len(self.stack) {
    return self.stack[-1].tokbuf.peekc(n)
  }
  return -1
}

func (self *StackTokBuf) discard(n int) {
  if len(self.stack) == 0 {
    return
  }
  tokbuf = self.stack[-1].tokbuf
  tokbuf.discard(n)
  if tokbuf.peekc(0) == -1 {
    self.stack = self.stack[:len(self.stack)-1]
    if self.onendbuf {
      self.onendbuf(tokbuf)
    }
    if len(self.stack) == 0 {
      self.endtok.filename = tokbuf.filename()
      self.endtok.line = tokbuf.line()
      self.endtok.col = tokbuf.col()
    }
  }
}

func (self *StackTokBuf) line() int {
  if len(self.stack) {
    item: StackTokBufItem = self.stack[-1]
    tokbuf = item.tokbuf
    if item.lineno > 0 {
      return item.lineno + (tokbuf.line()-item.startline)
    }
    return tokbuf.line()
  }
  return self.endtok.line
}

func (self *StackTokBuf) col(self) int {
  if len(self.stack) {
    item = self.stack[len(self.stack)-1]
    tokbuf = item.tokbuf
    return tokbuf.col()
  }
  return self.endtok.col
}

func (self *StackTokBuf) filename() string {
 if len(self.stack) {
    item = self.stack[len(self.stack)-1]
    tokbuf = item.tokbuf
    if len(item.filename) {
      return item.filename
    }
    return tokbuf.filename()
  }
  return self.endtok.filename
}

func (self *StackTokBuf) setline(lineno int, filename string) {
  if len(self.stack) {
    item = self.stack[len(self.stack)-1]
    tokbuf = item.tokbuf
    item.startline = tokbuf.line()
    item.lineno = lineno
    item.filename = filename
  }
}

func (self *StackTokBuf) getparsepos() []ParsePos {
  items := []ParsePos{}
  for _, item := range(reversed(self.stack)) {
    items = append(items,item.tokbuf.getparsepos())
  }
  return items
}

type Tokenizer interface {
  peek() int // get the next token without discarding it
  discard() // discard the next token"""
  tokstr() string // get the next token text
  filename() string // get the filename associated with the current token
  wslines() int // get the number of whitespaces lines before the current token
  wscols() int // get the number of whitespace columns before the current token
  wslen() int // get the total length of the whitespace before the current token
  line() int // get the line number of the current token
  col() int // get the column number of the current token
  tokid2str(tok int) string // get the name of the current token
  getparsepos() []ParsePos //get the full stack for the parser"""
}

// A TokBuf based on an IO interface
type FileTokBuf struct {
  io IO
  eof bool // = false
  buffer []int
  bufpos int // = 0
  _filename string
  pos int // = 0
  _line int // = 1
  _col int // = 1
}

func (self *FileTokBuf) addc() bool {
  if self.eof {
    return False
  }
  c = self.io.read(1)
  if ! len(c) {
    self.eof = True
    return False
  }
  self.buffer.append(ord(c))
  return True
}

// Look ahead n characters into the buffer and return the character, without discarding the buffer
func (self *FileTokBuf) peekc(n int) int {
  for n >= len(self.buffer)-self.bufpos {
    if ! self.addc() {
      return -1
    }
  }
  return self.buffer[self.bufpos+n]
}

func (self *FileTokBuf) discard(n int) {
  for i := range(n) {
    if len(self.buffer)-self.bufpos == 0 {
      if ! self.addc() {
        break
      }
    }
    c = self.buffer[self.bufpos]
    self.bufpos += 1
    if c == ord('\n') {
      self._line += 1
      self._col = 1
    } else {
      self._col += 1
    }
  }
  // Chop the buffer once in a while, gradually, not all at once
  if self.bufpos > max_buffer_accum {
    self.buffer = self.buffer[self.bufpos-buffer_trim_keep:]
    self.bufpos = buffer_trim_keep
  }
}

func (self *FileTokBuf) line() int {
    return self._line
}

func (self *FileTokBuf) col() int {
    return self._col
}

func (self *FileTokBuf) filename() string {
    return self._filename
}

// where are we?
func (self *FileTokBuf) getparsepos(self) []ParsePos {
  var pos = ParsePos{filename:self._filename,line:self._line,col:self._col}
  return [1]ParsePos{pos}
}

// A Tokenizer implementation with TokBuf input"""
type TokBufTokenizer struct {
  tokbuf TokBuf 
  tokinfo TokenInfo
  tok int // = -1
  _toklen int
  s string
  _wslines int
  _wscols int
  _wslen int
  stack []int // = [0]
}

// get the token considered accepted by the state, returns -1 for none
func (self *TokBufTokenizer) getTok(state int) int {
  var curstateinfo int = self.tokinfo.stateinfo_offset[state]
  var endstateinfo int = self.tokinfo.stateinfo_offset[state+1]
  if curstateinfo < endstateinfo {
    return self.tokinfo.stateinfo[curstateinfo]-1
  }
  return -1
}

// Let the caller know if the provided token is considered whitespace
func (self *TokBufTokenizer) isWs(tok int) bool {
    return self.tokinfo.isws[tok/8]&(1<<(tok%8))!=0
}

// Given a state, and a character, find the transition and return the next state, or -1 for none"""
func (self *TokBufTokenizer) nextState(state int, c int) int {
    var curstateinfo int = self.tokinfo.stateinfo_offset[state]
    var endstateinfo int = self.tokinfo.stateinfo_offset[state+1]
    if curstateinfo < endstateinfo {
      curstateinfo += 1 // tok
    }
    for curstateinfo < endstateinfo {
      to: int = self.tokinfo.stateinfo[curstateinfo]
      pairCount: int = self.tokinfo.stateinfo[curstateinfo+1]
      last = 0
      for i := range(pairCount) {
        low: int = self.tokinfo.stateinfo[curstateinfo+2+i*2]+last
        last = low
        high: int = self.tokinfo.stateinfo[curstateinfo+2+i*2+1]+last
        last = high
        if c >= low && c <= high {
          return to
	}
      }
      curstateinfo += 2+pairCount*2
    }
    return -1
}

func (self *TokBufTokenizer) push(tokset int) {
    self.stack.append(tokset)
}

func (self *TokBufTokenizer) pop() {
  if ! len(self.stack) {
    return
  }
  self.stack = self.stack[:len(self.stack)-1]
}

func (self *TokBufTokenizer) gotots(tokset int) {
    self.stack[-1] = tokset
}

func (self *TokBufTokenizer) peek() int {
  if self.tok != -1 {
    return self.tok
  }
  for {
    curSection: int = self.stack[-1]
    if self.tok != -1 {
      if self.isWs(self.tok) {
        tokstartline: int = self.tokbuf.line()
        tokstartcol: int = self.tokbuf.col()
        self.tokbuf.discard(self._toklen)
        tokendline: int = self.tokbuf.line()
        tokendcol: int = self.tokbuf.col()
        if tokendline > tokstartline {
          self._wslines += (tokendline - tokstartline)
          self._wscols = tokendcol - 1;
        } else {
          self._wscols += (tokendcol - tokstartcol)
	}
        self._wslen += self._toklen
      } else {
        self.tokbuf.discard(self._toklen)
        self._wslines = 0
        self._wscols = 0
        self._wslen = 0
      }
      self.tok = -1
      self._toklen = 0
    }
    state: int = self.nextState(0, curSection)
    tok: int
    toklen: int = -1
    length: int = 0
    for state != -1 {
      t: int = self.getTok(state)
      if t != -1 {
        tok = t;
        toklen = length
      }
      c: int = self.tokbuf.peekc(length)
      state = self.nextState(state, c)
      length += 1
    }
    self.tok = tok
    self._toklen = toklen
    if self.tok != -1 {
      var startsection int = self.tokinfo.sectioninfo_offset[curSection]
      var endsection int = self.tokinfo.sectioninfo_offset[curSection+1]
      for startsection < endsection {
        actiontok: int = self.tokinfo.sectioninfo[startsection]
        action: int = self.tokinfo.sectioninfo[startsection+1]
        actionarg: int = self.tokinfo.sectioninfo[startsection+2]
        if actiontok == self.tok {
          switch action {
          case 1: self.push(actionarg)
	  case 2: self.pop()
          case 3: self.gotots(actionarg)
	  }
          break
	}
        startsection += 3
      }
    }
    if self.tok == -1 || ! self.isWs(self.tok) {
      break
    }
  }
  return self.tok
}

func (self *TokBufTokenizer) discard() {
  if self.tok == -1 {
    self.peek()
  }
  if self.tok != -1 {
    self.tokbuf.discard(self._toklen)
    self.tok = -1
    self._toklen = 0
    self._wslines = 0
    self._wscols = 0
    self._wslen = 0
  }
}

func (self *TokBufTokenizer) length() int {
    return self._toklen
}

func (self *TokBufTokenizer) tokstr() string {
  if self.tok == -1 {
    return ""
  }
  if self.tok != -1 {
    s = StringIO()
    for i := range(self._toklen) {
      s.write(chr(self.tokbuf.peekc(i)))
    }
    self.s = s.getvalue()
  }
  return self.s
}

func (self *TokBufTokenizer) wslines() int {
    return self._wslines
}

func (self *TokBufTokenizer) wscols() int {
    return self._wscols;
}

func (self *TokBufTokenizer) wslen() int {
    return self._wslen;
}

func (self *TokBufTokenizer) line() int {
    return self.tokbuf.line()
}

func (self *TokBufTokenizer) col() int {
    return self.tokbuf.col()
}

func (self *TokBufTokenizer) filename() string {
    return self.tokbuf.filename()
}

func (self *TokBufTokenizer) tokid2str(tok int) string {
  if tok < 0 || tok >= self.tokinfo.tokenCount {
    return  "?"
  }
  return self.tokinfo.tokenstr[tok]
}

func (self *TokBufTokenizer) getparsepos() []ParsePos {
  return self.tokbuf.getparsepos()
}

