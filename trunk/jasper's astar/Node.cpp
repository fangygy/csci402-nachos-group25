#include "Node.h"
#include <cmath>
#include <iostream>

using namespace std;

Node::Node() {
	heuristic = 0;
	fn = 0;
	distance = 0;
	discontinue = false;
}

Node::Node(Grid g) {
	pathTakenGrid.push_back(g);
	calcHeuristic();
	fn = 0;
	distance = 0;
	discontinue = false;
}

Node::Node(Node* pNode, int moveRow, int moveCol, int zRow, int zCol) {
	discontinue = false;
	moved = pNode->moved;
	pathTakenGrid = pNode->pathTakenGrid;
	Grid tempGrid = pathTakenGrid[pathTakenGrid.size()-1];
	
	int moveNode = tempGrid.grid[moveCol][moveRow];
	tempGrid.grid[moveCol][moveRow] = 0;
	tempGrid.grid[zCol][zRow] = moveNode;
	
	if (!alreadyExists(tempGrid)) {
		pathTakenGrid.push_back(tempGrid);
		moved.push_back(moveNode);
		calcHeuristic();
		fn = 0;
		distance = 0;
		distance = pNode->distance;
		distance++;
		//fn = distance + heuristic;
		cout << fn << " = " << distance << "/" << moved.size() <<  " + " << heuristic << endl;
	}
	else {
		//cout << "Discontinue" << endl;
		heuristic = pNode->heuristic;
		bool discontinue = true;
	}

}

Node::~Node() {
		pathTakenGrid.clear();
}

bool Node::alreadyExists(Grid g) {
	
	for(int i = 0; i < pathTakenGrid.size(); i++) {
		
		bool sameGrid = true;
		for(int row = 0; sameGrid && row < 4; row++) {
			for(int col = 0; col < 4; col++) {
				
				if( pathTakenGrid[i].grid[col][row] != g.grid[col][row] ) {
					sameGrid = false;
					break;
				}
			}
		}
		
		if(sameGrid) {
			return true;
		}
	}
	return false;

}

void Node::calcHeuristic() {
	heuristic = 0;
	Grid tempGrid = pathTakenGrid[pathTakenGrid.size()-1];
	
	for (int row = 0; row < 4; row++) {
		for (int col = 0; col < 4; col++) {
		
			int num = tempGrid.grid[col][row];
			
			switch(num) {
				case 1:
					heuristic += abs(col-0) + abs(row-0);
					break;
				case 2:
					heuristic += abs(col-1) + abs(row-0);
					break;
				case 3:
					heuristic += abs(col-2) + abs(row-0);
					break;
				case 4:
					heuristic += abs(col-3) + abs(row-0);
					break;
				case 5:
					heuristic += abs(col-0) + abs(row-1);
					break;
				case 6:
					heuristic += abs(col-1) + abs(row-1);
					break;
				case 7:
					heuristic += abs(col-2) + abs(row-1);
					break;
				case 8:
					heuristic += abs(col-3) + abs(row-1);
					break;
				case 9:
					heuristic += abs(col-0) + abs(row-2);
					break;
				case 10:
					heuristic += abs(col-1) + abs(row-2);
					break;
				case 11:
					heuristic += abs(col-2) + abs(row-2);
					break;
				case 12:
					heuristic += abs(col-3) + abs(row-2);
					break;
				case 13:
					heuristic += abs(col-0) + abs(row-3);
					break;
				case 14:
					heuristic += abs(col-1) + abs(row-3);
					break;
				case 15:
					heuristic += abs(col-2) + abs(row-3);
					break;
				default:
					break;
			}
		}
	}
}
