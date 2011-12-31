#include <stdlib.h>
#include <stdio.h>
#include <math.h>

long min(long x, long y)
{
  return x < y ? x : y;
}

int main(int argc, char** argv) 
{
  FILE* f = fopen(argv[1], "r");
  if(f == NULL) {
    fprintf(stderr, "Could not open file: >>%s<<\n", argv[1]);
    return 1;
  }
  long start, end;
  sscanf(argv[2], "%ld", &start);
  sscanf(argv[3], "%ld", &end);

  fprintf(stderr, "Reading file %s, range [%ld, %ld)\n", argv[1], start, end);

  const long buf_size = 1024*1024;
  char buf[buf_size];
  fseek(f, start, SEEK_SET);
  while(ftell(f) < end) {
    long cTORead = min(buf_size, end-ftell(f));
    long cRead = fread(buf, 1, cTORead, f);
    fwrite(buf, 1, cRead, stdout);
  }

  fclose(f);
  return 0;
}
