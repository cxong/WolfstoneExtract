#include "doomerrors.h"
#include "files.h"
#include "filesys_steam.h"
#include "filesys.h"
#include "resourcefile.h"
#include "soundbank.h"
#include "zip.h"
#include "zstring.h"

#include <array>
#include <charconv>
#include <cstdio>
#include <regex>
#include <memory>
#include <vector>

static const FString soundsPath = "sound" PATH_SEPARATOR "soundbanks" PATH_SEPARATOR "pc";

static const std::map<uint32_t, const char*> SoundNames = {
	{0x00A7A4A8u, "DSSWITCH"},
	{0x011D4829u, "DSMCHSTP"},
	{0x015F0AC5u, "DSDOGDTH"},
	{0x017BD4A2u, "DSMUTDTH"},
	{0x02E7D2B6u, "DSENDBN2"},
	{0x0310F041u, "DSFAKSIT"},
	{0x03247F84u, "DSWALK1"},
	{0x03CA7301u, "NAZI_RAP"},
	{0x03F3872Cu, "DSPLDETH"},
	{0x057E0D67u, "DSHITWAL"},
	{0x05987A19u, "DSGRDFIR"},
	{0x05BA1401u, "GETTHEM"},
	{0x06A65B2Au, "DSGMOVER"},
	{0x06AEF84Cu, "DSWALK2"},
	{0x082A773Fu, "HEADACHE"},
	{0x084EC4ECu, "DSSHTDOR"},
	{0x08B07D1Cu, "DSDROPN"},
	{0x0910C9CEu, "DSMVGUN1"},
	// Actually VICTORS bpt used where NAZI_OMI is
	{0x09C2AFBAu, "NAZI_OMI"},
	{0x0A011B88u, "DSGDDTH5"},
	{0x0A1A4A9Du, "DSGOOB"},
	{0x0A3A2104u, "DSSSSIT"},
	{0x0B5B0E79u, "DSDOGATK"},
	{0x0C0E9104u, "DSSLCTWN"},
	{0x0C0E9F24u, "DSNOITEM"},
	{0x0C48B24Du, "SUSPENSE"},
	{0x0C50AA50u, "ROSTER"},
	{0x0D6ACAA5u, "SEARCHN"},
	{0x0D83D8B5u, "DSFOODUP"},
	{0x0DDB7AE3u, "DSHANSIT"},
	{0x0E0894DFu, "TWELFTH"},
	{0x0E0A7172u, "DSGRDSIT"},
	{0x0E0B6F1Au, "DSFAKDTH"},
	{0x0F3798CDu, "DSNAZPAI"},
	{0x0FA36143u, "DSRLAUNC"},
	{0x10B7A13Eu, "DSGDDTH4"},
	{0x10FE10ACu, "DSHITSHI"},
	// Actually HITLWLTZ but used where NAZI_NOR is
	{0x11225254u, "NAZI_NOR"},
	{0x11242DC5u, "DSFATSIT"},
	{0x1167D12Cu, "DSPISTOL"},
	{0x11BEF1F3u, "ENDLEVEL"},
	{0x13708AD1u, "DSMGUN"},
	{0x13BE4844u, "DSFATDTH"},
	{0x13D5899Fu, "DSGDDTH1"},
	{0x13F3E3EBu, "DSFART"},
	{0x1463D500u, "DSDRCLS"},
	{0x14FE3846u, "DSMVGUN2"},
	{0x152B095Fu, "DSENDBN1"},
	{0x15F2D059u, "DSSELECT"},
	{0x160EE504u, "DSSCBDTH"},
	{0x167ED112u, "DSCGUNUP"},
	{0x17D122ADu, "URAHERO"},
	{0x18020AD9u, "DSHANDTH"},
	{0x18A2C485u, "PACMAN"},
	{0x19CE1EB9u, "DSBNS1UP"},
	{0x1A5EBDD3u, "DSPSHWAL"},
	{0x1A6ADEEBu, "DSBOSSFR"},
	{0x1AE7B296u, "DSBONUS4"},
	{0x1B1D7751u, "DSGDDTH7"},
	{0x1B5090CFu, "DSESCPRS"},
	{0x1B66113Eu, "DSKNFSWG"},
	{0x1B8BF20Bu, "DSGDDTH6"},
	{0x1BEF5B1Du, "DSAMMOUP"},
	{0x1EE17085u, "DSOFFDTH"},
	{0x214D99C0u, "DSGETKEY"},
	{0x21A6CFDEu, "DSPRC100"},
	{0x21DA0A8Au, "DUNGEON"},
	{0x222D92DAu, "DSSSDTH"},
	{0x22F3B414u, "SALUTE"},
	{0x23AB1329u, "ZEROHOUR"},
	// Not sure on this one since there are many very similar sounds in Wolf3D.
	// However, it doesn't matter much since this sound is unused.
	{0x23AE4001u, "DSBOSSIT"},
	{0x2479AADBu, "DSSCBATK"},
	{0x24D870DFu, "DSMGUNUP"},
	{0x24F7A979u, "DSBONUS3"},
	{0x27067F95u, "DSGRTDTH"},
	{0x27370765u, "DSHARTBT"},
	{0x2761FCCBu, "DSPLPAIN"},
	{0x28BB8C3Du, "DSOTOSIT"},
	{0x295945DEu, "CORNER"},
	{0x2B9C7F9Fu, "FUNKYOU"},
	{0x2BDCCC72u, "DSSCBSIT"},
	{0x2BFE167Au, "DSGDDTH2"},
	{0x2C1F5DF3u, "DSMEDIUP"},
	{0x2D077554u, "DSOFFSIT"},
	{0x2E631A44u, "ULTIMATE"},
	{0x2EFC91F9u, "DSSSFIRE"},
	{0x2F9F663Fu, "PREGNANT"},
	{0x306646D8u, "DSOTODTH"},
	// This seems to be an extra sound. Not that it matters much since even
	// BOSSIT isn't used. Judging by length this is one of non-Hans boss sounds.
	{0x310FB57Bu, "DSBOSSI2"},
	{0x31A2B52Fu, "POW"},
	{0x31F30118u, "DSNOWAY"},
	// Copy of DSGDDTH2 (although bytes differ bitstream is same). Probably here
	// to complete the adlib sound table although it's never used.
	{0x3223E805u, "DSGDDTH3"},
	{0x34962C4Au, "DSBONUS2"},
	// Actually VICMARCH but used where WARMARCH and INTROCW3 is
	{0x34E37DC5u, "WARMARCH"},
	{0x34FAC876u, "DSNAZIHT"},
	{0x3570E2F0u, "DSGRTSIT"},
	{0x3613AA1Du, "DSCGUN"},
	{0x36AFEDDFu, "WONDERIN"},
	{0x370A956Fu, "DSGDDTH8"},
	{0x3893F45Fu, "DSFAKFIR"},
	{0x39475108u, "DSSLCTIT"},
	{0x39F47F3Eu, "DSSLURPE"},
	{0x3A63EC71u, "DSHITSIT"},
	{0x3B04E074u, "DSBONUS1"},
	{0x3C3E5E47u, "DSNOTIN"},
	{0x3D93D4EFu, "DSBAREXP"},
	{0x3D9BF301u, "DSDOGSIT"},
	{0x3DAEE1EAu, "GOINGAFT"},
	{0x3DB6F9F4u, "DSYEAH"},
	{0x3DEBD92Du, "DSNOBNS"},
	{0x3ED3FE9Fu, "DSHITDTH"}
};

//using ResourceCollection = std::vector<std::unique_ptr<FResourceFile>>;
struct ResourceCollection : std::vector<std::unique_ptr<FResourceFile>>
{
	using std::vector<std::unique_ptr<FResourceFile>>::vector;

	FResourceLump *Check(FString name) const
	{
		for(auto iter = rbegin(); iter != rend(); ++iter)
		{
			for(unsigned int i = 0; i < (*iter)->LumpCount(); ++i)
			{
				auto lump = (*iter)->GetLump(i);
				if(lump->FullName.Compare(name) == 0)
					return lump;
			}
		}
		return nullptr;
	}

	FResourceLump *Find(FString name) const
	{
		if(FResourceLump *lump = Check(name))
			return lump;
		throw CFatalError("Expected data file not found in resources");
	}

	void AddIn(ResourceCollection &&other)
	{
		for(auto &res : other)
			emplace_back(std::move(res));
	}
};

struct Options
{
	FString Language;
};

struct GameInfo
{
	FileSys::ESteamApp App;
	const char* Name;
	const char* Game;
	const char* OutputName;
	ResourceCollection (*LoadResources)(FString, FString);
};

struct MemoryLump : FResourceLump
{
	std::vector<uint8_t> Buffer;

	MemoryLump(std::vector<uint8_t> &&data) : Buffer(std::move(data))
	{
		LumpSize = Buffer.size();
	}

	int FillCache()
	{
		Cache = (char*)Buffer.data();
		RefCount = -1;
		return -1;
	}
};

// Creates ecwolf.wl6 data from the sound banks
template<typename ... T> // T should be FResourceLump but C++ doesn't have a nice way to represent that
static std::unique_ptr<FResourceLump> BuildECWolfArchive(T* ... soundResource)
{
	// We need to keep these around until we can make the FZip::Build call.
	std::vector<std::unique_ptr<MemoryLump>> soundStorage;

	FZip archive;

	auto readers = std::array<std::unique_ptr<FileReader>, sizeof...(soundResource)>{
		std::unique_ptr<FileReader>{soundResource->NewReader()}...
	};

	for(auto& reader : readers)
	{
		FSoundbank bank(reader.get());
		for(unsigned int i = 0; i < bank.Sounds.Size(); ++i)
		{
			uint32_t id = bank.Sounds[i].Id;
			const char* sname = "";
			if(auto value = SoundNames.find(id); value != SoundNames.end())
				sname = value->second;

			char name[32];
			if(sname[0] != 0 && !(sname[0] == 'D' && sname[1] == 'S'))
				snprintf(name, 32, "music/%s.ogg", sname);
			else if(sname[0] != 0)
				snprintf(name, 32, "sounds/%s.ogg", sname);
			else
			{
				fprintf(stderr, "Sound %08X does not have a name assigned. This is probably a bug.\n", id);
				snprintf(name, 32, "sounds/%08X.ogg", id);
			}
			archive.AddFile(name, soundStorage.emplace_back(std::make_unique<MemoryLump>(std::move(bank.Sounds[i].Data))).get());
		}
	}

	return std::make_unique<MemoryLump>(archive.Build());
}

// Creates language.txt from strings data
static std::unique_ptr<FResourceLump> BuildLanguage(FResourceLump *langLump)
{
	// TODO: Actually implement this
	std::vector<uint8_t> data;
	data.resize(langLump->LumpSize);
	memcpy(data.data(), langLump->CacheLump(), langLump->LumpSize);
	langLump->ReleaseCache();
	return std::make_unique<MemoryLump>(std::move(data));
}

// Returns a list of installed languages, so that we can extract them all
// and gracefully handle non-English users which may not have English installed.
//
// Known available languages:
//  - english(us)
//  - french(france)
//  - italian
//  - portuguese(brazil)
//  - russian
//  - spanish(spain)
//
// As of 2020-02-12 installing as Simplified Chinese will install all of the
// language voices.  Traditional Chinese, Polish, and of course English only
// install the English voices.  The rest install English and their respective
// language.  Interesting Simplified Chinese only provides the patched files
// for English.  Ultimately none of this matters since despite some differences
// in how the data is ordered the actual sounds are identical in all of the
// languages.
static std::vector<FString> DetectLanguages(FString basePath)
{
	File soundsDir(basePath + PATH_SEPARATOR "base" PATH_SEPARATOR + soundsPath);

	const std::regex langRegex{"([a-z\\(\\)]+).pack"};

	std::vector<FString> languages;

	auto dir = soundsDir.getFileList();
	for(unsigned int i = 0;i < dir.Size(); ++i)
	{
		const FString &file = dir[i];

		// Ignore the non-language specific pack
		if(file.Compare("sound.pack") == 0)
			continue;

		std::cmatch match;
		if(std::regex_match(file.GetChars(), file.GetChars() + file.Len(), match, langRegex))
			languages.push_back(match[1].str().c_str());
	}

	if(languages.size() == 0)
		throw CFatalError("Missing language-specific files.");

	std::sort(languages.begin(), languages.end(), [](FString a, FString b) { return a.Compare(b) < 0; });
	return languages;
}

static ResourceCollection LoadPatchedResource(File file)
{
	auto patchPattern = file.getFileName();
	if(patchPattern.Compare("gameresources.resources") == 0)
		patchPattern = "patch_([0-9]+).resources";
	else if(patchPattern.Compare("gameresources_pc.resources") == 0)
		patchPattern = "patch_([0-9]+)_pc.resources";
	else
	{
		patchPattern.Substitute("(", "\\(");
		patchPattern.Substitute(")", "\\)");
		patchPattern = FString("patch_([0-9]+)_") + patchPattern;
	}

	std::regex patchRegex{patchPattern};

	std::vector<File> fileList;
	fileList.push_back(file.getPath());

	auto dir = File(file.getDirectory()).getFileList();
	for(unsigned int i = 0; i < dir.Size(); ++i)
	{
		auto const& candidate = dir[i];

		std::cmatch match;
		if(std::regex_match(candidate.GetChars(), candidate.GetChars() + candidate.Len(), match, patchRegex))
		{
			auto const numstr = match[1].str();

			int num;
			auto const result = std::from_chars(numstr.data(), numstr.data()+numstr.size(), num);
			if(result.ec != std::errc())
				continue;

			if(fileList.size() <= num)
				fileList.resize(num+1);
			fileList[num] = file.getDirectory() + PATH_SEPARATOR + candidate;
		}
	}

	ResourceCollection resFiles;
	for(auto const& res : fileList)
	{
		printf("Loading %s", res.getFileName().GetChars());
		auto rf = FResourceFile::OpenResourceFile(res.getPath(), nullptr);
		if(!rf)
		{
			printf(", missing\n");
			continue;
		}
		resFiles.emplace_back(rf);
	}

	if(resFiles.size() == 0)
		throw CFatalError("Expected resource couldn't be loaded!");

	return resFiles;
}

static void LoadSoundbanks(FString basePath, FString lang, ResourceCollection &ret)
{
	ret.AddIn(LoadPatchedResource(basePath + PATH_SEPARATOR "base" PATH_SEPARATOR + soundsPath + PATH_SEPARATOR "sound.pack"));

	// At one point I tried to load all the language packs, but found all the
	// sounds to be identical so no point.  No harm in keeping the extra code
	// to support multiple languages around though.
	auto collection = LoadPatchedResource(basePath + PATH_SEPARATOR "base" PATH_SEPARATOR + soundsPath + PATH_SEPARATOR + lang + ".pack");

	// Deconflict the various languages that may be loaded.
	for(auto &rf : collection)
	{
		for(unsigned int i = 0; i < rf->LumpCount(); ++i)
		{
			auto lump = rf->GetLump(i);
			lump->LumpNameSetup(lang + "/" + lump->FullName);
		}
	}

	ret.AddIn(std::move(collection));
}

static ResourceCollection LoadWolfensteinIIResources(FString basePath, FString lang)
{
	ResourceCollection ret;

	ret.AddIn(LoadPatchedResource(basePath + PATH_SEPARATOR "base" PATH_SEPARATOR "gameresources.resources"));
	ret.AddIn(LoadPatchedResource(basePath + PATH_SEPARATOR "base" PATH_SEPARATOR "chunk_4.resources"));

	LoadSoundbanks(basePath, lang, ret);
	return ret;
}

static ResourceCollection LoadYoungbloodResources(FString basePath, FString lang)
{
	ResourceCollection ret;

	ret.AddIn(LoadPatchedResource(basePath + PATH_SEPARATOR "base" PATH_SEPARATOR "chunk_8_pc.resources"));

	// One of the patches not associated with chunk_8_pc has the updated vgagraph
	ret.AddIn(LoadPatchedResource(basePath + PATH_SEPARATOR "base" PATH_SEPARATOR "gameresources_pc.resources"));

	LoadSoundbanks(basePath, lang, ret);
	return ret;
}

static void Extract(GameInfo game, FString wolfpath, FString language)
{
	auto languages = DetectLanguages(wolfpath);

	// Default to first available language if one isn't specified.
	if(language.IsEmpty())
		language = languages[0];

	printf("Found languages (desired = %s):\n", language.GetChars());
	bool foundDesiredLanguage = false;
	for(auto l : languages)
	{
		if(l.Compare(language) == 0)
			foundDesiredLanguage = true;

		printf("  - %s\n", l.GetChars());
	}

	if(!foundDesiredLanguage)
		throw CFatalError("Could not detect desired language pack");

	printf("\nNOTE: Sounds in all languages are identical. Multi-language support in this program is purely academic.\n\n");

	auto resFiles = game.LoadResources(wolfpath, language);

	printf("Extracting %s...\n", game.Game);

	auto langStrings = BuildLanguage(resFiles.Find(game.App == FileSys::APP_WolfensteinII ? "strings/english.lang" : "strings/english.json"));

	auto ecwolfWl6 = game.App == FileSys::APP_WolfensteinII
		? BuildECWolfArchive(
			resFiles.Find("sb_wolfstone.bnk"),
			resFiles.Find(language + "/sb_vo_wolfstone.bnk")
		)
		: BuildECWolfArchive(resFiles.Find(language + "/wolfstone.bnk"));

	FZip zip;
	zip.AddFile("ecwolf.wl6", ecwolfWl6.get());
	zip.AddFile("gamemaps.wl6", resFiles.Find("gamemaps.wl6"));
	zip.AddFile("maphead.wl6", resFiles.Find("maphead.wl6"));
	zip.AddFile("vgadict.wl6", resFiles.Find("vgadict.wl6"));
	zip.AddFile("vgahead.wl6", resFiles.Find("vgahead.wl6"));
	zip.AddFile("vgagraph.wl6", resFiles.Find("vgagraph.wl6"));
	zip.AddFile("vswap.wl6", resFiles.Find("vswap.wl6"));
	zip.AddFile("language.json", langStrings.get());

	switch(game.App)
	{
	case FileSys::APP_WolfensteinII:
		break;
	case FileSys::APP_WolfensteinYoungblood:
		zip.AddFile("demo0.wl6", resFiles.Find("demo0.wl6"));
		zip.AddFile("demo1.wl6", resFiles.Find("demo1.wl6"));
		zip.AddFile("demo2.wl6", resFiles.Find("demo2.wl6"));
		zip.AddFile("demo3.wl6", resFiles.Find("demo3.wl6"));
		break;
	}

	if(auto f = File(game.OutputName).open("wb"))
	{
		auto zipData = zip.Build();
		if(fwrite(zipData.data(), zipData.size(), 1, f) != 1)
		{
			fclose(f);
			throw CFatalError("Failed to write file");
		}
		fclose(f);
	}
	else
		throw CFatalError("Couldn't open output file for writing");

	printf("Done!\n");
}

static std::tuple<GameInfo, FString> PickGame()
{
	constexpr std::array<GameInfo, FileSys::NUM_STEAM_APPS> GameInfoTable
	{
		GameInfo{FileSys::APP_WolfensteinII, "Wolfenstein II", "Wolfstone 3D", "wolfstone.pk3", LoadWolfensteinIIResources},
		GameInfo{FileSys::APP_WolfensteinYoungblood, "Wolfenstein: Youngblood", "Elite Hans: Die Neue Ordnung", "elitehans.pk3", LoadYoungbloodResources}
	};

	TArray<std::tuple<GameInfo, FString>> candidates;
	for(GameInfo game : GameInfoTable)
	{
		auto path = FileSys::GetSteamPath(game.App);
		if(path.IsNotEmpty())
			candidates.Push({game, path});
	}

	if(candidates.Size() == 0)
		throw CFatalError("Could not find installed Wolfenstein II or Youngblood game data.");

	int selection = 0;

	if(candidates.Size() > 1)
	{
		printf("Select game to extract (0 to exit):\n");
		int i = 1;
		for(int i = 0; i < candidates.Size(); ++i)
			printf("    %d: %s\n", i+1, std::get<0>(candidates[i]).Game);

		for(;;)
		{
			printf("? ");
			fflush(stdout);
			selection = getchar() - '1';
			if(selection == -1)
				throw CNoRunExit();

			if(selection >= 0 && selection < candidates.Size())
				break;

			// Flush any remaining input for the line
			while(getchar() != '\n')
			{
				if(feof(stdin))
					throw CNoRunExit();
			}
		}
	}

	printf("%s path: %s\n", std::get<0>(candidates[selection]).Name, std::get<1>(candidates[selection]).GetChars());
	return candidates[selection];
}

static Options ParseOptions(int argc, const char* const * argv)
{
	struct OptHandlers
	{
		const char* name;
		char shortCode;
		unsigned args;
		void (*handler)(Options &opt, const char* const * argv);
	};

	const std::array<OptHandlers, 1> handlers {
		{
			"language", 'l', 1, [](Options &opts, const char* const * argv){
				opts.Language = argv[0];
			}
		}
	};

	Options opts;

	for(int i = 1; i < argc; ++i)
	{
		if(strlen(argv[i]) < 2 || argv[i][0] != '-')
			throw CFatalError("Invalid command line arguments");

		decltype(handlers)::const_iterator it;
		if(argv[i][1] == '-')
		{
			const char* name = argv[i]+2;
			auto it = std::find_if(handlers.cbegin(), handlers.cend(), [name](OptHandlers const &opt) {
				return strcmp(opt.name, name) == 0;
			});
			if(it == handlers.end())
				throw CFatalError("Unknown command line switch");
			if(i + it->args >= argc)
				throw CFatalError("Command line switch takes arguments which are not present");

			it->handler(opts, argv + i + 1);
			argv += it->args;
			i += it->args;
		}
		else
		{
			char name = argv[i][2];
			auto it = std::find_if(handlers.cbegin(), handlers.cend(), [name](OptHandlers const &opt) {
				return opt.shortCode && opt.shortCode == name;
			});
			assert(it->args <= 1);

			if(it == handlers.end())
				throw CFatalError("Unknown command line switch");

			const char* arg;
			if(it->args)
			{
				if(strlen(argv[i]) > 2)
					arg = argv[i]+2;
				else if(i + 1 < argc)
				{
					arg = argv[i+1];
					++argv;
					++i;
				}
				else
					throw CFatalError("Command line switch takes arguments which are not present");
			}

			it->handler(opts, &arg);
		}
	}

	return opts;
}

int main(int argc, char* argv[])
{
	printf("Wolfstone Data Extraction Utility 1.0\n");

	try
	{
		auto opts = ParseOptions(argc, argv);

		auto [ game, path ] = PickGame();
		Extract(game, path, opts.Language);
	}
	catch(CNoRunExit&) {}
	catch(CDoomError &error)
	{
		fprintf(stderr, "\nFAILED: %s\n", error.GetMessage());
		return 1;
	}

	return 0;
}
