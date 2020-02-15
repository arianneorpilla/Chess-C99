# Chess-C99
![image](https://i.imgur.com/DT0L331.png)

The classic family-friendly, bloody and medieval game of Chess written in C-99 for use in Linux terminals. Written to apply memory management, pointers and structures and basic C knowledge in preparation for CSSE2310.

## Installation Commands
```
git clone https://github.com/ArtisanLRO/Chess-C99.git
cd Chess-C99
gcc -std=c99 chess.c -o chess -lm
./chess
```
![image](https://i.imgur.com/HltNU6k.png)
## To Do
- There are instances where a piece would randomly spawn or if a piece can't be dropped - check behavior is weird, may have to look into it.
## Done So Far
- Every piece moves as basically as it should, Pawn, Bishop, Knight, Rook, Queen, King.
- Game board is handled with malloc, passed and handled in a struct.
- Special rules of chess apply, double-step rule, En Passant, Pawn Promotion and Queenside and Kingside Castling all function like they should.
- Basic collision detection for Rook and Bishop (that shouldn't be able to hit anything once they hit the side of the board or an enemy).
- Vim style navigation - ijkl for navigation and xp for character swapping (lmao)
## Possible Extensions
- Game save/load functionality from previous Tic-Tac-Toe project could easily be ported over.
- Dabbled with sockets a bit. Almost thought I could get them to work, I could get chat going but converting the game to a client/server format was tougher than I imagined.
- Checkmate detection, as the game doesn't end and just refuses to let the player make any moves in a definite checkmate.
- Code could be cleaned up a lot, everything could be parameterised better.
- Use structs more, functions are too long and some are longwinded and repeat themselves.
