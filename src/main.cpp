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

const FString soundsPath = "sound" PATH_SEPARATOR "soundbanks" PATH_SEPARATOR "pc";

//using ResourceCollection = std::vector<std::unique_ptr<FResourceFile>>;
struct ResourceCollection : std::vector<std::unique_ptr<FResourceFile>>
{
	using std::vector<std::unique_ptr<FResourceFile>>::vector;

	FResourceLump *Find(FString name) const
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

// Creates ecwolf.wl6 data from the sound banks
static std::unique_ptr<FResourceLump> BuildECWolfArchive(FResourceLump *baseSounds, FResourceLump *langSounds)
{
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

	// We need to keep these around until we can make the FZip::Build call.
	std::vector<std::unique_ptr<MemoryLump>> soundStorage;

	FZip archive;

	std::unique_ptr<FileReader> baseReader{baseSounds->NewReader()};
	std::unique_ptr<FileReader> langReader{langSounds->NewReader()};

	FSoundbank baseBank(baseReader.get());
	for(unsigned int i = 0; i < baseBank.Sounds.Size(); ++i)
	{
		char name[32];
		snprintf(name, 32, "sounds/snd%05u.ogg", i);
		archive.AddFile(name, soundStorage.emplace_back(std::make_unique<MemoryLump>(std::move(baseBank.Sounds[i].Data))).get());
	}

	FSoundbank langBank(langReader.get());
	for(unsigned int i = 0; i < langBank.Sounds.Size(); ++i)
	{
		char name[32];
		snprintf(name, 32, "sounds/lang%04u.ogg", i);
		archive.AddFile(name, soundStorage.emplace_back(std::make_unique<MemoryLump>(std::move(langBank.Sounds[i].Data))).get());
	}

	return std::make_unique<MemoryLump>(archive.Build());
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
		auto rf = FResourceFile::OpenResourceFile(file.getPath(), nullptr);
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

static ResourceCollection LoadResources(FString basePath, FString lang)
{
	ResourceCollection ret;

	ret.AddIn(LoadPatchedResource(basePath + PATH_SEPARATOR "base" PATH_SEPARATOR "chunk_4.resources"));
	ret.AddIn(LoadPatchedResource(basePath + PATH_SEPARATOR "base" PATH_SEPARATOR + soundsPath + PATH_SEPARATOR "sound.pack"));

	// At one point I tried to load all the language packs, but found all the
	// sounds to be identical so no point.  No harm in keeping the extra code
	// to support multiple languages around though.
	{
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
	return ret;
}

static void Extract(FString language)
{
	auto wolf2path = FileSys::GetSteamPath();
	if(wolf2path.IsEmpty())
		throw CFatalError("Could not find installed Wolfenstein II game data.");

	printf("Wolfenstein II path: %s\n", wolf2path.GetChars());

	auto languages = DetectLanguages(wolf2path);

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

	auto resFiles = LoadResources(wolf2path, language);

	printf("Extracting...\n");

	auto ecwolfWl6 = BuildECWolfArchive(
		resFiles.Find("sb_wolfstone.bnk"),
		resFiles.Find(language + "/sb_vo_wolfstone.bnk")
	);

	FZip zip;
	zip.AddFile("ecwolf.wl6", ecwolfWl6.get());
	zip.AddFile("gamemaps.wl6", resFiles.Find("gamemaps.wl6"));
	zip.AddFile("maphead.wl6", resFiles.Find("maphead.wl6"));
	zip.AddFile("vgadict.wl6", resFiles.Find("vgadict.wl6"));
	zip.AddFile("vgahead.wl6", resFiles.Find("vgahead.wl6"));
	zip.AddFile("vgagraph.wl6", resFiles.Find("vgagraph.wl6"));
	zip.AddFile("vswap.wl6", resFiles.Find("vswap.wl6"));

	if(auto f = File("wolfstone.pk3").open("w"))
	{
		auto zipData = zip.Build();
		fwrite(zipData.data(), zipData.size(), 1, f);
		fclose(f);
	}
	else
		throw CFatalError("Couldn't open output file for writing");

	printf("Done!\n");
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

			it->handler(opts, argv + i);
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

		Extract(opts.Language);
	}
	catch(CDoomError &error)
	{
		fprintf(stderr, "\nFAILED: %s\n", error.GetMessage());
		return 1;
	}
	return 0;
}
