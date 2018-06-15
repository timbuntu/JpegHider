
#include <cstdlib>
#include <stdint.h>
#include <cstring>
#include <ctime>
#include <string>
#include <iostream>
#include <fstream>

struct hideParams {
    bool encrypt;
    char* src;
    char* dst;
};

struct fileData {
    uint64_t len;
    char* data;
};

fileData getFileData(const char* file) {
    std::ifstream in(file, std::ios::in | std::ios::binary | std::ios::ate);
    fileData data;
    data.len = in.tellg();
    data.data = new char[data.len];

    in.seekg(0, std::ios::beg);
    in.read(data.data, data.len);
    in.close();

    return data;
}

void encryptData(char* data, uint64_t len, const char key) {
    for(uint64_t i = 0; i < len; i++)
        if(data[i] != 0 && data[i] != key)
            data[i] ^= key;
}

void hideData(const char* dst, const char* src) {
    std::srand(std::time(NULL));
    const char key = rand();
    fileData data = getFileData(src);
#ifdef linux
	const char* pos = std::strrchr(src, '/');
	if (pos != NULL)
		src = pos+1;
#endif
#ifdef _WIN32
	const char* pos = std::strrchr(src, '\\');
	if(pos != NULL)
		src = pos+1;
#endif
    uint32_t nameLen = std::strlen(src);
    uint32_t nameLen2 = nameLen;
    uint64_t totalLen = 4 + nameLen + data.len + 9;
    
    encryptData((char*)&nameLen, 4, key);
    encryptData((char*)src, nameLen2, key);
    encryptData(data.data, data.len, key);
    encryptData((char*)&totalLen, 8, key);
    
    std::ofstream of(dst, std::ios::out | std::ios::app | std::ios::binary);
    of.write((char*)&nameLen, 4);
    of.write(src, nameLen2);
    of.write(data.data, data.len);
    of.write((char*)&totalLen, 8);
    of.write(&key, 1);

    of.close();
    std::cout << "Successfully hid data in file." << std::endl;
}

void extractData(const char* file) {
    fileData data = getFileData(file);
    const char key = data.data[data.len-1];
    uint64_t* hiddenLen = (uint64_t*)&data.data[data.len-9];
    encryptData((char*)hiddenLen, 8, key);
    
    char* hiddenData = &data.data[data.len - *hiddenLen];
    *hiddenLen -= 9;
    encryptData(hiddenData, *hiddenLen, key);
    
    uint32_t nameSize = *((uint32_t*)hiddenData);
    hiddenData += 4;
    *hiddenLen -= 4;
    char* fileName = new char[nameSize+1];
    std::strncpy(fileName, hiddenData, nameSize);
	fileName[nameSize] = 0;
    hiddenData += nameSize;
    *hiddenLen -= nameSize;
    
    std::ofstream of(fileName, std::ios::out | std::ios::binary);
    of.write(hiddenData, *hiddenLen);
    of.close();
    std::cout << "Successfully extracted data to " << fileName << std::endl;
}

hideParams menu() {
    hideParams params;
    std::string in;
    do {
        std::cout << "Would you like to (e)ncrypt, or (d)ecrypt?" << std::endl;
        std::cin >> in;
    } while(in != "e" && in != "d");
    
    if(in == "e") {
        params.encrypt = true;
        
        std::cout << "File to encrypt: ";
        std::cin >> in;
        params.src = new char[in.length()];
        std::strcpy(params.src, in.c_str());
        
        std::cout << "File to hide in: ";
        std::cin >> in;
        params.dst = new char[in.length()];
        std::strcpy(params.dst, in.c_str());
        
    } else {
        params.encrypt = false;
        
        std::cout << "File to decrypt: ";
        std::cin >> in;
        params.src = new char[in.length()];
        std::strcpy(params.src, in.c_str());

        params.dst = NULL;
    }

    return params;
}

int main(int argc, char** argv) {
    if(argc == 2)
        extractData(argv[1]);
    else if(argc == 3)
        hideData(argv[2], argv[1]);
    else {
        hideParams params = menu();
        if(params.encrypt)
            hideData(params.dst, params.src);
        else
            extractData(params.src);
    }
    
    return 0;
}
