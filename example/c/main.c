#include "exampleG.h"
#include <stdio.h>
#include <string.h>

struct filereader {
  reader rdr;
  FILE *f;
  bool close;
};
typedef struct filereader filereader;

size_t filereader_read(filereader *rdr, size_t n, void *buf) {
  return fread(buf,1,n,rdr->f);
}

int main(int argc, char **argv) {
  char *fname = 0;
  int ret = 0;
  extra_t extra;
  tokenizer tok;
  filereader rdr;
  fname = argv[1];
  rdr.rdr.read = (readfnct)filereader_read;
  if( strcmp(fname,"-") == 0 ) {
    rdr.f = fopen(fname,"r");
    rdr.close = true;
  } else {
    rdr.f = stdin;
    rdr.close = false;
  }
  tokenizer_init(&tok, &tkinfo);
  if( ! parse(&tok,&prsinfo,&extra,0,stderr_writer()) )
    ret = 1;
  tokenizer_destroy(&tok);
  if( rdr.close )
    fclose(rdr.f);
  return ret;
}

