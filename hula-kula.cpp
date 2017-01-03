#include <stdio.h>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>
#include "sdlpixel.h"
#include "SDL_tools.h"
#include <time.h>
#include <vector>
#include <algorithm>
#include <queue>
#include <string>
#include <windows.h>

#define All_Enemies for (iE=0; iE<enemyCount; iE++)
#define All_Players for (iP=0; iP<playerCount; iP++)
#define All_EnemyImg for (eImgI=0; eImgI<enemyImgCount; eImgI++)
#define All_Bonuses for (iB=0; iB<bonusCount; iB++)

#undef main

using namespace std;

struct offset
{
	int x, y;
};

int W = 1280, H = 720, HitTime = 250, alphaMin = 200, enemyCount = 100, enemyImgCount = 16,
	newEnemyIntervalMin = 50, newEnemyIntervalMax = 400, playerCount = 4, hitSoundCount = 16,
	difficultyInterval = 500, xShift = 200, vX_PlayerRatio = 10, bonusCount = 4, bonusIntervalMin = 4000,
	bonusIntervalMax = 6000, bonusSoundCount = 4, playerImgShift = 100, playerTextShift = 300;
int lifeTextPosition[] = {3, 1, 2, 4};
double vY_PlayerBegin = 0.75, vX_PlayerBegin = 0.04, vX_EnemyRatio = 1.5, vX_Bonus = 0.25,
	vY_BonusRatio = 0.05, vX_BonusRatio = 1;
SDL_Color livesTextColor[] = {{255, 255, 0}, {255, 0, 0}, {0, 255, 255}, {0, 0, 255}};

int vX_EnemyMinExp4, vX_EnemyMaxExp4;

class Object
{
	private:
		Timer tX, tY;
	public:
		Object();
		int left, right, top, bottom, lives;
		double vX, vY, x, y;
		bool hit, freeze;
		SDL_Surface* imgIn;
		SDL_Surface** img_ptr;
		Timer tHit;
		vector<offset> pixels;
		
		void show(), tStart(), tStop(), placeX(int), placeY(int), place(int, int), array_pixelsImg(), position(), array_pixels();
		void moveX(), moveY(), moveXF(), moveYF(), moveXFY();
		bool collision(Object);
		SDL_Surface* img();
};

int RandOffset(int beg, int end, int l)
{
	return beg + rand() % (end-beg-l+1);
}
bool cmp_offset(offset a, offset b)
{
	if (a.x != b.x) return a.x < b.x;
	return a.y < b.y;
}
void Resize()
{
	screen = SDL_SetVideoMode(W, H, 0, SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_RESIZABLE);
	vX_EnemyMinExp4 = W * 4;
	vX_EnemyMaxExp4 = W * 8;
}

int main()
{
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	TTF_Init();
	Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 4096);

	int iP, iE, iB, eImgI, i, winnerI = 0, loserI = 0, maxPlayerW = 0, maxPlayerCount; 
	int bonusFactor[bonusCount], playerTime[playerCount], winner[playerCount], loser[playerCount], livesTextX[playerCount];
	double vY_Player[playerCount], vX_Player[playerCount];
	string file;
	char tmpString[20];
	bool quit = false, wait = true;
	queue<int> avaibleEnemy;
	
	SDL_Surface* background;
	SDL_Surface* enemyImg[enemyImgCount];
	SDL_Event event;
	Uint8 *keystate = SDL_GetKeyState(NULL);
	Mix_Chunk* hitSound[hitSoundCount];
	Mix_Chunk* bonusSound[bonusSoundCount];
	Mix_Music* music;
	TTF_Font* defaultFont = TTF_OpenFont("font.ttf", 28);
	TTF_Font* boardFont = TTF_OpenFont("font.ttf", 72);
	TTF_Font* beginFont = TTF_OpenFont("font.ttf", 50);
	SDL_Color placeTextColor = {255, 255, 255}, red = {255, 0, 0}, green = {0, 255, 0}, beginColor = {250, 200, 0};
	
	Timer newEnemy((newEnemyIntervalMin + newEnemyIntervalMax)/2);
	Timer timerDifficulty(difficultyInterval);
	Timer gameTimer;
	Object player[playerCount], enemy[enemyCount], bonus[bonusCount];
	
	SDL_WM_SetCaption("Hula-Kula 1.3b", NULL);	
	putenv("SDL_VIDEO_CENTERED=1");
	Resize();
	background = SDL_DisplayFormat(IMG_Load("background.png"));
	
	srand(time(NULL));
	FreeConsole();
	
	All_Players
	{
		player[iP].vX = vX_PlayerBegin;
		player[iP].tHit.interval = HitTime;
		sprintf(tmpString, "%d", iP);
		file = "players/player_" + (string)tmpString + ".png";
		player[iP].imgIn = SDL_DisplayFormatAlpha(IMG_Load(file.c_str()));
		player[iP].img_ptr = &(player[iP].imgIn);
		if (player[iP].img()->w > maxPlayerW) maxPlayerW = player[iP].img()->w;
		player[iP].array_pixels();
		vY_Player[iP] = vY_PlayerBegin;
		vX_Player[iP] = vX_PlayerBegin;
	}
	
	All_EnemyImg
	{
		sprintf(tmpString, "%d", eImgI);
		file = "enemies/enemy_" + (string)tmpString + ".png";
		enemyImg[eImgI] = SDL_DisplayFormatAlpha(IMG_Load(file.c_str()));
	}
	
	i = 0;
	All_Enemies
	{
		if (i >= enemyImgCount) i = 0;
		enemy[iE].img_ptr = &(enemyImg[i]);
		enemy[iE].array_pixels();
		avaibleEnemy.push(iE);
		i++;
	}
	
	bonusFactor[0] = 1;
	for (i=1; i<bonusCount; i++) bonusFactor[i] = bonusFactor[i-1] * 2;
	All_Bonuses
	{
		sprintf(tmpString, "%d", iB);
		file = "bonuses/bonus_" + (string)tmpString + ".png";
		bonus[iB].imgIn = SDL_DisplayFormatAlpha(IMG_Load(file.c_str()));
		bonus[iB].img_ptr = &(bonus[iB].imgIn);
		bonus[iB].x = -xShift;
		bonus[iB].array_pixels();
		bonus[iB].tHit.interval = bonusIntervalMin + rand() % (bonusIntervalMax - bonusIntervalMin + 1);
		bonus[iB].tHit.interval *= bonusFactor[iB];
		bonus[iB].tHit.start();
	}
	
	for (i=0; i<hitSoundCount; i++)
	{
		sprintf(tmpString, "%d", i);
		file = "sounds/hit_" + (string)tmpString + ".wav";
		hitSound[i] = Mix_LoadWAV(file.c_str());
	}
	music = Mix_LoadMUS("music.wav");
	
	for (i=0; i<bonusSoundCount; i++)
	{
		sprintf(tmpString, "%d", i);
		file = "sounds/bonus_" + (string)tmpString + ".wav";
		bonusSound[i] = Mix_LoadWAV(file.c_str());
	}
	
	maxPlayerCount = playerCount;
	
	while (!quit && wait)
	{
		while(SDL_PollEvent(&event))
		{
			if (event.type == SDL_KEYDOWN)
			{
				switch (event.key.keysym.sym)
				{
					case SDLK_1:
						playerCount = 1;
						wait = false;
						break;
					case SDLK_2:
						playerCount = 2;
						wait = false;
						break;
					case SDLK_3:
						playerCount = 3;
						wait = false;
						break;
					case SDLK_4:
						playerCount = 4;
						wait = false;
						break;
				}
			}
			else if (event.type == SDL_QUIT) quit = true;
			else if (event.type == SDL_VIDEORESIZE)
			{
				W = event.resize.w;
				H = event.resize.h;
				Resize();
			}
		}
		ApplySurface(0, 0, background);
		ShowText((char*)"1 - 1 gracz", W/2, H/5, beginFont, beginColor);
		ShowText((char*)"2 - 2 graczy", W/2, 2*H/5, beginFont, beginColor);
		ShowText((char*)"3 - 3 graczy", W/2, 3*H/5, beginFont, beginColor);
		ShowText((char*)"4 - 4 graczy", W/2, 4*H/5, beginFont, beginColor);
		SDL_Flip(screen);
	}
	
	All_Players
	{
		player[iP].place(W/8, H*(iP+1)/(maxPlayerCount+1));
		livesTextX[iP] = W*lifeTextPosition[iP]/(maxPlayerCount+1);
	}
	All_Enemies enemy[iE].x = W + xShift;
	
	newEnemy.start();
	timerDifficulty.start();
	Mix_PlayMusic(music, -1);
	gameTimer.start();
	
	while (!quit)
	{
		while(SDL_PollEvent(&event))
		{
			if (event.type == SDL_KEYDOWN)
			{
				switch (event.key.keysym.sym)
				{
					case SDLK_UP:
						player[0].vY = -vY_Player[0];
						break;
					case SDLK_DOWN:
						player[0].vY = vY_Player[0];
						break;
					case SDLK_w:
						player[1].vY = -vY_Player[1];
						break;
					case SDLK_s:
						player[1].vY = vY_Player[1];
						break;
					case SDLK_i:
						player[2].vY = -vY_Player[2];
						break;
					case SDLK_k:
						player[2].vY = vY_Player[2];
						break;
					case SDLK_KP8:
						player[3].vY = -vY_Player[3];
						break;
					case SDLK_KP5:
						player[3].vY = vY_Player[3];
						break;
					case SDLK_m:
						if (Mix_PausedMusic()) Mix_ResumeMusic();
						else Mix_PauseMusic();
						break;
				}
			}
			else if (event.type == SDL_KEYUP)
			{
				switch (event.key.keysym.sym)
				{
					case SDLK_UP:
						if (keystate[SDLK_DOWN]) player[0].vY = vY_Player[0];
						else player[0].vY = 0;
						break;
					case SDLK_DOWN:
						if (keystate[SDLK_UP]) player[0].vY = -vY_Player[0];
						else player[0].vY = 0;
						break;
					case SDLK_w:
						if (keystate[SDLK_s]) player[1].vY = vY_Player[0];
						else player[1].vY = 0;
						break;
					case SDLK_s:
						if (keystate[SDLK_w]) player[1].vY = -vY_Player[0];
						else player[1].vY = 0;
						break;
					case SDLK_i:
						if (keystate[SDLK_k]) player[2].vY = vY_Player[0];
						else player[2].vY = 0;
						break;
					case SDLK_k:
						if (keystate[SDLK_i]) player[2].vY = -vY_Player[0];
						else player[2].vY = 0;
						break;
					case SDLK_KP8:
						if (keystate[SDLK_KP5]) player[3].vY = vY_Player[0];
						else player[3].vY = 0;
						break;
					case SDLK_KP5:
						if (keystate[SDLK_KP8]) player[3].vY = -vY_Player[0];
						else player[3].vY = 0;
						break;
				}
			}
			else if (event.type == SDL_VIDEORESIZE)
			{
				W = event.resize.w;
				H = event.resize.h;
				Resize();
				while (!avaibleEnemy.empty()) avaibleEnemy.pop();
				All_Enemies
				{
					enemy[iE].x = W + xShift;
					enemy[iE].tStop();
					avaibleEnemy.push(iE);
				}
				All_Bonuses
				{
					bonus[iB].x = -xShift;
					bonus[iB].tHit.start();
				}
			}
			else if (event.type == SDL_QUIT) quit = true;
		}
		
		//--------------------------------------------------------------------------
		if (newEnemy.finish() && !avaibleEnemy.empty())
		{
			enemy[avaibleEnemy.front()].tStart();
			enemy[avaibleEnemy.front()].vX = -double(vX_EnemyMinExp4 + (rand() % (vX_EnemyMaxExp4 - vX_EnemyMinExp4)))/10000;
			enemy[avaibleEnemy.front()].y = RandOffset(0, H, enemy[avaibleEnemy.front()].img()->h);
			avaibleEnemy.pop();
			newEnemy.interval = newEnemyIntervalMin + rand() % (newEnemyIntervalMax - newEnemyIntervalMin + 1);
		}
		
		All_Enemies
		{
			if (enemy[iE].x <= -enemy[iE].img()->w || (enemy[iE].x >= W && enemy[iE].vX > 0))
			{
				enemy[iE].x = W+xShift;
				enemy[iE].vX = 0;
				avaibleEnemy.push(iE);
			}
		}
		
		All_Players
		{
			if (!player[iP].freeze)
			{
				All_Enemies
				{
					if (enemy[iE].vX < 0 && player[iP].collision(enemy[iE]))
					{
						Mix_PlayChannel(-1, hitSound[rand() % hitSoundCount], 0);
						enemy[iE].vX *= -vX_EnemyRatio;
						if (player[iP].lives == 0)
						{
							if (!player[iP].hit) player[iP].vX = -vX_PlayerBegin * vX_PlayerRatio;
							player[iP].tHit.start();
							player[iP].hit = true;
						}
						else player[iP].lives--;
						break;
					}
				}
			}
			if (player[iP].tHit.finishStop())
			{
				player[iP].vX = vX_Player[iP];
				player[iP].hit = false;
			}
			
			if (!player[iP].freeze)
			{
				if (player[iP].x <= -player[iP].img()->w)
				{
					player[iP].freeze = true;
					loser[loserI++] = iP;
				}
				else if (player[iP].x >= W)
				{
					playerTime[iP] = gameTimer.time()/1000;
					player[iP].freeze = true;
					winner[winnerI++] = iP;
				}
			}
		}
		
		if (timerDifficulty.finish())
		{
			if (newEnemyIntervalMin > 1) newEnemyIntervalMin--;
			if (newEnemyIntervalMax > 1) newEnemyIntervalMax--;
		}
		
		All_Bonuses
		{
			if (bonus[iB].tHit.finishStop())
			{
				bonus[iB].y = RandOffset(0, H, bonus[iB].img()->h);
				bonus[iB].vX = vX_Bonus;
			}
			else if (bonus[iB].x >= W)
			{
				bonus[iB].x = -xShift;
				bonus[iB].vX = 0;
				bonus[iB].tHit.interval = bonusIntervalMin + rand() % (bonusIntervalMax - bonusIntervalMin + 1);
				bonus[iB].tHit.interval *= bonusFactor[iB];
				bonus[iB].tHit.start();
			}
			else
			{
				All_Players
				{
					if (!player[iP].freeze)
					{
						if (bonus[iB].collision(player[iP]))
						{
							Mix_PlayChannel(-1, bonusSound[rand() % bonusSoundCount], 0);
							bonus[iB].x = -xShift;
							bonus[iB].vX = 0;
							bonus[iB].tHit.interval = bonusIntervalMin + rand() % (bonusIntervalMax - bonusIntervalMin + 1);
							bonus[iB].tHit.interval *= bonusFactor[iB];
							bonus[iB].tHit.start();
							switch (iB)
							{
								case 0:
									vY_Player[iP] += vY_PlayerBegin * vY_BonusRatio;
									break;
								case 1:
									player[iP].lives += 3;
									break;
								case 2:
									vX_Player[iP] += vX_PlayerBegin * vX_BonusRatio;
									break;
								case 3:
									player[iP].lives += 3;
									vY_Player[iP] += vY_PlayerBegin * vY_BonusRatio;
									vX_Player[iP] += vX_PlayerBegin * vX_BonusRatio;
									break;
							}
						}
					}
				}
			}
		}
		
		All_Enemies enemy[iE].moveXFY();
		All_Players player[iP].moveXFY();
		All_Bonuses bonus[iB].moveXFY();
		
		//--------------------------------------------------------------------------
		ApplySurface(0, 0, background);
		All_Players player[iP].show();
		All_Bonuses bonus[iB].show();
		All_Enemies enemy[iE].show();
		
		All_Players
		{
			sprintf(tmpString, "%d", player[iP].lives);
			ShowTextF(tmpString, livesTextX[iP], 0, defaultFont, livesTextColor[iP]);
		}
		
		if (winnerI + loserI == playerCount)
		{
			int place = 1;
			string tmp;
			for (i=0; i<winnerI; i++,place++)
			{
				sprintf(tmpString, "%d", place);
				tmp = (string)tmpString + ".";
				ShowText((char*)tmp.c_str(), W/4, H*place/(playerCount+1), boardFont, placeTextColor);
				player[winner[i]].x = W/4 + playerImgShift;
				player[winner[i]].placeY(H*place/(playerCount+1));
				player[winner[i]].show();
				sprintf(tmpString, "%d", playerTime[winner[i]]);
				string tmp = (string)tmpString + " sek";
				ShowText((char*)tmp.c_str(), W/4+maxPlayerW+playerImgShift+playerTextShift, H*place/(playerCount+1), boardFont, green);
			}
			for (i=loserI-1; i>=0; i--,place++)
			{
				sprintf(tmpString, "%d", place);
				tmp = (string)tmpString + ".";
				ShowText((char*)tmp.c_str(), W/4, H*place/(playerCount+1), boardFont, placeTextColor);
				player[loser[i]].x = W/4 + playerImgShift;
				player[loser[i]].placeY(H*place/(playerCount+1));
				player[loser[i]].show();
				ShowText((char*)"PRZEGRANA", W/4+maxPlayerW+playerImgShift+playerTextShift, H*place/(playerCount+1), boardFont, red);
			}
		}
		
		SDL_Flip(screen);
	}
	
	SDL_FreeSurface(background);
	All_EnemyImg SDL_FreeSurface(enemyImg[eImgI]);
	All_Players SDL_FreeSurface(player[iP].imgIn);
	TTF_CloseFont(defaultFont);
	TTF_CloseFont(boardFont);
	TTF_CloseFont(beginFont);
	All_Bonuses SDL_FreeSurface(bonus[iB].imgIn);
	
	for (i=0; i<hitSoundCount; i++) Mix_FreeChunk(hitSound[i]);
	for (i=0; i<bonusSoundCount; i++) Mix_FreeChunk(bonusSound[i]);
	Mix_FreeMusic(music);
	
	Mix_CloseAudio();
	TTF_Quit();
	SDL_Quit();
	
	return 0;
}
Object::Object()
{
	x = y = vX = vY = lives = 0;
	hit = freeze = false;
}
void Object::show()
{
	ApplySurface(x, y, img());
}
void Object::moveXF()
{
	if (!freeze)
	{
		x += vX * tX.time();
		tX.start();
	}
}
void Object::moveYF()
{
	if (!freeze)
	{
		y += vY * tY.time();
		tY.start();
	}
}
void Object::moveX()
{
	if (!freeze)
	{
		moveXF();
		if (x < 0) x = 0;
		else if (x + img()->w > W) x = W - img()->w;
	}
}
void Object::moveY()
{
	if (!freeze)
	{
		moveYF();
		if (y < 0) y = 0;
		else if (y + img()->h > H) y = H - img()->h;
	}
}
void Object::moveXFY()
{
	if (!freeze)
	{
		moveXF();
		moveY();
	}
}
void Object::tStart()
{
	tX.start();
	tY.start();
}
void Object::tStop()
{
	tX.stop();
	tY.stop();
}
void Object::placeX(int x_c)
{
	x = x_c - img()->w/2;
}
void Object::placeY(int y_c)
{
	y = y_c - img()->h/2;
}
void Object::place(int x_c, int y_c)
{
	placeX(x_c);
	placeY(y_c);
}
void Object::array_pixels()
{
	pixels.clear();
	offset tmp;
	Uint8 R, G, B, A;
	for (int iY=0; iY<img()->h; iY++)
	{
		for (int iX=0; iX<img()->w; iX++)
		{
			SDL_GetRGBA(getpixel(img(), iX, iY), img()->format, &R, &G, &B, &A);
			if (A >= alphaMin)
			{
				tmp.x = iX;
				tmp.y = iY;
				pixels.push_back(tmp);
			}
		}
	}
	sort(pixels.begin(), pixels.end(), cmp_offset);
}
bool Object::collision(Object obj)
{
	position();
	obj.position();
	if (left >= obj.right || right <= obj.left || top >= obj.bottom || bottom <= obj.top) return false;
	
	int iA = 0, iB = 0;
	while (iA < pixels.size() && iB < obj.pixels.size())
	{
		if (int(pixels[iA].x + x) == int(obj.pixels[iB].x + obj.x))
		{
			if (int(pixels[iA].y + y) == int(obj.pixels[iB].y + obj.y)) return true;
			if (int(pixels[iA].y + y) < int(obj.pixels[iB].y + obj.y)) iA++;
			else if (int(pixels[iA].y + y) > int(obj.pixels[iB].y + obj.y)) iB++;
		}
		else if (int(pixels[iA].x + x) < int(obj.pixels[iB].x + obj.x)) iA++;
		else iB++;
	}
	return false;
}
void Object::position()
{
	left = x;
	right = x + img()->w;
	top = y;
	bottom = y + img()->h;
}
SDL_Surface* Object::img()
{
	return *img_ptr;
}

