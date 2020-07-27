#include "doomerrors.h"
#include "files.h"
#include "filesys_steam.h"
#include "filesys.h"
#include "resourcefile.h"
#include "scanner.h"
#include "soundbank.h"
#include "zip.h"
#include "zstring.h"

#include <array>
#include <cassert>
#include <charconv>
#include <cstdio>
#include <regex>
#include <memory>
#include <vector>

static const FString soundsPath = "sound" PATH_SEPARATOR "soundbanks" PATH_SEPARATOR "pc";

// We can't rely on the data files providing a correct date so lets set all the
// files to the date of Wolfenstein II's release so that the output of this
// program is still a constant.
constexpr auto WOLFII_DATE = DOSDate(2017, 10, 27);
constexpr auto YOUNGBLOOD_DATE = DOSDate(2019, 7, 25);

using FSoundNameTable = const std::map<uint32_t, const char*>;

static FSoundNameTable WolfstoneSoundNames = {
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
	{0x09C2AFBAu, "VICTORS"},
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
	{0x11225254u, "HITLWLTZ"},
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
	{0x34E37DC5u, "VICMARCH"},
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

static FSoundNameTable EliteHansSoundNames = {
	{0x0140173Du, "DSSSFIRE"},
	{0x018801ABu, "DSGDDTH8"},
	{0x02DBE468u, "DSMVGUN2"},
	{0x03116A17u, "DSSHTDOR"},
	{0x04903C67u, "DSGETKEY"},
	{0x05A26791u, "DSWALK2"},
	{0x064405D0u, "DSDRCLS"},
	{0x073ABB0Cu, "DSWALK1"},
	{0x07BFC6E0u, "DSBONUS2"},
	{0x07DE4D48u, "DSBOSSI2"},
	{0x07E460A8u, "DSMEDIUP"},
	{0x08640562u, "DSBONUS4"},
	{0x08FA10B2u, "DSGMOVER"},
	{0x0B8D2677u, "DSBONUS1"},
	{0x0BC08128u, "DSNOBNS"},
	{0x0BE15E7Bu, "DSESCPRS"},
	{0x0D754F33u, "DSBAREXP"},
	{0x0ECBF2EDu, "DSBOSSFR"},
	{0x0F239149u, "DSGOOB"},
	{0x1075648Fu, "DSSLURPE"},
	{0x11341359u, "DSBONUS3"},
	{0x12DB7EA6u, "DSGDDTH2"},
	{0x132EA92Cu, "DSDROPN"},
	{0x1345EEC1u, "DSMCHSTP"},
	{0x140E8BBFu, "DSSCBATK"},
	{0x144C96FBu, "DSPLDETH"},
	{0x15915329u, "DSNOWAY"},
	{0x15D4A3ECu, "DSNAZIHT"},
	{0x166A537Du, "DSGDDTH6"},
	{0x16E72951u, "DSCGUNUP"},
	{0x17EF6D70u, "DSNAZPAI"},
	{0x1AEA00D1u, "DSAMMOUP"},
	{0x1C7EABEFu, "DSSWITCH"},
	{0x20733D71u, "DSNOTIN"},
	{0x2229CA70u, "DSGDDTH1"},
	{0x24265DA7u, "DSCGUN"},
	{0x2563C672u, "DSYEAH"},
	{0x27C503C7u, "DSGDDTH4"},
	{0x28A272CCu, "DSPSHWAL"},
	{0x28CDC740u, "DSKNFSWG"},
	{0x293C4617u, "DSBOSSIT"},
	{0x2A186AFBu, "DSFART"},
	{0x2AAB0F89u, "DSDOGDTH"},
	{0x2B53FE1Au, "DSFOODUP"},
	{0x2B760104u, "DSGDDTH5"},
	{0x2CC45E4Eu, "DSSLCTWN"},
	{0x2FFF9F9Eu, "DSRLAUNC"},
	{0x3033F9A4u, "DSDOGSIT"},
	{0x30B6E57Eu, "DSHARTBT"},
	{0x30C9E01Au, "DSGDDTH3"},
	{0x3107288Du, "DSMVGUN1"},
	{0x32478A6Au, "DSPRC100"},
	{0x3373147Bu, "DSPLPAIN"},
	{0x33CCF4E9u, "DSBNS1UP"},
	{0x345ADA46u, "DSPISTOL"},
	{0x34B28759u, "DSDOGATK"},
	{0x370433CAu, "DSNOITEM"},
	{0x3744FCD8u, "DSGDDTH7"},
	{0x3768EFFBu, "DSMUTDTH"},
	{0x37A23BCDu, "DSFAKFIR"},
	{0x381DC6B8u, "DSMGUNUP"},
	{0x397E48CCu, "DSMGUN"},
	{0x3CF22009u, "DSENDBN2"},
	{0x3D81EF6Eu, "DSGRDFIR"},
	{0x3E1FCDE2u, "DSHITWAL"},
	{0x3E244F37u, "DSSELECT"},
	{0x3FB7865Au, "DSENDBN1"},
	{0x3FC5D66Bu, "DSSLCTIT"},

	// Music are loose WEM files in generic sound pack
	{0x0736EF20u, "URAHERO"},
	{0x0839C12Fu, "NAZI_RAP"},
	{0x0E4330B6u, "HITLWLTZ"},
	{0x11985AF6u, "SEARCHN"},
	{0x137C718Bu, "VICTORS"},
	{0x181A1A36u, "GETTHEM"},
	{0x18AE34A1u, "PACMAN"},
	{0x1A682365u, "VICMARCH"},
	{0x1A7D2BF6u, "SUSPENSE"},
	{0x1D88A829u, "WONDERIN"},
	{0x22E25169u, "ROSTER"},
	{0x25C4337Cu, "PREGNANT"},
	{0x26D7BFF4u, "ZEROHOUR"},
	{0x28F03F6Bu, "TWELFTH"},
	{0x2B7F2798u, "ULTIMATE"},
	{0x2BEC1B09u, "POW"},
	{0x2D81808Eu, "SALUTE"},
	{0x2E4DD2DCu, "DUNGEON"},
	{0x2E78620Cu, "GOINGAFT"},
	{0x35DA1B49u, "ENDLEVEL"},
	{0x39AC8E60u, "FUNKYOU"},
	{0x3FA0193Eu, "CORNER"},
	{0x3FCFC182u, "HEADACHE"},

	// Voices are loose WEM files in language pack
	{0x0015A563u, "DSHITDTH"},
	{0x07E8EEDDu, "DSFATDTH"},
	{0x0CA4A00Du, "DSGRTSIT"},
	{0x0E5E84A3u, "DSHITSIT"},
	{0x09D75597u, "DSFAKDTH"},
	{0x166E7272u, "DSFAKSIT"},
	{0x185DA298u, "DSSCBSIT"},
	{0x1A949A42u, "DSOTOSIT"},
	{0x1AC5C079u, "DSHANDTH"},
	{0x1C8A4903u, "DSGRDSIT"},
	{0x1E34DB4Au, "DSHITSHI"},
	{0x1F565751u, "DSFATSIT"},
	{0x20B7F099u, "DSSCBDTH"},
	{0x210F1B75u, "DSOFFSIT"},
	{0x280250E2u, "DSOTODTH"},
	{0x2F77B9C1u, "DSSSDTH"},
	{0x3341EAC7u, "DSSSSIT"},
	{0x3409E700u, "DSGRTDTH"},
	{0x3D69DE8Au, "DSHANSIT"},
	{0x3FC1AC3Cu, "DSOFFDTH"}
};

// Typically #str_wolfstone_x is STR_X, but some we need to do an explicit
// mapping for
static const std::map<std::string, const char*> LangMap = {
	{"#str_wolfstone_curgame", "CURGAME"},

	// The following are technically correctly named but ECWolf prefers them in
	// a different form. So rename them to something else that isn't used.
	{"#str_wolfstone_ratkill", "STR_RATKILL_FULL"},
	{"#str_wolfstone_rat2kill", "STR_RAT2KILL_FULL"},
	{"#str_wolfstone_ratsecret", "STR_RATSECRET_FULL"},
	{"#str_wolfstone_rat2secret", "STR_RAT2SECRET_FULL"},
	{"#str_wolfstone_rattreasure", "STR_RATTREASURE_FULL"},
	{"#str_wolfstone_rat2treasure", "STR_RAT2TREASURE_FULL"}
};

static const std::map<std::string, const char*> LanguageCodes = {
	{"brazilian_portuguese", "ptb"},
	{"english", "enu default"},
	{"french", "fr"},
	{"german", "de"},
	{"italian", "ita"},
	{"japanese", "jpn"},
	{"korean", "kok"},
	{"latin_spanish", "esm"},
	{"polish", "plk"},
	{"russian", "rus"},
	{"s_chinese", "chs"},
	{"spanish", "es"},
	{"t_chinese", "cht"}
};

static const FString Mapinfo = R"EOF(
gameinfo
{
	advisorypic = ""
	pageindextext = ""
	signon = ""
	quitmessages = "$STR_QUITSUR"
}
)EOF";

static const FString EliteHansEpisodes = R"EOF(clearepisodes

episode "MAP01"
{
	lookup = "STR_EPISODE1"
	picname = "M_EPIS1"
	key = "E"
}

episode "MAP11"
{
	lookup = "STR_EPISODE2"
	picname = "M_EPIS2"
	key = "O"
}
)EOF";

static const FString WolfstoneEpisodes = EliteHansEpisodes + R"EOF(
episode "MAP21"
{
	lookup = "STR_EPISODE3"
	picname = "M_EPIS3"
	key = "D"
}

episode "MAP31"
{
	lookup = "STR_EPISODE4"
	picname = "M_EPIS4"
	key = "A"
}

episode "MAP41"
{
	lookup = "STR_EPISODE5"
	picname = "M_EPIS5"
	key = "T"
}

episode "MAP51"
{
	lookup = "STR_EPISODE6"
	picname = "M_EPIS6"
	key = "K"
}
)EOF";

static const FString Sndinfo = R"EOF($musicalias NAZI_NOR HITLWLTZ
$musicalias NAZI_OMI VICTORS
$musicalias WARMARCH VICMARCH
$musicalias INTROCW3 VICMARCH
)EOF";

struct HighScore
{
	const char* Name;
	uint32_t Score;
	uint8_t Episode;
	uint8_t Level;
};

// I believe these default score tables are in the game code so digging them
// out would be a lot of work and probably be unreliable if the data were to
// change.
constexpr std::array WolfstoneScores {
	HighScore{"Max Hass", 42000, 6, 2},
	HighScore{"Caroline", 23000, 5, 1},
	HighScore{"Set", 17000, 4, 2},
	HighScore{"Panzer Mouse", 15000, 4, 1},
	HighScore{"Anya", 13000, 4, 1},
	HighScore{"Bombate", 10000, 3, 2},
	HighScore{"Shoshana", 7000, 2, 2}
};

constexpr std::array EliteHansScores {
	HighScore{"Maria", 43000, 2, 6},
	HighScore{"Kenneth", 35000, 2, 6},
	HighScore{"Arthur", 34000, 2, 5},
	HighScore{"Lafayette", 19000, 2, 3},
	HighScore{"Dimitri", 17000, 2, 3},
	HighScore{"Abby", 16000, 2, 2},
	HighScore{"Muldeen", 3000, 1, 1}
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
	FString Path;
};

struct GameInfo
{
	FileSys::ESteamApp App;
	const char* Name;
	const char* Game;
	const char* OutputName;
	FSoundNameTable &SoundNames;
	const FString &MapinfoEpisodes;
	const std::array<HighScore, 7> &Scores;
	ResourceCollection (*LoadResources)(FString, FString);
	uint16_t Date;
};

struct MemoryLump : FResourceLump
{
	std::vector<uint8_t> Buffer;

	MemoryLump(std::vector<uint8_t> &&data) : Buffer(std::move(data))
	{
		LumpSize = static_cast<int>(Buffer.size());
	}

	int FillCache()
	{
		Cache = (char*)Buffer.data();
		RefCount = -1;
		return -1;
	}

	static std::unique_ptr<MemoryLump> FromString(const char* str)
	{
		std::vector<uint8_t> data;
		data.resize(strlen(str));
		memcpy(data.data(), str, data.size());
		return std::make_unique<MemoryLump>(std::move(data));
	}
};

static const char* MakeSoundFilename(char* dest, int destLen, uint32_t id, const char* name)
{
	if(name[0] != 0 && !(name[0] == 'D' && name[1] == 'S'))
		snprintf(dest, destLen, "music/%s.ogg", name);
	else if(name[0] != 0)
		snprintf(dest, destLen, "sounds/%s.ogg", name);
	else
	{
		fprintf(stderr, "Sound %08X does not have a name assigned. This is probably a bug.\n", id);
		snprintf(dest, destLen, "sounds/%08X.ogg", id);
	}
	return dest;
}

// Creates ecwolf.wl6 data from the sound banks
template<typename ... T> // T should be FResourceLump but C++ doesn't have a nice way to represent that
static std::unique_ptr<FResourceLump> BuildECWolfArchive(const GameInfo &game, const ResourceCollection &resFiles, FResourceLump *langLump, T* ... soundResource)
{
	// We need to keep these around until we can make the FZip::Build call.
	std::vector<std::unique_ptr<MemoryLump>> lumpStorage;

	FZip archive;
	archive.SetDate(game.Date);
	archive.AddFile("language.txt", langLump);
	archive.AddFile("mapinfo.txt", lumpStorage.emplace_back(MemoryLump::FromString(game.MapinfoEpisodes + Mapinfo)).get());
	archive.AddFile("sndinfo.txt", lumpStorage.emplace_back(MemoryLump::FromString(Sndinfo)).get());

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
			if(auto value = game.SoundNames.find(id); value != game.SoundNames.end())
				sname = value->second;

			char name[32];
			archive.AddFile(MakeSoundFilename(name, 32, id, sname), lumpStorage.emplace_back(std::make_unique<MemoryLump>(std::move(bank.Sounds[i].Data))).get());
		}
	}

	// Look for loose wem files with the right ID. Right now we only need to do
	// this for Youngblood as Wolfenstein II has everything in the soundbanks.
	if(game.App == FileSys::APP_WolfensteinYoungblood)
	{
		int found = 0;
		printf("Scanning for additional sounds:     0");
		fflush(stdout);
		for(auto iter = resFiles.rbegin(); iter != resFiles.rend(); ++iter)
		{
			for(unsigned int i = 0; i < (*iter)->LumpCount(); ++i)
			{
				auto lump = (*iter)->GetLump(i);

				auto name = lump->FullName;
				if(name.Right(4).Compare(".wem") != 0)
					continue;

				if(auto index = name.LastIndexOf('/'); index != -1)
					name = name.Mid(index+1);

				const auto id = strtoul(name, nullptr, 10);
				if(auto entry = game.SoundNames.find(id); entry != game.SoundNames.end())
				{
					printf("\b\b\b\b\b%5d", found++);
					fflush(stdout);

					char buf[32];
					archive.AddFile(MakeSoundFilename(buf, 32, id, entry->second), lumpStorage.emplace_back(std::make_unique<MemoryLump>(FSoundbank::ConvertWem(lump->CacheLump(), lump->LumpSize))).get());
					lump->ReleaseCache();
				}
			}
		}
		puts("");
	}

	return std::make_unique<MemoryLump>(archive.Build());
}

// Builds config.wl6
static std::unique_ptr<FResourceLump> BuildConfig(const std::array<HighScore, 7> &scores)
{
	std::vector<uint8_t> configData;
	configData.resize(522);
	memset(configData.data(), 0, configData.size());

	uint8_t *ptr = configData.data();
	for(auto score : scores)
	{
		strncpy(reinterpret_cast<char*>(ptr), score.Name, 57);
		WriteLittleLong(ptr+58, score.Score);
		WriteLittleShort(ptr+62, score.Level);
		WriteLittleShort(ptr+64, score.Episode-1);
		ptr += 66;
	}

	// The values below are basically the default config.wl6 from Wolf3D.
	// Just increased viewsize to match Wolfstone's screen size.

	WriteLittleShort(ptr, 2); // sd
	ptr += 2;
	WriteLittleShort(ptr, 1); // sm
	ptr += 2;
	WriteLittleShort(ptr, 3); // sds
	ptr += 2;
	WriteLittleShort(ptr, 1); // mouseenabled
	ptr += 2;
	WriteLittleShort(ptr, 0); // joystickenabled
	ptr += 2;
	WriteLittleShort(ptr, 0); // joypadenabled
	ptr += 2;
	WriteLittleShort(ptr, 0); // joystickprogressive
	ptr += 2;
	WriteLittleShort(ptr, 0); // joystickport
	ptr += 2;

	constexpr std::array<uint16_t, 4+8+4+4> buttons {
		0x48, 0x4D, 0x50, 0x4B, // dirscan
		0x1D, 0x38, 0x36, 0x39, 0x02, 0x03, 0x04, 0x05, // buttonscan
		0x00, 0x01, 0x03, 0xFFFF, // buttonmouse
		0x00, 0x01, 0x03, 0x02 // buttonjoy
	};
	for(auto value : buttons)
	{
		WriteLittleShort(ptr, value);
		ptr += 2;
	}

	WriteLittleShort(ptr, 0x13); // viewsize
	ptr += 2;

	WriteLittleShort(ptr, 5); // mouseadjustment
	ptr += 2;

	assert(ptr == configData.data()+configData.size());

	return std::make_unique<MemoryLump>(std::move(configData));
}

// Creates language.txt from strings data
static FString ConvertLanguage(const char* langCode, FResourceLump *langLump)
{
	constexpr static const char* stringPrefix = "#str_wolfstone_";
	constexpr static const uint8_t utf8ByteOrderMark[3] = {0xEF, 0xBB, 0xBF};

	const char* langData = static_cast<char*>(langLump->CacheLump());
	if(langLump->LumpSize >= sizeof(utf8ByteOrderMark) && memcmp(langData, utf8ByteOrderMark, sizeof(utf8ByteOrderMark)) == 0)
		langData += sizeof(utf8ByteOrderMark);
	Scanner sc{langData, static_cast<size_t>(langLump->LumpSize)};
	langLump->ReleaseCache();

	FString out = FString("[") + langCode + "]\n";
	sc.MustGetToken('{');
	while(!sc.CheckToken('}'))
	{
		sc.MustGetToken(TK_StringConst);
		FString key = sc->str;

		sc.MustGetToken(TK_StringConst);
		FString value = sc->str;

		if(auto remap = LangMap.find(key.GetChars()); remap != LangMap.end())
		{
			key = remap->second;
		}
		else if(key.Left(strlen(stringPrefix)).Compare(stringPrefix) == 0)
		{
			key.ToUpper();
			key = FString("STR_") + key.Mid(strlen(stringPrefix));
		}
		else
			continue;

		out += key + " = \"" + Scanner::Escape(value) + "\";\n";
	}

	return out + "\n";
}

static std::unique_ptr<FResourceLump> BuildLanguage(const ResourceCollection &resFiles)
{
	std::map<std::string, FResourceLump*> stringsLumps;
	for(auto iter = resFiles.rbegin(); iter != resFiles.rend(); ++iter)
	{
		for(unsigned int i = 0; i < (*iter)->LumpCount(); ++i)
		{
			auto lump = (*iter)->GetLump(i);
			if(lump->FullName.Left(8).Compare("strings/") == 0)
			{
				std::string lang = lump->FullName.Mid(8, lump->FullName.LastIndexOf(".")-8).GetChars();
				if(stringsLumps.find(lang) == stringsLumps.end())
					stringsLumps[lang] = lump;
			}
		}
	}

	FString out;

	for(const auto [lang, lump] : stringsLumps)
	{
		// Not sure why this is in the strings/ directory
		if(lang == "shadowplay")
			continue;

		if(auto langCode = LanguageCodes.find(lang); langCode != LanguageCodes.end())
		{
			try {
				out += ConvertLanguage(langCode->second, lump);
			} catch(...) {
				printf("Failed to decode strings for %s\n", lang.c_str());
			}
		}
		else
		{
			printf("Skipping strings for %s as language code is unknown. This is probably a bug.\n", lang.c_str());
		}
	}

	return MemoryLump::FromString(out);
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

	auto langStrings = BuildLanguage(resFiles);

	auto ecwolfWl6 = game.App == FileSys::APP_WolfensteinII
		? BuildECWolfArchive(
			game, resFiles,
			langStrings.get(),
			resFiles.Find("sb_wolfstone.bnk"),
			resFiles.Find(language + "/sb_vo_wolfstone.bnk")
		)
		: BuildECWolfArchive(
			game, resFiles,
			langStrings.get(),
			resFiles.Find(language + "/wolfstone.bnk")
		);

	FZip zip;
	zip.SetDate(game.Date);
	zip.AddFile("ecwolf.wl6", ecwolfWl6.get());
	zip.AddFile("gamemaps.wl6", resFiles.Find("gamemaps.wl6"));
	zip.AddFile("maphead.wl6", resFiles.Find("maphead.wl6"));
	zip.AddFile("vgadict.wl6", resFiles.Find("vgadict.wl6"));
	zip.AddFile("vgahead.wl6", resFiles.Find("vgahead.wl6"));
	zip.AddFile("vgagraph.wl6", resFiles.Find("vgagraph.wl6"));
	zip.AddFile("vswap.wl6", resFiles.Find("vswap.wl6"));

	auto configLump = BuildConfig(game.Scores);
	zip.AddFile("config.wl6", configLump.get());

	// Add file for easy identification by contents
	auto idLump = MemoryLump::FromString(FString(game.Game) +"\n\nExtracted from " + game.Name + " by WolfstoneExtract " TOOL_VERSION "\n");
	zip.AddFile(FString(game.OutputName).Left(strlen(game.OutputName)-4) + ".txt", idLump.get());

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

static std::tuple<GameInfo, FString> PickGame(FString explicitPath)
{
	constexpr std::array<GameInfo, FileSys::NUM_STEAM_APPS> GameInfoTable
	{
		GameInfo{FileSys::APP_WolfensteinII, "Wolfenstein II", "Wolfstone 3D", "wolfstone.pk3", WolfstoneSoundNames, WolfstoneEpisodes, WolfstoneScores, LoadWolfensteinIIResources, WOLFII_DATE},
		GameInfo{FileSys::APP_WolfensteinYoungblood, "Wolfenstein: Youngblood", "Elite Hans: Die Neue Ordnung", "elitehans.pk3", EliteHansSoundNames, EliteHansEpisodes, EliteHansScores, LoadYoungbloodResources, YOUNGBLOOD_DATE}
	};

	if(explicitPath.IsNotEmpty())
	{
		if(File(explicitPath, "base/gameresources.resources").exists())
			return {GameInfoTable[0], explicitPath};
		else if(File(explicitPath, "base/gameresources_pc.resources").exists())
			return {GameInfoTable[1], explicitPath};
		else
			throw CFatalError("Explicitly provided path does not appear to contain the expected game files.");
	}

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
		for(unsigned int i = 0; i < candidates.Size(); ++i)
			printf("    %d: %s\n", i+1, std::get<0>(candidates[i]).Game);

		for(;;)
		{
			printf("? ");
			fflush(stdout);
			selection = getchar() - '1';
			if(selection == -1)
				throw CNoRunExit();

			if(selection >= 0 && static_cast<unsigned>(selection) < candidates.Size())
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

static void ShowHelp(Options&, const char* const *)
{
	puts(
		"Usage: wolfstoneextract [options]\n"
		"\n"
		"Options:\n"
		"    --help, -h : Displays this text.\n"
		"    --language, -l <name> : Use game data for specified language instead of\n"
		"                            English or whatever language may be installed.\n"
		"    --path, -p <path> : Path to installed game skipping Steam detection.\n"
	);
	throw CNoRunExit();
}

static Options ParseOptions(int argc, const char* const * argv)
{
	struct OptHandlers
	{
		const char* name;
		char shortCode;
		int args;
		void (*handler)(Options &opt, const char* const * argv);
	};

	const std::array<OptHandlers, 3> handlers {
		OptHandlers{"help", 'h', 0, ShowHelp},
		OptHandlers{
			"language", 'l', 1, [](Options &opts, const char* const * argv){
				opts.Language = argv[0];
			}
		},
		OptHandlers{
			"path", 'p', 1, [](Options &opts, const char* const * argv){
				opts.Path = argv[0];
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
			i += it->args;
		}
		else
		{
			char name = argv[i][1];
			auto it = std::find_if(handlers.cbegin(), handlers.cend(), [name](OptHandlers const &opt) {
				return opt.shortCode && opt.shortCode == name;
			});

			if(it == handlers.end())
				throw CFatalError("Unknown command line switch");

			assert(it->args <= 1);

			const char* arg;
			if(it->args)
			{
				if(strlen(argv[i]) > 2)
					arg = argv[i]+2;
				else if(i + 1 < argc)
				{
					arg = argv[i+1];
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
	printf("Wolfstone Data Extraction Utility " TOOL_VERSION "\n");

	try
	{
		auto opts = ParseOptions(argc, argv);

		auto [ game, path ] = PickGame(opts.Path);
		Extract(game, path, opts.Language);
	}
	catch(CNoRunExit&) {}
	catch(CDoomError &error)
	{
		fprintf(stderr, "\nFAILED: %s\n", error.GetMessage());

#ifdef _WIN32
		// Don't automatically close the window on Windows
		fprintf(stderr, "An error has occured (press enter to dismiss)");
		fseek(stdin, 0, SEEK_END);
		getchar();
#endif
		return 1;
	}

	return 0;
}
