      = WS1        /[[:white:]]+/
      = WS2        /#[^\n]*\n/
      ~ WS         /({WS1}|{WS2})+/
        START      /<start>/
        EXTRA      /<extra>/
        ERROR      /<error>/
        PRECEDENCE /PRECEDENCE/
        REJECTABLE /REJECTABLE/
        TYPEDEFTOK /TYPEDEFTOK/
        LEFTASSOC  /LEFT-ASSOCIATIVE/
        RIGHTASSOC /RIGHT-ASSOCIATIVE/
        NONASSOC   /NON-ASSOCIATIVE/
        INT        /[[:digit:]]+/
        COLON      /:|->/
        ARROW      /-->/
        NOTARROW   /-\/->/
        RESTRICT   /RESTRICT/
        COMMA      /\,/
        SEMI       /;/
        ID         /[[:alpha:]_][[:alnum:]_]*/
push b  LBRACE     /\{/
        RBRACE     /\}/
        LBRKT      /\[/
        RBRKT      /\]/
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
        DOLLAREXTRA  /\$extra/
        DOLLARDOLLAR /\$\$/
        DOLLARNUMBER /\$[[:digit:]]+/
        SRC      /[^\{\}\"\'\$]+|{SQSTR}|{DQSTR}|\$/
push b  LBRACE   /\{/
pop     RBRACE   /\}/

