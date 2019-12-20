      = WS1        /[[:white:]]+/
      = WS2        /#[^\n]*\n/
      ~ WS         /({WS1}|{WS2})+/
        START      /<start>/
        EXTRA      /<extra>/
        ERROR      /<error>/
        RESTRICT   /RESTRICT/
        PRECEDENCE /PRECEDENCE/
        REJECTABLE /REJECTABLE/
        TYPEDEFTOK /TYPEDEFTOK/
        LEFTASSOC  /LEFT-ASSOCIATIVE/
        RIGHTASSOC /RIGHT-ASSOCIATIVE/
        NONASSOC   /NON-ASSOCIATIVE/
        ID         /[[:alpha:]_][[:alnum:]_]*/
        INT        /[[:digit:]]+/
        COLON      /:/
		COMMA      /\,/
        SEMI       /;/
push b  LBRACE     /\{/
        RBRACE     /\}/
        LBRKT      /\[/
        RBRKT      /\]/
        AT         /@/
        LT         /</
        GT         />/
        DOT        /\./
        STAR       /\*/
        PLUS       /\+/
        VSEP       /|/
        QUESTION   /\?/
        LPAREN     /\(/
        RPAREN     /\)/

- section b
      = SQSTR    /'([^']|\\.|\\x[[:xdigit:]]+)*'/
      = DQSTR    /"([^"]|\\.|\\x[[:xdigit:]]+)*"/
        DOLLARDOLLAR /\$\$/
        DOLLARNUMBER /\$[[:digit:]]+/
        SRC      /[^\{\}\"\'\$]+|{SQSTR}|{DQSTR}|\$/
push b  LBRACE   /\{/
pop     RBRACE   /\}/

