#include "cryptoutil.h"

CryptoUtil::CryptoUtil() {
    this->key = plusaes::key_from_string(&"EncD3f4ultK[y128"); // 16-char = 128-bit
}

void CryptoUtil::setKeyfromString(string key) {
    if (key.size() != 17)
        cerr << "Wrong key size, falling back to default key" << endl;
    else
        this->key = plusaes::key_from_string((const char (*)[17])key.data());
}

void CryptoUtil::setIV(vector<unsigned char> iv) {
    if (iv.size() < 16) {
        cerr << "Wrong iv size, falling back to default iv" << endl;
    } else {
        for (int i = 0; i < 16; i++) {
            this->iv[i] = iv[i];
        }
    }
}

string CryptoUtil::decrypt(string &text) {
    vector<unsigned char> in = base64_bindecode(text);
    unsigned long padded_size = 0;
    string decrypted(in.size(), '\0');

    plusaes::decrypt_cbc(&in[0], in.size(), &key[0], key.size(), &iv, (unsigned char*)decrypted.data(), decrypted.size(), &padded_size);

    return decrypted;
}

string CryptoUtil::encrypt(string &text) {
    const unsigned long encrypted_size = plusaes::get_padded_encrypted_size(text.size());
    std::vector<unsigned char> encrypted(encrypted_size);
    plusaes::encrypt_cbc((unsigned char*)text.data(), text.size(), &key[0], key.size(), &iv, &encrypted[0], encrypted.size(), true);
    return base64_binencode(encrypted);
}

string CryptoUtil::base64_strencode(string &text) {

    std::string out;

    int val = 0, valb = -6;
    for (unsigned char c : text) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[(val>>valb)&0x3F]);
            valb -= 6;
        }
    }
    if (valb>-6) out.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[((val<<8)>>(valb+8))&0x3F]);
    while (out.size()%4) out.push_back('=');
    return out;
}

string CryptoUtil::base64_binencode(vector<unsigned char> &text) {

    std::string out;

    int val = 0, valb = -6;
    for (unsigned char c : text) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[(val>>valb)&0x3F]);
            valb -= 6;
        }
    }
    if (valb>-6) out.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[((val<<8)>>(valb+8))&0x3F]);
    while (out.size()%4) out.push_back('=');
    return out;
}


string CryptoUtil::base64_strdecode(string &text) {

    std::string out;

    std::vector<int> T(256,-1);
    for (int i=0; i<64; i++) T["ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[i]] = i;

    int val=0, valb=-8;
    for (unsigned char c : text) {
        if ((c == '\r') || (c == '\n'))
            continue;
        if (T[c] == -1) break;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0) {
            out.push_back(char((val>>valb)&0xFF));
            valb -= 8;
        }
    }
    return out;
}

vector<unsigned char> CryptoUtil::base64_bindecode(string &text) {

    vector<unsigned char> out;

    std::vector<int> T(256,-1);
    for (int i=0; i<64; i++) T["ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[i]] = i;

    int val=0, valb=-8;
    for (unsigned char c : text) {
        if (T[c] == -1) break;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0) {
            out.push_back(char((val>>valb)&0xFF));
            valb -= 8;
        }
    }
    return out;
}