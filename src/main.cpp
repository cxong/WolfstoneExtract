#include "doomerrors.h"
#include "files.h"
#include "filesys_steam.h"
#include "resourcefile.h"
#include "zstring.h"

#include <array>
#include <cstdio>
#include <memory>

static std::array<std::unique_ptr<FResourceFile>, 3> LoadResources(FString basepath)
{
	std::array<std::unique_ptr<FResourceFile>, 3> ret;
	std::array<const char*, 3> names = {
		"chunk_4.resources",
		"sound" PATH_SEPARATOR "soundbanks" PATH_SEPARATOR "pc" PATH_SEPARATOR "sound.pack",
		"sound" PATH_SEPARATOR "soundbanks" PATH_SEPARATOR "pc" PATH_SEPARATOR "english(us).pack"
	};
	// Also in patch_1_english(us).pack
	// Also in patch_3_english(us).pack

	bool ismissing = false;
	for(int i = 0;i < names.size();++i)
	{
		printf("Loading %s", names[i]);
		if((ret[i] = std::unique_ptr<FResourceFile>(FResourceFile::OpenResourceFile(basepath + PATH_SEPARATOR "base" PATH_SEPARATOR + names[i], nullptr))) == nullptr)
		{
			ismissing = true;
			printf(", missing\n");
		}
	}

	if(ismissing)
		throw CFatalError("Missing expected data files.");

	return ret;
}

static void Extract()
{
	auto wolf2path = FileSys::GetSteamPath();
	if(wolf2path.IsEmpty())
		throw CFatalError("Could not find installed Wolfenstein II game data.");

	printf("Wolfenstein II path: %s\n", wolf2path.GetChars());

	auto [chunk4, sound, local] = LoadResources(wolf2path);

	for(uint32_t i = 0;i < chunk4->LumpCount();++i)
	{
		auto lump = chunk4->GetLump(i);
		printf("Extracting %s\n", lump->FullName.GetChars());
		auto f = fopen(lump->FullName, "w");
		auto cache = lump->CacheLump();
		fwrite(cache, lump->LumpSize, 1, f);
		fclose(f);
	}

	for(uint32_t i = 0;i < sound->LumpCount();++i)
	{
		auto lump = sound->GetLump(i);
		printf("Extracting %s\n", lump->FullName.GetChars());
		auto f = fopen(lump->FullName, "w");
		auto cache = lump->CacheLump();
		fwrite(cache, lump->LumpSize, 1, f);
		fclose(f);
	}

	for(uint32_t i = 0;i < local->LumpCount();++i)
	{
		auto lump = local->GetLump(i);
		printf("Extracting %s\n", lump->FullName.GetChars());
		auto f = fopen(lump->FullName, "w");
		auto cache = lump->CacheLump();
		fwrite(cache, lump->LumpSize, 1, f);
		fclose(f);
	}
}

int main(int argc, char* argv[])
{
	printf("Wolfstone Data Extraction Utility 1.0\n");

	try
	{
		Extract();
	}
	catch(CDoomError &error)
	{
		fprintf(stderr, "\nFAILED: %s\n", error.GetMessage());
		return 1;
	}
	return 0;
}
