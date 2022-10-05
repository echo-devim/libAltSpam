#include "email.h"

Email::Email(string email) {
    this->subject = "";
    this->body = "";
    this->headers = "";
    this->from = "";
    this->replyto = "";
    this->returnpath = "";
    this->parse(email);
    this->bodyText = this->removeHtmlTags(this->body);
    this->bodyText = this->removeWhiteSpaces(this->bodyText);
}

string Email::getField(string field, string &email) {
    string ret = "";
    field += ": ";
    size_t pos = email.find(field);
    if (pos != string::npos) {
        pos += field.length();
        size_t end = email.find("\r\n", pos);
        ret = email.substr(pos, end-pos);
    } else {
        //cerr << "Couldn't find '" << field << "' field in the email" << endl;
    }
    return ret;
}

string Email::removeHtmlTags(string &html) {
    size_t pos = html.find("<body");
    string ret;
    if (pos != string::npos)
        ret = html.substr(pos);
    else
        ret = html;
    pos = html.find("<");
    size_t end = 0;
    while (pos != string::npos) {
        pos = ret.find("<", end);
        if (pos == string::npos)
            break;
        end = ret.find(">", pos);
        if (end == string::npos)
            break;
        ret = ret.substr(0, pos) + ret.substr(end+1);
        // We cut some text from body, adjust the position. In this way the next search will restart from the correct place
        end = end - pos;
    }
    return ret;
}

string Email::removeWhiteSpaces(string text) {
    size_t pos = text.find("  ");
    while (pos != string::npos) {
        if ((pos < text.length()-3) && (text[pos+2] == '\r'))
            text.replace(pos, 2, "");
        else
            text.replace(pos, 2, " ");
        pos = text.find("  ", pos);
    }
    pos = text.find("\t");
    while (pos != string::npos) {
        text.erase(pos, 1);
        pos = text.find("\t", pos);
    }
    pos = text.find("\r");
    while (pos != string::npos) {
        text.erase(pos, 1);
        pos = text.find("\r", pos);
    }
    pos = text.find("\n\n");
    while (pos != string::npos) {
        text.replace(pos, 2, "\n");
        pos = text.find("\n\n", pos);
    }
    return text;
}

void Email::parse(string email) {
    this->raw = email;
    this->subject = this->getField("Subject", email);
    size_t pos = this->subject.find("-8?B?"); //it can be UTF-8?B? or utf-8?B? (case sensitive)
    if (pos != string::npos) {
        //Email subject is base64 encoded
        pos = pos+5;
        string enc = this->subject.substr(pos, this->subject.length()-pos-2);
        CryptoUtil cu;
        this->subject = cu.base64_strdecode(enc);
    }
    this->from = this->getField("From", email);
    this->replyto = this->getField("Reply-To", email);
    this->returnpath = this->getField("Return-Path", email);
    pos = email.find("\r\n\r\n");
    //Email format: HEADERS\r\nBODY+ATTACHMENTS
    if (pos != string::npos) {
        this->headers = email.substr(0, pos);
        this->body = email.substr(pos+4);
        size_t endHtml = this->body.find("</html>");
        if (endHtml != string::npos)
            this->body = this->body.substr(0, endHtml);
        //Remove multiline strings
        size_t pos = this->body.find("=\r\n");
        while (pos != string::npos) {
            this->body.erase(pos, 3);
            pos = this->body.find("=\r\n", pos);
        }
        //Eventually decode base64 payload
        if (this->getField("Content-Transfer-Encoding", this->headers) == "base64") {
            CryptoUtil cu;
            this->body = cu.base64_strdecode(this->body);
        }
    } else {
        this->headers = email;
        //cerr << "Couldn't find body field in the email" << endl;
    }
}





/* Enriched Email methods implementation */

EnrichedEmail::EnrichedEmail(string email) : Email(email) {
    this->links = this->parseLinks(this->body);
}

set<Link> EnrichedEmail::parseLinks(string &html) {
    set<Link> ret;
    size_t pos = html.find("http");
    size_t end;
    while (pos != string::npos) {
        //Check if we really found a link
        if ((pos < html.length()-10) && ((html[pos+4] == ':') || ((html[pos+4] == 's') && (html[pos+5] == ':')))) {
            //Check the end of the link based on one of these terminator characters: ", ' or space
            end = html.find_first_of(" '\"", pos);
            if (end != string::npos) {
                Link link(html.substr(pos,end-pos));
                ret.insert(link);
            }
        }
        pos = html.find("http", pos + 1);
    }
    return ret;
}

vector<string> EnrichedEmail::getWords() {
    vector<string> words;
    std::istringstream ss(this->from + " " + this->subject + " " + this->bodyText);
    std::string token;

    while(ss >> token) {
        //Remove punctuation, new lines or trailing white spaces
        token.erase(0, token.find_first_not_of("\t\n\v\f\r ,.'\":;-!?()")); // left trim
        token.erase(token.find_last_not_of("\t\n\v\f\r ,.'\":;-!?()") + 1); // right trim

        //Skip html tags
        if (token[0] == '<')
            continue;

        //Convert characters to lower case
        for(char &ch : token){
            ch = tolower(ch);
        }

        // Filter out words too short or too long and special html entities
        if ((token[0] != '&') && (token.length() > 3) && (token.length() < 25)) {
            words.push_back(token);
        }
    }

    return words;
}