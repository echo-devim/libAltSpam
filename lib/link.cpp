#include "link.h"

Link::Link(string link) {
    this->parse(link);
    this->raw = link;
}

void Link::parse(string &link) {
    size_t pos = link.find("://");
    this->scheme = link.substr(0, pos);
    string partial = link.substr(pos+3);
    pos = partial.find("/");
    if (pos != string::npos) {
        this->domain = partial.substr(0, pos);
        this->path = partial.substr(pos);
    } else {
        this->domain = partial;
        this->path = "";
    }
}

ostream& operator<<(std::ostream &strm, const Link &link) {
    return strm << link.raw;
}

bool operator< (const Link &left, const Link &right) {
    return left.raw < right.raw;
}