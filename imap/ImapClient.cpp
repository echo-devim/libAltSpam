#include "ImapClient.h"

static string temp_buf;

ssize_t curl_data_callback(char* buf, size_t size, size_t nmemb, void* userdata) {
    temp_buf.append((char*)buf, size*nmemb);
    return size*nmemb;
}

/* Load settings from file */
ImapClient::ImapClient(string filepath) {
    map<string,string> prop;
    std::ifstream file(filepath);
    if (file.is_open()) {
        std::string line;
        int i = 0;
        while (std::getline(file, line)) {
            size_t pos = line.find('=');
            //Skip lines without assign operator
            if (pos == string::npos)
                continue;
            string key = line.substr(0,pos);
            string value = line.substr(pos+1);
            prop[key] = value;
            i++;
        }
        file.close();
        //folders=folder1,folder2,...
        if (prop.find("folders") != prop.end()) {
            size_t pos = 0;
            string token;
            while ((pos = prop["folders"].find(',')) != std::string::npos) {
                token = prop["folders"].substr(0, pos);
                this->folders.push_back(token);
                prop["folders"].erase(0, pos + 1);
            }
            this->folders.push_back(prop["folders"]);
        } else {
            this->folders.push_back("INBOX");
        }
        CryptoUtil cu;
        this->init(prop["server"], prop["username"], cu.decrypt(prop["password"]));
    } else {
        cerr << "Cannot open imap configuration file " << filepath << endl;
    }
}

ImapClient::ImapClient(string server, string username, string password) {
    this->init(server, username, password);
    this->folders.push_back("INBOX");
}

void ImapClient::init(string server, string username, string password) {
    this->curl = nullptr;
    CURLcode res = CURLE_OK;

    this->curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_USERNAME, username.c_str());
        curl_easy_setopt(curl, CURLOPT_PASSWORD, password.c_str());
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_data_callback);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
        this->url = "imaps://"+server+":993/";
    }
}

ImapClient::~ImapClient() {
    if (this->curl != nullptr)
        curl_easy_cleanup(this->curl);
}

/* Function to fetch a single email by UID from remote folder */
string ImapClient::fetchEmail(int id, string &folder) {
    CURLcode res = CURLE_OK;
    if(this->curl) {
        string url = this->url+folder+";MAILINDEX="+to_string(id);
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, nullptr);
        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
        }
    } else {
        cerr << "curl handle is invalid." << endl;
    }
    string email = temp_buf;
    temp_buf.clear();
    return email;
}

int ImapClient::getEmailCount(string &folder) {
    int ret = 0;
    CURLcode res = CURLE_OK;
    if(this->curl) {
        string url = this->url+folder;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, string("STATUS "+folder+" (MESSAGES)").c_str());
        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
        }
    } else {
        cerr << "curl handle is invalid." << endl;
    }
    string rep = temp_buf;
    temp_buf.clear();
    size_t pos = rep.find_last_of(')');
    if (pos == string::npos) {
        cerr << "Failed to fetch email count. Response from server: " << rep << endl;
    } else {
        size_t start = rep.find_last_of(' ', pos)+1;
        ret = stoi(rep.substr(start, pos-start));
    }
    return ret;
}

vector<int> ImapClient::getUnseenEmailsID(string &folder) {
    vector<int> ids;
    CURLcode res = CURLE_OK;
    if(this->curl) {
        string url = this->url+folder+"?UNSEEN";
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, nullptr);
        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
        }
    } else {
        cerr << "curl handle is invalid." << endl;
    }
    string rep = temp_buf;
    temp_buf.clear();
    size_t pos = rep.find("SEARCH ");
    if (pos == string::npos) {
        cerr << "Cannot find search results for unseen emails. Response: " << rep << endl;
    } else {
        rep = ' ' + rep.substr(pos+7);
        pos = rep.find_last_of(' ');
        while (pos != string::npos) {
            int id = stoi(rep.substr(pos));
            ids.push_back(id);
            rep.erase(pos);
            pos = rep.find_last_of(' ');
        }
    }
    return ids;
}

/* Get last n received emails from each folder specified */
vector<ImapMail> ImapClient::getEmails(int lastn) {
    vector<ImapMail> emails;
    for (string folder : this->folders) {
        int tot = this->getEmailCount(folder);
        int start = 0;
        if (tot > lastn) {
            start = tot - lastn;
        }
        for (int i = tot; i > start; i--) {
            emails.push_back(ImapMail(i, folder, this->fetchEmail(i, folder)));
        }
    }
    return emails;
}

/* Get last n received unseen emails from each folder specified */
vector<ImapMail> ImapClient::getUnseenEmails(int lastn) {
    vector<ImapMail> emails;
    for (string folder : this->folders) {
        vector<int> ids = this->getUnseenEmailsID(folder);
        for (int id : ids) {
            if (lastn < 1)
                break;
            ImapMail mail(id, folder, this->fetchEmail(id, folder));
            emails.push_back(mail);
            //The email is marked as read when we access its content
            //Mark the email as unread
            this->storeFlags(mail, "\\Seen", true);
            lastn--;
        }
    }
    return emails;
}

bool ImapClient::moveToFolder(ImapMail mail, string folderdest) {
    int ret = 0;
    CURLcode res = CURLE_OK;
    if(this->curl) {
        string url = this->url+mail.folder;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, string("COPY "+to_string(mail.id)+" "+folderdest).c_str());
        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
            return false;
        }
    } else {
        cerr << "curl handle is invalid." << endl;
        return false;
    }
    //We copied the email to destination folder, now delete source email
    return deleteMail(mail);
}

bool ImapClient::deleteMail(ImapMail mail) {
    return this->storeFlags(mail, "\\Deleted");
}

/* Function to add or remove flags for the given email */
bool ImapClient::storeFlags(ImapMail mail, string flags, bool remove) {
    int ret = 0;
    CURLcode res = CURLE_OK;
    if(this->curl) {
        string url = this->url+mail.folder;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        string sign = remove ? "-" : "+";
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, string("STORE "+to_string(mail.id)+" "+sign+"Flags "+flags).c_str());
        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
            return false;
        }
    } else {
        cerr << "curl handle is invalid." << endl;
        return false;
    }
    return true;
}