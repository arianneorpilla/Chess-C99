/*
The classic family-friendly, bloody and medieval game of Chess 
written in ANSI C for use in Linux terminals.
by ArtisanLRO 

GNU General Public License v3.0

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <termios.h>
#include <math.h>

#define BOARD_SIZE 8

#define PERSISTENT_TRUE 1
#define PERSISTENT_FALSE 0

#define OFF_BOARD -1

#define PLAYER_1 'X'
#define PLAYER_2 'O'

// Keyhandler logic.
char getch(void)
{
    char buf = 0;
    struct termios old = {0};
    fflush(stdout);
    if(tcgetattr(0, &old) < 0)
        perror("tcsetattr()");
    old.c_lflag &= ~ICANON;
    old.c_lflag &= ~ECHO;
    old.c_cc[VMIN] = 1;
    old.c_cc[VTIME] = 0;
    if(tcsetattr(0, TCSANOW, &old) < 0)
        perror("tcsetattr ICANON");
    if(read(0, &buf, 1) < 0)
        perror("read()");
    old.c_lflag |= ICANON;
    old.c_lflag |= ECHO;
    if(tcsetattr(0, TCSADRAIN, &old) < 0)
        perror("tcsetattr ~ICANON");
    return buf;
}

// Necessary game data.
struct gameState 
{
    char* board;
    char currentPlayer;
    char lastPlayer;
    int turnCount;
    int cursorX;
    int cursorY;
    int* selectMode;
    int* selectedX;
    int* selectedY;
    char selectedPiece;
    int* queensideCastleWhite;
    int* queensideCastleBlack;
    int* kingsideCastleWhite;
    int* kingsideCastleBlack;
    int* kingWhiteX;
    int* kingWhiteY;
    int* kingBlackX;
    int* kingBlackY;
    int* whiteCheck;
    int* blackCheck;
};

// Swap two characters, essential for alternating the checkerboard pattern.
void swapChar(char* a, char* b) {
    char temp = *a;
    *a = *b;
    *b = temp;
}

// Set the current item in a given board position.
void setGridItem(char* board, int x, int y, char item) {
    *(board + y * BOARD_SIZE + x) = item; 
}

// Show the current item in a given board position.
char getGridItem(char* board, int x, int y) {
    return *(board + y * BOARD_SIZE + x);
}

// Check if a given board position is an empty space.
bool isEmpty(char* board, int x, int y) {
    return (getGridItem(board, x, y) == '0');
}

// Check if the given position is within board bounds.
bool validBoardPosition(int x, int y) {
    return ((x >= 0 && x < BOARD_SIZE) && (y >= 0 && y < BOARD_SIZE));
}

// Get the string value of the piece we want to display.
char* getSymbol(char* board, int x, int y) {
    switch(getGridItem(board, x, y)) {
        // Regular Pieces
        case 'p': // Pawn Black
            return "♙";
        case 'P': // Pawn White
            return "♟";
        case 'b': // Bishop Black
            return "♗";
        case 'B': // Bishop White 
            return "♝";
        case 'r': // Rook Black
            return "♖";
        case 'R': // Rook White 
            return "♜";
        case 'n': // Knight Black
            return "♘";
        case 'N': // Knight White 
            return "♞";
        case 'k': // King Black
            return "♔";
        case 'K': // King White 
            return "♚";
        case 'q': // Queen Black
            return "♕";
        case 'Q': // Queen White 
            return "♛";
        case 'E': // Double-Step Pawn White
            return "♟";
        case 'e': // Double-Step Pawn Black
            return "♙";
        case 'A': // En Passant White
            return "♟";
        case 'a': // En Passant Black
            return "♙";
        case '0': // Empty Space
            return " ";
        default: // Corrupt
            return ".";
    }
}

// Check if the given target character is an enemy.
bool checkNotFriendlyTarget(char t, char positionPiece) {
    switch(positionPiece) {
        case 'p': // Pawn Black
        case 'b': // Bishop Black
        case 'r': // Rook Black
        case 'n': // Knight Black
        case 'k': // King Black
        case 'q': // Queen Black
        case 'e': // Double Step Pawn
        case 'a': // En Passant Pawn
            if (t == 'p' || t == 'b' || t == 'r' || t == 'n' 
                || t == 'k' || t == 'q' || t == 'e' || t == 'a') {
                    return true;
            }
            else {
                return false;
            }
            break;
        case 'P': // Pawn White
        case 'B': // Bishop White
        case 'R': // Rook White
        case 'N': // Knight White
        case 'K': // King White
        case 'Q': // Queen White
        case 'E': // Double Step Pawn
        case 'A': // En Passant Pawn
            if (t == 'P' || t == 'B' || t == 'R' || t == 'N' 
                || t == 'K' || t == 'Q' || t == 'E' || t == 'A') {
                    return true;
            }
            else {
                return false;
            }
            break;
        default:
            return false;
    }
}

// Check if the piece selected is the current player's.
int whosePiece(char player) {
    switch(player) {
        case 'p': // Pawn Black
        case 'b': // Bishop Black
        case 'r': // Rook Black
        case 'n': // Knight Black
        case 'k': // King Black
        case 'q': // Queen Black
        case 'e': // Double Step Pawn Black
        case 'a': // En Passant Pawn Black
            if ('O' == player) return true;
        case 'P': // Pawn White
        case 'B': // Bishop White
        case 'R': // Rook White
        case 'N': // Knight White
        case 'K': // King White
        case 'Q': // Queen White
        case 'E': // Double Step Pawn White
        case 'A': // En Passant Pawn White
            if ('X' == player) return true;
        default:
            return '.';
    }
}

// Get the player character given a character piece.
char whoseTurn(char piece) {
    switch(piece) {
        case 'p': // Pawn Black
        case 'b': // Bishop Black
        case 'r': // Rook Black
        case 'n': // Knight Black
        case 'k': // King Black
        case 'q': // Queen Black
        case 'e': // Double Step Pawn Black
        case 'a': // En Passant Pawn Black
            return 'O';
        case 'P': // Pawn White
        case 'B': // Bishop White
        case 'R': // Rook White
        case 'N': // Knight White
        case 'K': // King White
        case 'Q': // Queen White
        case 'E': // Double Step Pawn White
        case 'A': // En Passant Pawn White
            return 'X';
        default:
            return '.';
    }
}

// Helper function necessary for testing where movement for the bishop, rook and queen should stop.
int testCollision(char* board, int positionX, int positionY, int offsetX, int offsetY) {
    int testX = positionX + offsetX;
    int testY = positionY + offsetY;

    int i = 0;

    bool collision = isEmpty(board, testX, testY);

    while(collision && validBoardPosition(testX, testY)) {
        i++;     
        testX = positionX + (i * offsetX);
        testY = positionY + (i * offsetY);
        
        collision = isEmpty(board, testX, testY);
    }

    // Debug collision detection.
    // printf("%d", i);
    return i;
}

// Collision detection for the Bishop piece for diagonal movement.
bool bishopMove(char* board, int targetX, int targetY, int positionX, int positionY) {
    int bounds;
    int offsetX;
    int offsetY;
    
    if (targetX > positionX) {
        // South East Direction
        if (targetY > positionY) {
            bounds = testCollision(board, positionX, positionY, 1, 1);
            offsetX = 1;
            offsetY = 1;
        }
        // North East Direction
        else {
            bounds = testCollision(board, positionX, positionY, 1, -1);
            offsetX = 1;
            offsetY = -1;
        }
    }
    // North West Direction
    else if (targetY > positionY) {
        bounds = testCollision(board, positionX, positionY, -1, 1);
        offsetX = -1;
        offsetY = 1;
    }
    // South West Direction
    else {
        bounds = testCollision(board, positionX, positionY, -1, -1);
        offsetX = -1;
        offsetY = -1;
    }

    // If the offset falls within the given bounds, allow movement there.
    for (int i = 1; i < bounds; i++) {
        int testX = positionX + (offsetX * i);
        int testY = positionY + (offsetY * i);
        bool locationMatch = (testX == targetX) && (testY == targetY);
        
        if (locationMatch) return true;
    }

    return false;
}

// Collision detection for the Rook piece for vertical and horizontal movement.
bool rookMove(char* board, int targetX, int targetY, int positionX, int positionY) {
    int bounds;
    int offsetX;
    int offsetY;

    if (targetX == positionX) {
        // South
        if (targetY > positionY) {
            bounds = testCollision(board, positionX, positionY, 0, 1);
            offsetX = 0;
            offsetY = 1;
        }
        // North
        else {
            bounds = testCollision(board, positionX, positionY, 0, -1);
            offsetX = 0;
            offsetY = -1;
        }
    }
    // West
    else if (targetX > positionX) {
        bounds = testCollision(board, positionX, positionY, 1, 0);
        offsetX = 1;
        offsetY = 0;
    }
    // East
    else {
        bounds = testCollision(board, positionX, positionY, -1, 0);
        offsetX = -1;
        offsetY = 0;
    }

    // If the offset falls within the given bounds, allow movement there.
    for (int i = 1; i < bounds; i++) {
        int testX = positionX + (offsetX * i);
        int testY = positionY + (offsetY * i);
        bool locationMatch = (testX == targetX) && (testY == targetY);
        
        if (locationMatch) return true;
    }

    return false;
}

// The Knight can jump over pieces, so its attack and movement helper functions are the same.
bool knightMove(char* board, int targetX, int targetY, int positionX, int positionY) {

    if (abs(targetX - positionX) == 2 && abs(targetY - targetY) == 1) {
        return true;
    }
    else if (abs(targetX - positionX) == 1 && abs(targetY - positionY) == 2){
        return true;
    }
    else if (abs(targetY - positionY) == 2 && abs(targetX - positionX) == 1) {
        return true;
    }
    else if (abs(targetY - positionY) == 1 && abs(targetX - positionX) == 2){
        return true;
    }
    else {
        return false;
    }
}

// Check if Bishop can attack a given location.
bool bishopAttack(char* board, int positionX, int positionY, int targetX, int targetY) {

    // Collision detection.
    int offsetSE = testCollision(board, positionX, positionY, 1, 1);
    int offsetNW = testCollision(board, positionX, positionY, -1, -1);
    int offsetSW = testCollision(board, positionX, positionY, -1, 1);
    int offsetNE = testCollision(board, positionX, positionY, 1, -1);

    // Calculate non-proximity offsets.
    bool SE = ((positionX + (offsetSE * 1)) == targetX) && (positionY + (offsetSE * 1) == targetY);
    bool NW = ((positionX + (offsetNW * -1)) == targetX) && (positionY + (offsetNW * -1) == targetY);
    bool SW = ((positionX + (offsetSW * -1)) == targetX) && (positionY + (offsetSW * 1) == targetY);
    bool NE = ((positionX + (offsetNE * 1)) == targetX) && (positionY + (offsetNE * -1) == targetY);

    // Positions direct to the offset will report value zero - these can't be multiplied and need to be done too.
    bool SEP = ((positionX + 1 == targetX) && (positionY + 1 == targetY));
    bool NWP = ((positionX + -1 == targetX) && (positionY + -1 == targetY));
    bool SWP = ((positionX + -1 == targetX) && (positionY + 1 == targetY));
    bool NEP = ((positionX + 1 == targetX) && (positionY + -1 == targetY));

    // If any of these locations, allow movement.
    if (SE) return true;
    if (NW) return true;
    if (SW) return true;
    if (NE) return true;

    if (SEP) return true;
    if (NWP) return true;
    if (SWP) return true;
    if (NEP) return true;

    return false;
}

// Check if Rook can attack a given location.
bool rookAttack(char* board, int positionX, int positionY, int targetX, int targetY) {

    int offsetN = testCollision(board, positionX, positionY, 0, 1);
    int offsetS = testCollision(board, positionX, positionY, 0, -1);
    int offsetW = testCollision(board, positionX, positionY, -1, 0);
    int offsetE = testCollision(board, positionX, positionY, 1, 0);

    // Calculate direct offsets.
    bool NORTHP = (positionX == targetX) && (positionY + 1 == targetY);
    bool SOUTHP = (positionX == targetX) && (positionY -1 == targetY);
    bool WESTP = (positionX -1 == targetX) && (positionY == targetY);
    bool EASTP = (positionX + 1 == targetX) && (positionY == targetY);

     // Positions direct to the offsets will report value zero - these can't be multiplied and need to be done too.
    bool NORTH = ((positionX + (offsetN * 0)) == targetX) && (positionY + (offsetN * 1) == targetY);
    bool SOUTH = ((positionX + (offsetS * 0)) == targetX) && (positionY + (offsetS * -1) == targetY);
    bool WEST = ((positionX + (offsetW * -1)) == targetX) && (positionY + (offsetW * 0) == targetY);
    bool EAST = ((positionX + (offsetE * 1)) == targetX) && (positionY + (offsetE * 0) == targetY);

    if (NORTH) return true;
    if (SOUTH) return true;
    if (WEST) return true;
    if (EAST) return true;

    if (NORTHP) return true;
    if (SOUTHP) return true;
    if (WESTP) return true;
    if (EASTP) return true;

    return false;
}

// Special En Passant helper function.
char convertSpecialPiece(char piece) {
    switch(piece) {
        case 'E': // Double-Step Pawn White
            return 'A';
        case 'e': // Double-Step Pawn Black
            return 'a';
        case 'A': // En Passant White
            return 'P';
        case 'a': // En Passant Black
            return 'p';
        default:
            return piece;
    }
}

// Header files? Never heard of them.
bool isAttackLegal(char* board, char piece, int targetX, int targetY, int positionX, int positionY, char turn);

// Check if the King is in check, given a player.
bool kingPassiveCheck(char* board, char turn) {
    int kingBlackX;
    int kingBlackY;
    int kingWhiteX;
    int kingWhiteY;

    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            if (getGridItem(board, x, y) == 'K') {
                kingWhiteX = x;
                kingWhiteY = y;
            }
            else if (getGridItem(board, x, y) == 'k') {
                kingBlackX = x;
                kingBlackY = y;
            }
        }
    }

    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            char piece = getGridItem(board, x, y);
            bool isLegal = isAttackLegal(board, piece, kingBlackX, kingBlackY, x, y, turn);
            
            // Black is check
            if (isLegal) {
                return true;
            }
        }
    }

    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            char piece = getGridItem(board, x, y);
            bool isLegal = isAttackLegal(board, piece, kingWhiteX, kingWhiteY, x, y, turn);
            
            // White is check
            if (isLegal) {
                return true;
            }
        }
    }

    return false;
}

// Check if the King will be in check, given a possible movement by the king.
bool kingActiveCheck(char* board, char pieceKing, int targetX, int targetY, int positionX, int positionY, char turn) {
    
    char pieceAtTarget = getGridItem(board, targetX, targetY);
    setGridItem(board, positionX, positionY, '.');
    setGridItem(board, targetX, targetY, pieceKing);
    
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            char piece = getGridItem(board, x, y);
            bool isLegal = isAttackLegal(board, piece, targetX, targetY, x, y, turn);

            bool notOwnTurn = (turn != whosePiece(pieceKing));
            
            if (isLegal && notOwnTurn) {
                setGridItem(board, targetX, targetY, pieceAtTarget);
                setGridItem(board, positionX, positionY, pieceKing);
                return false;
            }
            else if (isLegal && ~notOwnTurn) {
                setGridItem(board, targetX, targetY, pieceAtTarget);
                setGridItem(board, positionX, positionY, pieceKing);
                return true;
            }
        }
    }
    setGridItem(board, positionX, positionY, pieceKing);
    setGridItem(board, targetX, targetY, pieceAtTarget);
    return true;
}

// Check if the King can move to a given adjacent tile.
bool kingMove(char* board, char piece, int targetX, int targetY, int positionX, int positionY, char turn) {
    
    char pieceKing = getGridItem(board, positionX, positionY);
    
    // For each up, down, left and right offset, detect if the King will be in check.
    if ((targetX - positionX) == 0 && abs(targetY - targetY) == 1) {
        return kingActiveCheck(board, pieceKing, targetX, targetY, positionX, positionY, turn);
    }
    else if (abs(targetX - positionX) == 1 && (targetY == positionY)) {
        return kingActiveCheck(board, pieceKing, targetX, targetY, positionX, positionY, turn);
    }
    else if (abs(targetX - positionX) == 0 && abs(targetY - positionY) == 1) {
        return kingActiveCheck(board, pieceKing, targetX, targetY, positionX, positionY, turn);
    }
    else if (abs(targetY - positionY) == 1 && abs(targetX - positionX) == 1) {
        return kingActiveCheck(board, pieceKing, targetX, targetY, positionX, positionY, turn);
    }
    else {
        return false;
    }
}

// Segmentation fault. Probably the use of nested for-loops that end up chaining isAttackLegal()

// Detect check, given a player.
// char detectCheck(char* board, char pieceMove, int targetX, int targetY, int positionX, int positionY, char turn) {

//     char pieceAtTarget = getGridItem(board, targetX, targetY);
//     setGridItem(board, targetX, targetY, pieceMove);
//     setGridItem(board, positionX, positionY, pieceAtTarget);

//     for (int y = 0; y < BOARD_SIZE; y++) {
//         for (int x = 0; x < BOARD_SIZE; x++) {
//             if (getGridItem(board, x, y) == 'k') {
//                 if (kingPassiveCheck(board, turn)) {
//                     setGridItem(board, targetX, targetY, pieceAtTarget);
//                     setGridItem(board, positionX, positionY, pieceMove);
//                     return 'k';
                    
//                 }
//             }
//             else if (getGridItem(board, x, y) == 'K') {
//                 if (kingPassiveCheck(board, turn)) {
//                     setGridItem(board, targetX, targetY, pieceAtTarget);
//                     setGridItem(board, positionX, positionY, pieceMove);
//                     return 'K';
//                 }
//             }
//         }
//     }

//     setGridItem(board, targetX, targetY, pieceAtTarget);
//     setGridItem(board, positionX, positionY, pieceMove);
//     return '.';
// }

// Simulate if a player's desired move will resolve the check.
bool resolveCheck(char* board, char pieceMove, int targetX, int targetY, int positionX, int positionY, int kingX, int kingY, char turn) {

    char pieceAtTarget = getGridItem(board, targetX, targetY);
    setGridItem(board, targetX, targetY, pieceMove);
    setGridItem(board, positionX, positionY, '0');

    if(kingPassiveCheck(board, turn)) {
        setGridItem(board, targetX, targetY, pieceAtTarget);
        setGridItem(board, positionX, positionY, pieceMove);
        return false;
    }
    
    setGridItem(board, targetX, targetY, pieceAtTarget);
    setGridItem(board, positionX, positionY, pieceMove);
    return true;
}

// Special helper function for Castling.
bool isQueensideCastleWhiteLegal(char* board, char turn) {

    if (kingPassiveCheck(board, turn)) {
        setGridItem(board, 4, 7, 'K');
        setGridItem(board, 3, 7, '0');
        setGridItem(board, 2, 7, '0');
        setGridItem(board, 1, 7, '0');
        setGridItem(board, 0, 7, 'R');
        return false;
    }

    setGridItem(board, 4, 7, '0');
    setGridItem(board, 3, 7, 'K');

    if (kingPassiveCheck(board, turn)) {
        setGridItem(board, 4, 7, 'K');
        setGridItem(board, 3, 7, '0');
        setGridItem(board, 2, 7, '0');
        setGridItem(board, 1, 7, '0');
        setGridItem(board, 0, 7, 'R');
        return false;
    }

    setGridItem(board, 3, 7, '0');
    setGridItem(board, 2, 7, 'K');

    if (kingPassiveCheck(board, turn)) {
        setGridItem(board, 4, 7, 'K');
        setGridItem(board, 3, 7, '0');
        setGridItem(board, 2, 7, '0');
        setGridItem(board, 1, 7, '0');
        setGridItem(board, 0, 7, 'R');
        return false;
    }

    setGridItem(board, 4, 7, 'K');
    setGridItem(board, 3, 7, '0');
    setGridItem(board, 2, 7, '0');
    setGridItem(board, 1, 7, '0');
    setGridItem(board, 0, 7, 'R');
    return true;
}

// Special helper function for Castling.
bool isKingsideCastleWhiteLegal(char* board, char turn) {

    if (kingPassiveCheck(board, turn)) {
        setGridItem(board, 7, 7, 'R');
        setGridItem(board, 6, 7, '0');
        setGridItem(board, 5, 7, '0');
        setGridItem(board, 4, 7, 'K');
        return false;
    }

    setGridItem(board, 4, 7, '0');
    setGridItem(board, 5, 7, 'K');

    if (kingPassiveCheck(board, turn)) {
        setGridItem(board, 7, 7, 'R');
        setGridItem(board, 6, 7, '0');
        setGridItem(board, 5, 7, '0');
        setGridItem(board, 4, 7, 'K');
        return false;
    }

    setGridItem(board, 5, 7, '0');
    setGridItem(board, 6, 7, 'K');

    if (kingPassiveCheck(board, turn)) {
        setGridItem(board, 7, 7, 'R');
        setGridItem(board, 6, 7, '0');
        setGridItem(board, 5, 7, '0');
        setGridItem(board, 4, 7, 'K');
        return false;
    }

    setGridItem(board, 7, 7, 'R');
    setGridItem(board, 6, 7, '0');
    setGridItem(board, 5, 7, '0');
    setGridItem(board, 4, 7, 'K');
    return true;
}

// Special helper function for Castling.
bool isQueensideCastleBlackLegal(char* board, char turn) {

    if (kingPassiveCheck(board, turn)) {
        setGridItem(board, 4, 0, 'k');
        setGridItem(board, 3, 0, '0');
        setGridItem(board, 2, 0, '0');
        setGridItem(board, 1, 0, '0');
        setGridItem(board, 0, 0, 'r');
        return false;
    }
    setGridItem(board, 4, 0, '0');
    setGridItem(board, 3, 0, 'k');

    if (kingPassiveCheck(board, turn)) {
        setGridItem(board, 4, 0, 'k');
        setGridItem(board, 3, 0, '0');
        setGridItem(board, 2, 0, '0');
        setGridItem(board, 1, 0, '0');
        setGridItem(board, 0, 0, 'r');
        return false;
    }
    setGridItem(board, 3, 0, '0');
    setGridItem(board, 2, 0, 'k');

    if (kingPassiveCheck(board, turn)) {
        setGridItem(board, 4, 0, 'k');
        setGridItem(board, 3, 0, '0');
        setGridItem(board, 2, 0, '0');
        setGridItem(board, 1, 0, '0');
        setGridItem(board, 0, 0, 'r');
        return false;
    }

    setGridItem(board, 4, 0, 'k');
    setGridItem(board, 3, 0, '0');
    setGridItem(board, 2, 0, '0');
    setGridItem(board, 1, 0, '0');
    setGridItem(board, 0, 0, 'r');
    return true;
}

// Special helper function for Castling.
bool isKingsideCastleBlackLegal(char* board, char turn) {

    if (kingPassiveCheck(board, turn)) {
        setGridItem(board, 7, 0, 'r');
        setGridItem(board, 6, 0, '0');
        setGridItem(board, 5, 0, '0');
        setGridItem(board, 4, 0, 'k');
        return false;
    }
    setGridItem(board, 4, 7, '0');
    setGridItem(board, 5, 7, 'K');

    if (kingPassiveCheck(board, turn)) {
        setGridItem(board, 7, 0, 'r');
        setGridItem(board, 6, 0, '0');
        setGridItem(board, 5, 0, '0');
        setGridItem(board, 4, 0, 'k');
        return false;
    }
    setGridItem(board, 5, 0, '0');
    setGridItem(board, 6, 0, 'K');

    if (kingPassiveCheck(board, turn)) {
        setGridItem(board, 7, 0, 'r');
        setGridItem(board, 6, 0, '0');
        setGridItem(board, 5, 0, '0');
        setGridItem(board, 4, 0, 'k');
        return false;
    }
    
    setGridItem(board, 7, 0, 'r');
    setGridItem(board, 6, 0, '0');
    setGridItem(board, 5, 0, '0');
    setGridItem(board, 4, 0, 'k');
    return true;
}

// Check if movement is legal for a given piece, given its position and a target location.
bool isMoveLegal(char* board, char piece, int targetX, int targetY, int positionX, int positionY, char turn) {

    char targetPiece = getGridItem(board, targetX, targetY);
    bool friendlyPiece = checkNotFriendlyTarget(targetPiece, piece);
    if (friendlyPiece) return false;

    switch(piece) {
        case 'e': // Double-Step Pawn Black
            if (isEmpty(board, targetX,  targetY)) {
                if (positionY == 1) {
                    return ((targetY + -1 == positionY && positionX == targetX) ||
                                (targetY + -2 == positionY && positionX == targetX &&
                                    isEmpty(board, targetX, targetY - 1)));
                }
                else {
                    return (targetY + -1 == positionY && positionX == targetX);
                }
            }
        case 'E': // Double-Step Pawn White
            if (isEmpty(board, targetX, targetY)) {
                if (positionY == 6) {
                    return ((targetY + 1 == positionY && positionX == targetX) ||
                                (targetY + 2 == positionY && positionX == targetX &&
                                    isEmpty(board, targetX, targetY + 1)));
                }
                else {
                    return (targetY + 1 == positionY && positionX == targetX);
                }
            }
        case 'p':
        case 'a': // En Passant Black
            if (isEmpty(board, targetX, targetY)) {
                return (targetY - 1 == positionY && positionX == targetX);
            }
            else {
                return false;
            }
        case 'P': // Pawn White
        case 'A': // En Passant White
            if (isEmpty(board, targetX, targetY)) {
                return (targetY + 1 == positionY && positionX == targetX);
            }
            else {
                return false;
            }
        case 'b': // Bishop Black
        case 'B': // Bishop White
            return bishopMove(board, targetX, targetY, positionX, positionY);
        case 'r': // Rook Black
        case 'R': // Rook White
            return rookMove(board, targetX, targetY, positionX, positionY);
        case 'n': // Rook Black
        case 'N': // Rook White
            return knightMove(board, targetX, targetY, positionX, positionY);
        case 'q':
        case 'Q':
            return ((rookMove(board, targetX, targetY, positionX, positionY)) || (bishopMove(board, targetX, targetY, positionX, positionY)));
        case 'k':
        case 'K':
            return kingMove(board, piece, targetX, targetY, positionX, positionY, turn);
        default: // Corrupt
            return false;
    }
}

// Check if attacking is legal for a given piece, given its position and a target location.
bool isAttackLegal(char* board, char piece, int targetX, int targetY, int positionX, int positionY, char turn) {
    
    char targetPiece = getGridItem(board, targetX, targetY);
    bool friendlyPiece = checkNotFriendlyTarget(targetPiece, piece);
    bool targetEmpty = isEmpty(board, targetX, targetY);
    
    bool whitePassant = getGridItem(board, targetX, targetY + 1) == 'a';
    bool blackPassant = getGridItem(board, targetX, targetY - 1) == 'A';

    if (friendlyPiece) return false;
    
    if (isEmpty(board, targetX, targetY)) {
        if (piece == 'P') {
            if (positionY == 3 && whitePassant && abs(targetX - positionX) == 1) {
                return true;
            }
            else {
                return false;
            }
        }
        else if (piece == 'p') {
            if (positionY == 4 && blackPassant && abs(targetX - positionX) == 1) {
                return true;
            }
            else {
                return false;
            }
        }
        else {
            return false;
        }
    }
    switch (piece) {
        case 'E': // Double Step Pawn White
        case 'P': // Pawn White
        case 'A': // En Passant White
            return (targetY + 1 == positionY && abs(targetX - positionX) == 1);
            break;
        case 'e': // Double Step Pawn Black
        case 'p': // Pawn Black
        case 'a': // En Passant Black
            return (targetY - 1 == positionY && abs(targetX - positionX) == 1);
            break;
        case 'B': // Bishop Black
        case 'b': // Bishop White
            return bishopAttack(board, positionX, positionY, targetX, targetY);
            break;
        case 'R': // Rook White
        case 'r': // Rook Black
            return rookAttack(board, positionX, positionY, targetX, targetY);
            break;
        case 'Q': // Queen White
        case 'q': // Queen Black
            return (bishopAttack(board, positionX, positionY, targetX, targetY) || rookAttack(board, positionX, positionY, targetX, targetY));
            break;
        case 'K':
        case 'k':
            return kingMove(board, piece, targetX, targetY, positionX, positionY, turn);
        case 'N':
        case 'n':
            return knightMove(board, targetX, targetY, positionX, positionY);
        default:
            return false;
    }
}

// Check if castling is legal for a rook or a king, given its position and a target location.
bool isCastleLegal(struct gameState state, char* board, char castlePiece, int targetX, int targetY, int positionX, int positionY, char turn) {

    bool rookSelectedQCW = false;
    bool rookSelectedKCW = false;
    bool rookSelectedQCB = false;
    bool rookSelectedKCB = false;

    if (castlePiece == 'R') {
        if (positionX == 7 && positionY == 7) {
            rookSelectedKCW = true;
        }
        else if (positionX == 0 && positionY == 7) {
            rookSelectedQCW = true;
        }
    }
    else if (castlePiece == 'r') {
        if (positionX == 0 && positionY == 0) {
            rookSelectedQCB = true;
        }
        else if (positionX == 7 && positionY == 0) {
            rookSelectedKCB = true;
        }
    }

    if (whoseTurn(castlePiece) == turn) {
        switch(castlePiece) {
            case 'R':
                if (rookSelectedQCW && *state.queensideCastleWhite) {
                    if (isEmpty(board, 1, 7) && isEmpty(board, 2, 7) && isEmpty(board, 3, 7)) {
                        if (targetX == 3 && targetY == 7 && isQueensideCastleWhiteLegal(board, turn)) {
                            return true;
                        }
                    }
                }
                else if (rookSelectedKCW && *state.kingsideCastleWhite) {
                    if (isEmpty(board, 6, 7) && isEmpty(board, 5, 7)) {
                        if (targetX == 5 && targetY == 7 && isKingsideCastleWhiteLegal(board, turn)) {
                            return true;
                        }
                    }
                }
                return false;
            case 'r':
                if (rookSelectedQCB && *state.queensideCastleBlack) {
                    if (isEmpty(board, 1, 0) && isEmpty(board, 2, 0) && isEmpty(board, 3, 0)) {
                        if (targetX == 3 && targetY == 0 && isQueensideCastleBlackLegal(board, turn)) {
                            return true;
                        }
                    }
                }
                else if (rookSelectedKCB && *state.kingsideCastleBlack) {
                    if (isEmpty(board, 6, 0) && isEmpty(board, 5, 0)) {
                        if (targetX == 5 && targetY == 0 && isKingsideCastleBlackLegal(board, turn)) {
                            return true;
                        }
                    }
                }
                return false;
            case 'K':
                if (*state.queensideCastleWhite) {
                    if (isEmpty(board, 1, 7) && isEmpty(board, 2, 7) && isEmpty(board, 3, 7)) {
                        if (targetX == 2 && targetY == 7 && isQueensideCastleWhiteLegal(board, turn)) {
                            return true;
                        }
                    }
                }
                if (*state.kingsideCastleWhite) {
                    if (isEmpty(board, 6, 7) && isEmpty(board, 5, 7)) {
                        if (targetX == 6 && targetY == 7 && isKingsideCastleWhiteLegal(board, turn)) {
                            return true;
                        }
                    }
                }
                return false;
            case 'k':
                if (*state.queensideCastleBlack) {
                    if (isEmpty(board, 1, 0) && isEmpty(board, 2, 0) && isEmpty(board, 3, 0)) {
                        if (targetX == 2 && targetY == 0 && isQueensideCastleBlackLegal(board, turn)) {
                            return true;
                        }
                    }
                }
                if (*state.kingsideCastleBlack) {
                    if (isEmpty(board, 6, 0) && isEmpty(board, 5, 0)) {
                        if (targetX == 6 && targetY == 0 && isKingsideCastleBlackLegal(board, turn)) {
                            return true;
                        }
                    }
                }
                return false;
            default:
                return false;
        }
    }
    return false;
}

// Color codes for the game board.
void printTile(char color, char* symbol) {
    switch(color) {
        case 'n': // black
            printf("\e[40m %s \e[0m", symbol);  
            break;
        case 'r': // red
            printf("\e[41m %s \e[0m", symbol);
            break;
        case 'g': // green
            printf("\e[42m %s \e[0m", symbol);
            break;
        case 'y': // yellow
            printf("\e[43m %s \e[0m", symbol);
            break;
        case 'b': // blue
            printf("\e[44m %s \e[0m", symbol);
            break;
        case 'm': // magenta
            printf("\e[45m %s \e[0m", symbol);
            break;
        case 'c': // cyan
            printf("\e[46m %s \e[0m", symbol);
            break;
        case 'w': // white
            printf("\e[47m %s \e[0m", symbol);
            break;
        default:
            ;
    }
}

// Display the game board in the terminal.
void renderBoard(struct gameState state) {

    char whiteSpace = 'A';
    char blackSpace = 'B';
    char init = blackSpace;

    *(state.whiteCheck) = PERSISTENT_FALSE;
    *(state.blackCheck) = PERSISTENT_FALSE;

    for (int y = 0; y < BOARD_SIZE; y++) {
        printf("\e[0m  %d ", 8 - y);

        for (int x = 0; x < BOARD_SIZE; x++) {
            char hoverPiece = getGridItem(state.board, state.cursorX, state.cursorY);
            char selectedPiece = getGridItem(state.board, *state.selectedX, *state.selectedY);
            char processPiece = getGridItem(state.board, x, y);

            bool hoverMoveLegal = isMoveLegal(state.board, hoverPiece, x, y, state.cursorX, state.cursorY, state.currentPlayer);
            bool selectedMoveLegal = isMoveLegal(state.board, selectedPiece, x, y, *state.selectedX, *state.selectedY, state.currentPlayer);

            bool hoverAttackLegal = isAttackLegal(state.board, hoverPiece, x, y, state.cursorX, state.cursorY, state.currentPlayer);
            bool selectedAttackLegal = isAttackLegal(state.board, selectedPiece, x, y, *state.selectedX, *state.selectedY, state.currentPlayer);

            bool checkWhite = isAttackLegal(state.board, processPiece, *state.kingWhiteX, *state.kingWhiteY, x, y, state.currentPlayer);
            bool checkBlack = isAttackLegal(state.board, processPiece, *state.kingBlackX, *state.kingBlackY, x, y, state.currentPlayer);
            
            bool hoverCastleLegal = isCastleLegal(state, state.board, hoverPiece, x, y, state.cursorX, state.cursorY, state.currentPlayer);
            bool selectedCastleLegal = isCastleLegal(state, state.board, selectedPiece, x, y, *state.selectedX, *state.selectedY, state.currentPlayer);

            bool notTurn = (whoseTurn(hoverPiece) != state.currentPlayer);
            
            bool cursorFocus = (state.cursorX == x && state.cursorY == y);
            bool selectedFocus = *state.selectMode && (*state.selectedX == x && *state.selectedY == y);
            
            if (checkWhite) {
                *(state.whiteCheck) = PERSISTENT_TRUE;
                if (selectedAttackLegal) {
                    printTile('m', getSymbol(state.board, x, y));
                }
                else if (hoverAttackLegal) {
                    printTile('r', getSymbol(state.board, x, y));
                }
                else {
                    printTile('r', getSymbol(state.board, x, y));
                }
            }
            else if (checkBlack) {
                *(state.blackCheck) = PERSISTENT_TRUE;
                if (selectedAttackLegal) {
                    printTile('m', getSymbol(state.board, x, y));
                }
                else if (hoverAttackLegal) {
                    printTile('r', getSymbol(state.board, x, y));
                }
                else {
                    printTile('r', getSymbol(state.board, x, y));
                }
            }
            // If the tile is on the user cursor.
            else if (cursorFocus) {
                // If the user is in select mode.
                if (*state.selectMode) {
                    // If the user is hovering over their selected piece.
                    if (selectedFocus) {
                        printTile('c', getSymbol(state.board, x, y));
                    }
                    // If the user is hovering over a piece they can attack.
                    else if (selectedAttackLegal) {
                        printTile('m', getSymbol(state.board, x, y));
                    }
                    // If the user is hovering over a legal tile to move to.
                    else if (selectedMoveLegal) {
                        printTile('w', getSymbol(state.board, x, y));
                    }
                    else if (selectedCastleLegal) {
                        printTile('y', getSymbol(state.board, x, y));
                    }
                    // If the user is hovering over an illegal tile.
                    else {
                        printTile('r', getSymbol(state.board, x, y));
                    }
                }
                // If the piece hovered by the user is not the current player's turn.
                else if (notTurn) {
                    printTile('r', getSymbol(state.board, x, y));
                }
                else if (hoverMoveLegal) {
                    printTile('b', getSymbol(state.board, x, y));
                }
                else if (hoverCastleLegal) {
                    printTile('y', getSymbol(state.board, x, y));
                }
                else {
                    printTile('b', getSymbol(state.board, x, y));
                }
                swapChar(&whiteSpace, &blackSpace);
            }
            else {
                if (*state.selectMode) {
                    // Glow the player's selected tile.
                    if (selectedFocus) {
                        printTile('c', getSymbol(state.board, x, y));
                    }
                    // GLow any possible attacks.
                    else if (selectedAttackLegal) {
                        printTile('r', getSymbol(state.board, x, y));
                    }
                    // Glow any possible castle positions.
                    else if (selectedCastleLegal) {
                        printTile('y', getSymbol(state.board, x, y));
                    }
                    // Glow any possible movement positions.
                    else if (selectedMoveLegal) {
                        printTile('g', getSymbol(state.board, x, y));
                    }
                    // Print board pattern.
                    else if (init == whiteSpace) {
                        printf("\e[40m %s \e[0m", getSymbol(state.board, x, y)); 
                    }
                    // If not, show the contents according to pointer offset.
                    else {
                        printf("\e[0;100m %s \e[0m", getSymbol(state.board, x, y)); 
                    }   
                }
                else if (hoverCastleLegal) {
                    printTile('y', getSymbol(state.board, x, y));
                }
                else if (hoverAttackLegal) {
                    printTile('m', getSymbol(state.board, x, y));
                }
                else if (hoverMoveLegal) {
                    if (notTurn) {
                        printTile('m', getSymbol(state.board, x, y));
                    }
                    else {
                        printTile('c', getSymbol(state.board, x, y));
                    }
                }
                else {
                    // Print the board background, if the information is unimportant.
                    if (init == whiteSpace) {
                        printf("\e[40m %s \e[0m", getSymbol(state.board, x, y)); 
                        
                    }
                    // Print the board in an alternating pattern.
                    else {
                        printf("\e[0;100m %s \e[0m", getSymbol(state.board, x, y)); 
                    }
                }
                swapChar(&whiteSpace, &blackSpace);      
            }
        }
        // Swap the alternating checkered pattern of the board after every row.
        swapChar(&whiteSpace, &blackSpace);
        printf("\e[0m %d  \n", 8 - y);
    }
    // Render algebraic notation letters.
    printf("\e[0m     a  b  c  d  e  f  g  h     ");
}

// Display the game board in the terminal.
void renderDummyBoard() {

    char whiteSpace = 'A';
    char blackSpace = 'B';
    char init = blackSpace;

    printf("\n\e[0m     a  b  c  d  e  f  g  h     \n");

    for (int y = 0; y < BOARD_SIZE; y++) {
        printf("\e[0m  %d ", 8 - y);
        for (int x = 0; x < BOARD_SIZE; x++) {
            if (init == whiteSpace) {
                printf("\e[40m   \e[0m", '0'); 
            }
            else {
                printf("\e[0;100m   \e[0m"); 
            }
            swapChar(&whiteSpace, &blackSpace);
        }
        
        // Swap the alternating checkered pattern of the board after every row.
        swapChar(&whiteSpace, &blackSpace);
        printf("\e[0m %d  \n", 8 - y);
    }
    // Render algebraic notation letters.
    printf("\e[0m     a  b  c  d  e  f  g  h     ");
}

// Check if Castling is possible, then show feedback to controls if possible.
void printCastle(struct gameState state) {

    bool showCastleHoverOverlay = false;
    bool showCastleSelectOverlay = false;

    // Detect if Castling is legal with any pieces.
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            char hoverPiece = getGridItem(state.board, state.cursorX, state.cursorY);
            char selectedPiece = getGridItem(state.board, *state.selectedX, *state.selectedY);
            
            bool hoverCastleLegal = isCastleLegal(state, state.board, hoverPiece, x, y, state.cursorX, state.cursorY, state.currentPlayer);
            bool selectedCastleLegal = isCastleLegal(state, state.board, selectedPiece, x, y, *state.selectedX, *state.selectedY, state.currentPlayer);

            if (hoverCastleLegal) {
                showCastleHoverOverlay = true;
            }
            if (selectedCastleLegal) {
                showCastleSelectOverlay = true;
            }
        }
    }

    // If the player can castle, report this in the controls.
    if (*state.selectMode && showCastleSelectOverlay) {
        printf("\n           c - CASTLE \n\n");
        
    }
    else if (~(*state.selectMode) && showCastleHoverOverlay) {
        printf("\n           c - CASTLE \n\n");
    }
    else {
        printf("\n\n");
    }
}

int pawnPromotion(char* board, int positionX, int positionY, char currentPlayer) {
   
    printf("\n\n         PAWN PROMOTION    ");
    printf("\n           b - bishop      ");
    printf("\n           k - knight      ");
    printf("\n           q - queen       ");
    printf("\n           r - rook        ");
    bool notSelected = PERSISTENT_TRUE;

    while(notSelected) {
        char ch = getch();
        if (currentPlayer == 'X') {
            switch(ch) {
                case 'B':
                case 'b':
                    setGridItem(board, positionX, positionY, 'B');
                    notSelected = PERSISTENT_FALSE;
                    break;
                case 'K':
                case 'k':
                    setGridItem(board, positionX, positionY, 'N');
                    notSelected = PERSISTENT_FALSE;
                    break;
                case 'Q':
                case 'q':
                    setGridItem(board, positionX, positionY, 'Q');
                    notSelected = PERSISTENT_FALSE;
                    break;
                case 'r':
                case 'R':
                    setGridItem(board, positionX, positionY, 'R');
                    notSelected = PERSISTENT_FALSE;
                    break;
                default:
                    ;
            }
        }
        else {
            switch(ch) {
                case 'b':
                case 'B':
                    setGridItem(board, positionX, positionY, 'b');
                    notSelected = PERSISTENT_FALSE;
                    break;
                case 'k':
                case 'K':
                    setGridItem(board, positionX, positionY, 'n');
                    notSelected = PERSISTENT_FALSE;
                    break;
                case 'q':
                case 'Q':
                    setGridItem(board, positionX, positionY, 'q');
                    notSelected = PERSISTENT_FALSE;
                    break;
                case 'r':
                case 'R':
                    setGridItem(board, positionX, positionY, 'r');
                    notSelected = PERSISTENT_FALSE;
                    break;
                default:
                    ;
            }
        }
    }
}

// 
int printGame(struct gameState state) {

    // Clear the terminal.
    system("clear");

    // Print the header.
    printf("\n\e[0;107m                                ");
    printf("\n\e[0;100m           C99 CHESS            ");
    printf("\n\e[0;100m         by ArtisanLRO          ");
    printf("\n\e[0;107m                                ");
    printf("\n\e[0m     a  b  c  d  e  f  g  h     \n");
    
    // Render the game board.
    renderBoard(state);

    // Print player information.
    if (state.currentPlayer == 'X') {
        printf("\n\e[0;100m■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■\e[0m");
        printf("\n\e[0m       Turn %d - White Turn    ", state.turnCount);
        printf("\n\e[0;100m■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■\e[0m");
    }
    else {
        printf("\n\e[0;100m□□□□□□□□□□□□□□□□□□□□□□□□□□□□□□□□\e[0m");
        printf("\n\e[0m       Turn %d - Black Turn    ", state.turnCount);
        printf("\n\e[0;100m□□□□□□□□□□□□□□□□□□□□□□□□□□□□□□□□\e[0m");
    }
    
    // Show controls
    printf("\n\e                               \e[0m");
    printf("\n        ijkl - NAVIGATE");
    printf("\n      x - SELECT  p - DROP\e[0m");
    
    // Show if Castling is possible.
    printCastle(state);

    // Show if Selection Mode is on.
    if (*state.selectMode) {
        printf("         SELECT MODE ON");
    }

    if (*state.whiteCheck) {
        printf("\n       CHECK ON KING WHITE");
    }

    if (*state.blackCheck) {
        printf("\n       CHECK ON KING BLACK");
    }

    // Hide the actual terminal cursor.
    printf("\e[?25l");

    return 1;
}

void gameLoop(struct gameState state) {

    while(1) {
        printGame(state);

        // Debug Castling

        // printf("\n%d: QCB\n", *state.queensideCastleBlack);
        // printf("%d: QCW\n", *state.queensideCastleWhite);
        // printf("%d: KCB\n", *state.kingsideCastleBlack);
        // printf("%d: KCW\n", *state.kingsideCastleWhite);

        // Keyhandler
        char ch = getch();

        switch(ch) {
            // If on the edges, loop back to the other side of the board.
            case 'I':
            case 'i': // Cursor Up
                if (state.cursorY == 0) state.cursorY = BOARD_SIZE - 1;
                else state.cursorY--;
                break;
            case 'K':
            case 'k': // Cursor Down
                if (state.cursorY == BOARD_SIZE - 1) state.cursorY = 0;
                else state.cursorY++;
                break;
            case 'L':
            case 'l': // Cursor Right
                if (state.cursorX == BOARD_SIZE - 1) state.cursorX = 0;
                else state.cursorX++;
                break;
            case 'J':
            case 'j': // Cursor Left
                if (state.cursorX == 0) state.cursorX = BOARD_SIZE - 1;
                else state.cursorX--;
                break;
            case 'X':
            case 'x': // Select
                state.selectedPiece = getGridItem(state.board, state.cursorX, state.cursorY);
                bool properTurn = (whoseTurn(state.selectedPiece) == state.currentPlayer);

                // Toggle Selection Mode
                if (*state.selectMode) {
                    *state.selectMode = PERSISTENT_FALSE;
                    *state.selectedX = OFF_BOARD;
                    *state.selectedY = OFF_BOARD;
                }
                else {
                    if (properTurn) {
                        *state.selectMode = PERSISTENT_TRUE;
                        *state.selectedX = state.cursorX;
                        *state.selectedY = state.cursorY;
                    }
                }
                break;
            case 'P':
            case 'p': // Drop Piece
                    ;
                if (*state.selectMode) {
                    bool canAttack = isAttackLegal(state.board, state.selectedPiece, state.cursorX, state.cursorY, *state.selectedX, *state.selectedY, state.currentPlayer);
                    bool canMove = isMoveLegal(state.board, state.selectedPiece, state.cursorX, state.cursorY, *state.selectedX, *state.selectedY, state.currentPlayer);
                    bool canResolveCheck = PERSISTENT_TRUE;
                
                    char movePiece = getGridItem(state.board, *state.selectedX, *state.selectedY);

                    if (*state.whiteCheck) {
                        canResolveCheck = resolveCheck(state.board, state.selectedPiece, state.cursorX, state.cursorY, *state.selectedX, *state.selectedY, *state.kingBlackX, *state.kingBlackY, state.currentPlayer);
                    }
                    else if (*state.blackCheck) {
                        canResolveCheck = resolveCheck(state.board, state.selectedPiece, state.cursorX, state.cursorY, *state.selectedY, *state.selectedY, *state.kingWhiteX, *state.kingWhiteY, state.currentPlayer);
                    }

                    // Only perform this action if the player can move or attack.
                    if ((canAttack || canMove) && canResolveCheck) {

                        // Enables En Passant special rule for pawns, normally the game ignores any empty spots in attack,
                        // so this has to be handled in a special way.
                        if (movePiece == 'P') {
                            if (getGridItem(state.board, state.cursorX, state.cursorY + 1) == 'a' && (getGridItem(state.board, state.cursorX, state.cursorY) == '0')) {
                                setGridItem(state.board, state.cursorX, state.cursorY + 1, '0');
                            }
                        }
                        else if (movePiece == 'p') {
                            if (getGridItem(state.board, state.cursorX, state.cursorY - 1) == 'A' && (getGridItem(state.board, state.cursorX, state.cursorY) == '0')) {
                                setGridItem(state.board, state.cursorX, state.cursorY - 1, '0');
                            }
                        }

                        // If the piece moved is the king, then castling is no longer permitted.
                        if (movePiece == 'K') {
                            *state.kingsideCastleWhite = PERSISTENT_FALSE;
                            *state.queensideCastleWhite = PERSISTENT_FALSE;
                            *state.kingWhiteX = state.cursorX;
                            *state.kingWhiteY = state.cursorY;
                        }
                        else if (movePiece == 'k') {
                            *state.kingsideCastleBlack = PERSISTENT_FALSE;
                            *state.queensideCastleBlack = PERSISTENT_FALSE;
                            *state.kingBlackX = state.cursorX;
                            *state.kingBlackY = state.cursorY;
                        }

                        // If the piece moved is the rook, then castling with that certain rook is no longer permitted.
                        if (movePiece == 'R') {
                            if (*state.selectedX == 7 && *state.selectedY == 7) {
                                *state.kingsideCastleWhite = PERSISTENT_FALSE;
                            }
                            else if (*state.selectedX == 0 && *state.selectedY == 7) {
                                *state.queensideCastleWhite = PERSISTENT_FALSE;
                            }
                        }
                        else if (movePiece == 'r') {
                            if (*state.selectedX == 0 && *state.selectedY == 0) {
                                *state.queensideCastleBlack = PERSISTENT_FALSE;
                            }
                            else if (*state.selectedX == 7 && *state.selectedY == 0) {
                                *state.kingsideCastleBlack = PERSISTENT_FALSE;
                            }
                        }

                        // Empty the position the piece moved from.
                        setGridItem(state.board, *state.selectedX, *state.selectedY, '0');

                        // If there are any En Passant vulnerable pieces, they are no longer vulnerable in the next turn.
                        for (int y = 0; y < BOARD_SIZE; y++) {
                            for (int x = 0; x < BOARD_SIZE; x++) {
                                char pieceAt = getGridItem(state.board, x, y);
                                if (pieceAt == 'A' || pieceAt == 'a') {
                                    setGridItem(state.board, x, y, convertSpecialPiece(pieceAt));
                                }
                            }
                        }

                        // Demote any double move pawns into single move pawns.                        
                        bool doubleMove = ((movePiece == 'E' || movePiece == 'e') 
                                            && abs(state.cursorY - *state.selectedY) == 2);

                        if (doubleMove) {
                            setGridItem(state.board, state.cursorX, state.cursorY, convertSpecialPiece(movePiece));
                        }
                        else if (movePiece == 'P' && state.cursorY == 0) {
                            pawnPromotion(state.board, state.cursorX, state.cursorY, state.currentPlayer);
                        }
                        else if (movePiece == 'p' && state.cursorY == 7) {
                            pawnPromotion(state.board, state.cursorX, state.cursorY, state.currentPlayer);
                        }
                        else {
                            setGridItem(state.board, state.cursorX, state.cursorY, movePiece);
                        }

                        // Selection move will be toggled off at the end of the turn.
                        *state.selectMode = PERSISTENT_FALSE;
                        
                        // Swap turns.
                        state.turnCount++;

                        state.lastPlayer = state.currentPlayer;

                        // Can probably be changed into a pointer swap for two characters.
                        if (state.currentPlayer == PLAYER_1) {
                            state.currentPlayer = PLAYER_2;
                        }
                        else {
                            state.currentPlayer = PLAYER_1;
                        }
                    }
                    else {
                        printf("\n\n          ILLEGAL MOVE      ");
                        if (*state.whiteCheck || *state.blackCheck) {
                            printf("\n        CHECK UNRESOLVED    ");
                        }
                        getch();
                    }
                }
                else {
                    printf("       PIECE NOT SELECTED    ");
                    getch();
                } 
                break;
            case 'c': // Castle
                ;
                bool canCastle = isCastleLegal(state, state.board, state.selectedPiece, state.cursorX, state.cursorY, *state.selectedX, *state.selectedY, state.currentPlayer);
                char movePiece = getGridItem(state.board, *state.selectedX, *state.selectedY);

                if (*state.selectMode && canCastle) {
                    if (movePiece == 'k' || movePiece == 'r') {
                        *(state.kingsideCastleBlack) = PERSISTENT_FALSE;
                        *(state.queensideCastleBlack) = PERSISTENT_FALSE; 
                    }
                    else if (movePiece == 'K' || movePiece == 'R') {
                        *(state.kingsideCastleWhite) = PERSISTENT_FALSE;
                        *(state.queensideCastleWhite) = PERSISTENT_FALSE;
                    }

                    switch(movePiece) {
                        case 'R':
                            // QCW
                            if (state.cursorX == 3 && state.cursorY == 7) {
                                setGridItem(state.board, 4, 7, '0');
                                setGridItem(state.board, 3, 7, '0');
                                setGridItem(state.board, 2, 7, 'K');
                                setGridItem(state.board, 1, 7, 'R');
                                setGridItem(state.board, 0, 7, '0');
                            }
                            // KCW
                            else if (state.cursorX == 5 && state.cursorY == 7) {
                                setGridItem(state.board, 4, 7, '0');
                                setGridItem(state.board, 5, 7, 'R');
                                setGridItem(state.board, 6, 7, 'K');
                                setGridItem(state.board, 7, 7, '0');
                            }
                        case 'r':
                            // QCB
                            if (state.cursorX == 3 && state.cursorY == 0) {
                                setGridItem(state.board, 4, 0, '0');
                                setGridItem(state.board, 3, 0, '0');
                                setGridItem(state.board, 2, 0, 'K');
                                setGridItem(state.board, 1, 0, 'R');
                                setGridItem(state.board, 0, 0, '0');
                            }
                            // QCB
                            else if (state.cursorX == 5 && state.cursorY == 7) {
                                setGridItem(state.board, 4, 0, '0');
                                setGridItem(state.board, 5, 0, 'R');
                                setGridItem(state.board, 6, 0, 'K');
                                setGridItem(state.board, 7, 0, '0');
                            }
                        case 'K':
                            // QCW
                            if (state.cursorX == 2 && state.cursorY == 7) {
                                setGridItem(state.board, 4, 7, '0');
                                setGridItem(state.board, 3, 7, '0');
                                setGridItem(state.board, 2, 7, 'K');
                                setGridItem(state.board, 1, 7, 'R');
                                setGridItem(state.board, 0, 7, '0');
                            }
                            // KCW
                            else if (state.cursorX == 6 && state.cursorY == 7) {\
                                setGridItem(state.board, 4, 7, '0');
                                setGridItem(state.board, 5, 7, 'R');
                                setGridItem(state.board, 6, 7, 'K');
                                setGridItem(state.board, 7, 7, '0');
                            }
                        case 'k':
                            // QCB
                            if (state.cursorX == 2 && state.cursorY == 0) {
                                setGridItem(state.board, 4, 0, '0');
                                setGridItem(state.board, 3, 0, '0');
                                setGridItem(state.board, 2, 0, 'K');
                                setGridItem(state.board, 1, 0, 'R');
                                setGridItem(state.board, 0, 0, '0');
                            }
                            // QCB
                            else if (state.cursorX == 6 && state.cursorY == 0) {
                                setGridItem(state.board, 4, 0, '0');
                                setGridItem(state.board, 5, 0, 'R');
                                setGridItem(state.board, 6, 0, 'K');
                                setGridItem(state.board, 7, 0, '0');
                            }
                            
                        *state.selectMode = PERSISTENT_FALSE;
                        *state.selectedX = -1;
                        *state.selectedY = -1;
                        
                        // Swap turns.
                        state.turnCount++;

                        state.lastPlayer = state.currentPlayer;

                        // Can probably be changed into a pointer swap for two characters.
                        if (state.currentPlayer == PLAYER_1) {
                            state.currentPlayer = PLAYER_2;
                        }
                        else {
                            state.currentPlayer = PLAYER_1;
                        }
                    }
                }
                else {
                    printf("       PIECE NOT SELECTED    ");
                    getch();
                }  
            default: 
                ; // Do nothing, if not any of the controls.
        }
    }
}

void populateBoard(char* board) {

    // Pawns White
    for (int x = 0; x < BOARD_SIZE; x++) {
        setGridItem(board, x, 1, 'e');
    }
    // Pawns Black
    for (int x = 0; x < BOARD_SIZE; x++) {
        setGridItem(board, x, 6, 'E');
    }

    // Rooks Black
    setGridItem(board, 0, 0, 'r');
    setGridItem(board, 7, 0, 'r');
    // Rooks White
    setGridItem(board, 7, 7, 'R');
    setGridItem(board, 0, 7, 'R');

    // Bishops Black
    setGridItem(board, 2, 0, 'b');
    setGridItem(board, 5, 0, 'b');
    // Bishops White
    setGridItem(board, 2, 7, 'B');
    setGridItem(board, 5, 7, 'B');

    // Knights Black
    setGridItem(board, 1, 0, 'n');
    setGridItem(board, 6, 0, 'n');
    // Knights White
    setGridItem(board, 6, 7, 'N');
    setGridItem(board, 1, 7, 'N');

    // King Black
    setGridItem(board, 4, 0, 'k');
    // King White
    setGridItem(board, 4, 7, 'K');

    // Queen Black
    setGridItem(board, 3, 0, 'q');
    // Queen White
    setGridItem(board, 3, 7, 'Q');

    // Empty Space
    for (int y = 2; y < 6; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            setGridItem(board, x, y, '0');
        }
    }
}

// Set initial variables and start the game.
int initialiseGame() {

    // Player symbols.
    char playerFirst = 'X';
    char playerSecond = 'O';

    // Necessary to record the current player's turn.
    char currentPlayer = 'X';
    char lastPlayer = '.';
    char selectedPiece = '.';

    // The game starts on the first turn.
    int turnCount = 1;

    // Initialise cursor position.
    int cursorX = 4;
    int cursorY = 7;
    
    // Dynamically allocates memory for the square grid of the game board.
    char * board = (char *)calloc(BOARD_SIZE * BOARD_SIZE, sizeof(char));

    // Add the starting pieces to the game board.
    populateBoard(board);

    // Selection values are always changed and passed by reference.
    int * selectMode = (int *)malloc(sizeof(int));
    int * selectedX = (int *)malloc(sizeof(int));
    int * selectedY = (int *)malloc(sizeof(int));
    
    int * kingWhiteX = (int *)malloc(sizeof(int));
    int * kingWhiteY = (int *)malloc(sizeof(int));
    int * kingBlackX = (int *)malloc(sizeof(int));
    int * kingBlackY = (int *)malloc(sizeof(int));
    

    // Boolean values for castling need to be handled and modified by the game state.
    int * queensideCastleWhite = (int *)malloc(sizeof(int));
    int * queensideCastleBlack = (int *)malloc(sizeof(int));
    int * kingsideCastleWhite = (int *)malloc(sizeof(int));
    int * kingsideCastleBlack = (int *)malloc(sizeof(int));
    
    int * whiteCheck = (int *)malloc(sizeof(int));
    int * blackCheck = (int *)malloc(sizeof(int));
    
    *(selectMode) = PERSISTENT_FALSE;
    *(selectedX) = OFF_BOARD;
    *(selectedY) = OFF_BOARD;

    *(queensideCastleWhite) = PERSISTENT_TRUE;
    *(queensideCastleBlack) = PERSISTENT_TRUE;
    *(kingsideCastleWhite) = PERSISTENT_TRUE;
    *(kingsideCastleBlack) = PERSISTENT_TRUE;

    *(kingWhiteX) = 4;
    *(kingWhiteY) = 7;
    *(kingBlackX) = 4;
    *(kingBlackY) = 0;

    *(whiteCheck) = PERSISTENT_FALSE;
    *(blackCheck) = PERSISTENT_FALSE;

    // Initialise a game state structure.
    struct gameState state = {
        board,
        currentPlayer,
        lastPlayer,
        turnCount,
        cursorX,
        cursorY,
        selectMode,
        selectedX,
        selectedY,
        selectedPiece,
        queensideCastleWhite,
        queensideCastleBlack,
        kingsideCastleWhite,
        kingsideCastleBlack,
        kingWhiteX,
        kingWhiteY,
        kingBlackX,
        kingBlackY,
        whiteCheck,
        blackCheck
    };    

    // The game loop.
	gameLoop(state);
    
    // Free memory dynamically allocated for the game board at end of game.
    // This will never be reached, unless an actual checkmate detection system is properly added.
    // Possibly, one can count possible moves and check if it is zero, but currently the game segfaults with the current implementation.
    free(board);
    free(selectMode);
    free(selectedX);
    free(selectedY);
    free(queensideCastleWhite);
    free(queensideCastleBlack);
    free(kingsideCastleWhite);
    free(kingsideCastleBlack);
    free(kingWhiteX);
    free(kingWhiteY);
    free(kingBlackX);
    free(kingBlackY);
    free(whiteCheck);
    free(blackCheck);
}

// Initialise main menu.
int main() {
    // Clear the terminal.
    system("clear");

    // Print the header.
    printf("\n\e[0;107m                                ");
    printf("\n\e[0;100m           C99 CHESS            ");
    printf("\n\e[0;100m         by ArtisanLRO          ");
    printf("\n\e[0;107m                                ");
    
    // Print a dummy board.
    renderDummyBoard();

    printf("\n\e[0;100m■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■\e[0m");
    printf("\n\e[0m     PRESS ANY KEY TO START");
    printf("\n\e[0;100m■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■\e[0m");\
    
    getch();

    initialiseGame();
}