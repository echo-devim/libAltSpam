#include <map>
#include <fstream>
#include <ctype.h>
#include <algorithm>
#include "email.h"

using namespace std;

#define DBWORDS "wc.dat"
#define SW_VERSION "1.0.0"

/*
    Features:
        - Score suspicious sublinks (e.g. with weird domains, count subdomains, etc.)
        - Count the number of non standard characters
        - Weight suspicious words
        - Check if sender is equal to receiver
        - Check differences in reciver, return path, reply to, etc.
*/
class AltSpam {
private:
    map<string, int> word_count;
    string wc_db_path;

public:
    float threshold;
    bool debug;
    AltSpam();
    ~AltSpam();
    bool loadWordCount(string path);
    bool reloadWordCount();
    bool importWordCount(string path);
    bool saveWordCount(string path);
    void addToSpam(EnrichedEmail &email);
    void removeFromSpam(EnrichedEmail &email);
    bool isSpam(EnrichedEmail &email);
    float getSpamScore(EnrichedEmail &email);

};