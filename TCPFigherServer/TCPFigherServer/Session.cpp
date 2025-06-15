#include <queue>
#include "Session.h"

PlayerSession g_Members[TOTAL_PLAYER];

// DATA
std::queue<DeleteJob> g_DeleteQueue;

