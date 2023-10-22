// Provided by parsergenerator - https://github.com/rn-src/parsergenerator - free to use under MIT license
package lrparse

import (
  "tok"
  "io"
  "fmt"
)

const action_shift = 0
const action_reduce = 1
const action_stop = 2

type ParseInfo struct {
  nstates int
  actions []int
  actionstart []int
  prod0 int
  nproductions int
  productions []int
  productionstart []int
  start int
  parse_error int
  nonterminals []string
  //reduce fnct(extra Any, []Any in) bool
  //extra_t type
  //stack_t type
}

type Parser struct {
  parseinfo ParseInfo
  verbosity int
  fout Writer
}

type intAny struct {
  state int
  value Any
}

func parse(toks Tokenizer) bool {
  var states []int = []int{}
  var values []Any = []Any{}
  var inputqueue []intAny
  states.append(0)
  values.append(self.parseinfo.stack_t())
  var tok int = -1
  var inputnum int = 0
  var err string = nil
  for len(states) > 0 {
    if self.verbosity && self.fout {
      self.fout.write(`lrstates = {$states}\n`)
    }
    stateno = states[-1]
    if len(inputqueue) == 0 {
      inputnum += 1
      nxt: int = toks.peek()
      t = self.parseinfo.stack_t()
      t.tok = Token(toks.tokstr(),toks.filename(),toks.wslines(), toks.wscols(),toks.wslen(),toks.line(),toks.col(),nxt)
      toks.discard()
      inputqueue.append(intAny{nxt,t})
    }
    tok = inputqueue[-1][0]
    if self.verbosity && self.fout {
      t = inputqueue[-1][1]
      if t.tok.s {
        self.fout.write(`input (# {$inputnum}) = {$tok} {$toks.tokid2str(tok)} = \"{$t.tok.s}\" - {$t.tok.filename}({$t.tok.line}:{$t.tok.col})\n`)
      }
    }
    actions: Sequence[int] = self.parseinfo.actions
    firstaction: int = self.parseinfo.actionstart[stateno]
    lastaction: int = self.parseinfo.actionstart[stateno+1]
    for firstaction != lastaction {
      action: int = actions[firstaction]
      didaction: bool = False
      nextaction: int = firstaction
      nsymbols: int = 0
      if action == action_shift {
        var shiftto int = actions[firstaction+1];
        nsymbols = actions[firstaction+2];
        nextaction = firstaction+3+nsymbols;
        if tok in actions[firstaction+3:nextaction] {
          if self.verbosity && self.fout {
            self.fout.write(`shift to {$shiftto}\n`)
	  }
          states.append(shiftto)
          values.append(inputqueue[-1][1])
          del inputqueue[-1]
          didaction = True
	}
      } elif action == action_reduce or action == action_stop {
        reduceby: int = actions[firstaction+1];
        reducecount: int = actions[firstaction+2];
        nsymbols = actions[firstaction+3];
        nextaction = firstaction+4+nsymbols;
	if tok in actions[firstaction+4:nextaction] {
	  var reducebyp int = reduceby-self.parseinfo.prod0
          if self.verbosity && self.fout {
            self.fout.write(`reduce {$reducecount} states by rule {$reducebyp+1} [`)
            production: Sequence[int] = self.parseinfo.productions[self.parseinfo.productionstart[reducebyp]:self.parseinfo.productionstart[reducebyp+1]]
	    var first bool = True
	    for sym := range production {
              self.fout.write(" ")
              if sym >= self.parseinfo.start {
                self.fout.write(self.parseinfo.nonterminals[sym-self.parseinfo.start])
	      } else {
                self.fout.write(toks.tokid2str(sym))
	      }
              if first {
                first = False
                self.fout.write(" :")
	      }
	    }
            self.fout.write(" ] ? ... ")
	  }
          output = self.parseinfo.stack_t()
          output.tok = Token("",toks.filename(),toks.wslines(),toks.wscols(),toks.wslen(),toks.line(),toks.col(),self.parseinfo.productions[self.parseinfo.productionstart[reducebyp]])
          inputs = values[-reducecount:]
          if self.reduce(self.extra,reduceby,inputs,output,self.fout) {
            if self.verbosity && self.fout {
              self.fout.write("YES\n")
	    }
            del states[len(states)-reducecount:]
            del values[len(values)-reducecount:]
            inputqueue.append( (reduceby,output) )
            didaction = True
          } else {
            if self.verbosity && self.fout {
              self.fout.write("NO\n")
	    }
	  }
        }
        if action == action_stop && didaction {
          if self.verbosity && self.fout {
            self.fout.write("ACCEPT\n")
	  }
          return True
	}
      }
      if didaction {
        break
      }
      firstaction = nextaction
    }
    if firstaction == lastaction {
      if tok == self.parseinfo.parse_error {
        if self.verbosity && self.fout {
          self.fout.write(`during error handling, popping lr state {$states[-1]}\n`)
	}
        del states[-1]
        if len(states) == 0 {
          if self.verbosity && self.fout {
            self.fout.write("NO ACTION AVAILABLE, FAIL\n")
	  }
          return False
	}
      } else {
        // report the error for verbose output, but add error to the input, hoping for a handler, and carry on
        posstack = toks.getparsepos()
        t = inputqueue[-1][1]
        switch {
	case err && self.fout:
          self.fout.write(`{$t.tok.filename}({$t.tok.line}:{$t.tok.col}) : error : {err}\n`)
	  for pos := range posstack {
            self.fout.write(`  at {$pos.filename}({$pos.line}:{$pos.col})\n`)
          }
        case self.fout:
          self.fout.write(`{$t.tok.filename}({$t.tok.line}:{$t.tok.col}) : parse error : input (# {$inputnum}) = {$tok} {$toks.tokid2str(tok)} = \"{$t.tok.s}\" - \n`)
	  for pos := range posstack {
            self.fout.write(`  at {$pos.filename}({$pos.line}:{$pos.col})\n`)
	  }
	}
        t_err = self.parseinfo.stack_t()
        t_err.tok = Token{"",t.tok.filename,t.tok.wslines,t.tok.wscols,t.tok.wslen,t.tok.line,t.tok.col,self.parseinfo.parse_error}
        inputqueue.append(intTok{self.parseinfo.parse_error,t_err})
      }
    }
  }
  return True
}

