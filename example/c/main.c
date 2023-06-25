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
  if( i < argc ) {
    fname = argv[i];
    rdr.rdr.name = fname;
    rdr.f = fopen(fname, "r");
    rdr.close = true;
    if( ! rdr.f ) {
      fprintf(stderr, "Unable to open %s", fname);
      return 1;
    }
  } else {
    rdr.rdr.name = "<stdin>";
    rdr.f = stdin;
    rdr.close = false;
  }
  rdr.rdr.read = (readfnct)filereader_read;
  tokenizer_init(&tok, &tkinfo, &rdr.rdr);
  writer *writer = stderr_writer();
  stack_t *final = 0;
  if( ! (final = (stack_t*)parse(&tok,&prsinfo,&extra,verbosity,writer)) )
    ret = 1;
  printf("%f", extra);
  tokenizer_destroy(&tok);
  if( rdr.close )
    fclose(rdr.f);
  return ret;
}

