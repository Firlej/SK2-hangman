#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
#include <string>
#include <pthread.h>
#include <time.h>       /* time */

#define PORT 2000
#define MAX_CLIENTS 3
#define MIN_CLIENTS 2
#define LIVES 8
#define FOR(i, n) for(int i=0; i<n; i++)
#define WORDS 4

char words[WORDS][30] = {
    "ABC",
    "XYZ",
    "QWERTY",
    "WASD",
};

void reset_game();

struct Client {
   char  username[30];
//    char guessed[27] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
   int lives = LIVES;
   int done = true; // is player done with this round -> lose, or guessed, or just joined an ongoing game
   char word[30];
   int socket; // 0 == free socket
} clients[MAX_CLIENTS];

struct Game {
    char word[30];
    bool on = false;
} game;

bool wordwin(int index) {
    if (strcmp(clients[index].word, game.word) == 0) {
        clients[index].done = true;
        return true;
    } else {
        return false;
    }
}

int allplayers() {
    int cnt = 0;
    FOR(i, MAX_CLIENTS) {
        if (clients[i].socket != 0) {
            cnt++;
        }
    }
    return cnt;
}

int activeplayers() {
    int cnt = 0;
    FOR(i, MAX_CLIENTS) {
        if (clients[i].socket != 0 && !clients[i].done) {
            cnt++;
        }
    }
    return cnt;
}

void sendHeader() {
    char clients_conn[2 + sizeof(char)];
    char clients_playing[2 + sizeof(char)];

    snprintf(clients_conn, sizeof clients_conn, "%d", allplayers());
    snprintf(clients_playing, sizeof clients_playing, "%d", activeplayers());

    char header[100] = "header,";
    strcat(header, "Connected!");
    strcat(header, " | Players connected: ");
    strcat(header, clients_conn);
    strcat(header, " | PLayers playing: ");
    strcat(header, clients_playing);
    strcat(header, ".");

    FOR(i, MAX_CLIENTS) {
        if (clients[i].socket != 0) {
            if((unsigned)send(clients[i].socket, header, strlen(header), 0) != strlen(header)) {
                perror("send");
            }
        }
    }
}

void reset_client(int i) {
    clients[i].socket = 0;
    clients[i].done = true;
    strcpy(clients[i].word, "");
}

void sendWord(int i) {
    char wordUpdate[100] = "wordUpdate,";
    strcat(wordUpdate, clients[i].word);
    strcat(wordUpdate, "\n\nLives left:");
    char str[12];
    sprintf(str, "%d", clients[i].lives);
    strcat(wordUpdate, " ");
    strcat(wordUpdate, str);
    strcat(wordUpdate, ".");
    if((unsigned)send(clients[i].socket, wordUpdate, strlen(wordUpdate), 0) != strlen(wordUpdate)) {
        perror("send");
    }
}

void sendClients() {
    char usersLives[500] = "usersLives,";
    FOR(i, MAX_CLIENTS) {
        if (clients[i].socket != 0) {
            char str[12];
            if (!clients[i].done) {
                strcat(usersLives, "PLAYING | ");
            } else {
                if (strcmp(clients[i].word, "") == 0) {
                    strcat(usersLives, "LOBBY | ");
                } else if (wordwin(i)) {
                    strcat(usersLives, "WINNER | ");
                } else if (clients[i].lives <= 0) {
                    strcat(usersLives, "HANGED | ");
                } else {
                    strcat(usersLives, "LOBBY2 | ");
                }
            }
            sprintf(str, "%d", clients[i].lives);
            strcat(usersLives, clients[i].username);
            strcat(usersLives, ": ");
            strcat(usersLives, str);
            strcat(usersLives, "\n\n");
        }
    }
    strcat(usersLives, ".");

    FOR(i, MAX_CLIENTS) {
        if (clients[i].socket != 0) {
            if((unsigned)send(clients[i].socket, usersLives, strlen(usersLives), 0) != strlen(usersLives)) {
                perror("send");
            }
        }
    }
}

void reset_game() {
    printf("All players: %d\n", activeplayers());
    printf("Active players: %d\n", activeplayers());
    if (activeplayers() > 0 || allplayers() == 0) {
        return;
    }

    if (allplayers() < MIN_CLIENTS) {
        FOR(i, MAX_CLIENTS) {
            if (clients[i].socket != 0) {
                clients[i].done = true;
            }
        }
        sendClients();
        sendHeader();
        return;
    }

    srand(time(NULL));
    int index = rand() % WORDS;
    strcpy(game.word, words[index]);
    printf("%s\n", game.word);
    game.on = true;

    FOR(i, MAX_CLIENTS) {
        if (clients[i].socket != 0) {
            strcpy(clients[i].word, game.word);
            for (int j=0; clients[i].word[j] != '\0'; j++) {
                clients[i].word[j] = '_';
            }
            clients[i].lives = LIVES;
            clients[i].done = false;
            sendWord(i);
        }
    }
    sendClients();
    sendHeader();
}

void guess(int index, char guess) {
    if (game.on && !clients[index].done) {
        bool guessed = false;
        for (int i=0; game.word[i] != '\0'; i++) {
            if (game.word[i] == guess) {
                guessed = true;
                clients[index].word[i] = guess;
            }
        }
        if (wordwin(index)) {
            clients[index].done = true;
            sendClients();
            sendHeader();
            reset_game();
        }
        if (!guessed) {
            clients[index].lives--;
            if (clients[index].lives <= 0) {
                clients[index].done = true;
            }
            sendClients();
            sendHeader();
            reset_game();
        }
        sendWord(index);
    }
}

int main(int argc , char *argv[])
{
    printf("Starting server...\n");

    int opt = true;
    int master_socket, addrlen, new_socket, activity, valread, sd;
    int max_sd;
    struct sockaddr_in address;

    char buffer[1024]; //data buffer of 1K

    //set of socket descriptors
    fd_set readfds;

    // set all sockets to 0
    FOR(i, MAX_CLIENTS) {
        clients[i].socket = 0;
    }

    //create a master socket
    if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    //set master socket to allow multiple connections ,
    if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 ) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    
    //type of socket created
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    printf("Listening on port %d \n", PORT);

    //try to specify maximum of MAX_CLIENTS pending connections for the master socket
    if (listen(master_socket, MAX_CLIENTS) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    //accept the incoming connection
    addrlen = sizeof(address);
    puts("Waiting for connections ...");

    while (true)
    {
        //clear the socket set
        FD_ZERO(&readfds);

        //add master socket to set
        FD_SET(master_socket, &readfds);
        max_sd = master_socket;

        FOR(i, MAX_CLIENTS) {
            //socket descriptor
            sd = clients[i].socket;

            //if valid socket descriptor then add to read list
            if(sd > 0)
                FD_SET(sd, &readfds);

            //highest file descriptor number, need it for the select function
            if(sd > max_sd)
                max_sd = sd;
        }

        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);

        if ((activity < 0) && (errno!=EINTR)) {
            printf("select error");
        }

        // incoming connection
        if (FD_ISSET(master_socket, &readfds))
        {
            // check for errors
            if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            valread = read(new_socket, buffer, 1024);

            // printf("%s\n", buffer); // todo remove

            if (valread <= 0) {
                printf("Empty username\n");
                continue;
            }
            char *ptr = strtok(buffer, ",");
            ptr = strtok(NULL, ",");
            char *username = ptr;

            // check for player count
            if (allplayers() >= MAX_CLIENTS) {
                printf("Too many users\n");
                char limit[30] = "limit.";
                if ((unsigned)send(new_socket, limit, strlen(limit), 0) != strlen(limit) ) {
                    perror("send");
                }
                continue;
            }

            // check for duplicate username
            bool duplicate_user_error = false;
            FOR(i, MAX_CLIENTS) {
                if (clients[i].socket != 0 && strcmp(username, clients[i].username ) == 0) {
                    printf("User tried to log in with duplicate username\n");
                    char duplicate[30] = "duplicate.";
                    if ((unsigned)send(new_socket, duplicate, strlen(duplicate), 0) != strlen(duplicate)) {
                        perror("send");
                    }
                    duplicate_user_error = true;
                    break;
                }
            }
            if (duplicate_user_error) {
                continue;
            }
            
            
            // inform user of socket number - used in send and receive commands
            printf(
                "New connection, socket fd: %d, ip: %s, port: %d\n",
                new_socket,
                inet_ntoa(address.sin_addr),
                ntohs(address.sin_port)
            );

            // save client information
            FOR(i, MAX_CLIENTS)
            {
                if (clients[i].socket == 0)
                {
                    clients[i].socket = new_socket;
                    strcpy(clients[i].username, username);

                    char wordUpdate[100] = "wordUpdate,";
                    strcat(wordUpdate, "Succesfully connected\nPlease wait for\nthe next game to start.");
                    if((unsigned)send(clients[i].socket, wordUpdate, strlen(wordUpdate), 0) != strlen(wordUpdate)) {
                        perror("send");
                    }

                    if (game.on) {
                        clients[i].done = true;
                    }
                    reset_game();
                    sendHeader();
                    sendClients();

                    break;
                }
            }
        }

        // else its some IO operation on some other socket
        FOR(i, MAX_CLIENTS)
        {

            if (FD_ISSET(clients[i].socket, &readfds))
            {
                //Check if it was for closing, and also read the incoming message
                if ((valread = read(clients[i].socket, buffer, 1024)) <= 0)
                {
                    //Somebody disconnected
                    getpeername(clients[i].socket, (struct sockaddr*)&address, (socklen_t*)&addrlen);
                    printf("%s disconnected\n", clients[i].username);

                    //Close the socket and mark as 0 in list for reuse
                    close(clients[i].socket);
                    allplayers();
                    reset_client(i);
                    if (allplayers() == 0) {
                        game.on = false;
                    }
                    reset_game();
                    sendClients();
                    sendHeader();
                } else {
                    //set the string terminating NULL byte on the end of the data read
                    buffer[valread] = '\0';
                    printf("message received from %s: %s\n", clients[i].username, buffer);

                    char delim[] = ",";
                    char *ptr = strtok(buffer, delim);
                    char *type = ptr;
                    ptr = strtok(NULL, delim);
                    char *message = ptr;

                    // printf("%s\n", message);

                    if (strcmp(type, "guess") == 0) {
                        guess(i, message[0]);
                    }
                }
            }
        }
    }

    return 0;
}