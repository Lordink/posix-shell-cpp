#include <iostream>
#include <string>

using std::cerr;
using std::cout;
using std::getline;

int main() {
  // Flush after every std::cout / std:cerr
  cout << std::unitbuf;
  cerr << std::unitbuf;

  // Uncomment this block to pass the first stage
  cout << "$ ";

  std::string input;
  getline(std::cin, input);
}
