#include "utils.h"

int sd;

//se il giocatore fa ctrl^C o chiude il terminale richiede di terminare la partita
void handler_client(int sig){
    printf("Il giocatore ha chiuso partita da terminale\n");
    char buffer[BUFFER_SIZE] = "quit";
    send_msg(sd, QUIT_TAG ,buffer); 

    close(sd);
    exit(0);
}

//stampa il menù di avvio e controlla che la risposta a quest'ultimo sia valida
int start_menu(){

    int scelta = 0;
    char buffer[10];

    printf("Trivia quiz\n");
    printf("+++++++++++++++++++++++\n");
    printf("Menù:\n");
    printf("1-Comincia una sessione di Trivia\n");
    printf("2-Esci\n");
    printf("+++++++++++++++++++++++\n");
    printf("La tua scelta:\n");
    
    while (1) {
        fgets(buffer, sizeof(buffer), stdin); 

        // se l'utente inserisce caratteri o numero non validi
        if (sscanf(buffer, "%d", &scelta) == 1 && (scelta == 1 || scelta == 2)) {
            if(scelta == 2)
                return 0;
            else 
                return 1;
        }
        printf("Scelta non valida, riprova: "); //input non valido
    }
}

int sd;
struct sockaddr_in server_addr;

//quando il giocatore riceve una domanda, invia la risposta e aspetta di ricevere
//prima l'esito della risposta di goni altro messaggio dal server
void manage_question(char* buffer, char* tag, char* response){

    //invio la risposta al server
    send_msg(sd, ANSW_TAG, response);

    //resetto buffer in ingresso
    memset(buffer, '\0', BUFFER_SIZE);
    memset(tag, '\0', TAG_SIZE);

    //ricevo l'esito della risposta
    recv_msg(sd, tag ,buffer);
    if(strcmp(tag, ESIT_TAG) != 0){
        perror("Server disconnesso.\n");
        close(sd);
        exit(1);
    }
    //stampo l'esito
    printf("%s", buffer);
}

int main(int argc, char* argv[]){

    while(1){ //serve per far tornare il client al menù di avvio dopo endquiz
        
        //funzione per ignorare il segnale SIGPIPE, in questo modo un processo non viene terminato se
        //prova a scrivere su un socket chiuso dall'altro lato
        signal(SIGPIPE, SIG_IGN);
        //gestione della chiusura del terminale e ctrl+C
        signal(SIGHUP, handler_client);
        signal(SIGINT, handler_client);
    
        int port = 1234;
        
        if(argc > 1){
            port = atoi(argv[1]);
            if(port == 0){
                port = 1234;
            }
        } 
    
        char buffer[BUFFER_SIZE] = {0}; //buffer di appoggio per messaggi dal server
    
        sd = socket(AF_INET, SOCK_STREAM, 0);
        if(sd == -1){
            perror("Errore nella creazione del socket");
            exit(1);
        }
    
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        if(inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) < 0 ){
            perror("Errore conversione indirizzo IP");
            close(sd);
            exit(EXIT_FAILURE);
        }
        
        if(start_menu() != 1){
            printf("Uscita..\n");
            close(sd);
            return 0;
        }
    
        if(connect(sd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
            perror("Errore nella connessione al server");
            close(sd);
            exit(1);
        }
       
        bool started = false; //se il giocatore ha iniziato a giocare
        char previous_tag[TAG_SIZE] = {0};
    
        while (1) {
            char response[BUFFER_SIZE] = {0}; //buffer di appoggio per risposta
            char tag[TAG_SIZE] = {0}; //buffer per ricezione tag
            char response_tag[TAG_SIZE] = {0}; //buffer di appoggio per tag da inviare 

            memset(buffer, '\0', BUFFER_SIZE); //reset buffer 
    
            recv_msg(sd, tag ,buffer);

            
            //se il client riceve una domanda vuol dire che ha iniziato il quiz
            if(strcmp(tag, QUEST_TAG) == 0)
                started = true;
            
            // stampa il messaggio del server
            printf("%s", buffer);
            
            if(strcmp(tag, QUIT_TAG) == 0){
                // il server è stato terminato o è stata chiusa la connessione
                close(sd);
                printf("Connessione con il server chiusa.\n");
                return 0;
            }

            do {
                //finchè il giocatore non invia una riga non vuota
                // o comando speciale (endquiz, show score) nella fase di classifica finale
                memset(response, '\0', BUFFER_SIZE);
                fgets(response, BUFFER_SIZE, stdin);
            } while((strcmp("\n", response) == 0));
            printf("\n");

            
            response[strcspn(response, "\n")] = '\0';
            
            
            if(strcmp(response, "endquiz") == 0){
                //richiesta di fine quiz
                send_msg(sd, QUIT_TAG, response);
                printf("Uscita...\n");
                break;
            }
            
            if(strcmp(response, "show score") == 0 && started){
                //richiesta di vedere la classifica 
                
                // salvo il tag precedente per continuare il servizio dopo la ricezione
                // della classifica
                if(strcmp(tag, SCORE_TAG) != 0)
                strcpy(previous_tag, tag);
                
                send_msg(sd, SCORE_TAG, response);
                continue;
            }

            if(strcmp(tag, FINALSCORE_TAG) == 0){
                // se il giocatore inserisce qualcosa di diverso da un comando speciale alla classifica finale
                // il gioco termina
                if(strcmp(response, "show score") != 0 && strcmp(response, "endquiz") != 0){
                    send_msg(sd, QUIT_TAG, response);
                    printf("Uscita...\n");
                    break;
                }
            }
    
            if(strcmp(tag, SCORE_TAG) == 0){
                //ricezione della classifica

                //debug //printf("prossimo tag:%s \n", previous_tag);
                //a seconda del tag precedente rispondo in modo diverso
                if(strcmp(previous_tag, QUEST_TAG) == 0)
                    manage_question(buffer, tag, response);
                else 
                    send_msg(sd, previous_tag, response);
                continue;
            }
    
            //scelta nickname
            if(strcmp(tag, NICK_TAG) == 0)
                strcpy(response_tag, NICK_TAG);
    
            //scelta quiz
            if(strcmp(tag, MENU_TAG) == 0) 
                strcpy(response_tag, MENU_TAG);
    
            //ricezione domanda del quiz
            if(strcmp(tag, QUEST_TAG) == 0){
                manage_question(buffer, tag, response);
                continue;
            }
    
            send_msg(sd, response_tag, response);
        }
    
        close(sd);
    }

    return 0;
}