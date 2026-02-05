#include <stdio.h> 
#include <stdbool.h>
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <signal.h>
#include <errno.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <sys/select.h>
#include <arpa/inet.h> 
#include <netinet/in.h> 

#define PORT 1234

#define BUFFER_SIZE 1024 
#define TAG_SIZE 16
#define MENU_TAG "MENU"                 //fase menu di gioco
#define NICK_TAG "NICKNAME"             //fase scelta nickname
#define QUEST_TAG "QUESTION"            //domanda del quiz
#define ANSW_TAG "ANSWER"               //risposta al quiz
#define ESIT_TAG "ESIT"                 //esito della risposta
#define SCORE_TAG "SCORE"               //fase classifica
#define QUIT_TAG "QUIT"                 //uscita dal gioco
#define FINALSCORE_TAG "FINALSCORE"     //classifica finale (a quiz terminati)

//funzione per inviare messaggi - prima tag e lunghezza poi il messaggio vero e proprio
int send_msg(int sock, const char* tag, const char* payload) {
    int length = strlen(payload);
    int net_length = htonl(length); //conversione length in network byte order
    int bytes_sent;

    // verifica che il messagggio non sia troppo grande
    if (length >= BUFFER_SIZE){
        perror("Messaggio troppo lungo, rifiutato.\n");
        close(sock);
        exit(1);
    } 

    // invia tag - dimensione fissa
    if(send(sock, tag, TAG_SIZE, 0) != TAG_SIZE){
        perror("Errore invio tag");
        close(sock);
        return -1;
    }

    // invia lunghezza
    bytes_sent = send(sock, &net_length, sizeof(net_length), 0);
    if (bytes_sent != sizeof(int)){
        printf("Errore invio net_length.\n");
        close(sock);
        exit(1);
    }
    
    // invia messaggio
    int ret = send(sock, payload, length, 0);
    if(ret == -1){
        printf("Errore invio payload. \n");
        close(sock);
        exit(1);
    }

    return ret;
}

//funzione per ricevere messaggi - prima tag e lunghezza poi il messagggio vero e proprio
int recv_msg(int sock, char* tag, char* buffer) {
    int length;
    int net_length;
    int bytes_received, tag_received, msg_received;

    // ricezione tag
    tag_received = recv(sock, tag, TAG_SIZE, 0);
    if(tag_received <= 0){
        perror("Errore invio tag");
        close(sock);
        return -1;
    }
    
    // ricezione lunghezza del messaggio
    bytes_received = recv(sock, &net_length, sizeof(net_length), 0);
    if (bytes_received <= 0) {
        perror("Errore ricezione net_length.\n");
        close(sock);
        exit(1);
    }

    length = ntohl(net_length); //ocnversione da formato di rete a formato dell'host

    // verifica che il messagggio non sia troppo grande
    if (length >= BUFFER_SIZE){
        perror("Messaggio troppo lungo, rifiutato.\n");
        close(sock);
        exit(1);
    } 
        
    // ricezione contenuto del messaggio
    msg_received = recv(sock, buffer, length, 0);
    if (msg_received <= 0) {
        perror("Errore ricezione payload\n");
        close(sock);
        exit(1);
    }

    buffer[msg_received] = '\0'; // Aggiunge terminatore
    return msg_received;
}

