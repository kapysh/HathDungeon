#include <iostream>
#include <vector>
#include <random>
#include <ctime>

#include "DungeonLevel.h"
#include "XMLSerializable.h"
#include "Player.h"
#include "Creature.h"
#include "Entity.h"
#include "Tile.h"
#include "Factory.h"
#include "Item.h"

using namespace std;

std::mt19937 mt (time(NULL));

DungeonLevel::~DungeonLevel()
{
    for(int i = 0; i < vSectors.size(); i++)
    {
        //deleting all dynamically created vector elements
        delete vSectors.back();
        vSectors.pop_back();
    }
}

DungeonLevel::DungeonLevel(int iWidth, int iHeight)
{ 
    iStartRow = 0;	//because (0,0) should be a wall tile
    iEndRow = iHeight - 1;	//should be 19
    iStartColumn = 0;
    iEndColumn = iWidth - 1;	//should be 78
    iNumTiles = 0;
    //requirements
    iMinLength = 9; //includes wall tiles and invisible boundary
    iMinTiles = 400; 
	iMinRooms = 5; 
    cHoriWallTile = '-'; //horizontal wall tile
    cVertiWallTile = '|'; //vertical wall tile
    cVacantTile = ' '; //vacant tile
    cRoomTile = '.';
	cTunnelTile = '#'; 
	cUpTile = '<';
	cDownTile = '>';   
    t_UpStairs = NULL; //so that spawnelements knows if the level is built

    vvTiles.resize(iHeight);

    // And then resize the inner vectors.
    for(auto it = vvTiles.begin(); it != vvTiles.end(); it++ )
    {
        (*it).resize(iWidth);
    }

	for(int y = iStartRow; y <= iEndRow; y++)
		for(int x = iStartColumn; x <= iEndColumn; x++)
			vvTiles[y][x] = new Tile(x, y);
}

//Creates all rooms in DungeonLevel
void DungeonLevel::BuildRooms() 
{
    int iRandY; //holds random number value for follwing calculations
    int iRandX; //holds random number value for follwing calculations
    
    //for 4 rooms, create a new DL object,
    //load the starting points into the object,
    //pass the objects address to SpawnSector

	//stops when minimum tiles is reached AND there are more than 4 rooms spawned
	//	to ensure that one room touches each dungeon wall
	for(int i = 1; ((iNumTiles <= iMinTiles)||(i <= iMinRooms)); i++)
    {
        DungeonLevel* dlSector = new DungeonLevel(0,0);
        do
        {
            iRandY = (iMinLength + (mt() % 6));		
            iRandX = (iMinLength + (mt() % 6));
    		if(i == 1)		//this is bottom wall condition
        	{
                (dlSector->iEndRow) = iEndRow;
        	    (dlSector->iStartRow) = (iEndRow - iRandY); 
					//this long line is so that top/bottom walls don't spawn too close to right/left walls
				(dlSector->iStartColumn) = (iMinLength + (mt() % (iEndColumn - iRandX - (2 * iMinLength))));
      			(dlSector->iEndColumn) = ((dlSector->iStartColumn) + iRandX);
			}
			else if(i == 2)	//this is right wall condition
        	{
                (dlSector->iEndColumn) = iEndColumn;
                (dlSector->iStartColumn) = (iEndColumn - iRandX);
                (dlSector->iStartRow) = (mt() % (iEndRow - iRandY)); 
                (dlSector->iEndRow) = ((dlSector->iStartRow) + iRandY); 
        	}
        	else if(i == 3)	//this is top wall condition
        	{
                (dlSector->iStartRow) = iStartRow;
                (dlSector->iEndRow) = iRandY;	
				(dlSector->iStartColumn) = (iMinLength + (mt() % (iEndColumn - iRandX - (2 * iMinLength))));
				(dlSector->iEndColumn) = (dlSector->iStartColumn) + iRandX;
			}
       		else if(i == 4)	//this is left wall condition
        	{
                (dlSector->iStartColumn) = iStartColumn;
                (dlSector->iEndColumn) = iRandX; 
                (dlSector->iStartRow) = (mt() % (iEndRow - iRandY));  
                (dlSector->iEndRow) = ((dlSector->iStartRow) + iRandY); 
        	}
            else			//this is the rest of the rooms
			{
            	(dlSector->iStartRow) = (mt() % (iEndRow - iRandY));
            	(dlSector->iEndRow) = ((dlSector->iStartRow) + iRandY);
				(dlSector->iStartColumn) = (mt() % (iEndColumn - iRandX));
            	(dlSector->iEndColumn) = ((dlSector->iStartColumn) + iRandX);
            }
        }while(!SpawnSector(dlSector));
		vSectors.push_back(dlSector); //found a good room, saving it in vector
    }

	SortSectors(); 	//sorting vectors
	DrawTunnels(); //drawing tunnels
	SpawnElements(); //spawning elements
}
     
//receives an object loaded with a start/end column/row
//checks to make sure sector is vacant
//if not vacant, returns false
//if vacant, first shrinks all "non-dungeon-wall" walls by one tile
//then populates map with rectangle of dots within sector bounds 
//then surrounds room with '-' and '|' for walls
bool DungeonLevel::SpawnSector(DungeonLevel* dlSector)
{
    //first checking to make sure sector is good
    for(int itRow = (dlSector->iStartRow);
        itRow <= (dlSector->iEndRow);
        itRow++)
    {
        for(int itCol = (dlSector->iStartColumn);
        itCol <= (dlSector->iEndColumn);
        itCol++)
        {	//if tile is not vacant, return false
            if(vvTiles[itRow][itCol]->Get() != cVacantTile)
                return false;
        }
    }

    //if wall doesn't lie on a dungeon wall, shrink it to erase "boundary"
    if((dlSector->iStartRow) != iStartRow)
        (dlSector->iStartRow)++;
    if((dlSector->iEndRow) != iEndRow)
        (dlSector->iEndRow)--;
    if((dlSector->iStartColumn) != iStartColumn)
        (dlSector->iStartColumn)++;
    if((dlSector->iEndColumn) != iEndColumn)
        (dlSector->iEndColumn)--;
    
    //loop for rows
    for(int itRow = (dlSector->iStartRow);
        itRow <= (dlSector->iEndRow);
        itRow++)
    {
        //loop for columns
        for(int itCol = (dlSector->iStartColumn);
        	itCol <= (dlSector->iEndColumn);
        	itCol++)
        {
           	vvTiles[itRow][itCol]->Set(cRoomTile); //first drawing room tile 
    
            //overwriting room tile if it's a wall
            if((itRow == (dlSector->iStartRow))||(itRow == (dlSector->iEndRow)))    
               	vvTiles[itRow][itCol]->Set(cHoriWallTile);
            if((itCol == (dlSector->iStartColumn))||(itCol == (dlSector->iEndColumn)))
                vvTiles[itRow][itCol]->Set(cVertiWallTile);
			//Display(); //i just think adding this looks cool
        }
    }
    //incrementing global tile count with number of room tiles in sector
   	SetTiles(iNumTiles += (dlSector->GetTiles()));
    return true;
}

//walks thru dungeon from top left to bottom right
//if it "steps" on a room tile, it identifies the room, and places it into a "sorted" rooms vector
void DungeonLevel::SortSectors()
{
	vector<DungeonLevel*> vSortedSects; //temporary vector to store sorted sectors
	int iSec;

	//starting with left Dungeon column
	for(int itCol = iStartColumn; itCol <= iEndColumn; itCol++)
	{
		//starting with top Dungeon row
		for(int itRow = iStartRow; itRow <= iEndRow; itRow++)
		{	
			//if you see a non-vacant Tile
			if(vvTiles[itRow][itCol]->Get() != cVacantTile)
			{
				//if we cant find the tile in our SortedSects vector
				if(FindSector(itRow, itCol, vSortedSects) < 0)
				{
					//then call findsector on vSectors
					iSec = FindSector(itRow, itCol, vSectors);
					//push the index returned to vSorted
					vSortedSects.push_back(vSectors[iSec]);
					//and erase the indexed object from vSectors to speed up further operations
					vSectors.erase(vSectors.begin() + iSec);
				}
			}
		}
	}
	
	//pushing the sorted sectors back into the original vector
	for(int itSec = 0; (itSec < vSortedSects.size()); itSec++)
	{
		vSectors.push_back(vSortedSects[itSec]);
	}
}

//will tunnel first horizontally, then vertically, from start point 
//to target point - all while dodging obstacles 
void DungeonLevel::DrawTunnels()
{
	int iMidY;	//holds random row inside current room
	int iMidX;	//holds random column inside current room
	int iTargMidY;	//holds random row inside target room
	int iTargMidX;	//holds random column inside target room
	int iMoveCount;	//just a 'moves' counter for use in method
	int xClean;	//to use during horizontal tunnelling

	//for each sector in Sorted
	for(int itSec = 0; (itSec < vSectors.size()); itSec++)
	{
		//initializing iMoveCount to 0
		iMoveCount = 0;	
	
		if(itSec < (vSectors.size()-1))
		{
			//getting a random position in current room
			RandomPos(iMidX, iMidY, vSectors[itSec]);

			//here are the target coordinates
			//do loop is to try and avoid running into wall tiles while tunnelling
			do{
            	RandomPos(iTargMidX, iTargMidY, vSectors[itSec + 1]);
			}while(iTargMidX == (vSectors[itSec]->iEndColumn));

			//drawing horizontal tunnel to next room
            //and the current room is never a column ahead of the next room
            for(int x = iMidX; x <= iTargMidX; x++)
            {
				//if it's not a room tile
                if(vvTiles[iMidY][x]->Get() != cRoomTile)
				{
					//if we are at a corner
					if((vvTiles[iMidY][x]->Get() == cVertiWallTile)&&
						(vvTiles[iMidY][x+1]->Get() == cHoriWallTile))
					{
						//moving the row up 1 -- doesn't matter if we move up or down
						//will effectively avoid mass horizontal wall tunnelling
						iMidY++;
						vvTiles[iMidY][x-1]->Set(cTunnelTile); 
					}
		
					//if we are on the last room column
					if(x == iTargMidX)
					{
						//if we are on a vertical wall
						if(vvTiles[iMidY][x]->Get() == cVertiWallTile)
						{
							//"cleaning" the coordinate by moving back a column
							//so i don't have to write an extra condition in vertical tunnelling
							xClean = (x - 1);
						}
					}
					//else we are not on the last room col, nothing to check
					else
					{
 						xClean = x;					
					}
					//setting tunnel tile
					vvTiles[iMidY][xClean]->Set(cTunnelTile);
				}
            }

			//drawing vertical lower tunnels to next room
			//if next room is lower than current room, get there
			if(iTargMidY > iMidY)
			{	//moving from current random column to target random column
				for(int y = iMidY; y <= iTargMidY; y++)
				{
					//if it's not a room tile
					if(vvTiles[y][iTargMidX]->Get() != cRoomTile)
					{
						//if we are on a vertical room wall
						if(vvTiles[y][iTargMidX]->Get() == cVertiWallTile)
						{
							iTargMidX--; //moving back one
							iMoveCount++; //incrementing moveCount
						}
						
						//if we have moved
						if(iMoveCount != 0)
						{
							//if we are at a corner, wrap back around so we try
							//not to hit any more wall tiles on upper rooms
							if((vvTiles[y-1][iTargMidX+1]->Get() == cVertiWallTile)&&
								(vvTiles[y-1][iTargMidX+2]->Get() == cHoriWallTile))
							{
								vvTiles[y][iTargMidX]->Set(cTunnelTile); //cornering
								iTargMidX++;
								iMoveCount--;
							}
							//if we are at the end and still hugging the wall
							else if(y == iTargMidY)
							{
								vvTiles[y][iTargMidX]->Set(cTunnelTile); //cornering
								iTargMidX++;
								iMoveCount--;
							}
						}
						//setting a tunnel tile
						vvTiles[y][iTargMidX]->Set(cTunnelTile);
					}
				}
			}

			//else, next room is higher than current room
			else
			{	//moving from current random row to target random row
				for(int y = iMidY; y >= iTargMidY; y--)
				{
                    //if it's not a room tile
                    if(vvTiles[y][iTargMidX]->Get() != cRoomTile)
                    {
                        //if we are on a vertical room wall
                        if(vvTiles[y][iTargMidX]->Get() == cVertiWallTile)
                        {
                            iTargMidX--; //moving back one
                            iMoveCount++; //incrementing moveCount
                        }

                        //if we have moved
                        if(iMoveCount != 0)
                        {
                            //if we are at a corner, wrap back around so we try
                            //not to hit any more wall tiles on upper rooms
                            if((vvTiles[y+1][iTargMidX+1]->Get() == cVertiWallTile)&&
                                (vvTiles[y+1][iTargMidX+2]->Get() == cHoriWallTile))
                            {
                                vvTiles[y][iTargMidX]->Set(cTunnelTile); //cornering
                                iTargMidX++; 
                                iMoveCount--;
                            }
                            //if we are at the end and still hugging the wall
                            else if(y == iTargMidY)
                            {
                                vvTiles[y][iTargMidX]->Set(cTunnelTile); //creating corner
                                iTargMidX++; //moving forward into room
                                iMoveCount--;
                            }
                        }
						//setting a tunnel tile
						vvTiles[y][iTargMidX]->Set(cTunnelTile);
					}
				}
			}
		}
		
		//loop to overwrite holes tunneled into left/right walls with room tiles
		//note - i do these steps after tunnel placement so that the tunnel positions become
		//more interesting as number of rooms increases
		for(int y = (vSectors[itSec]->iStartRow);
			y <= (vSectors[itSec]->iEndRow);
			 y++)
		{
			if(vvTiles[y][(vSectors[itSec]->iStartColumn)]->Get() == cTunnelTile)
				vvTiles[y][(vSectors[itSec]->iStartColumn)]->Set(cRoomTile);
			if(vvTiles[y][(vSectors[itSec]->iEndColumn)]->Get() == cTunnelTile)
				vvTiles[y][(vSectors[itSec]->iEndColumn)]->Set(cRoomTile);
		}

//		//loop to overwrite holes tunneled into top/bottom walls with room tiles
		for(int x = (vSectors[itSec]->iStartColumn);
			x <= (vSectors[itSec]->iEndColumn);
			x++)
		{
			if(vvTiles[(vSectors[itSec]->iStartRow)][x]->Get() == cTunnelTile)
				vvTiles[(vSectors[itSec]->iStartRow)][x]->Set(cRoomTile);
			if(vvTiles[vSectors[itSec]->iEndRow][x]->Get() == cTunnelTile)
				vvTiles[vSectors[itSec]->iEndRow][x]->Set(cRoomTile);
		}
	}
}

//uses randomPos to find a random position in the first room drop an upstairs tile
//same for last room to drop a downstairs tile
void DungeonLevel::SpawnElements()
{
	int iCol;
	int iRow;
	
	//if spawnelements is being called on an existing dungeonlevel, just spawn one creature
	if(GetUpStairs() != NULL)
	{
        do{
            RandomPos(iCol, iRow, this);
        }while(!(vvTiles[iRow][iCol]->IsGoodE()));
        Creature* c_Creature = Factory::Instance().GenerateCreature(10);

        if(c_Creature)
        {
            vCreatures.push_back(c_Creature);
            c_Creature->SetTile(vvTiles[iRow][iCol]);
        }
		
		return;
	}
	else;
	
	//spawning upstairs
	do		//do loop just ensures that spawned stairways do not block a tunnel entrance
	{
		RandomPos(iCol, iRow, vSectors[0]);
	}while(!(vvTiles[iRow+1][iCol+1]->IsGoodE())||
			!(vvTiles[iRow-1][iCol+1]->IsGoodE())||
			!(vvTiles[iRow-1][iCol-1]->IsGoodE())||
			!(vvTiles[iRow+1][iCol-1]->IsGoodE()));
	t_UpStairs = vvTiles[iRow][iCol];
	vvTiles[iRow][iCol]->Set(cUpTile);
	//spawning downstairs
	do
	{
		RandomPos(iCol, iRow, vSectors[vSectors.size()-1]);
    }while(!(vvTiles[iRow+1][iCol+1]->IsGoodE())||
            !(vvTiles[iRow-1][iCol+1]->IsGoodE())||
            !(vvTiles[iRow-1][iCol-1]->IsGoodE())||
            !(vvTiles[iRow+1][iCol-1]->IsGoodE()));
	t_DownStairs = vvTiles[iRow][iCol];
	vvTiles[iRow][iCol]->Set(cDownTile);
	//spawning items
	for(int i = 0; i < (20 + mt() % 5); i ++)
	{	
		do{
			RandomPos(iCol, iRow, this);
		}while(!(vvTiles[iRow][iCol]->IsGoodE()));
		Item* i_Item = Factory::Instance().GenerateItem();
		
		if(i_Item)
		{
			vvTiles[iRow][iCol]->AddItem(i_Item);
		}	
	}
	//spawning monsters
	for(int i = 0; i < (4 + mt() % 5); i++)
	{
		do{
			RandomPos(iCol, iRow, this);
		}while(!(vvTiles[iRow][iCol]->IsGoodE()));
		Creature* c_Creature = Factory::Instance().GenerateCreature(10);
		
		if(c_Creature)
		{
			vCreatures.push_back(c_Creature);
			c_Creature->SetTile(vvTiles[iRow][iCol]);
		}
	}
}

//receives a column and a row integer by reference and a dlSector
//finds a random point in the given sector and loads the coordinates into iCol and iRow
void DungeonLevel::RandomPos(int & iCol, int & iRow, DungeonLevel* dlSector)
{
	int iStartY = (dlSector->iStartRow) + 1;
	int	iStartX = (dlSector->iStartColumn) + 1;
	
    int iEndY = (dlSector->iEndRow) - 1;
    int iEndX = (dlSector->iEndColumn) - 1;
			
	//random target inside target room - just to randomize tunnel pattern
	iCol = (iEndX - (mt() % (iEndX - iStartX)));
	iRow = (iEndY - (mt() % (iEndY - iStartY))); 
}

//receives a vector of dlSectors and a row/col coordinate pair
//just iterates thru the vector of dlSectors and returns the vector index of the sector where point was found
int DungeonLevel::FindSector(int iRow, int iCol, vector<DungeonLevel*> vTestSects)
{
	//for each sector in vector
	for(int itSec = 0;
		(itSec < vTestSects.size());
		itSec ++)
	{
		//for each row in sector
		for(int itSecRow = (vTestSects[itSec]->iStartRow);
    		((itSecRow <= (vTestSects[itSec]->iEndRow)));
       		itSecRow++)
		{
    		//if the row matches
        	if(iRow == itSecRow)
        	{
        		//check thru columns until match is found
        		for(int itSecCol = (vTestSects[itSec]->iStartColumn);
            		((itSecCol <= (vTestSects[itSec]->iEndColumn)));
                	itSecCol++)
            	{
            		//if the column matches we have matched a tile with a sector
                	if(iCol == itSecCol)
						return itSec;
				}
			}
		}
	}
	return (-1);	//this is to simplify result detection
}

int DungeonLevel::GetTiles()
{
    return((iEndRow-iStartRow)*(iEndColumn-iStartColumn));
}

void DungeonLevel::SetTiles(int i)
{         
	iNumTiles = i;
}

int DungeonLevel::GetHeight()
{
	return(vvTiles.size());
}

int DungeonLevel::GetWidth(int iRow)
{
	return(vvTiles[iRow].size());
}

char DungeonLevel::Get(int iRow, int iCol)
{
	return(vvTiles[iRow][iCol]->Get());
}

void DungeonLevel::Set(int iRow, int iCol, char cAvatar)
{
	vvTiles[iRow][iCol]->Set(cAvatar);
}

vector<DungeonLevel*> DungeonLevel::GetVSectors()
{
	return(vSectors);
}

int DungeonLevel::GetStartRow()
{
	return(iStartRow);
}

int DungeonLevel::GetStartColumn()
{
	return(iStartColumn);
}

int DungeonLevel::GetEndRow()
{
	return(iEndRow);
}

int DungeonLevel::GetEndColumn()
{
	return(iEndColumn);
}

Tile* DungeonLevel::GetUpStairs()
{
	return t_UpStairs;
}

Tile* DungeonLevel::GetDownStairs()
{
	return t_DownStairs;
}

// Dump the dungeon level to console
void DungeonLevel::Display()
{
    // Iterate the outer vector
    for(auto it = vvTiles.begin(); it != vvTiles.end(); it++ )
    {
        // Iterate the inner vector
        for( auto itInner = (*it).begin();
            itInner != (*it).end();
            itInner++ )
        {
            cout << ((*itInner)->Get());
        }
		
        // End of line as we just output a row
        cout << endl;
    }
}

//will return a 2d array of characters with only static tile chars, for use with ncurses
vector<vector<char>> DungeonLevel::Dump()
{
	vector<vector<char>> vvChars; //2d vector of chars

	vvChars.resize(iEndRow+1); //resizing height

	for(auto it = vvChars.begin(); it != vvChars.end(); it++)
		(*it).resize(iEndColumn+1); //resizing width

	for(int y = iStartRow; y <= iEndRow; y++) //iterating thru y axis
		for(int x = iStartColumn; x <= iEndColumn; x++) //thru x axis
		{
			vvChars[y][x] = vvTiles[y][x]->Get(); //setting the chars
		}
	return(vvChars);
}

//will return a tile object
Tile* DungeonLevel::GetTileObj(int iRow, int iCol)
{
	return(vvTiles[iRow][iCol]);
}

vector<Creature*> DungeonLevel::GetVCreatures()
{
	return(vCreatures);
}

void DungeonLevel::KillCreature(Creature* c_Creature2)
{
	(c_Creature2->GetTile())->SetActor(NULL);

	for(int i = 0; i < vCreatures.size(); i++)
	{
		if(vCreatures[i] == c_Creature2)
			vCreatures.erase(vCreatures.begin() + i);
	}
}

void DungeonLevel::SetLevel(int iLevel2)
{
	iLevel = iLevel2;
}

int DungeonLevel::GetLevel()
{
	return iLevel;
}
