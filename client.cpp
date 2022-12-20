#include <unistd.h>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string>
using namespace std;
//Client to server connection code credit goes to Professor Rincon
struct node { //struct to hold our info. for the main thread to easily access
    node* next;
    char symbol;
    string code;
    double probability;
    double count; //count freq. of letters
    double cumulative_sum;
    int portno;
    struct hostent *server;
};
int findFreq(string in, node*& head) { //finding frequences of symbols and storing them into the struct.
    //first count how many of each unique item there are
    int totalCharLength = in.length();
    int size = 0;
    for (int i = 0; i < totalCharLength; i++) {
        if (head == NULL) { //adding first element into list
            node* tmp = new node();
            tmp->symbol = in[i];
            tmp->next = NULL;
            tmp->count = 1;
            head = tmp;
            continue;
        }
        node* curr = head;
        node* prev = NULL;
        bool exists = false;
        while (curr != NULL) { //checking if element already exists in list so can update it's count
            prev = curr;
            if (curr->symbol == in[i]) { //element exists already, update it's count
                curr->count += 1;
                exists = true;
                break;
            }
            curr = curr->next;
        }
        if (exists)
            continue;
        node* tmp = new node(); //symbol doesn't exist in list so create new symbol
        tmp->symbol = in[i];
        tmp->count = 1;
        tmp->next = NULL;
        prev->next = tmp;
    }
    //Now can find the frequencies of each unique symbol that we counted above
    node* curr = head;
    while (curr != NULL) {
        curr->probability = curr->count / totalCharLength;
        size++;
        curr = curr->next;
    }
    return size;
}
node* sortFreq(node*& head, int size) { //sorting linked list of frequencies in decreasing order by frequency (if same frequency then sort lexicographically)
    node* h2 = NULL;
 
    node* curr = head;
    while (curr != NULL) {
        if (h2 == NULL) { //insert at beginning of new list
            node* tmp = new node();
            tmp->symbol = curr->symbol;
            tmp->probability = curr->probability;
            tmp->next = NULL;
            h2 = tmp;
        }
        else {
            node* tmp = new node();
            tmp->symbol = curr->symbol;
            tmp->probability = curr->probability;
            tmp->next = NULL;
            node* curr2 = h2;
            node* prev = NULL;
            while (curr2 != NULL) {
                if (tmp->probability == curr2->probability) { //same probability, so sort in lexicographic order
                    if (tmp->symbol <= curr2->symbol) { //insert tmp node after curr2 node
                        if (prev == NULL) {
                            tmp->next = curr2;
                            h2 = tmp;
                            break;
                        }
                        else {
                            prev->next = tmp;
                            tmp->next = curr2;
                            break;
                        }
                    }
                }
                if (tmp->probability > curr2->probability) { //sort freq. in decreasing order
                    if (prev == NULL) { //insert at beginning of list as new head
                        tmp->next = curr2;
                        h2 = tmp;
                        break;
                    }
                    else { //insert in middle of list
                        prev->next = tmp;
                        tmp->next = curr2;
                        break;
                    }
                }
                
                prev = curr2; //keep looking for spot to insert
                curr2 = curr2->next;
                if (curr2 == NULL) { //reached end of list so just insert at the end
                    prev->next = tmp;
                }
            }
        }
        curr = curr->next;
    }
    return h2;
}
void cumu_sum(node*& head) { //getting the f(x) value
    node* curr = head;
    double sum = 0;
    while (curr != NULL) {
        curr->cumulative_sum = sum;
        sum += curr->probability;
        curr = curr->next;
    }
}
void* shannon_fano_elias(void* current_prob) { //function to calculate the shannon-fano-elias algorithm, this function will create a socket for each symbol, send the symbols info. to the server, and then wait to get the code for each symbol back from the server which it will then store in the main thread
    node* curr = (node*)current_prob;
    double half_px = curr->probability / 2; //getting half of current probability, send to server
    char buffer[256];
    
    int sockfd, n, m;
    struct sockaddr_in serv_addr; //stores info. about host name
    
    bzero((char *) &serv_addr, sizeof(serv_addr)); //putting terminating character in character array
    serv_addr.sin_family = AF_INET; //set family as AF_INET
    bcopy((char *)curr->server->h_addr, (char *)&serv_addr.sin_addr.s_addr, curr->server->h_length); //copying and setting address of the server
    serv_addr.sin_port = htons(curr->portno);
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0); //child thread creating a socket to communicate with server
    if (sockfd < 0)
        std::cerr<< "ERROR opening socket";

    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) //connecting to the server
    {
        std::cerr << "ERROR connecting";
        exit(1);
    }
    //send half_px and curr->cumulative_sum to the server to get the fbar value there which the server will use to get the code we want and send that code back to the client
    string halfpx = to_string(half_px); //convert decimal to string then to char array below & send that to server
    string cumu_sum = to_string(curr->cumulative_sum);
    n = write(sockfd, halfpx.c_str(), sizeof(double)); //writing info. to the server
    m = write(sockfd, cumu_sum.c_str(), sizeof(double));
    if (n < 0 || m < 0)
    {
        std::cerr << "ERROR writing to socket";
        exit(1);
    }
    bzero(buffer,256);
    n = read(sockfd,buffer,255); //waiting for code. back from the server
    if (n < 0)
    {
        std::cerr << "ERROR reading from socket";
        exit(1);
    }
    curr->code = buffer; //storing code we got back from server in a location accessible by main thread
    close(sockfd);
    return nullptr;
}
void printList(node*& head) { //printing our final codes
    node* curr = head;
    cout << "SHANNON-FANO-ELIAS Codes:\n" << endl;
    while (curr != NULL) {
        cout << "Symbol " << curr->symbol << ", Code: " << curr->code << endl;
        curr = curr->next;
    }
}
int main(int argc, char *argv[])
{
    node* head = NULL;
    string in;
    getline(cin, in); //getting input of one line of characters
    int size = findFreq(in, head); //gets size of our list of elements
    head = sortFreq(head, size); //creating final list that will have the frequencies and symbols sorted in order we want
    cumu_sum(head); //getting fx values from our final list we created above

    if (argc < 3) {
       cerr << "usage " << argv[0] << "hostname port\n";
       exit(0);
    }
    pthread_t tid[size]; //creating n threads below
    node* curr = head;
    for (int i = 0; i < size; i++) {
        curr->portno = atoi(argv[2]); //store port number in our struct
        curr->server = gethostbyname(argv[1]); //store info. about server in struct
        if (curr->server == NULL) {
            cerr << "ERROR, no such host\n";
            exit(0);
        }
        if (pthread_create(&tid[i], nullptr, shannon_fano_elias, curr) != 0) { //executing shanon-fano-elias algo. using posix threads to calculate for each code
            cout << "Can not create the thread" << endl;
            return -1;
        }
        curr = curr->next;
    }
    for (int i = 0; i < size; i++) {
        pthread_join(tid[i], nullptr); //waiting for threads to finish execution
    }
    
    printList(head);
    
    delete head; //deleting the linked list
    return 0;
}
