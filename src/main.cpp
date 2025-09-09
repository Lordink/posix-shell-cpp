#include <iostream>
#include <string>

using std::cerr;
using std::cout;
using std::endl;
using std::getline;

int main() {
  // Flush after every std::cout / std:cerr
  cout << std::unitbuf;
  cerr << std::unitbuf;

  cout << "$ ";

  std::string input;
  getline(std::cin, input);

  cout << input << ": command not found" << endl;
}
