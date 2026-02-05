# TriviaQuizMulti

Multiplayer client-server quiz application developed in C for the Reti Informatiche course - University of Pisa.

## Description

TriviaQuizMulti is an interactive quiz system that allows multiple players to connect simultaneously to a server and answer questions on three different topics:

- **World Capitals and Geography**
- **Football Players and Teams**
- **Classic Video Games**

The system supports up to 10 simultaneous players and maintains separate leaderboards for each topic.

## Architecture

- **Protocol**: TCP/IP with stream sockets
- **Model**: Client-Server with I/O multiplexing (select)
- **Default Port**: 1234
- **Communication**: Custom protocol based on TAG + length + payload

### Message Structure

Each message follows this format:

1. **TAG** (16 fixed bytes) - identifies the message type
2. **Length** (4 bytes in network byte order)
3. **Payload** (actual content)

## Features

### Server

- Management of up to 10 concurrent clients using `select()`
- Loading questions/answers from text files
- Game state management for each player
- Real-time score calculation and leaderboards
- Graceful shutdown with signal handling (SIGINT, SIGHUP)

### Client

- Interactive menu for quiz selection
- User input validation
- Real-time leaderboard viewing (command `show score`)
- Controlled disconnection (command `endquiz`)

## Compilation

```bash
# Server
gcc -o server TriviaQuizMulti/server_pellegrini.c

# Client
gcc -o client TriviaQuizMulti/client_pellegrini.c
```

## Usage

### Starting the Server

```bash
./server
```

The server listens on port 1234 and loads questions from `questions.txt` and `answers.txt`.

### Starting the Client

```bash
./client [port]
```

If not specified, the default port is 1234.

## Configuration Files

- **questions.txt**: contains the 15 questions (5 per topic)
- **answers.txt**: contains the corresponding answers

## Special Commands

During the game, the client can use:

- `show score` - view the current leaderboard
- `endquiz` - end the session

## Project Structure

```
TriviaQuizMulti/
├── README.md
├── documentazione.pdf
└── TriviaQuizMulti/
    ├── server_pellegrini.c    # Server source code
    ├── client_pellegrini.c    # Client source code
    ├── utils.h                # Header with communication functions
    ├── questions.txt          # Questions database
    └── answers.txt            # Answers database
```

## Communication Protocol

TAGs used to identify messages:

- `MENU` - game menu phase
- `NICKNAME` - nickname selection phase
- `QUESTION` - quiz question
- `ANSWER` - quiz answer
- `ESIT` - answer result
- `SCORE` - leaderboard display
- `QUIT` - exit game
- `FINALSCORE` - final leaderboard (all quizzes completed)

## Author

**Tommaso Pellegrini**  
University of Pisa  
Course: Reti Informatiche - Ingegneria Informatica

## License

Academic project developed for educational purposes.
