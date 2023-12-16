#include <stdlib.h>
#include <iostream>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>
#include <string>
#include <vector>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sstream>
#include <filesystem>
#include <cstdio>
#include <ctime>
#include <cstring>
#include <iomanip>
#include <fstream>





#define BUFSIZE 128
#define MAXQUEUE 5
#define BUFFER_SIZE 128
#define CHUNCK_SIZE 1024

extern int errno;

using namespace std;

char* PORT = nullptr; // Initializing to nullptr
bool verbose = false;


volatile sig_atomic_t exitFlag = 0;


vector<string> splitString(const std::string& input) {
    istringstream iss(input);
    vector<std::string> words;
    string word;

    while (iss >> word) {
        words.push_back(word);
    }

    return words;
}

//funcao que le o ficheiro START_(AID).txt e devolve o seu conteudo
string getAuctionStart(string AID) {
    string startFileName = "AUCTIONS/" + AID + "/START_" + AID + ".txt";
    ifstream file(startFileName);

    if (!file.is_open()) {
        return ""; // Return an empty string if the file couldn't be opened
    }

    string line;
    if (getline(file, line)) { // Read a line from the file
        istringstream iss(line); // Create a stringstream from the line
        string start;
        iss >> start; // Extract the first word from the line

        // Extract the remaining words (if any) from the line
        string remaining_words;
        getline(iss, remaining_words);
        start += remaining_words; // Append the remaining words to the start string

        return start;
    }

    return ""; // Return an empty string if the file is empty
}

void parseArguments(int argc, char *argv[]) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-p" && i + 1 < argc) {
            const char* portStr = argv[i + 1];
            PORT = new char[strlen(portStr) + 1]; // Allocate memory for PORT
            strcpy(PORT, portStr); // Copy the string
        } else if (arg == "-v") {
            verbose = true;
        }
    }

    if (PORT == nullptr) {
        // PORT is not provided, setting default value based on GN
        int defaultPort = 58077;
        PORT = new char[10]; // Allocate memory for PORT
        sprintf(PORT, "%d", defaultPort);
    }
}

int getTimeDifference(int givenTime) {
    time_t currentTime = time(nullptr);
    return static_cast<int>(currentTime - givenTime);
}





string getCurrentTime() {
    time_t fulltime;
    struct tm *currenttime;

    time(&fulltime); // Get current time in seconds starting at 1970...
    currenttime = gmtime(&fulltime); // Convert time to struct tm

    // Create a stringstream for formatting the time
    std::stringstream ss;

    // Use stream manipulators to format the time
    ss << setfill('0') << setw(4) << (currenttime->tm_year + 1900) << "-"
       << setw(2) << (currenttime->tm_mon + 1) << "-"
       << setw(2) << currenttime->tm_mday << " "
       << setw(2) << currenttime->tm_hour << ":"
       << setw(2) << currenttime->tm_min << ":"
       << setw(2) << currenttime->tm_sec;

    // Return the formatted time as a string
    return ss.str();
}

//funcao que cria um ficheiro END_(AID).txt na diretoria AUCTIONS/AID, onde dentro do ficheiro tem a data atual no formato YYYY-MM-DD HH:MM:SS e o tempo usando a funcao time() onde o AID é uma string
int CreateEnd(string AID, string end_sec_time) {
    char endFileName[50]; // Increased buffer size to accommodate longer file paths
    FILE *fp;
    time_t fulltime;
    struct tm *currenttime;
    char timestr[25];

    if (AID.size() != 3)
        return 0;

    time(&fulltime);
    currenttime = gmtime(&fulltime);

    strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", currenttime);

    snprintf(endFileName, sizeof(endFileName), "AUCTIONS/%s/END_%s.txt", AID.c_str(), AID.c_str());
    fp = fopen(endFileName, "w");

    if (fp == NULL)
        return 0;

    fprintf(fp, "%s %s", timestr, end_sec_time.c_str());
    fclose(fp);

    return 1;
}


char* processCharData(const char* input, int bytes) {
    int length = 0;

    // Encontrar o comprimento da string até o número de bytes especificado
    while (length < bytes && input[length] != '\0') {
        length++;
    }

    // Alocar memória para o novo array de caracteres
    char* result = new char[length + 1];

    // Copiar os caracteres até o número especificado de bytes para o novo array
    strncpy(result, input, length);

    // Adicionar o caractere nulo no final do novo array
    result[length] = '\0';

    return result;
}



bool isNumeric(const std::string& str) {
    for (char c : str) {
        if (!std::isdigit(c)) {
            return false;
        }
    }
    return true;
}

bool isAlphaNumeric(const std::string& str) {
    for (char c : str) {
        if (!std::isalnum(c)) {
            return false;
        }
    }
    return true;
}




//cria uma funcao que ve se um diretorio com um certo path existe e devolve true or false
bool directoryExists(const std::string& path) {
    struct stat info;
    if (stat(path.c_str(), &info) != 0) {
        return false;
    }
    return (info.st_mode & S_IFDIR) != 0;
}




//cria o direcorio AUCTIONS e USERS

int create_initial_Directories(){
    int ret;
    ret = mkdir("AUCTIONS", 0700);
    if (ret == -1)
        return 0;
    ret = mkdir("USERS", 0700);
    if (ret == -1)
        return 0;
    return 1;
}

//funcao que recebe uma string e devolve a mesma string com 6 algarismos metendo 0s a esquerda se necessario
string padLeft(string str, int n) {
    if (str.size() >= n)
        return str;

    string paddedStr = str;

    while (paddedStr.size() < n)
        paddedStr = "0" + paddedStr;

    return paddedStr;
}


int CreateAUCTIONDir(int AID) {
    char AID_dirname[15];
    char BIDS_dirname[20];
    int ret;
    
    if (AID < 1 || AID > 999)
        return 0;
    
    sprintf(AID_dirname, "AUCTIONS/%03d", AID);
    ret = mkdir(AID_dirname, 0700);
    
    if (ret == -1)
        return 0;
    
    sprintf(BIDS_dirname, "AUCTIONS/%03d/BIDS", AID);
    ret = mkdir(BIDS_dirname, 0700);
    
    if (ret == -1) {
        rmdir(AID_dirname);
        return 0;
    }
    
    return 1;
}

int CreateUSERDir(int UID) {
    char UID_dirname[15];
    char HOSTED_dirname[20];
    char BIDDED_dirname[20];
    int ret;
    
    if (UID < 1 || UID > 999999)
        return 0;
    
    sprintf(UID_dirname, "USERS/%06d", UID);
    ret = mkdir(UID_dirname, 0700);
    
    if (ret == -1)
        return 0;
    
    sprintf(HOSTED_dirname, "USERS/%06d/HOSTED", UID);
    ret = mkdir(HOSTED_dirname, 0700);
    
    if (ret == -1) {
        rmdir(UID_dirname);
        return 0;
    }

    sprintf(BIDDED_dirname, "USERS/%06d/BIDDED", UID);
    ret = mkdir(BIDDED_dirname, 0700);

    if (ret == -1) {
        rmdir(HOSTED_dirname);
        rmdir(UID_dirname);
        return 0;
    }

    return 1;
}





void safe_stop(int signal) {
    cout << "\nCtrl+C detected. Initiating graceful shutdown..." << endl;
    exitFlag = 1;
}






//Cria ficheiro com o login
int CreateLogin(char *UID) {
    char loginName[35];
    FILE *fp;

    if (strlen(UID) != 6)
        return 0;

    sprintf(loginName, "USERS/%s/%s_login.txt", UID, UID);
    fp = fopen(loginName, "w");

    if (fp == NULL)
        return 0;

    fprintf(fp, "Logged in\n");
    fclose(fp);

    return 1;
}

//cria um ficheiro com a password
int CreatePassword(char *UID, char *password) {
    char passwordName[35];
    FILE *fp;

    if (strlen(UID) != 6)
        return 0;

    sprintf(passwordName, "USERS/%s/%s_password.txt", UID, UID);
    fp = fopen(passwordName, "w");

    if (fp == NULL)
        return 0;

    fprintf(fp, "%s\n", password);
    fclose(fp);

    return 1;
}

//funcao que abre o ficheiro de password e le o conteudo e devolve a password
int ReadPassword(char *UID, char *password) {
    char passwordName[35];
    FILE *fp;

    if (strlen(UID) != 6)
        return 0;

    sprintf(passwordName, "USERS/%s/%s_password.txt", UID, UID);
    fp = fopen(passwordName, "r");

    if (fp == NULL)
        return 0;

    fgets(password, 255, fp); // Read up to 255 characters or until newline
    fclose(fp);

    // Remove newline character if present
    size_t len = strlen(password);
    if (len > 0 && password[len - 1] == '\n')
        password[len - 1] = '\0';


    return 1;
}


//funcao que ve se o ficheiro de password existe na diretoria do user
int CheckPassword(char *UID) {
    char passwordName[35];
    FILE *fp;

    if (strlen(UID) != 6)
        return 0;

    sprintf(passwordName, "USERS/%s/%s_password.txt", UID, UID);
    fp = fopen(passwordName, "r");

    if (fp == NULL)
        return 0;

    fclose(fp);

    return 1;
}
//funcao que ve se o ficheiro de login existe na diretoria do user
int CheckLogin(char *UID) {
    char loginName[35];
    FILE *fp;

    if (strlen(UID) != 6)
        return 0;

    sprintf(loginName, "USERS/%s/%s_login.txt", UID, UID);
    fp = fopen(loginName, "r");

    if (fp == NULL)
        return 0;

    fclose(fp);

    return 1;
}


//Apaga ficheiro com o login
int EraseLogin(char *UID) {
    char loginName[35];

    if (strlen(UID) != 6)
        return 0;

    sprintf(loginName, "USERS/%s/%s_login.txt", UID, UID);
    unlink(loginName);

    return 1;
}

//Apaga ficheiro com a password
int ErasePassword(char *UID) {
    char passwordName[35];

    if (strlen(UID) != 6)
        return 0;

    sprintf(passwordName, "USERS/%s/%s_password.txt", UID, UID);
    unlink(passwordName);

    return 1;
}

//ve se a diretoria USERS/UID/HOSTED está vazia e devolve true or false
bool isHostedEmpty(char *UID) {
    char hostedDirName[35];
    DIR *dir;
    struct dirent *ent;

    if (strlen(UID) != 6)
        return false;

    sprintf(hostedDirName, "USERS/%s/HOSTED", UID);
    dir = opendir(hostedDirName);

    if (dir == NULL)
        return false;

    while ((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
            closedir(dir);
            return false;
        }
    }

    closedir(dir);
    return true;
}

//funcao que devolve uma lista com os valores dos AID que estao na diretoria USERS/UID/HOSTED
vector<int> getHostedAIDs(char *UID) {
    char hostedDirName[35];
    DIR *dir;
    struct dirent *ent;
    vector<int> AIDs;

    if (strlen(UID) != 6)
        return AIDs;

    sprintf(hostedDirName, "USERS/%s/HOSTED", UID);
    dir = opendir(hostedDirName);

    if (dir == NULL)
        return AIDs;

    while ((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
            AIDs.push_back(atoi(ent->d_name));
        }
    }

    closedir(dir);
    return AIDs;
}

//ve se na diretoria AUCTIONS/AID tem um ficheiro END_(AID).txt e devolve true or false
bool isAuctionEnded(int AID) {
    char endFileName[35];
    FILE *fp;

    if (AID < 1 || AID > 999)
        return false;

    string AID_string = padLeft(to_string(AID), 3);
    sprintf(endFileName, "AUCTIONS/%s/END_%s.txt", AID_string.c_str(),AID_string.c_str());
    fp = fopen(endFileName, "r");

    if (fp == NULL)
        return false;

    fclose(fp);
    return true;
}



//ve se a diretoria USERS/UID/BIDDED está vazia e devolve true or false
bool isBiddedEmpty(char *UID) {
    char biddedDirName[35];
    DIR *dir;
    struct dirent *ent;

    if (strlen(UID) != 6)
        return false;

    sprintf(biddedDirName, "USERS/%s/BIDDED", UID);
    dir = opendir(biddedDirName);

    if (dir == NULL)
        return false;

    while ((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
            closedir(dir);
            return false;
        }
    }

    closedir(dir);
    return true;
}

string remove_slashN(string input) {
    size_t lastNonNewline = input.find_last_not_of("\n");

    if (lastNonNewline != string::npos) {
        return input.substr(0, lastNonNewline + 1);
    } else {
        // Se a string consistir apenas de quebras de linha, retorna uma string vazia
        return "";
    }
}

//funcao que devolve uma lista com os valores dos AID que estao na diretoria USERS/UID/BIDDED
vector<int> getBiddedAIDs(char *UID) {
    char biddedDirName[35];
    DIR *dir;
    struct dirent *ent;
    vector<int> AIDs;

    if (strlen(UID) != 6)
        return AIDs;

    sprintf(biddedDirName, "USERS/%s/BIDDED", UID);
    dir = opendir(biddedDirName);

    if (dir == NULL)
        return AIDs;

    while ((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
            AIDs.push_back(atoi(ent->d_name));
        }
    }

    closedir(dir);
    return AIDs;
}

//funcao que ve se a diretoria AUCTIONS esta vazia e devolve true or false
bool isAuctionsEmpty() {
    DIR *dir;
    struct dirent *ent;

    dir = opendir("AUCTIONS");

    if (dir == NULL)
        return false;

    while ((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
            closedir(dir);
            return false;
        }
    }

    closedir(dir);
    return true;
}





//funcao que devolve um vetor com os valores dos AID que estao na diretoria AUCTIONS
vector<int> getAIDs() {
    DIR *dir;
    struct dirent *ent;
    vector<int> AIDs;

    dir = opendir("AUCTIONS");

    if (dir == NULL)
        return AIDs;

    while ((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
            AIDs.push_back(atoi(ent->d_name));
        }
    }

    closedir(dir);
    return AIDs;
}


bool isAuction(const string& AID) {
    int parsed_AID;
    stringstream ss(AID);
    ss >> parsed_AID;

    if (ss.fail() || ss.peek() != EOF) {
        // A conversão falhou ou a string contém caracteres não numéricos
        return false;
    }

    if (parsed_AID < 1 || parsed_AID > 999) {
        // Fora do intervalo válido
        return false;
    }

    string AID_dirname = "AUCTIONS/" + AID;
    DIR *dir = opendir(AID_dirname.c_str());

    if (dir == nullptr) {
        return false;
    }

    closedir(dir);
    return true;
}


//funcao que le o ficheiro END_(AID).txt e devolve o seu conteudo
string getAuctionEnd(int AID) {
    char endFileName[50];
    FILE *fp;
    char end[1024]; // Adjust the size based on your maximum expected line length

    if (AID < 1 || AID > 999)
        return "";

    string AID_string = padLeft(to_string(AID), 3);
    sprintf(endFileName, "AUCTIONS/%s/END_%s.txt", AID_string.c_str(), AID_string.c_str());
    fp = fopen(endFileName, "r");

    if (fp == NULL)
        return "";

    // Read entire line from file
    if (fgets(end, sizeof(end), fp) == NULL) {
        fclose(fp);
        return "";
    }

    fclose(fp);

    return end;
}
//funcao que ve se o ficheiro END_(AID).txt existe na diretoria AUCTIONS/AID
bool CheckAuctionEnd(int AID) {
    char endFileName[35];
    FILE *fp;

    if (AID < 1 || AID > 999)
        return false;

    sprintf(endFileName, "AUCTIONS/%03d/END_%03d.txt", AID, AID);
    fp = fopen(endFileName, "r");

    if (fp == NULL)
        return false;

    fclose(fp);
    return true;
}


//funcao que cria um vector separa os valores das bids VVVVVV de um ficheiro VVVVVV.txt na diretoria AUCTIONS/AID/BIDS
vector<int> getBids(string AID) {
    char bidsDirName[35];
    DIR *dir;
    struct dirent *ent;
    vector<int> bids;


    sprintf(bidsDirName, "AUCTIONS/%s/BIDS", AID.c_str());
    dir = opendir(bidsDirName);

    if (dir == NULL)
        return bids;

    while ((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
            bids.push_back(atoi(ent->d_name));
        }
    }

    closedir(dir);
    return bids;
}

//funcao que recebe um vector de inteiros e devolve um novo vetor com o maximo de 50 maiores elementos
vector<int> get50Bids(vector<int> bids) {
    vector<int> fiftyBids;
    int max, maxIndex;

    while (fiftyBids.size() < 50 && bids.size() > 0) {
        max = bids[0];
        maxIndex = 0;

        for (int i = 1; i < bids.size(); i++) {
            if (bids[i] > max) {
                max = bids[i];
                maxIndex = i;
            }
        }

        fiftyBids.push_back(max);
        bids.erase(bids.begin() + maxIndex);
    }

    return fiftyBids;
}


string getBid(int AID, int VVVVVV) {
    char bidFileName[50];
    FILE *fp;
    char bid[1024]; // Adjust the size based on your maximum expected line length

    if (AID < 1 || AID > 999 || VVVVVV < 1 || VVVVVV > 999999)
        return "";

    string AID_string = padLeft(to_string(AID), 3);
    string VVVVVV_string = padLeft(to_string(VVVVVV), 6);
    sprintf(bidFileName, "AUCTIONS/%s/BIDS/%s.txt", AID_string.c_str(), VVVVVV_string.c_str());
    fp = fopen(bidFileName, "r");

    if (fp == NULL)
        return "";

    // Read entire line from file
    if (fgets(bid, sizeof(bid), fp) == NULL) {
        fclose(fp);
        return "";
    }

    fclose(fp);

    return bid;
}

// int main() {
//     time_t fulltime;
//     struct tm *currenttime;
//     char timestr[20];

//     time(&fulltime); // Get current time in seconds starting at 1970...

//     currenttime = gmtime(&fulltime); // Convert time to YYYY-MM-DD HH:MM:SS. currenttime points to a struct of type tm

//     sprintf(timestr, "%4d-%02d-%02d %02d:%02d:%02d",
//             currenttime->tm_year + 1900, currenttime->tm_mon + 1, currenttime->tm_mday,
//             currenttime->tm_hour, currenttime->tm_min, currenttime->tm_sec);

//     printf("Formatted time string: %s\n", timestr);

//     return 0;
// }

string loginprocess (string user, string password){
    string buffer = "RLI";
    if (directoryExists("USERS/" + user) == false || CheckPassword((char*)user.c_str()) == false){
        CreateUSERDir(stoi(user));
        CreateLogin((char*)user.c_str());
        CreatePassword((char*)user.c_str(), (char*)password.c_str());
        buffer = buffer + " REG";
    
    }else{
        //le a password do ficheiro e guada numa variavel chamada password2

        string password2 = "default_password"; // Provide a default password or empty string
        char retrievedPassword[256];
        if (ReadPassword((char*)user.c_str(), retrievedPassword)) {
            password2 = retrievedPassword; // Assign the retrieved password to password2
        }
        if (password2 == password){
            //escreve no ficheiro login que o user esta logged in
            CreateLogin((char*)user.c_str());
            buffer = buffer + " OK";
        }else{
            buffer = buffer + " NOK";
        }
    }
    
    return buffer ;
}

string logoutprocess (string user, string password){
    string buffer = "RLO";
    if (directoryExists ("USERS/" + user) == false || (CheckPassword((char*)user.c_str()) == false && CheckLogin((char*)user.c_str()) == false)){
        buffer = buffer + " UNR";
    }else{
        if (CheckLogin((char*)user.c_str()) == false){
            buffer = buffer + " NOK";
        }else{
            EraseLogin((char*)user.c_str());
            buffer = buffer + " OK";
        }
    }
    return buffer;
}

string unregisterprocess (string user, string password){
    string buffer = "RUR";
    if (directoryExists ("USERS/" + user) == false || (CheckPassword((char*)user.c_str()) == false && CheckLogin((char*)user.c_str()) == false)){
        buffer = buffer + " UNR";
    }else{
        if (CheckLogin((char*)user.c_str()) == false){
            buffer = buffer + " NOK";
        }else{
            EraseLogin((char*)user.c_str());
            ErasePassword((char*)user.c_str());
            buffer = buffer + " OK";
        }
    }
    
    
    
    return buffer;
}

string my_auctionsprocess(string user){
    string buffer = "";
    if (CheckLogin((char*)user.c_str()) == false){
        buffer = "RMA NLG";
    }else if (isHostedEmpty((char*)user.c_str()) == true){
        buffer = "RMA NOK";
    }else{
        buffer = "RMA OK";
        vector<int> AIDs = getHostedAIDs((char*)user.c_str());
        for (int i = 0; i < AIDs.size(); i++){
            if (isAuctionEnded(AIDs[i]) == true){
                buffer = buffer + " " + to_string(AIDs[i]) + " " + "0";
            }else{
                buffer = buffer + " " + to_string(AIDs[i]) + " " + "1";
            }
        }
    }

    return buffer;
}

string my_bids_process (string user){
    string buffer = "";
    if (CheckLogin((char*)user.c_str()) == false){
        buffer = "RMB NLG";
    }else if (isBiddedEmpty((char*)user.c_str()) == true){
        buffer = "RMB NOK";
    }else{
        buffer = "RMB OK";
        vector<int> AIDs = getBiddedAIDs((char*)user.c_str());
        for (int i = 0; i < AIDs.size(); i++){
            if (isAuctionEnded(AIDs[i]) == true){
                buffer = buffer + " " + to_string(AIDs[i]) + " " + "0";
            }else{
                buffer = buffer + " " + to_string(AIDs[i]) + " " + "1";
            }
        }
    }
    return buffer;
}

string list_process (){
    string buffer = "";
    if (isAuctionsEmpty() == true){
        buffer = buffer + "RLS NOK";
    }else{
        buffer = buffer + "RLS OK";
        vector<int> AIDs = getAIDs();
        for (int i = 0; i < AIDs.size(); i++){
            if (isAuctionEnded(AIDs[i]) == true){
                buffer = buffer + " " + to_string(AIDs[i]) + " " + "0";
            }else{
                buffer = buffer + " " + to_string(AIDs[i]) + " " + "1";
            }
        }
    }
    return buffer;
}

string show_record_process (string AID){
    string buffer = "";
    if (isAuction(AID) == false){
        buffer ="RRC NOK";
    }else{
        buffer = "RRC OK";
        string start = getAuctionStart(AID);
        vector<string> start_info = splitString(start);
        string host_UID = start_info[0];
        string auction_name = start_info[1];
        string asset_fname = start_info[2];
        string start_value = start_info[3];
        string start_date_time = start_info[5];
        string timeactive = start_info[4];

        buffer = buffer + " " + host_UID + " " + auction_name + " " + asset_fname + " " + start_value + " " + start_date_time + " " + timeactive;

        vector<int> bids = getBids(AID);
        vector<int> fiftyBids = get50Bids(bids);
        for (int i = 0; i < fiftyBids.size(); i++){
            string bid = getBid(stoi(AID), fiftyBids[i]);
            vector<string> bid_info = splitString(bid);
            string bidder_UID = bid_info[0];
            string bid_value = bid_info[1];
            string bid_date_time = bid_info[2];
            string bid_sec_time = bid_info[3];

            buffer = buffer + " " + bidder_UID + " " + bid_value + " " + bid_date_time + " " + bid_sec_time;
        }

        if (CheckAuctionEnd(stoi(AID)) == true){
            string end = getAuctionEnd(stoi(AID));
            vector<string> end_info = splitString(end);
            string end_date_time = end_info[0];
            string end_sec_time = end_info[1];

            buffer = buffer + " " + end_date_time + " " + end_sec_time;
        }
    }
    
    return buffer;

}

string no_match_error (){
    string buffer = "ERR";
    return buffer;
}

//funcao que percorre todas as auctions e ve se alguma delas tem o tempo de vida expirado
void checkAuctions() {
    vector<int> AIDs = getAIDs();

    for (int i = 0; i < AIDs.size(); i++) {
        string start = getAuctionStart(padLeft(to_string(AIDs[i]), 3));
        vector<string> start_info = splitString(start);
        string timeactive = start_info[4];
        string start_time = start_info[7];



        if (isAuctionEnded(AIDs[i]) == false) {
            // Auction has not been ended yet
            string end_sec_time = to_string(getTimeDifference(stoi(start_time)));
            if (stoi(end_sec_time) >= stoi(timeactive)) {
                CreateEnd(padLeft(to_string(AIDs[i]), 3), timeactive);
            }
        }
        
    }
}



int udp_server (){

    struct addrinfo hints, *res;
    int fd; // file descriptor
    ssize_t n, nw; // number of bytes read or written
    struct sockaddr_in addr; // address of the server
    socklen_t addrlen; // size of the address
    char buffer[BUFSIZE]; // buffer to store data
    char messageBuffer[BUFSIZE]; // buffer to store data
    
    if((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1 )/*error*/ exit(1); // create socket with UDP protocol

    memset(&hints, 0, sizeof(hints)); // initialize hints to 0s
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_DGRAM; // UDP socket
    hints.ai_flags = AI_PASSIVE; // use local IP address

    if(getaddrinfo(NULL, PORT, &hints, &res) != 0){ //if(getaddrinfo(NULL, to_string(PORT).c_str(), &hints, &res) != 0){
        exit(1);
    }
    if(::bind(fd,res->ai_addr,res->ai_addrlen)==-1)/*error*/exit(1);
    
    while(true) {

        addrlen = sizeof(addr); // set address length
        n = recvfrom(fd, buffer, BUFSIZE, 0, (struct sockaddr*)&addr, &addrlen); // receive data from socket
        if (n==-1)/*error*/exit(1);

        checkAuctions(); // Check if any auctions have ended

        char* processedData = processCharData(buffer,n);
        
        vector<string> result = splitString(processedData);
        string first_word = result[0];
        int result_size= result.size();
        char messageBuffer[BUFSIZE];

        int errcode;
        char host[NI_MAXHOST],service[NI_MAXSERV];

        if((errcode=getnameinfo((struct sockaddr *)&addr,addrlen,host,sizeof host,service,sizeof service,0))!=0){
            fprintf(stderr,"error: getnameinfo: %s\n",gai_strerror(errcode));
        }else{
            cout << "sent by [" << host << ":" << service << "] Request type: " << first_word.c_str() << endl;
        }

        if (result[0] == "LIN" && result_size == 3 && result[1].size() == 6 && isNumeric(result[1]) && result[2].size() == 8 && isAlphaNumeric(result[2])){
            string user = result[1];
            string password = result[2];
            string message_to_send = loginprocess(user, password);
            message_to_send = message_to_send + "\n";
            strncpy(messageBuffer, message_to_send.c_str(), BUFSIZE);

        }
        else if (result[0] == "LOU" && result_size == 3 && result[1].size() == 6 && isNumeric(result[1]) && result[2].size() == 8 && isAlphaNumeric(result[2])){
            string user = result[1];
            string password = result[2];
            string message_to_send = logoutprocess(user, password);
            message_to_send = message_to_send + "\n";
            strncpy(messageBuffer, message_to_send.c_str(), BUFSIZE);

            
        }
        else if (result[0] == "UNR" && result_size == 3 && result[1].size() == 6 && isNumeric(result[1]) && result[2].size() == 8 && isAlphaNumeric(result[2])){
            string user = result[1];
            string password = result[2];
            string message_to_send = unregisterprocess(user, password);
            message_to_send = message_to_send + "\n";
            strncpy(messageBuffer, message_to_send.c_str(), BUFSIZE);

            
        }
        else if(result[0] == "LMA" && result_size == 2 && result[1].size() == 6 && isNumeric(result[1])){
            string user = result[1];
            string message_to_send = my_auctionsprocess(user);
            message_to_send = message_to_send + "\n";
            strncpy(messageBuffer, message_to_send.c_str(), BUFSIZE);

        }
        else if ( result[0] == "LMB" && result_size == 2 && result[1].size() == 6 && isNumeric(result[1])){
            string user = result[1];
            string message_to_send = my_bids_process(user);
            message_to_send = message_to_send + "\n";
            strncpy(messageBuffer, message_to_send.c_str(), BUFSIZE);

        }
        else if (result[0] == "LST" && result_size == 1){
            string message_to_send = list_process();
            message_to_send = message_to_send + "\n";
            strncpy(messageBuffer, message_to_send.c_str(), BUFSIZE);

        }
        else if (result[0] == "SRC" && result_size == 2 && result[1].size() == 3 && isNumeric(result[1])){
            string AID = result[1];
            string message_to_send = show_record_process(AID);
            message_to_send = message_to_send + "\n";
            strncpy(messageBuffer, message_to_send.c_str(), BUFSIZE);

        }
        else{
            string message_to_send = no_match_error();
            message_to_send = message_to_send + "\n";
            strncpy(messageBuffer, message_to_send.c_str(), BUFSIZE);
            
        }


        //nw = sendto(fd, messageBuffer, n, 0, (struct sockaddr*)&addr, addrlen); // send data to socket
        nw = sendto(fd, messageBuffer, strlen(messageBuffer), 0, (struct sockaddr*)&addr, addrlen);
        if(nw==-1)/*error*/ exit(1);
        //if(n != nw);/*error*/exit(1); // all bytes sent

        
    }

    freeaddrinfo(res); // free address info
    close(fd); // close socket

    return 0;
}

//funcao que ve se um user tem o ficheiro de login na diretoria USERS/UID
bool isUserLogged(string UID) {
    char loginName[35];
    FILE *fp;

    if (strlen(UID.c_str()) != 6)
        return false;

    sprintf(loginName, "USERS/%s/%s_login.txt", UID.c_str(), UID.c_str());
    fp = fopen(loginName, "r");

    if (fp == NULL)
        return false;

    fclose(fp);
    return true;
}



//funcao que cria um ficheiro START_(AID).txt na diretoria AUCTIONS/AID, onde dentro do ficheiro tem o UID do host, o nome do auction, o nome do asset, o valor inicial, o tempo ativo, a data atual no formato YYYY-MM-DD HH:MM:SS e o tempo usando a funcao time()
string CreateStart(int AID, std::string UID, std::string auction_name, std::string asset_fname, std::string start_value, std::string timeactive) {
    char startFileName[50]; // Increased buffer size for the file name
    FILE *fp;
    time_t fulltime;
    struct tm *currenttime;
    char timestr[25];

    std::string time_ = getCurrentTime();

    time(&fulltime);
    currenttime = gmtime(&fulltime);

    strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", currenttime);

    snprintf(startFileName, sizeof(startFileName), "AUCTIONS/%03d/START_%03d.txt", AID, AID);
    fp = fopen(startFileName, "w");

    if (fp == NULL) {
        cout << "Error opening file... Exiting" << endl;
        return ""; // Return an empty string indicating an error
    }

    // Assuming 'fulltime' needs to be converted to a string
    fprintf(fp, "%s %s %s %s %s %s %ld", UID.c_str(), auction_name.c_str(), asset_fname.c_str(), start_value.c_str(), timeactive.c_str(), time_.c_str(), fulltime);

    fclose(fp);

    return to_string(fulltime);
}

//funcao que ve os diretorias AUCTIONS/AID e devolve o numero do proximo AID disponivel
int getNextAID() {
    DIR *dir;
    struct dirent *ent;
    int max = 0;

    dir = opendir("AUCTIONS");

    if (dir == NULL)
        return 0;

    while ((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0 && atoi(ent->d_name) > max) {
            max = atoi(ent->d_name);
        }
    }

    closedir(dir);
    return max + 1;
}

//funcao que recebe um file name, file size e file data e cria um ficheiro com esse nome e com o conteudo do file data na diretoria AUCTIONS/AID
int CreateFILE(int AID, string asset_fname) {
    char assetFileName[35];
    FILE *fp;

    sprintf(assetFileName, "AUCTIONS/%03d/%s", AID, asset_fname.c_str());
    fp = fopen(assetFileName, "w");

    if (fp == NULL)
        return 0;

    fclose(fp);

    return 1;
}

//funcao que cria a diretoria BIDS na diretoria AUCTIONS/AID
int CreateBidsDir(int AID) {
    char bidsDirName[35];

    if (AID < 1 || AID > 999)
        return 0;

    sprintf(bidsDirName, "AUCTIONS/%03d/BIDS", AID);
    mkdir(bidsDirName, 0777);

    return 1;
}

// //funcao que cria um ficheiro VVVVVV.txt na diretoria AUCTIONS/AID/BIDS, onde dentro do ficheiro tem o UID do bidder, o valor da bid, a data atual no formato YYYY-MM-DD HH:MM:SS e o tempo usando a funcao time() onde VVVVVV é uma string
// int CreateBid(string AID, string VVVVVV, string UID, string bid_value, string start_time) {
//     char bidFileName[50]; // Increased buffer size to accommodate longer file paths
//     FILE *fp;
//     time_t fulltime, start_time_t;
//     struct tm start_tm, *currenttime;
//     char timestr[25];

//     if (!isNumeric(AID) || VVVVVV.size() != 6 || UID.size() != 6 || bid_value.size() > 6)
//         return 0;

//     // Convert start_time string to time_t
//     if (strptime(start_time.c_str(), "%Y-%m-%d %H:%M:%S", &start_tm) == nullptr)
//         return 0;
//     start_time_t = mktime(&start_tm);

//     time(&fulltime);
//     currenttime = gmtime(&fulltime);

//     strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", currenttime);

//     // Calculate elapsed time in seconds
//     cout << "START TIME: " << start_time <<endl;

//     int end_sec_time = getTimeDifference(stoi(start_time));

//     snprintf(bidFileName, sizeof(bidFileName), "AUCTIONS/%s/BIDS/%s.txt", AID.c_str(), VVVVVV.c_str());
//     fp = fopen(bidFileName, "w");

//     if (fp == NULL)
//         return 0;

//     fprintf(fp, "%s %s %s %s", UID.c_str(), bid_value.c_str(), timestr, to_string(end_sec_time).c_str());
//     fclose(fp);

//     return 1;
// }

int CreateBid(string AID, string VVVVVV, string UID, string bid_value, string start_time) {
    char bidFileName[50]; // Increased buffer size to accommodate longer file paths
    FILE *fp;
    time_t fulltime;
    char timestr[25];

    if (!isNumeric(AID) || VVVVVV.size() != 6 || UID.size() != 6 || bid_value.size() > 6)
        return 0;

    time(&fulltime);

    strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", localtime(&fulltime));



    int end_sec_time = getTimeDifference(stoi(start_time));


    snprintf(bidFileName, sizeof(bidFileName), "AUCTIONS/%s/BIDS/%s.txt", AID.c_str(), VVVVVV.c_str());
    fp = fopen(bidFileName, "w");

    if (fp == NULL)
        return 0;

    // If end_sec_time is not used, you can remove it from fprintf
    fprintf(fp, "%s %s %s %s", UID.c_str(), bid_value.c_str(), timestr, to_string(end_sec_time).c_str());
    fclose(fp);

    return 1;
}


//cria a funcao CreateAuctionDir que cria a diretoria AUCTIONS/AID
int CreateAuctionDir(int AID) {
    char auctionDirName[35];

    if (AID < 1 || AID > 999)
        return 0;

    sprintf(auctionDirName, "AUCTIONS/%03d", AID);
    mkdir(auctionDirName, 0777);

    return 1;
}



//cria uma funcao que cria um ficheiro (AID).txt na diretoria USERS/(UID)/HOSTED

int CreateHOSTEDfile(string UID, string AID) {
    char filename[35];
    FILE *fp;


    sprintf(filename, "USERS/%s/HOSTED/%s.txt", UID.c_str(), AID.c_str());
    fp = fopen(filename, "w");

    if (fp == NULL)
        return 0;

    fclose(fp);

    return 1;
}

string open_process(string user, string password, string name, string start_value, string timeactive, string asset_fname, string asset_size, int newfd){
    string buffer_to_print = "";
    if (CheckLogin((char*)user.c_str()) == false){
        buffer_to_print = "ROA NLG";
    }else{
        try {
            int AID = getNextAID();
            string AID_string = padLeft(to_string(AID), 3);
            CreateAuctionDir(AID);
            string start_time = CreateStart(AID, user, name, asset_fname, start_value, timeactive);
            CreateFILE(AID, asset_fname);
            CreateBidsDir(AID);
            CreateBid(AID_string, padLeft(start_value,6), user, start_value, start_time);
            CreateHOSTEDfile(user, AID_string);

            

            char filename[35];
            int bytesLeft = stoi(asset_size);
            char buffer [CHUNCK_SIZE];



            sprintf(filename, "AUCTIONS/%s/%s", AID_string.c_str(), asset_fname.c_str());
            FILE *jpgFile = fopen(filename, "wb"); // Abre o arquivo para escrita binária
            if (jpgFile == NULL) {
                throw runtime_error("Erro ao abrir o arquivo.");
            }


            while (bytesLeft > 0) {
            int bytesRead = read(newfd, buffer, CHUNCK_SIZE);
            bytesLeft -= bytesRead;

            // Verifica se é a última iteração
            if (bytesLeft <= 0) {
                // Se for a última iteração, calcula quantos bytes escrever
                int bytesToWrite = bytesRead + bytesLeft;
                size_t elements_written = fwrite(buffer, sizeof(char), bytesToWrite, jpgFile);

                if (elements_written != bytesToWrite) {
                    cerr << "Erro ao escrever no arquivo." << endl;
                    fclose(jpgFile);
                    throw runtime_error("Erro ao escrever no arquivo.");
                }
            } else {
                // Caso contrário, escreve todo o buffer normalmente
                size_t elements_written = fwrite(buffer, sizeof(char), bytesRead, jpgFile);

                if (elements_written != bytesRead) {
                    cerr << "Erro ao escrever no arquivo." << endl;
                    fclose(jpgFile);
                    throw runtime_error("Erro ao escrever no arquivo.");
                }
            }
        }
           
            fclose(jpgFile);

            buffer_to_print = "ROA OK " + padLeft(to_string(AID), 3);

        } catch (exception& e) {
            buffer_to_print = "ROA NOK";
            //falta apagar os ficheiros que podem ter sido criados no try
        }
    }

    return buffer_to_print;
}

//funcao que ve se o ficheiro END_(AID).txt existe na diretoria AUCTIONS/AID onde o AID é uma string
bool CheckAuctionEnd(string AID) {
    char endFileName[35];
    FILE *fp;

    if (AID.size() != 3)
        return false;

    sprintf(endFileName, "AUCTIONS/%s/END_%s.txt", AID.c_str(), AID.c_str());
    fp = fopen(endFileName, "r");

    if (fp == NULL)
        return false;

    fclose(fp);
    return true;
}

//funcao que ve se o AID pertence ao user
bool CheckAuctionHost(string AID, string UID) {
    char startFileName[35];
    FILE *fp;
    char start[20];

    if (AID.size() != 3 || UID.size() != 6)
        return false;

    sprintf(startFileName, "AUCTIONS/%s/START_%s.txt", AID.c_str(), AID.c_str());
    fp = fopen(startFileName, "r");

    if (fp == NULL)
        return false;

    fscanf(fp, "%s", start);
    fclose(fp);

    vector<string> start_info = splitString(start);
    string host_UID = start_info[0];

    if (host_UID == UID)
        return true;
    else
        return false;
}



//funcao que ve se um user existe na diretoria USERS/UID
bool isUser(string UID) {
    DIR *dir;
    struct dirent *ent;

    if (strlen(UID.c_str()) != 6)
        return false;

    dir = opendir("USERS");

    if (dir == NULL)
        return false;

    while ((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, UID.c_str()) == 0) {
            closedir(dir);
            return true;
        }
    }

    closedir(dir);
    return false;
}

//funcao que le a password do ficheiro (UID)_password.txt na diretoria USERS/UID e compara com a password recebida
bool doesPasswordMatch(string UID, string password) {
    char passwordFileName[35];
    FILE *fp;
    char retrievedPassword[256];

    if (strlen(UID.c_str()) != 6)
        return false;

    sprintf(passwordFileName, "USERS/%s/%s_password.txt", UID.c_str(), UID.c_str());
    fp = fopen(passwordFileName, "r");

    if (fp == NULL)
        return false;

    fscanf(fp, "%s", retrievedPassword);
    fclose(fp);

    if (strcmp(retrievedPassword, password.c_str()) == 0)
        return true;
    else
        return false;
}



string close_process (string user, string password, string AID){
    string buffer = "";
    //OK se AID is on going e pertence ao user e log in
    if (CheckLogin((char*)user.c_str()) && CheckAuctionEnd(AID) == false && CheckAuctionHost(AID, user)){
        buffer = "RCL OK";
        string start = getAuctionStart(AID);
        vector<string> start_info = splitString(start);
        string start_time = start_info[7];
        string end_sec_time = to_string(getTimeDifference(stoi(start_time)));
        CreateEnd(AID, end_sec_time);
    }else if (isUser(user) == false || doesPasswordMatch(user, password) == false){
        buffer = "RCL NOK";
    }else if (CheckLogin((char*)user.c_str()) == false){
        buffer = "RCL NLG";
    }else if (isAuction(AID) == false){
        buffer =  "RCL EAU";
    }else if (CheckAuctionHost(AID, user) == false){
        buffer = "RCL EOW";
    }else{
        buffer =  "RCL END";
    }
    return buffer;
}


//funcao que recebe um asset name e devolve o tamanho em bytes do asset na diretoria AUCTIONS/AID
int getAssetSize(string AID, string asset_fname) {
    char assetFileName[35];
    FILE *fp;
    char asset[20];

    if (AID.size() != 3 || asset_fname.size() > 20)
        return 0;

    sprintf(assetFileName, "AUCTIONS/%s/%s", AID.c_str(), asset_fname.c_str());
    fp = fopen(assetFileName, "r");

    if (fp == NULL)
        return 0;

    fscanf(fp, "%s", asset);
    fclose(fp);

    vector<string> asset_info = splitString(asset);
    string asset_size = asset_info[1];

    return stoi(asset_size);
}

//funcao que recebe um asset name, procura o ficheiro com esse nome na diretoria AUCTIONS/AID e devolve a file data do asset
string getAssetData(string AID, string asset_fname) {
    char assetFileName[35];
    FILE *fp;
    char asset[20];

    if (AID.size() != 3 || asset_fname.size() > 20)
        return "";

    sprintf(assetFileName, "AUCTIONS/%s/%s", AID.c_str(), asset_fname.c_str());
    fp = fopen(assetFileName, "r");

    if (fp == NULL)
        return "";

    fscanf(fp, "%s", asset);
    fclose(fp);

    vector<string> asset_info = splitString(asset);
    string asset_data = asset_info[2];

    return asset_data;
}



void show_asset_process (string AID, int newfd){
    string buffer_to_output = "";
    try {

        string start_text = getAuctionStart(AID);
        vector<string> start_info = splitString(start_text);
        string asset_fname = start_info[2];
        char assetFileName[35];

        FILE* filePointer;
        char buffer[2048];
        int fileSize;
        int nleft, nwritten, nread;

        // Open the file in read mode
        
        sprintf(assetFileName, "AUCTIONS/%s/%s", AID.c_str(), asset_fname.c_str());
        filePointer = fopen(assetFileName, "rb"); // "rb" for opening a binary file

        if (filePointer == NULL) {
            // Handle the error (e.g., return, throw an exception, etc.)
            throw runtime_error("File open error!");
        }

        // Move the file pointer to the end of the file
        fseek(filePointer, 0, SEEK_END);

        // Get the current position of the file pointer, which is the file size
        fileSize = ftell(filePointer);

        // Check if the file size exceeds the limit of 10 MB
        if (fileSize > 10000000) {
            fclose(filePointer);
            throw runtime_error("File size exceeds the limit of 10 MB!");
        }

        buffer_to_output = "RSA OK " + asset_fname + " " + to_string(fileSize) + " ";
        // send(fd, str.c_str(), str.size(), 0);
        write(newfd, buffer_to_output.c_str(), buffer_to_output.size());
        // Reset the file pointer to the beginning
        fseek(filePointer, 0, SEEK_SET);

        char buffer_to_read[CHUNCK_SIZE];

        // Transmit the file in chunks
        nleft = fileSize;
        while (nleft > 0) {
            nread = fread(buffer_to_read, 1, sizeof(buffer_to_read), filePointer);
            if (nread == -1) {
                throw runtime_error("Error reading from file");

            }
            nwritten = write(newfd, buffer_to_read, nread);
            if (nwritten == -1) {
                throw runtime_error("Error sending data over TCP");
            }
            nleft -= nwritten;
        }
        fclose(filePointer);

        // nread = write(newfd, "\n", 1);
        // if (nread == -1) {
        //     throw runtime_error("Error sending data over TCP");
        // }


    }catch (exception& e) {
        buffer_to_output = "RSA NOK\n";
        int bytesSent = write(newfd, buffer_to_output.c_str(), buffer_to_output.size());
    }
}


//recebe um user e um AID e ve se na diretorio USERS/UID/HOSTED existe o AID
bool isHosted(string UID, string AID) {
    struct stat buffer;

    if (strlen(UID.c_str()) != 6 || AID.size() != 3)
        return false;

    string filePath = "USERS/" + UID + "/HOSTED/" + AID + ".txt";

    if (stat(filePath.c_str(), &buffer) == 0) {
        // Arquivo encontrado
        return true;
    }

    return false;
}

//recebe um string AID e vai a diretoria AUCTIONS/AID/BIDS e devolve o valor da bid mais alta
string getHighestBid(string AID) {
    DIR *dir;
    struct dirent *ent;
    int max = 0;

    if (AID.size() != 3)
        return "";

    dir = opendir(("AUCTIONS/" + AID + "/BIDS").c_str());

    if (dir == NULL)
        return "";

    while ((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0 && atoi(ent->d_name) > max) {
            max = atoi(ent->d_name);
        }
    }

    closedir(dir);
    return to_string(max);
}

int CreateBIDDEDfile(string UID, string AID) {
    char filename[35];
    FILE *fp;


    sprintf(filename, "USERS/%s/BIDDED/%s.txt", UID.c_str(), AID.c_str());
    fp = fopen(filename, "w");

    if (fp == NULL)
        return 0;

    fclose(fp);

    return 1;
}



string bid_process(string user, string password, string AID, string value){
    string buffer = "";
    if (CheckAuctionEnd(AID)){
        buffer = buffer + "RBD NOK";
    }else if (CheckLogin((char*)user.c_str()) == false){
        buffer = buffer + "RBD NLG";
    }else if (isHosted(user, AID) == true){
        buffer = buffer + "RBD ILG";
    }else if (atoi(value.c_str()) <= atoi(getHighestBid(AID).c_str())){
        buffer = buffer + "RBD REF";
    }else{
        try {
            string VVVVVV = padLeft(value, 6);
            string start_text = getAuctionStart(AID);
            vector<string> start_info = splitString(start_text);
            string start_time = start_info[7];
            CreateBid(AID, VVVVVV, user, value, start_time);
            CreateBIDDEDfile(user, AID);

            buffer = buffer + "RBD ACC";
        } catch (exception& e) {
            buffer = buffer + "RBD NOK";
        }
    }
    return buffer;
}
    



int tcp_server() {
    int fd, newfd, ret;
    ssize_t n, nw;
    struct sockaddr_in addr;
    socklen_t addrlen;
    char buffer[BUFSIZE];
    struct sigaction act;
    struct addrinfo hints, *res;

    memset(&act, 0, sizeof(act));
    act.sa_handler = SIG_IGN;
    if (sigaction(SIGCHLD, &act, NULL) == -1){
        exit(1);
    }

    if((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        exit(1);
    }
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, PORT, &hints, &res) != 0){
        cout << "PORT NOT AVAILABLE - CHOOSE ANOTHER" << endl;
        exit(0);
    }


    if (::bind(fd, res->ai_addr, res->ai_addrlen) == -1){
        cout << "PORT NOT AVAILABLE - CHOOSE ANOTHER" << endl;
        exit(1);
    }
    if(listen(fd, 5) == -1){
        cout << "PORT NOT AVAILABLE - CHOOSE ANOTHER" << endl;
        exit(1);
    }
    freeaddrinfo(res);

    while (true) {
        do {
            addrlen = sizeof(addr);
            newfd = accept(fd, (struct sockaddr*)&addr, &addrlen);
        } while (newfd == -1 && errno == EINTR);
        
        if(newfd == -1){
            exit(1);
        }

        pid_t pid = fork();
        if (pid == -1) {
            perror("Fork failed");
            close(newfd);
            continue;
        }

        if (pid == 0) { // Child process
            close(fd); // Close listening socket in child process

            checkAuctions();// Check if any auctions have ended
            int bytesRead;
            string first_word = "";
            while (1) {
                    read(newfd, buffer, 1);
                    if (buffer[0] == ' ') {
                        break;
                    } else {
                        first_word += buffer[0];
                    }
                }
            char messageBuffer [BUFSIZE];
            int errcode;
            char host[NI_MAXHOST],service[NI_MAXSERV];

            if((errcode=getnameinfo((struct sockaddr *)&addr,addrlen,host,sizeof host,service,sizeof service,0))!=0){
                fprintf(stderr,"error: getnameinfo: %s\n",gai_strerror(errcode));
            }else{
                cout << "sent by [" << host << ":" << service << "] Request type: " << first_word.c_str() << endl;

            }


            if (first_word == "OPA"){
                string user = "";
                string password = "";
                string name = "";
                string start_value = "";
                string timeactive = "";
                string asset_name = "";
                string asset_size = "";
                
                while (1) {
                    read(newfd, buffer, 1);
                    if (buffer[0] == ' ') {
                        break;
                    } else {
                        user += buffer[0];
                    }
                }
                while (1) {
                    read(newfd, buffer, 1);
                    if (buffer[0] == ' ') {
                        break;
                    } else {
                        password += buffer[0];
                    }
                }
                while (1) {
                    read(newfd, buffer, 1);
                    if (buffer[0] == ' ') {
                        break;
                    } else {
                        name += buffer[0];
                    }
                }
                while (1) {
                    read(newfd, buffer, 1);
                    if (buffer[0] == ' ') {
                        break;
                    } else {
                        start_value += buffer[0];
                    }
                }
                while (1) {
                    read(newfd, buffer, 1);
                    if (buffer[0] == ' ') {
                        break;
                    } else {
                        timeactive += buffer[0];
                    }
                }
                while (1) {
                    read(newfd, buffer, 1);
                    if (buffer[0] == ' ') {
                        break;
                    } else {
                        asset_name += buffer[0];
                    }
                }
                while (1) {
                    read(newfd, buffer, 1);
                    if (buffer[0] == ' ') {
                        break;
                    } else {
                        asset_size += buffer[0];
                    }
                }
                
                if (user.size() == 6 && isNumeric(user) && password.size() == 8 && isAlphaNumeric(password) && isNumeric(start_value) && isNumeric(timeactive) && asset_name.size () <=24){
                    string message_to_send = open_process(user, password, name, start_value, timeactive, asset_name, asset_size, newfd);
                    message_to_send = message_to_send + "\n";
                    strncpy(messageBuffer, message_to_send.c_str(), BUFSIZE);

                }else{
                    string message_to_send = "ERR";
                    message_to_send = message_to_send + "\n";
                    strncpy(messageBuffer, message_to_send.c_str(), BUFSIZE);
                }

            }else if (first_word == "CLS"){
                string user = "";
                string password = "";
                string AID = "";
                while (1) {
                    read(newfd, buffer, 1);
                    if (buffer[0] == ' ' || buffer[0] == '\n') {
                        break;
                    } else {
                        user += buffer[0];
                    }
                }
                while (1) {
                    read(newfd, buffer, 1);
                    if (buffer[0] == ' ' || buffer[0] == '\n') {
                        break;
                    } else {
                        password += buffer[0];
                    }
                }
                while (1) {
                    read(newfd, buffer, 1);
                    if (buffer[0] == ' ' || buffer[0] == '\n') {
                        break;
                    } else {
                        AID += buffer[0];
                    }
                }
                if (user.size() == 6 && isNumeric(user) && password.size() == 8 && isAlphaNumeric(password) && AID.size() == 3 && isNumeric(AID)){
                    string message_to_send = close_process(user, password, AID);
                    message_to_send = message_to_send + "\n";
                    strncpy(messageBuffer, message_to_send.c_str(), BUFSIZE);
                }else{
                    string message_to_send = "ERR";
                    message_to_send = message_to_send + "\n";
                    strncpy(messageBuffer, message_to_send.c_str(), BUFSIZE);
                }
            }else if (first_word == "SAS"){
                string AID = "";
                while (1) {
                    read(newfd, buffer, 1);
                    if (buffer[0] == ' ' || buffer[0] == '\n') {
                        break;
                    } else {
                        AID += buffer[0];
                    }
                }

                if (AID.size() == 3 && isNumeric(AID)){
                    show_asset_process(AID, newfd);
                }else{
                    string message_to_send = "ERR";
                    message_to_send = message_to_send + "\n";
                    strncpy(messageBuffer, message_to_send.c_str(), BUFSIZE);
                }
            }else if (first_word == "BID"){
                string user = "";
                string password = "";
                string AID = "";
                string value = "";
                while (1) {
                    read(newfd, buffer, 1);
                    if (buffer[0] == ' ' || buffer[0] == '\n') {
                        break;
                    } else {
                        user += buffer[0];
                    }
                }
                while (1) {
                    read(newfd, buffer, 1);
                    if (buffer[0] == ' ' || buffer[0] == '\n') {
                        break;
                    } else {
                        password += buffer[0];
                    }
                }
                while (1) {
                    read(newfd, buffer, 1);
                    if (buffer[0] == ' ') {
                        break;
                    } else {
                        AID += buffer[0];
                    }
                }
                while (1) {
                    read(newfd, buffer, 1);
                    if (buffer[0] == ' ' || buffer[0] == '\n') {
                        break;
                    } else {
                        value += buffer[0];
                    }
                }


                if (user.size() == 6 && isNumeric(user) && password.size() == 8 && isAlphaNumeric(password) && isNumeric(value)){
                    string message_to_send = bid_process(user, password, AID, value);
                    message_to_send = message_to_send + "\n";
                    strncpy(messageBuffer, message_to_send.c_str(), BUFSIZE);
                }else{
                    string message_to_send = "ERR";
                    message_to_send = message_to_send + "\n";
                    strncpy(messageBuffer, message_to_send.c_str(), BUFSIZE);
                }
            }else{
                string message_to_send = "ERR";
                message_to_send = message_to_send + "\n";
                strncpy(messageBuffer, message_to_send.c_str(), BUFSIZE);
            }

            if (first_word != "SAS"){
                size_t bufferSize=strlen(messageBuffer);
                // Echo back to the client (if needed)
                if (write(newfd, messageBuffer, bufferSize) <= 0) {
                    perror("Write error");
                    exit(1);
                }
            }

            close(newfd);
            exit(0);
        } else { // Parent process
            close(newfd); // Close newfd in the parent process
        }
    }

    close(fd); // Close listening socket
    delete[] PORT; // Release memory
    return 0;
}







int main(int argc, char *argv[]){
    parseArguments(argc, argv);
    cout << "PORT: " << PORT << endl;

    create_initial_Directories();

    // Setting up Ctrl+C signal handling for safe termination
    struct sigaction stop;
    memset(&stop, 0, sizeof(stop));
    stop.sa_handler = safe_stop;
    if (sigaction(SIGINT, &stop, NULL) == -1) {
        cerr << "Signal handling setup failed." << endl;
        return 1;
    }

    // Creating a child process for TCP server and managing the UDP server in the parent process
    pid_t pid = fork();
    if (pid == -1) {
        cerr << "Failed to create a child process." << endl;
        return 1;
    }

    if (pid == 0) { // Child process manages the TCP server
        tcp_server();
    } else { // Parent process manages the UDP server
        udp_server();
    }



    return 0;
}
