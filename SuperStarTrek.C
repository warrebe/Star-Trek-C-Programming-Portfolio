#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <time.h>

#define PI 3.14159265359
#define MAX_INPUT_LENGTH 127
#define GAME_NUM_KLINGONS 26
#define GAME_NUM_STARBASES 3
#define GAME_NUM_STARS 192
#define GAME_ENERGY_MAX 3000
#define GAME_TORPEDOES_MAX 10
#define GAME_INITIAL_STARDATE 2700.0
#define GAME_STARDATE_LIMIT 26

// enum declarations:
typedef enum {GREEN, YELLOW, RED, DOCKED} Condition;

// struct declarations:
struct Starbase {
  int position[4];
  int energy;
};

struct Klingon {
  int position[4];
  int energy;
};

struct Enterprise {
  int position[4];
  int explored[9][9];
  bool isDestroyed;
  double sysDamage[8];
  char sysNames[8][32];
  int energy;
  int shields;
  int torpedoes;
  Condition condition;
  bool userQuit;
};

struct Galaxy {
  char coordinates[9][9][9][9];
  double stardate;
  struct Starbase starbases[GAME_NUM_STARBASES];
  int starbaseCount;
  int starbasesDestroyed;
  struct Klingon klingons[GAME_NUM_KLINGONS];
  int klingonCount;
  struct Enterprise enterprise;
  bool glblDEBUG;                                                               // global debug variable which enables non-game debug options; triggered by giving DBG command
};

// function declarations:
struct Galaxy createGalaxy(void);
struct Enterprise createEnterprise(struct Galaxy *refGalaxy);
struct gameVitals getGameVitals(struct Galaxy *refGalaxy);
struct Galaxy gameIntro(void);
bool gameEnd(struct Galaxy* refGalaxy);
void getCommand(struct Galaxy* refGalaxy);
void exeHLP(struct Galaxy *refGalaxy);
void exeNAV(struct Galaxy *refGalaxy);
void exeSRS(struct Galaxy *refGalaxy);
void exeLRS(struct Galaxy *refGalaxy);
void exeDAM(struct Galaxy *refGalaxy);
void exeCOM(struct Galaxy *refGalaxy);
void exePHA(struct Galaxy *refGalaxy);
void exeTOR(struct Galaxy *refGalaxy);
void exeSHE(struct Galaxy *refGalaxy);
void exeXXX(struct Galaxy *refGalaxy);
void exeDBG(struct Galaxy *refGalaxy);
void exeSLR(struct Galaxy* refGalaxy);
int getTotalEnergy(struct Enterprise* refEnt);
double getDist(struct Galaxy* refGalaxy, int* destination);
void setDest(int* _start, double dir, double dist, int* _destination);
double getDirection(double yD, double xD);
double checkObstacles(int* _start, double dir, double dist, struct Galaxy* refGalaxy);
void KlingonsFire(struct Galaxy *refGalaxy);
void KlingonsFireMT(struct Galaxy* refGalaxy, double WS);
void KlingonsMove(struct Galaxy* refGalaxy);
int StarbasesInQuadrant(struct Galaxy* refGalaxy, int* Q);
int KlingonsInQuadrant(struct Galaxy* refGalaxy, int* Q);
struct Klingon* getNthClosestKlingon(struct Galaxy* refGalaxy, int n);
struct Starbase* getClosestStarbase(struct Galaxy* refGalaxy);
void updateCondition(struct Galaxy* refGalaxy);
double maxnum(double a, double b);
double minnum(double a, double b);
void strtrim(char* string, int n);
void remNL(char* string, int n);
void strToLower(char* string, int n);
void strToUpper(char* string, int n);
double RND1(void);
void displayManual(void);

////////////////////////////////////////////
//////////////// START MAIN ////////////////
int main() {
    bool gameRunning = true;
    struct Galaxy theGalaxy;
    theGalaxy = gameIntro();
    while (gameRunning) {                                                       // Main game loop:
        getCommand(&theGalaxy);
        gameRunning = gameEnd(&theGalaxy);
    }
    return 0;                                                                   // indicates normal return from main; i.e. no errors
}
///////////////// END MAIN /////////////////
////////////////////////////////////////////

//function definitions:
struct Galaxy createGalaxy(void) {
    srand((int)time(0));                                                        // Seed rand function with current time
    int numStarbases = GAME_NUM_STARBASES;
    int numKlingons = 0;
    int numStars = GAME_NUM_STARS;
    int klingonsInQuad[9][9] = {0};
    int a = 0;
    struct Galaxy _galaxy;
    int sbCount = 0;
    int kCount = 0;

    // First initialize Galaxy with blank spaces to overwrite anything that was already there.
    for (int i = 0; i <= 8; ++i) {
        for (int j = 0; j <= 8; ++j) {
            for (int k = 0; k <= 8; ++k) {
                for (int l = 0; l <= 8; ++l) {
                    _galaxy.coordinates[i][j][k][l] = ' ';
                }
            }
        }
    }

    // Place starbases randomly in the galaxy:
    for (int i = 0; i<numStarbases; ++i) {
        struct Starbase newSB;                                                  // create blank starbase struct
        bool notPlaced = true;                                                  // Assign random position:
        while (notPlaced) {                                                     // repeat until valid location found
            int w = rand()%8 + 1;                                               // Generate and save random number coordinates:
            int x = rand()%8 + 1;
            int y = rand()%8 + 1;
            int z = rand()%8 + 1;
            if (_galaxy.coordinates[w][x][y][z] == ' ') {                       // Check that the spot is empty
                newSB.position[0] = w;                                          // Save location in starbase.position array
                newSB.position[1] = x;
                newSB.position[2] = y;
                newSB.position[3] = z;
                newSB.energy = 1000;
                notPlaced = false;
            }
        }
        _galaxy.starbases[i] = newSB;                                           // save completed starbase in galaxy.starbases array
        sbCount++;
    }

    //Klingons are spawning, however going over limit per quadrant
    while (numKlingons < GAME_NUM_KLINGONS - 2) { //Produces max 26, must be less than 24 so if producing 3 on last loop, then max will be 26
        int tempK;
        struct Klingon newKlingon;
        bool notPlaced = true;
        while (notPlaced) {                                                     // Assign random position:
            int w = (int)ceil(RND1()*8.0);                                      //Made all the coordinates random again, iterating exceeds max number allowed due to the while loop already accounting for that
            int x = (int)ceil(RND1()*8.0);

            int chance = rand()%101;                                            // Generate random percentage
            tempK = klingonsInQuad[w][x];                                       // temp save for number of klingons in current quadrant

            if (chance > 0.95) {
            if(klingonsInQuad[w][x] < 1) {                                      // Checks if current quadrant has the less than the max number of klingons allowed
            klingonsInQuad[w][x] = 3;
            numKlingons = numKlingons + 3;
            }
            else {                                                              // If number does not permit adding more to current quadrant then skips to next randomly selected quadrant
            klingonsInQuad[w][x] = 0;
            }
            }
            else if (chance > 0.90) {
            if(klingonsInQuad[w][x] < 2) {
            klingonsInQuad[w][x] = 2;
            numKlingons = numKlingons + 2;
            }
            else {
            klingonsInQuad[w][x] = 0;
            }
            }
            else if (chance > 0.80) {
            if(klingonsInQuad[w][x] < 3) {
            klingonsInQuad[w][x] = 1;
            numKlingons = numKlingons + 1;
            }
            else {
            klingonsInQuad[w][x] = 0;
            }
            }
            else {
            klingonsInQuad[w][x] = 0;
            }

            for (int j = 0; j < klingonsInQuad[w][x]; ++j) {
                int y = rand()%8 + 1;                                           // Use rand() to choose a random sector in current quadrant
                int z = rand()%8 + 1;
                if (_galaxy.coordinates[w][x][y][z] == ' ') {                   // Check that spot is empty
                    newKlingon.position[0] = w;                                 // Save location in klingons.position array
                    newKlingon.position[1] = x;
                    newKlingon.position[2] = y;
                    newKlingon.position[3] = z;
                    newKlingon.energy = 100 + (rand()%201);                     // randomly from 100-300
                    notPlaced = false;
                    _galaxy.klingons[a] = newKlingon;                           // save completed klingon in galaxy.klingons array
                    kCount++;
                    ++a;                                                        //save is now in the loop, so we can save the individual if more than 1 is created
                }
            }
            klingonsInQuad[w][x] = klingonsInQuad[w][x] + tempK;                //After check reinstates number in quadrant and adds any added klingons
        }
    }

    // Place stars randomly in the galaxy:
    for (int i = 0; i < numStars; ++i) {                                        // Generate and save random number coordinates
        bool notPlaced = true;
        while (notPlaced) {
            int w = (rand()%8) + 1;
            int x = (rand()%8) + 1;
            int y = (rand()%8) + 1;
            int z = (rand()%8) + 1;
            if (_galaxy.coordinates[w][x][y][z] == ' ') {
                _galaxy.coordinates[w][x][y][z] = '*';                          // '*' is a star
                notPlaced = false;
            }
        }
    }

    _galaxy.glblDEBUG = false;
    _galaxy.klingonCount = kCount;
    _galaxy.starbaseCount = sbCount;
    _galaxy.stardate = GAME_INITIAL_STARDATE;

    return _galaxy;
};

struct Enterprise createEnterprise(struct Galaxy *refGalaxy) {
    struct Enterprise _enterprise;
    _enterprise.isDestroyed = false;
    // Set Enterprise system names
    strcpy(_enterprise.sysNames[0],"WARP ENGINES");
    strcpy(_enterprise.sysNames[1],"SHORT RANGE SENSORS");
    strcpy(_enterprise.sysNames[2],"LONG RANGE SENSORS");
    strcpy(_enterprise.sysNames[3],"PHASER CONTROL");
    strcpy(_enterprise.sysNames[4],"PHOTON TUBES");
    strcpy(_enterprise.sysNames[5],"DAMAGE CONTROL");
    strcpy(_enterprise.sysNames[6],"SHIELD CONTROL");
    strcpy(_enterprise.sysNames[7],"LIBRARY COMPUTER");
    // Place Enterprise in the middle of the galaxy
        _enterprise.position[0] = 5;
        _enterprise.position[1] = 6;
        _enterprise.position[2] = 4;
        _enterprise.position[3] = 1;
    // Mark all quadrants as 0 (not explored yet) except for current quadrant
    for (int i = 1; i <= 8; i++) {
        for (int k = 1; k <= 8; k++) {
            _enterprise.explored[i][k] = 0;
        }
    }
    _enterprise.explored[_enterprise.position[0]][_enterprise.position[1]] = 1; // Mark current quadrant as Explored

    // Set up initial conditions for enterprise
    for (int i = 0; i < 8; ++i) { _enterprise.sysDamage[i] = 0; }
    _enterprise.energy = GAME_ENERGY_MAX;
    _enterprise.shields = 0;
    _enterprise.torpedoes = GAME_TORPEDOES_MAX;
    _enterprise.condition = RED;
    _enterprise.isDestroyed = false;
    _enterprise.userQuit = false;
    return _enterprise;
};

struct Galaxy gameIntro(void) {                                                 // display splash ASCII, call new game generation functions
  printf("\n                                    ,------*------,\n");
  printf("                    ,-------------   '---  ------'\n");
  printf("                     '-------- --'      / /\n");
  printf("                         ,---' '-------/ /--,\n");
  printf("                          '----------------'\n\n");
  printf("                    THE USS ENTERPRISE --- NCC-1701\n\n");
  printf("ENTER 'M' TO VIEW THE GAME MANUAL BEFORE STARTING GAME\nOTHERWISE, PRESS ENTER TO BEGIN GAME\n");

    char input[MAX_INPUT_LENGTH];
    fgets(input, MAX_INPUT_LENGTH, stdin);
    strtrim(input, strlen(input));                                              // Trim any leading spaces off the input
    input[1] = '\0';                                                            // Truncate the command string to 3 characters
    strToUpper(input, strlen(input));                                           // convert to uppercase
    if (input[0] == 'M') { displayManual(); }

  struct Galaxy newGalaxy;
  newGalaxy = createGalaxy();
  newGalaxy.enterprise = createEnterprise(&newGalaxy);
    printf("YOUR ORDERS ARE AS FOLLOWS :\n");
    printf("\tDESTROY THE %d KLINGON WARSHIPS WHICH HAVE INVADED\n", newGalaxy.klingonCount);
    printf("\tTHE GALAXY BEFORE THEY CAN ATTACK FEDERATION HEADQUARTERS\n");
    printf("\tON STARDATE %4.0f. THIS GIVES YOU %d DAYS. THERE ARE\n", (newGalaxy.stardate)+GAME_STARDATE_LIMIT, GAME_STARDATE_LIMIT);
    printf("\t%d STARBASES IN THE GALAXY FOR RESUPPLYING YOUR SHIP.\n", newGalaxy.starbaseCount);
  exeSRS(&newGalaxy);

  return newGalaxy;
}

bool gameEnd(struct Galaxy *refGalaxy) {
    bool gameContinue = true;
    double stardatesLeft = ((GAME_INITIAL_STARDATE + GAME_STARDATE_LIMIT) - (*refGalaxy).stardate);
    if ((*refGalaxy).enterprise.isDestroyed == true) {                          //check if Enterprise destroyed
        gameContinue = false;                                                   //set gameContinue to false until the user says otherwise
        printf("THE ENTERPRISE HAS BEEN DESTROYED. THE FEDERATION WILL BE CONQUERED\n");
        printf("THERE WERE %d KLINGON BATTLE CRUISERS LEFT AT THE END OF YOUR MISSION.\n", (*refGalaxy).klingonCount);
    } else if (stardatesLeft <= 0) {                                            //check stardate
        gameContinue = false;
        printf("IT IS STARDATE %.0f.\n", (*refGalaxy).stardate);
        printf("THERE WERE %d KLINGON BATTLE CRUISERS LEFT AT THE END OF YOUR MISSION.\n", (*refGalaxy).klingonCount);
    } else if ((*refGalaxy).starbaseCount <= 0) {                               //check number of Starbases
        printf("YOUR STUPIDITY HAS LEFT YOU ON YOUR OWN IN THE GALAXY -- YOU HAVE NO STARBASES LEFT!\n\n");
    } else if(getTotalEnergy(&((*refGalaxy).enterprise)) <= 10) {               // check energy
        gameContinue = false;
        printf("** FATAL ERROR **\nYOU'VE JUST STRANDED YOUR SHIP IN SPACE.\nYOU HAVE INSUFFICIENT MANEUVERING ENERGY, AND SHIELD CONTROL IS PRESENTLY INCAPABLE OF CROSS-CIRCUITING TO ENGINE ROOM!!\n\n");
    } else if ((*refGalaxy).klingonCount <= 0) {                                //check number of Klingons
        gameContinue = false;
        printf("CONGRATULATIONS, CAPTAIN. THE LAST KLINGON BATTLE CRUISER\n");
        printf("MENACING THE FEDERATION HAS BEEN DESTROYED\n");
        double efficiency = 1000 * pow((GAME_NUM_KLINGONS/stardatesLeft), 2);
        printf("YOUR EFFICIENCY RATING IS: %.2f\n", efficiency);
    } else if ((*refGalaxy).enterprise.userQuit) {                              // If user chooses Quit
        gameContinue = false;
    } else {
        gameContinue = true;
    }

    if (!gameContinue) {
        char getInput[100] = {0};
        printf("THE FEDERATION IS IN NEED OF A NEW STARSHIP COMMANDER\n");
        printf("FOR A SIMILAR MISSION -- IF THERE IS A VOLUNTEER\n");
        printf("LET HIM STEP FORWARD AND ENTER 'AYE' ");

        fgets(getInput, 100, stdin);
        getInput[strlen(getInput) - 1] = '\0';

        // Check if user starts a new game
        if((strcmp(getInput, "AYE") == 0) || (strcmp(getInput, "Aye") == 0)|| (strcmp(getInput, "aye") == 0)) {
            (*refGalaxy).enterprise.userQuit = false;
            struct Galaxy newGalaxy = gameIntro();
            (*refGalaxy) = newGalaxy;
            return true;
        } else {
            printf("\nGame ended\n");
            (*refGalaxy).enterprise.userQuit = true;
            return false;
        }
    }
    else { return true;}
}

void getCommand(struct Galaxy *refGalaxy) {
    char cmdString[MAX_INPUT_LENGTH];
    printf("COMMAND: ");                                                        // Prompt user for command input
    fgets(cmdString, MAX_INPUT_LENGTH, stdin);
    strtrim(cmdString, strlen(cmdString));                                      // Trim any leading spaces off the input
    cmdString[3] = '\0';                                                        // Truncate the command string to 3 characters
    strToUpper(cmdString, strlen(cmdString));                                   // convert to uppercase
    if (strcmp(cmdString, "NAV") == 0) { exeNAV(refGalaxy);                     // check for possible commands and execute if found
    } else if (strcmp(cmdString, "SRS") == 0) { exeSRS(refGalaxy);
    } else if (strcmp(cmdString, "LRS") == 0) { exeLRS(refGalaxy);
    } else if (strcmp(cmdString, "DAM") == 0) { exeDAM(refGalaxy);
    } else if (strcmp(cmdString, "COM") == 0) { exeCOM(refGalaxy);
    } else if (strcmp(cmdString, "PHA") == 0) { exePHA(refGalaxy);
    } else if (strcmp(cmdString, "TOR") == 0) { exeTOR(refGalaxy);
    } else if (strcmp(cmdString, "SHE") == 0) { exeSHE(refGalaxy);
    } else if (strcmp(cmdString, "MAN") == 0) { displayManual();
    } else if (strcmp(cmdString, "XXX") == 0) { (*refGalaxy).enterprise.userQuit = true;     // If user chooses resign, trigger the 'userQuit' gameVital
    } else if (strcmp(cmdString, "DBG") == 0) { exeDBG(refGalaxy);                           // DEBUG: 'DBG' == a "secret" debug option showing game data and enabling '+' commands below:
  } else if ((strcmp(cmdString, "+LR") == 0)&&((*refGalaxy).glblDEBUG)) { exeSLR(refGalaxy); // DEBUG: '+LR' == hidden "super long range" sensors: calls LRS for all quadrants
    } else if ((strcmp(cmdString, "+EN") == 0)&&((*refGalaxy).glblDEBUG)) {
        (*refGalaxy).enterprise.energy += 1000;
        printf("ENTERPRISE ENERGY INCREASED BY 1000 UNITS.\n");
        printf("ENERGY LEVELS NOW AT %d UNITS.\n\n", (*refGalaxy).enterprise.energy);
    } else if ((strcmp(cmdString, "+TP") == 0)&&((*refGalaxy).glblDEBUG)) {
        (*refGalaxy).enterprise.torpedoes += 10;
        printf("10 TORPEDOES ADDED TO ENTERPRISE INVENTORY.\n");
        printf("%d TORPEDOES TOTAL NOW CARRIED.\n\n", (*refGalaxy).enterprise.torpedoes);
    } else if ((strcmp(cmdString, "+RP") == 0)&&((*refGalaxy).glblDEBUG)) {
        (*refGalaxy).enterprise.sysDamage[0] = 1.0;
        (*refGalaxy).enterprise.sysDamage[1] = 1.0;
        (*refGalaxy).enterprise.sysDamage[2] = 1.0;
        (*refGalaxy).enterprise.sysDamage[3] = 1.0;
        (*refGalaxy).enterprise.sysDamage[4] = 1.0;
        (*refGalaxy).enterprise.sysDamage[5] = 1.0;
        (*refGalaxy).enterprise.sysDamage[6] = 1.0;
        (*refGalaxy).enterprise.sysDamage[7] = 1.0;
        printf("ALL ENTERPRISE SYSTEMS RESTORED TO FULL POWER.\n\n");
    } else if ((strcmp(cmdString, "+OV") == 0)&&((*refGalaxy).glblDEBUG)) {
        (*refGalaxy).enterprise.sysDamage[0] += 2.0;
        (*refGalaxy).enterprise.sysDamage[1] += 2.0;
        (*refGalaxy).enterprise.sysDamage[2] += 2.0;
        (*refGalaxy).enterprise.sysDamage[3] += 2.0;
        (*refGalaxy).enterprise.sysDamage[4] += 2.0;
        (*refGalaxy).enterprise.sysDamage[5] += 2.0;
        (*refGalaxy).enterprise.sysDamage[6] += 2.0;
        (*refGalaxy).enterprise.sysDamage[7] += 2.0;
        printf("POWER TO ALL ENTERPRISE SYSTEMS INCREASED ABOVE NORMAL.\n\n");
    } else { exeHLP(refGalaxy); }                                               // If no other command recognized, print help menu
}

void exeHLP(struct Galaxy* refGalaxy) {
    printf("Enter one of the following:\n");
    printf("  NAV  (TO SET COURSE)\n");
    printf("  SRS  (FOR SHORT RANGE SENSOR SCAN)\n");
    printf("  LRS  (FOR LONG RANGE SENSOR SCAN)\n");
    printf("  PHA  (TO FIRE PHASERS)\n");
    printf("  TOR  (TO FIRE PHOTON TORPEDOS)\n");
    printf("  SHE  (TO RAISE OR LOWER SHIELDS)\n");
    printf("  DAM  (FOR DAMAGE CONTROL REPORTS)\n");
    printf("  COM  (TO CALL ON LIBRARY COMPUTER)\n");
    printf("  MAN  (TO VIEW GAME MANUAL)\n");
    printf("  XXX  (TO RESIGN YOUR COMMAND)\n\n");
    return;
}

void exeNAV(struct Galaxy* refGalaxy) {
    char cmdString[MAX_INPUT_LENGTH];
    double setCourse = 0.0;
    double setWarp = 0.0;
    printf("COURSE (1-9) ");                                                    // Prompt user for course info
    fgets(cmdString, MAX_INPUT_LENGTH, stdin);
    int cmdLen = strlen(cmdString);
    remNL(cmdString, cmdLen);
    strtrim(cmdString, cmdLen);                                                 // Trim any leading spaces off the input
    cmdLen = strlen(cmdString);
    setCourse = atof(cmdString);
    if ((setCourse < 1.0) || (setCourse > 9.0)) {
        printf("LT. SULU REPORTS, 'INCORRECT COURSE DATA, SIR!'\n");
    } else {
        double maxWarp = 9.0;
        double enExp = 0.0;
        if ((*refGalaxy).enterprise.sysDamage[0] < 0) { maxWarp = 0.2; }
        printf("WARP FACTOR (0-%.1f) ", maxWarp);
        fgets(cmdString, MAX_INPUT_LENGTH, stdin);
        cmdLen = strlen(cmdString);
        remNL(cmdString, cmdLen);
        strtrim(cmdString, cmdLen);                                             // Trim any leading spaces off the input
        setWarp = atof(cmdString);
        enExp = floor(setWarp * 8.0 + 0.5) + 10.0;
        if ((setWarp < 0) || (setWarp > 9.0)) {     printf("CHIEF ENGINEER SCOTT REPORTS, 'THE ENGINES WON'T TAKE WARP %.1f!'\n", setWarp);
        } else if (setWarp > maxWarp) {             printf("WARP ENGINES ARE DAMAGED. MAXIMUM SPEED = WARP %.1f!'\n", maxWarp);
        } else if (enExp > (*refGalaxy).enterprise.energy + (*refGalaxy).enterprise.shields) {
            printf("ENGINEERING REPORTS 'INSUFFICIENT ENERGY AVAILABLE FOR MANEUVERING AT WARP %.1f!'\n", setWarp);
            return;
        } else {
            if ((enExp > (*refGalaxy).enterprise.energy) && (enExp < ((*refGalaxy).enterprise.energy + (*refGalaxy).enterprise.shields))) {
                printf("SHIELD CONTROL SUPPLIES ENERGY TO COMPLETE THE MANEUVER.\n"); // if (energy needed > free energy), but <= (energy + shields), allow warp
            }
            if (KlingonsInQuadrant(refGalaxy, (*refGalaxy).enterprise.position) > 0) {
                KlingonsFire(refGalaxy);
                KlingonsMove(refGalaxy);
            }
            int curPos[4] = {(*refGalaxy).enterprise.position[0], (*refGalaxy).enterprise.position[1], (*refGalaxy).enterprise.position[2], (*refGalaxy).enterprise.position[3]};
            int newPos[4] = {(*refGalaxy).enterprise.position[0], (*refGalaxy).enterprise.position[1], (*refGalaxy).enterprise.position[2], (*refGalaxy).enterprise.position[3]};
            double dest = checkObstacles(curPos,setCourse,setWarp,refGalaxy);
            if (dest < setWarp) {
                setDest(curPos, setCourse, dest, newPos);
                (*refGalaxy).enterprise.position[0] = newPos[0];
                (*refGalaxy).enterprise.position[1] = newPos[1];
                (*refGalaxy).enterprise.position[2] = newPos[2];
                (*refGalaxy).enterprise.position[3] = newPos[3];
                printf("WARP ENGINES SHUT DOWN AT SECTOR: %d, %d DUE TO BAD NAVIGATION\n\n", newPos[2], newPos[3]);
            } else {
                setDest(curPos, setCourse, setWarp, newPos);
                (*refGalaxy).enterprise.position[0] = newPos[0];
                (*refGalaxy).enterprise.position[1] = newPos[1];
                (*refGalaxy).enterprise.position[2] = newPos[2];
                (*refGalaxy).enterprise.position[3] = newPos[3];
            }
            (*refGalaxy).enterprise.energy -= floor(setWarp * 8.0 + 0.5) + 10.0;   // subtract expended energy
            if ((*refGalaxy).enterprise.energy < 0) {                              // if warp was allowed, but energy expended was greater than free amount
                (*refGalaxy).enterprise.shields += (*refGalaxy).enterprise.energy; // subtract excess energy from shields
                (*refGalaxy).enterprise.energy = 0;                                // set energy to 0
            }
            (*refGalaxy).stardate += (double)minnum((double)1.0, setWarp);
            updateCondition(refGalaxy);
            if ((*refGalaxy).enterprise.condition == DOCKED) {                      // if docked, replenish energy (less amound in shields)
                (*refGalaxy).enterprise.energy = (GAME_ENERGY_MAX -  (*refGalaxy).enterprise.shields);
                (*refGalaxy).enterprise.torpedoes = GAME_TORPEDOES_MAX;             // and replace spent torpedoes
            }
            exeSRS(refGalaxy);
        }
    }
    return;
}

void exeSRS(struct Galaxy* refGalaxy) {
    // Check if SRS sensors are working
    if ((*refGalaxy).enterprise.sysDamage[1] < 0) {
        printf("SHORT RANGE SENSORS ARE OUT\n");
        return;
    } else {
        updateCondition(refGalaxy);
        printf("\n");
        if (KlingonsInQuadrant(refGalaxy,(*refGalaxy).enterprise.position) > 0) { printf("\n\tCOMBAT AREA\tCONDITION RED\n"); }
        if ((*refGalaxy).enterprise.shields <= 200) { printf("\t   SHIELDS DANGEROUSLY LOW\n"); }
    }
    printf("-----------------------------------------\n");
    int w = (*refGalaxy).enterprise.position[0];
    int x = (*refGalaxy).enterprise.position[1];
    int column = 0;
    char debris[13] = {"`~^-=:;\"\',.?"};                                        // string which can be referenced randomly by using [rand()%12]

    char quadrantStr[8][38];
    for (int i=0; i<8; i++) {
        for (int j=0; j<38; j++) {
            if (((j+2)%4 == 0) && ((j+2)/4 <= 8)) {
                quadrantStr[i][j] = ((*refGalaxy).coordinates[w][x][i+1][(int)floor((j+2)/4.0)]);
            } else if (quadrantStr[i][j-1] == 'x') {                                // if wreck marker found, surround it with random "debris" characters
                quadrantStr[i][j] = debris[rand()%12];
                quadrantStr[i][j-2] = debris[rand()%12];
            } else {
                quadrantStr[i][j] = ' ';
            }
        }
    }
    for (int i=0; i<GAME_NUM_KLINGONS; i++) {
        if (((*refGalaxy).klingons[i].position[0] == w) &&
            ((*refGalaxy).klingons[i].position[1] == x) &&
            ((*refGalaxy).klingons[i].energy > 0)) {
            column = (((*refGalaxy).klingons[i].position[3] * 4) - 2);
            quadrantStr[(*refGalaxy).klingons[i].position[2]-1][column-1] = '+';
            quadrantStr[(*refGalaxy).klingons[i].position[2]-1][column] = 'K';
            quadrantStr[(*refGalaxy).klingons[i].position[2]-1][column+1] = '+';
            }
    }
    for (int i=0; i<GAME_NUM_STARBASES; i++) {
        if (((*refGalaxy).starbases[i].position[0] == w) &&
            ((*refGalaxy).starbases[i].position[1] == x) &&
            ((*refGalaxy).starbases[i].energy > 0)) {
            column = (((*refGalaxy).starbases[i].position[3] * 4) - 2);
            quadrantStr[(*refGalaxy).starbases[i].position[2]-1][column-1] = '>';
            quadrantStr[(*refGalaxy).starbases[i].position[2]-1][column] = 'B';
            quadrantStr[(*refGalaxy).starbases[i].position[2]-1][column+1] = '<';
            }
    }
    column = (((*refGalaxy).enterprise.position[3] * 4) - 2);
    quadrantStr[(*refGalaxy).enterprise.position[2]-1][column-1] = '<';
    quadrantStr[(*refGalaxy).enterprise.position[2]-1][column] = 'E';
    quadrantStr[(*refGalaxy).enterprise.position[2]-1][column+1] = '>';

    for (int y = 1; y<=8; y++) {                                                // iterate through quadrant line-by-line, checking for entities
        printf("  ");
        for (int z=0; z<38; z++) {
            printf("%c", quadrantStr[y-1][z]);                                  // draw canvas line
        }
        switch(y) {
            char cond[10];
            case 1:                                                             // print additional SRS readout info, according to current line
                printf("\tSTARDATE:\t\t%.1f\n", ((*refGalaxy).stardate));
                break;
            case 2:
                if ((*refGalaxy).enterprise.condition == DOCKED)        { strcpy(cond,"DOCKED");    }
                else if ((*refGalaxy).enterprise.condition == RED)      { strcpy(cond,"*RED*");     }
                else if ((*refGalaxy).enterprise.condition == YELLOW)   { strcpy(cond,"*YELLOW*");  }
                else                                                    { strcpy(cond,"GREEN");     }
                printf("\tCONDITION:\t\t%s\n", cond);
                break;
            case 3:
                printf("\tQUADRANT:\t\t%d,%d\n", (*refGalaxy).enterprise.position[0], (*refGalaxy).enterprise.position[1]);
                break;
            case 4:
                printf("\tSECTOR:\t\t\t%d,%d\n", (*refGalaxy).enterprise.position[2], (*refGalaxy).enterprise.position[3]);
                break;
            case 5:
                printf("\tPHOTON TORPEDOES:\t%d\n", (*refGalaxy).enterprise.torpedoes);
                break;
            case 6:
                printf("\tTOTAL ENERGY:\t\t%d\n", (*refGalaxy).enterprise.energy + (*refGalaxy).enterprise.shields);
                break;
            case 7:
                printf("\tSHIELDS:\t\t%d\n", (*refGalaxy).enterprise.shields);
                break;
            case 8:
                printf("\tKLINGONS REMAINING:\t%d\n", (*refGalaxy).klingonCount);
                break;
            default:
                break;
        }
    }
    printf("-----------------------------------------\n");
    return;
}

void exeLRS(struct Galaxy* refGalaxy) {
    if((*refGalaxy).enterprise.sysDamage[2] < 0) {
        printf("LONG RANGE SENSORS ARE INOPERABLE.\n");
        return;
    }
    else {
        printf("LONG RANGE SCAN FOR QUADRANT %d, %d\n", (*refGalaxy).enterprise.position[0], (*refGalaxy).enterprise.position[1]);
        printf("  ---------------------    \n");
        for (int w = (*refGalaxy).enterprise.position[0]-1; w <= (*refGalaxy).enterprise.position[0]+1; w++) {
            for (int x = (*refGalaxy).enterprise.position[1]-1; x <= (*refGalaxy).enterprise.position[1]+1; x++) {
                // Mark scanned quadrants as Explored
                (*refGalaxy).enterprise.explored[w][x] = 1;
                int starCounter = 0;
                for (int y = 0; y < 8; y++) {
                    for (int z = 0; z < 8; z++) {
                        if ((*refGalaxy).coordinates[w][x][y][z] == '*') { starCounter++; }
                    }
                }
                int thisQuad[2] = {w, x};
                printf(" :  %d%d%d", KlingonsInQuadrant(refGalaxy,thisQuad), StarbasesInQuadrant(refGalaxy,thisQuad), starCounter);
            }
            printf("  :\n  ---------------------    \n");
        }
    }
    return;
}

void exeSLR(struct Galaxy* refGalaxy) {
    printf("LONG RANGE SCAN FOR ALL QUADRANTS:\n");
    printf("       1      2      3      4      5      6      7      8\n");
    printf("    --------------------------------------------------------    \n");
    for (int w = 1; w <=8; w++) {
        printf(" %d", w);
        for (int x = 1; x <=8; x++) {
            int starCounter = 0;
            for (int y = 1; y <= 8; y++) {
                for (int z = 1; z <= 8; z++) {
                    if ((*refGalaxy).coordinates[w][x][y][z] == '*') { starCounter++; }
                }
            }
            int thisQuad[2] = {w, x};
            printf(" :  %d%d%d", KlingonsInQuadrant(refGalaxy,thisQuad), StarbasesInQuadrant(refGalaxy,thisQuad), starCounter);
        }
        printf("  :\n    --------------------------------------------------------    \n");
    }
    printf("       I     II     III    IV      I     II     III    IV\n\n");
    return;
}

void exeDAM(struct Galaxy* refGalaxy) {
    char input;
    int damageCounter = 0;
    if((*refGalaxy).enterprise.sysDamage[5] >= 0) {
        printf("\nDEVICE\t\t\t  STATE OF REPAIR\n");
        printf("WARP ENGINES\t\t\t%.1f\n", (*refGalaxy).enterprise.sysDamage[0]);
        printf("SHORT RANGE SENSORS\t\t%.1f\n", (*refGalaxy).enterprise.sysDamage[1]);
        printf("LONG RANGE SENSORS\t\t%.1f\n", (*refGalaxy).enterprise.sysDamage[2]);
        printf("PHASER CONTROL\t\t\t%.1f\n", (*refGalaxy).enterprise.sysDamage[3]);
        printf("PHOTON TUBES\t\t\t%.1f\n", (*refGalaxy).enterprise.sysDamage[4]);
        printf("DAMAGE CONTROL\t\t\t%.1f\n", (*refGalaxy).enterprise.sysDamage[5]);
        printf("SHIELD CONTROL\t\t\t%.1f\n", (*refGalaxy).enterprise.sysDamage[6]);
        printf("LIBRARY - COMPUTER\t\t%.1f\n", (*refGalaxy).enterprise.sysDamage[7]);
        printf("\n\n");
    }
    else {
        printf("DAMAGE CONTROL REPORT NOT AVAILABLE\n");
        return;
    }
    for (int i = 0; i < 8; i++) {
        // Check for damage on each of 8 items in enterprise.damage array
        if ((*refGalaxy).enterprise.sysDamage[i] < 0) {
            damageCounter++;    // Increment damageCounter if any part is damaged
        }
    }
    if (damageCounter <= 0) {       // if no parts of Enterprise are damaged
        return;
    }
    printf("\nTECHNICIANS STANDING BY TO EFFECT REPAIRS TO YOUR SHIP\n");
    printf("ESTIMATED TIME TO REPAIR : %d STARDATES\n", damageCounter);
    printf("WILL YOU AUTHORIZE THE REPAIR ORDER? (Y/N) ");
    scanf("%c", &input);
    getchar();
    if(input != 'Y' || input != 'y') {
        return;
    }
    // Cycle through each item in enterprise.damage array
    for(int i = 0; i < 8; ++i) {
        // if any member in the array < 0, set it equal to 0 and decrement stardate counter
        if((*refGalaxy).enterprise.sysDamage[i] < 0) {
            (*refGalaxy).enterprise.sysDamage[i] = 0;
            (*refGalaxy).stardate--;
        }
    }
    return;
}

void exeCOM(struct Galaxy* refGalaxy) {
    printf("Computer active and awaiting command: ");
    char command = getchar();
    getchar();                                                                  // Extra getchar to get rid of the newline

    // Switch branching statement to execute each possible computer command:
    switch(command) {
        case ('0'):
            printf("\n   ----   ----   ----   ----   ----   ----   ----   ----\n");
            // Count klingons, starbases, and stars in Explored quadrants:
            int klingonCounter = 0;
            int starbaseCounter = 0;
            int starCounter = 0;
            for (int w = 0; w < 8; w++) {
                for (int x = 0; x < 8; x++) {
                    if ((*refGalaxy).enterprise.explored[w][x] == 1) {
                        for (int y = 0; y < 8; y++) {
                        for (int z = 0; z < 8; z++) {
                            if ((*refGalaxy).coordinates[w][x][y][z] == 'K') {
                                klingonCounter = klingonCounter + 1;
                            }
                            if ((*refGalaxy).coordinates[w][x][y][z] == 'S') {
                                starbaseCounter = starbaseCounter + 1;
                            }
                            if ((*refGalaxy).coordinates[w][x][y][z] == '*') {
                                starCounter = starCounter + 1;
                            }
                        }
                        }
                        // Print quantities of elements in explored quadrants:
                        printf("    %d%d%d", klingonCounter, starbaseCounter, starCounter);
                        klingonCounter = 0;         // Set counters back to 0 before counting next quadrant
                        starbaseCounter = 0;
                        starCounter = 0;
                        }
                    // Otherwise print *** for unexplored quadrants:
                    else {
                        printf("    ***");
                    }
                }
                printf("\n   ----   ----   ----   ----   ----   ----   ----   ----\n");
            }
            break;
        case ('1'):
            printf("\nSTATUS REPORT:\n");
            printf("KLINGONS LEFT: %d\n", (*refGalaxy).klingonCount);
            printf("MISSION MUST BE COMPLETED IN %d STARDATES\n", (int)(*refGalaxy).stardate);
            printf("THE FEDERATION IS MAINTAINING %d STARBASES IN THE GALAXY\n", (*refGalaxy).starbaseCount);
            printf("\nDEVICE           STATE OF REPAIR\n");
            printf("WARP ENGINES         %.3f\n", (*refGalaxy).enterprise.sysDamage[0]);
            printf("SHORT RANGE SENSORS  %.3f\n", (*refGalaxy).enterprise.sysDamage[1]);
            printf("LONG RANGE SENSORS   %.3f\n", (*refGalaxy).enterprise.sysDamage[2]);
            printf("PHASER CONTROL       %.3f\n", (*refGalaxy).enterprise.sysDamage[3]);
            printf("PHOTON TUBES         %.3f\n", (*refGalaxy).enterprise.sysDamage[4]);
            printf("DAMAGE CONTROL       %.3f\n", (*refGalaxy).enterprise.sysDamage[5]);
            printf("SHIELD CONTROL       %.3f\n", (*refGalaxy).enterprise.sysDamage[6]);
            printf("LIBRARY COMPUTER     %.3f\n", (*refGalaxy).enterprise.sysDamage[7]);
            printf("\n\n");
            break;
        case ('2'):
          printf("FROM ENTERPRISE TO KLINGON BATTLE CRUISER\n");
          int iniVals[2];
          int finVals[2];
          iniVals[0] = (*refGalaxy).enterprise.position[0]*8+(*refGalaxy).enterprise.position[2];
          iniVals[1] = (*refGalaxy).enterprise.position[1]*8+(*refGalaxy).enterprise.position[3];
          double dist = getDist(refGalaxy, finVals);
          double yDiff = (double)iniVals[0] - (double)finVals[0];
          double xDiff = (double)iniVals[1] - (double)finVals[1];
          double dir = getDirection(yDiff, xDiff);
          printf("DIRECTION = %f\n", dist);
          printf("DISTANCE = %f\n", dir);

        break;
        case ('3'):
            // Initialize starbaseCounter to check current quadrant for starbases
            starbaseCounter = 0;
            // Check enterprise position quadrant against the other items in this sector
            int w = (*refGalaxy).enterprise.position[0];
            int x = (*refGalaxy).enterprise.position[1];
            for (int y = 0; y < 8; y++) {
                for (int z = 0; z < 8; z++) {
                    if ((*refGalaxy).coordinates[w][x][y][z] == 'S') {
                    starbaseCounter = starbaseCounter + 1;
                    }
                }
            }
            printf("MR. SPOCK REPORTS, ""SENSORS SHOW %d STARBASES IN THIS QUADRANT.""\n", starbaseCounter);
            break;
        case ('4'):
            printf("DIRECTION/DISTANCE CALCULATOR\n");
            // Print current location
            printf("YOU ARE AT QUADRANT %d, %d, SECTOR %d, %d\n", (*refGalaxy).enterprise.position[0], (*refGalaxy).enterprise.position[1], (*refGalaxy).enterprise.position[2], (*refGalaxy).enterprise.position[3]);
            // Ask user for destination coordinates
            printf("PLEASE ENTER \n\tINITIAL COORDINATES(X,Y): \n");
            int iniCoords[2];
            int finCoords[2];
            scanf("%d,%d", &(iniCoords[0]), &(iniCoords[1]));
            printf("\tFINAL COORDINATES(X,Y): \n");
            scanf("%d,%d", &(finCoords[0]), &(finCoords[1]));
            getchar();                                                          // Extra getchar to get rid of newline in input buffer
            // Calculate Distance with Pythagorean Theorem
            double yDist = (double)finCoords[0] - (double)iniCoords[0];
            double xDist = (double)finCoords[1] - (double)iniCoords[1];             printf("Getting direction with dists: (%.2f,%.2f)\n", yDist, xDist);
            double distance = (sqrt(pow(xDist,2) + pow(yDist,2)));
            // Call function to calculate direction
            double direction = getDirection(yDist, xDist);
            // Print distance and direction to screen
            printf("DIRECTION = %.5f\n", direction);
            printf("DISTANCE = %.2f\n", distance);
            break;
        case ('5'):
            printf("                        THE GALAXY\n");
            printf("    1      2      3      4      5      6      7      8\n");
            printf("1           ANTARES          |           SIRIUS\n");
            printf("2           RIGEL            |           DENEB\n");
            printf("3           PROCYON          |           CAPELLA\n");
            printf("4           VEGA             |           BETELGEUSE\n");
            printf("5           CANOPUS          |           ALDEBARAN\n");
            printf("6           ALTAIR           |           REGULUS\n");
            printf("7           SAGITTARIUS      |           ARCTURUS\n");
            printf("8           POLLUX           |           SPICA\n");
            break;
        default:
            printf("UNRECOGNIZED COMMAND. ENTER 0 - 5.\n");
            break;
    }
    return;
}

void exePHA(struct Galaxy* refGalaxy) {
    struct Enterprise* ePr = &((*refGalaxy).enterprise);
    int kCount = KlingonsInQuadrant(refGalaxy, (*ePr).position);
    if((*ePr).sysDamage[3] <= -1) {                                             // If phasers disabled, print message and return
        printf("PHASERS INOPERATIVE\n\n");
        return;
    } else if (kCount < 1) {                                                    // Check whether there are klingons in this quadrant; If there are none, print message and return
        printf("SCIENCE OFFICER SPOCK REPORTS 'SENSORS SHOW NO ENEMY SHIPS\n IN THIS QUADRANT'\n");
        return;
    } else {                                                                    // Otherwise, find klingons and fire phasers
        float dmgMod = 1.0;
        if((*refGalaxy).enterprise.sysDamage[7] < 0) {                          // Check if computer is damaged; if so, display warning and penalize max damage
            printf("COMPUTER FAILURE HAMPERS ACCURACY\n");
            dmgMod = RND1();
        }
        int phasEn;
        printf("PHASERS LOCKED ON TARGET;  ENERGY AVAILABLE =%d\n", (*ePr).energy);
        bool awatingSln = true;
        while (awatingSln) {
          printf("NUMBER OF UNITS TO FIRE? ");
          char cmdString[MAX_INPUT_LENGTH];
          fgets(cmdString, MAX_INPUT_LENGTH, stdin);                            // collect user input
          int cmdLen = strlen(cmdString);
          remNL(cmdString, cmdLen);                                             // remove newline character captured by fgets
          strtrim(cmdString, cmdLen);                                           // Trim any leading spaces off the input
          phasEn = atoi(cmdString);                                             // convert to integer
          if (phasEn < 0)  {
              return;
          } else if (phasEn > (*ePr).energy) {
              printf("ENERGY AVAILABLE =%d\n", (*ePr).energy);
          } else if (phasEn > 0)  {
              awatingSln = false;
          } else { printf("?REENTER (-1 TO CANCEL)/n"); }
        }
        double baseDmg = ((double)phasEn/(double)kCount);                       // calculate shared "base" damage that is applied to each target
        for (int i=1; i<=kCount; i++) {
            struct Klingon* thisK = getNthClosestKlingon(refGalaxy, i);
            int thisDmg = (int)(floor(baseDmg/getDist(refGalaxy,(*thisK).position)) * (2.0+RND1()) * dmgMod);
            if (thisDmg < (((double)(*thisK).energy)*0.15)) {
                printf("SENSORS SHOW NO DAMAGE TO ENEMY AT %d,%d\n", (*thisK).position[2], (*thisK).position[3]);
            } else {
                (*thisK).energy -= thisDmg;
                printf("%d UNIT HIT ON KLINGON AT SECTOR %d,%d\n", thisDmg, (*thisK).position[2], (*thisK).position[3]);
                if ((*thisK).energy <= 0) {
                    printf("*** KLINGON DESTROYED ***\n");
                    // add wreck marker to galaxy:
                    (*refGalaxy).coordinates[(*thisK).position[0]][(*thisK).position[1]][(*thisK).position[2]][(*thisK).position[3]] = 'x';
                } else {
                }

            }
        }
        (*ePr).energy -= phasEn;                                                // deduct expended energy from reserves
    }
    if (KlingonsInQuadrant(refGalaxy, (*ePr).position) > 0) {
      KlingonsFire(refGalaxy);
      KlingonsMove(refGalaxy);
    }
    return;
}

void exeTOR(struct Galaxy* refGalaxy) {
    // If torpedoes are used up
    if ((*refGalaxy).enterprise.torpedoes < 1) {
        printf("ALL PHOTON TORPEDOES EXPENDED.\n");
        return;
    }
    // If photon tubes are damaged
    if ((*refGalaxy).enterprise.sysDamage[4] < 0) {
        printf("PHOTON TUBES ARE NOT OPERATIONAL.\n");
        return;
    }
    struct Enterprise* ePr = &((*refGalaxy).enterprise);
    printf("INPUT PHOTON TORPEDO COURSE (1-9): ");
    float fCourse = 0.0;
    scanf("%f", &fCourse);
    getchar();   // Get extra newline character from input buffer
    if ((fCourse < 1) || (fCourse > 9)) {
        printf("ENSIGN CHEKOV REPORTS, ""INCORRECT COURSE DATA, SIR!""\n");
        return;
    } else if (abs(fCourse - 9) <= 0.1) {                                       // if user chooses 9, set to 1
        fCourse = 1;
    }

    fCourse = round(fCourse * 2.0);                                             // multiply by 2 and round to force 16 (9 == 1) possible directions: [2, 17]
    int iCourse = (int)(fCourse) - 1;                                           // subtract 1 to make range: [1, 16]

    /*
    // 7   5   3
    //   \ | /
    //    \|/
    // 9 --*-- 1
    //    /|\
    //   / | \
    // 11  13 15
    // (1.0 == 9.0)
    // half-directions became even numbers, between the odds (not shown)
    */

    int rMove = 1;                                                              // positive/negative movement on axes
    int cMove = 1;                                                              // (r = row, c = column)
    int rSlopeMod = 1;                                                          // modulo factor to determine how often torpedo moves in each axis
    int cSlopeMod = 1;

    switch (iCourse) {                                                          // based on direction chosen, adjust horizontal/vertical movement properties
        case 1:
            rMove = 0;
            break;
        case 2:
            rMove = -1;
            rSlopeMod = 2;
            break;
        case 3:
            rMove = -1;
            break;
        case 4:
            rMove = -1;
            cSlopeMod = 2;
            break;
        case 5:
            cMove = 0;
            rMove = -1;
            break;
        case 6:
            cMove = -1;
            rMove = -1;
            cSlopeMod = 2;
            break;
        case 7:
            cMove = -1;
            rMove = -1;
            break;
        case 8:
            cMove = -1;
            rMove = -1;
            rSlopeMod = 2;
            break;
        case 9:
            rMove = 0;
            cMove = -1;
            break;
        case 10:
            cMove = -1;
            rSlopeMod = 2;
            break;
        case 11:
            cMove = -1;
            break;
        case 12:
            cMove = -1;
            cSlopeMod = 2;
            break;
        case 13:
            cMove = 0;
            break;
        case 14:
            cSlopeMod = 2;
            break;
        case 15:
            break;
        case 16:
            rSlopeMod = 2;
            break;
        default:
            break;
    }

    // Decrement resources:
    (*ePr).torpedoes--;
    (*ePr).energy -= 2;

    int torpedoPos[2] = { ((*ePr).position[2]), ((*ePr).position[3])};          //start at enterprise's location

    int k = KlingonsInQuadrant(refGalaxy,(*ePr).position);
    int b = StarbasesInQuadrant(refGalaxy,(*ePr).position);

    printf("TORPEDO TRACK:\n");
    for (int i=1; i<9; i++) {                                                   // Iterate through maximum 8 sectors;
        if (i%rSlopeMod == 0) {
            torpedoPos[0] += rMove;                                             //increment vertical position by rMove
        }
        if (i%cSlopeMod == 0) {
            torpedoPos[1] += cMove;                                             //increment horizontal position by cMove
        }
        //test if torpedo has left quadrant:
        if ((torpedoPos[0] < 1) || (torpedoPos[1] < 1) || (torpedoPos[0] > 8) || (torpedoPos[1] > 8)) {
            printf("TORPEDO MISSED!\n");                                        //if so, end track
            if (KlingonsInQuadrant(refGalaxy, (*ePr).position)) {
              KlingonsFire(refGalaxy);
              KlingonsMove(refGalaxy);
            }
            return;
        } else if (k > 0) {                                                     //if there are klingons in quadrant, check for collision
                for (int j=1; j<=k; j++) {
                    struct Klingon* thisK = getNthClosestKlingon(refGalaxy,j);
                    if (((*thisK).position[2] == torpedoPos[0]) && ((*thisK).position[3] == torpedoPos[1])) {
                        (*thisK).energy = -1000;
                        printf("*** KLINGON DESTROYED ***\n");
                        // add wreck marker to galaxy:
                        (*refGalaxy).coordinates[(*thisK).position[0]][(*thisK).position[1]][(*thisK).position[2]][(*thisK).position[3]] = 'x';
                        if (KlingonsInQuadrant(refGalaxy, (*ePr).position)) {
                          KlingonsFire(refGalaxy);
                          KlingonsMove(refGalaxy);
                        }
                        return;
                    }
                }
        } else if (b > 0) {                                                     //if there are starbases in quadrant, check for collision
                struct Starbase* thisB = getClosestStarbase(refGalaxy);
                if (((*thisB).position[2] == torpedoPos[0]) && ((*thisB).position[3] == torpedoPos[1])) {
                    (*thisB).energy = -1000;
                    printf("*** STARBASE DESTROYED ***\n");
                    // add wreck marker to galaxy:
                    (*refGalaxy).coordinates[(*thisB).position[0]][(*thisB).position[1]][(*thisB).position[2]][(*thisB).position[3]] = 'x';
                    (*refGalaxy).starbasesDestroyed++;
                    if ((*refGalaxy).starbasesDestroyed > 1) {
                        printf("THAT DOES IT, CAPTAIN!! YOU ARE HEREBY RELIEVED OF COMMAND AND SENTENCED TO\n99 STARDATES AT HARD LABOR ON CYGNUS 12!!\n\n");
                        (*ePr).userQuit = true;
                        return;
                    } else {
                        printf("STARFLEET COMMAND REVIEWING YOUR RECORD TO CONSIDER COURT MARTIAL!\n");
                        if (KlingonsInQuadrant(refGalaxy, (*ePr).position)) {
                          KlingonsFire(refGalaxy);
                          KlingonsMove(refGalaxy);
                        }
                        return;
                    }
                }
        } else if ((*refGalaxy).coordinates[(*ePr).position[0]][(*ePr).position[1]][torpedoPos[0]][torpedoPos[1]] == '*') {
            printf("STAR AT %d,%d ABSORBED TORPEDO ENERGY.\n", torpedoPos[0], torpedoPos[1]);
            if (KlingonsInQuadrant(refGalaxy, (*ePr).position)) {
              KlingonsFire(refGalaxy);
              KlingonsMove(refGalaxy);
            }
            return;
        } else {
            printf("\t\t%d,%d\n", torpedoPos[0], torpedoPos[1]);
        }
    }
    printf("TORPEDO MISSED!\n");
    if (KlingonsInQuadrant(refGalaxy, (*ePr).position)) {
      KlingonsFire(refGalaxy);
      KlingonsMove(refGalaxy);
    }
    return;
}

void exeSHE(struct Galaxy* refGalaxy) {
    //User input for amount of energy to allocate to shields:
    int x;
    if((*refGalaxy).enterprise.sysDamage[6] < 0) {
      printf("SHIELD CONTROL INOPERABLE\n\n");
      return;
    }
    printf("ENERGY AVAILABLE = %d\n", ((*refGalaxy).enterprise.energy + (*refGalaxy).enterprise.shields));
    printf("INPUT NUMBER OF UNITS TO SHIELDS? ");
    scanf("%d", &x);
    getchar();                                                                  // To get rid of the extra newline in input buffer
    if(x < 0 || (*refGalaxy).enterprise.shields == x) {
      printf("\n<SHIELDS UNCHANGED>\n\n");
      return;
    }
    if(x <= ((*refGalaxy).enterprise.energy + (*refGalaxy).enterprise.shields)) {
      (*refGalaxy).enterprise.energy = (*refGalaxy).enterprise.energy + (*refGalaxy).enterprise.shields - x;
      (*refGalaxy).enterprise.shields = x;
      printf("DEFLECTOR CONTROL ROOM REPORT : \n");
      printf("SHIELDS NOW AT %d UNITS PER YOUR COMMAND\n\n", (*refGalaxy).enterprise.shields);
      return;
    }
    printf("SHIELD CONTROL REPORTS 'THIS IS NOT THE FEDEREATION TREASURY'\n");
    printf("<SHIELDS UNCHANGED>\n\n");
    return;
}

void exeDBG(struct Galaxy* refGalaxy) {                                         // "secret" debug menu option
    if (!(*refGalaxy).glblDEBUG) {
      printf("\n\tACCESSING DEBUG SUBROUTINES...\n");
      printf("\tCREDENTIALS AUTHENTICATED...\n");
      printf("\tUNIVERSE ADMIN ACCESS GRANTED!\n");
      printf("\nGOD COMMANDS UNLOCKED:\n");
      (*refGalaxy).glblDEBUG = true;
    } else {
      printf("\nGOD COMMANDS ACTIVE:\n");
    }
    printf("  +EN: ADD ENTERPRISE ENERGY\n");
    printf("  +TP: ADD ENTERPRISE TORPEDOES\n");
    printf("  +RP: REPAIR ALL SYSTEMS\n");
    printf("  +OV: OVERPOWER ALL SYSTEMS\n");
    printf("  +LR: SUPER-LONG-RANGE SENSORS\n\n");
    printf("GAME VITALS STATUS:\n");
    printf("  STARBASES REMAINING:\t%d\n", (*refGalaxy).starbaseCount);
    printf("  KLINGONS REMAINING:\t%d\n", (*refGalaxy).klingonCount);
    printf("  KLINGONS IN QUADRANT:\t%d\n", KlingonsInQuadrant(refGalaxy,(*refGalaxy).enterprise.position));
    printf("  STARDATE\t\t%.1f\n", (*refGalaxy).stardate);
    printf("  USER HAS QUIT:\t");
    if ((*refGalaxy).enterprise.userQuit) { printf("TRUE\n\n");
    } else { printf("FALSE\n\n"); }
    printf("\nENTERPRISE STATUS:\n");
    printf("  POSITION-\n    QUADRANT:\t\t(%d,%d)\n    SECTOR:\t\t(%d,%d)\n", (*refGalaxy).enterprise.position[0], (*refGalaxy).enterprise.position[1], (*refGalaxy).enterprise.position[2], (*refGalaxy).enterprise.position[3]);
    printf("  ENERGY:\t\t%d (%d FREE)\n", getTotalEnergy(&((*refGalaxy).enterprise)), (*refGalaxy).enterprise.energy);
    printf("  SHIELDS:\t\t%d\n", (*refGalaxy).enterprise.shields);
    printf("  TORPEDOES:\t\t%4d\n\n", (*refGalaxy).enterprise.torpedoes);
    printf("\n");
    return;
}

int getTotalEnergy(struct Enterprise* refEnt) {
    int E = (*refEnt).energy;
    int S = (*refEnt).shields;
    return (E + S);
}

void setDest(int* _start, double dir, double dist, int* _destination) {         // Using starting coordinates, direction (NAV number), and distance, it sets destination coordinates accordingly
    int sStart[2] = {(_start[0] * 8 + _start[2]), (_start[1] * 8 + _start[3])}; // for simplicity of calculation, convert (Qx,Qy,Sx,Sy) format to an equivalent (Sx,Sy)
    int sEnd[2] = {sStart[0], sStart[1]};
    int sDiff[2] = {0, 0};
    int lclDiff[4] = {0, 0, 0, 0};

    double theta = ((dir - 1.0) * -PI) / 4.0;                                   // convert "1.0 to 9.0" game direction to angle in radians
    sDiff[0] = round(dist * 8 * sin(theta));                                    // find vertical position change, in sectors
    sDiff[1] = round(dist * 8 * cos(theta));                                    // find horizontal position change, in sectors

    sEnd[0] = sStart[0] + sDiff[0];                                             // apply change
    sEnd[1] = sStart[1] + sDiff[1];

    if (((sEnd[0]) > 71) || ((sEnd[0]) < 9) || ((sEnd[1]) > 71) || ((sEnd[1]) < 9)) { // check if out of bounds
        sEnd[0] = minnum(71.0, maxnum(9.0, (double)sEnd[0]));                   //  if so, constrict to bounds
        sEnd[1] = minnum(71.0, maxnum(9.0, (double)sEnd[1]));

        lclDiff[0] = floor(sEnd[0] / 8) - floor(sStart[0] / 8);                 // apply (altered) change
        lclDiff[1] = floor(sEnd[1] / 8) - floor(sStart[1] / 8);
        lclDiff[2] = (sEnd[0] % 8) - (sStart[0] % 8);
        lclDiff[3] = (sEnd[1] % 8) - (sStart[1] % 8);
        _destination[0] = _start[0] + lclDiff[0];
        _destination[1] = _start[1] + lclDiff[1];
        _destination[2] = _start[2] + lclDiff[2];
        _destination[3] = _start[3] + lclDiff[3];
        printf("LT. UHURA REPORTS MESSAGE FROM STARFLEET COMMAND: \n");         // then print warning message with new position
        printf("'PERMISSION TO ATTEMPT CROSSING OF GALACTIC PERIMETER\n");
        printf("IS HEREBY *DENIED*. SHUT DOWN YOUR ENGINES.' CHIEF\n");
        printf("ENGINEER SCOTT REPORTS 'WARP ENGINES SHUT DOWN AT\n");
        printf("SECTOR %d,%d OF QUADRANT %d,%d'\n\n", _destination[2], _destination[3], _destination[0], _destination[1]);
        return;
    } else {                                                                    // otherwise, apply changes
        lclDiff[0] = floor(sEnd[0] / 8) - floor(sStart[0] / 8);
        lclDiff[1] = floor(sEnd[1] / 8) - floor(sStart[1] / 8);
        lclDiff[2] = (sEnd[0] % 8) - (sStart[0] % 8);
        lclDiff[3] = (sEnd[1] % 8) - (sStart[1] % 8);

        _destination[0] = _start[0] + lclDiff[0];
        _destination[1] = _start[1] + lclDiff[1];
        _destination[2] = _start[2] + lclDiff[2];
        _destination[3] = _start[3] + lclDiff[3];
        return;
    }
}


double getDirection(double yD, double xD) {                                     // Calculates direction between two sets of coordinates
    // Translate given coordinates from (row, column) to Cartesian:
    printf("yD: %.1f\n", yD);
    printf("xD: %.1f\n", xD);
    double theta = atan(yD/xD);
    if ((xD >= 0) && (yD >= 0)) { theta = atan((double)yD/xD); }
    else if ((xD >= 0) && (yD < 0)) {
        yD = abs(yD);
        theta = 2*PI - asin((double)yD/xD);
    } else if ((xD < 0) && (yD >= 0)) {
        xD = abs(xD);
        theta = PI - asin((double)yD/xD);
    } else {
        xD = abs(xD);
        yD = abs(yD);
        theta = asin((double)yD/xD) + PI;
    }
    // Convert angle into 1-9 game notation (1 = East = positive x-axis)
    double direction = (((4.0 * theta) / (double)PI) + (double)1.0);
    return direction;
}

double checkObstacles(int* _start, double dir, double dist, struct Galaxy* refGalaxy) { // iterates through parameters' travel path to check for obstacles, returns furthest good coordinate if found
    for (double i=0.1; i<=dist; i+=0.1) {
        int chkCoords[4] = {0, 0, 0, 0};
        setDest(_start, dir, i, chkCoords);
        if ((*refGalaxy).coordinates[chkCoords[0]][chkCoords[1]][chkCoords[2]][chkCoords[3]] != ' ') { return (i - 0.1); }
    }
    return dist;
}


double getDist(struct Galaxy* refGalaxy, int* destination) {
    double yDist = abs(((*refGalaxy).enterprise.position[0]*8)+(*refGalaxy).enterprise.position[2]) - ((destination[0]*8)+destination[2]);
    double xDist = abs(((*refGalaxy).enterprise.position[1]*8)+(*refGalaxy).enterprise.position[3]) - ((destination[1]*8)+destination[3]);
    return (sqrt(pow(xDist,2) + pow(yDist,2)));
}

void KlingonsFire(struct Galaxy* refGalaxy) {                                   //Klingon firing behavior when player flies through their occupied quadrant
    struct Enterprise* ePr = &((*refGalaxy).enterprise);
    if ((*ePr).isDestroyed) { return; }                                         // if enterprise destroyed, "Stop! Stop! He's already dead!".mp4
    int klingonsFiring = KlingonsInQuadrant(refGalaxy, (*ePr).position);
    if (klingonsFiring > 0) {
        for (int i=1; i<=klingonsFiring; i++) {
            struct Klingon* thisK = getNthClosestKlingon(refGalaxy,i);
            double kDmg = (*thisK).energy;
            kDmg /= getDist(refGalaxy,(*thisK).position);
            kDmg *= (2.0 + RND1());
            int kAtk = (int)round(kDmg);
            (*thisK).energy = (int)round((*thisK).energy/(2.0+RND1()));
            printf("%d UNIT HIT ON ENTERPRISE FROM SECTOR %d,%d\n", kAtk, (*thisK).position[2], (*thisK).position[3]);
            (*ePr).shields -= kAtk;
            if ((*ePr).shields <= 0) {
                (*ePr).isDestroyed = true;
                return;
            }
            printf("\t<SHIELDS DOWN TO %d UNITS>\n", (*ePr).shields);
            if ((kAtk >= 20) && (((double)kAtk/(double)(*ePr).shields) > 0.02) && (RND1() > 0.6)) {
              int randSys = (int)round(RND1()*8);
              (*ePr).sysDamage[randSys] = maxnum(-1.0, (*ePr).sysDamage[randSys] - (kAtk/10.0));
              printf("DAMAGE CONTROL REPORTS %s DAMAGED BY THE HIT\n\n", (*ePr).sysNames[randSys]);
            }
        }
    }
    KlingonsMove(refGalaxy);
    return;
}

void KlingonsMove(struct Galaxy* refGalaxy) {                                   //Klingon moving behavior
    struct Enterprise* ePr = &((*refGalaxy).enterprise);
    int klingonsMoving = KlingonsInQuadrant(refGalaxy, (*ePr).position);
    if (klingonsMoving > 0) {
        for (int i=1; i<=klingonsMoving; i++) {
            struct Klingon* thisK = getNthClosestKlingon(refGalaxy,i);
            int W = (*thisK).position[0];
            int X = (*thisK).position[1];
            int Y = (int)ceil(RND1()*8.0);
            int Z = (int)ceil(RND1()*8.0);
            bool foundSpot = false;
            while (!foundSpot) {
                Y = (int)ceil(RND1()*8.0);
                Z = (int)ceil(RND1()*8.0);
                if ((*refGalaxy).coordinates[W][X][Y][Z] != '*') {
                    foundSpot = true;
                    for (int j=0; j<GAME_NUM_KLINGONS; j++) {
                        if (((*refGalaxy).klingons[j].position[0] == W) &&
                        ((*refGalaxy).klingons[j].position[1] == X) &&
                        ((*refGalaxy).klingons[j].position[2] == Y) &&
                        ((*refGalaxy).klingons[j].position[3] == Z)) {
                            foundSpot = false;
                        }
                    }
                }
            }
            (*thisK).position[2] = Y;
            (*thisK).position[3] = Z;
        }
    }
    return;
}

struct Klingon* getNthClosestKlingon(struct Galaxy* refGalaxy, int n) {         // returns a pointer to the nth-closest klingon to the enterprise within its quadrant
    struct Enterprise* ePr = &((*refGalaxy).enterprise);
    if (KlingonsInQuadrant(refGalaxy, (*ePr).position) < 1) { return NULL; }    // Nothing to return!
    int kCount = 0;
    double distIs[GAME_NUM_KLINGONS][2];                                        // make a 2-column list: each contains the distance of a klingon and its index in the galaxy array
    double tmpcrry[2];                                                          // temp space for sorting
    for (int i=0; i<GAME_NUM_KLINGONS; i++) {                                   // clear array values
        distIs[i][0] = 8000;
        distIs[i][1] = 0.0;
    }
    for (int i=0; i<GAME_NUM_KLINGONS; i++) {                                   // check for valid klingons in quadrant
        if (((*refGalaxy).klingons[i].position[0] == (*ePr).position[0]) &&
            ((*refGalaxy).klingons[i].position[1] == (*ePr).position[1]) &&
            ((*refGalaxy).klingons[i].energy > 0))  {
            double thisDist = getDist(refGalaxy,(*refGalaxy).klingons[i].position); // get desired info from them
            double thisI = (double)i;
            for (int j=0;j<=kCount; j++) {
                if (thisDist < distIs[j][0]) {                                  // Sort!!
                    tmpcrry[0] = distIs[j][0];
                    tmpcrry[1] = distIs[j][1];
                    distIs[j][0] = thisDist;
                    distIs[j][1] = thisI;
                    thisDist = tmpcrry[0];
                    thisI = tmpcrry[1];
                    tmpcrry[0] = 8000;                                          // set temp to very low precedence
                    tmpcrry[1] = 8000;
                }
            }
            kCount++;                                                           // keep track of how many have qualified
        }
    }
    return &((*refGalaxy).klingons[(int)distIs[n-1][1]]);                       // return the pointer to the klingon that matches the specified precedence
}

int StarbasesInQuadrant(struct Galaxy* refGalaxy, int* Q) {                     // checks enterprise's current quadrant and returns number of intact starbases within
    int SBCount = 0;
    for (int i=0; i<GAME_NUM_STARBASES; i++) {
        if (((*refGalaxy).starbases[i].position[0] == Q[0]) &&
            ((*refGalaxy).starbases[i].position[1] == Q[1]) &&
            ((*refGalaxy).starbases[i].energy > 0))  { SBCount++; }
    }
    return SBCount;
}

int KlingonsInQuadrant(struct Galaxy* refGalaxy, int* Q) {                      // checks enterprise's current quadrant and returns number of living Klingons within
    int KlingonCount = 0;
    for (int i=0; i<GAME_NUM_KLINGONS; i++) {
        if (((*refGalaxy).klingons[i].position[0] == Q[0]) &&
            ((*refGalaxy).klingons[i].position[1] == Q[1]) &&
            ((*refGalaxy).klingons[i].energy > 0))  { KlingonCount++; }
    }
    return KlingonCount;
}

bool checkIfDocked(struct Galaxy* refGalaxy) {                                  // checks for starbases within 1 sector of enterprise; sets condition to DOCKED if so
    struct Enterprise* ePr = &((*refGalaxy).enterprise);
    for (int i=0; i<GAME_NUM_STARBASES; i++) {
        struct Starbase* thisSB = &((*refGalaxy).starbases[i]);
        if (((*thisSB).energy > 0) &&
        ((*ePr).position[0] == (*thisSB).position[0]) &&
        ((*ePr).position[1] == (*thisSB).position[1]) &&
        (abs((*ePr).position[2] - (*thisSB).position[2]) <= 1) &&
        (abs((*ePr).position[3] - (*thisSB).position[3]) <= 1)) {
            (*ePr).condition = DOCKED;
            return true;
        }
    }
    return false;
}

struct Starbase* getClosestStarbase(struct Galaxy* refGalaxy) {                 // returns a pointer to the closest starbase to the enterprise within its quadrant
    struct Enterprise* ePr = &((*refGalaxy).enterprise);
    if (StarbasesInQuadrant(refGalaxy, (*ePr).position) < 1) { return NULL; }   // nothing to return!
    double bestDist = 8000;                                                     // remember the best distance
    int bestI = 0;                                                              // and which one it was
    for (int i=0; i<GAME_NUM_STARBASES; i++) {                                  // check for valid starbases
        if (((*refGalaxy).starbases[i].position[0] == (*ePr).position[0]) &&
            ((*refGalaxy).starbases[i].position[1] == (*ePr).position[1]) &&
            ((*refGalaxy).starbases[i].energy > 0))  {
            double thisDist = getDist(refGalaxy,(*refGalaxy).starbases[i].position); // find its distance
            int thisI = i;
            if (thisDist < bestDist) {                                          // if better than current best
                bestI = thisI;                                                  // remember which it is
                bestDist = thisDist;                                            // and update bestDist to the new best
            }
        }
    }
    return &((*refGalaxy).starbases[bestI]);                                    // return the pointer to the closest starbase
}

void updateCondition(struct Galaxy* refGalaxy) {                                // updates the enterprise's alert condition based on enemy proximity and ship status
    struct Enterprise* ePr = &((*refGalaxy).enterprise);
    if (StarbasesInQuadrant(refGalaxy,(*ePr).position) > 0) {
        if (checkIfDocked(refGalaxy)) {
            (*refGalaxy).enterprise.condition = DOCKED;
            return;
        }
    } else if (KlingonsInQuadrant(refGalaxy,(*refGalaxy).enterprise.position) > 0) {
        (*refGalaxy).enterprise.condition = RED;
        return;
    } else if (getTotalEnergy(&((*refGalaxy).enterprise)) < round(GAME_ENERGY_MAX * 0.1)) {
        (*refGalaxy).enterprise.condition = YELLOW;
        return;
    } else {
        (*refGalaxy).enterprise.condition = GREEN;
        return;
    }
    return;
}

double maxnum(double a, double b) { if (a >= b) { return a; } else { return b; } } // returns the larger of the two parameters

double minnum(double a, double b) { if (a <= b) { return a; } else { return b; } } // returns the smaller of the two parameters

void strtrim(char* string, int n) {                                             // Checks through string for leading whitespace, then copies characters to the string start, then ends string after n chars
    for (int i=0; i<n; i++) {
        if (string[i] == '\0') {
            return;
        } else if (string[i] != ' ') {
            for (int j=0; j<(n-i); j++) { string[j] = string[j+i]; }
            string[n-i] = '\0';
            return;
        }
    }
    return;
}

void remNL(char* string, int n) {                                               // Checks the end of the string parameter for the newline character and, if found, removes it
    if (string[n-1] == '\n') { string[n-1] = '\0'; }                            // (useful with fgets)
    return;
}

void strToUpper(char* string, int n) {                                          // Loops through string parameter, changing each index to its uppercase version
    for (int i=0; i<n; i++) { string[i] = toupper(string[i]); }
    return;
}

double RND1() { return (rand()%1000)/((double)1000.0); }                        // returns random floating point number between 0 and 1 (as is used in BASIC source)

void displayManual() {                                                          // displays game manual, then immediately returns
    printf("\n\n1. You are the captain of the starship ''Enterprise'' with a mission to seek and destroy a fleet of Klingon warships (usually about 17) which are menacing the United Federation of Planets. You have a specified number of stardates in which to complete your mission. You also have two or three Federation Starbases for resupplying your ship.\n\n");
    printf("2. You will be assigned a starting position somewhere in the galaxy. The galaxy is divided into an 8 x 8 quadrant grid. The astronomical name of a quadrant is called out upon entry into a new region. (See ''Quadrant Nomenclature.'') Each quadrant is further divided into an 8 x 8 section grid.\n\n");
    printf("3. On a section diagram, the following symbols are used:\n\n");
    printf("\t<E>\tEnterprise\t\t>!<\tStarbase\n\t+K+\tKlingon\t\t\t*\tStar\n\n");
    printf("4. You have eight commands available to you. (A detailed description of each command is given in the program instructions.)\n\n");
    printf("\tNAV\tNavigate the Starship by setting course and warp engine speed.\n\tSRS\tShort-range sensor scan (one quadrant)\n\tLRS\tLong-range sensor scan (9 quadrants)\n\tPHA\tPhaser control (energy gun)\n\tTOR\tPhoton torpedo control\n\tSHE\tShield control (protects against phaser fire)\n\tDAM\tDamage and state-of-repair report\n\tCOM\tCall library computer\n\n");
    printf("(Press Enter to continue...)\n");
    getchar();
    printf("When setting a course with the NAV command, direction is indicated by a floating point number from 1.0 to 9.0, according to the following convention:\n");
    printf("\t  4  3  2\n");
    printf("\t   \\ | / \n");
    printf("\t    \\|/  \n");
    printf("\t  5--*--1\n");
    printf("\t    /|\\  \n");
    printf("\t   / | \\ \n");
    printf("\t  6  7  8\n");
    printf("\t(1.0 == 9.0)\n");
    printf("Non-whole numbers interpolate between directions. E.g. '1.5' would direct halfway between the 1.0 and 2.0 directions.(1.0 == 9.0)\n");
    printf("5. Library computer options are as follows (more complete descriptions are in program instructions):\n\n");
    printf("\t0\tCumulative galactic record\n\t1\tStatus report\n\t2\tPhoton torpedo course data\n\t3\tStarbase navigation data\n\t4\tDirection/distance calculator\n\t5\tQuadrant nomenclature map\n\n");
    printf("6. Certain reports on the ship's status are made by officers of the Enterprise who appeared on the original TV show - Spock, Scott, Uhura, Chekov, etc.\n\n");
    printf("7. Klingons are non-stationary within their quadrants. If you try to maneuver on then, they will move and fire on you.\n\n");
    printf("8. Firing and damage notes:\n\n\tA. Phaser fire diminishes with increased distance between combatants.\n\tB. If a Klingon zaps you hard enough (relative to your shield strength) he will generally cause damage to some part of your ship with an appropriate ''Damage Control'' report resulting.\n\tC. If you don't zap a Klingon hard enough (relative to his shield strength) you won't damage him at all. Your sensors will tell the story.\n\tD. Damage control will let you know when out-of-commission devices have been completely repaired.\n\n");
    printf("9. Your engines will automatically shut down if you should attempt to leave the galaxy, or if you should try to maneuver through a star, a Starbase, or - heaven help you - a Klingon warship.\n\n");
    printf("10. In a pinch, or if you should miscalculate slightly, some shield control energy will be automatically diverted to warp engine control (if your shields are operational!).\n\n");
    printf("(Press Enter to continue...)\n");
    getchar();
    printf("11. While you're docked at a Starbase, a team of technicians can repair your ship (if you're willing for the to spend the time required - and the repairmen always underestimate...)\n\n");
    printf("12. If, to save maneuvering time toward the end of the gane, you should cold-bloodedly destroy a Starbase, you get a nasty note from Starfleet Command. If you destroy your last Starbase, you lose the game!\n\n");
    printf("13. End game logic has been ''cleaned up'' in several spots, and it is possible to get a new command after successfully completing your mission (or, after resigning your old one).\n\n\n");
    printf("\t(Enter game command 'MAN' at any time to see this manual again)\n\n");
    return;
}
