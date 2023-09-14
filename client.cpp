/* 
   Progetto di un'applicativo Client-Server a cura di Carlo Capodilupo 1851916 .
   Lato Client.
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
using namespace std; //so we can just the 'std::' prefix every time.

int* refClientSocket; //reference to client socket for orderly fashion exiting.

void orderlyFashionExitingC(int); //prototype of the orderly fashion exit function activated once termination signal are caught.

int main() {

   char ipServer[16]; //server ip address declaration.
   int  portaServer; //server port number declaration.
   
   //following two lines can become a comment just if uncommented the following 4 lines starting from cout << "..." etc.
   /* 
   strcpy(ipServer, "192.168.43.223"); //pro tempore assignment of the server ip address.
   portaServer = 2115; //pro tempore assignment of the server port number.
   */
   cout << "\nINSERISCI L' IPv4 DEL SERVER\n";
   cin >> ipServer; //alternative interactive assignment of server ip address.
   cout << "\nINSERISCI IL NUMERO DI PORTA DEL SERVER\n";
   cin >> portaServer; //alternative interactive assignment of server port number.
   
   //client socket creation for connection to the service.
   //AF_INET specifies the internet protocol family IPv4.
   //SOCK_STREAM specifies the socket type TCP the only one that assures the stream.
   //0 specifies the default protocol for the previous couple.
   int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
   if(clientSocket<0) {
      cerr << "\nCreazione socket fallita\n";
      exit(1);
   }

   //signal management (no SIGKILL, number 9, handling because it can't be caught).
   refClientSocket = &clientSocket; //assignment of the client socket descriptor to the reference.
   signal(SIGINT, orderlyFashionExitingC); //call of orderly fashion exiting function on SIGINT alias Ctrl+C, number 2.
   signal(SIGTERM, orderlyFashionExitingC); //call of orderly fashion exiting function on SIGTERM normal termination, number 15.
   //from now on, with a termination signal sent to this main and only process there'll be an orderly fashion exit.

   //assignment of server ip address and port number to the structured server address.
   sockaddr_in indirizzoServer; //declaration of the structured server address.
   socklen_t lenghtIndirizzoServer = sizeof(indirizzoServer); //declaration and assignment of the lenght of the structured server address.
   indirizzoServer.sin_family = AF_INET; //specification of the IPv4 internet protocol family.
   indirizzoServer.sin_port = htons(portaServer); //assignment of the server port address converted from Host byte order (Little Endian for Intel x64 processors such as mine) to the Network byte order (Big Endian).
   indirizzoServer.sin_addr.s_addr = inet_addr(ipServer); //assignment of the server ip address converted thanks to inet_addr() from decimal pointed numeration address to binary 32 bit address.
   
   //estanblishment of a connection to the specified socket that'll be assigned by the server.
   //clientSocket is the client socket descriptor of the socket on which the client wants the stream to be.
   //indirizzoServer is the structured server address that points to a sockaddr structure.
   //lenghtIndirizzoServer is the length in byte of the structured server address.
   int esitoConnect = connect(clientSocket, (sockaddr *)&indirizzoServer, lenghtIndirizzoServer);
   if(esitoConnect<0) {
      cerr << "\nConnessione al server fallita\n";
      exit(2);
   }

   cout << "\nConnessione instaurata con il server " << inet_ntoa(indirizzoServer.sin_addr) << ":" << ntohs(indirizzoServer.sin_port) << "\n";
   cout << "Istruzioni:\n1)Per inviare un messaggio, prima scriverlo, poi inserire come ultimo carattere '#' prima di premere ENTER;\n2)Per aggiornare la chat inserire solo il carattere '#' prima di premere ENTER;\n3)Per lasciare la chat inserire il carattere '*' prima di premere ENTER;\n";

   //declaration of the variables needed.
   //size is the size in bytes of messagges that'll be exchanged.
   //nome is the name of the client user, the first message sent to the server.
   //buffer is created to store all incoming messages from the server.
   //message is created to store the outcoming message typed by the user for the groupchat participants.
   //msg_ready_to_display is created to store all the chat content sent by the server each time is required.
   int size=1024;
   char nome[size], buffer[size], message[size], msg_to_display[size];

   //first thing a client does is catching the user's name as input and sending it to the server.
   cout << "\nIl tuo nome: ";
   cin >> nome;
   send(clientSocket,nome,size,0); //sending to the client socket stream a message for the server of size bytes, the message is stored in the nome variable, no flags used.

   //as long as the client doesn't close the connection, messages are exchanged with the server. The closing condition is the '*' input.
   do {
      
      cout << nome << ": "; //to the user we show an input line such as this one: "carlo: ".
      strcpy(message,""); //make sure message has no previous content before sending the current message to the server(this cleaning up is fundamental after the first send to the server).

      //each word typed in input is firstly saved in the buffer and then concatenated to the message to send until the end character '#' is detected, in this case the message is complete and ready to be sent.
      do{
         cin >> buffer; //taking first word type.
         strcat(message,buffer); //appending it to the message to send.
         strcat(message," "); //for each word must be left a space.

         //if we type '*' in the buffer, this means we just want to quit the chat, so we have to communicate our intention to the server and close the socket on our side, this end of the stream.
         if(!strcmp(buffer,"*")){ //"*" is correct, be aware no spaces are needed after the '*' (error: "* "), in fact we are having a comparation with the buffer, not with message.
            send(clientSocket,buffer,size,0); //sending to the server the '*' character which signals our intention to close the stream.
            close(clientSocket); //client socket closing on client side.
            break;
         }
      }while(strcmp(buffer,"#")); //"#" is correct, be aware no spaces are needed after the '#' (error: "# "), in fact we are having a comparation with the buffer, not with message.

      send(clientSocket,message,size,0); //sending to the server the entire message typed by the user of size bytes by putting it on the clientSocket end of the stream, no flags used.

      recv(clientSocket,msg_to_display,size,0); //receiving from the server the entire chat ready to be displayed on the application screen.
      system("clear"); //let's clean up the screen of the server so that it's not showed the previously printed and now deprecated chat content.
      cout << msg_to_display; //printing on screen of the entire chat since it has started, messages before the client join are displayed as well.
      
   }while(strcmp(buffer,"*"));
   //connection has been closed, here's a confirmation message for the client's user
   cout << "\nChiusura della connessione con il server " << inet_ntoa(indirizzoServer.sin_addr) << ":" << ntohs(indirizzoServer.sin_port) << "\n"; //prints something such as: "Chiusura della connessione con il server 192.168.43.223:2115".

   return 0;
   
}

//it is guaranteed an orderly fashion exiting from the main process.
void orderlyFashionExitingC(int signal){
   send(*refClientSocket,"*",1024,0); //client sends to the server the character '*' which signals to the server that the client has closed its end of the stream with the clientSocket closure.
   close(*refClientSocket); //clientSocket is closed thanks to the reference previously declared.
   cerr << "\nCHIUSURA DEL SERVIZIO\n";

   exit(0); //exiting from main process.
}
