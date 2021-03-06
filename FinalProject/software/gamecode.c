#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>

#define player_sizeX 104/2
#define player_sizeY 158/2
#define screenWidth 640
#define screenHeight 480

volatile unsigned int * VGA_VS_PIO = (volatile unsigned int *) 0x000000a0;
volatile unsigned int * LED_PIO = (volatile unsigned int *) 0x00000090;
volatile unsigned int * GAME_INTERFACE = (volatile unsigned int *) 0x00000040;

int game_state;

enum mvt_state {Standing, Move_Left, Move_Right, Jump_Up, Jump_Up_Left, Jump_Up_Right, Jump_Down, Jump_Down_Left, Jump_Down_Right, Landing};
enum act_state {None, Punching, Punching_Wait, Blocking, Taking_Damage, Dying, Dead}; // punching needs to be depressed, hence punching_wait as extra state

typedef struct Player_Software {
    unsigned int x;
    unsigned int y;
    int health;
    enum mvt_state last_movement_state, movement_state;
    enum act_state last_action_state, action_state;
    unsigned int animation_cycle;
    unsigned int action_cycle;
} Player_Software;

void setDefaultSW(int x, Player_Software * player) {
	player->x = x;
	player->y = 400;
	player->health = 100;
	player->last_movement_state = player->movement_state = Standing;
	player->last_action_state = player->action_state = None;
	player->animation_cycle = 0;
    player->action_cycle = 0;
}

typedef struct Player_Hardware {
    volatile unsigned int keycode;
    /** Keycode Map:
    1) W
    2) A
    3) A+W
    4) S
    5) S+W
    6) S+A
    7) S+A+W
    8) D
    9) D+W
    **/
    volatile unsigned int x;
    volatile unsigned int y;
    volatile unsigned int direction;
    volatile unsigned int width;
    volatile unsigned int height;
    volatile unsigned int health;
    volatile unsigned int animation;
    /** Animation Code:
    0) Standing 1
    1) Walking
    2) Standing 2
    3) Jump (up)
    4) Jump (down)
    5) Jump (landing)
    6) Punching
    7) Blocking
    8) Damaged
    9) Dying
    10) Dead
    **/
} Player_Hardware;

void setDefaultHW(int x, int direction, Player_Hardware * hardware) {
	hardware->x = x;
	hardware->y = 400;
    hardware->direction = 0;
	hardware->width = 106/2;
	hardware->height = 160/2;
	hardware->health = 100;
	hardware->animation = 1;
}

void updateMovement(Player_Software * player, Player_Hardware * hardware) {
	unsigned mvt_keycode = hardware->keycode & 0x0f;
	// lockout movements in certain states
    if ((player->movement_state == Standing || player->movement_state == Move_Left || player->movement_state == Move_Right) && player->action_state != Dying && player->action_state != Dead) {
    	if(mvt_keycode == 0) { // none being pressed
    		player->movement_state = Standing;
    	}
    	else if(mvt_keycode == 8) { // only D being pressed
            player->movement_state = Move_Right;
        }
        else if(mvt_keycode == 2) { // only A being pressed
            player->movement_state = Move_Left;
        }
        else if(mvt_keycode == 1) { // only W being pressed
            player->movement_state = Jump_Up;
        }
        else if(mvt_keycode == 3) { // WA pressed
            player->movement_state = Jump_Up_Left;
        }
        else if(mvt_keycode == 9) { // WD pressed
            player->movement_state = Jump_Up_Right;
        }
    }
	// change from jump up to down automatically after a few cycle
    if (((player->movement_state == Jump_Up) || (player->movement_state == Jump_Up_Left) || (player->movement_state == Jump_Up_Right)) && player->animation_cycle == 20) {
    	if (player->movement_state == Jump_Up) player->movement_state = Jump_Down;
    	else if (player->movement_state == Jump_Up_Left) player->movement_state = Jump_Down_Left;
    	else if (player->movement_state == Jump_Up_Right) player->movement_state = Jump_Down_Right;
    	player->animation_cycle = 0;
    }
	// change from jumping to landing
    else if (((player->movement_state == Jump_Down) || (player->movement_state == Jump_Down_Left) || (player->movement_state == Jump_Down_Right)) && player->y >= 400) {
    	player->y = 400;
        player->movement_state = Landing;
        player->animation_cycle = 0;
    }
}
void performMovement(Player_Software * player, Player_Hardware * hardware) {
	if (player->movement_state != Landing) { // don't move while landing
		if ((player->movement_state == Move_Right) || (player->movement_state == Jump_Up_Right) || (player->movement_state == Jump_Down_Right)) {
			if (player->x + 5 < screenWidth - player_sizeX) player->x += 5;
		}
		else if ((player->movement_state == Move_Left) || (player->movement_state == Jump_Up_Left) || (player->movement_state == Jump_Down_Left)) {
			if (player->x + 5 > player_sizeX) player->x -= 5;
		}
		// jumping has acceleration based on animaiton cycle
		if ((player->movement_state == Jump_Up) || (player->movement_state == Jump_Up_Left) || (player->movement_state == Jump_Up_Right)) {
			player->y -= abs(player->animation_cycle - 30)/2;
		}
		else if ((player->movement_state == Jump_Down) || (player->movement_state == Jump_Down_Left) || (player->movement_state == Jump_Down_Right)) {
			player->y += abs(player->animation_cycle)/2;
		}
		// set x and y
		hardware->x = player->x;
		hardware->y = player->y;
	}
}

void setDirection(Player_Software * player_1, Player_Hardware * hardware_1, Player_Software * player_2, Player_Hardware * hardware_2) {
	// set direction players are facing
	if (player_1->x > player_2->x) {
		if (player_1->action_state != Dying && player_1->action_state != Dead) hardware_1->direction = 1;
		if (player_2->action_state != Dying && player_2->action_state != Dead) hardware_2->direction = 0;
	}
	else  {
		if (player_1->action_state != Dying && player_1->action_state != Dead) hardware_1->direction = 0;
		if (player_2->action_state != Dying && player_2->action_state != Dead) hardware_2->direction = 1;
	}
}

void drawMovementAnimation(Player_Software * player, Player_Hardware * hardware) {

    // set different animations if movement didn't change
    if (player->last_movement_state == player->movement_state) {
        if (player->animation_cycle == 30) { // set a new animation every .5 seconds
            if (player->movement_state == Standing) {
                if (hardware->animation == 0) hardware->animation = 2;
                else hardware->animation = 0;
                player->animation_cycle = 0; // reset animation counter at end of .5 seconds
            }
            else if (player->movement_state == Move_Left || player->movement_state == Move_Right) {
                if (hardware->animation == 1) hardware->animation = 0; // swap between standing and moving when moving
                else hardware->animation = 1;
                player->animation_cycle = 0; // reset animation counter at end of .5 seconds
            }
        }
        if (player->animation_cycle == 10 && player->movement_state == Landing) {
			player->movement_state = Standing;
			player->animation_cycle = 0; // reset animation counter at end of .5 seconds
		}
        else player->animation_cycle++;
    }
    else { // a new movement this cycle
        if (player->movement_state == Standing) hardware->animation = 0;
        if (player->movement_state == Move_Right || player->movement_state == Move_Left) hardware->animation = 1;
        if (player->movement_state == Jump_Up || player->movement_state == Jump_Up_Left || player->movement_state == Jump_Up_Right) hardware->animation = 3;
        if ((player->movement_state == Jump_Down) || (player->movement_state == Jump_Down_Left) || (player->movement_state == Jump_Down_Right)) hardware->animation = 4;
        if (player->movement_state == Landing) hardware->animation = 5;
        player->last_movement_state = player->movement_state;
        player->animation_cycle = 0; // reset animation counter at start of different movement
    }
}

void updateActions(Player_Software * player, Player_Hardware * hardware) {
    if (player->action_state != Taking_Damage && player->action_state != Punching && player->action_state != Dying && player->action_state != Dead) { // lock player out from doing actions when taking_damage or mid punch
        if (hardware->keycode >> 6 == 0) { // release of button resets action state
            player->action_state = None;
        }
        else if (hardware->keycode >> 6 == 1) { // only block button was pressed
            player->action_state = Blocking;
        }
        else if (hardware->keycode >> 6 == 2) { // only punch button was pressed
            player->action_state = Punching;
        }
    }
}

void performActions(Player_Software * player_1, Player_Hardware * hardware_1, Player_Software * player_2, Player_Hardware * hardware_2) {

	if (player_1->action_state == Punching) {
        if (player_1->action_cycle == 5 && ( player_2->action_state != Dying && player_2->action_state != Dead) ) { // perform damage in 5th action cycle
            if (abs(player_1->x - player_2->x) < player_sizeX && abs(player_1->y - player_2->y) < player_sizeY) { // player's are in bound of each other, will do damage to other player
                player_2->health = (player_2->action_state == Blocking) ? player_2->health - 5 : player_2->health - 10; // if player 2 was blocking, less damage recieved
                player_2->action_state = Taking_Damage;
            }
            player_1->action_cycle++;
        }
        else if (player_1->action_cycle == 20) {
            player_1->action_state = Punching_Wait;
            player_1->action_cycle = 0;
        }
        else {player_1->action_cycle++;}
    }
    else if (player_1->action_state == Taking_Damage) { // recover after 15 cycles
        if (player_1->action_cycle == 7) {
            player_1->action_state = None;
            player_1->action_cycle = 0;
        }
        else { player_1->action_cycle++; }
    }
    else if (player_1->action_state == Dying) { // recover after 15 cycles
		if (player_1->action_cycle == 15) {
			player_1->action_state = Dead;
			player_1->action_cycle = 0;
		}
		else { player_1->action_cycle++; }
	}
}

void drawActionAnimation(Player_Software * player, Player_Hardware * hardware) {
    // set health
    hardware->health = player->health;

    // set animation for action
    if (player->action_state == Punching || player->action_state == Punching_Wait) {
        hardware->animation = 6;
    }
    else if (player->action_state == Blocking) {
        hardware->animation = 7;
    }
    else if (player->action_state == Taking_Damage) {
    	printf("damage");
        hardware->animation = 8;
    }
    else if (player->action_state == Dying) {
    	printf("dying");
		hardware->animation = 9;
	}
    else if (player->action_state == Dead) {
    	printf("dead");
    	hardware->animation = 10;
    }

}
void checkHealth(Player_Software * player, Player_Hardware * hardware) {
	if (player->health <= 0) {
		if (game_state == 1) {
			player->action_state = Dying;
		}
		player->health = 0;
		hardware->health = 0;
		game_state = 0;
	}

}


int main() {
	// initialize player
    Player_Software p1s;
    setDefaultSW(100, &p1s);
    Player_Hardware * p1h = (Player_Hardware *) GAME_INTERFACE; // change
    setDefaultHW(100, 0, p1h);

    Player_Software p2s;
    setDefaultSW(540, &p2s);
    Player_Hardware * p2h = (Player_Hardware *) &(GAME_INTERFACE[8]); // change
    setDefaultHW(540, 1, p2h);

    game_state = 1;

	int frame_synchronizer = 1;
    while (1) {
    	if (frame_synchronizer != *VGA_VS_PIO) {}
    	else {
    		frame_synchronizer = !frame_synchronizer;
			
			// Reset game
    		if (p2h->keycode == 4 && p1h->keycode == 4) {
    			setDefaultSW(100, &p1s);
    			setDefaultSW(540, &p2s);
    			setDefaultHW(100, 0, p1h);
    			setDefaultHW(540, 1, p2h);
    			game_state = 1;
    		}

			updateMovement(&p1s,p1h); updateMovement(&p2s,p2h);
			performMovement(&p1s,p1h); performMovement(&p2s,p2h);
			drawMovementAnimation(&p1s,p1h); drawMovementAnimation(&p2s,p2h);
			setDirection(&p1s,p1h,&p2s,p2h);

			updateActions(&p1s, p1h); updateActions(&p2s, p2h);
			performActions(&p1s,p1h,&p2s,p2h); performActions(&p2s,p2h,&p1s,p1h);  // randomize who will win keep press speed battle
			checkHealth(&p1s, p1h); checkHealth(&p2s, p2h);
			drawActionAnimation(&p1s, p1h); drawActionAnimation(&p2s, p2h);
			printf("heathp1: %d    heathp2: %d\n", p1s.health, p2s.health);



    	}
    }
}
