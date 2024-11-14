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
int set_up_server(int PORT);
void begin(int player_fd, int player, int *width, int *height);
void create_board(int *width, int *height);
void initialize(int player_fd, int player, char* pieces_info, int *width, int *height);
void shoot(int player_fd, int player, int *width, int *height);

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


int main(){
    printf("Welcome to the game of Battleship!\n");
    printf("main blablabla");
    printf("blablabla");
    int player1_fd = set_up_server(2201); // Port for player 1
    int player2_fd = set_up_server(2202); // Port for player 2
    
    //begin
    int width, height;
    width = height = 20;
    printf("Beginning game...\n");
    begin(player1_fd, 1, &width, &height);
    begin(player2_fd, 2, NULL, NULL);

    //creates board for both players 1 and 2
    create_board(&width, &height);
    
    char* pieces_info1 = "I 1 1 0 0 1 1 0 2 1 1 0 4 1 1 2 2 1 1 2 0";
    char* pieces_info2 = "I 1 1 0 0 1 1 0 2 1 1 0 4 1 1 2 2 1 1 2 0";
    initialize(player1_fd, 1, pieces_info1, &width, &height);
    initialize(player2_fd, 2, pieces_info2, &width, &height);

}

//Server Setup --------------------------------------------------------------------------->
int set_up_server(int PORT){
    int listen_fd, conn_fd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    // char buffer[BUFFER_SIZE] = {0};

    // Create socket
    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        perror("setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt))");
        exit(EXIT_FAILURE);
    }

    // Bind socket to port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    if (bind(listen_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("[Server] bind() failed.");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(listen_fd, 3) < 0)
    {
        perror("[Server] listen() failed.");
        exit(EXIT_FAILURE);
    }

    printf("[Server] Running on port %d\n", PORT);

    // Accept incoming connection
    if ((conn_fd = accept(listen_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
    {
        perror("[Server] accept() failed.");
        exit(EXIT_FAILURE);
    }

    printf("[Server] Connection accepted on port %d\n", PORT);
    return conn_fd;
}
// End Server Setup --------------------------------------------------------------------------

//------------------------------------BEGIN------------------------------------------------------>

void begin(int player_fd, int player, int *width, int *height){
    char buffer[BUFFER_SIZE] = {0};
    int bytes_read = read(player_fd, buffer, BUFFER_SIZE);

    printf("buffer index 0: %d\n", buffer[0]);
    if(bytes_read <= 0){
        printf("Failed to read from Player %d. Connection closed.\n", player);
        exit(EXIT_FAILURE);
    }

    if(player == 1){
        if(buffer[0] == 'B' && sscanf(buffer, "B %d %d", width, height) == 2){
            send(player_fd, "A", sizeof("A"), 0);
        }
        else{
            send(player_fd, "E 200", sizeof("E 200"), 0);
            exit(EXIT_FAILURE);
        }
    }
    else if(player == 2){
        if(buffer[0] == 'B'){
            send(player_fd, "B", sizeof("B"), 0);

        }
        else{
            send(player_fd, "E 200", sizeof("E 200"), 0);
            exit(EXIT_FAILURE);
        }
    }
    else{
        printf("Invalid Player: %d\n", player);
        exit(EXIT_FAILURE);
    }
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

void initialize(int player_fd, int player, char* piece_info, int *width, int *height){
    char buffer[BUFFER_SIZE] = {0};
    int bytes_read = read(player_fd, buffer, BUFFER_SIZE);

    if(bytes_read <= 0){
        printf("Failed to read from Player. Connection closed.\n");
        exit(EXIT_FAILURE);
    }
    
    if(buffer[0] != 'I'){
        printf("101: Invalid packet type (Expected Initialize packet)");
        send(player_fd, "E 101", sizeof("E 101"), 0);
        exit(EXIT_FAILURE);
    }
    int pieces[5][4] = {0};
    char *token = strtok(buffer + 1, " "); //first char skipping I
    for(int i = 0; i < 5; i++){
        for(int j = 0; j < 4; j++){
            if(token == NULL){
                send(player_fd, "E 201", sizeof("E 201"), 0);
                exit(EXIT_FAILURE);
            }

            if(*token >= '0' && *token <= '9'){
                int num = atoi(token); //make token an int
                pieces[i][j] = num;
                token = strtok(NULL, " ");
            }
            else{
                send(player_fd, "E 201", sizeof("E 201"), 0); //other ascii chars were read
                exit(EXIT_FAILURE);
            }

        }
    }
    // each piece should contain information now about its own position
    // printing for debugging purposes
    for (int i = 0; i < 5; i++) {
        printf("Piece %d: %d %d %d %d\n", i + 1, pieces[i][0], pieces[i][1], pieces[i][2], pieces[i][3]);
    }

    //Check placement of pieces
    for(int i = 0; i < 5; i++){
        if(!shapeInRange(pieces[i][0])){
            printf("Shape Type out of range\n");
            send(player_fd, "E 300", sizeof("E 300"), 0); // 300: Invalid Initialize packet (shape out of range)
            exit(EXIT_FAILURE);
        }
        if(!rotationInRange(pieces[i][1])){
            printf("Rotation out of range\n");
            send(player_fd, "E 301", sizeof("E 301"), 0); // 301: Invalid Initialize packet (rotation out of range)
            exit(EXIT_FAILURE);
        }

        if(pieces[i][0] == 1 && !checkShape1(pieces[i][1], pieces[i][2], pieces[i][3], *width, *height)){
            printf("Ship doesnt fit in the board\n");
            send(player_fd, "E 302", sizeof("E 302"), 0); // 302: Invalid Initialize packet (ship doesnt fit in the board)
            exit(EXIT_FAILURE);
        }
        else if(pieces[i][0] == 2 && !checkShape2(pieces[i][1], pieces[i][2], pieces[i][3], *width, *height)){
            printf("Ship doesnt fit in the board\n");
            send(player_fd, "E 302", sizeof("E 302"), 0);
            exit(EXIT_FAILURE);
        }
        else if(pieces[i][0] == 3 && !checkShape3(pieces[i][1], pieces[i][2], pieces[i][3], *width, *height)){
            printf("Ship doesnt fit in the board\n");
            send(player_fd, "E 302", sizeof("E 302"), 0);
            exit(EXIT_FAILURE);
        }
        else if(pieces[i][0] == 4 && !checkShape4(pieces[i][1], pieces[i][2], pieces[i][3], *width, *height)){
            printf("Ship doesnt fit in the board\n");
            send(player_fd, "E 302", sizeof("E 302"), 0);
            exit(EXIT_FAILURE);
        }
        else if(pieces[i][0] == 5 && !checkShape5(pieces[i][1], pieces[i][2], pieces[i][3], *width, *height)){
            printf("Ship doesnt fit in the board\n");
            send(player_fd, "E 302", sizeof("E 302"), 0);
            exit(EXIT_FAILURE);
        }
        else if(pieces[i][0] == 6 && !checkShape6(pieces[i][1], pieces[i][2], pieces[i][3], *width, *height)){
            printf("Ship doesnt fit in the board\n");
            send(player_fd, "E 302", sizeof("E 302"), 0);
            exit(EXIT_FAILURE);
        }
        else if(pieces[i][0] == 7 && !checkShape7(pieces[i][1], pieces[i][2], pieces[i][3], *width, *height)){
            printf("Ship doesnt fit in the board\n");
            send(player_fd, "E 302", sizeof("E 302"), 0);
            exit(EXIT_FAILURE);
        }
    }
    //at this point, all the ships should be valid inputs
    //Check if the ships overlap

    if(!checkOverlap(player, pieces, width, height)){   //if the ships overlap, reset the board
        resetBoard(player, width, height);
        send(player_fd, "E 303", sizeof("E 303"), 0);
        exit(EXIT_FAILURE);
    }

    send(player_fd, "A", sizeof("A"), 0); // finally, acknowledge yay

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
        for(int i = 0; i < *height; i++){
            for(int j = 0; j < *width; j++){
                p1_board[i][j] = 0;
            }
        }
    }
    else if(player == 2){
        for(int i = 0; i < *height; i++){
            for(int j = 0; j < *width; j++){
                p2_board[i][j] = 0;
            }
        }
    }
}

//check if the ships overlap
bool checkOverlap(int player, int pieces[5][4], int *width, int *height){
    if(player == 1){
        for(int i = 0; i < 5; i++){
            for(int j = 0; j < 4; j++){
                int rotation = pieces[i][1];
                int row = pieces[i][2];
                int col = pieces[i][3];
                if(pieces[i][0] == 1){
                    if(p1_board[row][col] == 1 || p1_board[row][col + 1] == 1 || p1_board[row + 1][col] == 1 || p1_board[row + 1][col + 1] == 1){
                        return false;
                    }
                    else{
                        p1_board[row][col] = p1_board[row][col + 1] = p1_board[row - 1][col] = p1_board[row - 1][col + 1] = 1;
                    }
                }
                else if(pieces[i][0] == 2){
                    if(rotation == 1 || rotation == 3){
                        if(p1_board[row][col] == 1 || p1_board[row + 1][col] == 1 || p1_board[row + 2][col] == 1 || p1_board[row + 3][col] == 1){
                            return false;
                        }
                        else{
                            p1_board[row][col] = p1_board[row + 1][col] = p1_board[row + 2][col] = p1_board[row + 3][col] = 1;
                        }
                    }else{// rotation == 2 || rotation == 4
                        if(p1_board[row][col] == 1 || p1_board[row][col + 1] == 1 || p1_board[row][col + 2] == 1 || p1_board[row][col + 3] == 1){
                            return false;
                        }
                        else{
                            p1_board[row][col] = p1_board[row][col + 1] = p1_board[row][col + 2] = p1_board[row][col + 3] = 1;
                        }
                    }
                }
                else if(pieces[i][0] == 3){
                    if(rotation == 1 || rotation == 3){
                        if(p1_board[row][col] == 1 || p1_board[row][col + 1] == 1 || p1_board[row - 1][col + 1] == 1 || p1_board[row - 1][col + 2] == 1){
                            return false;
                        }
                        else{
                            p1_board[row][col] = p1_board[row][col + 1] = p1_board[row - 1][col + 1] = p1_board[row - 1][col + 2] = 1;
                        }
                    }else{// rotation == 2 || rotation == 4
                        if(p1_board[row][col] == 1 || p1_board[row + 1][col] == 1 || p1_board[row + 1][col + 1] == 1 || p1_board[row + 2][col + 1] == 1){
                            return false;
                        }
                        else{
                            p1_board[row][col] = p1_board[row + 1][col] = p1_board[row + 1][col + 1] = p1_board[row + 2][col + 1] = 1;
                        }
                    }
                }
                else if(pieces[i][0] == 4){
                    if(rotation == 1){
                        if(p1_board[row][col] == 1 || p1_board[row + 1][col] == 1 || p1_board[row + 2][col] == 1 || p1_board[row + 2][col + 1] == 1){
                            return false;
                        }
                        else{
                            p1_board[row][col] = p1_board[row + 1][col] = p1_board[row + 2][col] = p1_board[row + 2][col + 1] = 1;
                        }
                    }
                    else if(rotation == 2){
                        if(p1_board[row][col] == 1 || p1_board[row + 1][col] == 1 || p1_board[row][col + 1] == 1 || p1_board[row][col + 2] == 1){
                            return false;
                        }
                        else{
                            p1_board[row][col] = p1_board[row + 1][col] = p1_board[row][col + 1] = p1_board[row][col + 2] = 1;
                        }
                    }
                    else if(rotation == 3){
                        if(p1_board[row][col] == 1 || p1_board[row][col + 1] == 1 || p1_board[row + 1][col + 1] == 1 || p1_board[row + 2][col + 1] == 1){
                            return false;
                        }
                        else{
                            p1_board[row][col] = p1_board[row][col + 1] = p1_board[row + 1][col + 1] = p1_board[row + 2][col + 1] = 1;
                        }
                    }
                    else{// rotation == 4
                        if(p1_board[row][col] == 1 || p1_board[row][col + 1] == 1 || p1_board[row][col + 2] == 1 || p1_board[row - 1][col + 2] == 1){
                            return false;
                        }
                        else{
                            p1_board[row][col] = p1_board[row][col + 1] = p1_board[row][col + 2] = p1_board[row - 1][col + 2] = 1;
                        }
                    }
                }
                else if(pieces[i][0] == 5){
                    if(rotation == 1 || rotation == 3){
                        if(p1_board[row][col] == 1 || p1_board[row][col + 1] == 1 || p1_board[row + 1][col + 1] == 1 || p1_board[row + 1][col + 2] == 1){
                            return false;
                        }
                        else{
                            p1_board[row][col] = p1_board[row][col + 1] = p1_board[row + 1][col + 1] = p1_board[row + 1][col + 2] = 1;
                        }
                    }else{// rotation == 2 || rotation == 4
                        if(p1_board[row][col] == 1 || p1_board[row + 1][col] == 1 || p1_board[row][col + 1] == 1 || p1_board[row - 1][col + 1] == 1){
                            return false;
                        }
                        else{
                            p1_board[row][col] = p1_board[row + 1][col] = p1_board[row][col + 1] = p1_board[row - 1][col + 1] = 1;
                        }
                    }
                }
                else if(pieces[i][0] == 6){
                    if(rotation == 1){
                        if(p1_board[row][col] == 1 || p1_board[row][col + 1] == 1 || p1_board[row - 1][col + 1] == 1 || p1_board[row - 2][col + 1] == 1){
                            return false;
                        }
                        else{
                            p1_board[row][col] = p1_board[row][col + 1] = p1_board[row - 1][col + 1] = p1_board[row - 2][col + 1] = 1;
                        }
                    }
                    else if(rotation == 2){
                        if(p1_board[row][col] == 1 || p1_board[row + 1][col] == 1 || p1_board[row + 1][col + 1] == 1 || p1_board[row + 1][col + 2] == 1){
                            return false;
                        }
                        else{
                            p1_board[row][col] = p1_board[row + 1][col] = p1_board[row + 1][col + 1] = p1_board[row + 1][col + 2] = 1;
                        }
                    }
                    else if(rotation == 3){
                        if(p1_board[row][col] == 1 || p1_board[row][col + 1] == 1 || p1_board[row + 1][col] == 1 || p1_board[row + 2][col] == 1){
                            return false;
                        }
                        else{
                            p1_board[row][col] = p1_board[row][col + 1] = p1_board[row + 1][col] = p1_board[row + 2][col] = 1;
                        }
                    }
                    else{// rotation == 4
                        if(p1_board[row][col] == 1 || p1_board[row][col + 1] == 1 || p1_board[row][col + 2] == 1 || p1_board[row + 1][col + 2] == 1){
                            return false;
                        }
                        else{
                            p1_board[row][col] = p1_board[row][col + 1] = p1_board[row][col + 2] = p1_board[row + 1][col + 2] = 1;
                        }
                    }
                }
                else if(pieces[i][0] == 7){
                    if(rotation == 1){
                        if(p1_board[row][col] == 1 || p1_board[row][col + 1] == 1 || p1_board[row + 1][col + 1] == 1 || p1_board[row][col + 2] == 1){
                            return false;
                        }
                        else{
                            p1_board[row][col] = p1_board[row][col + 1] = p1_board[row + 1][col + 1] = p1_board[row][col + 2] = 1;
                        }
                    }
                    else if(rotation == 2){
                        if(p1_board[row][col] == 1 || p1_board[row][col + 1] == 1 || p1_board[row - 1][col + 1] == 1 || p1_board[row + 1][col + 1] == 1){
                            return false;
                        }
                        else{
                            p1_board[row][col] = p1_board[row][col + 1] = p1_board[row - 1][col + 1] = p1_board[row + 1][col + 1] = 1;
                        }
                    }
                    else if(rotation == 3){
                        if(p1_board[row][col] == 1 || p1_board[row][col + 1] == 1 || p1_board[row - 1][col + 1] == 1 || p1_board[row][col + 2] == 1){
                            return false;
                        }
                        else{
                            p1_board[row][col] = p1_board[row][col + 1] = p1_board[row - 1][col + 1] = p1_board[row][col + 2] = 1;
                        }
                    }
                    else{// rotation == 4
                        if(p1_board[row][col] == 1 || p1_board[row + 1][col] == 1 || p1_board[row + 1][col + 1] == 1 || p1_board[row + 2][col] == 1){
                            return false;
                        }
                        else{
                            p1_board[row][col] = p1_board[row + 1][col] = p1_board[row + 1][col + 1] = p1_board[row + 2][col] = 1;
                        }
                    }
                }
            }
        }
    }
    else if(player == 2){
        for(int i = 0; i < 5; i++){
            for(int j = 0; j < 4; j++){
                int rotation = pieces[i][1];
                int row = pieces[i][2];
                int col = pieces[i][3];
                if(pieces[i][0] == 1){
                    if(p2_board[row][col] == 1 || p2_board[row][col + 1] == 1 || p2_board[row + 1][col] == 1 || p2_board[row + 1][col + 1] == 1){
                        return false;
                    }
                    else{
                        p2_board[row][col] = p2_board[row][col + 1] = p2_board[row + 1][col] = p2_board[row + 1][col + 1] = 1;
                    }
                }
                else if(pieces[i][0] == 2){
                    if(rotation == 1 || rotation == 3){
                        if(p2_board[row][col] == 1 || p2_board[row + 1][col] == 1 || p2_board[row + 2][col] == 1 || p2_board[row + 3][col] == 1){
                            return false;
                        }
                        else{
                            p2_board[row][col] = p2_board[row + 1][col] = p2_board[row + 2][col] = p2_board[row + 3][col] = 1;
                        }
                    }else{// rotation == 2 || rotation == 4
                        if(p2_board[row][col] == 1 || p2_board[row][col + 1] == 1 || p2_board[row][col + 2] == 1 || p2_board[row][col + 3] == 1){
                            return false;
                        }
                        else{
                            p2_board[row][col] = p2_board[row][col + 1] = p2_board[row][col + 2] = p2_board[row][col + 3] = 1;
                        }
                    }
                }
                else if(pieces[i][0] == 3){
                    if(rotation == 1 || rotation == 3){
                        if(p2_board[row][col] == 1 || p2_board[row][col + 1] == 1 || p2_board[row - 1][col + 1] == 1 || p2_board[row - 1][col + 2] == 1){
                            return false;
                        }
                        else{
                            p2_board[row][col] = p2_board[row][col + 1] = p2_board[row - 1][col + 1] = p2_board[row - 1][col + 2] = 1;
                        }
                    }else{// rotation == 2 || rotation == 4
                        if(p2_board[row][col] == 1 || p2_board[row + 1][col] == 1 || p2_board[row + 1][col + 1] == 1 || p2_board[row + 2][col + 1] == 1){
                            return false;
                        }
                        else{
                            p2_board[row][col] = p2_board[row + 1][col] = p2_board[row + 1][col + 1] = p2_board[row + 2][col + 1] = 1;
                        }
                    }
                }
                else if(pieces[i][0] == 4){
                    if(rotation == 1){
                        if(p2_board[row][col] == 1 || p2_board[row + 1][col] == 1 || p2_board[row + 2][col] == 1 || p2_board[row + 2][col + 1] == 1){
                            return false;
                        }
                        else{
                            p2_board[row][col] = p2_board[row + 1][col] = p2_board[row + 2][col] = p2_board[row + 2][col + 1] = 1;
                        }
                    }
                    else if(rotation == 2){
                        if(p2_board[row][col] == 1 || p2_board[row + 1][col] == 1 || p2_board[row][col + 1] == 1 || p2_board[row][col + 2] == 1){
                            return false;
                        }
                        else{
                            p2_board[row][col] = p2_board[row + 1][col] = p2_board[row][col + 1] = p2_board[row][col + 2] = 1;
                        }
                    }
                    else if(rotation == 3){
                        if(p2_board[row][col] == 1 || p2_board[row][col + 1] == 1 || p2_board[row + 1][col + 1] == 1 || p2_board[row + 2][col + 1] == 1){
                            return false;
                        }
                        else{
                            p2_board[row][col] = p2_board[row][col + 1] = p2_board[row + 1][col + 1] = p2_board[row + 2][col + 1] = 1;
                        }
                    }
                    else{// rotation == 4
                        if(p2_board[row][col] == 1 || p2_board[row][col + 1] == 1 || p2_board[row][col + 2] == 1 || p2_board[row - 1][col + 2] == 1){
                            return false;
                        }
                        else{
                            p2_board[row][col] = p2_board[row][col + 1] = p2_board[row][col + 2] = p2_board[row - 1][col + 2] = 1;
                        }
                    }
                }
                else if(pieces[i][0] == 5){
                    if(rotation == 1 || rotation == 3){
                        if(p2_board[row][col] == 1 || p2_board[row][col + 1] == 1 || p2_board[row + 1][col + 1] == 1 || p2_board[row + 1][col + 2] == 1){
                            return false;
                        }
                        else{
                            p2_board[row][col] = p2_board[row][col + 1] = p2_board[row + 1][col + 1] = p2_board[row + 1][col + 2] = 1;
                        }
                    }else{// rotation == 2 || rotation == 4
                        if(p2_board[row][col] == 1 || p2_board[row + 1][col] == 1 || p2_board[row][col + 1] == 1 || p2_board[row - 1][col + 1] == 1){
                            return false;
                        }
                        else{
                            p2_board[row][col] = p2_board[row + 1][col] = p2_board[row][col + 1] = p2_board[row - 1][col + 1] = 1;
                        }
                    }
                }
                else if(pieces[i][0] == 6){
                    if(rotation == 1){
                        if(p2_board[row][col] == 1 || p2_board[row][col + 1] == 1 || p2_board[row - 1][col + 1] == 1 || p2_board[row - 2][col + 1] == 1){
                            return false;
                        }
                        else{
                            p2_board[row][col] = p2_board[row][col + 1] = p2_board[row - 1][col + 1] = p2_board[row - 2][col + 1] = 1;
                        }
                    }
                    else if(rotation == 2){
                        if(p2_board[row][col] == 1 || p2_board[row + 1][col] == 1 || p2_board[row + 1][col + 1] == 1 || p2_board[row + 1][col + 2] == 1){
                            return false;
                        }
                        else{
                            p2_board[row][col] = p2_board[row + 1][col] = p2_board[row + 1][col + 1] = p2_board[row + 1][col + 2] = 1;
                        }
                    }
                    else if(rotation == 3){
                        if(p2_board[row][col] == 1 || p2_board[row][col + 1] == 1 || p2_board[row + 1][col] == 1 || p2_board[row + 2][col] == 1){
                            return false;
                        }
                        else{
                            p2_board[row][col] = p2_board[row][col + 1] = p2_board[row + 1][col] = p2_board[row + 2][col] = 1;
                        }
                    }
                    else{// rotation == 4
                        if(p2_board[row][col] == 1 || p2_board[row][col + 1] == 1 || p2_board[row][col + 2] == 1 || p2_board[row + 1][col + 2] == 1){
                            return false;
                        }
                        else{
                            p2_board[row][col] = p2_board[row][col + 1] = p2_board[row][col + 2] = p2_board[row + 1][col + 2] = 1;
                        }
                    }
                }
                else if(pieces[i][0] == 7){
                    if(rotation == 1){
                        if(p2_board[row][col] == 1 || p2_board[row][col + 1] == 1 || p2_board[row + 1][col + 1] == 1 || p2_board[row][col + 2] == 1){
                            return false;
                        }
                        else{
                            p2_board[row][col] = p2_board[row][col + 1] = p2_board[row + 1][col + 1] = p2_board[row][col + 2] = 1;
                        }
                    }
                    else if(rotation == 2){
                        if(p2_board[row][col] == 1 || p2_board[row][col + 1] == 1 || p2_board[row - 1][col + 1] == 1 || p2_board[row + 1][col + 1] == 1){
                            return false;
                        }
                        else{
                            p2_board[row][col] = p2_board[row][col + 1] = p2_board[row - 1][col + 1] = p2_board[row + 1][col + 1] = 1;
                        }
                    }
                    else if(rotation == 3){
                        if(p2_board[row][col] == 1 || p2_board[row][col + 1] == 1 || p2_board[row - 1][col + 1] == 1 || p2_board[row][col + 2] == 1){
                            return false;
                        }
                        else{
                            p2_board[row][col] = p2_board[row][col + 1] = p2_board[row - 1][col + 1] = p2_board[row][col + 2] = 1;
                        }
                    }
                    else{// rotation == 4
                        if(p2_board[row][col] == 1 || p2_board[row + 1][col] == 1 || p2_board[row + 1][col + 1] == 1 || p2_board[row + 2][col] == 1){
                            return false;
                        }
                        else{
                            p2_board[row][col] = p2_board[row + 1][col] = p2_board[row + 1][col + 1] = p2_board[row + 2][col] = 1;
                        }
                    }
                }
            }
        }
    }
    return true;
}

//-----------------------------------------------Shoot ships game functions------------------------------------------->

void shoot(int player_fd, int player, int *width, int *height){
    char buffer[BUFFER_SIZE] = {0};
    int bytes_read = read(player_fd, buffer, BUFFER_SIZE);

    if(bytes_read <= 0){
        printf("Failed to read from Player %d. Connection closed.\n", player);
        send(player_fd, "E 102", sizeof("E 102"), 0);
        exit(EXIT_FAILURE);
    }

    if(buffer[0] != 'S'){
        printf("101: Invalid packet type (Expected Shoot packet)");
        send(player_fd, "E 101", sizeof("E 101"), 0);
        exit(EXIT_FAILURE);
    }
}


