#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <stdbool.h>
//#define PORT 8080
#define BUFFER_SIZE 1024

int **p1_board, **p2_board;

// Helper functions Declarations
bool begin(int player_fd, int player, int *width, int *height);
void create_board(int *width, int *height);
bool initialize(int player_fd, int player, char* pieces_info, int *width, int *height);
bool shoot_query_forfeit(int shooter_fd, int opponent_fd, int player, int *width, int *height);

//check Piece Shape and rotation for Board Functions
bool shapeInRange(int shape);
bool rotationInRange(int rotation);

bool checkShape1(int rotation, int row, int col, int width, int height);
bool checkShape2(int rotation, int row, int col, int width, int height);
bool checkShape3(int rotation, int row, int col, int width, int height);
bool checkShape4(int rotation, int row, int col, int width, int height);
bool checkShape5(int rotation, int row, int col, int width, int height);
bool checkShape6(int rotation, int row, int col, int width, int height);
bool checkShape7(int rotation, int row,int col, int width, int height);

//this function not only checks if the ships overlap, but also populates the board with ships if it doesn't
bool checkOverlap(int player, int pieces[5][4], int *width, int *height); 
void resetBoard(int player, int *width, int *height);

//shoot helper functions
int count_ships(int **board, int width, int height);

int listen_fd1, listen_fd2, player1_fd, player2_fd;

int main() {
    printf("Welcome to the game of Battleship!\n");

    // Declare socket variables for the players
    struct sockaddr_in address1, address2;
    socklen_t addrlen1 = sizeof(address1), addrlen2 = sizeof(address2);

    // Create sockets for both players
    if ((listen_fd1 = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed for player 1");
        exit(EXIT_FAILURE);
    }
    if ((listen_fd2 = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed for player 2");
        exit(EXIT_FAILURE);
    }

    // Configure address structures
    address1.sin_family = AF_INET;
    address1.sin_addr.s_addr = INADDR_ANY;
    address1.sin_port = htons(2201);

    address2.sin_family = AF_INET;
    address2.sin_addr.s_addr = INADDR_ANY;
    address2.sin_port = htons(2202);

    // Bind sockets to ports
    int opt = 1;
    setsockopt(listen_fd1, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(listen_fd2, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(listen_fd1, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
    setsockopt(listen_fd2, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));


    if (bind(listen_fd1, (struct sockaddr *)&address1, sizeof(address1)) < 0) {
        perror("Bind failed for player 1");
        exit(EXIT_FAILURE);
    }
    if (bind(listen_fd2, (struct sockaddr *)&address2, sizeof(address2)) < 0) {
        perror("Bind failed for player 2");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(listen_fd1, 1) < 0) {
        perror("Listen failed for player 1");
        exit(EXIT_FAILURE);
    }
    if (listen(listen_fd2, 1) < 0) {
        perror("Listen failed for player 2");
        exit(EXIT_FAILURE);
    }

    printf("[Server] Waiting for Player 1 on port 2201...\n");
    if ((player1_fd = accept(listen_fd1, (struct sockaddr *)&address1, &addrlen1)) < 0) {
        perror("Accept failed for player 1");
        exit(EXIT_FAILURE);
    }
    printf("[Server] Player 1 connected!\n");

    printf("[Server] Waiting for Player 2 on port 2202...\n");
    if ((player2_fd = accept(listen_fd2, (struct sockaddr *)&address2, &addrlen2)) < 0) {
        perror("Accept failed for player 2");
        exit(EXIT_FAILURE);
    }
    printf("[Server] Player 2 connected!\n");


    // Begin phase  (also checks for early forfeit)
    int width, height;
    printf("Beginning game...\n");

    bool begin_p1 = begin(player1_fd, 1, &width, &height);
    while(!begin_p1){ // begin_p1 is not initialized
        begin_p1 = begin(player1_fd, 1, &width, &height);
    }
    bool begin_p2 = begin(player2_fd, 2, NULL, NULL);
    while(!begin_p2){ // begin_p2 is not initialized
        begin_p2 = begin(player2_fd, 2, NULL, NULL);
    }

    // Create boards for both players
    create_board(&width, &height);

    // Initialize phase
    bool initialize_p1 = initialize(player1_fd, 1, NULL, &width, &height);
    while(!initialize_p1){ // initialize_p1 is not initialized
        initialize_p1 = initialize(player1_fd, 1, NULL, &width, &height);
    }
    bool initialize_p2 = initialize(player2_fd, 2, NULL, &width, &height);
    while(!initialize_p2){ // initialize_p2 is not initialized
        initialize_p2 = initialize(player2_fd, 2, NULL, &width, &height);
    }


    // Game loop

    //BELOW CHANGE IMPLEMENTATION
    bool game_over = shoot_query_forfeit(player1_fd, player2_fd, 1, &width, &height);
    while (!game_over) {
        game_over = shoot_query_forfeit(player1_fd, player2_fd, 1, &width, &height);
    }

    printf("Game over!\n");
    // Close listening sockets as they are no longer needed
    close(listen_fd1);
    close(listen_fd2);

    close(player1_fd);
    close(player2_fd);
    return 0;
}

// End Server Setup --------------------------------------------------------------------------

//------------------------------------BEGIN------------------------------------------------------>

bool begin(int player_fd, int player, int *width, int *height){
    char buffer[BUFFER_SIZE] = {0};
    int bytes_read = read(player_fd, buffer, BUFFER_SIZE);
    printf("bytes read: %d\n", bytes_read);

    if(bytes_read <= 0){
        printf("Failed to read from Player %d.\n", player);
        return false;
    }

    if(player == 1){
        int extra;
        if(buffer[0] == 'F'){
            printf("Player %d forfeited the game.\n", player);
            send(player1_fd, "H 0", strlen("H 0"), 0);
            send(player2_fd, "H 1", strlen("H 1"), 0);
            exit(EXIT_FAILURE);
            return false;
        }
        if(buffer[0] != 'B'){
            send(player_fd, "E 100", sizeof("E 100"), 0);
            return false;
        }
        if(buffer[0] == 'B' && sscanf(buffer, "B %d %d %d", width, height, &extra) != 2){
            send(player_fd, "E 200", sizeof("E 200"), 0);
            return false;
        }
        if(*width < 10 || *height < 10){
            send(player_fd, "E 200", sizeof("E 200"), 0);
            return false;
        }
        send(player_fd, "A", sizeof("A"), 0);
        return true;
    }
    else if(player == 2){
        int extra;
        if(buffer[0] == 'F'){
            printf("Player %d forfeited the game.\n", player);
            send(player2_fd, "H 0", strlen("H 0"), 0);
            send(player1_fd, "H 1", strlen("H 1"), 0);
            exit(EXIT_FAILURE);
            return false;
        }
        if(buffer[0] != 'B'){
            send(player_fd, "E 100", sizeof("E 100"), 0);
            return false;
        }
        if(buffer[0] == 'B' && sscanf(buffer, "B %d", &extra) == 1){
            send(player_fd, "E 200", sizeof("E 200"), 0);
            return false;
        }
        send(player_fd, "A", sizeof("A"), 0);
        return true;
    }
    return true;
}


void create_board(int *width, int *height){
    p1_board = malloc((*height) * sizeof(int*));
    p2_board = malloc((*height) * sizeof(int*));
    for(int i = 0; i < *height; i++){
        p1_board[i] = malloc((*width) * sizeof(int));
        memset(p1_board[i], 0, (*width) * sizeof(int));

        p2_board[i] = malloc((*width) * sizeof(int));
        memset(p2_board[i], 0, (*width) * sizeof(int));
    }
}

//---------------------------------------INITIALIZE------------------------------------------------>

bool initialize(int player_fd, int player, char* piece_info, int *width, int *height){
    char buffer[BUFFER_SIZE] = {0};
    int bytes_read = read(player_fd, buffer, BUFFER_SIZE);

    if(bytes_read <= 0){
        printf("Failed to read from Player.\n");
        return false;
    }
    if(buffer[0] == 'F'){
        if(player == 1){
            printf("Player %d forfeited the game.\n", player);
            send(player1_fd, "H 0", strlen("H 0"), 0);
            send(player2_fd, "H 1", strlen("H 1"), 0);
            exit(EXIT_FAILURE);
            return false;
        }
        else if(player == 2){
            printf("Player %d forfeited the game.\n", player);
            send(player2_fd, "H 0", strlen("H 0"), 0);
            send(player1_fd, "H 1", strlen("H 1"), 0);
            exit(EXIT_FAILURE);
            return false;
        }
    }
    
    if(buffer[0] != 'I'){
        printf("101: Invalid packet type (Expected Initialize packet)");
        send(player_fd, "E 101", sizeof("E 101"), 0);
        return false;
    }
    int pieces[5][4] = {0};
    char *token = strtok(buffer + 1, " "); //first char skipping I
    int token_count = 0;

    int row = 0;
    int col = 0;
    while(token != NULL){
        token_count++; // Count the tokens
        if(token == NULL){
            send(player_fd, "E 201", sizeof("E 201"), 0);
            return false;
        }
        // If more than 20 tokens are found, return an error
        if(token_count > 20){
            printf("Too many arguments in the initialize packet.\n");
            send(player_fd, "E 201", sizeof("E 201"), 0); // too many arguments
            return false;
        }

        if(*token >= '0' && *token <= '9'){
            int num = atoi(token); //make token an int
            pieces[row][col] = num;
            token = strtok(NULL, " ");
            col++;
            if(col > 3){    //column reached 4 items
                col = 0;
                row++;
            }
        }
        else{
            send(player_fd, "E 201", sizeof("E 201"), 0); //other ascii chars were read
            return false;
        }
    }

    if(token_count < 20){
        send(player_fd, "E 201", sizeof("E 201"), 0);
        return false;
    }

    // each piece should contain information now about its own position
    // printing for debugging purposes
    for(int i = 0; i < 5; i++){
        printf("Piece %d: %d %d %d %d\n", i + 1, pieces[i][0], pieces[i][1], pieces[i][2], pieces[i][3]);
    }

    //Check placement of pieces
    for(int i = 0; i < 5; i++){
        if(!shapeInRange(pieces[i][0])){
            printf("Shape Type out of range\n");
            send(player_fd, "E 300", sizeof("E 300"), 0); // 300: Invalid Initialize packet (shape out of range)
            return false;
        }
        if(!rotationInRange(pieces[i][1])){
            printf("Rotation out of range\n");
            send(player_fd, "E 301", sizeof("E 301"), 0); // 301: Invalid Initialize packet (rotation out of range)
            return false;
        }

        if(pieces[i][0] == 1 && !checkShape1(pieces[i][1], pieces[i][2], pieces[i][3], *width, *height)){
            printf("Ship doesnt fit in the board\n");
            send(player_fd, "E 302", sizeof("E 302"), 0); // 302: Invalid Initialize packet (ship doesnt fit in the board)
            return false;
        }
        else if(pieces[i][0] == 2 && !checkShape2(pieces[i][1], pieces[i][2], pieces[i][3], *width, *height)){
            printf("Ship doesnt fit in the board\n");
            send(player_fd, "E 302", sizeof("E 302"), 0);
            return false;
        }
        else if(pieces[i][0] == 3 && !checkShape3(pieces[i][1], pieces[i][2], pieces[i][3], *width, *height)){
            printf("Ship doesnt fit in the board\n");
            send(player_fd, "E 302", sizeof("E 302"), 0);
            return false;
        }
        else if(pieces[i][0] == 4 && !checkShape4(pieces[i][1], pieces[i][2], pieces[i][3], *width, *height)){
            printf("Ship doesnt fit in the board\n");
            send(player_fd, "E 302", sizeof("E 302"), 0);
            return false;
        }
        else if(pieces[i][0] == 5 && !checkShape5(pieces[i][1], pieces[i][2], pieces[i][3], *width, *height)){
            printf("Ship doesnt fit in the board\n");
            send(player_fd, "E 302", sizeof("E 302"), 0);
            return false;
        }
        else if(pieces[i][0] == 6 && !checkShape6(pieces[i][1], pieces[i][2], pieces[i][3], *width, *height)){
            printf("Ship doesnt fit in the board\n");
            send(player_fd, "E 302", sizeof("E 302"), 0);
            return false;
        }
        else if(pieces[i][0] == 7 && !checkShape7(pieces[i][1], pieces[i][2], pieces[i][3], *width, *height)){
            printf("Ship doesnt fit in the board\n");
            send(player_fd, "E 302", sizeof("E 302"), 0);
            return false;
        }
    }
    //at this point, all the ships should be valid inputs
    //Check if the ships overlap
    printf("does it check overlap\n");
    if(!checkOverlap(player, pieces, width, height)){   //if the ships overlap, reset the board
        resetBoard(player, width, height);
        send(player_fd, "E 303", sizeof("E 303"), 0);
        return false;
    }

    printf("sending Ack, end of the initialize function\n");
    send(player_fd, "A", sizeof("A"), 0); // finally, acknowledge yay
    return true;
}

// Checking Piece Shapes if they're valid
bool shapeInRange(int shape){
    if(shape > 0 && shape < 8){
        return true;
    }
    return false;
}
bool rotationInRange(int rotation){
    if(rotation > 0 && rotation < 5){
        return true;
    }
    return false;
}

// Shape 1: "O" - Square
bool checkShape1(int rotation, int row, int col,int width,int height){
    if(row < 0 || col < 0 || row >= height || col >= width) return false;
    if(row > (height - 2) || col > (width - 2)) return false;
    return true;
}

// Shape 2: "I" - Straight line
bool checkShape2(int rotation, int row, int col, int width, int height){
    if(row < 0 || col < 0 || row >= height || col >= width) return false;

    if(rotation == 1 || rotation == 3){ // Vertical
        if(row > (height - 4) || col >= width) return false;
    }else if(rotation == 2 || rotation == 4){ // Horizontal
        if(col >( width - 4) || row >= height)return false;
    }
    return true;
}

// Shape 3: "S"
bool checkShape3(int rotation, int row, int col,int width,int height){
    if(row < 0 || col < 0 || row >= height || col >= width) return false;

    if(rotation == 1 || rotation == 3){// Horizontal
        if(row <= 0 || col > (width - 3)) return false;
    }else if(rotation == 2 || rotation == 4){// Vertical
        if(row > (height - 3) || col > (width - 2)) return false;
    }
    return true;
}

// Shape 4: "L"
bool checkShape4(int rotation, int row, int col,int width,int height){
    if(row < 0 || col < 0 || row >= height || col >= width) return false;

    if(rotation == 1){// L normal
        if(row > (height - 3) || col > (width - 2)) return false;
    }else if(rotation == 2){// L pointing left
        if(row > (height - 2) || col >( width - 3)) return false;
    }else if(rotation == 3){// L pointing down
        if(row > (height - 3) || col > (width - 2)) return false;
    }else if(rotation == 4){// L pointing right
        if(row <= 0 || col > (width - 3)) return false;
    }
    return true;
}

// Shape 5: "Z"
bool checkShape5(int rotation, int row,int col,int width,int height){
    if(row < 0 || col < 0 || row >= height || col >= width) return false;

    if(rotation == 1 || rotation == 3){// Horizontal
        if(row > (height - 2) || col > (width - 3)) return false;
    }else if(rotation == 2 || rotation == 4){// Vertical
        if(row > (height - 2) || col > (width - 2)) return false;
    }
    return true;
}

// Shape 6: "J" -- inverted L
bool checkShape6(int rotation, int row, int col, int width, int height){
    if(row < 0 || col < 0 || row >= height || col >= width) return false;

    if(rotation == 1){// J normal
        if(row < 2 || col > (width - 2)) return false;
    }else if(rotation == 2){// J pointing right
        if(row > (height - 2) || col > (width - 3)) return false;
    }else if(rotation == 3){// J pointing up
        if(row > (height - 3) || col > width - 2) return false;
    }else if(rotation == 4){// J pointing left
        if(row > (height - 2) || col > (width - 3)) return false;
    }
    return true;
}

// Shape 7: "T"
bool checkShape7(int rotation,int row,int col,int width,int height){
    if(row < 0 || col < 0 || row >= height || col >= width) return false;

    if(rotation == 1){// T pointing down
        if(row > (height - 2) || col > (width - 3)) return false;
    }else if(rotation == 2){// T pointing left
        if(row > (height - 2) || col > (width - 2)) return false;
    }else if(rotation == 3){// T pointing up (middle finger)
        if(row <= 0 || col > (width - 3)) return false;
    }else if(rotation == 4){// T pointing right
        if(row > (height - 3) || col > (width - 2))return false;
    }
    return true;
}

//reset the board (called when ships overlap)
void resetBoard(int player, int *width, int *height){
    if(player == 1){
        printf("this is the board1 before reset\n");
        for(int i = 0; i < *height; i++){
            for(int j = 0; j < *width; j++){
                printf("%d ", p1_board[i][j]);
            }
            printf("\n");
        }
        //reset
        for(int i = 0; i < *height; i++){
            for(int j = 0; j < *width; j++){
                p1_board[i][j] = 0;
            }
        }
        printf("this is the board1 after reset\n");
        for(int i = 0; i < *height; i++){
            for(int j = 0; j < *width; j++){
                printf("%d ", p1_board[i][j]);
            }
            printf("\n");
        }
    }
    else if(player == 2){
        printf("this is the board2 before reset\n");
        for(int i = 0; i < *height; i++){
            for(int j = 0; j < *width; j++){
                printf("%d ", p2_board[i][j]);
            }
            printf("\n");
        }
        for(int i = 0; i < *height; i++){
            for(int j = 0; j < *width; j++){
                p2_board[i][j] = 0;
            }
        }
        printf("this is the board2 after reset\n");
        for(int i = 0; i < *height; i++){
            for(int j = 0; j < *width; j++){
                printf("%d ", p2_board[i][j]);
            }
            printf("\n");
        }
    }

}

//check if the ships overlap
bool checkOverlap(int player, int pieces[5][4], int *width, int *height){
    if(player == 1){
        for(int i = 0; i < 5; i++){
            int rotation = pieces[i][1];
            int row = pieces[i][2];
            int col = pieces[i][3];
            if(pieces[i][0] == 1){
                if(p1_board[row][col] == 1 || p1_board[row][col + 1] == 1 || p1_board[row + 1][col] == 1 || p1_board[row + 1][col + 1] == 1){
                    return false;
                }
                else{
                    p1_board[row][col] = 1;
                    p1_board[row][col + 1] = 1;
                    p1_board[row + 1][col] = 1;
                    p1_board[row + 1][col + 1] = 1;
                }
            }
            else if(pieces[i][0] == 2){
                if(rotation == 1 || rotation == 3){
                    if(p1_board[row][col] == 1 || p1_board[row + 1][col] == 1 || p1_board[row + 2][col] == 1 || p1_board[row + 3][col] == 1){
                        return false;
                    }
                    else{
                        p1_board[row][col] = 1;
                        p1_board[row + 1][col] = 1;
                        p1_board[row + 2][col] = 1;
                        p1_board[row + 3][col] = 1;
                    }
                }else{// rotation == 2 || rotation == 4
                    if(p1_board[row][col] == 1 || p1_board[row][col + 1] == 1 || p1_board[row][col + 2] == 1 || p1_board[row][col + 3] == 1){
                        return false;
                    }
                    else{
                        p1_board[row][col] = 1;
                        p1_board[row][col + 1] = 1;
                        p1_board[row][col + 2] = 1;
                        p1_board[row][col + 3] = 1;
                    }
                }
            }
            else if(pieces[i][0] == 3){
                if(rotation == 1 || rotation == 3){
                    if(p1_board[row][col] == 1 || p1_board[row][col + 1] == 1 || p1_board[row - 1][col + 1] == 1 || p1_board[row - 1][col + 2] == 1){
                        return false;
                    }
                    else{
                        p1_board[row][col] = 1;
                        p1_board[row][col + 1] = 1;
                        p1_board[row - 1][col + 1] = 1;
                        p1_board[row - 1][col + 2] = 1;
                    }
                }else{// rotation == 2 || rotation == 4
                    if(p1_board[row][col] == 1 || p1_board[row + 1][col] == 1 || p1_board[row + 1][col + 1] == 1 || p1_board[row + 2][col + 1] == 1){
                        return false;
                    }
                    else{
                        p1_board[row][col] = 1;
                        p1_board[row + 1][col] = 1;
                        p1_board[row + 1][col + 1] = 1;
                        p1_board[row + 2][col + 1] = 1;
                    }
                }
            }
            else if(pieces[i][0] == 4){
                if(rotation == 1){
                    if(p1_board[row][col] == 1 || p1_board[row + 1][col] == 1 || p1_board[row + 2][col] == 1 || p1_board[row + 2][col + 1] == 1){
                        return false;
                    }
                    else{
                        p1_board[row][col] = 1;
                        p1_board[row + 1][col] = 1;
                        p1_board[row + 2][col] = 1;
                        p1_board[row + 2][col + 1] = 1;
                    }
                }
                else if(rotation == 2){
                    if(p1_board[row][col] == 1 || p1_board[row + 1][col] == 1 || p1_board[row][col + 1] == 1 || p1_board[row][col + 2] == 1){
                        return false;
                    }
                    else{
                        p1_board[row][col] = 1;
                        p1_board[row + 1][col] = 1;
                        p1_board[row][col + 1] = 1; 
                        p1_board[row][col + 2] = 1;
                    }
                }
                else if(rotation == 3){
                    if(p1_board[row][col] == 1 || p1_board[row][col + 1] == 1 || p1_board[row + 1][col + 1] == 1 || p1_board[row + 2][col + 1] == 1){
                        return false;
                    }
                    else{
                        p1_board[row][col] = 1;
                        p1_board[row][col + 1] = 1;
                        p1_board[row + 1][col + 1] = 1;
                        p1_board[row + 2][col + 1] = 1;
                    }
                }
                else{// rotation == 4
                    if(p1_board[row][col] == 1 || p1_board[row][col + 1] == 1 || p1_board[row][col + 2] == 1 || p1_board[row - 1][col + 2] == 1){
                        return false;
                    }
                    else{
                        p1_board[row][col] = 1;
                        p1_board[row][col + 1] = 1;
                        p1_board[row][col + 2] = 1;
                        p1_board[row - 1][col + 2] = 1;
                    }
                }
            }
            else if(pieces[i][0] == 5){
                if(rotation == 1 || rotation == 3){
                    if(p1_board[row][col] == 1 || p1_board[row][col + 1] == 1 || p1_board[row + 1][col + 1] == 1 || p1_board[row + 1][col + 2] == 1){
                        return false;
                    }
                    else{
                        p1_board[row][col] = 1;
                        p1_board[row][col + 1] = 1;
                        p1_board[row + 1][col + 1] = 1;
                        p1_board[row + 1][col + 2] = 1;
                    }
                }else{// rotation == 2 || rotation == 4
                    if(p1_board[row][col] == 1 || p1_board[row + 1][col] == 1 || p1_board[row][col + 1] == 1 || p1_board[row - 1][col + 1] == 1){
                        return false;
                    }
                    else{
                        p1_board[row][col] = 1;
                        p1_board[row + 1][col] = 1;
                        p1_board[row][col + 1] = 1;
                        p1_board[row - 1][col + 1] = 1;
                    }
                }
            }
            else if(pieces[i][0] == 6){
                if(rotation == 1){
                    if(p1_board[row][col] == 1 || p1_board[row][col + 1] == 1 || p1_board[row - 1][col + 1] == 1 || p1_board[row - 2][col + 1] == 1){
                        return false;
                    }
                    else{
                        p1_board[row][col] = 1;
                        p1_board[row][col + 1] = 1;
                        p1_board[row - 1][col + 1] = 1;
                        p1_board[row - 2][col + 1] = 1;
                    }
                }
                else if(rotation == 2){
                    if(p1_board[row][col] == 1 || p1_board[row + 1][col] == 1 || p1_board[row + 1][col + 1] == 1 || p1_board[row + 1][col + 2] == 1){
                        return false;
                    }
                    else{
                        p1_board[row][col] = 1;
                        p1_board[row + 1][col] = 1;
                        p1_board[row + 1][col + 1] = 1;
                        p1_board[row + 1][col + 2] = 1;
                    }
                }
                else if(rotation == 3){
                    if(p1_board[row][col] == 1 || p1_board[row][col + 1] == 1 || p1_board[row + 1][col] == 1 || p1_board[row + 2][col] == 1){
                        return false;
                    }
                    else{
                        p1_board[row][col] = 1;
                        p1_board[row][col + 1] = 1;
                        p1_board[row + 1][col] = 1;
                        p1_board[row + 2][col] = 1;
                    }
                }
                else{// rotation == 4
                    if(p1_board[row][col] == 1 || p1_board[row][col + 1] == 1 || p1_board[row][col + 2] == 1 || p1_board[row + 1][col + 2] == 1){
                        return false;
                    }
                    else{
                        p1_board[row][col] = 1;
                        p1_board[row][col + 1] = 1;
                        p1_board[row][col + 2] = 1;
                        p1_board[row + 1][col + 2] = 1;
                    }
                }
            }
            else if(pieces[i][0] == 7){
                if(rotation == 1){
                    if(p1_board[row][col] == 1 || p1_board[row][col + 1] == 1 || p1_board[row + 1][col + 1] == 1 || p1_board[row][col + 2] == 1){
                        return false;
                    }
                    else{
                        p1_board[row][col] = 1;
                        p1_board[row][col + 1] = 1;
                        p1_board[row + 1][col + 1] = 1;
                        p1_board[row][col + 2] = 1;
                    }
                }
                else if(rotation == 2){
                    if(p1_board[row][col] == 1 || p1_board[row][col + 1] == 1 || p1_board[row - 1][col + 1] == 1 || p1_board[row + 1][col + 1] == 1){
                        return false;
                    }
                    else{
                        p1_board[row][col] = 1;
                        p1_board[row][col + 1] = 1;
                        p1_board[row - 1][col + 1] = 1;
                        p1_board[row + 1][col + 1] = 1;
                    }
                }
                else if(rotation == 3){
                    if(p1_board[row][col] == 1 || p1_board[row][col + 1] == 1 || p1_board[row - 1][col + 1] == 1 || p1_board[row][col + 2] == 1){
                        return false;
                    }
                    else{
                        p1_board[row][col] = 1;
                        p1_board[row][col + 1] = 1;
                        p1_board[row - 1][col + 1] = 1;
                        p1_board[row][col + 2] = 1;
                    }
                }
                else{// rotation == 4
                    if(p1_board[row][col] == 1 || p1_board[row + 1][col] == 1 || p1_board[row + 1][col + 1] == 1 || p1_board[row + 2][col] == 1){
                        return false;
                    }
                    else{
                        p1_board[row][col] = 1;
                        p1_board[row + 1][col] = 1;
                        p1_board[row + 1][col + 1] = 1;
                        p1_board[row + 2][col] = 1;
                    }
                }
            }
        
        }
    }
    else if(player == 2){
        for(int i = 0; i < 5; i++){
            int rotation = pieces[i][1];
            int row = pieces[i][2];
            int col = pieces[i][3];
            if(pieces[i][0] == 1){
                if(p2_board[row][col] == 1 || p2_board[row][col + 1] == 1 || p2_board[row + 1][col] == 1 || p2_board[row + 1][col + 1] == 1){
                    return false;
                }
                else{
                    p2_board[row][col] = 1;
                    p2_board[row][col + 1] = 1;
                    p2_board[row + 1][col] = 1;
                    p2_board[row + 1][col + 1] = 1;
                }
            }
            else if(pieces[i][0] == 2){
                if(rotation == 1 || rotation == 3){
                    if(p2_board[row][col] == 1 || p2_board[row + 1][col] == 1 || p2_board[row + 2][col] == 1 || p2_board[row + 3][col] == 1){
                        return false;
                    }
                    else{
                        p2_board[row][col] = 1;
                        p2_board[row + 1][col] = 1;
                        p2_board[row + 2][col] = 1;
                        p2_board[row + 3][col] = 1;
                    }
                }else{// rotation == 2 || rotation == 4
                    if(p2_board[row][col] == 1 || p2_board[row][col + 1] == 1 || p2_board[row][col + 2] == 1 || p2_board[row][col + 3] == 1){
                        return false;
                    }
                    else{
                        p2_board[row][col] = 1;
                        p2_board[row][col + 1] = 1;
                        p2_board[row][col + 2] = 1;
                        p2_board[row][col + 3] = 1;
                    }
                }
            }
            else if(pieces[i][0] == 3){
                if(rotation == 1 || rotation == 3){
                    if(p2_board[row][col] == 1 || p2_board[row][col + 1] == 1 || p2_board[row - 1][col + 1] == 1 || p2_board[row - 1][col + 2] == 1){
                        return false;
                    }
                    else{
                        p2_board[row][col] = 1;
                        p2_board[row][col + 1] = 1;
                        p2_board[row - 1][col + 1] = 1;
                        p2_board[row - 1][col + 2] = 1;
                    }
                }else{// rotation == 2 || rotation == 4
                    if(p2_board[row][col] == 1 || p2_board[row + 1][col] == 1 || p2_board[row + 1][col + 1] == 1 || p2_board[row + 2][col + 1] == 1){
                        return false;
                    }
                    else{
                        p2_board[row][col] = 1;
                        p2_board[row + 1][col] = 1;
                        p2_board[row + 1][col + 1] = 1;
                        p2_board[row + 2][col + 1] = 1;
                    }
                }
            }
            else if(pieces[i][0] == 4){
                if(rotation == 1){
                    if(p2_board[row][col] == 1 || p2_board[row + 1][col] == 1 || p2_board[row + 2][col] == 1 || p2_board[row + 2][col + 1] == 1){
                        return false;
                    }
                    else{
                        p2_board[row][col] = 1;
                        p2_board[row + 1][col] = 1;
                        p2_board[row + 2][col] = 1;
                        p2_board[row + 2][col + 1] = 1;
                    }
                }
                else if(rotation == 2){
                    if(p2_board[row][col] == 1 || p2_board[row + 1][col] == 1 || p2_board[row][col + 1] == 1 || p2_board[row][col + 2] == 1){
                        return false;
                    }
                    else{
                        p2_board[row][col] = 1;
                        p2_board[row + 1][col] = 1;
                        p2_board[row][col + 1] = 1;
                        p2_board[row][col + 2] = 1;
                    }
                }
                else if(rotation == 3){
                    if(p2_board[row][col] == 1 || p2_board[row][col + 1] == 1 || p2_board[row + 1][col + 1] == 1 || p2_board[row + 2][col + 1] == 1){
                        return false;
                    }
                    else{
                        p2_board[row][col] = 1;
                        p2_board[row][col + 1] = 1;
                        p2_board[row + 1][col + 1] = 1;
                        p2_board[row + 2][col + 1] = 1;
                    }
                }
                else{// rotation == 4
                    if(p2_board[row][col] == 1 || p2_board[row][col + 1] == 1 || p2_board[row][col + 2] == 1 || p2_board[row - 1][col + 2] == 1){
                        return false;
                    }
                    else{
                        p2_board[row][col] = 1;
                        p2_board[row][col + 1] = 1;
                        p2_board[row][col + 2] = 1;
                        p2_board[row - 1][col + 2] = 1;
                    }
                }
            }
            else if(pieces[i][0] == 5){
                if(rotation == 1 || rotation == 3){
                    if(p2_board[row][col] == 1 || p2_board[row][col + 1] == 1 || p2_board[row + 1][col + 1] == 1 || p2_board[row + 1][col + 2] == 1){
                        return false;
                    }
                    else{
                        p2_board[row][col] = 1;
                        p2_board[row][col + 1] = 1;
                        p2_board[row + 1][col + 1] = 1;
                        p2_board[row + 1][col + 2] = 1;
                    }
                }else{// rotation == 2 || rotation == 4
                    if(p2_board[row][col] == 1 || p2_board[row + 1][col] == 1 || p2_board[row][col + 1] == 1 || p2_board[row - 1][col + 1] == 1){
                        return false;
                    }
                    else{
                        p2_board[row][col] = 1;
                        p2_board[row + 1][col] = 1;
                        p2_board[row][col + 1] = 1;
                        p2_board[row - 1][col + 1] = 1;
                    }
                }
            }
            else if(pieces[i][0] == 6){
                if(rotation == 1){
                    if(p2_board[row][col] == 1 || p2_board[row][col + 1] == 1 || p2_board[row - 1][col + 1] == 1 || p2_board[row - 2][col + 1] == 1){
                        return false;
                    }
                    else{
                        p2_board[row][col] = 1;
                        p2_board[row][col + 1] = 1;
                        p2_board[row - 1][col + 1] = 1;
                        p2_board[row - 2][col + 1] = 1;
                    }
                }
                else if(rotation == 2){
                    if(p2_board[row][col] == 1 || p2_board[row + 1][col] == 1 || p2_board[row + 1][col + 1] == 1 || p2_board[row + 1][col + 2] == 1){
                        return false;
                    }
                    else{
                        p2_board[row][col] = 1;
                        p2_board[row + 1][col] = 1;
                        p2_board[row + 1][col + 1] = 1;
                        p2_board[row + 1][col + 2] = 1;
                    }
                }
                else if(rotation == 3){
                    if(p2_board[row][col] == 1 || p2_board[row][col + 1] == 1 || p2_board[row + 1][col] == 1 || p2_board[row + 2][col] == 1){
                        return false;
                    }
                    else{
                        p2_board[row][col] = 1;
                        p2_board[row][col + 1] = 1;
                        p2_board[row + 1][col] = 1;
                        p2_board[row + 2][col] = 1;
                    }
                }
                else{// rotation == 4
                    if(p2_board[row][col] == 1 || p2_board[row][col + 1] == 1 || p2_board[row][col + 2] == 1 || p2_board[row + 1][col + 2] == 1){
                        return false;
                    }
                    else{
                        p2_board[row][col] = 1;
                        p2_board[row][col + 1] = 1;
                        p2_board[row][col + 2] = 1;
                        p2_board[row + 1][col + 2] = 1;
                    }
                }
            }
            else if(pieces[i][0] == 7){
                if(rotation == 1){
                    if(p2_board[row][col] == 1 || p2_board[row][col + 1] == 1 || p2_board[row + 1][col + 1] == 1 || p2_board[row][col + 2] == 1){
                        return false;
                    }
                    else{
                        p2_board[row][col] = 1;
                        p2_board[row][col + 1] = 1;
                        p2_board[row + 1][col + 1] = 1;
                        p2_board[row][col + 2] = 1;
                    }
                }
                else if(rotation == 2){
                    if(p2_board[row][col] == 1 || p2_board[row][col + 1] == 1 || p2_board[row - 1][col + 1] == 1 || p2_board[row + 1][col + 1] == 1){
                        return false;
                    }
                    else{
                        p2_board[row][col] = 1;
                        p2_board[row][col + 1] = 1;
                        p2_board[row - 1][col + 1] = 1;
                        p2_board[row + 1][col + 1] = 1;
                    }
                }
                else if(rotation == 3){
                    if(p2_board[row][col] == 1 || p2_board[row][col + 1] == 1 || p2_board[row - 1][col + 1] == 1 || p2_board[row][col + 2] == 1){
                        return false;
                    }
                    else{
                        p2_board[row][col] = 1;
                        p2_board[row][col + 1] = 1;
                        p2_board[row - 1][col + 1] = 1;
                        p2_board[row][col + 2] = 1;
                    }
                }
                else{// rotation == 4
                    if(p2_board[row][col] == 1 || p2_board[row + 1][col] == 1 || p2_board[row + 1][col + 1] == 1 || p2_board[row + 2][col] == 1){
                        return false;
                    }
                    else{
                        p2_board[row][col] = 1;
                        p2_board[row + 1][col] = 1;
                        p2_board[row + 1][col + 1] = 1;
                        p2_board[row + 2][col] = 1;
                    }
                }
            }
        
        }
    }
    printf("returning true from end of checkOverlap\n");
    return true;
}

//-------------------------------------------SHOOT/QUERY/FORFEIT------------------------------------------->


bool shoot_query_forfeit(int shooter_fd, int opponent_fd, int player, int *width, int *height){
    char buffer[BUFFER_SIZE] = {0};
    int bytes_read = read(shooter_fd, buffer, BUFFER_SIZE);
    if(bytes_read <= 0){
        printf("Failed to read from Player %d. Connection closed.\n", player);
        send(shooter_fd, "E 102", sizeof("E 102"), 0); //102: Invalid packet type (Expected Shoot/Query/Forfeit)
        return false;
    }
    if(buffer[0] != 'S' && buffer[0] != 'Q' && buffer[0] != 'F'){
        printf("102: Invalid packet type (Expected Shoot/Query/Forfeit)\n");
        send(shooter_fd, "E 102", sizeof("E 102"), 0);
        return false;
    }
    //forfeit
    if(buffer[0] == 'F'){
        printf("Player %d forfeited the game.\n", player);
        send(shooter_fd, "H 0", strlen("H 0"), 0);
        send(opponent_fd, "H 1", strlen("H 1"), 0);
        exit(EXIT_FAILURE);
        return false;
    }

    //shoot
    if(buffer[0] == 'S'){
        int extra;
        int row, col;
        if(sscanf(buffer, "S %d %d %d", &row, &col, &extra) != 2){
            printf("Invalid Number of parameters in Shoot packet from Player %d.\n", player);
            send(shooter_fd, "E 202", sizeof("E 202"), 0);
            return false;
        }
        if(row < 0 || row >= *height || col < 0 || col >= *width){
            printf("Cell out of BOunds\n");
            send(shooter_fd, "E 400", sizeof("E 400"), 0);
            return false;
        }

        if(player == 1){
            if(p2_board[row][col] == -1 || p2_board[row][col] == -2){
                printf("Cell already guessed\n");
                send(shooter_fd, "E 401", sizeof("E 401"), 0);
                return false;
            }
        }
        else if(player == 2){
            if(p1_board[row][col] == -1 || p1_board[row][col] == -2){
                printf("Cell already guessed\n");
                send(shooter_fd, "E 401", sizeof("E 401"), 0);
                return false;
            }
        }

        //now implement hit or miss     // -1 for hit and -2 for miss
        if(player == 1){
            if(p2_board[row][col] == 1){ // hit
                p2_board[row][col] = -1; // mark hit
                int ships_remaining = count_ships(p2_board, *width, *height);
                char res[BUFFER_SIZE];
                snprintf(res, sizeof(res), "R %d H", ships_remaining);
                send(shooter_fd, res, strlen(res), 0);
                if(ships_remaining == 0){
                    // Game over
                    send(shooter_fd, "R 0 H", strlen("R 0 H"), 0);
                    send(opponent_fd, "HALT", strlen("HALT"), 0);
                    send(shooter_fd, "HALT", strlen("HALT"), 0);
                    return true;
                }
            }else{ // miss
                p2_board[row][col] = -2; // mark miss
                int ships_remaining = count_ships(p2_board, *width, *height);
                char res[BUFFER_SIZE];
                snprintf(res, sizeof(res), "R %d M", ships_remaining);
                send(shooter_fd, res, strlen(res), 0); // send response
            }
        }
        else if(player == 2){
            if(p1_board[row][col] == 1){ // hit
                p1_board[row][col] = -1; // mark hit
                int ships_remaining = count_ships(p1_board, *width, *height);
                char res[BUFFER_SIZE];
                snprintf(res, sizeof(res), "R %d H", ships_remaining);
                send(shooter_fd, res, strlen(res), 0);
                if(ships_remaining == 0){
                    // Game over
                    send(shooter_fd, "R 0 H", strlen("R 0 H"), 0);
                    send(opponent_fd, "HALT", strlen("HALT"), 0);
                    send(shooter_fd, "HALT", strlen("HALT"), 0);
                    return true;
                }
            }else{ // miss
                p1_board[row][col] = -2; // mark miss
                int ships_remaining = count_ships(p1_board, *width, *height);
                char res[BUFFER_SIZE];
                snprintf(res, sizeof(res), "R %d M", ships_remaining);
                send(shooter_fd, res, strlen(res), 0); // send response
            }
        }
        return false;
    }
    // Query
    if(buffer[0] == 'Q'){
        char res[BUFFER_SIZE] = "G ";
        int **board = (player == 1) ? p1_board : p2_board;
        int ships_remaining = count_ships(board, *width, *height);

        // Append Ships Remaining
        char temp[32];
        snprintf(temp, sizeof(temp), "%d ", ships_remaining);
        strcat(res, temp);

        // Append Hits and Misses
        for (int i = 0; i < *height; i++) {
            for (int j = 0; j < *width; j++) {
                if (board[i][j] == -1) {
                    snprintf(temp, sizeof(temp), "H %d %d ", i, j);
                    if (strlen(res) + strlen(temp) < BUFFER_SIZE)
                        strcat(res, temp);
                } else if (board[i][j] == -2) {
                    snprintf(temp, sizeof(temp), "M %d %d ", i, j);
                    if (strlen(res) + strlen(temp) < BUFFER_SIZE)
                        strcat(res, temp);
                }
            }
        }
        send(shooter_fd, res, strlen(res), 0);
        return false; // Continue the game
    }
    return false;
}

int count_ships(int **board, int width, int height){
    int count = 0;
    for(int i = 0; i < height; i++){
        for(int j = 0; j < width; j++){
            if(board[i][j] == 1){
                count++;
            }
        }
    }
    return count;
}
