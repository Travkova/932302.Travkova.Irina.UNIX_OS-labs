#include <iostream>
#include <filesystem>
#include <vector>
#include <fstream>
#include <string>
#include <sstream>
#include <openssl/evp.h>
#include <iomanip>
#include <unordered_map>

using namespace std;

string calculateHash(const string &filepath){
    ifstream fileStream(filepath, ios_base::binary);
    if (!fileStream.is_open()){
        cout << "Error: coudln't open the file" << filepath << endl;
        return "";
    }

    EVP_MD_CTX *hashContext = EVP_MD_CTX_new();
    EVP_DigestInit_ex(hashContext, EVP_sha1(), nullptr);
    char buffer[8192];
    while (fileStream.read(buffer, sizeof(buffer)) || fileStream.gcount() > 0 ) {
        EVP_DigestUpdate(hashContext, buffer, fileStream.gcount());
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_length = 0;
    EVP_DigestFinal_ex(hashContext, hash, &hash_length);
    EVP_MD_CTX_free(hashContext);

    ostringstream ss;
    ss << hex << setfill('0');
    for (unsigned int i = 0; i < hash_length; i++) {
        ss << setw(2) << (int)hash[i];
    }

    return ss.str();
}

void findFiles(vector<string> &fileCollection, const string &directory) {
    for (const auto& entry : filesystem::directory_iterator(directory)) {
        if (filesystem::is_directory(entry)) {
            findFiles(fileCollection, entry.path().string());
        }
        else if (filesystem::is_regular_file(entry)) {
            fileCollection.push_back(entry.path().string());
        }
    }
}

int main(){
    vector<string> foundFiles;
    findFiles(foundFiles, "test_dir");
    unordered_map<string, filesystem::path> hashRegistry;
    cout << "Total files found in the directory: " << foundFiles.size() << "\n\n";

    for (const auto& currentFilePath : foundFiles) {
        cout << "Processing the file: " << currentFilePath << endl;
        string fileHash = calculateHash(currentFilePath);

        if (fileHash.empty()) {
            cout << "The hash was not received. Skip.\n";
            continue;
        }

        cout << "SHA1: " << fileHash << endl;
        auto hashEntry = hashRegistry.find(fileHash);

        if (hashEntry == hashRegistry.end()) {
            hashRegistry[fileHash] = filesystem::path(currentFilePath);
            cout << "The file is unique\n";
        }
        else {
            filesystem::path originalFile = hashEntry->second;
            cout << "Duplicate found\n";
            cout << "Original file: " << originalFile << endl;

            if (filesystem::equivalent(originalFile, currentFilePath)) {
                cout << "The file is already a hard link to the original. Skip.\n";
                continue;
            }
            cout << "Deleting: " << currentFilePath << endl;
            filesystem::remove(currentFilePath);
            filesystem::create_hard_link(originalFile, currentFilePath);
            cout << "A hard link has been created: " << currentFilePath << " -> " << originalFile << "\n" << endl;
        }
        cout << endl;
    }
    cout << "\nProcessing completed\n";
    cout << "Unique files: " << hashRegistry.size() << endl;
    return 0;
}