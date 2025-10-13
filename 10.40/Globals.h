#pragma once

namespace Globals {
	bool bIsProdServer = false;

	bool bCreativeEnabled = false;
	bool bSTWEnabled = false;
	bool bEventEnabled = false;

	bool bBotsEnabled = true; // I think you can have bots in this version, not sure yet.

	int MaxBotsToSpawn = 100;
	int MinPlayersForEarlyStart = 95;

	//REAL PLAYERS
	inline static int NextTeamIndex = 0;
	inline static int CurrentPlayersOnTeam = 0;
	inline static int MaxPlayersPerTeam = 1;

	//BOTS PLAYERS
	inline static int NextBotTeamIndex = 0;
	inline static int CurrentBotsOnTeam = 0;
}
