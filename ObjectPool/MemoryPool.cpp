#define PROFILE

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <Windows.h>
#include "MemoryPool.h"
#include "Profile.h"
#include "ProfileInit.h"

class Player
{
private:
	int _x;
	int _y;
	int _hp;
	char _str[100];

public:
	Player()
	{
		_x = 10;
		_y = 20;
		_hp = 100;
		//wprintf(L"Player()\n");
	}

	void Attack()
	{
		_hp = 50;
	}

	~Player()
	{
		//wprintf(L"~Player()\n");
	}

	int getX() { return _x; }
	int getY() { return _y; }
	int getHP() { return _hp; }
};



int wmain()
{
	MemoryPool<Player> playerPool(0, false);
	int count = 0;

	while (count <= 1000000)
	{
		count++;
		Player* player;
		{
			Profile profile(L"MeomoryPool_New");
			player = playerPool.Alloc();
		}

		{
			Profile profile(L"MeomoryPool_Delete");
			playerPool.Free(player);
		}

		{
			Profile profile(L"New");
			player = new Player;
		}

		{
			Profile profile(L"Delete");
			delete player;
		}
	}
	return 0;
}