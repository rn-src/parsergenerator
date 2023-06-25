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
  filereader rdr ={0};
  int verbosity = 0;
  int i = 1;
  while (i < argc && strcmp(argv[i], "-v") == 0) {
    ++i;
    verbosity++;
  }
  rdr.rdr.name = "<stdin>";
  rdr.f = stdin;
  rdr.close = false;
  if( i < argc ) {
    fname = argv[i];
    rdr.rdr.name = fname;
    rdr.rdr.read = (readfnct)filereader_read;
    rdr.f = fopen(fname, "r");
    rdr.close = true;
    if( ! rdr.f )
      return 1;
  }
  tokenizer_init(&tok, &tkinfo, &rdr.rdr);
  if( ! parse(&tok,&prsinfo,&extra,verbosity,stderr_writer()) )
    ret = 1;
  printf("%f", extra);
  tokenizer_destroy(&tok);
  if( rdr.close )
    fclose(rdr.f);
  return ret;
}

