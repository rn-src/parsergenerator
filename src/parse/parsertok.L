      = WS1        /[[:white:]]+/
      = WS2        /#[^\n]*\n/
      ~ WS         /({WS1}|{WS2})+/
        START      /<start>/
        ERROR      /<error>/
		DISALLOW   /DISALLOW/
		PRECEDENCE /PRECEDENCE/
		REJECTABLE /REJECTABLE/
		TYPEDEF    /TYPEDEF/
		LEFTASSOC  /(l|left)-(assoc|associative)/
		RIGHTASSOC /(r|right)-(assoc|associative)/
		NONASSOC   /(n|non)-(assoc|associative)/
        ID         /[[:alpha:]_][[:alnum:]_]*/
        COLON      /:/
        SEMI       /;/
push b  LBRACE     /\{/
        RBRACE     /\}/
        LBRKT      /\[/
        RBRKT      /\]/
		ARROW      /-+>/
		GT         />/
		DOT        /\./
		STAR       /\*/
		VSEP       /|/

- section b
      = SQSTR   /'([^']|\\.|\\x[[:xdigit:]]+)*'/
      = DQSTR   /"([^"]|\\.|\\x[[:xdigit:]]+)*"/
        SRC     /[^\{\}\"\']+|{SQSTR}|{DQSTR}/
push b  LBRACE  /\{/
pop     RBRACE  /\}/

