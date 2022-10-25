#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <curl/curl.h>
#include "../misc/cryptoutil.h"

using namespace std;

class ImapMail {
public:
    int id;
    string folder;
    string content;
    ImapMail(int id, string folder, string content) {
        this->id = id;
        this->folder = folder;
        this->content = content;
    }
};

/* Partial implementation of imap client based on lincurl */
class ImapClient {
private:
    CURL *curl;
    string url;
    ssize_t callback(char* buf, size_t size, size_t nmemb, void* up);
    void init(string server, string username, string password);
    string fetchEmail(int id, string &folder);
    int getEmailCount(string &folder);
    vector<int> getUnseenEmailsID(string &folder);
    bool storeFlags(ImapMail mail, string flags, bool remove = false);

public:
    vector<string> folders;
    ImapClient(string filepath);
    ImapClient(string server, string username, string password);
    ~ImapClient();
    vector<ImapMail> getEmails(int lastn);
    vector<ImapMail> getUnseenEmails(int lastn);
    bool moveToFolder(ImapMail mail, string folderdest);
    bool deleteMail(ImapMail mail);
    
};