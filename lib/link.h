#include <string>
#include <iostream>
#include <vector>

using namespace std;

class Link {
private:
    void parse(string &link);

public:
    string scheme;
    string domain;
    string path;
    string raw;
    Link(string link);
    friend ostream& operator<<(std::ostream &strm, const Link &link);
    friend bool operator< (const Link &left, const Link &right);
};
