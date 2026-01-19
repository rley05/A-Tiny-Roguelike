//
//  main.cpp
//  Roguelike
//
//  Created by Riley White on 11/11/25.
//
//TODO: inventory, items, level transition, make enemies shoot ? (that's gonna be rough), color enemies, player, items

//NOTE TO SELF: I should lean into the unique experience and use the limitations of C++ to my advantage. Make sequential turn based combat upon fighting an enemy. (only have enemies move when player moves, otherwise, if the player attacks, the enemies will attack, similar to the original rogue).
//TODO: Find way to connect rooms,
//ROGUE DEFINES THE ENTIRE MAP AS CELLS THAT ARE EITHER VALID OR INVALID BEFOREHAND, THIS PREVENTS COMPLICATED LOADING ISSUES AND EACH CELL CAN STORE
//VISIBLE/INVISIBLE/WALL/FLOOR/ITEMS
//TODO: Make combat sequence | think of fun type of combat, gambling? Rock paper scissors? RPG style?
#include <iostream>
#include <cstdlib>
#include <thread>
#include <ncurses.h>
#include <ctime>
using namespace std;

vector<vector<vector<string>>> level;
int randX;
int randY;
int currentRoom = 0;
int playerRow;
int playerColumn;
int playerhp = 20;
int boundCol;
int boundRow;
char input;
char player = '^';
int prevPos[2] = {playerRow, playerColumn};
float deltaTime = 1.0;
float oldTime = 0;
float accumulator;
int numArrows = 100;
WINDOW *room;
WINDOW *info;
thread refr();
thread en();
thread move();
thread dead();

class enemy{
private:
public:
    int row = rand() % level[currentRoom].size();
    int col = rand() % level[currentRoom][0].size(); //uniform rows so doesn't matter
    int hp = rand() % 2 + 1; //TODO: scale HP with level
    int prevLoc[2] = {row,col};
};

class arrow{
public:
    int row;
    int col;
    int damage;
    int prevLoc[2];
    int direction;
    //row, column, damage, direction
    arrow(int r, int c, int da, int dir){ //dir=direction: 1=north, 2=east, 3=south, 4=west
        prevLoc[0]=r;
        prevLoc[1]=c;
        row = r;
        col = c;
        damage = da;
        direction = dir;
    }
};

vector<enemy> enemies;
vector<arrow> arrows;

void initCurses(){
    initscr();
    noecho();
    halfdelay(1);
    srand(time(0)*1.0);
}

float CurrentTime(){
    return time(nullptr);
}

void generateLevel(){
    //Level > Room > Column
    vector<vector<string>> room;
    for(int k = 0; k<10; k++){
        randX = rand() % 20 + 20;
        randX = randX%2==0 ? randX+=1 : randX;
        randY = rand() % 30 + 30;
        randY = randY%2==0 ? randY+=1 : randY;
        vector<string> column(randY);
        for(int i = 0; i<randX; i++){ //row
            for(int j = 0; j<randY; j++){ //column
                column[j] = ' ';
            }
            room.push_back(column);
        }
        level.push_back(room);
        column.clear();
        room.clear();
    }
    boundRow = level[currentRoom].size()-1.0;
    boundCol = level[currentRoom][0].size()-1.0;
    // could add a level modifier
}

void populateRoom(){
    boundRow = level[currentRoom].size()-1.0;
    boundCol = level[currentRoom][0].size()-1.0;
    int numEnemies = rand() % (3+currentRoom)+1;
    for(int i=0;i<numEnemies;i++){
        enemy enemy;
        enemies.push_back(enemy);
    }
    //when all enemy hp is 0, unlock doors, and clear enemy vector
        //initial enemy positions based on rand defined in their class
        for(enemy a : enemies){
            level[currentRoom][a.row][a.col] = to_string(a.hp);
        }
        playerRow = boundRow;
        playerColumn = boundCol/2;
        level[currentRoom][playerRow][playerColumn] = '^';
}

//I should probably use pointers or iterators, but because this is such a small case, it doesn't matter
int findEnemy(int r, int c){
    if(enemies.size()>0){
        int ind = 0;
        for(enemy en : enemies){
            if(en.col==c && en.row==r){
                return ind; //gives index of enemy
            } else{
                ind++;
            }
        }
        return -1; //no enemy found
    } else{
        return -1;
    }
}

void nextRoom(){
    enemies.clear();
    arrows.clear();
    currentRoom++;
    clear();
    wclear(room);
    //fancy transition
    printw("Next Room!");
    refresh();
    this_thread::sleep_for(2000ms);
    clear();
    
    populateRoom();
    room = newwin(boundRow+3, boundCol+3, 1, 1); // height, width, start_y, start_x
    info = newwin(10,20,1,boundCol+5);
    
}

bool unlockDoor(){
    //return if ONE value is not true
            if(enemies.size()>0){
                return false; //enemies still alive
            }
        return true; //all enemies dear, unlock the doors
}

void attack(){
    //TODO: Rework this to be interesting
    //create arrows and add them to arrow vector. Other function will handle arrow movement
    if(numArrows>0){
        if(player=='^' && (playerRow!=0)){
            arrow proj(playerRow, playerColumn, 1, 1);
            arrows.push_back(proj);
        } else if(player=='<' && playerColumn!=0){
            arrow proj(playerRow, playerColumn, 1, 4);
            arrows.push_back(proj);
        } else if(player=='v' && playerRow!=boundRow){
            arrow proj(playerRow, playerColumn, 1, 3);
            arrows.push_back(proj);
        } else if(player=='>' && playerColumn!=boundCol){
            arrow proj(playerRow, playerColumn, 1, 2);
            arrows.push_back(proj);
        }
        numArrows--;
    }
}

//arrow function?
//arrows have row, col, damage, direction
void arrowLogic(){
    if(arrows.size()>0){
        mvwprintw(info, 0, 15, "arrow logic");
        refresh();
        for(vector<arrow>::iterator at = arrows.begin(); at != arrows.end(); at++){
            at->prevLoc[0] = at->row;
            at->prevLoc[1] = at->col; //set previous location to be drawn over
            //if it hits something (enemy or bounds), find that something and deal damage and remove arrow from array
            int enInd = findEnemy(at->row, at->col);
            if(enInd!=-1){ //check it didn't hit an enemy
                enemies[enInd].hp-=at->damage;
                at = arrows.erase(at);
                at--;
            }
            if(at->row == 0 || at->row == boundRow || at->col == 0 || at->col == boundCol){
                level[currentRoom][at->prevLoc[0]][at->prevLoc[1]] = ' ';
                at = arrows.erase(at);
                at--;
            }
            //decode direction and move in that direction
            if(at->direction==1){
                at->row-=1;
                
            } else if(at->direction==2){
                at->col+=1;
            } else if(at->direction==3){
                at->row+=1;
            } else if(at->direction==4){
                at->col-=1;
            }
        }
        //update level array with current and previous location
            for(arrow ar : arrows){
                level[currentRoom][ar.prevLoc[0]][ar.prevLoc[1]] = ' ';
                if(ar.direction==1 || ar.direction==3){
                    level[currentRoom][ar.row][ar.col] = '|';
                } else if(ar.direction==2 || ar.direction==4){
                    level[currentRoom][ar.row][ar.col] = '-';
                } else{
                    level[currentRoom][ar.row][ar.col] = 'E';
                }
        }
    }
}

void movement(){
        //movement. duh.
        //up down left right attack
        //player direction facing (for combat later)
        while(true){
            //take damage if you try to go to a square with enemies
            input = wgetch(room);
            prevPos[0]=playerRow;
            prevPos[1]=playerColumn;
            if(input=='W' || input=='w'){
                player = '^';
                if(playerRow>0){
                    playerRow-=1;
                    break;
                }
            } else if(input=='A' || input=='a'){
                player = '<';
                if(playerColumn>0){
                    playerColumn-=1;
                    break;
                }
            } else if(input=='S' || input=='s'){
                player = 'v';
                if(playerRow<boundRow){
                    playerRow+=1;
                    break;
                }
            } else if(input=='D' || input=='d'){
                player = '>';
                if(playerColumn<boundCol){
                    playerColumn+=1;
                    break;
                }
            } else if(input=='E' || input=='e'){
                //attack
                attack();
                //this_thread::sleep_for(250ms);
                break; //Enter combat if enemy attacked
                //debug
                //nextRoom();
            }else if(input==ERR){
                break;
            } else{
                mvwprintw(info,4,0,"I can't do that...");
            }
        }
    }

void enemyMove(){
    int rEC;
    int rER;
    int moveRand;
    if(enemies.size()>0){
        for(vector<enemy>::iterator it = enemies.begin(); it != enemies.end(); it++){
            it->prevLoc[0] = it->row;
            it->prevLoc[1] = it->col;
            moveRand = rand()%4; //50% chance they move at all
            if(moveRand==0){
                rER = it->row<playerRow ? rER=it->row+1 : rER=it->row-1;
                if(rER>=0 && rER<=boundRow){
                    it->prevLoc[0]=it->row;
                    it->row=rER;
                } //change row
            } else if(moveRand==1){ //change column
                rEC = it->col<playerColumn ? it->col+1 : it->col-1;
                if(rEC>=0 && rEC<=boundCol){
                    it->prevLoc[1]=it->col;
                    it->col=rEC;
                }
            } else{
                
            }
        }
        for(enemy a : enemies){
                level[currentRoom][a.prevLoc[0]][a.prevLoc[1]] = ' ';
                level[currentRoom][a.row][a.col] = to_string(a.hp);
        }
    }
}

void enemiesDead(){
        //int indcount = 0;
        for(vector<enemy>::iterator it = enemies.begin(); it != enemies.end() && enemies.size()>0; ){
            if(it->row==playerRow && it->col==playerColumn){
                //should make it check if it's adjacent to, not inside. Could make it check if it's between playerRow-1<=enemy<=playerRow+1 and playerColumn-1<=enemy<=playerColumn+1
                playerhp--; //could make this activate a fight?
            }
            if(it->hp<=0){
                //mvwprintw(info, 3, 10, "last ind: %d", indcount);
                //refresh();
                level[currentRoom][it->row][it->col] = ' ';
                //Debugging proved it's this line. Causes a vector out of bounds error maybe?
                it = enemies.erase(it);
            } else{
                it++;
            }
            //indcount++;
        }
    }

void enemyAI(){
        enemyMove();
        enemiesDead();
}

void newRoom(){
    clear();
    room = newwin(boundRow+3, boundCol+3, 1, 1); // height, width, start_y, start_x
    info = newwin(10,20,1,boundCol+5);
    wrefresh(room);
    wrefresh(info);
}

void displayRoom(){
        this_thread::sleep_for(16ms);
        level[currentRoom][prevPos[0]][prevPos[1]] = ' ';
        level[currentRoom][playerRow][playerColumn] = player;
        
        for(int i = 0; i<(level[currentRoom].size());i++){
            for(int j = 0; j<(level[currentRoom][i].size()); j++){
                mvwprintw(room,i+1,j+1,level[currentRoom][i][j].c_str());
            }
        }
        mvwprintw(info,0,0,"Room: %d", currentRoom+1);
        mvwprintw(info,1,0,"HP: %d", playerhp);
    mvwprintw(info,2,0,"Arrows: %d", numArrows);
        mvwprintw(info,3,0,"Enemies: %d", enemies.size());
        
        box(room,0,0);
        refresh();
        wrefresh(room);
        wrefresh(info);
}

/*void progClock(){
        deltaTime = CurrentTime()-oldTime;
    oldTime = CurrentTime();
        accumulator += deltaTime;
        while(accumulator > 1.0/61.0){
            enemyAI();
            movement();
            if(unlockDoor()){
                nextRoom();
            }
            accumulator -= 1.0/59.0;
            if(accumulator < 0) accumulator = 0;
    }
}
*/
int main() {
    //cant move onto next level until threads have ended?? make the condiitons of the threads and while loop
    //based on that?
    initCurses(); //initialize curses
    generateLevel(); //create level
    populateRoom(); //add things to room
    newRoom(); //creates curses window for room
    while(playerhp>0 && currentRoom<=level.size()){
        displayRoom();
        if(unlockDoor()){
            nextRoom();
        }
        arrowLogic();
        enemyAI();
        movement();
    }
    if(playerhp<=0){
        clear();
        printw("You died :(");
        refresh();
    } else{
        clear();
        printw("You made it to the end of the dungeon");
        refresh();
    }
    return 0;
}
