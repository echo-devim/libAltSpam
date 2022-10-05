
#ifndef _CRYPTO_UTIL_
#define _CRYPTO_UTIL_

#include "plusaes.h"
#include <iostream>

using namespace std;

/* Utility Crypto class
    Implements base64 encoding and AES-128
    Recommended usage: Set custom key and iv
*/
class CryptoUtil {
private:
    //Note that the default key and iv are used also by the java application
    vector<unsigned char> key;
    unsigned char iv[16] = {
        0x52,0x37,0x42,0x42,0x6f,0x6d,0x49,0x31,
        0x32,0x74,0x56,0x62,0x63,0x63,0x39,0x30
    };

public:
    CryptoUtil();
    void setKeyfromString(string key);
    void setIV(vector<unsigned char> iv);
    string decrypt(string &text);
    string encrypt(string &text);
    string base64_strencode(string &text);
    string base64_strdecode(string &text);
    string base64_binencode(vector<unsigned char> &text);
    vector<unsigned char> base64_bindecode(string &text);
};

#endif