       ~ WS   /[[:space:]]+/               
       = IDRX /[[:alpha:]][[:alnum:]]*/
         ID   /{IDRX}/
push b   DQ   /\"/

- section b
pop      DQ2 /\"/
         C   /(\\.|[^"])+/

