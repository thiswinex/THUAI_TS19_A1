#include"communication.h"
#include<vector>
#include <unistd.h>

#include <iostream>
#include <math.h>

extern State* state ;
extern std::vector<State* > all_state;
extern int** ts19_map;
extern bool ts19_flag;

using std::cout;
using std::endl;

void player0();
void player1();

/***
 * AI使用Strategy类控制战略，用Tactic类控制战术，用Method类控制具体操作。
 *      Strategy类通过控制不同tactic的资源分配来达到控制战术的目的。
 *      Strategy类的spyMap()函数用于全局侦测，触发改变战略的扳机。
 *      Tactic类的spyMap()用于侦测地方单位。
 * ***/


//全局变量定义
class Point
{
public:
    Point(int x=0,int y=0, int way_id=-1);
    int x;
    int y;
    int way_id;
    Point *pre;
    Point *next;
};
static Point *sg_way[7][400]; //存储道路链表
static Point *sg_camp_front[7]; //指向每条路的造兵前线的Pos
static Point *sg_tower_front[7]; //指向每条路造塔前线的Pos
static int sg_con_map[200][200]; //存储是否可建造的数组；<=0代表合法，负x代表被x+1个建筑囊括；1~5代表周围建筑数，暂时非法；100代表不在范围内，暂时非法
//不包含道路和主基地信息；在player中初始化
static int sg_waynum; //道路数目
static Position sg_bldqueue[40]; //本回合建造序列
static Position sg_sellqueue[40]; //本回合出售序列
static double sg_tower_def[7][20]; //记录在某路做出防御的数目，防止侦测的时候重复判定一直建塔
static int sg_bldnum_now; //当前建筑数
static int sg_bldlabel_now; //当前建筑标签
static int sg_bldpoint_now; //当前建筑点
static int sg_income_now; //当前每回合收入
static int sg_res_now; //当前资源
static int sg_res_avl; //可花费资源
static int sg_bldpoint_avl; //可花费每回合收入
static int sg_choose_way; //开始时道路选择
static bool sg_wayAttacked[7]; //道路建筑被摧毁标识
static bool sg_sold; //建造家里的Musk防御塔的变量，标识家里的码农建筑是否被售卖

//宏定义
#define MAX_BLDNOW (MAX_BD_NUM+MAX_BD_NUM_PLUS*state->age[ts19_flag]) //当前最大建筑数
#define WITH_AGES *(1+AGE_INCREASE*state->age[ts19_flag]) //和科技等级匹配
#define WITH_LEVELS *(1+state->building[ts19_flag][i].level*AGE_INCREASE)

//枚举类型
enum Stg{ATTACK, DEFEND, SAVE, RUSH, DO_MORE_RES ,DEF_FOR_RUSH_UP, DEF_FOR_RUSH_DOWN}; //策略类型，分别为攻击、防御、攒钱、贴脸和反贴脸

//工具函数
int dist(Point p1, Point p2) //计算两点间距离
{
    int x=p1.x-p2.x;
    int y=p1.y-p2.y;
    if(x<0) x=-x;
    if(y<0) y=-y;
    return x+y;
}
int dist(Position p1, Position p2)  //计算两点间距离重载版本
{
    int x=p1.x-p2.x;
    int y=p1.y-p2.y;
    if(x<0) x=-x;
    if(y<0) y=-y;
    return x+y;
}
void upgradeMax(int unit_id) //升级 **规则所限，一回合只能升一级，所以这个函数GG了**
{
    upgrade(unit_id);
}

class Method
{
public:
    Method();
    
    bool conProg(int &res_avl, int &bld_avl, Point pos); //建造码农
    
    bool conShannon(int w, int &res_avl, int &bld_avl); //建造香农
    bool conThevenin(int w, int &res_avl, int &bld_avl); //建造戴维南
    bool conNorton(int w, int &res_avl, int &bld_avl); //建造诺顿
    bool conVon(int w, int &res_avl, int &bld_avl); //建造冯诺依曼
    bool conLee(int w, int &res_avl, int &bld_avl); //建造伯纳斯李
    bool conGao(int w, int &res_avl, int &bld_avl); //建造高锟
    bool conTuring(int w, int &res_avl, int &bld_avl); //建造图灵
    bool conStark(int w, int &res_avl, int &bld_avl); //建造斯塔克
    
    bool conBoolT(int w, int &res_avl, int &bld_avl); //建造布尔塔
    bool conOhmT(int w, int &res_avl, int &bld_avl); //建造欧姆塔
    bool conMooreT(int w, int &res_avl, int &bld_avl); //建造摩尔塔
    bool conMonteCarloT(int w, int &res_avl, int &bld_avl); //建造蒙特卡罗塔
    bool conLarryT(int w, int &res_avl, int &bld_avl); //建造拉里塔
    bool conRobertT(int w, int &res_avl, int &bld_avl); //建造罗伯特塔
    bool conMuskT(int w, int &res_avl, int &bld_avl); //建造马斯克塔
    bool conMuskT(Point a, int &res_avl, int &bld_avl); //家用马斯克塔建造
    bool conHawkingT(int w, int &res_avl, int &bld_avl); //建造霍金
    
    Position findSpace(int w); //寻找某条道路上的可建筑点(单道路变量)
    Position findSpaceT(int w); //寻找某条道路上塔的可建筑点(单道路变量)
    Position findSpace(Point pos); //寻找某个点附近的可建筑点
    Position findSpaceT(Point pos); //寻找某个点附近塔的可建筑点
    bool checkLegal(Position pos); //检测建筑点是否合法
    void refreshMapLegal(int x, int y, int value); //更新地图上可建造区域
    void refreshMapilLegal(int x, int y, int value); //更新地图上不可建造区域
};

class Tactic
{
public:
    Tactic();
    
    Method method_;
    
    void produceData(int w, int &res_avl, int &bld_avl); //建造数据兵种
    void produceSub(int w, int &res_avl, int &bld_avl); //建造实体兵种
    void produceCharge(int w, int &res_avl, int &bld_avl); //建造冲锋兵种
    void producePush(int w, int &res_avl, int &bld_avl); //建造推塔兵种
    void produceLine(int w, int &res_avl, int &bld_avl); //建造抗线兵种
    void produceTower(int &res_avl, int &bld_avl); //建造塔
    void produceTower(int w, int &res_avl, int &bld_avl); //重载版本
    void produceRes(int &res_avl, int &bld_avl); //建造码农
    void produceMusk(int &res_avl, int &bld_avl); //专用的家用马斯克建造函数
    
    void upgradeFront(int w, int &res_avl, int &bld_avl); //升级前线周围建筑
    void upgradeRes(int &res_avl, int &bld_avl); //升级码农
    void upgradeData(int &res_avl, int &bld_avl); //升级数据兵种
    void upgradeSub(int &res_avl, int &bld_avl); //升级实体兵种
    void upgradeCharge(int &res_avl, int &bld_avl); //升级冲锋兵种
    void upgradePush(int &res_avl, int &bld_avl); //升级推塔兵种
    void upgradeLine(int &res_avl, int &bld_avl); //升级抗线兵种
    void upgradeTower(int &res_avl, int &bld_avl); //升级防御塔
    
    void checkRepair(); //修理检测
    void checkSell(); //售卖检测
    
    void spyMap(); //侦查敌方信息
    Point musk_pos; //家里的马斯克的专用位置
    int musk_pos_id; //记录被变卖码农的ID
    
    double unitInfo_[7][30]; //存储道路上敌方出兵建筑的信息
    double towerInfo_[7][30]; //存储道路上敌方防御塔的信息
};

class Strategy
{
public:
    Strategy();
    
    Tactic tactic_;
    
    //对资源的分配，总为0到1之间的小数，代表当前资源比例
    double res_for_save_;
    double res_for_attack_;
    double res_for_defence_;
    double res_for_moreres_;
    double bld_for_attack_;
    double bld_for_defence_;
    double bld_for_moreres_;
    
    //计数器，计量策略还剩几回合
    int stg_turn_remain_;
    
    //策略状态
    Stg stg_now_;
    bool ifSold;
    bool ifMusk; //是否在家里建造了一个musk
    
    //战略
    void doSave(); //省钱攀科技战略
    void doMoreRes(); //多造码农策略
    void doAttack(); //造兵攻击战略
    void doDefence(); //造塔防御战略
    void defRush(); //对贴脸流的响应
    void normalActive(); //非响应的行动
    
    //侦测扳机
    void spyMap(); //侦测
    
    //初始化函数
    void inputWay(); //初始化sg_way数组
    int findwayNum(); //寻找道路数目
    
};

//////////////////
///以下为类的实现///
//////////////////

//---------------------------------构造函数------------------------------------//

Point::Point(int x, int y, int way_id)
{
    this->x = x;
    this->y = y;
    this->way_id = way_id;
}

Method::Method()
{
    
}

Tactic::Tactic()
{
    for(int i=0;i<7;i++)
        for(int j=0;j<30;j++)
        {
            unitInfo_[i][j] = 0;
            towerInfo_[i][j] = 0;
        }
}

Strategy::Strategy()
{
    ifSold = false;
    ifMusk = false;
}

//-------------------------------------------------------------------------//





//------------------------Strategy中的函数----------------------------------//

int Strategy::findwayNum() //寻找道路数目 (3,5,7) , way[i]的顺序按顺时针增序(player0)
{
    if(ts19_flag == 0)
    {
        int num = 0, j = 0;
        for (int i = 7; i>=0; i--)
            if (ts19_map[7][i] == 1)
            {
                num++;
                sg_way[3-j][0] = new Point(7,i,0);
                j++;
            }
        j=1;
        for (int i = 6; i>=0; i--)
            if (ts19_map[i][7] == 1)
            {
                num++;
                sg_way[3+j][0] = new Point(i,7,0);
                j++;
            }
        return num;
    }
    if(ts19_flag == 1)
    {
        int num = 0, j = 0;
        for (int i = 192; i<=199; i++)
            if (ts19_map[192][i] == 1)
            {
                num++;
                sg_way[3-j][0] = new Point(192,i,0);
                j++;
            }
        j=1;
        for (int i = 193; i<=199; i++)
            if (ts19_map[i][192] == 1)
            {
                num++;
                sg_way[3+j][0] = new Point(i,192,0);
                j++;
            }
        return num;
    }
    return 0;
}

void Strategy::inputWay() //输入道路信息
{
    for(int i=-(sg_waynum-1)/2;i<=(sg_waynum-1)/2;i++)
    {
        sg_camp_front[3+i] = sg_way[3+i][0];
        
        sg_way[3+i][0]->pre = nullptr;
        
        for(int j=0;j<2*MAP_SIZE;j++)
        {
            
            if(ts19_flag == 0)
            {
                if(ts19_map[ sg_way[3+i][j]->x+1 ][ sg_way[3+i][j]->y ] == 1)
                {
                    sg_way[3+i][j+1] = new Point(sg_way[3+i][j]->x+1, sg_way[3+i][j]->y, j+1);
                    sg_way[3+i][j+1]->pre = sg_way[3+i][j];
                    sg_way[3+i][j]->next = sg_way[3+i][j+1];
                    continue;
                }
                if(ts19_map[ sg_way[3+i][j]->x ][ sg_way[3+i][j]->y+1 ] == 1)
                {
                    sg_way[3+i][j+1] = new Point(sg_way[3+i][j]->x, sg_way[3+i][j]->y+1, j+1);
                    sg_way[3+i][j+1]->pre = sg_way[3+i][j];
                    sg_way[3+i][j]->next = sg_way[3+i][j+1];
                    continue;
                }
                
                //找到主基地则停止输入
                if(ts19_map[ sg_way[3+i][j]->x+1 ][ sg_way[3+i][j]->y ] == 2)
                {
                    sg_way[3+i][j]->next = nullptr;
                    break;
                }
                if(ts19_map[ sg_way[3+i][j]->x ][ sg_way[3+i][j]->y+1 ] == 2)
                {
                    sg_way[3+i][j]->next = nullptr;
                    break;
                }
                if(ts19_map[ sg_way[3+i][j]->x+1 ][ sg_way[3+i][j]->y+1 ] == 2)
                {
                    sg_way[3+i][j]->next = nullptr;
                    break;
                }
            }
            if(ts19_flag == 1)
            {
                if(ts19_map[ sg_way[3+i][j]->x-1 ][ sg_way[3+i][j]->y ] == 1)
                {
                    sg_way[3+i][j+1] = new Point(sg_way[3+i][j]->x-1, sg_way[3+i][j]->y, j+1);
                    sg_way[3+i][j+1]->pre = sg_way[3+i][j];
                    sg_way[3+i][j]->next = sg_way[3+i][j+1];
                    continue;
                }
                if(ts19_map[ sg_way[3+i][j]->x ][ sg_way[3+i][j]->y-1 ] == 1)
                {
                    sg_way[3+i][j+1] = new Point(sg_way[3+i][j]->x, sg_way[3+i][j]->y-1, j+1);
                    sg_way[3+i][j+1]->pre = sg_way[3+i][j];
                    sg_way[3+i][j]->next = sg_way[3+i][j+1];
                    continue;
                }
                
                //找到主基地则停止输入
                if(ts19_map[ sg_way[3+i][j]->x-1 ][ sg_way[3+i][j]->y ] == 2)
                {
                    sg_way[3+i][j]->next = nullptr;
                    break;
                }
                if(ts19_map[ sg_way[3+i][j]->x ][ sg_way[3+i][j]->y-1 ] == 2)
                {
                    sg_way[3+i][j]->next = nullptr;
                    break;
                }
                if(ts19_map[ sg_way[3+i][j]->x-1 ][ sg_way[3+i][j]->y-1 ] == 2)
                {
                    sg_way[3+i][j]->next = nullptr;
                    break;
                }

            }
        }

        
    }
    
}

void Strategy::doAttack()
{
    res_for_attack_ = 0.5;
    bld_for_attack_ = 0.5;
    res_for_defence_ = 0.25;
    bld_for_defence_ = 0.25;
    res_for_moreres_ = 0.25;
    bld_for_moreres_ = 0.25;
    stg_now_ = ATTACK;
    
    //更新前线
    for(int i=3-(sg_waynum-1)/2;i<=3+(sg_waynum-1)/2;i++)
    {
        Point *front_pre = sg_camp_front[i];
        sg_camp_front[i] = sg_way[i][300]; //从300开始往前搜索
        for(int j=0;j<320;j++) //往前搜索
        {
            if(sg_con_map[ sg_camp_front[i]->x ][ sg_camp_front[i]->y ] > 0)
                sg_camp_front[i] = &(*sg_camp_front[i]->pre);
            else  break;
        }
    }
    
}

void Strategy::doSave()
{
    res_for_attack_ = 0;
    res_for_defence_ = 0.5;
    bld_for_defence_ = 1;
    res_for_moreres_ = 0;
    stg_now_ = SAVE;
}

void Strategy::doDefence()
{
    res_for_attack_ = 0.2;
    bld_for_attack_ = 0.2;
    res_for_defence_ = 0.5;
    bld_for_defence_ = 0.5;
    res_for_moreres_ = 0.3;
    bld_for_moreres_ = 0.3;
    stg_now_ = DEFEND;

    for(int i=3-(sg_waynum-1)/2;i<=3+(sg_waynum-1)/2;i++)
    {
        Point *front_pre = sg_camp_front[i];
        sg_camp_front[i] = sg_way[i][300]; //从300开始往前搜索
        for(int j=0;j<320;j++) //往前搜索
        {
            if(sg_con_map[ sg_camp_front[i]->x ][ sg_camp_front[i]->y ] > 0)
                sg_camp_front[i] = &(*sg_camp_front[i]->pre);
            else  break;
        }
    }
}

void Strategy::doMoreRes()
{
    res_for_moreres_ = 1;
    bld_for_moreres_ = 1;
    res_for_attack_ = 0;
    bld_for_attack_ = 0;
    res_for_defence_ = 0;
    bld_for_defence_ = 0;
    stg_now_ = DO_MORE_RES;
}

void Strategy::normalActive()
{
    int res_a1 = (sg_res_now * res_for_attack_)*0.6;
    int res_a2 = (sg_res_now * res_for_attack_)*0.0;
    int res_a3 = (sg_res_now * res_for_attack_)*0.4;
    int res_d = sg_res_now * res_for_defence_;
    int res_r = sg_res_now * res_for_moreres_;
    int bld_a1 = (sg_bldpoint_now * bld_for_attack_)*0.6;
    int bld_a2 = (sg_bldpoint_now * bld_for_attack_)*0.0;
    int bld_a3 = (sg_bldpoint_now * bld_for_attack_)*0.4;
    int bld_d = sg_bldpoint_now * bld_for_defence_;
    int bld_r = sg_bldpoint_now * bld_for_moreres_;

    tactic_.upgradeTower(res_d, bld_d);
    tactic_.produceTower(res_d, bld_d);

    int conway = (state->turn % sg_waynum)+3-floor(sg_waynum/2.0);
    if(stg_now_ == ATTACK) //单一道路进攻
        conway = 3-(sg_waynum-1)/2;
    tactic_.upgradePush(res_a1, bld_a1);
    tactic_.producePush(conway, res_a1, bld_a1);

    //建造家里的Musk
    if(!ifMusk)
    {
        tactic_.produceMusk(res_d, bld_d);
        ifMusk = true;
    }


    if(stg_now_ == ATTACK)
    {
        tactic_.upgradePush(res_a1, bld_a1);
        tactic_.producePush(conway+1, res_a1, bld_a1);
        
        tactic_.upgradePush(res_a1, bld_a1);
        tactic_.producePush(conway+2, res_a1, bld_a1);
        
        if(sg_waynum >= 5)
        {
            tactic_.upgradePush(res_a1, bld_a1);
            tactic_.producePush(conway+3, res_a1, bld_a1);
            tactic_.upgradePush(res_a1, bld_a1);
            tactic_.producePush(conway+4, res_a1, bld_a1);
        }
        if(sg_waynum == 7)
        {
            tactic_.upgradePush(res_a1, bld_a1);
            tactic_.producePush(conway+5, res_a1, bld_a1);
            tactic_.upgradePush(res_a1, bld_a1);
            tactic_.producePush(conway+6, res_a1, bld_a1);
        }
    }

    tactic_.upgradeCharge(res_a2, bld_a2);
    tactic_.produceCharge(conway,res_a2, bld_a2);
    if(stg_now_ == ATTACK)
    {
        tactic_.upgradeCharge(res_a2, bld_a2);
        tactic_.produceCharge(conway+1,res_a2, bld_a2);
        
        tactic_.produceCharge(conway+2,res_a2, bld_a2);
        if(sg_waynum >= 5)
        {
            tactic_.produceCharge(conway+3, res_a2, bld_a2);
            tactic_.produceCharge(conway+4, res_a2, bld_a2);
        }
        if(sg_waynum == 7)
        {
            tactic_.produceCharge(conway+5, res_a2, bld_a2);
            tactic_.produceCharge(conway+6, res_a2, bld_a2);
        }
    }
    tactic_.upgradeLine(res_a3, bld_a3);
    tactic_.produceLine(conway,res_a3, bld_a3);
    if(stg_now_ == ATTACK)
    {
        tactic_.upgradeLine(res_a3, bld_a3);
        tactic_.produceLine(conway+1,res_a3, bld_a3);

        tactic_.produceLine(conway+2,res_a3, bld_a3);
        if(sg_waynum >= 5)
        {
            tactic_.produceLine(conway+3, res_a3, bld_a3);
            tactic_.produceLine(conway+4, res_a3, bld_a3);
        }
        if(sg_waynum == 7)
        {
            tactic_.produceLine(conway+5, res_a3, bld_a3);
            tactic_.produceLine(conway+6, res_a3, bld_a3);
        }
    }
    tactic_.upgradeRes(res_r, bld_r);
    tactic_.produceRes(res_r, bld_r);
    int res_n, bld_n;
    res_n = res_r;//state->resource[ts19_flag].resource;
    bld_n = bld_r;//state->resource[ts19_flag].building_point;
    tactic_.upgradeRes(res_n, bld_n);
    tactic_.produceRes(res_n, bld_n);
    if(stg_now_ == DO_MORE_RES)
    {
        tactic_.produceRes(res_n, bld_n);
        tactic_.produceRes(res_n, bld_n);
        tactic_.produceRes(res_n, bld_n);
        tactic_.produceRes(res_n, bld_n);
        tactic_.produceRes(res_n, bld_n);
        tactic_.produceRes(res_n, bld_n);
        tactic_.produceRes(res_n, bld_n);
        tactic_.produceRes(res_n, bld_n);
        tactic_.produceRes(res_n, bld_n);
    }
    
}

void Strategy::defRush()
{
    if(stg_now_ == DEF_FOR_RUSH_UP)
    {
        int res_d, res_a1, res_a2, res_r, bld_d, bld_a1, bld_a2, bld_r;
        for(int i=3-(sg_waynum-1)/2;i<=3+(sg_waynum-1)/2;i++)
        {
            sg_camp_front[i] = sg_way[i][2];
        }
        if(state->age[ts19_flag] < 1)
        {
            res_d = sg_res_now * 0;
            res_a1 = sg_res_now * 0.1;
            res_r = sg_res_now * 0.1;
            bld_d = sg_bldpoint_now * 0;
            bld_a1 = sg_bldpoint_now * 0.5;
            bld_r = sg_bldlabel_now * 0.5;

        }
        if(state->age[ts19_flag] >= 1)
        {
            res_d = sg_res_now * 0.5;
            res_a1 = sg_res_now * 0.2;
            res_a2 = sg_res_now * 0.2;
            res_r = sg_res_now * 0.1;
            bld_d = sg_bldpoint_now * 0.5;
            bld_a1 = sg_bldpoint_now * 0.2;
            bld_a2 = sg_bldpoint_now * 0.2;
            bld_r = sg_bldlabel_now * 0.1;
        }
        tactic_.upgradePush(res_a2, bld_a2);
        tactic_.upgradeTower(res_d, bld_d);
        tactic_.upgradeRes(res_r, bld_r);
        tactic_.produceTower(3-(sg_waynum-1)/2, res_d, bld_d);
        tactic_.producePush(3-(sg_waynum-1)/2, res_a1, bld_a1);
        tactic_.producePush(4-(sg_waynum-1)/2, res_a2, bld_a2);
        tactic_.producePush(5-(sg_waynum-1)/2, res_a1, bld_a1);
        tactic_.producePush(5-(sg_waynum-1)/2, res_a2, bld_a2);
        tactic_.produceRes(res_r, bld_r);
    }
    if(stg_now_ == DEF_FOR_RUSH_DOWN)
    {
        int res_d, res_a1, res_a2, res_r, bld_d, bld_a1, bld_a2, bld_r;
        for(int i=3-(sg_waynum-1)/2;i<=3+(sg_waynum-1)/2;i++)
        {
            sg_camp_front[i] = sg_way[i][2];
        }
        if(state->age[ts19_flag] < 1)
        {
            res_d = sg_res_now * 0;
            res_a1 = sg_res_now * 0.1;
            res_r = sg_res_now * 0.1;
            bld_d = sg_bldpoint_now * 0;
            bld_a1 = sg_bldpoint_now * 0.5;
            bld_r = sg_bldlabel_now * 0.5;

        }
        if(state->age[ts19_flag] >= 1)
        {
            res_d = sg_res_now * 0.5;
            res_a1 = sg_res_now * 0.2;
            res_a2 = sg_res_now * 0.2;
            res_r = sg_res_now * 0.1;
            bld_d = sg_bldpoint_now * 0.5;
            bld_a1 = sg_bldpoint_now * 0.2;
            bld_a2 = sg_bldpoint_now * 0.2;
            bld_r = sg_bldlabel_now * 0.1;
        }
        tactic_.upgradePush(res_a2, bld_a2);
        tactic_.upgradeTower(res_d, bld_d);
        tactic_.upgradeRes(res_r, bld_r);
        tactic_.produceTower(3+(sg_waynum-1)/2, res_d, bld_d);
        tactic_.producePush(3+(sg_waynum-1)/2, res_a1, bld_a1);
        tactic_.producePush(2+(sg_waynum-1)/2, res_a2, bld_a2);
        tactic_.producePush(1+(sg_waynum-1)/2, res_a1, bld_a1);
        tactic_.producePush(1+(sg_waynum-1)/2, res_a2, bld_a2);
        tactic_.produceRes(res_r, bld_r);

    }
}

void Strategy::spyMap() //侦测
{
    //侦测贴脸  //贴脸已经规则被弄死了
    //弄死个屁
    if(state->turn < 40)
    {
        for(int i=1;i < state->building[!ts19_flag].size();i++)
        {
            if(ts19_flag == 0)
            {
                if(state->building[!ts19_flag][i].pos.x + state->building[!ts19_flag][i].pos.y <= 190) //如果地方建筑出现在地图的左上半边
                {
                    //以下算法基于对方大概率会从地图最边界处的道路进行攻击
                    if(state->building[!ts19_flag][i].pos.x > 90)
                    {
                        stg_now_ = DEF_FOR_RUSH_UP;
                    }
                    if(state->building[!ts19_flag][i].pos.x <= 110)
                    {
                        stg_now_ = DEF_FOR_RUSH_DOWN;
                    }
                }
            }
            if(ts19_flag == 1)
            {
                if(state->building[!ts19_flag][i].pos.x + state->building[!ts19_flag][i].pos.y >= 210) //如果地方建筑出现在地图的右下半边
                {
                    //以下算法基于对方大概率会从地图最边界处的道路进行攻击
                    if(state->building[!ts19_flag][i].pos.x > 110)
                    {
                        stg_now_ = DEF_FOR_RUSH_DOWN;
                    }
                    if(state->building[!ts19_flag][i].pos.x <= 90)
                    {
                        stg_now_ = DEF_FOR_RUSH_UP;
                    }
                }
            }
        }
    }

    //侦测地图
    tactic_.spyMap();
    
    //策略扳机
    ifSold = false;
    if(state->building[ts19_flag].size() >= MAX_BLDNOW) //建筑满了就攒钱升级 / 5级科技就卖东西
    {
        if(state->age[ts19_flag] == 5)
        {
            for(int i=1;i<state->building[ts19_flag].size();i++)
            {
                if(state->building[ts19_flag][i].building_type == Shannon
                        || state->building[ts19_flag][i].building_type == Thevenin
                        || state->building[ts19_flag][i].building_type == Bool)
                {
                    sell(state->building[ts19_flag][i].unit_id);
                    if(!ifSold)
                    {
                        construct(Musk,Position(state->building[ts19_flag][i].pos.x,state->building[ts19_flag][i].pos.y));
                        ifSold = true;
                    }
                }
            }
        }
        else doSave();
    }

    //维修检测
    tactic_.checkRepair();
    
    //防御前线跳跃

    for(int i=3-(sg_waynum-1)/2;i<=3+(sg_waynum-1)/2;i++)
    {
        if(sg_wayAttacked[i] == false) //遭受攻击则不再往前寻找
        {
            Point *front_pre = sg_tower_front[i];
            sg_tower_front[i] = sg_way[i][370]; //从370开始往前搜索

            for(int j=0;j<370;j++) //往前搜索
            {
                if(sg_con_map[ sg_tower_front[i]->x ][ sg_tower_front[i]->y ] > 0)
                    sg_tower_front[i] = &(*sg_tower_front[i]->pre);
                else
                    break;
            }
            if(state->turn > 0 && state->turn <= 17) //前17回合塔前线回缩3格
            {
                for(int j=0;j<3;j++)
                    if(sg_tower_front[i]->pre != nullptr) sg_tower_front[i] = &(*sg_tower_front[i]->pre);
            }

            if(state->turn > 17) //17回合后塔前线回缩5格
            {
                for(int j=0;j<5;j++)
                    if(sg_tower_front[i]->pre != nullptr) sg_tower_front[i] = &(*sg_tower_front[i]->pre);
            }

        }
    }
}

//-------------------------------------------------------------------------//




//--------------------------Tactic中的函数----------------------------------//
void Tactic::produceMusk(int &res_avl, int &bld_avl) //家用马斯克建造函数
{
    if(state->age[ts19_flag] == 5)
    {
        for(int i=1;i<state->building[ts19_flag].size();i++)
        {
            if(state->building[ts19_flag][i].pos.x == musk_pos.x
                    && state->building[ts19_flag][i].pos.y == musk_pos.y)
                sell(state->building[ts19_flag][i].unit_id);
        }
        method_.conMuskT(musk_pos,res_avl, bld_avl);
    }
}

void Tactic::produceData(int w, int &res_avl, int &bld_avl) //建造数据兵种
{
    if (state->age[ts19_flag] < 3)
        method_.conShannon(w, res_avl, bld_avl);
    
    if (state->age[ts19_flag] == 3)
        method_.conLee(w, res_avl, bld_avl);
    
    if (state->age[ts19_flag] == 4)
        method_.conTuring(w, res_avl, bld_avl);
    
}

void Tactic::produceSub(int w, int &res_avl, int &bld_avl) //建造实体兵种
{
    if (state->age[ts19_flag] == 1)
        method_.conThevenin(w, res_avl, bld_avl);
    
    if (state->age[ts19_flag] == 2)
        method_.conVon(w, res_avl, bld_avl);
    
    if (state->age[ts19_flag] == 3)
        method_.conGao(w, res_avl, bld_avl);
    
    if (state->age[ts19_flag] == 4)
        method_.conStark(w, res_avl, bld_avl);
    
}

void Tactic::produceCharge(int w, int &res_avl, int &bld_avl) //建造冲锋兵种
{
    if(state->age[ts19_flag] >= 1 && state->age[ts19_flag] < 4)
        method_.conNorton(w, res_avl, bld_avl);
    if(state->age[ts19_flag] >= 4)
        method_.conLee(w, res_avl, bld_avl);
    
}

void Tactic::producePush(int w, int &res_avl, int &bld_avl) //建造推塔兵种
{
    bool ifCon = false;
    while(res_avl >= 0 && bld_avl>= 0)
    {
        if(state->age[ts19_flag] == 0)
            ifCon = method_.conShannon(w, res_avl, bld_avl);
        if(state->age[ts19_flag] >= 1 && state->age[ts19_flag] < 4)
            ifCon = method_.conThevenin(w, res_avl, bld_avl);
        if(state->age[ts19_flag] == 5)
            ifCon = method_.conStark(w, res_avl, bld_avl);
        if(state->age[ts19_flag] >= 4)
        {
            ifCon = method_.conGao(w, res_avl, bld_avl);
        }

        if(!ifCon) break;
    }
}

void Tactic::produceLine(int w, int &res_avl, int &bld_avl) //建造抗线兵种
{
    bool ifCon = false;
    while(res_avl >= 0 && bld_avl >= 0)
    {
        if(state->age[ts19_flag] >= 2 && state->age[ts19_flag] <= 5)
            ifCon = method_.conVon(w, res_avl, bld_avl);
        if(state->age[ts19_flag] == 5)
            ifCon = method_.conTuring(w, res_avl, bld_avl);
        if(!ifCon)  break;
    }
}

void Tactic::produceTower(int w, int &res_avl, int &bld_avl) //建造塔
{
    
    bool ifCon = false;


    if(unitInfo_[w][Shannon] > 0)
    {
        if(state->age[ts19_flag] >= 0 && state->age[ts19_flag] < 4)
        {
            ifCon = method_.conBoolT(w, res_avl, bld_avl);
            if(ifCon) sg_tower_def[w][Shannon] += 1.15;
        }
        if(state->age[ts19_flag] >= 4)
        {
            ifCon = method_.conLarryT(w, res_avl, bld_avl);
            if(ifCon) sg_tower_def[w][Shannon] += 2.2;
        }
    }
    if(unitInfo_[w][Thevenin] > 0)
    {
        if(state->age[ts19_flag] >= 1)
        {
            ifCon = method_.conOhmT(w, res_avl, bld_avl);
            if(ifCon) sg_tower_def[w][Thevenin] += 1.25;
            if(unitInfo_[w][Norton] > 0)
            {
                if(ifCon) sg_tower_def[w][Thevenin] += 3;
                if(ifCon) sg_tower_def[w][Norton] += 3;
            }
        }
        
    }
    if(unitInfo_[w][Norton] > 0)
    {
        if(state->age[ts19_flag] == 5)
        {
            ifCon = method_.conMuskT(w, res_avl, bld_avl);
            if(ifCon) sg_tower_def[w][Berners_Lee] += 3;
            if(ifCon) sg_tower_def[w][Turing] += 2;
            if(ifCon) sg_tower_def[w][Von_Neumann] += 3;
            if(ifCon) sg_tower_def[w][Norton] += 3;
        }
        if(state->age[ts19_flag] >= 1)
        {
            ifCon = method_.conOhmT(w, res_avl, bld_avl);
            if(ifCon) sg_tower_def[w][Norton] += 0.65;
        }
    }
    if(unitInfo_[w][Von_Neumann] > 0)
    {
        if(state->age[ts19_flag] >= 2)
        {
            ifCon = method_.conMooreT(w, res_avl, bld_avl);
            if(ifCon) sg_tower_def[w][Von_Neumann] += 1.25;
        }
    }
    if(unitInfo_[w][Berners_Lee] > 0)
    {
        if(state->age[ts19_flag] == 5)
        {
            ifCon = method_.conMuskT(w, res_avl, bld_avl);
            if(ifCon) sg_tower_def[w][Berners_Lee] += 3;
            if(ifCon) sg_tower_def[w][Turing] += 2;
            if(ifCon) sg_tower_def[w][Von_Neumann] += 3;
            if(ifCon) sg_tower_def[w][Norton] += 3;
        }
        if(state->age[ts19_flag] >= 4)
        {
            ifCon = method_.conLarryT(w,res_avl,bld_avl);
            if(ifCon) sg_tower_def[w][Berners_Lee] += 0.5;
        }
        if(state->age[ts19_flag] < 4)
        {
            ifCon = method_.conBoolT(w,res_avl,bld_avl);
            if(ifCon) sg_tower_def[w][Berners_Lee] += 0.15;
        }
    }
    if(unitInfo_[w][Kuen_Kao] > 0)
    {
        if(state->age[ts19_flag] >= 3)
        {
            ifCon = method_.conMonteCarloT(w, res_avl, bld_avl);
            if(ifCon) sg_tower_def[w][Kuen_Kao] += 1.25;
        }
    }
    if(unitInfo_[w][Turing] > 0)
    {
        if(state->age[ts19_flag] == 5)
        {
            ifCon = method_.conMuskT(w, res_avl, bld_avl);
            if(ifCon) sg_tower_def[w][Berners_Lee] += 3;
            if(ifCon) sg_tower_def[w][Turing] += 2;
            if(ifCon) sg_tower_def[w][Von_Neumann] += 3;
            if(ifCon) sg_tower_def[w][Norton] += 3;
        }
        if(state->age[ts19_flag] >= 4)
        {
            ifCon = method_.conRobertT(w,res_avl,bld_avl);
            if(ifCon) sg_tower_def[w][Turing] += 1.25;
        }
        
    }
    if(unitInfo_[w][Tony_Stark] > 0)
    {
        if(state->age[ts19_flag] >= 5)
        {
            ifCon = method_.conMuskT(w,res_avl,bld_avl);
            if(ifCon) sg_tower_def[w][Tony_Stark] += 1.25;
        }
        if(state->age[ts19_flag] >= 3 && state->age[ts19_flag] < 5)
        {
            ifCon = method_.conMonteCarloT(w,res_avl,bld_avl);
            if(ifCon) sg_tower_def[w][Tony_Stark] += 0.25;
        }
    }

}

void Tactic::produceTower(int &res_avl, int &bld_avl) //建造塔重载
{
    for(int w=3-(sg_waynum-1)/2;w<=3+(sg_waynum-1)/2;w++)
    {
        bool ifCon = false;
        if(unitInfo_[w][Shannon] > 0)
        {
            if(state->age[ts19_flag] >= 0 && state->age[ts19_flag] < 4)
            {
                ifCon = method_.conBoolT(w, res_avl, bld_avl);
                if(ifCon) sg_tower_def[w][Shannon] += 1.15;
            }
            if(state->age[ts19_flag] >= 4)
            {
                ifCon = method_.conLarryT(w, res_avl, bld_avl);
                if(ifCon) sg_tower_def[w][Shannon] += 2.2;
            }
        }
        if(unitInfo_[w][Thevenin] > 0)
        {
            if(state->age[ts19_flag] >= 1)
            {
                ifCon = method_.conOhmT(w, res_avl, bld_avl);
                if(ifCon) sg_tower_def[w][Thevenin] += 1.25;
                if(unitInfo_[w][Norton] > 0)
                {
                    if(ifCon) sg_tower_def[w][Thevenin] += 3;
                    if(ifCon) sg_tower_def[w][Norton] += 3;
                }
            }
        }
        if(unitInfo_[w][Norton] > 0)
        {
            if(state->age[ts19_flag] == 5)
            {
                ifCon = method_.conMuskT(w, res_avl, bld_avl);
                if(ifCon) sg_tower_def[w][Berners_Lee] += 3;
                if(ifCon) sg_tower_def[w][Turing] += 2;
                if(ifCon) sg_tower_def[w][Von_Neumann] += 3;
                if(ifCon) sg_tower_def[w][Norton] += 3;
            }
            if(state->age[ts19_flag] >= 1)
            {
                ifCon = method_.conOhmT(w, res_avl, bld_avl);
                if(ifCon) sg_tower_def[w][Norton] += 0.85;
            }
        }
        if(unitInfo_[w][Von_Neumann] > 0)
        {
            if(state->age[ts19_flag] == 5)
            {
                ifCon = method_.conMuskT(w, res_avl, bld_avl);
                if(ifCon) sg_tower_def[w][Von_Neumann] += 4;
            }
            if(state->age[ts19_flag] >= 2)
            {
                ifCon = method_.conMooreT(w, res_avl, bld_avl);
                if(ifCon) sg_tower_def[w][Von_Neumann] += 1.25;
            }

        }
        if(unitInfo_[w][Berners_Lee] > 0)
        {
            if(state->age[ts19_flag] >= 4)
            {
                ifCon = method_.conLarryT(w,res_avl,bld_avl);
                if(ifCon) sg_tower_def[w][Berners_Lee] += 0.5;
            }
            if(state->age[ts19_flag] < 4)
            {
                ifCon = method_.conBoolT(w,res_avl,bld_avl);
                if(ifCon) sg_tower_def[w][Berners_Lee] += 0.15;
            }
        }
        if(unitInfo_[w][Kuen_Kao] > 0)
        {
            if(state->age[ts19_flag] == 5)
            {
                ifCon = method_.conMuskT(w, res_avl, bld_avl);
                if(ifCon) sg_tower_def[w][Berners_Lee] += 3;
                if(ifCon) sg_tower_def[w][Turing] += 2;
                if(ifCon) sg_tower_def[w][Von_Neumann] += 3;
                if(ifCon) sg_tower_def[w][Norton] += 3;
            }
            if(state->age[ts19_flag] >= 3)
            {
                ifCon = method_.conMonteCarloT(w, res_avl, bld_avl);
                if(ifCon) sg_tower_def[w][Kuen_Kao] += 1.25;
            }
        }
        if(unitInfo_[w][Turing] > 0)
        {
            if(state->age[ts19_flag] == 5)
            {
                ifCon = method_.conMuskT(w, res_avl, bld_avl);
                if(ifCon) sg_tower_def[w][Berners_Lee] += 3;
                if(ifCon) sg_tower_def[w][Turing] += 2;
                if(ifCon) sg_tower_def[w][Von_Neumann] += 3;
                if(ifCon) sg_tower_def[w][Norton] += 3;
            }
            if(state->age[ts19_flag] >= 4)
            {
                ifCon = method_.conRobertT(w,res_avl,bld_avl);
                if(ifCon) sg_tower_def[w][Turing] += 1.25;
            }
            
        }
        if(unitInfo_[w][Tony_Stark] > 0)
        {
            if(state->age[ts19_flag] >= 5)
            {
                ifCon = method_.conMuskT(w,res_avl,bld_avl);
                if(ifCon) sg_tower_def[w][Tony_Stark] += 1.25;
            }
            if(state->age[ts19_flag] >= 3 && state->age[ts19_flag] < 5)
            {
                ifCon = method_.conMonteCarloT(w,res_avl,bld_avl);
                if(ifCon) sg_tower_def[w][Tony_Stark] += 0.25;
            }
        }
        if(!ifCon) continue;
    }
}

void Tactic::produceRes(int &res_avl, int &bld_avl) //建造码农
{
    bool ifCon=false;
    if(state->turn == 0) sg_choose_way = 3-(sg_waynum-1)/2;
    while(res_avl >= 100 && bld_avl >= 10)
    {
        if(ts19_flag == 0)
        {

            //计算码农的前线
            Point p_up, p_middle, p_down;
            for(int i=192;i>=7;i--)
                if(method_.checkLegal(Position(i,state->turn/5)))
                {
                    p_up = Point(i,state->turn/5);
                    break;
                }

            bool ifFound = false;
            for(int i=192;i>=7;i--)
            {
                if(method_.checkLegal(Position(i,i)))
                {
                    p_middle = Point(i,i);
                    if(ifFound) break;
                    ifFound = true;
                }
                if(method_.checkLegal(Position(i-1,i)))
                {
                    p_middle = Point(i-1,i);
                    if(ifFound) break;
                    ifFound = true;
                }
                if(method_.checkLegal(Position(i-2,i)))
                {
                    p_middle = Point(i-2,i);
                    if(ifFound) break;
                    ifFound = true;
                }
                if(method_.checkLegal(Position(i,i-1)))
                {
                    p_middle = Point(i,i-1);
                    if(ifFound) break;
                    ifFound = true;
                }
                if(method_.checkLegal(Position(i,i-2)))
                {
                    p_middle = Point(i,i-2);
                    if(ifFound) break;
                    ifFound = true;
                }
            }
            for(int i=192;i>=7;i--)
                if(method_.checkLegal(Position(state->turn/5,i)))
                {
                    p_down = Point(state->turn/5,i);
                    break;
                }




            if(state->turn % 3 == 0)
                ifCon = method_.conProg(res_avl, bld_avl, p_up);
            else if(state->turn % 3 == 1)
            {
                ifCon = method_.conProg(res_avl, bld_avl, p_down);
            }
            else
            {
                ifCon =method_.conProg(res_avl, bld_avl, p_middle);
                if(state->turn < 3)
                {
                    musk_pos = p_middle;
                }
            }

            if(!ifCon) break;
        }
        if(ts19_flag == 1)
        {

            Point p_up, p_middle, p_down;
            for(int i=7;i<193;i++)
                if(method_.checkLegal(Position(i,199-state->turn/5)))
                {
                    p_up = Point(i,199-state->turn/5);
                    break;
                }

            //用来控制推慢一点
            bool ifFound = false;

            for(int i=7;i<193;i++)
            {


                if(method_.checkLegal(Position(i,i)))
                {
                    p_middle = Point(i,i);
                    if(ifFound) break;
                    ifFound = true;
                }
                if(method_.checkLegal(Position(i+1,i)))
                {
                    p_middle = Point(i+1,i);
                    if(ifFound) break;
                    ifFound = true;
                }
                if(method_.checkLegal(Position(i+2,i)))
                {
                    p_middle = Point(i+2,i);
                    if(ifFound) break;
                    ifFound = true;
                }
                if(method_.checkLegal(Position(i,i+1)))
                {
                    p_middle = Point(i,i+1);
                    if(ifFound) break;
                    ifFound = true;
                }
                if(method_.checkLegal(Position(i,i+2)))
                {
                    p_middle = Point(i,i+2);
                    if(ifFound) break;
                    ifFound = true;
                }
            }
            for(int i=7;i<193;i++)
            {
                if(method_.checkLegal(Position(199-state->turn/5,i)))
                {
                    p_down = Point(199-state->turn/5,i);
                    break;
                }

            }

                if(state->turn % 3 == 0)
                    ifCon = method_.conProg(res_avl, bld_avl, p_up);
                else if(state->turn % 3 == 1)
                    ifCon = method_.conProg(res_avl, bld_avl, p_down);
                else
                    ifCon =method_.conProg(res_avl, bld_avl, p_middle);

            if(!ifCon) break;
        }
    }
}

void Tactic::upgradeData(int &res_avl, int &bld_avl) // 升级数据
{
    for(int i=1;i<state->building[ts19_flag].size();i++)
    {
        if(state->building[ts19_flag][i].building_type == Shannon
                || state->building[ts19_flag][i].building_type == Berners_Lee
                || state->building[ts19_flag][i].building_type == Turing)
        {
            if(state->building[ts19_flag][i].level < state->age[ts19_flag])
            {
                upgradeMax(state->building[ts19_flag][i].unit_id);
                if(state->building[ts19_flag][i].building_type == Shannon) { res_avl -= 75; bld_avl -= 7.5; }
                if(state->building[ts19_flag][i].building_type == Berners_Lee) { res_avl -= 125; bld_avl -= 12.5; }
                if(state->building[ts19_flag][i].building_type == Turing) { res_avl -= 300; bld_avl -= 30; }
            }
        }
        if(res_avl <= 0 || bld_avl <= 0) break;
    }
}

void Tactic::upgradeSub(int &res_avl, int &bld_avl) // 升级实体
{
    for(int i=1;i<state->building[ts19_flag].size();i++)
    {
        if(state->building[ts19_flag][i].building_type == Norton
                || state->building[ts19_flag][i].building_type == Thevenin
                || state->building[ts19_flag][i].building_type == Von_Neumann
                || state->building[ts19_flag][i].building_type == Kuen_Kao
                || state->building[ts19_flag][i].building_type == Tony_Stark)
        {
            if(state->building[ts19_flag][i].level < state->age[ts19_flag])
            {
                upgradeMax(state->building[ts19_flag][i].unit_id);
                if(state->building[ts19_flag][i].building_type == Norton) { res_avl -= 80; bld_avl -= 8; }
                if(state->building[ts19_flag][i].building_type == Thevenin) { res_avl -= 80; bld_avl -= 8; }
                if(state->building[ts19_flag][i].building_type == Von_Neumann) { res_avl -= 100; bld_avl -= 10; }
                if(state->building[ts19_flag][i].building_type == Kuen_Kao) { res_avl -= 200; bld_avl -= 20; }
                if(state->building[ts19_flag][i].building_type == Tony_Stark) { res_avl -= 300; bld_avl -= 30; }
            }
        }
        if(res_avl <= 0 || bld_avl <= 0) break;
    }
}

void Tactic::upgradePush(int &res_avl, int &bld_avl) // 升级推塔
{
    for(int i=1;i<state->building[ts19_flag].size();i++)
    {
        if(state->building[ts19_flag][i].building_type == Shannon
                || state->building[ts19_flag][i].building_type == Thevenin
                || state->building[ts19_flag][i].building_type == Kuen_Kao
                || state->building[ts19_flag][i].building_type == Tony_Stark)
        {
            if(state->building[ts19_flag][i].level < state->age[ts19_flag])
            {
                upgradeMax(state->building[ts19_flag][i].unit_id);
                if(state->building[ts19_flag][i].building_type == Shannon) { res_avl -= 75; bld_avl -= 7.5; }
                if(state->building[ts19_flag][i].building_type == Thevenin) { res_avl -= 80; bld_avl -= 8; }
                if(state->building[ts19_flag][i].building_type == Kuen_Kao) { res_avl -= 200; bld_avl -= 20; }
                if(state->building[ts19_flag][i].building_type == Tony_Stark) { res_avl -= 300; bld_avl -= 30; }
            }
        }
        if(res_avl <= 0 || bld_avl <= 0) break;
    }
}

void Tactic::upgradeLine(int &res_avl, int &bld_avl) // 升级抗线
{
    for(int i=1;i<state->building[ts19_flag].size();i++)
    {
        if(state->building[ts19_flag][i].building_type == Von_Neumann
                || state->building[ts19_flag][i].building_type == Turing)
        {
            if(state->building[ts19_flag][i].level < state->age[ts19_flag])
            {
                upgradeMax(state->building[ts19_flag][i].unit_id);
                if(state->building[ts19_flag][i].building_type == Von_Neumann) { res_avl -= 100; bld_avl -= 10; }
                if(state->building[ts19_flag][i].building_type == Turing) { res_avl -= 300; bld_avl -= 30; }
            }
        }
        if(res_avl <= 0 || bld_avl <= 0) break;
    }
}

void Tactic::upgradeTower(int &res_avl, int &bld_avl) // 升级塔
{
    for(int i=1;i<state->building[ts19_flag].size();i++)
    {
        if(state->building[ts19_flag][i].building_type == Bool
                || state->building[ts19_flag][i].building_type == Ohm
                || state->building[ts19_flag][i].building_type == Mole
                || state->building[ts19_flag][i].building_type == Monte_Carlo
                || state->building[ts19_flag][i].building_type == Larry_Roberts
                || state->building[ts19_flag][i].building_type == Robert_Kahn
                || state->building[ts19_flag][i].building_type == Musk
                || state->building[ts19_flag][i].building_type == Hawkin)
        {
            if(state->building[ts19_flag][i].level < state->age[ts19_flag])
            {
                upgradeMax(state->building[ts19_flag][i].unit_id);
                if(state->building[ts19_flag][i].building_type == Bool) { res_avl -= 75; bld_avl -= 7.5; }
                if(state->building[ts19_flag][i].building_type == Ohm) { res_avl -= 100; bld_avl -= 10; }
                if(state->building[ts19_flag][i].building_type == Mole) { res_avl -= 112.5; bld_avl -= 11; }
                if(state->building[ts19_flag][i].building_type == Monte_Carlo) { res_avl -= 100; bld_avl -= 10; }
                if(state->building[ts19_flag][i].building_type == Larry_Roberts) { res_avl -= 125; bld_avl -= 12.5; }
                if(state->building[ts19_flag][i].building_type == Robert_Kahn) { res_avl -= 225; bld_avl -= 22.5; }
                if(state->building[ts19_flag][i].building_type == Musk) { res_avl -= 250; bld_avl -= 25; }
                if(state->building[ts19_flag][i].building_type == Hawkin) { res_avl -= 250; bld_avl -= 25; }
                
            }
        }
        if(res_avl <= 0 || bld_avl <= 0) break;
    }
}

void Tactic::upgradeCharge(int &res_avl, int &bld_avl) // 升级冲锋
{
    for(int i=1;i<state->building[ts19_flag].size();i++)
    {
        if(state->building[ts19_flag][i].building_type == Norton
                || state->building[ts19_flag][i].building_type == Berners_Lee)
        {
            if(state->building[ts19_flag][i].level < state->age[ts19_flag])
            {
                upgradeMax(state->building[ts19_flag][i].unit_id);
                if(state->building[ts19_flag][i].building_type == Norton) { res_avl -= 80; bld_avl -= 8; }
                if(state->building[ts19_flag][i].building_type == Berners_Lee) { res_avl -= 125; bld_avl -= 12.5; }
                
            }
        }
        if(res_avl <= 0 || bld_avl <= 0) break;
    }
}

void Tactic::upgradeRes(int &res_avl, int &bld_avl) // 升级码农
{
    for(int i=1;i<state->building[ts19_flag].size();i++)
    {
        if(state->building[ts19_flag][i].building_type == Programmer)
        {
            if(state->building[ts19_flag][i].level < state->age[ts19_flag])
            {
                upgrade(state->building[ts19_flag][i].unit_id);
                sg_income_now += 5;
                res_avl -= 50;
                bld_avl -= 5;
            }
        }
        if(res_avl <= 0 || bld_avl <= 0) break;
    }
}

void Tactic::checkRepair() //修理检测
{
    for (int i = 1;i < state->building[ts19_flag].size(); i++)
    {
        if(state->building[ts19_flag][i].building_type == Programmer)
        {
            if(state->building[ts19_flag][i].heal < 100 WITH_LEVELS)
                toggleMaintain(state->building[ts19_flag][i].unit_id);
        }
        if(state->building[ts19_flag][i].building_type == Shannon)
        {
            if(state->building[ts19_flag][i].heal < 120 WITH_LEVELS)
                toggleMaintain(state->building[ts19_flag][i].unit_id);
        }
        if(state->building[ts19_flag][i].building_type == Thevenin)
        {
            if(state->building[ts19_flag][i].heal < 200 WITH_LEVELS)
                toggleMaintain(state->building[ts19_flag][i].unit_id);
        }
        if(state->building[ts19_flag][i].building_type == Norton)
        {
            if(state->building[ts19_flag][i].heal < 180 WITH_LEVELS)
                toggleMaintain(state->building[ts19_flag][i].unit_id);
        }
        if(state->building[ts19_flag][i].building_type == Von_Neumann)
        {
            if(state->building[ts19_flag][i].heal < 200 WITH_LEVELS)
                toggleMaintain(state->building[ts19_flag][i].unit_id);
        }
        if(state->building[ts19_flag][i].building_type == Berners_Lee)
        {
            if(state->building[ts19_flag][i].heal < 150 WITH_LEVELS)
                toggleMaintain(state->building[ts19_flag][i].unit_id);
        }
        if(state->building[ts19_flag][i].building_type == Kuen_Kao)
        {
            if(state->building[ts19_flag][i].heal < 160 WITH_LEVELS)
                toggleMaintain(state->building[ts19_flag][i].unit_id);
        }
        if(state->building[ts19_flag][i].building_type == Turing)
        {
            if(state->building[ts19_flag][i].heal < 250 WITH_LEVELS)
                toggleMaintain(state->building[ts19_flag][i].unit_id);
        }
        if(state->building[ts19_flag][i].building_type == Tony_Stark)
        {
            if(state->building[ts19_flag][i].heal < 220 WITH_LEVELS)
                toggleMaintain(state->building[ts19_flag][i].unit_id);
        }
        if(state->building[ts19_flag][i].building_type == Bool)
        {
            if(state->building[ts19_flag][i].heal < 200 WITH_LEVELS)
                toggleMaintain(state->building[ts19_flag][i].unit_id);
        }
        if(state->building[ts19_flag][i].building_type == Ohm)
        {
            if(state->building[ts19_flag][i].heal < 320 WITH_LEVELS)
                toggleMaintain(state->building[ts19_flag][i].unit_id);
        }
        if(state->building[ts19_flag][i].building_type == Mole)
        {
            if(state->building[ts19_flag][i].heal < 250 WITH_LEVELS)
                toggleMaintain(state->building[ts19_flag][i].unit_id);
        }
        if(state->building[ts19_flag][i].building_type == Monte_Carlo)
        {
            if(state->building[ts19_flag][i].heal < 350 WITH_LEVELS)
                toggleMaintain(state->building[ts19_flag][i].unit_id);
        }
        if(state->building[ts19_flag][i].building_type == Larry_Roberts)
        {
            if(state->building[ts19_flag][i].heal < 220 WITH_LEVELS)
                toggleMaintain(state->building[ts19_flag][i].unit_id);
        }
        if(state->building[ts19_flag][i].building_type == Robert_Kahn)
        {
            if(state->building[ts19_flag][i].heal < 520 WITH_LEVELS)
                toggleMaintain(state->building[ts19_flag][i].unit_id);
        }
        if(state->building[ts19_flag][i].building_type == Musk)
        {
            if(state->building[ts19_flag][i].heal < 750 WITH_LEVELS)
                toggleMaintain(state->building[ts19_flag][i].unit_id);
        }
        if(state->building[ts19_flag][i].building_type == Hawkin)
        {
            if(state->building[ts19_flag][i].heal < 360 WITH_LEVELS)
                toggleMaintain(state->building[ts19_flag][i].unit_id);
        }
        //if (state->building[ts19_flag][i].heal < 100 && state->building[ts19_flag][i].heal >= 50) toggleMaintain(state->building[ts19_flag][i].unit_id); //有问题
    }
}

void Tactic::checkSell() //售卖检测
{
    //for (int i = 0;i < MAX_BLDNOW; i++)
    //    if (state->building[ts19_flag][i].heal < 50) sell(state->building[ts19_flag][i].unit_id);
}


/***
 * 如何使用allstate优化算法？
 * 是否需要兵种的等级信息？
 * 待测试...
 * ***/
void Tactic::spyMap()
{
    for(int w=3-(sg_waynum-1)/2;w<=3+(sg_waynum-1)/2;w++)
    {
        //初始化
        unitInfo_[w][Shannon] = 0;
        unitInfo_[w][Thevenin] = 0;
        unitInfo_[w][Norton] = 0;
        unitInfo_[w][Von_Neumann] = 0;
        unitInfo_[w][Berners_Lee] = 0;
        unitInfo_[w][Kuen_Kao] = 0;
        unitInfo_[w][Turing] = 0;
    }
    
    
    
    if(state->soldier[!ts19_flag].size() > 0)
    {
        for(int i=0;i<state->soldier[!ts19_flag].size();i++) //对士兵进行检索，i为士兵标签
        {
            for(int w=3-(sg_waynum-1)/2;w<=3+(sg_waynum-1)/2;w++) //对道路条数检索
            {
                Point *pos = sg_tower_front[w]; //pos为当前搜索道路位置，从塔前线开始算
                
                
                bool ifFound = false; //找到即为真，控制此循环退出

                for(int j=0;j<144;j++) //除奥创外，相同建筑两个兵最大间距为光纤（72），对前线往后144格进行检索
                {
                    if(state->soldier[!ts19_flag][i].pos.x == pos->x && state->soldier[!ts19_flag][i].pos.y == pos->y)
                    {
                        if(state->soldier[!ts19_flag][i].soldier_name == BIT_STREAM)
                        { unitInfo_[w][Shannon]++; ifFound = true; break; }
                        if(state->soldier[!ts19_flag][i].soldier_name == VOLTAGE_SOURCE)
                        { unitInfo_[w][Thevenin]++; ifFound = true; break; }
                        if(state->soldier[!ts19_flag][i].soldier_name == CURRENT_SOURCE)
                        { unitInfo_[w][Norton]++; ifFound = true; break; }
                        if(state->soldier[!ts19_flag][i].soldier_name == ENIAC)
                        { unitInfo_[w][Von_Neumann]++; ifFound = true; break; }
                        if(state->soldier[!ts19_flag][i].soldier_name == PACKET)
                        { unitInfo_[w][Berners_Lee]++; ifFound = true; break; }
                        if(state->soldier[!ts19_flag][i].soldier_name == OPTICAL_FIBER)
                        { unitInfo_[w][Kuen_Kao]++; ifFound = true; break; }
                        if(state->soldier[!ts19_flag][i].soldier_name == TURING_MACHINE)
                        { unitInfo_[w][Turing]++; ifFound = true; break; }
                        if(state->soldier[!ts19_flag][i].soldier_name == ULTRON)
                        { unitInfo_[w][Tony_Stark]++; ifFound = true; break; }
                    }
                    
                    if(pos->next != nullptr) pos = pos->next;
                }
                if(ifFound) break;
            }
        }
    }
    for(int w=3-(sg_waynum-1)/2;w<=3+(sg_waynum-1)/2;w++)
    {
        //对72距离进行修正，不同士兵移动速度和建造CD不同
        unitInfo_[w][Shannon] = unitInfo_[w][Shannon]/4.0; //36
        unitInfo_[w][Shannon] -= sg_tower_def[w][Shannon];
        unitInfo_[w][Thevenin] = unitInfo_[w][Thevenin]/2.57; //56
        unitInfo_[w][Thevenin] -= sg_tower_def[w][Thevenin];
        unitInfo_[w][Norton] = unitInfo_[w][Norton]/1.92; //75
        unitInfo_[w][Norton] -= sg_tower_def[w][Norton];
        unitInfo_[w][Von_Neumann] = unitInfo_[w][Von_Neumann]/2.25; //64
        unitInfo_[w][Von_Neumann] -= sg_tower_def[w][Von_Neumann];
        unitInfo_[w][Berners_Lee] = unitInfo_[w][Berners_Lee]/2.25; //64
        unitInfo_[w][Berners_Lee] -= sg_tower_def[w][Berners_Lee];
        unitInfo_[w][Kuen_Kao] = unitInfo_[w][Kuen_Kao]/1.71; //84
        unitInfo_[w][Kuen_Kao] -= sg_tower_def[w][Kuen_Kao];
        unitInfo_[w][Turing] = unitInfo_[w][Turing]/2.67; //54
        unitInfo_[w][Turing] -= sg_tower_def[w][Turing];
        
        unitInfo_[w][Tony_Stark] -= sg_tower_def[w][Tony_Stark];
    }
}


//-------------------------------------------------------------------------//




//---------------------------Method中的函数---------------------------------//

bool Method::checkLegal(Position pos) //判断合法性
{
    if(sg_con_map[pos.x][pos.y] <= 0) return true;
    return false;
}

void Method::refreshMapLegal(int x, int y, int value) //建造/拆除建筑后刷新可建造范围，在回合结束时更新
{
    if(value==100) //100为拆除/摧毁
    {
        for(int i=-5;i<6;i++)
        {
            for(int j=-5;j<6;j++)
            {
                if(x+i<200 && y+j<200 && x+i>=0 && y+j>=0)
                {
                    if(dist(Point(x+i,y+j),Point(x,y)) < MAX_BD_RANGE) sg_con_map[x+i][y+j]--;
                }
            }
        }
    }
    for(int i=x-MAX_BD_RANGE;i<=x+MAX_BD_RANGE;i++)
        for(int j=y-MAX_BD_RANGE;j<=y+MAX_BD_RANGE;j++)
        {
            if(i<0 || j<0 || i>=MAP_SIZE || j>=MAP_SIZE) continue;
            if(dist(Position(x,y),Position(i,j)) <= MAX_BD_RANGE)
            {
                if(sg_con_map[i][j] >= 100) sg_con_map[i][j] -= (100-value); //value为100时保持>=100的那个值
                if(sg_con_map[i][j] == 0 && value == 100) sg_con_map[i][j] = value;
            }
        }
}

void Method::refreshMapilLegal(int x, int y, int value)
{
    if(value==0) //0为建造，允许sg_con_map中出现101、102，代表当前不能建筑区域被标记1、2的地方
    {
        //sg_con_map[x][y]++;
        for(int i=-5;i<6;i++)
        {
            for(int j=-5;j<6;j++)
            {
                if(x+i<200 && y+j<200 && x+i>=0 && y+j>=0)
                {
                    if(dist(Point(x+i,y+j),Point(x,y)) < 6) sg_con_map[x+i][y+j]++;
                }
            }
        }
    }
}

Position Method::findSpace(int w) //可建筑点寻找(单道路变量)
{
    Position pos = findSpace(*sg_camp_front[w]);
    
    return pos;
}

Position Method::findSpaceT(int w)
{
    Position pos = findSpaceT(*sg_tower_front[w]);
    return pos;
}

Position Method::findSpace(Point pos) //可建筑点寻找(点变量)
{

    if(ts19_flag == 0)
    {
        if(ts19_map[ pos.x ][ pos.y ] == 0 && checkLegal(Position(pos.x, pos.y)))
        {
            return Position(pos.x, pos.y);
        }
        if(ts19_map[ pos.x +1][ pos.y ] == 0 && checkLegal(Position(pos.x+1, pos.y)))
            return Position(pos.x+1, pos.y);

        if(ts19_map[ pos.x ][ pos.y +1] == 0 && checkLegal(Position(pos.x, pos.y+1)))
            return Position(pos.x, pos.y+1);

        for(int i=1;i<=12;i++)
        {
            for(int j=1;j<=12;j++)
            {
                if(pos.x+i >= 0 && pos.x+i < 200 && pos.y+j >=0 && pos.y+j < 200)
                {
                    if(ts19_map[ pos.x+i ][ pos.y+j ] == 0 && checkLegal(Position(pos.x+i, pos.y+j)))
                        return Position(pos.x+i, pos.y+j);
                }
            }
        }
    }
    if(ts19_flag == 1)
    {
        if(ts19_map[ pos.x ][ pos.y ] == 0 && checkLegal(Position(pos.x, pos.y)))
            return Position(pos.x, pos.y);
        if(ts19_map[ pos.x -1][ pos.y ] == 0 && checkLegal(Position(pos.x-1, pos.y)))
            return Position(pos.x-1, pos.y);
        if(ts19_map[ pos.x ][ pos.y -1] == 0 && checkLegal(Position(pos.x, pos.y-1)))
            return Position(pos.x, pos.y-1);
        for(int i=1;i<=12;i++)
        {
            for(int j=1;j<=12;j++)
            {
                if(pos.x-i >= 0 && pos.x-i < 200 && pos.y-j >=0 && pos.y-j < 200)
                {
                    if(ts19_map[ pos.x-i ][ pos.y-j ] == 0 && checkLegal(Position(pos.x-i, pos.y-j)))
                        return Position(pos.x-i, pos.y-j);
                }
            }
        }
    }
    
    return Position(0,0);
    
}

Position Method::findSpaceT(Point pos)
{
    if(ts19_flag == 0)
    {
        for(int j=0;j<4;j++)
        {
            for(int i=1;i<9;i++)
            {
                if(pos.x+i-j >= 0 && pos.x+i-j < 200 && pos.y-i-j >= 0 && pos.y-i-j < 200)
                {
                    if(ts19_map[ pos.x +i-j][ pos.y-i-j ] == 0 && checkLegal(Position(pos.x+i-j, pos.y-i-j)))
                        return Position(pos.x+i-j, pos.y-i-j);
                }
                if(pos.x-i-j >= 0 && pos.x-i-j < 200 && pos.y+i-j >= 0 && pos.y+i-j < 200)
                {
                    if(ts19_map[ pos.x -i-j][ pos.y+i-j ] == 0 && checkLegal(Position(pos.x-i-j, pos.y+i-j)))
                        return Position(pos.x-i-j, pos.y+i-j);
                }
            }
        }
    }
    if(ts19_flag == 1)
    {
        for(int j=0;j<4;j++)
        {
            for(int i=1;i<9;i++)
            {

                if(pos.x+i+j >= 0 && pos.x+i+j < 200 && pos.y-i+j >= 0 && pos.y-i+j < 200)
                {
                    if(ts19_map[ pos.x +i+j][ pos.y-i+j ] == 0 && checkLegal(Position(pos.x+i+j, pos.y-i+j)))
                        return Position(pos.x+i+j, pos.y-i+j);
                }
                if(pos.x-i+j >= 0 && pos.x-i+j < 200 && pos.y+i+j >= 0 && pos.y+i+j < 200)
                {
                    if(ts19_map[ pos.x -i+j][ pos.y+i+j ] == 0 && checkLegal(Position(pos.x-i+j, pos.y+i+j)))
                        return Position(pos.x-i+j, pos.y+i+j);
                }

            }
        }
    }
    if(ts19_flag == 0)
    {
        for(int i=0;i<=8;i++)
        {
            for(int j=0;j<=8;j++)
                if(ts19_map[ pos.x+i ][ pos.y+j ] == 0 && checkLegal(Position(pos.x+i, pos.y+j)))
                    return Position(pos.x+i,pos.y+j);
        }
    }
    if(ts19_flag == 1)
    {
        for(int i=0;i<=8;i++)
        {
            for(int j=0;j<=8;j++)
                if(ts19_map[ pos.x-i ][ pos.y-j ] == 0 && checkLegal(Position(pos.x-i, pos.y-j)))
                    return Position(pos.x-i,pos.y-j);
        }
    }
    return Position(0,0);
}


/////////以下为对建造具体建筑的实现




bool Method::conProg(int &res_avl, int &bld_avl, Point pos) //建造码农
{
    Position bldpos = findSpace(pos);
    
    if(sg_bldnum_now < MAX_BLDNOW && res_avl >= 100  && bld_avl >= 10 && (bldpos.x != 0 || bldpos.y != 0))
    {
        construct(Programmer, bldpos, Position(0,0));

        //填入建造序列中
        for(int i=0;;i++)
        {
            if(sg_bldqueue[i].x == -1 && sg_bldqueue[i].y== -1) { sg_bldqueue[i].x = bldpos.x; sg_bldqueue[i].y = bldpos.y; break; }
        }
        refreshMapilLegal(bldpos.x, bldpos.y, 0); //更新不能建造区域
        refreshMapLegal(bldpos.x, bldpos.y, 0); //更新能建造区域
        sg_bldnum_now++; //当前建筑数增加
        sg_bldlabel_now++;
        bld_avl -= 10;
        res_avl -= 100;
        
        sg_income_now += 10;
        return 1;
    }
    return 0;
}

bool Method::conShannon(int w, int &res_avl, int &bld_avl) //建造香农
{
    Position bldpos = findSpace(w);
    if(sg_bldnum_now < MAX_BLDNOW && res_avl >= 150   && bld_avl >= 15 && (bldpos.x != 0 || bldpos.y != 0))
    {
        construct(Shannon, bldpos, Position( (*sg_camp_front[w]).x,(*sg_camp_front[w]).y ));
        
        for(int i=0;;i++)
        {
            if(sg_bldqueue[i].x == -1 && sg_bldqueue[i].y== -1) { sg_bldqueue[i].x = bldpos.x; sg_bldqueue[i].y = bldpos.y; break; }
        }
        for(int i=0;i<5;i++) sg_camp_front[w] = sg_camp_front[w]->next;
        refreshMapilLegal(bldpos.x,bldpos.y,0);
        refreshMapLegal(bldpos.x, bldpos.y, 0); //更新能建造区域
        sg_bldnum_now++; //当前建筑数增加
        sg_bldlabel_now++;
        res_avl -= 150;
        bld_avl -= 15;
        return 1;
    }
    return 0;
}

bool Method::conThevenin(int w, int &res_avl, int &bld_avl) //建造戴维南
{
    Position bldpos = findSpace(w);
    if(sg_bldnum_now < MAX_BLDNOW && res_avl >= 160   && bld_avl >= 16 && (bldpos.x != 0 || bldpos.y != 0))
    {
        construct(Thevenin, bldpos, Position( (*sg_camp_front[w]).x,(*sg_camp_front[w]).y ));
        
        for(int i=0;;i++)
        {
            if(sg_bldqueue[i].x == -1 && sg_bldqueue[i].y== -1) { sg_bldqueue[i].x = bldpos.x; sg_bldqueue[i].y = bldpos.y; break; }
        }
        for(int i=0;i<5;i++) sg_camp_front[w] = sg_camp_front[w]->next; //前线移动
        refreshMapilLegal(bldpos.x,bldpos.y,0);
        refreshMapLegal(bldpos.x, bldpos.y, 0); //更新能建造区域
        sg_bldnum_now++; //当前建筑数增加
        sg_bldlabel_now++;
        res_avl -= 160;
        bld_avl -= 16;
        return 1;
    }
    return 0;
}

bool Method::conNorton(int w, int &res_avl, int &bld_avl) //建造诺顿
{
    Position bldpos = findSpace(w);
    if(sg_bldnum_now < MAX_BLDNOW && res_avl >= 160   && bld_avl >= 16 && (bldpos.x != 0 || bldpos.y != 0))
    {
        construct(Norton, bldpos, Position( (*sg_camp_front[w]).x,(*sg_camp_front[w]).y ));
        
        for(int i=0;;i++)
        {
            if(sg_bldqueue[i].x == -1 && sg_bldqueue[i].y== -1) { sg_bldqueue[i].x = bldpos.x; sg_bldqueue[i].y = bldpos.y; break; }
        }
        for(int i=0;i<5;i++) sg_camp_front[w] = sg_camp_front[w]->next; //前线移动
        refreshMapilLegal(bldpos.x,bldpos.y,0);
        refreshMapLegal(bldpos.x, bldpos.y, 0); //更新能建造区域
        sg_bldnum_now++; //当前建筑数增加
        sg_bldlabel_now++;
        res_avl -= 160;
        bld_avl -= 16;
        return 1;
    }
    return 0;
}

bool Method::conVon(int w, int &res_avl, int &bld_avl) //建造冯诺依曼
{
    Position bldpos = findSpace(w);
    if(sg_bldnum_now < MAX_BLDNOW && res_avl >= 200   && bld_avl >= 20 && (bldpos.x != 0 || bldpos.y != 0))
    {
        construct(Von_Neumann, bldpos, Position( (*sg_camp_front[w]).x,(*sg_camp_front[w]).y ));
        
        for(int i=0;;i++)
        {
            if(sg_bldqueue[i].x == -1 && sg_bldqueue[i].y== -1) { sg_bldqueue[i].x = bldpos.x; sg_bldqueue[i].y = bldpos.y; break; }
        }
        for(int i=0;i<5;i++) sg_camp_front[w] = sg_camp_front[w]->next; //前线移动
        refreshMapilLegal(bldpos.x,bldpos.y,0);
        refreshMapLegal(bldpos.x, bldpos.y, 0); //更新能建造区域
        sg_bldnum_now++; //当前建筑数增加
        sg_bldlabel_now++;
        res_avl -= 200;
        bld_avl -= 20;
        return 1;
    }
    return 0;
}

bool Method::conLee(int w, int &res_avl, int &bld_avl) //建造伯纳斯李
{
    Position bldpos = findSpace(w);
    if(sg_bldnum_now < MAX_BLDNOW && res_avl >= 250   && bld_avl >= 25 && (bldpos.x != 0 || bldpos.y != 0))
    {
        construct(Berners_Lee, bldpos, Position( (*sg_camp_front[w]).x,(*sg_camp_front[w]).y ));
        
        for(int i=0;;i++)
        {
            if(sg_bldqueue[i].x == -1 && sg_bldqueue[i].y== -1) { sg_bldqueue[i].x = bldpos.x; sg_bldqueue[i].y = bldpos.y; break; }
        }
        for(int i=0;i<5;i++) sg_camp_front[w] = sg_camp_front[w]->next; //前线移动
        refreshMapilLegal(bldpos.x,bldpos.y,0);
        refreshMapLegal(bldpos.x, bldpos.y, 0); //更新能建造区域
        sg_bldnum_now++; //当前建筑数增加
        sg_bldlabel_now++;
        res_avl -= 250;
        bld_avl -= 25;
        return 1;
    }
    return 0;
}

bool Method::conGao(int w, int &res_avl, int &bld_avl) //建造高锟
{
    Position bldpos = findSpace(w);if(sg_bldnum_now < MAX_BLDNOW && res_avl >= 300   && bld_avl >= 30 && (bldpos.x != 0 || bldpos.y != 0))
    {
        construct(Kuen_Kao, bldpos, Position( (*sg_camp_front[w]).x,(*sg_camp_front[w]).y ));
        
        for(int i=0;;i++)
        {
            if(sg_bldqueue[i].x == -1 && sg_bldqueue[i].y== -1) { sg_bldqueue[i].x = bldpos.x; sg_bldqueue[i].y = bldpos.y; break; }
        }
        for(int i=0;i<5;i++) sg_camp_front[w] = sg_camp_front[w]->next; //前线移动
        refreshMapilLegal(bldpos.x,bldpos.y,0);
        refreshMapLegal(bldpos.x, bldpos.y, 0); //更新能建造区域
        sg_bldnum_now++; //当前建筑数增加
        sg_bldlabel_now++;
        res_avl -= 300;
        bld_avl -= 30;
        return 1;
    }
    return 0;
}

bool Method::conTuring(int w, int &res_avl, int &bld_avl) //建造图灵
{
    Position bldpos = findSpace(w);
    if(sg_bldnum_now < MAX_BLDNOW && res_avl >= 600   && bld_avl >= 60 && (bldpos.x != 0 || bldpos.y != 0))
    {
        construct(Turing, bldpos, Position( (*sg_camp_front[w]).x,(*sg_camp_front[w]).y ));
        
        for(int i=0;;i++)
        {
            if(sg_bldqueue[i].x == -1 && sg_bldqueue[i].y== -1) { sg_bldqueue[i].x = bldpos.x; sg_bldqueue[i].y = bldpos.y; break; }
        }
        for(int i=0;i<5;i++) sg_camp_front[w] = sg_camp_front[w]->next; //前线移动
        refreshMapilLegal(bldpos.x,bldpos.y,0);
        refreshMapLegal(bldpos.x, bldpos.y, 0); //更新能建造区域
        sg_bldnum_now++; //当前建筑数增加
        sg_bldlabel_now++;
        res_avl -= 600;
        bld_avl -= 60;
        return 1;
    }
    return 0;
}

bool Method::conStark(int w, int &res_avl, int &bld_avl) //建造斯塔克
{
    Position bldpos = findSpace(w);
    if(sg_bldnum_now < MAX_BLDNOW && res_avl >= 600   && bld_avl >= 60 && (bldpos.x != 0 || bldpos.y != 0))
    {
        construct(Tony_Stark, bldpos, Position( (*sg_camp_front[w]).x,(*sg_camp_front[w]).y ));
        
        for(int i=0;;i++)
        {
            if(sg_bldqueue[i].x == -1 && sg_bldqueue[i].y== -1) { sg_bldqueue[i].x = bldpos.x; sg_bldqueue[i].y = bldpos.y; break; }
        }
        for(int i=0;i<5;i++) sg_camp_front[w] = sg_camp_front[w]->next; //前线移动
        refreshMapilLegal(bldpos.x,bldpos.y,0);
        refreshMapLegal(bldpos.x, bldpos.y, 0); //更新能建造区域
        sg_bldnum_now++; //当前建筑数增加
        sg_bldlabel_now++;
        res_avl -= 600;
        bld_avl -= 60;
        return 1;
    }
    return 0;
}

bool Method::conBoolT(int w, int &res_avl, int &bld_avl) //建造布尔塔
{
    Position bldpos = findSpaceT(w);
    
    if(sg_bldnum_now < MAX_BLDNOW && res_avl >= 150  && bld_avl >= 15 && (bldpos.x != 0 || bldpos.y != 0))
    {
        construct(Bool, bldpos, Position(0,0));
        
        for(int i=0;;i++)
        {
            if(sg_bldqueue[i].x == -1 && sg_bldqueue[i].y== -1) { sg_bldqueue[i].x = bldpos.x; sg_bldqueue[i].y = bldpos.y; break; }
        }
        refreshMapilLegal(bldpos.x,bldpos.y,0);
        refreshMapLegal(bldpos.x, bldpos.y, 0); //更新能建造区域
        sg_bldnum_now++; //当前建筑数增加
        sg_bldlabel_now++;
        res_avl -= 150;
        bld_avl -= 15;
        return 1;
    }
    return 0;
}

bool Method::conOhmT(int w, int &res_avl, int &bld_avl) //建造欧姆塔
{
    
    Position bldpos = findSpaceT(w);
    if(sg_bldnum_now < MAX_BLDNOW && res_avl >= 200  && bld_avl >= 20 && (bldpos.x != 0 || bldpos.y != 0))
    {
        construct(Ohm, bldpos, Position(0,0));
        for(int i=0;;i++)
        {
            if(sg_bldqueue[i].x == -1 && sg_bldqueue[i].y== -1) { sg_bldqueue[i].x = bldpos.x; sg_bldqueue[i].y = bldpos.y; break; }
        }
        refreshMapilLegal(bldpos.x,bldpos.y,0);
        refreshMapLegal(bldpos.x, bldpos.y, 0); //更新能建造区域
        sg_bldnum_now++; //当前建筑数增加
        sg_bldlabel_now++;
        bld_avl -= 20;
        res_avl -= 200;
        return 1;
    }
    return 0;
}

bool Method::conMooreT(int w, int &res_avl, int &bld_avl) //建造摩尔塔
{
    Position bldpos = findSpaceT(w);
    if(sg_bldnum_now < MAX_BLDNOW && res_avl >= 225  && bld_avl >= 22 && (bldpos.x != 0 || bldpos.y != 0))
    {
        construct(Mole, bldpos, Position(0,0));
        for(int i=0;;i++)
        {
            if(sg_bldqueue[i].x == -1 && sg_bldqueue[i].y== -1) { sg_bldqueue[i].x = bldpos.x; sg_bldqueue[i].y = bldpos.y; break; }
        }
        refreshMapilLegal(bldpos.x,bldpos.y,0);
        refreshMapLegal(bldpos.x, bldpos.y, 0); //更新能建造区域
        sg_bldnum_now++; //当前建筑数增加
        sg_bldlabel_now++;
        res_avl -= 225;
        bld_avl -= 22;
        return 1;
    }
    return 0;
}

bool Method::conMonteCarloT(int w, int &res_avl, int &bld_avl) //建造蒙特卡罗塔
{
    Position bldpos = findSpaceT(w);
    if(sg_bldnum_now < MAX_BLDNOW && res_avl >= 200  && bld_avl >= 20 && (bldpos.x != 0 || bldpos.y != 0))
    {
        construct(Monte_Carlo, bldpos, Position(0,0));
        
        
        for(int i=0;;i++)
        {
            if(sg_bldqueue[i].x == -1 && sg_bldqueue[i].y== -1) { sg_bldqueue[i].x = bldpos.x; sg_bldqueue[i].y = bldpos.y; break; }
        }
        refreshMapilLegal(bldpos.x,bldpos.y,0);
        refreshMapLegal(bldpos.x, bldpos.y, 0); //更新能建造区域
        sg_bldnum_now++; //当前建筑数增加
        sg_bldlabel_now++;
        res_avl -= 200;
        bld_avl -= 20;
        return 1;
    }
    return 0;
}

bool Method::conLarryT(int w, int &res_avl, int &bld_avl) //建造拉里塔
{
    Position bldpos = findSpaceT(w);
    if(sg_bldnum_now < MAX_BLDNOW && res_avl >= 250  && bld_avl >= 25 && (bldpos.x != 0 || bldpos.y != 0))
    {
        construct(Larry_Roberts, bldpos, Position(0,0));
        for(int i=0;;i++)
        {
            if(sg_bldqueue[i].x == -1 && sg_bldqueue[i].y== -1) { sg_bldqueue[i].x = bldpos.x; sg_bldqueue[i].y = bldpos.y; break; }
        }
        refreshMapilLegal(bldpos.x,bldpos.y,0);
        refreshMapLegal(bldpos.x, bldpos.y, 0); //更新能建造区域
        sg_bldnum_now++; //当前建筑数增加
        sg_bldlabel_now++;
        res_avl -= 250;
        bld_avl -= 25;
        return 1;
    }
    return 0;
}

bool Method::conRobertT(int w, int &res_avl, int &bld_avl) //建造罗伯特塔
{
    Position bldpos = findSpaceT(w);
    if(sg_bldnum_now < MAX_BLDNOW && res_avl >= 450  && bld_avl >= 45 && (bldpos.x != 0 || bldpos.y != 0))
    {
        construct(Robert_Kahn, bldpos, Position(0,0));
        
        for(int i=0;;i++)
        {
            if(sg_bldqueue[i].x == -1 && sg_bldqueue[i].y== -1) { sg_bldqueue[i].x = bldpos.x; sg_bldqueue[i].y = bldpos.y; break; }
        }
        refreshMapilLegal(bldpos.x,bldpos.y,0);
        refreshMapLegal(bldpos.x, bldpos.y, 0); //更新能建造区域
        sg_bldnum_now++; //当前建筑数增加
        sg_bldlabel_now++;
        res_avl -= 450;
        bld_avl -= 45;
        return 1;
    }
    return 0;
}

bool Method::conMuskT(int w, int &res_avl, int &bld_avl) //建造马斯克塔
{
    Point *finding = sg_way[w][7];
    for(int i=0;i<30;i++)
    {
        Position bldpos = findSpace(*finding);
        if(sg_bldnum_now < MAX_BLDNOW && res_avl >= 500  && bld_avl >= 50 && (bldpos.x != 0 || bldpos.y != 0))
        {
            construct(Musk, bldpos, Position(0,0));

            for(int i=0;;i++)
            {
                if(sg_bldqueue[i].x == -1 && sg_bldqueue[i].y== -1) { sg_bldqueue[i].x = bldpos.x; sg_bldqueue[i].y = bldpos.y; break; }
            }
            refreshMapilLegal(bldpos.x,bldpos.y,0);
            refreshMapLegal(bldpos.x, bldpos.y, 0); //更新能建造区域
            sg_bldnum_now++; //当前建筑数增加
            sg_bldlabel_now++;
            res_avl -= 500;
            bld_avl -= 50;
            return 1;
        }
        finding = finding->next;
        finding = finding->next;
        finding = finding->next;
        finding = finding->next;
    }
    return 0;
}

bool Method::conMuskT(Point a, int &res_avl, int &bld_avl)
{
    for(int i=0;i<30;i++)
    {
        Position bldpos = Position(a.x,a.y);
        if(sg_bldnum_now < MAX_BLDNOW && res_avl >= 500  && bld_avl >= 50 && (bldpos.x != 0 || bldpos.y != 0))
        {
            construct(Musk, bldpos, Position(0,0));

            for(int i=0;;i++)
            {
                if(sg_bldqueue[i].x == -1 && sg_bldqueue[i].y== -1) { sg_bldqueue[i].x = bldpos.x; sg_bldqueue[i].y = bldpos.y; break; }
            }
            refreshMapilLegal(bldpos.x,bldpos.y,0);
            refreshMapLegal(bldpos.x, bldpos.y, 0); //更新能建造区域
            sg_bldnum_now++; //当前建筑数增加
            sg_bldlabel_now++;
            res_avl -= 500;
            bld_avl -= 50;
            return 1;
        }
    }
    return 0;
}

bool Method::conHawkingT(int w, int &res_avl, int &bld_avl) //建造霍金
{
    
    Position bldpos = findSpaceT(w);
    if(sg_bldnum_now < MAX_BLDNOW && res_avl >= 500  && sg_bldpoint_now >= 50 && (bldpos.x != 0 || bldpos.y != 0))
    {
        construct(Hawkin, bldpos, Position(0,0));
        
        for(int i=0;;i++)
        {
            if(sg_bldqueue[i].x == -1 && sg_bldqueue[i].y== -1) { sg_bldqueue[i].x = bldpos.x; sg_bldqueue[i].y = bldpos.y; break; }
        }
        refreshMapilLegal(bldpos.x,bldpos.y,0);
        refreshMapLegal(bldpos.x, bldpos.y, 0); //更新能建造区域
        sg_bldnum_now++; //当前建筑数增加
        sg_bldlabel_now++;
        res_avl -= 500;
        sg_bldpoint_now -= 50;
        return 1;
    }
    return 0;
}

//===================================================================================================================//

void f_player()
{
    if(ts19_flag==0)
    {
        player0();
        
    }
    else
        player1();
    
    
};

static Strategy strategy;
static Tactic tactic;
static Method method;

void player0() {
    
    //首回合初始化
    if(state->turn < 1)
    {
        sg_waynum = strategy.findwayNum();
        sg_bldnum_now = 0;
        sg_bldlabel_now = 0;
        sg_income_now = 0;
        sg_sold = false;
        strategy.inputWay();
        for(int i=3-(sg_waynum-1)/2;i<3+(sg_waynum-1)/2;i++)
            sg_wayAttacked[i] = false;
        strategy.doMoreRes(); //初始化策略
    }
    
    //每回合初始化
    sg_bldpoint_now = state->resource[0].building_point;
    sg_bldpoint_avl = sg_bldpoint_now;
    sg_res_now = state->resource[0].resource;
    sg_res_avl = sg_res_now;
    
    sg_bldnum_now = state->building[0].size();
    
    for(int i=0;i<200;i++) //检索地图（运算量有点大，考虑优化）重置 sg_con_map
        for(int j=0;j<200;j++)
        {
            if(i<11 && j<11) sg_con_map[i][j] = 0;
            else sg_con_map[i][j] = 100;
        }
    
    
    for(int i=1;i<state->building[0].size();i++) //对建筑检索
    {
        //重置 sg_con_map
        method.refreshMapilLegal(state->building[0][i].pos.x, state->building[0][i].pos.y, 0);
        method.refreshMapLegal(state->building[0][i].pos.x, state->building[0][i].pos.y, 0);
    }
    for(int i=1;i<state->building[!ts19_flag].size();i++)
    {
        method.refreshMapilLegal(state->building[!ts19_flag][i].pos.x, state->building[!ts19_flag][i].pos.y, 0);
    }


    ///
    
    tactic.spyMap();
    
    ///
    strategy.spyMap();
    
    ///
    
    if(state->turn <= 15)
    {
        if(!(strategy.stg_now_ == DEF_FOR_RUSH_DOWN || strategy.stg_now_ == DEF_FOR_RUSH_UP))
        {
            strategy.doMoreRes();
            strategy.normalActive();
        }
        else
        {
            strategy.defRush();
        }

    }
    
    if(state->turn > 15)
    {
        if(state->turn <= 60)
        {
            if(!(strategy.stg_now_ == DEF_FOR_RUSH_DOWN || strategy.stg_now_ == DEF_FOR_RUSH_UP))
            {
                if(state->age[ts19_flag] >= 1 && strategy.stg_now_ != ATTACK) strategy.doAttack();
                strategy.normalActive();
            }
            else strategy.defRush();
        }
        else
        {
            strategy.normalActive();
        }
    }


    if(state->age[0] < 4)
    {
        if(state->resource[0].resource >= (UPDATE_COST + UPDATE_COST_SQUARE*state->age[0]*state->age[0]))
        {
            updateAge();
            strategy.doAttack();
        }

    }
    if(state->age[0] == 4)
    {
        if(state->resource[0].resource >= (UPDATE_COST + UPDATE_COST_SQUARE*state->age[0]*state->age[0]))
        {
            updateAge();
            strategy.doAttack();
        }
    }

    //回合结束时更新数据
    for(int i=0;;i++) //建造地图更新
    {
        if(sg_bldqueue[i].x == -1 && sg_bldqueue[i].y == -1) break;
        sg_bldqueue[i].x = -1, sg_bldqueue[i].y = -1;
    }
    for(int i=0;;i++) //出售地图更新
    {
        if(sg_sellqueue[i].x == -1 && sg_sellqueue[i].y == -1) break;
        method.refreshMapLegal(sg_sellqueue[i].x,sg_sellqueue[i].y,100);
        sg_sellqueue[i].x = -1, sg_sellqueue[i].y = -1;
    }

}

void player1() {
    
    //首回合初始化
    if(state->turn < 1)
    {
        sg_waynum = strategy.findwayNum();
        sg_bldnum_now = 0;
        sg_bldlabel_now = 0;
        sg_income_now = 0;
        sg_sold = false;
        for(int i=3-(sg_waynum-1)/2;i<3+(sg_waynum-1)/2;i++)
            sg_wayAttacked[i] = false;
        strategy.inputWay();
        strategy.doMoreRes(); //初始化策略

    }
    
    //每回合初始化
    sg_bldpoint_now = state->resource[1].building_point;
    sg_bldpoint_avl = sg_bldpoint_now;
    sg_res_now = state->resource[1].resource;
    sg_res_avl = sg_res_now;
    sg_bldnum_now = state->building[1].size();
    
    for(int i=0;i<200;i++) //检索地图（运算量有点大，考虑优化）重置 sg_con_map
        for(int j=0;j<200;j++)
        {
            if(i>188 && j>188) sg_con_map[i][j] = 0;
            else sg_con_map[i][j] = 100;
        }
    
    
    for(int i=1;i<state->building[1].size();i++) //对建筑检索
    {
        //重置 sg_con_map
        method.refreshMapilLegal(state->building[1][i].pos.x, state->building[1][i].pos.y, 0);
        method.refreshMapLegal(state->building[1][i].pos.x, state->building[1][i].pos.y, 0);
    }
    for(int i=1;i<state->building[!ts19_flag].size();i++)
    {
        method.refreshMapilLegal(state->building[!ts19_flag][i].pos.x, state->building[!ts19_flag][i].pos.y, 0);
    }
    
    ///
    
    tactic.spyMap();
    ///
    strategy.spyMap();
    
    if(state->turn <= 15)
    {
        if(!(strategy.stg_now_ == DEF_FOR_RUSH_DOWN || strategy.stg_now_ == DEF_FOR_RUSH_UP))
        {
            strategy.doMoreRes();
            strategy.normalActive();
        }
        else
        {
            strategy.defRush();
        }

    }

    if(state->turn > 15)
    {
        if(state->turn <= 60)
        {
            if(!(strategy.stg_now_ == DEF_FOR_RUSH_DOWN || strategy.stg_now_ == DEF_FOR_RUSH_UP))
            {
                if(state->age[ts19_flag] >= 1 && strategy.stg_now_ != ATTACK) strategy.doAttack();
                strategy.normalActive();
            }
            else strategy.defRush();
        }
        else
        {
            strategy.normalActive();
        }
    }

    if(state->age[1] < 4)
    {
        if(state->resource[1].resource >= (UPDATE_COST + UPDATE_COST_SQUARE*state->age[1]*state->age[1]))
            updateAge();
    }
    if(state->age[1] == 4)
    {
        if(state->resource[1].resource >= (UPDATE_COST + UPDATE_COST_SQUARE*state->age[1]*state->age[1]))
            updateAge();
    }


    //回合结束时更新数据

    for(int i=0;;i++) //建造地图更新
    {
        if(sg_bldqueue[i].x == -1 && sg_bldqueue[i].y == -1) break;
        sg_bldqueue[i].x = -1, sg_bldqueue[i].y = -1;
    }
    for(int i=0;;i++) //出售地图更新
    {
        if(sg_sellqueue[i].x == -1 && sg_sellqueue[i].y == -1) break;
        method.refreshMapLegal(sg_sellqueue[i].x,sg_sellqueue[i].y,100);
        sg_sellqueue[i].x = -1, sg_sellqueue[i].y = -1;
    }

}

