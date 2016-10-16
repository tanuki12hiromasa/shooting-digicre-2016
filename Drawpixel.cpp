#include "DxLib.h"
#include <math.h>

#define PI 3.141592653
#define ENEMY_AMOUNT 8
#define ENEMY_KIND 8
#define PLAYER_ARM_X 32
#define PLAYER_ARM_Y 32
#define PLAYER_MOTION 3
#define SHOT_ENEMY_AMOUNT 40
#define SHOT_PLAYER_AMOUNT 12
#define SHOT_SLAVE_AMOUNT 8
#define SHOT_ENEMY_KIND 4
#define SHOT_PLAYER_KIND 4
#define PICTURE_PATTERN_ENEMY 7
#define PICTURE_PATTERN_PLAYER 4

static int Framecount = 0;
static double Fpsmajor = 0;
int BLACK = GetColor(0, 0, 0);
int WHITE = GetColor(255, 255, 255);
int LASERRED = GetColor(255, 60, 100);
int superflag;
char keydata[256];
int GameState = 0;
int HandState = 0;

enum gameState {
    TITLE = 0, OPTION = -2, STAGE_1 = 1, STAGE_2 = 2, GAMEOVER = -1
};

//自機弾名称
enum NameBulletPlayer {
    pb_zero = 0, pb_quarter, pb_half, pb_threequarters, pb_fullcharge = 4
};

//敵機弾名称
enum NameBulletEnemy {
    solidball=0,missile=1,lazer=2
};

int AtariJudge(int ax, int ay, int asizex, int asizey, int bx, int by, int bsizex, int bsizey, int marginx, int marginy);

struct Player{
    int x;
    int y;
    int dx;
    int dy;
    int sizex;
    int sizey;
    bool dmgflg;
    int dmgtime;
    int life;
    int motionnum;//動きに対応する画像番号
    int actnum;//アニメーションさせる番号
    int shotstate;
    int chargetime;
    int shotnumber;
   // int pic[12];//各画像のハンドル
};

struct Enemy {
    bool flg;
    int enumber;
    int sizex;
    int sizey;
    int x;
    int y;
    int dx;
    int dy;
    int movetime;
    int life;
    bool memoryflg;
    int actnumber;
    int acttime;
};

struct Shot {
    int type;
    int dmg;
    int vel;
    int x;
    int y;
    double theta;
    int sizex;
    int sizey;
    bool flg;
    bool memoryflg;
};

struct Hand {
    int pic;
    int x[40];
    int y[40];
    double theta;
    int sizex;
    int sizey;
    int amount;
    int number;
    int enemy;
    int flg;
};

struct BomberEffect{
    int x;
    int y;
    int acttime;
};

typedef struct background {
    int x;
    int y;
    int sizex;
    int sizey;
    int pic;
    bool flg;
}BG;

//システム関係クラス
class GameSystem {
public:
    Player player;
    Shot playershot[SHOT_PLAYER_AMOUNT] = {0};
    Enemy slave;
    Shot slaveshot[SHOT_SLAVE_AMOUNT];
    Hand hand;
    Enemy enemy[ENEMY_AMOUNT];
    Shot enemyshot[SHOT_ENEMY_AMOUNT];
    int playershotsize[SHOT_PLAYER_KIND][2];
    int enemysize[ENEMY_KIND][2];
    int slavesize[ENEMY_KIND][2];
    int enemyshotsize[SHOT_ENEMY_KIND][2];
    int shotdamage[SHOT_ENEMY_KIND];
    int enemycurrentamount = 0;

    int ArrayChecker();
    void ObjectMove();
    void ObjectAtari();
    void EnemySporn(int,int,int);
    void TimeManage();
    void EnemyAction();
    int EnemyBulletStatus(bool,int);
    void BulletAction();
    void PlayerAction(const char key[256]);
    void SuperHand(const char key[256]);
    void SlaveControl(const char key[256]);
    void PlayerShot();
    void JudgeBombered();
    void EnemyDestroy();
    void GameOver();
    void UIDraw();

    GameSystem();
};
//コンストラクタ
GameSystem::GameSystem() {
    player = { 20,240,0,0,64,64,false,0,100,0,0,0 };
    hand.enemy = -1;
    hand.flg = 0;
    enemy[0] = { false,0 };
    slave = { false,0 };
    playershot[SHOT_PLAYER_AMOUNT] = { 0 };
    enemyshot[SHOT_ENEMY_AMOUNT] = { 0 };
    slaveshot[SHOT_SLAVE_AMOUNT] = { 0 };
}

//経過フレーム数計測とフレーム維持
void TimeManager(int *before,int *after,int *be60){
    *after = GetNowCount();
    if(1000/60 > *after - *before) WaitTimer( 1000 / 60 - (*after - *before) );
    *before = GetNowCount();
    if (Framecount % 60 == 0) { 
        Fpsmajor = 60000.0 / (*after - *be60);
        *be60 = *before;
    }
}

//プレイヤーの動きの種類
enum motion_player_num {
    motion_default = 0, motion_forward = 1, motion_back = 2
};

//自機衝突判定
void GameSystem::ObjectAtari() {
    int xif = player.x + player.dx;
    int yif = player.y + player.dy;
    if (xif >= 0 && xif + player.sizex <= 640)
        player.x = xif;
    if (yif >= 0 && yif + player.sizey <= 480) 
        player.y = yif;
}
//プレイヤー行動関数
void GameSystem::PlayerAction(const char key[256]) {
    int i = 0;

    player.dx = 0;
    player.dy = 0;

    //移動
    if (key[KEY_INPUT_UP] == 1 && player.y > 0) {
        player.dy = -4;
    }
    if (key[KEY_INPUT_DOWN] == 1 && player.y + player.sizey < 480) {
        player.dy = 4;
    }
    if (key[KEY_INPUT_RIGHT] == 1 && player.x + player.sizex < 640) {
        player.dx = 4;
    }
    if (key[KEY_INPUT_LEFT] == 1 && player.x > 0) {
        player.dx = -4;
    }
    if (HandState == 3) {
        player.dx /= 2;
        player.dy /= 2;
    }
    else if (HandState == 4) {
        player.dx = 0;
        player.dy = 0;
    }

    ObjectAtari();

    if (player.dx > 0) player.motionnum = motion_forward;
    else if (player.dx < 0) player.motionnum = motion_back;
    else player.motionnum = motion_default;

    if (player.dy == 0) {
        player.actnum = (Framecount % 24) / 8;
    }
    else {
        player.actnum = (Framecount % 15) / 5;
    }

    //発砲
    switch (player.shotstate) {
    case 0:
        if (key[KEY_INPUT_C] == 1) {
            for (i = 0;i < SHOT_PLAYER_AMOUNT;i++) {
                if (playershot[i].memoryflg == false) {
                    playershot[i].flg = true;
                    playershot[i].memoryflg = true;
                    superflag = i;

                    playershot[i].sizex = playershotsize[pb_zero][0];
                    playershot[i].sizey = playershotsize[pb_zero][1];
                    playershot[i].x = player.x + player.sizex / 2;
                    playershot[i].y = player.y + player.sizey / 2 - playershot[i].sizey / 2 - 4;
                    playershot[i].dmg = 5;
                    playershot[i].vel = 20;
                    playershot[i].theta = 0;
                    playershot[i].type = pb_zero;

                    break;
                }
            }
            i++;
            for (i;i < SHOT_PLAYER_AMOUNT;i++) {
                if (playershot[i].memoryflg == false) {
                    playershot[i].memoryflg = true;
                    player.shotnumber = i;
                }
            }
            player.chargetime = 0;
            player.shotstate = 1;
        }
        break;
    case 1:
        player.chargetime++;

        if (key[KEY_INPUT_C] != 1) {
            if (player.chargetime < 6) player.shotstate = 0;
            i = player.shotnumber;
            playershot[i].flg = true;
            if (player.chargetime < 5) {
                playershot[i].type = pb_zero;
                playershot[i].sizex = playershotsize[pb_zero][0];
                playershot[i].sizey = playershotsize[pb_zero][1];
                playershot[i].dmg = 5;
                playershot[i].vel = 20;
            }
            else if (player.chargetime < 20) {
                playershot[i].type = pb_quarter;
                playershot[i].sizex = playershotsize[pb_quarter][0];
                playershot[i].sizey = playershotsize[pb_quarter][1];
                playershot[i].dmg = 15;
                playershot[i].vel = 20;
            }
            else if (player.chargetime < 40) {
                playershot[i].type = pb_half;
                playershot[i].sizex = playershotsize[pb_half][0];
                playershot[i].sizey = playershotsize[pb_half][1];
                playershot[i].dmg = 30;
                playershot[i].vel = 20;
            }
            else if (player.chargetime < 60) {
                playershot[i].type = pb_threequarters;
                playershot[i].sizex = playershotsize[pb_threequarters][0];
                playershot[i].sizey = playershotsize[pb_threequarters][1];
                playershot[i].dmg = 60;
                playershot[i].vel = 20;
            }
            else {
                playershot[i].type = pb_fullcharge;
                playershot[i].sizex = playershotsize[pb_fullcharge][0];
                playershot[i].sizey = playershotsize[pb_fullcharge][1];
                playershot[i].dmg = 100;
                playershot[i].vel = 20;
            }
            playershot[i].x = player.x + player.sizex/2;
            playershot[i].y = player.y + player.sizey / 2 - playershot[i].sizey / 2 - 4;
            playershot[i].theta = 0;
            player.shotstate = 0;
        }
    }

}

//自弾移動＆排除関数
void GameSystem::PlayerShot() {
    int i;
    for (i = 0;i < SHOT_PLAYER_AMOUNT;i++) {
        if (playershot[i].flg == true) {
            playershot[i].x += playershot[i].vel*cos(playershot[i].theta);
            playershot[i].y += playershot[i].vel*sin(playershot[i].theta);
            if (playershot[i].x >= 680) {
                playershot[i].flg = false;
                playershot[i].memoryflg = false;
            }
        }
    }
}

//敵名称
enum enemy_picture_num {
    ZAKO_ROCKET = 0,ZAKO_FISHBORN,ZAKO_TOBIUO,ZAKO_MONORIS,ZAKO_CANNON
};
//グラフィック表示関係クラス
class Picture {
public:
    Picture();
    int pic_player[PLAYER_MOTION][PICTURE_PATTERN_PLAYER] = { 0 };
    int pic_enemy[ENEMY_KIND][PICTURE_PATTERN_ENEMY] = { 0 };
    int pic_slave[ENEMY_KIND][PICTURE_PATTERN_ENEMY] = { 0 };
    int pic_shot_enemy[SHOT_ENEMY_KIND] = { 0 };
    int pic_shot_player[SHOT_PLAYER_KIND] = { 0 };
    int pic_shot_slave[SHOT_ENEMY_KIND] = { 0 };
    void DrawObject(const Player p, const Hand hd, const Enemy sl, const Enemy *e,int amount,
        const Shot *ps, const Shot *ss, const Shot *es, const BG bg1, const BG bg2);
    void PictureSizeGet(int plshotsize[SHOT_PLAYER_KIND][2], int slavesize[ENEMY_KIND][2],int enesize[ENEMY_KIND][2],int shotsize[SHOT_ENEMY_KIND][2]);
};

//初期化時画像格納関数 コンストラクタ
Picture::Picture() {
    //プレイヤー画像
    pic_player[motion_default][0] = LoadGraph("player\\player_center00.bmp");
    pic_player[motion_default][1] = LoadGraph("player\\player_center01.bmp");
    pic_player[motion_default][2] = LoadGraph("player\\player_center02.bmp");
    pic_player[motion_back][0] = LoadGraph("player\\player_back00.bmp");
    pic_player[motion_back][1] = LoadGraph("player\\player_back01.bmp");
    pic_player[motion_back][2] = LoadGraph("player\\player_back02.bmp");
    pic_player[motion_back][3] = LoadGraph("player\\player_back03.bmp");
    pic_player[motion_forward][0] = LoadGraph("player\\player_forward00.bmp");
    pic_player[motion_forward][1] = LoadGraph("player\\player_forward01.bmp");
    pic_player[motion_forward][2] = LoadGraph("player\\player_forward02.bmp");

    //敵画像
    pic_enemy[ZAKO_ROCKET][0]=LoadGraph("zako_rocket00.bmp");
    pic_enemy[ZAKO_ROCKET][1] = LoadGraph("zako_rocket00.bmp");
    pic_enemy[ZAKO_ROCKET][2] = LoadGraph("zako_rocket00.bmp");
    pic_enemy[ZAKO_FISHBORN][0] = LoadGraph("zako_rocket00.bmp");
    pic_enemy[ZAKO_FISHBORN][1] = LoadGraph("zako_rocket00.bmp");
    pic_enemy[ZAKO_FISHBORN][2] = LoadGraph("zako_rocket00.bmp");
    pic_enemy[ZAKO_MONORIS][0] = LoadGraph("enemy\\zako_monoris00.bmp");
    pic_enemy[ZAKO_MONORIS][1] = LoadGraph("enemy\\zako_monoris01.bmp");
    pic_enemy[ZAKO_MONORIS][2] = LoadGraph("enemy\\zako_monoris02.bmp");
    pic_enemy[ZAKO_TOBIUO][0] = LoadGraph("zako_rocket00.bmp");
    pic_enemy[ZAKO_CANNON][0] = LoadGraph("zako_rocket00.bmp");
    pic_enemy[ZAKO_CANNON][1] = LoadGraph("zako_rocket00.bmp");
    pic_enemy[ZAKO_CANNON][2] = LoadGraph("zako_rocket00.bmp");

    //奴隷画像
    pic_slave[ZAKO_ROCKET][0] = LoadGraph("slave_rocket00.bmp");
    pic_slave[ZAKO_ROCKET][1] = LoadGraph("zako_rocket00.bmp");
    pic_slave[ZAKO_ROCKET][2] = LoadGraph("zako_rocket00.bmp");
    pic_slave[ZAKO_FISHBORN][0] = LoadGraph("zako_rocket00.bmp");
    pic_slave[ZAKO_FISHBORN][1] = LoadGraph("zako_rocket00.bmp");
    pic_slave[ZAKO_FISHBORN][2] = LoadGraph("zako_rocket00.bmp");
    pic_slave[ZAKO_MONORIS][0] = LoadGraph("enemy\\slave_monoris02.bmp");
    pic_slave[ZAKO_MONORIS][1] = LoadGraph("enemy\\zako_monoris01.bmp");
    pic_slave[ZAKO_MONORIS][2] = LoadGraph("enemy\\slave_monoris02.bmp");
    pic_slave[ZAKO_TOBIUO][0] = LoadGraph("zako_rocket00.bmp");
    pic_slave[ZAKO_CANNON][0] = LoadGraph("zako_rocket00.bmp");
    pic_slave[ZAKO_CANNON][1] = LoadGraph("zako_rocket00.bmp");
    pic_slave[ZAKO_CANNON][2] = LoadGraph("zako_rocket00.bmp");

    //自弾画像
    pic_shot_player[pb_zero] = LoadGraph("bullet\\p_zero.bmp");
    pic_shot_player[pb_quarter] = LoadGraph("bulletp.bmp");
    pic_shot_player[pb_half] = LoadGraph("");
    pic_shot_player[pb_threequarters] = LoadGraph("");
    pic_shot_player[pb_fullcharge] = LoadGraph("");

    //敵弾画像
    pic_shot_enemy[solidball] = LoadGraph("bullet_enemy01.bmp");

    //奴隷画像
    pic_shot_slave[solidball] = pic_shot_enemy[solidball];
}

//画像サイズ測定格納関数
void Picture::PictureSizeGet(int plshotsize[SHOT_PLAYER_KIND][2],int slavesize[ENEMY_KIND][2], int enesize[ENEMY_KIND][2], int enshotsize[SHOT_ENEMY_KIND][2]) {
    int i;
    for (i = 0;i < SHOT_PLAYER_KIND;i++) {
        GetGraphSize(pic_shot_player[i], &plshotsize[i][0], &plshotsize[i][1]);
    }
    for (i = 0;i < ENEMY_KIND;i++) {
        GetGraphSize(pic_slave[i][0], &slavesize[i][0], &slavesize[i][1]);
    }
    for (i = 0;i < ENEMY_KIND;i++) {
        GetGraphSize(pic_enemy[i][0],&enesize[i][0],&enesize[i][1]);
    }
    for (i = 0;i < SHOT_ENEMY_KIND;i++) {
        GetGraphSize(pic_shot_enemy[i], &enshotsize[i][0], &enshotsize[i][1]);
    }

}

//触手行動関数 書き直したい
void GameSystem::SuperHand(const char key[256]){
    int i;
    static int count = 0;
    double l, r;


    int armx = player.x + PLAYER_ARM_X;
    int army = player.y + PLAYER_ARM_Y;

    if (HandState == 0) {                    //触手待機
        if (key[KEY_INPUT_Z]==1) HandState = 1;
    }
    else if (HandState == 1) {                  //レーザーサイト
        count = 0;

        for (i = 0;i < ENEMY_AMOUNT;i++) {
            if (army >= enemy[i].y && army <= enemy[i].y + enemy[i].sizey && enemy[i].flg==true) {
                hand.enemy = i;
                count++;
            }
        }

        hand.x[0] = armx;
        hand.y[0] = army;
        hand.y[1] = army;
        if (count > 0) {
            hand.x[1] = enemy[hand.enemy].x + enemy[hand.enemy].sizex / 2;
        }
        else { 
            hand.x[1] = 640;
        }
        if (key[KEY_INPUT_Z] == 0) {
            if (hand.enemy >= 0) HandState = 2, hand.number = 0 ;
            else HandState = 0, hand.enemy = -1;
        }
    }
    else if (HandState == 2) {              //触手上り

        hand.number++;
        int delx = enemy[hand.enemy].x + enemy[hand.enemy].sizex / 2 - armx;
        int dely = enemy[hand.enemy].y + enemy[hand.enemy].sizey / 2 - army;
        r = sqrt((double)(delx*delx + dely*dely));
        hand.amount = (int)(r / hand.sizex);
        l = r / (hand.amount + 1);
        //hand.theta = atan((double)(dely / delx));
        if (dely >= 0) hand.theta = acos(delx / r);
        else hand.theta = -acos(delx / r);

        for (i = 0;i < hand.number;i++) {
            hand.x[i] = (int)(armx + (l*i*cos(hand.theta)));
            hand.y[i] = (int)(army + (l*i*sin(hand.theta)));
        }
        DrawFormatString(10, 400, WHITE, "%d,%d,%lf,%lf,%lf", delx, dely, hand.theta, cos(hand.theta), sin(hand.theta));

        if (hand.number >= hand.amount) {

            slave = enemy[hand.enemy];
            slave.actnumber = 0;
            slave.flg = false;

            HandState = 3;

            count = 0;
        }

    }
    else if (HandState == 3) {          //触手接続

        count++;

        int delx = slave.x + slave.sizex / 2 - armx;
        int dely = slave.y + slave.sizey / 2 - army;
        r = sqrt((double)(delx*delx + dely*dely));
        l = r / (hand.amount + 1);
        hand.theta = atan((double)(dely / delx));

        if (dely >= 0) hand.theta = acos(delx / r);
        else hand.theta = -acos(delx / r);

        for (i = 0;i < hand.number;i++) {
            hand.x[i] = (int)(armx + (l*i*cos(hand.theta)));
            hand.y[i] = (int)(army + (l*i*sin(hand.theta)));
        }

        if (count >= 10) {
            enemy[hand.enemy].flg = false;
            enemy[hand.enemy].actnumber = -1;
      //      SlavePicture(sl);
            slave.flg = true;
            HandState = 4;
        }
    }
    else if (HandState == 4) {      //触手下り

        hand.number -= 2;
        if (hand.number < 0) hand.number = 0;

        int delx = slave.x + slave.sizex / 2 - armx;
        int dely = slave.y + slave.sizey / 2 - army;
        r = sqrt((double)(delx*delx + dely*dely));
        l = r / (hand.amount + 1);
        //hand.theta = atan((double)(dely / delx));

        if (r == 0) {
            hand.theta = 0;
        }
        else {
            if (dely >= 0) hand.theta = acos(delx / r);
            else hand.theta = -acos(delx / r);
        }

        for (i = 0;i < hand.number;i++) {
            hand.x[i] = (int)(armx + (l*i*cos(hand.theta)));
            hand.y[i] = (int)(army + (l*i*sin(hand.theta)));
        }

        slave.x = hand.x[hand.number - 1] + slave.sizex / 2;
        slave.y = hand.y[hand.number - 1] - slave.sizey / 2;

        if (hand.number == 0) {

            hand.enemy = -1;
            HandState = 0;
            hand.flg = true;
        }
    }
    else HandState = 0;
    
}

//奴隷制御関数
void GameSystem::SlaveControl(const char key[256]){
    slave.x = player.x + player.sizex - 2;
    slave.y = player.y + (player.sizey / 2 + 4) - slave.sizey / 2;
    switch (slave.enumber) {
    case ZAKO_ROCKET:

        break;
    case ZAKO_FISHBORN:

        break;
    case ZAKO_TOBIUO:

        break;
    case ZAKO_MONORIS:

        break;
    case ZAKO_CANNON:

        break;
    }

    if (key[KEY_INPUT_X]) {
        slave.flg = false;
        hand.flg = false;
    }
}

#if 0
//敵空き配列探し
int GameSystem::ArrayChecker(){
    int i=0;
    for (i = 0;i < ENEMY_AMOUNT;i++) {
        if (enemy[i].flg == false && enemy[i].memoryflg == false) {
            return i;
        }
    }
    return -1;
}
#endif

//敵スポーン関数
void GameSystem::EnemySporn(int enemynumber,int x,int y) {

    int i;
    i = 0;
    while (i < ENEMY_AMOUNT) {
        if (enemy[i].flg == false && enemy[i].memoryflg == false) { break; }
        i++;
    }

    if (i == -1) return;

    enemy[i].flg = true;
    enemy[i].memoryflg = true;
    enemy[i].movetime = 0;
    enemy[i].x = x;
    enemy[i].y = y;
    enemy[i].enumber = enemynumber;
    enemy[i].actnumber = 0;
    switch (enemynumber) {
    case ZAKO_ROCKET:
        enemy[i].life = 5;
        break;
    case ZAKO_FISHBORN:
        enemy[i].life = 5;
        break;
    case ZAKO_MONORIS:
        enemy[i].life = 20;
        break;
    case ZAKO_TOBIUO:
        break;
    case ZAKO_CANNON:
        break;
    }
    enemy[i].flg = true;
    enemy[i].memoryflg = true;
    enemycurrentamount++;
}
//時間割関数
void GameSystem::TimeManage() {
    switch (Framecount) {
    case 200:
        EnemySporn(ZAKO_MONORIS, 640, 120);
        break;
    case 210:
        EnemySporn(ZAKO_MONORIS, 640, 150);
        break;
    }

}



//敵弾生成可否＆ステータス付け
int GameSystem::EnemyBulletStatus(bool wheather_enemy,const int bulletnumber) {
    int i;
    Shot *shot;
    wheather_enemy == true ? (shot = enemyshot) : (shot = slaveshot);
    for (i = 0;i < SHOT_ENEMY_AMOUNT;i++) {
        if ((shot + i)->flg == false && (shot + i)->memoryflg == false) {
            (shot + i)->memoryflg = true;
            (shot + i)->dmg = shotdamage[bulletnumber];
            (shot + i)->sizex = enemyshotsize[i][0];
            (shot + i)->sizey = enemyshotsize[i][1];
            return i;
            break;
        }
    }
    return -1;
}

void GameSystem::ObjectMove() {

}

//自由弾の角度の関数 aは弾元 bは目標
void BulletDegree(double *theta,int ax,int ay,int bx,int by) {
    int delx = bx - ax, dely = by - ay;
    double r = sqrt((double)(delx*delx + dely*dely));
    if(dely>=0) *theta = acos((double)delx/r);
    else *theta = -acos((double)delx / r);
}


//敵行動関数
void GameSystem::EnemyAction() {
    int i,j,k;
    for (i = 0;i < ENEMY_AMOUNT;i++) {
        if (enemy[i].flg == true) {
            enemy[i].movetime++;
            switch (enemy[i].enumber) {
            case ZAKO_ROCKET:

                enemy[i].acttime++;
                if (enemy[i].x > 480)enemy[i].x -= 3;

                //弾制御
                if (enemy[i].acttime % 300 == 0) {
                    j = BulletStatus(true, solidball);
                    if (j >= 0 && enemyshot[j].memoryflg == true) {
                        enemyshot[j].type = solidball;
                        enemyshot[j].x = enemy[i].x + 2;
                        enemyshot[j].y = enemy[i].y + enemy[i].sizey / 2;
                        BulletDegree(&(enemyshot[j].theta), enemyshot[j].x + enemyshot[j].sizex / 2, enemyshot[j].y + enemyshot[j].sizey / 2, player.x + player.sizex / 2, player.y + player.sizey / 2);
                        enemyshot[j].vel = 4;
                    }
                }
                break;
            case ZAKO_FISHBORN:
                break;
            case ZAKO_MONORIS:
                if (enemy[i].movetime < 60) {
                    enemy[i].x -= 1;
                }
                else if (enemy[i].movetime < 100) {
                    enemy[i].x += 1;
                    enemy[i].y -= 3;
                }
                else {
                    enemy[i].x -= 2;
                }
                if (enemy[i].movetime == 60 && enemy[i].movetime == 60) {
                    for (k = 0;k < 6;k++) {
                        j = BulletStatus(true, solidball);
                        enemyshot[j].theta = PI / 6 * (2 * k - 1);
                        enemyshot[j].x = enemy[i].x + enemy[i].sizex / 2;
                        enemyshot[j].y = enemy[i].y + enemy[i].sizey / 2;
                        enemyshot[j].vel = 6;
                    }
                    
                }
                break ;
            case ZAKO_TOBIUO:
                break;
            case ZAKO_CANNON:
                break;
            }
        }
    }

}


//敵弾移動関数
void GameSystem::BulletAction() {
    int i;
    for (i = 0;i < SHOT_ENEMY_AMOUNT;i++) {
        if (enemyshot[i].flg == true) {
            switch (enemyshot[i].type) {
            case solidball:
                enemyshot[i].x += (int)(enemyshot[i].vel*cos(enemyshot[i].theta));
                enemyshot[i].y -= (int)(enemyshot[i].vel*sin(enemyshot[i].theta));
            }
        }
    }
}

//被弾判定処理関数
void GameSystem::JudgeBombered() {
    int i,j;
    if (slave.flg == true) {
        //敵弾-奴隷
        for (i = 0;i < SHOT_ENEMY_AMOUNT;i++) {
            if (enemyshot[i].flg == true && AtariJudge(enemyshot[i].x, enemyshot[i].y, enemyshot[i].sizex, enemyshot[i].sizey, slave.x, slave.y, slave.sizex, slave.sizey, 0, 0)) {
                slave.life -= enemyshot[i].dmg;
                enemyshot[i].dmg = 0;
            }
        }
        //敵-奴隷
        for (i = 0;i < ENEMY_AMOUNT;i++) {
            if (enemy[i].flg == true && AtariJudge(enemy[i].x, enemy[i].y, enemy[i].sizex, enemy[i].sizey, slave.x, slave.y, slave.sizex, slave.sizey, 0, 0)) {
                    slave.life -= enemy[i].life;
                    enemy[i].life = 0;
            }
        }
    }
    //敵弾-自機
    for (i = 0;i < SHOT_ENEMY_AMOUNT;i++) {
       if (enemyshot[i].flg == true && AtariJudge(enemyshot[i].x, enemyshot[i].y, enemyshot[i].sizex, enemyshot[i].sizey, player.x, player.y, player.sizex, player.sizey, 4, 4)) {
           if (player.dmgflg == false) {
            player.life -= enemyshot[i].dmg;
            player.dmgflg = true;
           }
           enemyshot[i].dmg = 0;
            
        }
    }

    for (i = 0;i < ENEMY_AMOUNT;i++) {
        if (enemy[i].flg == true) {
            //自弾-敵機
            for (j = 0;j < SHOT_PLAYER_AMOUNT;j++) {
                if (playershot[j].flg==true && AtariJudge(playershot[j].x, playershot[j].y, playershot[j].sizex, playershot[j].sizey, enemy[i].x, enemy[i].y, enemy[i].sizex, enemy[i].sizey, 0, 0)) {
                    enemy[i].life -= playershot[j].dmg;
                    playershot[j].dmg = 0;
                }
            }
            //奴隷弾-敵機
            for (j = 0;j < SHOT_SLAVE_AMOUNT;j++) {
                if (slaveshot[j].flg == true && AtariJudge(slaveshot[j].x, slaveshot[j].y, slaveshot[j].sizex, slaveshot[j].sizey, enemy[i].x, enemy[i].y, enemy[i].sizex, enemy[i].sizey, 0, 0)) {
                    enemy[i].life -= slaveshot[j].dmg;
                    slaveshot[j].dmg = 0;
                }
            }
        }
    }
    //敵機-自機
    for (i = 0;i < ENEMY_AMOUNT;i++){
        if (enemy[i].flg == true && AtariJudge(enemy[i].x, enemy[i].y, enemy[i].sizex, enemy[i].sizey, player.x, player.y, player.sizex, player.sizey, 4, 4)) {
            if (player.dmgflg == false) {
                player.life -= enemy[i].life;
                player.dmgflg = true;
            }
            enemy[i].life = 0;
        }
    }
}

//敵撃破後処理関数
void EnemyDestroy(Enemy *ene) {
    int i;
    for (i = 0;i < ENEMY_AMOUNT;i++) {
        if (ene->memoryflg = true && ene->flg == false) {
            if (ene->actnumber == -1) {
                ene->enumber = 0;
                ene->x = 0;
                ene->y = 0;
                ene->sizex = 0;
                ene->sizey = 0;
                ene->acttime = 0;
                ene->actnumber = 0;
                ene->memoryflg = false;
            }
            else {

            }
        }
        ene++;
    }
}
void GameSystem::EnemyDestroy() {
    int i;
    for (i = 0;i < ENEMY_AMOUNT;i++) {
        if (enemy[i].life <= 0 && enemy[i].memoryflg == true) {
            
        }
    }
}


//ゲームオーバー
void GameSystem::GameOver() {
    if (player.life <= 0) {
        int i = 0, j = 0;
        //goto使う説ある
        while (keydata[KEY_INPUT_ESCAPE] == 0 && ClearDrawScreen() == 0 && ProcessMessage() == 0) {
            switch (j) {
            case 1:
                i++;
                SetDrawBright(256 - 4 * i, 256 - 4 * i, 256 - 4 * i);
                //ここに描画関数を入れたかった
                if (i >= 64) j = 2;
                break;
            case 2:
                SetDrawBright(0, 0, 0);
            }

            WaitTimer(60 / 1000);
            ScreenFlip();
            
        }
    }
}

//背景画像管理
void BGPicture(BG *bg){
    static int num=1;
    switch (num)
    {
    case 1:
        bg->pic = LoadGraph("stage1_all.png");
        break;
    case 2:
        break;
    }
    
    GetGraphSize(bg->pic, &(bg->sizex),&(bg->sizey));
    //num++;

}

//背景スクロール関数
void BGScrol(BG *bg){

    if (bg->x >= -(bg->sizex) + 640) {
        bg->x -= 1;
    }

/*臨時の措置によりコメントアウト（前期最終発表用）
    bg ->x -= 1;
    if ((bg + 1)->flg == true) ((bg + 1)->x -= 1);

    //次の背景生成
    if (bg->x == -(bg->sizex) + 640 && (bg+1)->flg==false ) {
        BGPicture((bg + 1)); // 次の背景画像

        (bg + 1)->flg = true;
        (bg + 1)->x = 640;
    }

    //主背景の交代
    if ((-(bg->x) >= bg->sizex) && ((bg+1)->flg==true) ) {
        
        DeleteGraph(bg->pic);
        *bg = *(bg + 1);
        (bg + 1)->flg = false;
    }
*/
}

//ゲーム描画関数
void Picture::DrawObject(const Player p,const Hand hd,const Enemy sl , const Enemy *e ,const int amount,
                         const Shot *ps ,const Shot *ss, const Shot *es,
                         const BG bg1,const BG bg2)
    {
    int i,num;

    DrawGraph(bg1.x, bg1.y, bg1.pic, FALSE);//主背景
    if (bg2.flg == true)  DrawGraph(bg2.x, bg2.y, bg2.pic, FALSE);  //次背景
    
    //触手
    if (HandState == 1) DrawLine(hd.x[0], hd.y[0], hd.x[1], hd.y[1], LASERRED); 
    else if (HandState >= 2) {
        for (i = 0;i < hd.number;i++) {
            DrawRotaGraph(hd.x[i], hd.y[i], 1.0, hd.theta, hd.pic, TRUE, FALSE);
        }
    }
    
    DrawGraph(p.x, p.y, pic_player[p.motionnum][p.actnum], TRUE);//プレイヤー

    if (sl.flg == true)  DrawGraph(sl.x, sl.y, pic_slave[sl.enumber][sl.actnumber], TRUE);//奴隷
    
    num = 0;
    for (i = 0;i < ENEMY_AMOUNT;i++) {//雑魚敵
        if ((e + i)->flg == true) {
            DrawGraph((e + i)->x, (e + i)->y, pic_enemy[(e + i)->enumber][(e + i)->actnumber], TRUE);
            num++;
        }
        if (num >= amount)
            break;
    }

    //弾
    for (i = 0;i < SHOT_PLAYER_AMOUNT;i++) {
        if ((ps + i)->flg == true) {
            DrawGraph((ps + i)->x, (ps + i)->y, pic_shot_player[(ps + i)->type], TRUE);
        }
    }
    for (i = 0;i < SHOT_ENEMY_AMOUNT ;i++) {
        if ((es + i)->flg == true) {
            DrawGraph((es + i)->x, (es + i)->y, pic_shot_enemy[(es + i)->type], TRUE);
        }
    }
    for (i = 0;i < SHOT_SLAVE_AMOUNT ;i++) {
        if ((ss + i)->flg == true) {
            DrawGraph((ss + i)->x, (ss + i)->y, pic_shot_enemy[(ss + i)->type], TRUE);
        }
    }

    /*    敵サイズ確認用
    int fig1;
    fig1 = LoadGraph("zako_rocket00.bmp");
    DrawGraph(480,160,fig1,TRUE);
    DrawExtendGraph(450, 230, 450 + 72, 230 + 48, fig1, TRUE);
    DrawExtendGraph(450, 300, 450 + 96, 300 + 64, fig1, TRUE);    */
}

//UI描画関数
void GameSystem::UIDraw() {
    DrawFormatString(10, 5, BLACK, "%d", Framecount);
    DrawFormatString(10, 25, BLACK, "%.2f", Fpsmajor);

    DrawFormatString(600, 10, BLACK, "%d", player.life);
    DrawFormatString(600, 25, WHITE, "%d",superflag);
}


//メイン関数
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR IpCmdLine, int nCmdShow) {


    ChangeWindowMode(TRUE);
    SetMainWindowText("Shooting und ansteckend");
    if (DxLib_Init() == -1)return -1;              //初期化処理　エラーが起きたら直ちに終了    
    SetWindowSizeChangeEnableFlag(TRUE);

    //画面サイズ 640*480
    SetDrawScreen(DX_SCREEN_BACK);

    SetTransColor(255, 255, 255);


    //ゲーム初期化
    //システム変数初期化
    
    int timebefore;
    int timeafter;
    int time60before;
    

    //変数初期化
    GameSystem game;


    BG background[2] = {
        {0,0,1060,480,0,true},
        {0,0,0,480},
    };
    Picture picture;
    picture.PictureSizeGet(game.playershotsize ,game.slavesize, game.enemysize, game.enemyshotsize);

   /* plr.pic[0] = LoadGraph("player_center.bmp");
    plr.pic[PLAYER_BACK] = LoadGraph("player_back.bmp");
    plr.pic[PLAYER_FORWARD] = LoadGraph("player_forward.bmp");*/
    game.hand.pic = LoadGraph("syokusyu02.bmp");
    GetGraphSize(game.hand.pic, &game.hand.sizex, &game.hand.sizey);
    background[0].pic = LoadGraph("stage1_all.png");
    background[1].flg = false;
    GetGraphSize(background[0].pic, &background[0].sizex, &background[0].sizey);
   /* EnemyPicture(enemy);
    SlavePicture(&slave);*/

    //仮タイトル画面
    int cr;
    cr = GetColor(254,255,255);
    DrawString(220, 240 - 32, "Press any buttom", cr);
    ScreenFlip();
    WaitKey();

    //メインループ
    GameState = 1;
    timebefore = GetNowCount();
    time60before = timebefore - 1000;

    while (CheckHitKey(KEY_INPUT_ESCAPE) == 0 && ClearDrawScreen() == 0 && ProcessMessage() == 0) {

        
        GetHitKeyStateAll(keydata);

        BGScrol(background);

        game.ObjectMove();

        game.TimeManage();

        game.EnemyAction();

        if (game.hand.flg == false) game.SuperHand(keydata);
        else game.SlaveControl(keydata);

        game.PlayerShot();

        game.EnemyDestroy();

        game.PlayerAction(keydata);

        picture.DrawObject(game.player, game.hand, game.slave, game.enemy,game.enemycurrentamount,
            game.playershot,game.slaveshot ,game.enemyshot , background[0], background[1]);

        game.UIDraw();

        /*デバッグ
        DrawFormatString(400, 25, WHITE, "%d,%d,%d,%d,%d", background[1].x, background[1].y, background[1].sizex, background[1].flg, background[1].pic);
        DrawFormatString(400, 40, WHITE, "%d,%d,%d,%d,%d", background[0].x, background[0].y, background[0].sizex, background[0].flg, background[0].pic);
        */

        ScreenFlip();

        TimeManager(&timebefore, &timeafter, &time60before);
        Framecount++;

    }


    DxLib_End();                //DXライブラリ使用の終了処理

    return 0;

}

//当たり判定関数. a系が当てる側,b系が食らう側.marginは当たり判定の猶予.返り値は0or1.
int AtariJudge(int ax,int ay,int asizex,int asizey,int bx,int by,int bsizex,int bsizey,int marginx,int marginy){
    if (ax < (bx + bsizex - marginx) && (ax + asizex) > (bx + marginx) && ay < (by + bsizey - marginy) && (ay + asizey) > (by + marginy)) {
        return 1;
    }
    else {
        return 0;
    }
}