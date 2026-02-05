#include "utils.h"

#define MAX_PLAYERS 10 // numero massimo di giocatori allo stesso momento
#define QUESTIONS 5 // numero domande per tema
#define QUESTION_SIZE 512 // lunghezza massima domanda
#define NICKNAME_SIZE 25 // lunghezza massima nickname giocatore
#define THEMES 3 // numero dei temi

char questions[QUESTIONS * THEMES][BUFFER_SIZE];
char answers[QUESTIONS * THEMES][BUFFER_SIZE];
char scores[BUFFER_SIZE];

struct player{
    char nickname[NICKNAME_SIZE];
    int quiz_terminated[THEMES]; //quiz terminati dal giocatore (1 -> terminato, 0 -> non terminato)
    int quiz_point[THEMES]; //punti per ogni quiz
    int current_quiz; //numero di quiz corrente
    int current_question; //numero di domanda corrente
    int sd; //socket relativo al player
};
struct player players[MAX_PLAYERS];
int available_to_play = 0; //posti disponibili

//inizializza struttura dati giocatore i
void init_player(int i){ 
    strcpy(players[i].nickname, "");
    memset(&players[i].quiz_terminated, 0, sizeof(players[i].quiz_terminated));
    memset(&players[i].quiz_point, 0, sizeof(players[i].quiz_point));
    players[i].current_quiz = 0;
    players[i].current_question = 0;
    players[i].sd = -1;
    available_to_play++;
}

// domande e relative risposte 
struct quiz{ 
    char question[QUESTION_SIZE];
    char answer[QUESTION_SIZE];
};
struct quiz quizzes[THEMES * QUESTIONS];

//trova indice del posto giocatore disponibile nella struttura players
int find_available_player(){ 
    int i = 0;
    for(;i < MAX_PLAYERS; i++){
        if(players[i].sd == -1)
            return i;
    }
    return -1;
}

//stampa le statistiche del gioco ogni volta che arriva un messaggio dal client
void print_stats(){
    int playing = MAX_PLAYERS - available_to_play;
    printf("Partecipanti(%d)\n", playing);

    for(int i = 0; i < MAX_PLAYERS; i++){
        if(strcmp(players[i].nickname, "") != 0)
            printf("- %s \n", players[i].nickname);
    }

    printf("\n");

    for (int i = 0; i < THEMES; i++){
        printf("Punteggio tema %d\n", i+1);
        for(int j = 0; j < MAX_PLAYERS; j++)
            if(strcmp(players[j].nickname, "") != 0)
                printf("- %s %d\n", players[j].nickname, players[j].quiz_point[i]);

        printf("\n");
    }

    for (int i = 0; i < THEMES; i++){
        printf("Quiz tema %d completato\n", i + 1);
        for(int j = 0; j < MAX_PLAYERS; j++)
            if(strcmp(players[j].nickname, "") != 0 && players[j].quiz_terminated[i])
                printf("- %s\n", players[j].nickname);
        printf("--------- \n");
        if(i != THEMES -1)
            printf("\n");
    }
}

//stampa tutto, fase iniziale del server
void print_all(){
    printf("\nTriva quiz \n");
    printf("++++++++++++++++++++++++++++++\n");
    printf("Temi:\n");
    printf("1 - Capitali del mondo e geografia\n");
    printf("2 - Calciatori e squadre\n");
    printf("3 - Videogiochi classici\n");
    printf("++++++++++++++++++++++++++++++\n");
    printf("\n");

    print_stats();
}

//invia al client la richiesta di scelta di nickname
void choice_nick(int player_sd){
    char request_nick[BUFFER_SIZE];
    strcpy(request_nick, "\nTrivia Quiz\n");
    strcat(request_nick, "++++++++++++++++++++++++++++++\n");
    strcat(request_nick, "Scegli un nickname (deve essere univoco):\n");
    send_msg(player_sd, NICK_TAG, request_nick);
}

//controlla che il nickname scelto dal client non esista già, 
//se esiste ritorna 0
int check_nick(char * nickname){
    int ok = 1;
    for(int i = 0; i < MAX_PLAYERS; i++){
        if(strcasecmp(nickname, players[i].nickname) == 0){
            ok = 0;
            return ok;
        }
    }
    return ok;
}

//invia al client il menù di gioco
//se first è true allora significa che il giocatore riceve il menù per la prima volta
//quindi non ha completato nessun quiz
void menu(int ind_player, char* response, bool first){
    struct player* current_player = &players[ind_player];

    if(first){
        //prima volta che viene mostrato il menu -> nessun quiz completato
        strcpy(response, "Quiz disponibili\n");
    } else {
        strcpy(response, "Complimenti, hai terminato il quiz, scegline un altro!\n");
        strcat(response, "Quiz disponibili\n");
    }
    strcat(response, "++++++++++++++++++++++++++++++\n");
    strcat(response, "1 - Capitali e Geografia\n");
    strcat(response, "2 - Squadre e Calciatori\n");
    strcat(response, "3 - Videogiochi Classici\n");
    strcat(response, "++++++++++++++++++++++++++++++\n");
    strcat(response, "La tua scelta:\n");

    send_msg(current_player->sd, MENU_TAG, response);
}

//gestione di una nuova connessione al server da parte di un client
int new_connection(int server_sd){
    struct sockaddr_in cl_addr;
    int addrlen = sizeof(cl_addr);
    int client_sd = accept(server_sd, (struct sockaddr*)&cl_addr, (socklen_t*)&addrlen);

    if(client_sd < 0){
        perror("Accept fallita \n");
        return -1;
    }

    //se non ci sono posti disponibili invia il tag QUIT al client e chiude la connessione
    if(available_to_play == 0){
        send_msg(client_sd, QUIT_TAG, "Server al completo, riprova più tardi\n");
        close(client_sd);
        return 0;        
    }
    
    //altrimenti aggiorna le strutture dati utili al gioco
    int index = find_available_player();
    players[index].sd = client_sd;
    available_to_play--;
    
    //manda la richiesta di scelta nickname al nuovo client
    choice_nick(players[index].sd);
    return 1;
}

// controlla che la scelta di quiz inviata dal giocatore sia valida 
// se valida ritorna 1 altrimenti 0
int check_quiz_choice(int ind_player, const char* q, char* response){
    struct player* current_player = &players[ind_player];
    int choice;

    if (strlen(q) != 1 || q[0] < '1' || q[0] > '3') {
        //la scelta non è valida
        strcpy(response, "La scelta fornita non è valida. Riprova:\n");
        return 0; 
    } else {
        choice = q[0] - '0'; //conversione da char a int

        if(current_player->quiz_terminated[choice - 1] == 1){
            // il giocatore ha già finito il quiz scelto
            strcpy(response, "Hai già terminato il quiz scelto. Riprova:\n");
            return 0;
        }

        //aggiorna la struttura dati
        current_player->current_quiz = choice - 1;
        current_player->current_question = QUESTIONS * (choice - 1);
        return 1;
    }
}

//concatena al buffer response la classifica di ogni tema in ordine decrescente per punteggio
void get_scoreboard(char* response) {
    //struttura utile alla creazione della classifica
    struct {
        char nickname[NICKNAME_SIZE];
        int score;
    } scores[MAX_PLAYERS];

    char temp[BUFFER_SIZE] = {0};

    for (int theme = 0; theme < THEMES; theme++) {
        int active_count = 0;

        // lista giocatori attivi e punteggi per il tema i
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (players[i].sd != -1) {
                strcpy(scores[active_count].nickname, players[i].nickname);
                scores[active_count].score = players[i].quiz_point[theme];
                active_count++;
            }
        }

        // ordina in ordine decrescente per punteggio (bubble sort)
        for (int i = 0; i < active_count - 1; i++) {
            for (int j = 0; j < active_count - i - 1; j++) {
                if (scores[j].score < scores[j + 1].score) {
                    // swap
                    char tmp_name[NICKNAME_SIZE];
                    strcpy(tmp_name, scores[j].nickname);
                    strcpy(scores[j].nickname, scores[j + 1].nickname);
                    strcpy(scores[j + 1].nickname, tmp_name);

                    int tmp_score = scores[j].score;
                    scores[j].score = scores[j + 1].score;
                    scores[j + 1].score = tmp_score;
                }
            }
        }

        // formattazione testo
        sprintf(temp, "Punteggio tema %d:\n", theme + 1);
        strcat(response, temp);
        for (int i = 0; i < active_count; i++) {
            sprintf(temp, "%s %d\n", scores[i].nickname, scores[i].score);
            strcat(response, temp);
        }

        strcat(response, "\n");
    }
}

//funzione che parte quando un giocatore termina uno dei tre quiz, 
//invia al giocatore di nuovo il menu dei quiz oppure la classifica
// nel caso in cui li abbia terminati tutti
void term_quiz(int ind_player, char* response){
    struct player* current_player = &players[ind_player];

    current_player->quiz_terminated[current_player->current_quiz] = 1;
    bool terminated = true;

    // se terminated rimane true allora il giocatore ha finito tutti i quiz
    for (int i = 0; i < THEMES && 
        (terminated = terminated && current_player->quiz_terminated[i] == 1); i++); 

    if(terminated){
        // ha finito tutti i quiz
        strcpy(response, "Complimenti, hai completato tutti i quiz, ecco i risultati:\n");
        get_scoreboard(response);
        strcat(response, "Scrivi endquiz per uscire. Ci vediamo presto.\n");

        //mando la classifica finale al giocatore
        send_msg(current_player->sd, FINALSCORE_TAG, response);
    } else {
        menu(ind_player, response, false);
    }
}

// riempie il buffer per l'invio del messaggio con 
// la domanda per il giocatore numero ind_player
void question(int ind_player, char* response){
    struct player* current_player = &players[ind_player];

    //numero del quiz
    int i = current_player->current_quiz;
    strcpy(response, "Quiz - ");

    // intestazione quiz
    (i == 0) ? strcat(response, "Capitali e Geografia") :
    (i == 1) ? strcat(response, "Squadre e Giocatori") : 
    strcat(response, "Videogiochi classici");

    strcat(response, "\n++++++++++++++++++++++++++++++\n");
    strcat(response, quizzes[current_player->current_question].question);

    //debug
    //printf("Richiedo la domanda %d, tema %d: %s\n", current_player->current_question, current_player->current_quiz, quizzes[current_player->current_question].question);

    strcat(response, "\nRisposta: \n");
    send_msg(current_player->sd, QUEST_TAG, response);
}

//controlla la validità della risposta data dal giocatore, 
// invia l'esito di quest'ultima e controlla se il giocatore 
// in questione ha finito il quiz
void check_answer(int ind_player, const char* answer, char* response){
    struct player* current_player = &players[ind_player];

    int ok = 0; //se la risposta è giusta
    if(strcasecmp(answer, quizzes[current_player->current_question].answer) == 0){
        send_msg(current_player->sd, ESIT_TAG, "Risposta corretta\n");
        ok = 1;
    } else {
        send_msg(current_player->sd, ESIT_TAG, "Risposta errata\n");
    }
    current_player->quiz_point[current_player->current_quiz] += ok;
    
    //controllo se ha finito le domande del quiz
    if((current_player->current_question + 1) % 5 == 0){
        //debug
        //printf("quiz n. %d, di %s terminato\n", current_player->current_quiz, current_player->nickname);
        term_quiz(ind_player, response); 
        return;
    }
    
    current_player->current_question++;
    //se non ha finito invio una nuova domanda
    question(ind_player, response);
}

//gestione di un messaggio da parte di un giocatore, 
void manage_player(int ind_player){
    char message[BUFFER_SIZE] = {0}; //buffer di appoggio per messaggio ricevuto da client
    char response[BUFFER_SIZE] = {0}; // buffer di apposìggio per inviare messaggio a client
    char tag[TAG_SIZE] = {0}; //buffer di appoggio per ricezione del tag

    struct player* current_player = &players[ind_player];

    recv_msg(current_player->sd, tag, message);

    //fase scelta del nickname
    if(strcmp(tag, NICK_TAG) == 0){
        // il giocatore non ha ancora scelto il nickname, quindi il messaggio contiene il nickname
        if(check_nick(message) == 0){
            strcpy(response, "Il nickname scelto esiste già, scegline un altro: \n");
            send_msg(current_player->sd, NICK_TAG,response);
        } else {
            strcpy(current_player->nickname, message);
            menu(ind_player, response, true);
        }
    }

    //fase menu di gioco
    if(strcmp(tag, MENU_TAG) == 0){
        //il giocatore ha inviato la scelta per un quiz
        if(check_quiz_choice(ind_player, message ,response) == 0){
            send_msg(current_player->sd, MENU_TAG, response);
        } else {
            //debug printf("Procedo a inviare domanda...\n");
            question(ind_player, response);
        }
    }

    //fase di gioco: ricevuta una risposta a una domanda
    if(strcmp(tag, ANSW_TAG) == 0){
        //il giocatore ha risposto a una domanda del quiz
        check_answer(ind_player, message, response);
    }

    //richiesta di visionare la classifica da parte del giocatore
    if(strcmp(tag, SCORE_TAG) == 0){
        strcpy(response, "Ecco la classifica a questo momento:\n");
        get_scoreboard(response);
        strcat(response, "Per continuare rispondi alla domanda precedente\n");
        send_msg(current_player->sd, SCORE_TAG, response);
    }

    //il giocatore è uscito
    if(strcmp(tag, QUIT_TAG) == 0){
        printf("il giocatore %s è uscito.\n", current_player->nickname);
        close(current_player->sd);
        init_player(ind_player);
    }
    
    printf("\n");
    //alla fine stampo le statistiche a ogni messaggio
    print_stats();

}

int server_sd, client_sd, max_sd, sd, activity;
struct sockaddr_in sv_addr;
fd_set readfds;
char buffer[BUFFER_SIZE] = {0};
int addrlen = sizeof(sv_addr);

//carica le domanda e le risposte di ogni quiz
int load_data(){
    FILE *fq = fopen("questions.txt", "r");
    FILE *fa = fopen("answers.txt", "r");

    if (!fq || !fa) {
        perror("Errore apertura file");
        if (fq)
            fclose(fq);
        if (fa)
            fclose(fa);
        return -1;
    }
    
    int i = 0;
    for(; i < THEMES * QUESTIONS ; i++ ){

        if (!fgets(quizzes[i].question, QUESTION_SIZE, fq) ||
            !fgets(quizzes[i].answer, QUESTION_SIZE, fa)) {

            break; // Se non riesce a leggere, esce
        }

        // rimuovo il carattere finale e aggiungo il carattere di fine stringa
        quizzes[i].question[strcspn(quizzes[i].question, "\n")] = '\0';
        quizzes[i].answer[strcspn(quizzes[i].answer, "\n")] = '\0';
        //printf("%d: %s \n", i, quizzes[i].question);

    }
    
    fclose(fq);
    fclose(fa);
    return i; // numero di domande caricate
}

//parte alla disconnessione del server con ctrl^C o chiusura
//del terminale, chiude i socket dei giocatori connessi e il socket del server
void handler_server(int sig){
    printf("\nDisconnessione del server\n");
    for(int i=0; i<MAX_PLAYERS; i++){
        if(players[i].sd != -1){
            close(players[i].sd);
        }
    }
    close(server_sd);
    exit(0);
}

int main(int argc, char* argv[]){
    //ignora il segnale SIGPIPE, così un processo non viene terminato se
    //prova a scrivere su un socket chiuso dall'altro lato
    signal(SIGPIPE, SIG_IGN);
    //gestione della chiusura del terminale e ctrl+C
    signal(SIGHUP, handler_server);
    signal(SIGINT, handler_server);


    if(load_data() != QUESTIONS * THEMES){
        printf("Errore nel caricamento delle domande.");
        exit(1);
    }

    server_sd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_sd == -1){
        perror("Errore nella creazione del socket");
        exit(1);
    }

    sv_addr.sin_family = AF_INET;
    sv_addr.sin_addr.s_addr = INADDR_ANY;
    sv_addr.sin_port = htons(PORT);
    if(bind(server_sd, (struct sockaddr*)&sv_addr, sizeof(sv_addr)) < 0 ){
        perror("Errore nel bind");
        close(server_sd);
        exit(1);
    }

    if(listen(server_sd, MAX_PLAYERS) < 0){
        perror("Errore nella listen");
        close(server_sd);
        exit(1);
    }
    
    for(int i = 0; i < MAX_PLAYERS; i++)
        init_player(i);
    print_all();
    
    while (1) {
        //reset del set e del max_sd
        FD_ZERO(&readfds);
        FD_SET(server_sd, &readfds);
        max_sd = server_sd;

        //aggiungo ogni volta tutti i client attivi
        for (int i = 0; i < MAX_PLAYERS; i++) {
            sd = players[i].sd;
            if (sd > 0) 
                FD_SET(sd, &readfds);
            if (sd > max_sd) 
                max_sd = sd;
        }

        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if(activity < 0){
            if(errno == EINTR){
                continue;
            } else {
                perror("Errore in select");
                close(server_sd);
                exit(1);
            }
        }

        // nuovo giocatore
        if (FD_ISSET(server_sd, &readfds)) {
            //debug printf("Nuova connessione in corso...\n");
            new_connection(server_sd);
        }

        // giocatori già connessi
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if(FD_ISSET(players[i].sd, &readfds)){
                //debug printf("Gestisco giocatore già connesso...\n");
                manage_player(i);
            }

        }    
    }
    return 0;
}
