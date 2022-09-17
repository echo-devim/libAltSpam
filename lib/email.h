#include <string>
#include <iostream>
#include <vector>
#include <set>
#include <sstream>
#include "link.h"

using namespace std;

class Email {
private:
    string getField(string field, string &email);
    void parse(string email);
    string removeHtmlTags(string &html);
    string removeWhiteSpaces(string text);
    string decode(string &text);

public:
    string body;
    string bodyText;
    string headers;
    string subject;
    string replyto;
    string from;
    string returnpath;
    string raw;
    Email(string email);
};


class EnrichedEmail : public Email {
private:
    set<Link> parseLinks(string &html);
public:
    set<Link> links;
    vector<string> getWords();
    EnrichedEmail(string email);
};