#include "entityManager.h"

// constructor/destructor
entityManager::entityManager()
{
	players = NULL;
	enemies = NULL;
	m_stuff = NULL;
	m_checkPoints = NULL;
	dxSound = NULL;
	checkPnt = -1;
	numPlayers = 0;
	numEnemies = 0;
	numStuff = 0;
	numCheckPoints = 0;
	victoryCondition = false;
}
entityManager::~entityManager()
{
	// destroy all ojbects pertaining to each entity
	removeAll();
	enemyFiles.~stringArray();
	playerFile.~basic_string();
}

// member functions
bool entityManager::init(objectManager * a_objMgr, sound * a_sound, std::string a_enemyFiles, std::string a_playerFile, std::string a_stuffFile, std::string a_checkPointFile)
{
	// get pointer for the objectManager
	objMgr = a_objMgr;
	dxSound = a_sound;
	
	enemyFiles.loadFromTextFile(a_enemyFiles);
	
	// make sure there are file names inside the enemy filename string array
	if(enemyFiles.getSize() == 0)
	{
		MessageBox(NULL, "No enemy files were loaded", "Entity Manager Error", MB_OK);
		return false;
	}
	else
	{
		playerFile = a_playerFile;
		stuffFile = a_stuffFile;
		checkPointFile = a_checkPointFile;
		loadPlayers();
		loadEnemies(0);
		loadStuff();
		loadCheckPoints();
		initPlayerSound();
		return true;
	}
}
void entityManager::update(float timePassed)
{
	bool updateIndexes = false;
	// update all players
	for(int i = 0; i < numPlayers; i++)
	{
		players[i]->update(timePassed);

		if(players[i]->getObject()->getCollHistory()->getList() && players[i]->isAlive())
		{
			//check for collision with enemies
			for(int j = 0; j < numEnemies; j++)
			{
				for(int g = 0; g < players[i]->getObject()->getCollHistory()->endOfList(); g++)
				{
					if(players[i]->getObject()->getCollHistory()->get(g) == enemies[j]->getObject()->getObjectIndex())
					{
						if(players[i]->hasArmor())
						{
							player * plr = (player*)players[i];
							plr->bounce();
							plr->removeArmor();
						}
						else
							players[i]->entityDead();
					}
					if(enemies[j]->getType() == entityTurret)
					{
						turret * turr = (turret *)enemies[j];
						for(int x =0; x < turr->getNumProjectiles();x++)
						{
							if(players[i]->getObject()->getCollHistory()->get(g) == turr->getProjectile(x)->getObject()->getObjectIndex())
							{
								if(players[i]->hasArmor())
								{
									player * plr = (player*)players[i];
									plr->bounce();
									plr->removeArmor();
								}
								else
									players[i]->entityDead();
							}
						}
					}
				}
			}
			//check for collision with stuff
			for(int j = 0; j < numStuff; j++)
			{
				for(int g = 0; g < players[i]->getObject()->getCollHistory()->endOfList(); g++)
				{
					if(players[i]->getObject()->getCollHistory()->get(g) == m_stuff[j]->getObject()->getObjectIndex())
					{
						player * ply = (player*)players[i];
						switch(m_stuff[j]->getType())
						{
						case stuff_teleporter:
							teleporter * tel;
							tel = (teleporter*)m_stuff[j];
							players[i]->setPosition(tel->getData());
							dxSound->playSound(soundTeleport);
							break;
						case stuff_armor:
							if(!players[i]->hasArmor())
							{
								// if player does not already have armor, give him armor, and remove it from the game world
								ply->armorPickup();
								removeFromStuff(j);
								updateIndexes = true;
							}
							break;
						case stuff_victory:
							m_stuff[j]->pickUp();
							victoryCondition = true;
							break;
						case stuff_key:
							if(!ply->playerHasKey())
							{
								ply->keyPickup();
								removeFromStuff(j);
								updateIndexes = true;
							}
							break;
						case stuff_door:
							if(ply->playerHasKey())
							{
								door * dr = (door*)m_stuff[j];
								if(!dr->isOpen())
								{
									dr->open();
									ply->removeKey();
								}
							}
							break;
						default:
							break;
						}
					}
				}
			}
			// check for collision with checkpoints
			for(int j = 0; j < numCheckPoints; j++)
			{
				for(int g = 0; g < players[i]->getObject()->getCollHistory()->endOfList(); g++)
				{
					if(players[i]->getObject()->getCollHistory()->get(g) == m_checkPoints[j]->getObject()->getObjectIndex())
					{
						if(!m_checkPoints[j]->isPickedUp())
						{
							m_checkPoints[j]->pickUp();
							D3DXVECTOR3 * pos = m_checkPoints[j]->getObject()->getPosition();
							players[i]->setDefaultPos(pos);
							dxSound->playSound(soundCheckPoint);
							if((checkPnt + 1) == j) // check if checkpoint is consecutive
								checkPnt = j;
							else
							{
								checkPnt = j;
								checkOldCheckpoints(); // if not check old checkpoints
							}
						}
					}
				}
			}
			// clear collision history
			players[i]->getObject()->getCollHistory()->resetList();
		}
	}
	
	// update all enemies
	for(int i = 0; i < numEnemies; i++)
	{
		enemies[i]->update(timePassed);
		if(enemies[i]->getObject()->getCollHistory()->getList() && enemies[i]->isAlive())
		{
			//check for collision with stuff
			for(int j = 0; j < numStuff; j++)
			{
				for(int g = 0; g < enemies[i]->getObject()->getCollHistory()->endOfList(); g++)
				{
					if(enemies[i]->getObject()->getCollHistory()->get(g) == m_stuff[j]->getObject()->getObjectIndex())	
					{
						if(m_stuff[j]->getType() == stuff_teleporter)
						{
							teleporter * tel;
							tel = (teleporter*)m_stuff[j];
							enemies[i]->setPosition(tel->getData());
						}
					}
				}
			}


			//check for collision with enemies
			for(int j = 0; j < numEnemies; j++)
			{
				for(int g = 0; g < enemies[i]->getObject()->getCollHistory()->endOfList(); g++)
				{
					if(enemies[i]->getObject()->getCollHistory()->get(g) == enemies[j]->getObject()->getObjectIndex())
					{
						// if enemies collide with each other turn around
						enemies[i]->flip();
					}
				}
			}
			enemies[i]->getObject()->getCollHistory()->resetList();
		}
		
	}

	// update all stuff
	for(int i = 0; i < numStuff; i++)
	{
		m_stuff[i]->update(timePassed);
		m_stuff[i]->getObject()->getCollHistory()->resetList();
	}
	
	// update all checkpoints
	for(int i = 0; i < numCheckPoints; i++)
	{
		m_checkPoints[i]->update(timePassed);
		m_checkPoints[i]->getObject()->getCollHistory()->resetList();
	}
	if(updateIndexes)
	{
		objMgr->getList()->setObjectIndexes();
	}
}

// load functions
void entityManager::loadPlayers()
{
	std::fstream file(playerFile.c_str());

	int size = 0;
	// count the number of strings in the text file
	file.peek();
	while(!file.eof())
	{
		int c;
		c = file.get();
		if(c == '\n' || file.eof()) 
		{size++;}
	}
	
	// clear fstream flags
	file.clear();
	// set fstream get pointer back to the beginning
	file.seekg(0, std::ios::beg);
	
	// check if there is already an existing array
	if(!players)
	{
		// if not create a new list
		players = new entity * [size];
	}
	else
	{
		// create a new array
		entity ** tempArray;
		tempArray = new entity * [numPlayers + size];
		// copy over array data
		for(int i = 0; i < numPlayers; i++)
		{
			tempArray[i] = players[i];
		}
		// delete old array and transfer new array into players
		delete [] players;
		players = tempArray;
	}

	for(int i = numPlayers; i < numPlayers + size; i++)
	{
		players[i] = new player();

		float x, y;

		file >> x >> y;

		objMgr->loadObjectsFromTxtFile("defaultPlayer.txt");
		objMgr->indexEnd();
		objMgr->getObject()->togglePhysics();
		players[i]->setObject(objMgr->getObject());
		players[i]->setPosition(x, y);
		players[i]->getObject()->setSprite(0,0);
		// also set default position for re-spawing
		players[i]->setDefaultPos(x, y);
	}
	numPlayers += size;
}
void entityManager::loadEnemies(int fileIndex)
{
	if(fileIndex < enemyFiles.getSize()) // check if index is greater than size
	{
		std::string * fileName = enemyFiles.getStringPtrAt(fileIndex);
		std::fstream file(fileName->c_str());

		int size = 0;
		// count the number of strings in the text file
		file.peek();
		while(!file.eof())
		{
			int c;
			c = file.get();
			if(c == '\n' || file.eof()) 
			{size++;}
		}

		// clear fstream flags
		file.clear();
		// set fstream get pointer back to the beginning
		file.seekg(0, std::ios::beg);

		// make the corresponding amount of entity pointers
		// check if there is already an existing array
		if(!enemies)
		{
			// if new create a new list
			enemies = new entity * [size];
		}
		else
		{
			// create a new array
			entity ** tempArray;
			tempArray = new entity * [numEnemies + size];
			// copy over array data
			for(int i = 0; i < numEnemies; i++)
			{
				tempArray[i] = enemies[i];
			}
			// delete old array and transfer new array into players
			delete [] enemies;
			enemies = tempArray;
		}

		for(int i = numEnemies; i < numEnemies + size; i++)
		{
			char enemyType, direction;
			/**********************************************************
			* In the future direction and behavior type will change the behavior of the turrets
			**********************************************************/
			float x, y;

			file >> enemyType >> x >> y >> direction;

			if(enemyType == 't') // load turret
			{
				turret * turr = new turret();
				enemies[i] = turr;

				turr->setWall(direction);

				// create 5 objects for projectiles, and send them into the turret
				object ** turrProjectiles = new object *[5];
				for(int i = 0; i < 5; i++)
				{
					turrProjectiles[i] = NULL;
					objMgr->loadObjectsFromTxtFile("defaultProjectile.txt");
					objMgr->indexEnd();
					turrProjectiles[i] = objMgr->getObject();
				}
				turr->setProjectiles(turrProjectiles, 5);

				objMgr->loadObjectsFromTxtFile("defaultTurret.txt");
			}
			else if(enemyType == 'z') // load ziggy
			{
				enemies[i] = new ziggy();
				objMgr->loadObjectsFromTxtFile("defaultZiggy.txt");
			}
			else if(enemyType == 'g') // load goomba
			{
				enemies[i] = new goomba();
				objMgr->loadObjectsFromTxtFile("defaultGoomba.txt");
			}
			else if(enemyType == 'o') // load obstacle
			{
				enemies[i] = new obstacle();
				objMgr->loadObjectsFromTxtFile("defaultObstacle.txt");
			}
			else if(enemyType == 'r') // load troll
			{
				enemies[i] = new troll();
				objMgr->loadObjectsFromTxtFile("defaultTroll.txt");
			}

			objMgr->indexEnd();
			enemies[i]->setObject(objMgr->getObject());
			enemies[i]->getObject()->setSprite(0,0);
			enemies[i]->setPosition(x, y);
			enemies[i]->setDefaultPos(x, y);
			enemies[i]->setDirection(direction);
			if(enemyType == 'g' || enemyType == 'z' || enemyType == 'r') // turn on physics for goombas and ziggy, and trolls
			{
				enemies[i]->getObject()->togglePhysics();
			}
			if(enemyType == 'z') // lower gravity for ziggy's
			{
				enemies[i]->getObject()->getPhysics()->setGravity(-10.0f);
			}

			if(enemyType == 't')
			{
				turret * turr = (turret*)enemies[i];
				// hide the projectiles behind the turret
				turr->hideProjectiles();
			}
		}
		numEnemies += size;
	}
}

void entityManager::loadStuff()
{
	std::fstream file(stuffFile.c_str());

	int size = 0;
	// count the number of strings in the text file
	file.peek();
	while(!file.eof())
	{
		int c;
		c = file.get();
		if(c == '\n' || file.eof()) 
		{size++;}
	}

	// clear fstream flags
	file.clear();
	// set fstream get pointer back to the beginning
	file.seekg(0, std::ios::beg);

	// check if there is already an existing array
	if(!m_stuff)
	{
		// if not create a new list
		m_stuff = new stuff * [size];
	}
	else
	{
		// create a new array
		stuff ** tempArray;
		tempArray = new stuff * [numStuff + size];
		// copy over array data
		for(int i = 0; i < numStuff; i++)
		{
			tempArray[i] = m_stuff[i];
		}
		// delete old array and transfer new array into players
		delete [] m_stuff;
		m_stuff = tempArray;
	}

	for(int i = numStuff; i < numStuff + size; i++)
	{
		char stuffType;
		float x, y, data1, data2;

		file >> stuffType >> x >> y;

		if(stuffType == 'a') // load armor
		{
			m_stuff[i] = new armor;
			objMgr->loadObjectsFromTxtFile("defaultArmor.txt");
		}
		else if(stuffType == 'k') // load key
		{
			m_stuff[i] = new key;
			objMgr->loadObjectsFromTxtFile("defaultKey.txt");
		}
		else if(stuffType == 'd') // load door
		{
			m_stuff[i] = new door;
			objMgr->loadObjectsFromTxtFile("defaultDoor.txt");
		}
		else if(stuffType == 't')
		{
			// also extract two more pieces of data for teleporter
			file >> data1 >> data2;

			teleporter * tel = new teleporter;
			m_stuff[i] = tel;
			tel->setData(data1, data2);
			objMgr->loadObjectsFromTxtFile("defaultTeleporter.txt");
		}
		else if(stuffType == 'v')
		{
			m_stuff[i] = new victory();
			objMgr->loadObjectsFromTxtFile("defaultVictory.txt");
		}

		objMgr->indexEnd();
		m_stuff[i]->setObject(objMgr->getObject());
		m_stuff[i]->setPosition(x, y);
		m_stuff[i]->getObject()->setSprite(0,0);
		// toggle collision for some m_stuff
		if(stuffType == 'a' || stuffType == 'k' || stuffType == 't')
		{
			m_stuff[i]->getObject()->toggleCollision();
		}
	}
	numStuff += size;
}

void entityManager::loadCheckPoints()
{
	std::fstream file(checkPointFile.c_str());

	int size = 0;
	// count the number of strings in the text file
	file.peek();
	while(!file.eof())
	{
		int c;
		c = file.get();
		if(c == '\n' || file.eof()) 
		{size++;}
	}

	// clear fstream flags
	file.clear();
	// set fstream get pointer back to the beginning
	file.seekg(0, std::ios::beg);

	// check if there is already an existing array
	if(!m_checkPoints)
	{
		// if not create a new list
		m_checkPoints = new stuff * [size];
	}
	else
	{
		// create a new array
		stuff ** tempArray;
		tempArray = new stuff * [numCheckPoints + size];
		// copy over array data
		for(int i = 0; i < numCheckPoints; i++)
		{
			tempArray[i] = m_checkPoints[i];
		}
		// delete old array and transfer new array into players
		delete [] m_checkPoints;
		m_checkPoints = tempArray;
	}

	for(int i = numCheckPoints; i < numCheckPoints + size; i++)
	{
		float x, y;

		file >> x >> y;

		m_checkPoints[i] = new checkpoint;
		objMgr->loadObjectsFromTxtFile("defaultCheckpoint.txt");

		objMgr->indexEnd();
		m_checkPoints[i]->setObject(objMgr->getObject());
		m_checkPoints[i]->setPosition(x, y);
		m_checkPoints[i]->getObject()->setSprite(0,0);
		// toggle collision for some m_stuff
		m_checkPoints[i]->getObject()->toggleCollision();
	}
	numCheckPoints += size;
}

// remove entity
void entityManager::removeEnemies()
{

	// delete all enemie entities
	if(enemies)
	{
		for(int i = 0; i < numEnemies; i++)
		{
			// check if it is a turret, turrets have more objects in thier projectiles
			if(enemies[i]->getType() == entityTurret)
			{
				turret *  turr = (turret*)enemies[i];
				// must remove all projectile objects first
				for(int i = 0; i < turr->getNumProjectiles(); i++)
				{
					projectile * proj = turr->getProjectile(i);
					objMgr->popObject(proj->getObject()->getObjectIndex());
					// udpate object indexes
					objMgr->getList()->setObjectIndexes();
				}
				turr->destroyProjectiles();
			}
			// tell object manager to pop that particular
			objMgr->popObject(enemies[i]->getObject()->getObjectIndex());
			// udpate object indexes
			objMgr->getList()->setObjectIndexes();
			enemies[i]->setObject(NULL);
			delete enemies[i];
			enemies[i] = NULL;
		}
		delete [] enemies;
		enemies = NULL;
		// contract object list to prevent it from growing out of control
		objMgr->getList()->contractList();
	}
	// reset count data
	numEnemies = 0;
}
void entityManager::removePlayers()
{
	// delete all player entities
	if(players)
	{
		for(int i = 0; i < numPlayers; i++)
		{
			// tell object manager to pop that particular
			objMgr->popObject(players[i]->getObject()->getObjectIndex());
			// udpate object indexes
			objMgr->getList()->setObjectIndexes();
			players[i]->setObject(NULL);
			delete players[i];
			players[i] = NULL;
		}
		delete [] players;
		players = NULL;
		// contract object list to prevent it from growing out of control
		objMgr->getList()->contractList();
	}
	// reset count data
	numPlayers = 0;
}

void entityManager::removeStuff()
{
	// delete all player entities
	if(m_stuff)
	{
		for(int i = 0; i < numStuff; i++)
		{
			// tell object manager to pop that particular
			objMgr->popObject(m_stuff[i]->getObject()->getObjectIndex());
			// udpate object indexes
			objMgr->getList()->setObjectIndexes();
			m_stuff[i]->setObject(NULL);
			delete m_stuff[i];
			m_stuff[i] = NULL;
		}
		delete [] m_stuff;
		m_stuff = NULL;
		// contract object list to prevent it from growing out of control
		objMgr->getList()->contractList();
	}
	// reset count data
	numStuff = 0;
}
void entityManager::removeCheckPoints()
{
	// delete all player entities
	if(m_checkPoints)
	{
		for(int i = 0; i < numCheckPoints; i++)
		{
			// tell object manager to pop that particular
			objMgr->popObject(m_checkPoints[i]->getObject()->getObjectIndex());
			// udpate object indexes
			objMgr->getList()->setObjectIndexes();
			m_checkPoints[i]->setObject(NULL);
			delete m_checkPoints[i];
			m_checkPoints[i] = NULL;
		}
		delete [] m_checkPoints;
		m_checkPoints = NULL;
		// contract object list to prevent it from growing out of control
		objMgr->getList()->contractList();
	}
	// reset count data
	numCheckPoints = 0;
}
void entityManager::removeAll()
{
	// remove EVERYTHING
	removeEnemies();
	removePlayers();
	removeStuff();
	removeCheckPoints();
}
void entityManager::removeAllExceptCheckPoints()
{
	// remove EVERYTHING except for checkpoints
	removeEnemies();
	removePlayers();
	removeStuff();
}

void entityManager::resetPlayers()
{
	for(int i = 0; i < numPlayers; i++)
	{
		players[i]->moveToDefaultPos();
		players[i]->reset();
	}
}

void entityManager::removeFromStuff(int index)
{
	if(m_stuff)
	{
		// move stuff to the end of the list
		for(int i = index; i < numStuff-1; i++)
		{
			stuff * temp = m_stuff[i];
			m_stuff[i] = m_stuff[i+1];
			m_stuff[i+1] = temp;
		}

		// after its at the end of the list, destroy it
		objMgr->popObject(m_stuff[numStuff-1]->getObject()->getObjectIndex());
		m_stuff[numStuff-1]->setObject(NULL);
		// contract object list to prevent it from growing out of control
		objMgr->getList()->contractList();
	}
	// decriment stuff by 1
	numStuff -= 1;
}

void entityManager::checkOldCheckpoints()
{
	for(int i = 0; i < checkPnt; i++)
	{
		if(!m_checkPoints[i]->isPickedUp())
		{
			m_checkPoints[i]->pickUp();
		}
	}
}

//accesors
entity * entityManager::getEnemy(int index)
{
	if(index >= 0 && index < numEnemies)
		return enemies[index];
	else return NULL;
}
player * entityManager::getPlayer(int index)
{
	if(index >= 0 && index < numPlayers)
		return (player*)players[index];
	else return NULL;
}
stuff * entityManager::getStuff(int index)
{
	if(index >= 0 && index < numStuff)
		return (stuff*)m_stuff[index];
	else return NULL;
}
int entityManager::getCheckPoint(){return checkPnt;}
bool entityManager::getVictoryCondition(){return victoryCondition;}
void entityManager::initPlayerSound()
{
	for(int i = 0; i < numPlayers; i++)
	{
		player * plr = (player*)players[i];
		plr->setSound(dxSound);
	}
}