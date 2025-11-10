#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "time.h"
#include "math.h"
#include <stdbool.h>

#define CONSOLE 0

#define SCANF(...) scanf(__VA_ARGS__)
/***************************** TIMERS **************************************/
#define MAX_TIME 30

/***************************** PRINTERS ************************************/
#define DUMP_INPUT(...)    fprintf(stderr,__VA_ARGS__);
#define TIME_OUT(...)      fprintf(stderr,__VA_ARGS__);
#define MINMAX_OUT(...)    fprintf(stderr,__VA_ARGS__);
#define OUT(...)           fprintf(stderr,__VA_ARGS__);
#define OUT2(...)          fprintf(stderr,__VA_ARGS__);
#define NODE_OUT(...)      fprintf(stderr,__VA_ARGS__);
#define DIFF(...)          fprintf(stderr,__VA_ARGS__);
#define PRINT_DEPTH 10
#define MAX_MSG 100

/********************** GAME CONSTS ***************************************/
#define MAX_PLAYERS 2
#define MAX_ENTITIES 100
#define MAX_WIDTH 17630
#define MAX_HEIGHT 9000
#define MAX_BASE_HIT 3
#define NB_HEROES 3
#define MOVE_MAX 800
#define BEAT_RADIUS 800
#define MONSTER_BEAT_POINTS 2
#define MONSTER_SPEED 400
#define MONSTER_TARGET_BASE_RADIUS 5000
#define SPELL_COST  10
#define SPELL_RANGE 2200

#define ROUND(f) ((int)(f >= 0.0 ? (f + 0.5) : (f - 0.5)))

/********************************************************************************************************************************
 * GAME STRUCTS *
 ********************************************************************************************************************************/
typedef enum  { MIN, MAX } NodeType_t;

typedef struct _IPosition_ IPosition_t;
struct _IPosition_ {
    int x;
    int y;
};

double IIDistance(IPosition_t p1, IPosition_t p2) {
    return (sqrt((p1.x-p2.x)*(p1.x-p2.x)+(p1.y-p2.y)*(p1.y-p2.y)));
}

typedef struct _Map_ Map_t;
struct _Map_ {
    IPosition_t myBase;
    IPosition_t enemyBase;
    int heroesPerPlayer;
    IPosition_t HeroPos[NB_HEROES];
};

static Map_t s_Map;
static Map_t *pMap = &s_Map;

typedef enum  { WAIT, MOVE, SPELL } Command_t;
typedef enum  { WIND, SHIELD, CONTROL, VOID_SPELL } Spell_t;
typedef struct _Move_ Move_t;
struct _Move_ {
    Command_t command[NB_HEROES];
    Spell_t Spell[NB_HEROES];
    IPosition_t pos[NB_HEROES];
    int EntityId[NB_HEROES];
    char MVMSG[NB_HEROES][MAX_MSG];
};

void Move_Print(Move_t *pMove) {
    for (int c = 0; c < NB_HEROES; c++) {
        //OUT("Print %d %d %f %f\n",c,pMove->command[c],pMove->moves[c].x,pMove->moves[c].y);
        switch (pMove->command[c]) {
            case WAIT : printf("WAIT"); break;
            case MOVE : printf("MOVE %d %d",pMove->pos[c].x,pMove->pos[c].y); break;
            case SPELL : switch (pMove->Spell[c]) {
                case WIND : printf("SPELL WIND %d %d",pMove->pos[c].x,pMove->pos[c].y); break;
                case SHIELD : printf("SPELL SHIELD %d",pMove->EntityId[c]); break;
                case CONTROL : printf("SPELL CONTROL %d %d %d",pMove->EntityId[c],pMove->pos[c].x,pMove->pos[c].y); break;
                default : printf("WAIT");
            } break;
            default : printf("WAIT");
            
        }
        if (pMove->MVMSG[c][0] != 0) {
            pMove->MVMSG[c][MAX_MSG-1] = 0;
            printf(" %s",pMove->MVMSG[c]);
        }
        printf("\n");
    }
}
void Move_Init(Move_t *pMove)
{
    for (int i = 0; i < NB_HEROES; i++) {
        pMove->command[i] = WAIT;
        pMove->pos[i].x = 0;
        pMove->pos[i].y = 0;
        pMove->Spell[i] = VOID_SPELL;
        pMove->EntityId[i] = -1;
        pMove->MVMSG[i][0] = 0;
    }
}

void Move_Add(Move_t *pMove,
              int HeroeID,
              Command_t command,
              Spell_t Spell,
              int EntityId,
              IPosition_t position)
{
    pMove->command[HeroeID] = command;
    pMove->Spell[HeroeID] = Spell;
    pMove->EntityId[HeroeID] = EntityId;
    pMove->pos[HeroeID].x = position.x;
    pMove->pos[HeroeID].y = position.y;
    OUT("Add P%d Hero %d : command %d (%d,%d) spell %d EntityId %d\n",0,
        HeroeID,
        pMove->command[HeroeID],
        pMove->pos[HeroeID].x,
        pMove->pos[HeroeID].y,
        pMove->Spell[HeroeID],
        pMove->EntityId[HeroeID]);
}

typedef enum  { MONSTER, HERO, ENEMY } EntityType_t;

typedef struct _Entity_ Entity_t;
struct _Entity_ {
    int id;
    EntityType_t type;
    IPosition_t pos;
    //FPosition_t FPos;
    int shieldLife;
    int isControlled;
    int health;
    IPosition_t Speed;
    //IPosition_t ControlSpeed;
    int nearBase;
    int threatFor;
};

typedef struct _Node_ Node_t;
struct _Node_ {
    int health[MAX_PLAYERS];
    int mana[MAX_PLAYERS];
    int MyHeroIDs[NB_HEROES];
    int EnemyHeroIDs[NB_HEROES];
    int entityCount; // number of entities still in game
    Entity_t pEntities[MAX_ENTITIES];
};
/**
 * Auto-generated code below aims at helping you parse
 * the standard input according to the problem statement.
 **/
void Parse_input(Node_t *pNode)
{
    for (int i = 0; i < MAX_PLAYERS; i++) {
        // Each player's base health
        int health;
        // Ignore in the first league; Spend ten mana to cast a spell
        int mana;
        SCANF("%d%d", &health, &mana);
        DUMP_INPUT("%d %d\n", health, mana);  
        pNode->health[i] = health;
        pNode->mana[i] = mana;
    }
    // Amount of heros and monsters you can see
    int myHeroCount = 0;
    int enemyHeroCount = 0;
    int entity_count;
    SCANF("%d", &entity_count);
    DUMP_INPUT("%d\n", entity_count);
    pNode->entityCount = entity_count;
    for (int i = 0; i < entity_count; i++) {
        // Unique identifier
        int id;
        // 0=monster, 1=your hero, 2=opponent hero
        int type;
        // Position of this entity
        int x;
        int y;
        // Ignore for this league; Count down until shield spell fades
        int shield_life;
        // Ignore for this league; Equals 1 when this entity is under a control spell
        int is_controlled;
        // Remaining health of this monster
        int health;
        // Trajectory of this monster
        int vx;
        int vy;
        // 0=monster with no target yet, 1=monster targeting a base
        int near_base;
        // Given this monster's trajectory, is it a threat to 1=your base, 2=your opponent's base, 0=neither
        int threat_for;
        SCANF("%d%d%d%d%d%d%d%d%d%d%d", &id, &type, &x, &y, &shield_life, &is_controlled, &health, &vx, &vy, &near_base, &threat_for);
        DUMP_INPUT("%d %d %d %d %d %d %d %d %d %d %d\n", id, type, x, y, shield_life, is_controlled, health, vx, vy, near_base, threat_for);
        Entity_t *pEntity = pNode->pEntities+i;
        pEntity->id = id;
        if (type == 0) {
            pEntity->type = MONSTER;
        } else if (type == 1) {
            pEntity->type = HERO;
        } else {
            pEntity->type = ENEMY;
        }
        pEntity->pos.x = x;
        pEntity->pos.y = y;
        pEntity->shieldLife = shield_life;
        pEntity->isControlled = is_controlled;
        pEntity->health = health;
        pEntity->Speed.x = vx;
        pEntity->Speed.y = vy;
        pEntity->nearBase = near_base;
        pEntity->threatFor = threat_for;
        if (type == 1) {
            pNode->MyHeroIDs[myHeroCount] = i;
            myHeroCount++;
        } else if (type == 2) {
            pNode->EnemyHeroIDs[enemyHeroCount] = i;
            enemyHeroCount++;
        }
    }
}

void Map_Init(void)
{
     int base_x;
    int base_y;
    SCANF("%d%d", &base_x, &base_y);
    DUMP_INPUT("%d %d\n", base_x, base_y);
    // Always 3
    int heroes_per_player;
    SCANF("%d", &heroes_per_player);
    DUMP_INPUT("%d\n", heroes_per_player);
    pMap->myBase.x = base_x;
    pMap->myBase.y = base_y;
    pMap->enemyBase.x = MAX_WIDTH-base_x;
    pMap->enemyBase.y = MAX_HEIGHT-base_y;
    pMap->heroesPerPlayer = heroes_per_player;
}

IPosition_t GetClosestMonsterPos(Node_t *pNode, int heroID)
{
    Entity_t *pHero = pNode->pEntities + pNode->MyHeroIDs[heroID];
    IPosition_t closestMonsterPos = {0,0};
    closestMonsterPos.x = pHero->pos.x;
    closestMonsterPos.y = pHero->pos.y;
    double closestDistance = MAX_WIDTH;
    for (int i = 0; i < pNode->entityCount; i++) {
        Entity_t *pEntity = pNode->pEntities + i;
        if (pEntity->type == MONSTER) {
            double distance = IIDistance(pHero->pos,pEntity->pos);
            if (distance < closestDistance) {
                closestDistance = distance;
                closestMonsterPos = pEntity->pos;
            }
        }
    }
    return closestMonsterPos;
}

IPosition_t GetBaseClosestMonsterPos(Node_t *pNode, int heroID)
{
    Entity_t *pHero = pNode->pEntities + pNode->MyHeroIDs[heroID];
    IPosition_t closestMonsterIdx = {0,0};
    double closestDistance = MAX_WIDTH;
    for (int i = 0; i < pNode->entityCount; i++) {
        Entity_t *pEntity = pNode->pEntities + i;
        if (pEntity->type == MONSTER) {
            if (IIDistance(pEntity->pos,pMap->myBase) > 13000) {
                continue;
            }
            double distance = IIDistance(pHero->pos,pEntity->pos);
            if (distance < closestDistance) {
                closestDistance = distance;
                closestMonsterIdx = pEntity->pos;
            }
        }
    }
    return closestMonsterIdx;
}


int KeepHeroDistance(Node_t *pNode, int heroID)
{
    Entity_t *pHero = pNode->pEntities + pNode->MyHeroIDs[heroID];
    for (int h = 0; h < 3; h++) {
        if (h == heroID) {
            break;
        }
        Entity_t *pHero2 = pNode->pEntities + pNode->MyHeroIDs[h];
        if (pHero2->type == HERO) {
            double distance = IIDistance(pHero->pos,pHero2->pos);
            if (distance < 1000) {
                return 1;
            }
        }
    }
    return 0;
}

int KeepBaseDistance(Node_t *pNode, int heroID)
{
    Entity_t *pHero = pNode->pEntities + pNode->MyHeroIDs[heroID];
    double distance = IIDistance(pHero->pos,pMap->myBase);
    if (distance < 6000) {
        return 1;
    }
    return 0;
}

int IsCritical(Node_t *pNode)
{
    for (int i = 0; i < pNode->entityCount; i++) {
        Entity_t *pEntity = pNode->pEntities + i;
        if (pEntity->type == MONSTER) {
            if (pEntity->threatFor == 1) {
                if (IIDistance(pEntity->pos,pMap->myBase) < 700) {
                    OUT("Is Critical Enemy %d\n",pEntity->id);
                    return 1;
                }
            }
        }
    }
    return 0;
}

int IsDangerous(Node_t *pNode)
{
    for (int i = 0; i < pNode->entityCount; i++) {
        Entity_t *pEntity = pNode->pEntities + i;
        if (pEntity->type == MONSTER) {
            if (pEntity->threatFor == 1) {
                if (IIDistance(pEntity->pos,pMap->myBase) < 6000) {
                    OUT("Is Dangerous Enemy %d\n",pEntity->id);
                    return 1;
                }
            }
        }
    }
    return 0;
}

IPosition_t Move_Toward(IPosition_t start, IPosition_t end, int radius){
    IPosition_t toward = {0,0};
    double distance = IIDistance(start,end);
    if (distance > ((double) radius)) {
        toward.x = (int) (start.x+radius * ((double)  (end.x - start.x)) / distance);
        toward.y = (int) (start.y+radius * ((double)  (end.y - start.y)) / distance);
    } else {
        toward.x = end.x;
        toward.y = end.y;
    }
    OUT("Move Toward from %d %d to %d %d radius %d -> %d %d\n",start.x,start.y,end.x,end.y,radius,toward.x,toward.y);
    return toward;
}
void Dummy_Move(Node_t *pNode, Move_t *pMove)
{
    Move_Init(pMove);
    for (int i = 0; i < NB_HEROES; i++) {

        OUT("Hero %d\n",i);
        //Entity_t *pHero = pNode->pEntities + pNode->MyHeroIDs[i];
        IPosition_t closestMonsterPos = GetBaseClosestMonsterPos(pNode,i);
        if (KeepHeroDistance(pNode,i)) {
            OUT("Keep Hero Distance goto Base %d %d\n",pMap->myBase.x,pMap->myBase.y);
            Move_Add(pMove,i,MOVE,VOID_SPELL,-1,pMap->myBase);
        } else {
            OUT("Go to Closest Monster %d %d\n",closestMonsterPos.x,closestMonsterPos.y);
            Move_Add(pMove,i,MOVE,VOID_SPELL,-1,closestMonsterPos);
            if (KeepBaseDistance(pNode,i) == 0) {
                OUT("Keep Base Distance Go toward Base %d %d\n",pMap->myBase.x,pMap->myBase.y);
                Move_Add(pMove,i,MOVE,VOID_SPELL,-1,Move_Toward(closestMonsterPos,pMap->myBase,800));
            }
        }
    }
    if (IsDangerous(pNode) == 1) {
        double ClosestToBase = MAX_WIDTH+MAX_HEIGHT;
        for (int i = 0; i < pNode->entityCount; i++) {
            Entity_t *pEntity = pNode->pEntities + i;
            if (pEntity->type == MONSTER) {
                if (pEntity->threatFor == 1) {
                    if (IIDistance(pEntity->pos,pMap->myBase) < ClosestToBase) {
                        OUT("Is Dangerous Enemy %d\n",pEntity->id);
                        Move_Add(pMove,1,MOVE,VOID_SPELL,pEntity->id,pEntity->pos);
                        Move_Add(pMove,2,MOVE,VOID_SPELL,pEntity->id,pEntity->pos);
                        
                    }
                }
            }
        }
    }
    if (IsCritical(pNode) == 1 && pNode->mana[0] >= SPELL_COST) {
        OUT("Is Critical\n");
        int closestHero = 1;
        double closestDistance = MAX_WIDTH;
        for (int h=1; h < NB_HEROES; h++) {
            if (IIDistance(pNode->pEntities[pNode->MyHeroIDs[h]].pos,pMap->myBase) < closestDistance) {
                closestDistance = IIDistance(pNode->pEntities[pNode->MyHeroIDs[h]].pos,pMap->myBase);
                closestHero = h;
            }
        }
        OUT("Closest Hero %d\n",closestHero);
        Move_Add(pMove,closestHero,SPELL,WIND,-1,pMap->enemyBase);
    }
}

int main(void)
{

    if (CONSOLE != 1)  {

        Map_Init();

        // game loop
        int loop = 0;

        while (1) {
            Node_t Node;
            Node_t *pNode = &Node;
            Parse_input(&Node);
            if (loop == 0) {
                // nothing yet at init time
            }

            Move_t Move;
            Move_Init(&Move);
            Dummy_Move(pNode,&Move);
            // Now move Hero 0 to enemy base    
            if (IIDistance(pNode->pEntities[pNode->MyHeroIDs[0]].pos,pMap->enemyBase) > 6000) {
                Move_Add(&Move,0,MOVE,VOID_SPELL,-1,pMap->enemyBase);
            } else {
                Entity_t *pHero = pNode->pEntities + pNode->MyHeroIDs[0];
                for (int i = 0; i < pNode->entityCount; i++) {
                    Entity_t *pEntity = pNode->pEntities + i;
                    if (IIDistance(pHero->pos,pEntity->pos) > 2200) {
                        continue;
                    }
                    if (pEntity->type == MONSTER && 
                        pEntity->nearBase == 1 &&
                        pEntity->threatFor == 2) {
                        if (pEntity->shieldLife > 0) {
                            continue;
                        }
                        if (pNode->mana[0] < SPELL_COST) {
                            continue;
                        }
                        if (pEntity->health < 15) {
                            continue;
                        }
                        Move_Add(&Move,0,SPELL,SHIELD,pEntity->id,pEntity->pos);
                        OUT("SHIELD ENTITY %d\n",pEntity->id);
                        break;
                    }
                }
        
            }
            Move_Print(&Move);
            loop++;
        }
    } else {
                Map_Init();

        // game loop
        int loop = 0;

        //while (1) {
            Node_t Node;
            Node_t *pNode = &Node;
            Parse_input(&Node);
            if (loop == 0) {
                // nothing yet at init time
            }

            Move_t Move;
            Move_Init(&Move);
            Dummy_Move(pNode,&Move);
            // Now move Hero 0 to enemy base    
            if (IIDistance(pNode->pEntities[pNode->MyHeroIDs[0]].pos,pMap->enemyBase) > 6000) {
                Move_Add(&Move,0,MOVE,VOID_SPELL,-1,pMap->enemyBase);
            } else {
                Entity_t *pHero = pNode->pEntities + pNode->MyHeroIDs[0];
                for (int i = 0; i < pNode->entityCount; i++) {
                    Entity_t *pEntity = pNode->pEntities + i;
                    if (IIDistance(pHero->pos,pEntity->pos) > 2200) {
                        continue;
                    }
                    if (pEntity->type == MONSTER && 
                        pEntity->nearBase == 1 &&
                        pEntity->threatFor == 2) {
                        if (pEntity->shieldLife > 0) {
                            continue;
                        }
                        if (pNode->mana[0] < SPELL_COST) {
                            continue;
                        }
                        if (pEntity->health < 15) {
                            continue;
                        }
                        Move_Add(&Move,0,SPELL,SHIELD,pEntity->id,pEntity->pos);
                        OUT("SHIELD ENTITY %d\n",pEntity->id);
                        break;
                    }
                }
        
            }
            Move_Print(&Move);
            loop++;
        //}
    }

    return 0;
}
