// Project Identifier: 01BD41C3BF016AD7E8B6F837DF18926EC3251350
#include <getopt.h>
#include <iostream>
#include "logman.hpp"
using namespace std;
int main (int argc, char **argv) {
    ios_base::sync_with_stdio(false);
    Logman l;
    l.getOptions(argc, argv);
    l.readFile();
    l.readUserInput();
    return 0;
}