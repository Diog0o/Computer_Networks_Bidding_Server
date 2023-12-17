#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <vector>
#include <iomanip>



#define CHUNCK_SIZE 2048

int fd,errcode;
ssize_t n;
socklen_t addrlen;
struct addrinfo hints,*res;
struct sockaddr_in addr;
char buffer[6002];


using namespace std;

char* PORT = nullptr; // Initializing to nullptr
char* HOST = nullptr; // Initializing to nullptr


void parseArguments(int argc, char *argv[]) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-n" && i + 1 < argc) {
            const char* ipStr = argv[i + 1];
            HOST = new char[strlen(ipStr) + 1]; // Allocate memory for HOST
            strcpy(HOST, ipStr); // Copy the string
        } else if (arg == "-p" && i + 1 < argc) {
            const char* portStr = argv[i + 1];
            PORT = new char[strlen(portStr) + 1]; // Allocate memory for PORT
            strcpy(PORT, portStr); // Copy the string
        }
    }

    if (HOST == nullptr) {
        // ASIP is not provided, assuming it runs on the same machine
        HOST = new char[10]; // Allocate memory for HOST
        strcpy(HOST, "localhost");
    }

    if (PORT == nullptr) {
        // ASport is not provided, setting default value based on GN
        int defaultPort = 58077;
        PORT = new char[10]; // Allocate memory for ASport
        sprintf(PORT, "%d", defaultPort);
    }
}


//create a struct that uses current user and current password in a struct exactly like the global variables current user and current password

struct User{
    string current_user=string(6,' ');
    string current_password=string(8,' ');
    bool is_logged_in= false;

};


struct UDPClient{
    void start_udp_client(){
        fd=socket(AF_INET,SOCK_DGRAM,0); //UDP socket
        if(fd==-1) /*error*/exit(1);
        memset(&hints,0,sizeof hints);
        hints.ai_family=AF_INET; //IPv4
        hints.ai_socktype=SOCK_DGRAM; //UDP socket
        errcode=getaddrinfo(HOST,PORT,&hints,&res); // "tejo.tecnico.ulisboa.pt"
        if(errcode!=0) /*error*/ exit(1);
    }

    void close_udp_client(){
        /*close the UDP*/
        freeaddrinfo(res);
        close(fd);    
    }
};

//create a struct to use the tcp protocol connection
struct TCPClient{
    int fd,errcode;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints,*res;
    struct sockaddr_in addr;
    
    void start_tcp_client(){
        fd=socket(AF_INET,SOCK_STREAM,0); //TCP socket
        if(fd==-1) /*error*/exit(1);
        memset(&hints,0,sizeof hints);
        hints.ai_family=AF_INET; //IPv4
        hints.ai_socktype=SOCK_STREAM; //TCP socket
        errcode=getaddrinfo(HOST,PORT,&hints,&res);
        if(errcode!=0) /*error*/ exit(1);
        
        n=connect(fd,res->ai_addr,res->ai_addrlen);
        if(n==-1) /*error*/ exit(1);

    }

    void close_tcp_client(){
        /*close the TCP*/
        freeaddrinfo(res);
        close(fd);
        
    }

};

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


string remove_slashN(string input) {
    size_t lastNonNewline = input.find_last_not_of("\n");

    if (lastNonNewline != string::npos) {
        return input.substr(0, lastNonNewline + 1);
    } else {
        // Se a string consistir apenas de quebras de linha, retorna uma string vazia
        return "";
    }
}
vector<string> splitString(const string& input) {
    istringstream iss(input);
    vector<std::string> words;
    string word;

    while (iss >> word) {
        words.push_back(word);
    }

    return words;
}


void checkLogin(string candidate_user,string candidate_password,User& user){
    n=recvfrom(fd,buffer,6002,0, (struct sockaddr*)&addr,&addrlen);
    if(n==-1) /*error*/ exit(1);

    if (n==6002){
        cout<<"Erro de formataçao"<<endl;
        return;
    }

    char* processedData = processCharData(buffer,n);

    if(strcmp(processedData, "RLI OK\n")==0){
        user.is_logged_in=true; 
        user.current_user=candidate_user;
        user.current_password=candidate_password;

        printf("sucessful login\n");

    }else if(strcmp(processedData,"RLI NOK\n")==0){
        printf("incorrect login attempt\n");

    }else if(strcmp(processedData,"RLI REG\n")==0){
        user.is_logged_in=true; 
        user.current_user=candidate_user;
        user.current_password=candidate_password; 
        printf("new user registered\n");

    }
}

void checkLogout(User& user){
    n=recvfrom(fd,buffer,6002,0, (struct sockaddr*)&addr,&addrlen);
    if(n==-1) /*error*/ exit(1);

    if (n==6002){
        cout<<"Erro de formataçao"<<endl;
        return;
    }
    
    char* processedData = processCharData(buffer,n);

    if(strcmp(processedData,"RLO OK\n")==0){
           
        printf("sucessful logout\n");
        user.is_logged_in=false;
        // is_logged_in=false;
        
    }else if(strcmp(processedData,"RLO NOK\n")==0){
        printf("user not logged in\n");

    }else if(strcmp(processedData,"RLO UNR\n")==0){
        printf("unknown user\n");
    }
}

void checkUnregister(User& user){
    n=recvfrom(fd,buffer,6002,0, (struct sockaddr*)&addr,&addrlen);
    if(n==-1) /*error*/ exit(1);

    if (n==6002){
        cout<<"Erro de formataçao"<<endl;
        return;
    }

    char* processedData = processCharData(buffer,n);
    if(strcmp(processedData,"RUR OK\n")==0){

        printf("sucessful unregister\n");
        user.is_logged_in=false;    

    }else if(strcmp(processedData,"RUR NOK\n")==0){
        printf("unknown user\n");
    }else if(strcmp(processedData,"RUR UNR\n")==0){
        printf("incorrect unregister attempt\n");
    }
}

void u_exit(User& user){
    
    if (user.is_logged_in==true){
        printf("You must logout first\n");
    }
    else{
        exit(0);
    }
}

void my_auctions(User& user){
    string str =  "LMA " + user.current_user +"\n";
    const char* messageBuffer =str.c_str();
    size_t bufferSize=strlen(messageBuffer);
    n=sendto(fd,messageBuffer,bufferSize,0,res->ai_addr,res->ai_addrlen);
    if(n==-1) /*error*/ exit(1);
}

//funcao que recebe um char[] e devolve outro char[] ate enconrar o \n


void checkMyAuctions(User &user){
    n=recvfrom(fd,buffer,6002,0, (struct sockaddr*)&addr,&addrlen);
    if(n==-1) /*error*/ exit(1);
    if (n==6002){
        cout<<"Erro de formataçao"<<endl;
        return;
    }
    char* processedData = processCharData(buffer,n);

    vector<string> splitted = splitString(processedData);
    string first_word = splitted[0];
    string second_word = splitted[1];
    
 
    if(first_word=="RMA" && second_word=="OK"){//meter first e second word 

        for(int i=2;i<splitted.size();i+=2){

            string res="";
            res=splitted[i]+" "+ splitted[i+1] +"\n";
        }
        
    }else if(strcmp(processedData,"RMA NOK\n")==0){
        printf("The user is not involved in any of the currently active auctions\n");

    }else if(strcmp(processedData,"RMA NLG\n")==0){
        printf("The user is not logged in\n");
    }
}

void my_bids(User& user){
    
    string str =  "LMB " + user.current_user +"\n";
    const char* messageBuffer =str.c_str();
    size_t bufferSize=strlen(messageBuffer);
    n=sendto(fd,messageBuffer,bufferSize,0,res->ai_addr,res->ai_addrlen);
    if(n==-1) /*error*/ exit(1);
}


void checkMyBids(){
    n=recvfrom(fd,buffer,6002,0, (struct sockaddr*)&addr,&addrlen);
    if(n==-1) /*error*/ exit(1);

    if (n==6002){
        cout<<"Erro de formataçao"<<endl;
        return ;
    }

    char* processedData = processCharData(buffer,n);
    vector<string> splitted = splitString(processedData);
    string first_word=splitted[0];
    string second_word=splitted[1];

    if(first_word=="RMB" && second_word=="OK"){//meter first e second word 


        for(int i=2;i<splitted.size();i+=2){

            string res="";
            res=splitted[i]+" "+ splitted[i+1] +"\n";
        }
        
    }else if(strcmp(processedData,"RMB NOK\n")==0){
        printf("The user has no active bids\n");

    }else if(strcmp(processedData,"RMB NLG\n")==0){
        printf("The user is not logged in\n");
    }

}

void list(){
    string str = "LST\n";
    const char* messageBuffer =str.c_str();
    size_t bufferSize=strlen(messageBuffer);

    n=sendto(fd,messageBuffer,bufferSize,0,res->ai_addr,res->ai_addrlen);
    if(n==-1) /*error*/ exit(1);
}

void checkList(){
    n=recvfrom(fd,buffer,6002,0, (struct sockaddr*)&addr,&addrlen);
    if(n==-1) /*error*/ exit(1);
    
    if (n==6002){
        cout<<"Erro de formataçao"<<endl;
        return;
    }

    char* processedData = processCharData(buffer,n);
    vector<string> splitted = splitString(processedData);
    string first_word=splitted[0];
    string second_word=splitted[1];
 
    if(strcmp(first_word.c_str(),"RLS")==0 && strcmp(second_word.c_str(),"OK")==0){//meter first e second word 


        for(int i=2;i<splitted.size();i+=2){

            string res="";
            res= "Auction Number: " + splitted[i]+ " Status: "+ splitted[i+1] +"\n";
            cout << res;
        }

    }else if(strcmp(processedData,"RLS NOK\n")==0){
        printf("No auction was yet started\n");
    }
}


void show_record(string aid){
    string str="SRC " + aid +"\n";
    const char* messageBuffer =str.c_str();
    size_t bufferSize=strlen(messageBuffer);

    n=sendto(fd,messageBuffer,bufferSize,0,res->ai_addr,res->ai_addrlen);
    if(n==-1) /*error*/ exit(1);    
}




void check_show_record(){
    n=recvfrom(fd,buffer,6002,0, (struct sockaddr*)&addr,&addrlen);
    if(n==-1) /*error*/ exit(1);

    if (n==6002){
        cout<<"Erro de formataçao"<<endl;
        return;
    }
    char* processedData = processCharData(buffer,n);

    cout << buffer << endl;

    string result= remove_slashN(buffer);
    vector<string> splitted = splitString(result);
    string first_word=splitted[0];
    string second_word=splitted[1];
    
    if(strcmp(first_word.c_str(),"RRC")==0 && strcmp(second_word.c_str(),"OK")==0){//meter first e second word 
    
    // cout << "RRC OK" << endl;

    // string res = "Host UID: " + splitted[2] + "\n";
    // res += "Auction Name: " + splitted[3] + "\n";
    // res += "Asset Name: " + splitted[4] + "\n";
    // res += "Start Value: " + splitted[5] + "\n";
    // res += "Start Date Time: " + splitted[6] + " " + splitted[7] + "\n";
    // res += "Timeactive: " + splitted[8] + "\n\n";

    // res += "BIDS:\n";
    // int counter = 1;
    // int i = 9;
    // while (i < splitted.size() && splitted[i] == "B"){
    //     res += "Bid " + to_string(counter++) + ":\n";
    //     res += "Bidder UID: " + splitted[i+1] + "\n";
    //     res += "Bid Value: " + splitted[i+2] + "\n";
    //     res += "Bid Date Time: " + splitted[i+3] + " " + splitted[i+4] + "\n";
    //     res += "Bid Sec Time: " + splitted[i+5] + "\n\n";
    //     i += 6;
    //     cout << "I: " << i<< res;
    // }

    // int size = splitted.size();

    // if (splitted[size-4] == "E"){
    //     res += "Auction is closed\n";
    //     res += "End Date Time: " + splitted[size-3] + " " + splitted[size-2] + "\n";
    //     res += "End Sec Time: " + splitted[size-1] + "\n";
    // }

    // cout << res;

    //retirar os 7 primeiros bytes do buffer e imprimir o resto
    for(int i=7;i<n;i++){
        cout << processedData[i];
    }



    }else if(strcmp(processedData,"RRC NOK\n")==0){
        printf("Auction does not exist\n");
    }
}

void login(string candidate_user, string candidate_password){
        
    string str =  "LIN " + candidate_user + " " + candidate_password + "\n";
    
    const char* messageBuffer =str.c_str();
    size_t bufferSize=strlen(messageBuffer);

    n=sendto(fd,messageBuffer,bufferSize,0,res->ai_addr,res->ai_addrlen);
    if(n==-1) /*error*/ exit(1);

}


void logout(User& user){
    string str =  "LOU " + user.current_user + " " + user.current_password + "\n";
        
    const char* messageBuffer =str.c_str();
    size_t bufferSize=strlen(messageBuffer);

    n=sendto(fd,messageBuffer,bufferSize,0,res->ai_addr,res->ai_addrlen);
    if(n==-1) /*error*/ exit(1);
}


void unregister(User& user){
    
    string str =  "UNR " + user.current_user + " " + user.current_password + "\n";

    const char* messageBuffer =str.c_str();
    size_t bufferSize=strlen(messageBuffer);

    n=sendto(fd,messageBuffer,bufferSize,0,res->ai_addr,res->ai_addrlen);
    if(n==-1) /*error*/ exit(1);

}

//TCP FUNCTIONS    

bool isValidFilename(const string& filename) {
    // Check if the filename length is not more than 24 characters
    return filename.length() <= 24;
}

void open(const std::string& name, int start_value, int timeactive, const std::string& asset_fname, User& user) {

    std::string str = "OPA " + user.current_user + " " + user.current_password + " " + name + " " +
                      std::to_string(start_value) + " " + std::to_string(timeactive) + " " + asset_fname;

    FILE* filePointer;
    char buffer[2048];
    long fileSize;
    int nleft, nwritten, nread;


    // Open the file in read mode
    filePointer = fopen(asset_fname.c_str(), "rb"); // "rb" for opening a binary file

    if (filePointer == NULL) {
        printf("File open error!\n");
        // Handle the error (e.g., return, throw an exception, etc.)
        return;
    }

    // Move the file pointer to the end of the file
    fseek(filePointer, 0, SEEK_END);

    // Get the current position of the file pointer, which is the file size
    fileSize = ftell(filePointer);

    // Check if the file size exceeds the limit of 10 MB
    if (fileSize > 10000000) {
        printf("File size exceeds the limit of 10 MB!\n");
        fclose(filePointer);
        // Handle the error (e.g., return, throw an exception, etc.)
        return;
    }

    str = str + " " + std::to_string(fileSize) + " ";
    // send(fd, str.c_str(), str.size(), 0);
    write(fd, str.c_str(), str.size());
    // Reset the file pointer to the beginning
    fseek(filePointer, 0, SEEK_SET);


    // Transmit the file in chunks
    nleft = fileSize;
    while (nleft > 0) {
        nread = fread(buffer, 1, sizeof(buffer), filePointer);
        if (nread == -1) {
            perror("Error reading from file");
            // fclose(filePointer);
            return;
        }
        nwritten = write(fd, buffer, nread);
        if (nwritten == -1) {
            perror("Error sending data over TCP");
            // fclose(filePointer);
            return;
        }
        nleft -= nwritten;
    }

    fclose(filePointer);


}







void check_open(){
    n=read(fd,buffer,128);
    if(n==-1)/*error*/exit(1);

    char* processedData = processCharData(buffer,n);

    if(strcmp(processedData,"ERR\n")==0){
        cout << "ERROR" << endl;
        return;
    }

    vector<string> splitted = splitString(processedData);
    string first_word=splitted[0];
    string second_word=splitted[1];

    if(strcmp(processedData,"ROA NOK\n")==0){
        printf("Auction could not be started\n");
    }else if(strcmp(processedData,"ROA NLG\n")==0){
        printf("User is not logged in\n");
    }else if(strcmp(first_word.c_str(),"ROA")==0 && strcmp(second_word.c_str(),"OK")==0){
        printf("%s\n",splitted[2].c_str());
    }

}

void close(string AID,User& user){

    string str= "CLS "+ user.current_user+ " "+ user.current_password + " " + AID + "\n";
    const char* messageBuffer =str.c_str();

    n=write(fd,messageBuffer,150);

    if(n==-1)/*error*/exit(1);
}

/* falta assumir o "user not logged in" */
void check_close(){

    n=read(fd,buffer,128);
    if(n==-1)/*error*/exit(1);

    char* processedData = processCharData(buffer,n);
    
    string result= remove_slashN(buffer);
    vector<string> splitted = splitString(buffer);

    string first_word = splitted[0];
    string second_word = splitted[1];
    
    if(strcmp(processedData,"RCL OK\n")==0){
        printf("Auction was sucessfully closed\n");

    }else if(strcmp(processedData,"RCL END\n")==0){
        printf("Auction has already finished\n");

    }else if(strcmp(processedData,"RCL NLG\n")==0){
        printf("User is not logged in\n");

    }else if(strcmp(processedData,"RCL EAU\n")==0){
        printf("Auction does not exist\n");

    }else if(strcmp(processedData,"RCL EOW\n")==0){
        printf("Auction is not owned by user\n");
    }
}

void show_asset(string AID){
    string str= "SAS " + AID + "\n";
    const char* messageBuffer =str.c_str();
    n=write(fd,messageBuffer,8);
    if(n==-1)/*error*/exit(1);
}

/* da erro quando nao ha loggin feito */
void check_show_asset(){
    string first_word = "";
    string status = "";
    string asset_name = "";
    string asset_size = "";
    while (1) {
        read(fd, buffer, 1);
        if (buffer[0] == ' ' || buffer[0] == '\n') {
            break;
        } else {
            first_word += buffer[0];
        }
    }
    while (1) {
        read(fd, buffer, 1);
        if (buffer[0] == ' ' || buffer[0] == '\n') {
            break;
        } else {
            status += buffer[0];
        }
    }
    cout << first_word << endl;
    cout << status << endl;

    if (first_word == "RSA" && status == "OK") {
        cout << "RSA OK" << endl;

    while (1) {
        read(fd, buffer, 1);
        if (buffer[0] == ' ' || buffer[0] == '\n') {
            break;
        } else {
            asset_name += buffer[0];
        }
    }
    while (1) {
        read(fd, buffer, 1);
        if (buffer[0] == ' ' || buffer[0] == '\n') {
            break;
        } else {
            asset_size += buffer[0];
        }
    }
    cout << asset_name << endl;
    cout << asset_size << endl;


    FILE *jpgFile = fopen(asset_name.c_str(), "wb"); // Abre o arquivo para escrita binária
    if (jpgFile == NULL) {
        throw runtime_error("Erro ao abrir o arquivo.");
    }


    int bytesLeft = stoi(asset_size);
    char buffer_to_read[CHUNCK_SIZE];

    while (bytesLeft > 0){
                int bytesRead = read(fd, buffer_to_read, CHUNCK_SIZE);
                bytesLeft -= bytesRead;
                size_t elements_written = fwrite(buffer_to_read, sizeof(char), bytesRead, jpgFile);

                if (elements_written != bytesRead) {
                    cerr << "Erro ao escrever no arquivo." << endl;
                    fclose(jpgFile);
                    throw runtime_error("Erro ao escrever no arquivo.");
                }
            }

    fclose(jpgFile);


    }else  if(first_word == "RSA" && status == "NOK"){
        printf("Something wrong happened\n");

    }
}

void bid(string aid, string value, User& user){
    
    string str= "BID " + user.current_user +" " + user.current_password + " "+ aid + " "+ value+"\n";
    const char* messageBuffer =str.c_str();
    n=write(fd,messageBuffer,100);
    if(n==-1)/*error*/exit(1);
}



void check_bid(){
    n=read(fd,buffer,128);
    if(n==-1)/*error*/exit(1);
    string result= remove_slashN(buffer);
    vector<string> splitted = splitString(result);
    string first_word=splitted[0];
    string second_word=splitted[1];
    if(strcmp(first_word.c_str(),"RBD")==0 && strcmp(second_word.c_str(),"NOK")==0){
        printf("Auction is not active\n");

    }else if(strcmp(first_word.c_str(),"RBD")==0 && strcmp(second_word.c_str(),"ACC")==0){
        printf("Bid was accepted\n");
    }else if(strcmp(first_word.c_str(),"RBD")==0 && strcmp(second_word.c_str(),"REF")==0){
        printf("Rejected:a larger bid has already been placed previously\n");
    }else if(strcmp(first_word.c_str(),"RBD")==0 && strcmp(second_word.c_str(),"ILG")==0){   
        printf("Auction hosted by yourself, you cannot bid\n");
    }else if(strcmp(first_word.c_str(),"RBD")==0 && strcmp(second_word.c_str(),"NLG")==0){
        printf("User is not logged in\n");
    }
}

int main(int argc, char *argv[]){

    parseArguments(argc, argv);
    cout << "HOST: " << HOST << endl;
    cout << "PORT: " << PORT << endl;

    char buffer[100];
    User user;
    UDPClient udpclient;
    TCPClient tcp;

    while (1){
        fgets(buffer, sizeof(buffer), stdin);
        vector<string> result = splitString(buffer);

        string first_word = result[0];
        int result_size= result.size();

        /*para o login*/
        if(first_word=="login" && result.size() == 3 ){
            
            udpclient.start_udp_client();
            string candidate_user = result[1];
            string candidate_password = result[2];
            login(candidate_user,candidate_password);
            checkLogin(candidate_user,candidate_password,user);
            udpclient.close_udp_client();
        }

        /*para o logout*/
        else if(first_word=="logout" && result.size() == 1){
            udpclient.start_udp_client();
        
            logout(user);
            checkLogout(user);

            udpclient.close_udp_client();
        }

        else if(first_word=="unregister" && result.size() == 1){
            udpclient.start_udp_client();
            
            unregister(user);
            checkUnregister(user);

            udpclient.close_udp_client();
        }
        

        else if(first_word=="exit" && result.size()==1){
            udpclient.start_udp_client();
            
            u_exit(user); /*para nao confundir com o exit(0) nem com o exit(1) meti u_exit*/
            udpclient.close_udp_client();
        }

        else if((first_word=="myauctions" || first_word =="ma") && result.size()==1){
            udpclient.start_udp_client();
            my_auctions(user);
            checkMyAuctions(user);
            udpclient.close_udp_client();
        }

        else if((first_word=="list" || first_word == "l" )&& result.size()==1){
            udpclient.start_udp_client();
            list();
            checkList();
            udpclient.close_udp_client();
        }

        else if((first_word=="mybids" || first_word=="mb")&& result.size()==1){
            udpclient.start_udp_client();
            my_bids(user);
            checkMyBids();
            udpclient.close_udp_client();
        }

        else if(((first_word=="show_record")|| first_word=="sr")&& result.size()==2){
            udpclient.start_udp_client();
            string aid = result[1];
            show_record(aid);
            check_show_record();
            udpclient.close_udp_client();
        }

        else if((first_word=="open")&& result.size()==5){
            tcp.start_tcp_client();
            open(result[1], stof(result[3]), stof(result[4]), result[2],user);
            check_open();   
            tcp.close_tcp_client();
        }

        else if((first_word=="close")&& result.size()==2){
            tcp.start_tcp_client();
            string aid= result[1];
            close(aid,user);
            check_close();
            tcp.close_tcp_client();
        }

        else if((first_word=="show_asset" or first_word=="sa")&& result.size()==2){
            tcp.start_tcp_client();
            string aid = result[1];
            cout << "AID: " << aid << endl;
            show_asset(aid);
            check_show_asset();
            tcp.close_tcp_client();
        }

        else if((first_word=="bid" or first_word=="b")&& result.size()==3){
            tcp.start_tcp_client();
            string aid=result[1];
            string value=result[2];
            
            bid(aid,value,user);
            check_bid();    
            tcp.close_tcp_client();
        }
        else{
            printf("Invalid command\n");
        }

    }
    delete[] HOST; // Release memory allocated for HOST
    delete[] PORT; // Release memory allocated for PORT

    return 0;
}
