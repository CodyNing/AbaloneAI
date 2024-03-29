#include "logic.h"

bool isWhitePiece(int index, std::bitset<128U>& state) {
	return state[index << 1];
}

bool isBlackPiece(int index, std::bitset<128U>& state) {
	return state[(index << 1) + 1];
}

std::bitset<128U>& logic::sideMove(std::bitset<128U>& state, action& act, bool isBlackTurn) {
	int marbleCount = act.count;
	int index = act.index;
	int direction = act.direction;

	//counter clockwise first
	int drr1 = (direction + 5) % 6;

	//counter clockwise second
	int drr2 = (direction + 4) % 6;
	int drr = drr1;

	int next = -1;
	for (int i = 0; i < marbleCount; ++i) {

		if (index == -1)
			return state;

		next = MOVE_TABLE[index][direction];
		
		auto indexState = state[index << 1] << 1 | state[(index << 1) + 1] << 0;

		//index slot is empty, maybe we can change direction
		if (!indexState) {
			//try to change direction
			if (i == 1) {
				drr = drr2;
				next = index;
				int drrOpp = (direction + 3) % 6;
				index = MOVE_TABLE[index][drrOpp];
				//the next direction out bound, nothing to move, both direction failed
				if (index == -1)
					return state;
				indexState = state[index << 1] << 1 | state[(index << 1) + 1] << 0;;
				//next direction is also emppty or different color, both direction failed
				if (!indexState || (indexState & 1 ^ isBlackTurn << 0))
					return state;
				//same color
				else {
					index = MOVE_TABLE[index][drr];
					
					continue;
				}
			}
			return state;
		}
		//different color
		if (indexState & 1 ^ isBlackTurn << 0)
			return state;

		//not allow suicide
		if (next == -1)
			return state;

		//not empty
		if (state[next << 1] || state[(next << 1) + 1])
			return state;

		index = MOVE_TABLE[index][drr];
	}

	index = act.index;
	next = MOVE_TABLE[index][direction];
	for (int i = 0; i < marbleCount; ++i) {
		state.set(index << 1, 0);
		state.set((index << 1) + 1, 0);
		state.set(next << 1, !isBlackTurn);
		state.set((next << 1) + 1, isBlackTurn);
		index = MOVE_TABLE[index][drr];
		next = MOVE_TABLE[index][direction];
	}
	return state;
}

std::bitset<128U>& logic::inlineMove(std::bitset<128U>& state, action& act, bool isBlackTurn) {
	int marbleCount = act.count;
	int index = act.index;
	int direction = act.direction;

	int sameColorCount = 0;
	int endIndex = 0;
	for (int next = index; sameColorCount < 4; ++sameColorCount, next = MOVE_TABLE[next][direction]) {
		//suicide case, directly return to disable suicide
		if (next == -1) {
			/*int resetI = 127 - (index << 1);
			state.set(resetI, 0);
			state.set(resetI - 1, 0);*/
			return state;
		}
		auto nextState = state[next << 1] << 1 | state[(next << 1) + 1] << 0;
		//empty maybe (valid)
		if (!nextState) {
			//not moving anything(invalid)
			if (sameColorCount == 0)
				return state;
			//create and return new state
			/*std::bitset<128U> mask{ 0 };
			std::bitset<128U> setMask{ 0 };
			for (int c = 0, mNext = index, nNext = MOVE_TABLE[mNext][direction]; c < sameColorCount; ++c, mNext = nNext, nNext = MOVE_TABLE[nNext][direction]) {
				std::bitset<128U> subMask{ 3 };
				int patternI = 127 - (index << 1);
				auto p = state[127 - patternI] << 1 | state[patternI - 1] << 0;
				std::bitset<128U> pattern(p);
				mask |= subMask << (125 - (mNext << 1));
				setMask |= pattern << (125 - (nNext << 1));
			}
			state &= ~mask;
			state |= setMask;
			return state;*/
			state.set(index << 1, 0);
			state.set((index << 1) + 1, 0);
			state.set(next << 1, !isBlackTurn);
			state.set((next << 1) + 1, isBlackTurn);
			return state;
		}
		//different color(check further)
		if (nextState & 1 ^ isBlackTurn << 0) {
			//different color but can't push(invalid)
			if (sameColorCount < 2) {
				return state;
			}
			break;
		}
		endIndex = next;
	}
	//no marble or more than 3 marble (invalid)
	if (!sameColorCount || sameColorCount > 3)
		return state;

	int oppIndex = MOVE_TABLE[endIndex][direction];
	int oppColorCount = 0;
	int oppEndIndex = 0;
	//The only case we enter this loop is that we met a different color
	for (int next = oppIndex; oppColorCount < sameColorCount; ++oppColorCount, next = MOVE_TABLE[next][direction]) {
		//KILL THE OPPONENT
		if (next == -1) {
			state.set(index << 1, 0);
			state.set((index << 1) + 1, 0);
			state.set(oppIndex << 1, !isBlackTurn);
			state.set((oppIndex << 1) + 1, isBlackTurn);

			//++score
			auto scoreI = (isBlackTurn << 0) * 3 + 122;
			auto max = scoreI + 3;
			bool val = state[scoreI];
			bool carry = val & 1;
			state.set(scoreI, val ^ 1);
			while (carry && scoreI < max) {
				val = state[++scoreI];
				state.set(scoreI, val ^ carry);
				carry &= val;
			}

			return state;
		}
		auto nextState = state[next << 1] << 1 | state[(next << 1) + 1] << 0;
		//empty(valid)
		if (!nextState) {
			//create and return new state
			state.set(next << 1, isBlackTurn);
			state.set((next << 1) + 1, !isBlackTurn);
			state.set(index << 1, 0);
			state.set((index << 1) + 1, 0);
			state.set(oppIndex << 1, !isBlackTurn);
			state.set((oppIndex << 1) + 1, isBlackTurn);
			return state;
		}
		//meet same color again(invalid)
		if ((nextState & 2) >> 1 ^ isBlackTurn << 0) {
			return state;
		}
		oppEndIndex = next;
	}
	//opponent has more, can't push(invalid)
	return state;

}

std::bitset<128U> logic::move(std::bitset<128U> state, action act, bool isBlackTurn) {
	return act.count == 1? inlineMove(state, act, isBlackTurn) : sideMove(state, act, isBlackTurn);
}

std::vector<std::pair<logic::action, std::bitset<128U>>> logic::getAllValidMove(std::bitset<128U>& state, bool isBlackTurn) {
	std::vector<std::pair<logic::action, std::bitset<128U>>> action_state{};

	//count 1-3, index 0-60, direction 0-5
	for (auto bitindex = isBlackTurn << 0; bitindex < 122; bitindex += 2) {
		if (state[bitindex]) {
			for (int direction = 0; direction < 6; ++direction) {
				for (int count = 1; count < 4; ++count) {
					action act{ count, (bitindex - (isBlackTurn << 0)) >> 1, direction };
					auto nstate = move(state, act, isBlackTurn);
					if (nstate != state) {
						action_state.push_back({ act, nstate });
					}
				}
			}
		}
	}
	return action_state;
}

std::multiset<logic::weightedActionState, std::greater<logic::weightedActionState>> logic::getAllValidMoveOrdered(std::bitset<128U>& state, bool isBlackTurn) {
	
	std::multiset<logic::weightedActionState, std::greater<logic::weightedActionState>> actionStates;
	int counter = 0;
	//count 1-3, index 0-60, direction 0-5
	for (auto bitindex = (int)isBlackTurn; bitindex < 122; bitindex += 2) {
		if (state[bitindex]) {
			for (int direction = 0; direction < 6; ++direction) {
				for (int count = 1; count < 4; ++count) {
					weightedAction act{ {count, bitindex >> 1, direction}, 0, 0, -1 };
					if (isValidMove_ai(state, act, isBlackTurn)) {
						++counter;
						actionStates.emplace(act, move_ai(state, act, isBlackTurn));
					}
				}
			}
		}
	}
	return actionStates;
}

bool logic::isValidSideMove(std::bitset<128U>& state, weightedAction & act, bool isBlackTurn)
{
	act.teamCount = act.act.count;
	//check 1: path is blocked
	int next = MOVE_TABLE[act.act.index][act.act.direction];
	if (next == -1 || state[next << 1] || state[(next << 1) + 1] ) {
		return false;
	}
	//check 2: aligning direction
	//assume its aligning on counter clockwise -1 direction
	int alignDrr = (act.act.direction + 5) % 6;
	int alignNext = MOVE_TABLE[act.act.index][alignDrr];

	//if check 2 is outbound or blocked, other direction is blocked anyway
	if (alignNext == -1 || state[(alignNext << 1) + (int)!isBlackTurn]) {
		return false;
	}

	//found same color 2nd marble, confirm aligning direction
	if (state[(alignNext << 1) + (int)isBlackTurn]) {
		next = MOVE_TABLE[alignNext][act.act.direction];
		//2nd marble's path is blocked or outbound
		if (next == -1 || state[next << 1] || state[(next << 1) + 1]) {
			return false;
		}
		//addtion check for count 3
		if (act.act.count == 3) {
			alignNext = MOVE_TABLE[alignNext][alignDrr];
			//expect 3rd, but not found
			if (alignNext == -1 || !state[(alignNext << 1) + (int)isBlackTurn]) {
				return false;
			}
			next = MOVE_TABLE[alignNext][act.act.direction];
			//3rd marble's path is blocked or outbound
			if (next == -1 || state[next << 1] || state[(next << 1) + 1]) {
				return false;
			}
		}
	}
	//drr 2
	else {
		//assume its aligning on counter clockwise -2 direction
		alignDrr = (act.act.direction + 4) % 6;

		//2nd marble's path is already clear, so check 2nd marble itself.
		alignNext = MOVE_TABLE[alignNext][(act.act.direction + 3) % 6];
		//2nd marble is outbound or is not same color
		if (alignNext == -1 || !state[(alignNext << 1) + (int)isBlackTurn]) {
			return false;
		}
		//addtion check for count 3
		if (act.act.count == 3) {
			alignNext = MOVE_TABLE[alignNext][alignDrr];
			//expect 3rd, but not found
			if (alignNext == -1 || !state[(alignNext << 1) + (int)isBlackTurn]) {
				return false;
			}
			next = MOVE_TABLE[alignNext][act.act.direction];
			//3rd marble's path is blocked or outbound
			if (next == -1 || state[next << 1] || state[(next << 1) + 1]) {
				return false;
			}
		}

	}
	act.alignDrr = alignDrr;
	//all check passed;
	return true;
}

bool logic::isValidInlineMove(std::bitset<128U>& state, weightedAction & act, bool isBlackTurn)
{
	act.alignDrr = act.act.direction;
	bool countingTeam = true;
	int teamCount = 1;
	int oppCount = 0;
	for (int next = MOVE_TABLE[act.act.index][act.act.direction]; true; next = MOVE_TABLE[next][act.act.direction]) {
		//outbound
		if (next == -1) {
			act.teamCount = teamCount;
			act.oppCount = oppCount;
			return !countingTeam;
		}

		//next is team
		if (state[(next << 1) + (int)isBlackTurn]) {
			//blocked
			if (!countingTeam || ++teamCount > 3) {
				return false;
			}
		}
		//next is not team
		else {
			//next is opponent
			if (state[(next << 1) + (int)!isBlackTurn]) {
				if (countingTeam) {
					countingTeam = !countingTeam;
				}
				//opponent is more than team
				if (++oppCount >= teamCount) {
					return false;
				}
			}
			//next is empty
			else {
				act.teamCount = teamCount;
				act.oppCount = oppCount;
				return true;
			}
		}
	}
	return false;
}

std::bitset<128U>& logic::inlineMove(std::bitset<128U>& state, weightedAction & act, bool isBlackTurn) {
	int index = act.act.index;
	int i = 0;
	state.set(index << 1, 0);
	state.set((index << 1) + 1, 0);
	for (; i < act.teamCount; ++i, index = MOVE_TABLE[index][act.act.direction]);
	state.set(index << 1, !isBlackTurn);
	state.set((index << 1) + 1, isBlackTurn);
	if (act.oppCount == 0)
		return state;
	for (; i < act.teamCount + act.oppCount; ++i, index = MOVE_TABLE[index][act.act.direction]);
	if (index == -1) {
		auto scoreI = (isBlackTurn << 0) * 3 + 122;
		auto max = scoreI + 3;
		bool val = state[scoreI];
		bool carry = val & 1;
		state.set(scoreI, val ^ 1);
		while (carry && scoreI < max) {
			val = state[++scoreI];
			state.set(scoreI, val ^ carry);
			carry &= val;
		}
		return state;
	}
	state.set(index << 1, isBlackTurn);
	state.set((index << 1) + 1, !isBlackTurn);
	return state;
}

std::bitset<128U>& logic::sideMove(std::bitset<128U>& state, weightedAction & act, bool isBlackTurn) {
	int index = act.act.index;
	int next = MOVE_TABLE[index][act.act.direction];
	for (int i = 0; i < act.teamCount; ++i) {
		state.set(index << 1, 0);
		state.set((index << 1) + 1, 0);
		state.set(next << 1, !isBlackTurn);
		state.set((next << 1) + 1, isBlackTurn);
		index = MOVE_TABLE[index][act.alignDrr];
		next = MOVE_TABLE[index][act.act.direction];
	}
	return state;
}

bool logic::isValidMove_human(std::bitset<128U> state, action act, bool isBlackTurn)
{
	return false;
}

sf::Vector2i logic::getScoreFromState(std::bitset<128U>& state) {
	auto whiteLost = 0, blackLost = 0;
	for (auto i = 124u, j = 127u; i > 121; --i, --j) {
		whiteLost <<= 1;
		blackLost <<= 1;
		whiteLost |= state[j] << 0;
		blackLost |= state[i] << 0;
	}
	return { blackLost, whiteLost };
}