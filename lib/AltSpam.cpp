#include "AltSpam.h"

AltSpam::AltSpam() {
    this->threshold = 0.9;
    this->debug = false;
    if (!this->loadWordCount(DBWORDS))
        cerr << "Error: Couldn't load word count file" << endl;

}

AltSpam::~AltSpam() {

}

/* Load word count map */
bool AltSpam::loadWordCount(string path) {
    this->wc_db_path = path;
    ifstream in(path);
    if (in.good()) {
        string input;
        while (getline(in, input)) {
            size_t pos = input.find(" ");
            if (pos != string::npos) {
                string key = input.substr(0, pos);
                int val = stoi(input.substr(pos+1));
                if (this->word_count.find(key) != this->word_count.end()) {
                    this->word_count[key] += val;
                } else {
                    this->word_count[key] = 1;
                }
            } else {
                cerr << "Invalid word count: " << input << endl;
            }
        }
        cout << "Loaded word vector, size: " << this->word_count.size() << endl;
        return true;
    }
    return false;
}

/* Re-load word count map */
bool AltSpam::reloadWordCount() {
    return this->loadWordCount(this->wc_db_path);
}

/* Import word count map adding the values to the existing one */
bool AltSpam::importWordCount(string path) {
    return this->loadWordCount(path);
}

/* Save word count map */
bool AltSpam::saveWordCount(string path) {
    ofstream out(path, ofstream::out);
    if (out.good() && out.is_open()) {
        for (const auto& kv : this->word_count) {
            out << kv.first << " " << kv.second << endl;
        }
        cout << "Saved word vector, size: " << this->word_count.size() << endl;
        return true;
    }
    return false;
}

/* Add words in the email to spam dataset */
void AltSpam::addToSpam(EnrichedEmail &email) {
    if (this->debug) cout << email.subject << " marked as spam" << endl;
    for (string word : email.getWords()) {
        if(this->word_count.find(word) != this->word_count.end()){
            this->word_count[word]++;
        } else {
            this->word_count[word] = 1;
        }
    }
    if (!this->saveWordCount(DBWORDS))
        cerr << "Error: Couldn't save word count file" << endl;
}

/* Remove words in the email from spam dataset */
void AltSpam::removeFromSpam(EnrichedEmail &email) {
    if (this->debug) cout << email.subject << " marked as not spam" << endl;
    for (string word : email.getWords()) {
        if(this->word_count.find(word) != this->word_count.end()){
            if (this->word_count[word] > 1) {
                this->word_count[word]--;
            } else {
                this->word_count.erase(word);
            }
        }
    }
    if (!this->saveWordCount(DBWORDS))
        cerr << "Error: Couldn't save word count file" << endl;
}

bool AltSpam::isSpam(EnrichedEmail &email) {
    float score = this->getSpamScore(email);
    bool result = (score > this->threshold);
    if (this->debug)
        cout << "Analyzed '" << email.subject.substr(0,email.subject.length()>20 ? 20 : email.subject.length()) << "' | SCORE: " << score << " | SPAM: " << result << endl;
    return result;
}


float AltSpam::getSpamScore(EnrichedEmail &email) {
    float spam_score = 0;
    if (this->debug) {
        cout << "Calculating score for " << email.subject << endl;
        cout << "Calculating weighted words average based on word vector" << endl;
    }
    // Calculate weighted words average based on our dataset
    float word_score = 0;
    vector<string> words = email.getWords();
    if (this->debug) {
        if (words.size() < 10) {
            cout << "Words: ";
            for (string w : words)
                cout << w << " ";
            cout << endl;
        }
        cout << "Total words: " << words.size();
    }
    if (words.size() > 0) {
        for (string word : words) {
            if(this->word_count.find(word) != this->word_count.end()){
                word_score += this->word_count[word];
            }
        }
        word_score = word_score / words.size();
    }
    if (this->debug) cout << ", word score: " << word_score << endl;
    // Count non standard characters
    float ns_chars = 0;
    if (this->debug) cout << "Calculating non standard characters";
    if (email.bodyText.size() > 0) {
        for (char c : email.bodyText) {
            if (!isascii(c))
                ns_chars++;
        }
        ns_chars = ns_chars / email.bodyText.size();
    }
    if (this->debug) cout << ", score: " << ns_chars << endl;

    // Check suspicious domains
    float domains_score = 0;
    if (this->debug) cout << "Calculating suspicious domains";
    for (Link link : email.links) {
        //Check if we have a large number of subdomains
        domains_score += (count(link.domain.begin(), link.domain.end(), '.') / 5);
    }
    if (this->debug) cout << ", score: " << domains_score << endl;

    //Check suspicious email addresses
    float address_score = 0;
    if (this->debug) cout << "Calculating suspicious email addresses";
    if (email.from != email.replyto)
        address_score += 0.1;
    if (email.from != email.returnpath)
        address_score += 0.1;
    if (email.from.find(",_") != string::npos) //Check hidden sender
        address_score += 0.2;
    if ((email.returnpath.find("no-reply") != string::npos) || (email.replyto.find("no-reply") != string::npos))
        address_score += 0.1;
    if (this->debug) cout << ", score: " << address_score << endl;
    spam_score = domains_score + address_score + ns_chars + word_score;
    return spam_score;
}