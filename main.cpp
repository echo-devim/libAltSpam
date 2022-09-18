
#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#endif
#include <fstream>
#include <sstream>
#include <iostream>
#include "lib/AltSpam.h"
#include "cmdline.h"
#include "server.cpp"


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
    a.add("verbose", 'v', "show debug prints");
    a.add("server", '\0', "start API server");
    a.parse_check(argc, argv);

    altspam.threshold = a.get<float>("threshold");
    if (a.exist("verbose")) {
        cout << "Setting service into debug mode" << endl;
        cout << "--- AltSpam " << SW_VERSION << " ---" << endl;
        cout << "BSD-4 License" << endl << "Copyright (c) 2022, devim" << endl << "All rights reserved." << endl;
        altspam.debug = true;
        cout << "Threshold set to " << altspam.threshold << endl;
    }

    if (a.exist("spam-score")) {
        EnrichedEmail mail(readFile(a.get<string>("spam-score")));
        cout << altspam.getSpamScore(mail) << endl;
    } else if (a.exist("is-spam")) {
        EnrichedEmail mail(readFile(a.get<string>("is-spam")));
        cout << altspam.isSpam(mail) << endl;
    } else if (a.exist("remove-from-spam")) {
        EnrichedEmail mail(readFile(a.get<string>("remove-from-spam")));
        altspam.removeFromSpam(mail);
        cout << "Done" << endl;
    } else if (a.exist("add-to-spam")) {
        EnrichedEmail mail(readFile(a.get<string>("add-to-spam")));
        altspam.addToSpam(mail);
        cout << "Done" << endl;
    } else if (a.exist("importwc")) {
        cout << altspam.importWordCount(a.get<string>("importwc")) << endl;
    } else if (a.exist("savewc")) {
        cout << altspam.saveWordCount(a.get<string>("savewc")) << endl;
    } else if (a.exist("loadwc")) {
        cout << altspam.loadWordCount(a.get<string>("loadwc")) << endl;
    }

    if (a.exist("server")) {
        start_server(altspam);
    }
    return 0;
}
