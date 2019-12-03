#pragma once

#ifdef _WIN32
#define PATH_SEPARATOR "\\"
#else
#define PATH_SEPARATOR "/"
#endif

class FString;

namespace FileSys
{
	FString GetSteamPath();
}
