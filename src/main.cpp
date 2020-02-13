#include "doomerrors.h"
#include "files.h"
#include "filesys_steam.h"
#include "filesys.h"
#include "resourcefile.h"
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

	void addIn(ResourceCollection &&other)
	{
		for(auto &res : other)
			emplace_back(std::move(res));
	}
};

struct Options
{
	FString Language;
};

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
// language.  Although the Simplified Chinese install only provides the patched
// files English, the Wolfstone voices are untouched so it's recommended for a
// complete dump.
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

static ResourceCollection LoadResources(FString basePath, const std::vector<FString> &languages)
{
	ResourceCollection ret;

	ret.addIn(LoadPatchedResource(basePath + PATH_SEPARATOR "base" PATH_SEPARATOR "chunk_4.resources"));
	ret.addIn(LoadPatchedResource(basePath + PATH_SEPARATOR "base" PATH_SEPARATOR + soundsPath + PATH_SEPARATOR "sound.pack"));

	for(auto lang : languages)
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

		ret.addIn(std::move(collection));
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

	printf("Found languages (* = default):\n");
	for(auto l : languages)
		printf("  %c %s\n", l.Compare(language) == 0 ? '*' : '-', l.GetChars());

	auto resFiles = LoadResources(wolf2path, languages);

	printf("Extracting...\n");
	for(auto const& res : resFiles)
	{
		for(uint32_t i = 0;i < res->LumpCount();++i)
		{
			auto lump = res->GetLump(i);

			if(!File(File(lump->FullName).getDirectory()).makeDir())
				throw CFatalError("Could not create directory for writing out file.");

			auto f = fopen(lump->FullName, "w");
			auto cache = lump->CacheLump();
			fwrite(cache, lump->LumpSize, 1, f);
			fclose(f);
		}
	}
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
