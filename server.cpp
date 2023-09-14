/*
    Progetto di un'applicativo Client-Server a cura di Carlo Capodilupo 1851916 .
    Lato Server.
*/

#include <sys/socket.h> //socket manipulation.
#include <arpa/inet.h> //internet operations, use of inet_addr, inet_ntoa, AF_INET.
#include <netinet/in.h> //internet family definitions and use of sin_family, sin_port, sin_addr.
#include <string.h> //string manipulation, use of strcpy, strlen etc.
#include <unistd.h> //access to standard Posix API and system calls such as fork, primitive I/O (read, write, close ecc.), getpid. 
#include <stdlib.h> //memory allocation and process control.
#include <iostream> //standard input output stream and use of cin,cout,cerr ecc.
#include <fstream> //classe di input/output per operare sui file.
#include <signal.h> //signal manipulation.
#include <time.h> //time manipulation.
using namespace std; //so we can just the 'std::' prefix every time.

int* refServerSocket; //reference to server socket for orderly fashion exiting.
fstream dbFile; //db.txt file global declaration.
bool available=true; //mutex lock variable for process synchronization.

void chat(int, char*, int); //prototype of the children processes function to execute.
void orderlyFashionExitingS(int); //prototype of the orderly fashion exit function activated once termination signal are caught.
void acquire(); //prototype for the acquiring lock function.
void release(); //prototype for the releasing lock function.

int main() {
    char ipServer[16]; //server ip address declaration.
    int portaServer; //server port number declaration.
    
    //following two lines can become a comment just if uncommented the following 4 lines starting from cout << "..." etc.
    /* 
    strcpy(ipServer, "192.168.43.223"); //pro tempore assignment of the server ip address.
    portaServer = 2115; //pro tempore assignment of the server port number.
    */
    cout << "\nINSERISCI L'IPv4 DEL SERVER\n";
    cin >> ipServer; //alternative interactive assignment of server ip address.
    cout << "\nINSERISCI IL NUMERO DI PORTA DEL SERVER\n";
    cin >> portaServer; //alternative interactive assignment of server port number.

    //server socket creation for the service implementation.
    //AF_INET specifies the internet protocol family IPv4.
    //SOCK_STREAM specifies the socket type TCP the only one that guarantees the stream.
    //0 specifies the default protocol for the previous couple.
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0); 
    if(serverSocket<0) {
        cerr << "\nCreazione socket fallita\n";
        exit(1);
    }

    //signal management (no SIGKILL, number 9, handling because it can't be caught).
    refServerSocket = &serverSocket; //assignment of the server socket descriptor to the reference.
    signal(SIGINT, orderlyFashionExitingS); //call of orderly fashion exiting function on SIGINT alias Ctrl+C, number 2.
    signal(SIGTERM, orderlyFashionExitingS); //call of orderly fashion exiting function on SIGTERM normal termination, number 15.
    //from now on, with a termination signal sent to the main process there'll be an orderly fashion exit.

    //assignment of server ip address and port number to the structured server address.
    sockaddr_in indirizzoServer; //declaration of the structured server address.
    socklen_t lenghtIndirizzoServer = sizeof(indirizzoServer); //declaration and assignment of the lenght of the structured server address.
    indirizzoServer.sin_family = AF_INET; //specification of the IPv4 internet protocol family.
    indirizzoServer.sin_port = htons(portaServer); //assignment of the server port address converted from Host byte order (Little Endian for Intel x64 processors such as mine) to the Network byte order (Big Endian).
    indirizzoServer.sin_addr.s_addr = inet_addr(ipServer); //assignment of the server ip address converted thanks to inet_addr() from decimal pointed numeration address to binary 32 bit address.

    //association of the structured server address to the server socket.
    //serverSocket is the server socket descriptor.
    //indirizzoServer is the structured server address that points to a sockaddr structure.
    //lenghtIndirizzoServer is the length in byte of the structured server address.
    int esitoBind = bind(serverSocket, (sockaddr *)&indirizzoServer, lenghtIndirizzoServer);
    if(esitoBind<0) {
        cerr << "Associazione indirizzo fallita (bind)\n";
        exit(2);
    }
    
    //setting of the socket in a listening state for clients connection requests.
    //serverSocket is the server socket descriptor.
    //5 is the maximum number of connection accepted.
    int esitoListen = listen(serverSocket,5);
    if(esitoListen<0) {
        cerr << "Ascolto su socket fallito (listen)\n";
        exit(3);
    }

    //declaration of the variables for the structured client address
    sockaddr_in indirizzoClient; //declaration of the structured client address.
    socklen_t lenghtIndirizzoClient = sizeof(indirizzoClient); //declaration and assignment of the lenght of the structured client address.
    memset(&indirizzoClient,0,lenghtIndirizzoClient); //sets to 0 the bytes of the structured client address (good practice).

    //busy waiting for clients connections request.
    cout << "\nSono in attesa di connessione (accept)\n\n";
    int clientSocket;

    //creation of the database db.txt file to store the chat service contents.
    dbFile.open("db.txt", ios::out | ios::app); //creation and opening of the file with write permissions, if the file already exist and is not empty we want to append contents instead of overwrite them on previous ones.
    dbFile << "\n___________________________________________________"; //writing new chat demarcation line on the db.txt file
    dbFile.close(); //file closing.

    //until a termination signal is sent to the main process(then called parent process as well), do as follows.
    do {
        //accept an incoming connection request from a client.
        //be aware that if the client is running on an different device from the one where the server is running, then the service port of the server device must be opened otherwise the firewall will block incoming request since the default action is to block all incoming connection and allow all outcoming connection.
        //serverSocket is the server socket descriptor of the socket the client wants to connect to, it doesn't change and continuesly listens for new requests.
        //indirizzoClient is the structured client address that points to a sockaddr structure.
        //lenghtIndirizzoClient is the length in byte of the structured client address.
        //clientSocket is the client socket descriptor of a new socket on which the server serves the connection with the accepted client.
        clientSocket = accept(serverSocket, (sockaddr *)&indirizzoClient, &lenghtIndirizzoClient);
        if(clientSocket<0) {
            cerr << "Connessione con il client fallita\n";
        }else{
            //connection attempted now the main process can have a son entrusted of manage the connection with the client.
            int pid = fork();
            if(pid==0){
                char* ipClient = inet_ntoa(indirizzoClient.sin_addr); //declaration and assignment of client ip address.
                int portClient = ntohs(indirizzoClient.sin_port); //declaration and assignment of client port number.
                chat(clientSocket, ipClient, portClient); //son process(pid==0) executes the chat service, client socket descriptor, ip address and port number are needed.
            }
        }
    }while(true);

    return 0;

}

//each child process is assigned to a client and executes the following function, core of the chat service.
void chat(int stream, char* add, int port){

    //declaration of the variables needed.
    //size is the size in bytes of messagges that'll be exchanged.
    //nome is the name of the client user, the first message received from the client.
    //buffer is created to store all others incoming messages from the client.
    //msg_ready_to_send is created to store all the content of the db.txt file adjourned and ready to be sent to the client each time are required.
    int size=1024;
    char nome[size], buffer[size], msg_ready_to_send[size];
    string msg_for_client, temp;

    recv(stream,nome,size,0); //receiving the name of the client.

    //having multiple processes that will write on the db.txt the messages from the respective clients, race conditions are highly possible.
    //we need synchronization to assert integrity of the informations in the database, one mutex lock is used and visible among all processes.
    acquire(); //acquisition of the file lock.
    dbFile.open("db.txt", ios::out | ios::app); //opening of the db.txt file previously created with write permission, contents will be appended to the previous ones.
    dbFile << "\n..." << nome << "@" << add << ":" << port << " si è aggiunto alla chat..."; //writes in the file something such as: "...carlo@192.168.43.223:58160 si è aggiunto alla chat..." on a new line.
    dbFile.close(); //file closing.
    release(); //release of the file lock.

    //as long as the client doesn't close the connection, the chat service is guaranteed for that client.
    do {
        //receive from the client socket stream a message of size bytes and store it in the buffer declared, no flags used.
        recv(stream,buffer,size,0);

        //'*' from the client means the client closed his end of the stream, we do it as well on our side and break the while. 
        if(!strcmp(buffer,"*")){ //"*" is correct, be aware no spaces are needed after the '*' (error: "* "), look the client code for better understanding.
            close(stream); //client socket closing on server side.
            break;
        }

        //'# ' from the client means that he just wants the updated chat from the server, if the client sends to the server just '# ' we don't need to write this message to the database because other clients have no interest knowing the moment when the client has pulled the chat from the server.
        //so we write messages in the database just if different from '# '.
        if(strcmp(buffer,"# ")){ //"# " is correct, be aware the space is needed after the '#' (error: "#"), look the client code for better understanding.
 
            //localtime declaration and acquisition each time a message arrives.
            time_t now = time(0);
            tm *ltm = localtime(&now);

            //received message can now be written in the db.txt file.
            acquire(); //acquisition of the file lock.
            dbFile.open("db.txt", ios::out | ios::app); //opening of the db.txt file with write permission, contents will be appended to the previous ones.
            dbFile << "\n"<<nome<<"["<<1900+ltm->tm_year<<"/"<<1+ltm->tm_mon<<"/"<<ltm->tm_mday<<" "<<ltm->tm_hour<<":"<<ltm->tm_min<<":"<<ltm->tm_sec<<"]: ";
            dbFile << buffer ; //the previous and current line write in the file something such as: "carlo[2023/1/15 14:15:38]: ciao a tutti #" on a new line.
            dbFile.close(); //file closing.
            release(); //release of the file lock.
        }

        //no locks needed for reading.
        dbFile.open("db.txt", ios::in); //opening of the db.txt file with only read permissions.
        msg_for_client = ""; //make sure msg_for_client has no previous content before sending the entire chat to the client (this cleaning up is fundamental after the first send from the server).
        while( getline(dbFile,temp) ) { //line by line till the end of the db.txt file content is stored in msg_for_client.
            temp += "\n";
            msg_for_client += temp;
        }
        dbFile.close(); //file closing.
        strcpy(msg_ready_to_send, msg_for_client.c_str()); //msg_for_client is a string and send() function accepts just fixed char messages, so a cast is needed: strint to char[].
        send(stream,msg_ready_to_send,size,0); //the entire char saved on db.txt is, here and only here, sent to the client.
        system("clear"); //let's clean up the screen of the server so that it's not showed the previously printed and now deprecated chat content.
        cout << msg_ready_to_send; //print of the update chat content, the same sent to each client who joined the chat.

    }while(true);
    
    acquire(); //acquisition of the file lock.
    dbFile.open("db.txt", ios::out | ios::app); //opening of the db.txt file with write permission, contents will be appended to the previous ones.
    dbFile << "\n..." << nome << "@" << add << ":" << port << " ha abbandonato la chat..."; //writes in the file something such as: "...carlo@192.168.43.223:58160 ha abbandonato la chat..." on a new line.
    dbFile.close(); //file closing.
    release(); //release of the file lock.

    system("clear"); //let's clean up the screen of the server so that it's not showed the previously printed and now deprecated chat content.
    dbFile.open("db.txt", ios::in); //opening of the db.txt file with only read permissions.
    while( getline(dbFile,temp) ){ //line by line till the end of the db.txt file content is printed, no storing in a variable or sending is needed because client has left the chat.
        temp += "\n";
        cout << temp;
    }
    dbFile.close(); //file closing.

    exit(0); //process has fulfilled his assignment, it can now rest in peace.

}

//it is guaranteed an orderly fashion exiting from the main process, children will follow.
void orderlyFashionExitingS(int signal){
    close(*refServerSocket); //serverSocket is closed thanks to the reference previously declared.
    release(); //mutex lock is released in can someone has it.
    dbFile.close(); //db.txt file is closed.
    remove("db.txt"); //db.txt file is deleted since is no more needed, this line can be commented if needs change.
    cerr << "\nCHIUSURA DEL SERVIZIO\n";
    
    exit(0); //exiting from main process.
}

//the mutex lock is structured as follows. On beginning lock is available, if someone wants it, he calls acquire() and takes it, if other processes now want the clock the have to stay in busy waiting till it newly comes available and so the owner releases it with release().
void acquire(){
    while(!available)
        ;
    available = false;
}

void release(){
    available=true;
}
