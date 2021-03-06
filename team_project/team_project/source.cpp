#define _CRT_SECURE_NO_WARNINGS

#include <bangtal.h>
//c standard
#include <cstdio>
#include <cstdlib>
#include <ctime>

//cpp standard
#include <vector>
#include <list>
#include <queue>

#define REGEN_FRAME_RATE 0.1f
#define BACK_SCENE_FRAME_PER_PIXEL 5
#define AGENT_UPDATE_TIME 0.01f
#define OBJECT_UPDATE_TIME 0.01f
#define AGENT_SPEED 20
#define AGENT_HEIGHT 50
#define AGENT_WIDTH 50
#define ENEMY_1_GEN_RATE 1.0f
#define ENEMY_1_HEIGHT 10
#define ENEMY_1_WIDHT 10
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 760
#define INITIAL_LIFE 2
#define ULTIMATE_FRAME_RATE 0.1f
#define ULTIMATE_TIME_COUNT 30

using namespace std;

struct agent
{
	ObjectID obj;
	ObjectID ultimateIconObj;
	ObjectID ultimateCountObj;
	ObjectID ultimateObj;
	vector<ObjectID> lifeObjList;
	int x = 0;
	int y = 500;
	int dx = 0, dy = 0;
	int cumulatedExp;
	int level;
	int life;
	int ultimate;
	int invincibleTimeCount;
	int ultimateTimeCount;
	bool ultimateActivate;
	bool invincible;
	bool shown;
};

struct movableObject //단순 이동객체 상호작용 x
{
	ObjectID obj;
	int x;
	int y;
	int dx;
	int dy;
};

struct enemyObject {
	ObjectID obj;
	int exp;
	int type;
	int hp;
	int x;
	int y;
	int dx;
	int dy;
	int moveTimer = 0;
};

struct bulitObject{
	ObjectID obj;
	int damage;
	int x;
	int y;
	int dx;
	int dy;
};

/*
Scenes
*/
SceneID backgroundScene;

/*
TimerID
*/
TimerID objectUpdateTimer;
TimerID agentUpdateTimer;
TimerID backgroundMovingTimer;
TimerID enemy1GenTimer;
TimerID reGenTimer;
TimerID ultimateTimer;

/*
ObjectIDs
*/

/*
UI component
*/
ObjectID startButtonObj, endButtonObj;

/*
Other variables
*/

agent heroAgent;
movableObject backSceneObject;


// index를 활용하지 않고, iterator기반으로만 접근하므로, list가 좋음
// 삽입속도는 상관 없지만 mid기준 제거속도가 list가 더빠름
list<enemyObject> enemyList1; 
list<bulitObject> bulitList;

bool gameStarted = false;

// 보스 등장 조절
bool bossAppeared = false;
bool boss[4] = { 0 };


ObjectID createObject(const char* image, SceneID scene, int x, int y, bool shown);
void keyboardCallback(KeyCode kc, KeyState ks);
void timerCallback(TimerID timer);
void mouseCallback(ObjectID object, int x, int y, MouseAction action);
void endGame(bool success);
void startGame();

pair<bool,int> isBulitHit(int x, int y, int width, int height);
bool isAgentHit(int x, int y, int width, int height);
void initTimers();
void initObjects();
void initBackScene();
void initHeroAgent();
void initBulits();
void initGameUI();
void initEnemys();
void createEnemy(int type);
void createEnemyBuilt(int x, int y);
void createBoss(int type);
void agentMove(int dx, int dy);
void createBulit();
void checkLevel();
void reGenAgent();
void createUltimate();


int main()
{
	srand((unsigned int)time(0)); // init random seed

	setMouseCallback(mouseCallback);
	setTimerCallback(timerCallback);
	setKeyboardCallback(keyboardCallback);

	/*
	scenes
	*/
	backgroundScene = createScene("코딩하기 싫다", "Images/background.png");
	backSceneObject.x = 0;
	backSceneObject.y = -150;
	backSceneObject.dx = BACK_SCENE_FRAME_PER_PIXEL;
	backSceneObject.obj = createObject("Images/nebula03.png", backgroundScene, backSceneObject.x, backSceneObject.y, true);

	/*
	UI
	*/
	startButtonObj = createObject("Images/start.png", backgroundScene, 590, 70, true);
	endButtonObj = createObject("Images/end.png", backgroundScene, 590, 20, true);
	
	/*
	Objects
	*/
	heroAgent.obj = createObject("Images/agent.png", backgroundScene, heroAgent.x, heroAgent.y, false);
	heroAgent.ultimateIconObj = createObject("Images/ultimate.png", backgroundScene, 0, 600, false);
	heroAgent.ultimateCountObj = createObject("Images/0.png", backgroundScene, 80, 600, false);
	heroAgent.ultimateObj = createObject("Images/laser3.png", backgroundScene, heroAgent.x, heroAgent.y, false);

	
	/*
	Timers
	*/
	backgroundMovingTimer = createTimer(0.1f);
	agentUpdateTimer = createTimer(AGENT_UPDATE_TIME);
	objectUpdateTimer = createTimer(OBJECT_UPDATE_TIME);
	enemy1GenTimer = createTimer(ENEMY_1_GEN_RATE);
	reGenTimer = createTimer(REGEN_FRAME_RATE);
	ultimateTimer = createTimer(ULTIMATE_FRAME_RATE);
	startGame(backgroundScene);
}
/*

callbacks

*/

void mouseCallback(ObjectID object, int x, int y, MouseAction action)
{

	if (object == endButtonObj) {
		endGame();
	}
	else if (object == startButtonObj) {
		startGame();
	}

}

void timerCallback(TimerID timer)
{
	if (timer == agentUpdateTimer)
	{
		//모든 객체 update
		if (gameStarted == false)
			return;
		agentMove(heroAgent.dx, heroAgent.dy);

		setTimer(agentUpdateTimer, AGENT_UPDATE_TIME);
		startTimer(agentUpdateTimer);

	}
	else if (timer ==objectUpdateTimer)
	{

		//미사일 위치 갱신
		list<bulitObject>::iterator iterB;
		for (iterB = bulitList.begin(); iterB != bulitList.end(); )
		{
			(*iterB).x += (*iterB).dx;
			(*iterB).y += (*iterB).dy;

			locateObject((*iterB).obj, backgroundScene, (*iterB).x, (*iterB).y);
			if ((*iterB).x > SCREEN_WIDTH + 100)
			{
				iterB = bulitList.erase(iterB); // 범위를 벗어났을때 목록에서 제거
			}
			else
			{
				iterB++;
			}
		}
		//적 위치 갱신
		list<enemyObject>::iterator iterE;
		for (iterE = enemyList1.begin(); iterE != enemyList1.end(); )
		{
			pair<bool, int> hit;
			bool destroy=false;

			(*iterE).x += (*iterE).dx;
			(*iterE).y += (*iterE).dy;

			// type 종류에 따라 move패턴 설정 위함
			if ((*iterE).type == 2)
			{
				if ((*iterE).moveTimer % 10 < 5)
					(*iterE).dy *= (-1);

				(*iterE).moveTimer += 1;
			}
			else if ((*iterE).type == 3)
			{
				if ((*iterE).moveTimer % 40 > 30)
					hideObject((*iterE).obj);
				else
					showObject((*iterE).obj);


				(*iterE).moveTimer += 1;
			}
			else if ((*iterE).type == 4)
			{
				if ((*iterE).moveTimer >= 3)
					(*iterE).dx = 0;

				if ((*iterE).moveTimer % 30 == 10)
					createEnemyBuilt((*iterE).x, (*iterE).y);


				(*iterE).moveTimer += 1;
			}
			// 보스
			else if ((*iterE).type == 11 || (*iterE).type == 12 || (*iterE).type == 13 || (*iterE).type == 14)
			{
				if ((*iterE).moveTimer >= 3)
					(*iterE).dx = 0;

				// 보스1
				if ((*iterE).type == 11)
				{
					if ((*iterE).moveTimer % 77 > 72)
					{
						for (int i = 0; i < 3; i++)
							createEnemyBuilt((*iterE).x, (*iterE).y + 100 * i);
					}
					if ((*iterE).moveTimer % 100 > 50 && (*iterE).moveTimer % 100 < 55)
					{
						for (int i = 0; i < 10; i++)
						{
							int randYPos = rand() % SCREEN_HEIGHT;
							createEnemyBuilt((*iterE).x, randYPos);
						}
						
					}
					
				}
				// 보스2
				else if ((*iterE).type == 12)
				{

				}
				// 보스3
				else if ((*iterE).type == 13)
				{

				}
				// 보스 최종
				else
				{

				}

				(*iterE).moveTimer += 1;
			}

			(*iterE).x += (*iterE).dx;
			(*iterE).y += (*iterE).dy;

			locateObject((*iterE).obj, backgroundScene, (*iterE).x, (*iterE).y);


			//총알과 부딫혔는지
			hit = isBulitHit((*iterE).x, (*iterE).y, ENEMY_1_WIDHT, ENEMY_1_HEIGHT);
			if (hit.first)
			{
				(*iterE).hp -= hit.second;
				if ((*iterE).hp <= 0)
				{
					if ((*iterE).type == 11 || (*iterE).type == 12 || (*iterE).type == 13 || (*iterE).type == 14)
					{
						bossAppeared = false;
						showMessage("Clear!")
					}
					destroy = true;
					heroAgent.cumulatedExp += (*iterE).exp;
					checkLevel();
				}

			}
			//조종대상에 부딫혔을때
			if(!(heroAgent.invincible)&&isAgentHit((*iterE).x, (*iterE).y,AGENT_WIDTH,AGENT_HEIGHT))
			{
				heroAgent.life--;
				if (heroAgent.life == -1)
				{
					endGame(false);
					return;
				}
				hideObject(heroAgent.lifeObjList.back());
				heroAgent.lifeObjList.pop_back();
				reGenAgent();

			}
			else if ((*iterE).x < -100 || destroy)
			{
				hideObject((*iterE).obj);
				iterE = enemyList1.erase(iterE); // 범위를 벗어났을때 목록에서 제거
			}
			else
			{
				iterE++;
			}
		}


		setTimer(objectUpdateTimer, OBJECT_UPDATE_TIME);
		startTimer(objectUpdateTimer);
	}
	else if (timer == backgroundMovingTimer)
	{
		backSceneObject.x -= backSceneObject.dx;
		locateObject(backSceneObject.obj, backgroundScene, backSceneObject.x, backSceneObject.y);

		if (backSceneObject.x < -3596)
			backSceneObject.x = 1280;

		setTimer(backgroundMovingTimer, 0.1f);
		startTimer(backgroundMovingTimer);
	}
	else if (timer == enemy1GenTimer)
	{
		if (!bossAppeared)
		{
			int randType;

			if (heroAgent.level >= 3)
				randType = (rand() % 4) + 1;
			else if (heroAgent.level >= 2)
				randType = (rand() % 3) + 1;
			else if (heroAgent.level >= 1) {
				randType = (rand() % 2) + 1;
			}
			else if (heroAgent.level >= 0) {
				randType = 1;
			}
			createEnemy(randType);
		}

		setTimer(enemy1GenTimer, ENEMY_1_GEN_RATE);
		startTimer(enemy1GenTimer);
	}
	else if (timer == reGenTimer)
	{
		if (heroAgent.shown)
		{
			heroAgent.invincibleTimeCount--;
			hideObject(heroAgent.obj);
			heroAgent.shown = false;
		}
		else
		{
			heroAgent.invincibleTimeCount--;
			showObject(heroAgent.obj);
			heroAgent.shown = true;
		}
		if (heroAgent.invincibleTimeCount == 0)
		{
			if(heroAgent.ultimateActivate==false)
				heroAgent.invincible = false;
			showObject(heroAgent.obj);
		}
		else
		{
			setTimer(reGenTimer, REGEN_FRAME_RATE);
			startTimer(reGenTimer);
		}
	}
	else if (timer == ultimateTimer)
	{
		heroAgent.ultimateTimeCount--;
		if (heroAgent.ultimateTimeCount== ULTIMATE_TIME_COUNT-1)
		{
			setObjectImage(heroAgent.ultimateObj, "Images/laser2.png");
		}
		else if (heroAgent.ultimateTimeCount == ULTIMATE_TIME_COUNT - 2)
		{
			setObjectImage(heroAgent.ultimateObj, "Images/laser1.png");
		}
		else if (heroAgent.ultimateTimeCount == ULTIMATE_TIME_COUNT - 3)
		{
			setObjectImage(heroAgent.ultimateObj, "Images/laser.png");
		}
		else if (heroAgent.ultimateTimeCount == 3)
		{
			setObjectImage(heroAgent.ultimateObj, "Images/laser1.png");
		}
		else if (heroAgent.ultimateTimeCount == 2)
		{
			setObjectImage(heroAgent.ultimateObj, "Images/laser2.png");
		}
		else if (heroAgent.ultimateTimeCount == 1)
		{
			setObjectImage(heroAgent.ultimateObj, "Images/laser3.png");
		}
		if (heroAgent.ultimateTimeCount>0)
		{


			list<enemyObject>::iterator iterE;
			for (iterE = enemyList1.begin(); iterE != enemyList1.end(); )
			{
				if (heroAgent.y-50 < (*iterE).y && (*iterE).y < heroAgent.y + 50 && heroAgent.x< (*iterE).x )
				{
					(*iterE).hp -= 10;
					if ((*iterE).hp <= 0)
					{
						if ((*iterE).type == 11 || (*iterE).type == 12 || (*iterE).type == 13 || (*iterE).type == 14)
							bossAppeared = false;

						heroAgent.cumulatedExp += (*iterE).exp;
						checkLevel();
						hideObject((*iterE).obj);
						iterE = enemyList1.erase(iterE);
					}
				}
				else
				{
					iterE++;
				}
			}
			setTimer(ultimateTimer, ULTIMATE_FRAME_RATE);
			startTimer(ultimateTimer);
		}
		else 
		{
			heroAgent.ultimateActivate = false;
			heroAgent.invincible = false;
			hideObject(heroAgent.ultimateObj);
		}
		

	}
}


void keyboardCallback(KeyCode kc, KeyState ks)
{
	if (gameStarted == false)
		return;
	if (ks == KeyState::KEYBOARD_PRESSED)
	{
		if (kc == 83)//오른족
		{
			heroAgent.dx = AGENT_SPEED;
		}
		else if (kc == 84)//위
		{
			heroAgent.dy = AGENT_SPEED;
		}
		else if (kc == 85) // 아래
		{
			heroAgent.dy = -AGENT_SPEED;
		}
		else if (kc == 82) //왼쪽
		{
			heroAgent.dx = -AGENT_SPEED;
		}
		else if (kc == 1) // a(공격)
		{
			createBulit();
		}
		else if (heroAgent.ultimateActivate==false&&kc == 19 )// s 필살기
		{
			if (heroAgent.ultimate == 0)
			{
				showMessage("궁극기 부족소리");
			}
			else
				createUltimate();
		}
		setTimer(agentUpdateTimer, AGENT_UPDATE_TIME);
		startTimer(agentUpdateTimer);
	}
	else if (ks == KeyState::KEYBOARD_RELEASED)
	{
		if (kc == 83)//오른족
		{
			if(heroAgent.dx == AGENT_SPEED)
				heroAgent.dx=0;
		}
		else if (kc == 84)//위
		{
			if (heroAgent.dy == AGENT_SPEED)
				heroAgent.dy = 0;
		}
		else if (kc == 85) // 아래
		{
			if (heroAgent.dy == -AGENT_SPEED)
				heroAgent.dy = 0;
		}
		else if (kc == 82) //왼쪽
		{
			if (heroAgent.dx == -AGENT_SPEED)
				heroAgent.dx = 0;
		}
		if (heroAgent.dx == 0 && heroAgent.dy == 0)
			stopTimer(agentUpdateTimer);
	}
}

/*
utils
*/
void startGame()
{
	gameStarted = true;

	//hide object
	hideObject(startButtonObj);
	hideObject(endButtonObj);

	initObjects();
	
	//timers

	/*
	backgroundMovingTimer = createTimer(0.1f);
	*/
	initTimers();

	startTimer(backgroundMovingTimer);
	startTimer(enemy1GenTimer);
	startTimer(objectUpdateTimer);
	startTimer(agentUpdateTimer);

}

void endGame(bool success)
{
	stopTimer(agentUpdateTimer);
	stopTimer(enemy1GenTimer);
	stopTimer(backgroundMovingTimer);
	stopTimer(objectUpdateTimer);

	gameStarted = false;

	setObjectImage(startButtonObj, "Images/restart.png");
	showObject(startButtonObj);
	showObject(endButtonObj);



	locateObject(heroAgent.obj, backgroundScene, heroAgent.x, heroAgent.y);
}

ObjectID createObject(const char* image, SceneID scene, int x, int y, bool shown)
{
	ObjectID object = createObject(image);
	locateObject(object, backgroundScene, x, y);

	if (shown) {
		showObject(object);
	}

	return object;
}

/*
actions
*/

void agentMove(int dx, int dy)
{
	//x,y축 이동범위 제한
	if (heroAgent.x + heroAgent.dx > SCREEN_WIDTH || heroAgent.x + heroAgent.dx < 0 
		|| heroAgent.y + heroAgent.dy > SCREEN_HEIGHT-100 || heroAgent.y + heroAgent.dy < 0)
	{
		return;
	}
	heroAgent.x += heroAgent.dx;
	heroAgent.y += heroAgent.dy;

	locateObject(heroAgent.obj, backgroundScene, heroAgent.x, heroAgent.y);
	locateObject(heroAgent.ultimateObj, backgroundScene, heroAgent.x+60, heroAgent.y+10);
	if (heroAgent.x > 1280) {
		endGame(true);
	}

	
}

void createEnemy(int type)
{
	enemyObject e;
	int yPos = rand() % (SCREEN_HEIGHT - 100);
	int dx = -((rand() % 10) + 2);//가로속도

	if (type == 1)
	{
		e.type = type;
		e.x = SCREEN_WIDTH;
		e.y = yPos;
		e.dx = dx;
		e.dy = 0;
		e.hp = 30;
		e.exp = 10;
		e.obj = createObject("Images/e1.png", backgroundScene, e.x, e.y, true);
		enemyList1.push_back(e);
	}
	else if (type == 2)
	{
		e.type = type;
		e.x = SCREEN_WIDTH;
		e.y = yPos;
		e.dx = dx;
		e.dy = 13;
		e.hp = 50;
		e.exp = 15;
		e.obj = createObject("Images/e2.png", backgroundScene, e.x, e.y, true);
		enemyList1.push_back(e);
	}
	else if (type == 3)
	{
		e.type = type;
		e.x = SCREEN_WIDTH;
		e.y = yPos;
		e.dx = dx;
		e.dy = 0;
		e.hp = 80;
		e.exp = 20;
		e.obj = createObject("Images/e3.png", backgroundScene, e.x, e.y, true);
		enemyList1.push_back(e);
	}
	else if (type == 4)
	{
		e.type = type;
		e.x = SCREEN_WIDTH;
		e.y = yPos;
		e.dx = -20;
		e.dy = 0;
		e.hp = 50;
		e.exp = 25;
		e.obj = createObject("Images/e4.png", backgroundScene, e.x, e.y, true);
		enemyList1.push_back(e);
	}
}

// 적4 미사일
void createEnemyBuilt(int x, int y)
{
	enemyObject e;
	e.x = x;
	e.y = y;
	e.dx = -10;
	e.dy = 0;
	e.hp = 1;
	e.exp = 1;
	e.obj = createObject("Images/enemyBuilt.png", backgroundScene, e.x, e.y, true);
	enemyList1.push_back(e);
}

void createBoss(int type)
{
	bossAppeared = true;

	enemyObject e;
	e.x = SCREEN_WIDTH;
	e.y = 200;
	e.dx = -50;
	e.dy = 0;
	e.exp = 0;
	// 보스 1
	if (type == 1)
	{
		e.type = 11;
		e.hp = 500;
		e.obj = createObject("Images/b1.png", backgroundScene, e.x, e.y, true);
		enemyList1.push_back(e);
	}
	// 보스 2
	else if (type == 2)
	{
		e.type = 12;
		e.hp = 1000;
		e.obj = createObject("Images/b2.png", backgroundScene, e.x, e.y, true);
		enemyList1.push_back(e);
	}
	// 보스 3
	else if (type == 3)
	{

	}
	// 최종
	else if(type == 4)
	{

	}

	showMessage("Warning !! Boss appears !!");
}


void createBulit()
{

	bulitObject b;
	int x = heroAgent.x+60;
	int y = heroAgent.y+20;
	int dx = 50;//탄속
	int dy = 0;

	if (heroAgent.level== 0)
	{
		b.x = x;
		b.y = y;
		b.dx = dx;
		b.dy = dy;
		b.damage = 10;
		b.obj = createObject("Images/bulit1.png", backgroundScene, b.x, b.y,true);
		bulitList.push_back(b);
	}
	else if(heroAgent.level == 1)
	{
		b.x = x;
		b.y = y;
		b.dx = dx;
		b.dy = dy;
		b.damage = 15;
		b.obj = createObject("Images/bulit2.png", backgroundScene, b.x, b.y, true);
		bulitList.push_back(b);
	}
	else if (heroAgent.level == 2)
	{
		b.x = x;
		b.y = y;
		b.dx = dx;
		b.dy = dy;
		b.damage = 20;
		b.obj = createObject("Images/bulit3.png", backgroundScene, b.x, b.y, true);
		bulitList.push_back(b);
	}
	else if (heroAgent.level == 3)
	{
		b.x = x;
		b.y = y;
		b.dx = dx;
		b.dy = dy;
		b.damage = 30;
		b.obj = createObject("Images/bulit4.png", backgroundScene, b.x, b.y, true);
		bulitList.push_back(b);
	}
}


void initTimers()
{

	setTimer(backgroundMovingTimer, 0.3f);
	setTimer(enemy1GenTimer, ENEMY_1_GEN_RATE);
	setTimer(objectUpdateTimer, OBJECT_UPDATE_TIME);
	setTimer(agentUpdateTimer, AGENT_UPDATE_TIME);

}

void initObjects()
{
	initEnemys();
	initHeroAgent();
	initBackScene();
	initGameUI();
}

void initBackScene()
{
	backSceneObject.x = 0;
	backSceneObject.y = -150;
}
void initGameUI()
{

}

void initHeroAgent()
{
	heroAgent.x = 0;
	heroAgent.y = 500;
	heroAgent.dx = 0;
	heroAgent.dy = 0;
	heroAgent.cumulatedExp = 0;
	heroAgent.level = 0;
	heroAgent.life = 2;
	heroAgent.invincible = false;
	heroAgent.ultimate = 2;
	heroAgent.shown = true;


	setObjectImage(heroAgent.obj, "Images/agent.png");
	showObject(heroAgent.obj);
	showObject(heroAgent.ultimateIconObj);
	setObjectImage(heroAgent.ultimateCountObj,"Images/2.png");
	showObject(heroAgent.ultimateCountObj);

	for( int i =0; i< INITIAL_LIFE; i++)
		heroAgent.lifeObjList.push_back(createObject("Images/life.png", backgroundScene, 70*i, 0, true));

}

void initBulits()
{
	list<bulitObject>::iterator iterB;
	for (iterB = bulitList.begin(); iterB != bulitList.end(); )
	{
		hideObject((*iterB).obj);
		iterB = bulitList.erase(iterB);
	}
}
void initEnemys()
{
	list<enemyObject>::iterator iterE;
	for (iterE = enemyList1.begin(); iterE != enemyList1.end(); )
	{
		hideObject((*iterE).obj);
		iterE = enemyList1.erase(iterE);
	}
}

bool isAgentHit(int x, int y, int width, int height)
{
	return (heroAgent.x < x&& x < heroAgent.x + width
		&& heroAgent.y < y&& y < heroAgent.y + height);
}

pair<bool,int> isBulitHit(int x, int y, int width, int height)
{
	list<bulitObject>::iterator iterB;
	if(bulitList.empty())
		return make_pair(false, 0);


	for (iterB = bulitList.begin(); iterB != bulitList.end(); )
	{
		int damage = 0;
		if ( x- 100< (*iterB).x && (*iterB).x < x + width + 100
			&& y -100 < (*iterB).y && (*iterB).y < y + height + 100)
		{
			hideObject((*iterB).obj);

			damage = (*iterB).damage;
			iterB = bulitList.erase(iterB);
			return make_pair(true, damage);
		}
		else
			iterB++;
	}
	return make_pair(false,0);

}
void checkLevel()
{
	char image[30];
	printf("cur exp :%d\n", heroAgent.cumulatedExp);
	if (heroAgent.cumulatedExp > 1000 && (heroAgent.level == 3) && (heroAgent.cumulatedExp % 500)==0 )
	{
		//궁극기증가
		if (heroAgent.ultimate < 5)
		{
			heroAgent.ultimate++;
			sprintf(image, "Images/%d.png", heroAgent.ultimate);
			setObjectImage(heroAgent.ultimateCountObj, image);
		}
		
	}
	else if (heroAgent.cumulatedExp > 1000 && (heroAgent.level == 2))
	{
		//궁극기증가
		heroAgent.ultimate++;
		sprintf(image, "Images/%d.png", heroAgent.ultimate);
		setObjectImage(heroAgent.ultimateCountObj, image);
		setObjectImage(heroAgent.obj, "Images/agent3.png");
		showMessage("레벨업 - 레벨 3");
		heroAgent.level = 3;
	}
	else if (heroAgent.cumulatedExp > 400 && (heroAgent.level == 1))
	{
		//궁극기증가
		heroAgent.ultimate++;
		sprintf(image, "Images/%d.png", heroAgent.ultimate);
		setObjectImage(heroAgent.ultimateCountObj, image);
		setObjectImage(heroAgent.obj, "Images/agent2.png");
		showMessage("레벨업 - 레벨 2");

		heroAgent.level = 2;

		if (heroAgent.cumulatedExp > 400 && boss[1] == 0)
		{
			boss[1] = 1;
			createBoss(heroAgent.level);
		}

	}
	else if (heroAgent.cumulatedExp > 150 && (heroAgent.level == 0))
	{
		//궁극기증가
		heroAgent.ultimate++;
		sprintf(image, "Images/%d.png", heroAgent.ultimate);
		setObjectImage(heroAgent.ultimateCountObj, image);
		setObjectImage(heroAgent.obj, "Images/agent1.png");


		showMessage("레벨업 - 레벨 1");
		heroAgent.level = 1;

		if (heroAgent.cumulatedExp > 150 && boss[0] == 0)
		{
			boss[0] = 1;
			createBoss(heroAgent.level);
		}
	}
}

void reGenAgent()
{
	heroAgent.invincibleTimeCount = 20;
	heroAgent.invincible = true;
	setTimer(reGenTimer, REGEN_FRAME_RATE);
	startTimer(reGenTimer);
}

void createUltimate()
{
	char image[30];
	heroAgent.invincible = true;
	heroAgent.ultimateActivate = true;
	heroAgent.ultimate--;
	heroAgent.ultimateTimeCount = ULTIMATE_TIME_COUNT;
	sprintf(image, "Images/%d.png", heroAgent.ultimate);
	setObjectImage(heroAgent.ultimateCountObj, image);
	showObject(heroAgent.ultimateObj);

	setTimer(ultimateTimer, ULTIMATE_FRAME_RATE);
	startTimer(ultimateTimer);
}
