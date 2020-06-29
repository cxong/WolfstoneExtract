#pragma once

class FString;

namespace FileSys
{
	enum ESteamApp
	{
		APP_WolfensteinII,
		APP_WolfensteinYoungblood,

		NUM_STEAM_APPS
	};

	FString GetSteamPath(ESteamApp game);
}
