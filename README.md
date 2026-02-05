# TriviaQuizMulti

Applicazione client-server per quiz multiplayer sviluppata in C per il corso di Reti Informatiche - Università di Pisa.

## Descrizione

TriviaQuizMulti è un sistema di quiz interattivo che permette a più giocatori di connettersi simultaneamente a un server e rispondere a domande su tre tematiche diverse:

- **Capitali del mondo e geografia**
- **Calciatori e squadre**
- **Videogiochi classici**

Il sistema supporta fino a 10 giocatori contemporaneamente e mantiene classifiche separate per ogni tema.

## Architettura

- **Protocollo**: TCP/IP con socket stream
- **Modello**: Client-Server con multiplexing I/O (select)
- **Porta predefinita**: 1234
- **Comunicazione**: Protocollo custom basato su TAG + lunghezza + payload

### Struttura Messaggi

Ogni messaggio segue il formato:

1. **TAG** (16 byte fissi) - identifica il tipo di messaggio
2. **Lunghezza** (4 byte in network byte order)
3. **Payload** (contenuto effettivo)

## Funzionalità

### Server

- Gestione di massimo 10 client simultanei con select()
- Caricamento domande/risposte da file di testo
- Gestione stato di gioco per ogni giocatore
- Calcolo punteggi e classifiche in tempo reale
- Graceful shutdown con gestione segnali (SIGINT, SIGHUP)

### Client

- Menu interattivo per scelta quiz
- Validazione input utente
- Visualizzazione classifiche in tempo reale (comando `show score`)
- Disconnessione controllata (comando `endquiz`)

## Compilazione

```bash
# Server
gcc -o server TriviaQuizMulti/server_pellegrini.c

# Client
gcc -o client TriviaQuizMulti/client_pellegrini.c
```

## Utilizzo

### Avvio Server

```bash
./server
```

Il server si mette in ascolto sulla porta 1234 e carica le domande dai file `questions.txt` e `answers.txt`.

### Avvio Client

```bash
./client [porta]
```

Se non specificata, la porta predefinita è 1234.

## File di Configurazione

- **questions.txt**: contiene le 15 domande (5 per tema)
- **answers.txt**: contiene le risposte corrispondenti

## Comandi Speciali

Durante il gioco il client può utilizzare:

- `show score` - visualizza la classifica corrente
- `endquiz` - termina la sessione

## Struttura del Progetto

```
TriviaQuizMulti/
├── README.md
├── documentazione.pdf
└── TriviaQuizMulti/
    ├── server_pellegrini.c    # Codice sorgente server
    ├── client_pellegrini.c    # Codice sorgente client
    ├── utils.h                # Header con funzioni di comunicazione
    ├── questions.txt          # Database domande
    └── answers.txt            # Database risposte
```

## Protocollo di Comunicazione

I TAG utilizzati per identificare i messaggi:

- `MENU` - fase menu di gioco
- `NICKNAME` - fase scelta nickname
- `QUESTION` - domanda del quiz
- `ANSWER` - risposta al quiz
- `ESIT` - esito della risposta
- `SCORE` - visualizzazione classifica
- `QUIT` - uscita dal gioco
- `FINALSCORE` - classifica finale (tutti i quiz completati)

## Autore

**Tommaso Pellegrini**  
Università di Pisa  
Corso: Reti Informatiche

## Licenza

Progetto accademico sviluppato per scopi didattici.
