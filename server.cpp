#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <math.h>
//Fireman and server code credit goes to Professor Rincon 
void fireman(int) { //waits for child processes to finish execution
   while (waitpid(-1, NULL, WNOHANG) > 0);
}
int find_length_of_code(double px, int length) { //finding length of our code to see what is the length required for the final code
    length = ceil(log2(1 / px) + 1);
    return length;
}
std::string fbarBinary(double fbar_sum, int length) { //function to generate the binary of the fbar value by converting from decimal to binary and returning the final generated code
    std::string binaryCode = "";
    double decimal = fbar_sum;
    for (int l = 0; l < length; l++) { //add binary digits up to the max length for that probability
        decimal *= 2;
        if (decimal >= 1) { //adding binary value to our code
            binaryCode += "1";
        }
        else {
            binaryCode += "0";
        }
        std::string binaryStr = std::to_string(decimal);
        for (int i = 0; i < binaryStr.length(); i++) { //getting only the values after the decimal point to calculate for the next binary value
            if (binaryStr[i] == '.') {
                binaryStr = binaryStr.substr(i, binaryStr.length() - 1);
            }
        }
        decimal = stod(binaryStr); //converting binary string to decimal so can solve for the next value for the code
    }
    return binaryCode;
}
std::string fbar(float half_px, float cumu_sum) { //function that calculates the fbar value and then calls a function to find the length of our code and then calls a second function to convert from decimal to binary. This function then receives the binary code and then returns that binary code to the caller
    double fbar_sum = cumu_sum + half_px;
    double px = half_px * 2; //stores px, will need it to find length of the code
    int length = 0;
    length = find_length_of_code(px, length);
    std::string code = fbarBinary(fbar_sum, length);
    return code;
}
int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno, clilen;
    char buffer[256], buffer2[256];
    struct sockaddr_in serv_addr, cli_addr;
    int n;
    signal(SIGCHLD, fireman); //creates signal and calls fireman function to take care of zombie processes
    if (argc < 2) {
        std::cerr << "ERROR, no port provided\n";
        exit(1);
    }
    sockfd = socket(AF_INET, SOCK_STREAM, 0); //creates socket function
    if (sockfd < 0) {
        std::cerr << "ERROR opening socket";
        exit(1);
    }
    bzero((char *)&serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET; //internet family of protocols
    serv_addr.sin_addr.s_addr = INADDR_ANY; //any address from internet can connect to this server
    serv_addr.sin_port = htons(portno); //creating a language common to all platforms
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) { //connecting empty socket w/ portno we want to use
        std::cerr << "ERROR on binding";
        exit(1);
    }
    listen(sockfd, 10); //maximum # of concurrent connections the server can handle
    clilen = sizeof(cli_addr);
    while (true) { //infinite loop, server keeps running waiting for info. from client
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, (socklen_t *)&clilen); //waiting for requests from the server
        if (fork() == 0) //each request we get from client is handled by a child process
        {
            if (newsockfd < 0)
            {
                std::cerr << "ERROR on accept";
                exit(1);
            }
            bzero(buffer, 256); //adds terminating character to the end of buffer
            n = read(newsockfd, buffer, sizeof(double)); //reads half_px from client
            bzero(buffer2, 256);
            int m = read(newsockfd, buffer2, sizeof(double)); //reads cumulative sum from client
            if (n < 0 || m < 0)
            {
                std::cerr << "ERROR reading from socket";
                exit(1);
            }
            double halfpx = std::atof(buffer); //convert char array to float value
            double cumu_sum = std::atof(buffer2);

            std::string code = fbar(halfpx, cumu_sum); //child function will pass info. to functions to generate and receive the code back after executing the shannon-fannon-elias algorithm
            
            n = write(newsockfd, code.c_str(), code.length()); //writing code we calculated above to the client using a socket
            if (n < 0)
            {
                std::cerr << "ERROR writing to socket";
                exit(1);
            }
            close(newsockfd); //close socket connection
            _exit(0);
        }
    }
    close(sockfd); //close connection to socket
    return 0;
}
