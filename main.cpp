
#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#endif
#include <fstream>
#include <sstream>
#include <iostream>
#include <thread>
#include "lib/AltSpam.h"
#include "cmdline.h"
#include "server.cpp"
#include "imap/ImapClient.h"

#define DEFAULT_MAIL_LIST 10

string readFile(string path) {
    std::ifstream inFile(path);
    string ret = "";
    if (inFile) {
        std::stringstream ss;
        ss << inFile.rdbuf();
        ret = ss.str();
    }
    return ret;
}

int checkSpam(AltSpam &altspam, string imap_conf, string spam_folder, int lastn, bool delete_mail) {
    ImapClient iclient(imap_conf);
    vector<ImapMail> emails = iclient.getUnseenEmails(lastn);
    int detected_spam_count = 0;
    for (ImapMail mail : emails) {
        EnrichedEmail em(mail.content);
        if (altspam.isSpam(em)) {
            detected_spam_count++;
            if (altspam.debug)
                cout << "Detected spam: " << em.subject << endl;
            if (delete_mail) {
                iclient.deleteMail(mail);
            } else {
                iclient.moveToFolder(mail, spam_folder);
            }
        }
    }
    return detected_spam_count;
}

void initIMAP(string confpath) {
    string server, username, password, folders;
    cout << "Interactive IMAP configuration" << endl;
    cout << "Configuration will be saved into " << confpath << endl;
    cout << "Server: ";
    cin >> server;
    cout << "Email: ";
    cin >> username;
    cout << "Password: ";
    cin >> password;
    CryptoUtil cu;
    password = cu.encrypt(password);
    cout << "Folders to monitor split by commas (default INBOX): ";
    cin >> folders;
    if (folders == "") {
        folders = "INBOX";
    }
    ofstream conf(confpath);
    if (conf.is_open()) {
        conf << "server=" << server << endl;
        conf << "username=" << username << endl;
        conf << "password=" << password << endl;
        conf << "folders=" << folders << endl;
    }
    cout << "Configuration saved" << endl;
}

int main(int argc, char *argv[]) {
    cmdline::parser a;
    AltSpam altspam;
    a.add<string>("loadwc", 'l', "load word vector from path", false, "");
    a.add<string>("savewc", 's', "save word vector to path", false, "");
    a.add<string>("importwc", '\0', "import word vector from existing file", false, "");
    a.add<float>("threshold", 't', "set spam threshold", false, 0.9);
    a.add<string>("add-to-spam", 'a', "add an email to spam dataset", false, "");
    a.add<string>("remove-from-spam", 'r', "remove an email from spam dataset", false, "");
    a.add<string>("is-spam", 'i', "check if an email is spam or not", false, "");
    a.add<string>("spam-score", '\0', "get the spam score for the specified email", false, "");
    a.add("list", '\0', "get the latest 10 email for each folder and print their subject to stdout");
    a.add("list-score", '\0', "get the latest 10 email for each folder and print their scores to stdout");
    a.add<int>("monitor", 'm', "monitor the latest 10 email and move to junk spam emails each n minutes (use 0 to run only once)", false, 10);
    a.add<string>("spam-folder", '\0', "set spam folder name where to move spam messages", false, "JUNK");
    a.add<string>("imap-conf", '\0', "imap configuration file", false, "");
    a.add<string>("init-imap", '\0', "interactive imap configuration", false, "prop.ini");
    a.add("verbose", 'v', "show debug prints");
    a.add("delete", 'd', "when combined with monitor option, force spam emails deletion");
    a.add("server", '\0', "start API server");
    a.parse_check(argc, argv);

    altspam.threshold = a.get<float>("threshold");
    if (a.exist("verbose")) {
        cout << "BSD-4 License" << endl << "Copyright (c) 2022, devim" << endl << "All rights reserved." << endl;
        cout << "Setting service into debug mode" << endl;
        cout << "--- AltSpam " << SW_VERSION << " ---" << endl;
        altspam.debug = true;
        cout << "Threshold set to " << altspam.threshold << endl;
    }

    if (altspam.debug && (a.exist("imap-conf"))) {
        string conf = a.get<string>("imap-conf");
        cout << "Using IMAP client" << endl << "Configuration file " << conf;
        cout << readFile(conf) << endl;
    }

    if (a.exist("monitor")) {
        int minutes = a.get<int>("monitor");
        bool delete_mail = a.exist("delete");
        if (altspam.debug)
            cout << "Operation to perform on spam: Delete=" << delete_mail << " MoveToJunk=" << !delete_mail << endl;
        do {
            int detected_spam = checkSpam(altspam, a.get<string>("imap-conf"), a.get<string>("spam-folder"), 10, delete_mail);
            cout << "Detected " << detected_spam << " spam emails" << endl;
            std::this_thread::sleep_for(std::chrono::minutes(minutes));
        } while(minutes > 0);
    } else if (a.exist("list")) {
        ImapClient iclient(a.get<string>("imap-conf"));
        vector<ImapMail> emails = iclient.getUnseenEmails(DEFAULT_MAIL_LIST);
        cout << "ID\tFOLDER\tSUBJECT" << endl;
        int i = 1;
        for (ImapMail mail : emails) {
            EnrichedEmail em(mail.content);
            cout << i << "\t" << mail.folder << "\t" << em.subject << endl;
            i++;
        }
    } else if (a.exist("list-score")) {
        ImapClient iclient(a.get<string>("imap-conf"));
        vector<ImapMail> emails = iclient.getUnseenEmails(DEFAULT_MAIL_LIST);
        cout << "ID\tFOLDER\tSUBJECT\tSPAM\tSCORE" << endl;
        int i = 1;
        for (ImapMail mail : emails) {
            EnrichedEmail em(mail.content);
            cout << i << "\t" << mail.folder << "\t" << em.subject << "\t" << altspam.isSpam(em) << "\t" << altspam.getSpamScore(em) << endl;
            i++;
        }
    } else if (a.exist("spam-score")) {
        if (a.exist("imap-conf")) {
            ImapClient iclient(a.get<string>("imap-conf"));
            vector<ImapMail> emails = iclient.getUnseenEmails(DEFAULT_MAIL_LIST);
            int index = stoi(a.get<string>("spam-score"))-1;
            if (index < 0) index = 0;
            else if (index > emails.size()) index = emails.size(); 
            ImapMail mail = emails[index];
            EnrichedEmail em(mail.content);
            cout << altspam.getSpamScore(em) << endl;
        } else {
            EnrichedEmail mail(readFile(a.get<string>("spam-score")));
            cout << altspam.getSpamScore(mail) << endl;
        }
    } else if (a.exist("is-spam")) {
        if (a.exist("imap-conf")) {
            ImapClient iclient(a.get<string>("imap-conf"));
            vector<ImapMail> emails = iclient.getUnseenEmails(DEFAULT_MAIL_LIST);
            int index = stoi(a.get<string>("is-spam"))-1;
            if (index < 0) index = 0;
            else if (index > emails.size()) index = emails.size(); 
            ImapMail mail = emails[index];
            EnrichedEmail em(mail.content);
            cout << altspam.isSpam(em) << endl;
        } else {
            EnrichedEmail mail(readFile(a.get<string>("is-spam")));
            cout << altspam.isSpam(mail) << endl;
        }
    } else if (a.exist("remove-from-spam")) {
        if (a.exist("imap-conf")) {
            ImapClient iclient(a.get<string>("imap-conf"));
            vector<ImapMail> emails = iclient.getUnseenEmails(DEFAULT_MAIL_LIST);
            int index = stoi(a.get<string>("remove-from-spam"))-1;
            if (index < 0) index = 0;
            else if (index > emails.size()) index = emails.size(); 
            ImapMail mail = emails[index];
            EnrichedEmail em(mail.content);
            altspam.removeFromSpam(em);
        } else {
            EnrichedEmail mail(readFile(a.get<string>("remove-from-spam")));
            altspam.removeFromSpam(mail);
        }
        cout << "Done" << endl;
    } else if (a.exist("add-to-spam")) {
        if (a.exist("imap-conf")) {
            ImapClient iclient(a.get<string>("imap-conf"));
            vector<ImapMail> emails = iclient.getUnseenEmails(DEFAULT_MAIL_LIST);
            int index = stoi(a.get<string>("add-to-spam"))-1;
            if (index < 0) index = 0;
            else if (index > emails.size()) index = emails.size(); 
            ImapMail mail = emails[index];
            EnrichedEmail em(mail.content);
            altspam.addToSpam(em);
        } else {
            EnrichedEmail mail(readFile(a.get<string>("add-to-spam")));
            altspam.addToSpam(mail);
        }
        cout << "Done" << endl;
    } else if (a.exist("importwc")) {
        cout << altspam.importWordCount(a.get<string>("importwc")) << endl;
    } else if (a.exist("savewc")) {
        cout << altspam.saveWordCount(a.get<string>("savewc")) << endl;
    } else if (a.exist("loadwc")) {
        cout << altspam.loadWordCount(a.get<string>("loadwc")) << endl;
    } else if (a.exist("init-imap")) {
        initIMAP(a.get<string>("init-imap"));
    }

    if (a.exist("server")) {
        start_server(altspam);
    }
    return 0;
}
