# CS120B_BOMBERMAN

Report details

	I developed a game of 2 player bomberman utilizing a Sega Genesis Controller, an 8x8 RGB Matrix,  and 3 shift registers. The 2 players can move either up, left, down, or right unless if their path is blocked by a bomb, the other player, or a wall. By pressing B, a player can place a bomb onto the ground, which will explode in all 4 directions until it hits a wall. Each player can place a maximum of 3 bombs. If a player gets hit by a bomb, then the game ends. A blue screen appears if player 1 wins, and a red screen appears if player 2 wins. Player 1 can then restart the game by pressing the C button. Bombs also set off other bombs, which force the players to be more careful of their positioning and where they place the bombs. For this project, I programmed an ATmega1284 microcontroller on  AVR Studio 6 using C.
	
	I progressed through my project by first aiming to get a dot to move on the screen, adding the walls, implementing collision, dropping dots when a player presses a, making the dots disappear after a while, having the dots explode, having the explosions get blocked by walls, restarting the game when a player gets hit, adding an additional player, having multiple bombs, and then finally having bombs be able to explode other bombs.

	The main problem I had during my project were hardware issues. I encountered few problems with my coding throughout the project. A bug that has not been fixed yet though when I demoed my project are that player 2 does not die when he gets hit by his own bomb at a range of 0 and player’s bombs do not set off the opposing player’s bombs.

Link to Demo Video:
http://www.youtube.com/watch?v=knljM_TF93k&feature=youtu.be

Link to Code:
https://github.com/kylemiho/CS120B_BOMBERMAN

