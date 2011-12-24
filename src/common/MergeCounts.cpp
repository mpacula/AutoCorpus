#include <iostream>
#include "merge.h"

using namespace std;

int main(int argc, char ** argv) {
  char* srcPath1;
  char* srcPath2;

  FILE* srcFile1;
  FILE* srcFile2;

  if(argc != 3) {
    cerr << "Usage: " << argv[0] << " file1 file2" << endl;
    return 1;
  }
  
  srcPath1 = argv[1];
  srcPath2 = argv[2];

  if((srcFile1 = fopen(srcPath1, "r"))) {
    if((srcFile2 = fopen(srcPath2, "r"))) {
      try {
        mergeCounts(srcFile1, srcFile2, stdout);
      } catch(string error) {
        cerr << error << endl;
        return 1;
      }
    } else {
      cerr << "Error opening file" << srcPath2 << endl;
      return 1;
    }
  } else {
    cerr << "Error opening file " << srcPath1 << endl;
    return 1;
  }
  return 0;
}
