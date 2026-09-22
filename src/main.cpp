#include "cli.hpp"
#include <iostream>
using namespace std;

int main(int argc,char *argv[]){
    vector<string>args(argv+1,argv+argc);
    return runCLI(args,cout,cerr);
}
