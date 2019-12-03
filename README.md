# Wolfstone Data Extraction Utility

This utility extracts the game data for Wolfstone 3D from an installed copy of Wolfenstein II: The New Collosus. To use, simply install Wolfenstein II from Steam and run the utility. It should automatically find your game data and produce a wolfstone.pk3.

This utility does not modify the game data in any way, merely extracts the data from the larger data set and repacks it into a container recognizable by ECWolf.

ECWolf could be made to read the Wolfenstein II data files directly, but it is believed that most people would prefer extraction since 20MB of data is more portable than the 2.6GB of blobs that the game is contained in. Additionally since most of the data is in vanilla format having the data extracted is more useful for modding.

## Technical details

Wolfenstein II actually uses vanilla Wolfenstein 3D data archives (gamemaps, vgagraph, vswap) to provide the levels and graphics for Wolfstone 3D. They're located in chunk_4.resources which is in a custom container format identified with the "IDCL" fourcc. While it does not appear that anyone has fully reverse engineered this container format, we do know enough about it to extract the Wolfstone data. Thus the implementation here is incomplete but good enough.

The real trick is that the IDCL container uses the proprietary Kraken compression from the Oodle data compression suite by RAD game tools. Fortunately this bit stream has been reverse engineered, so it is feasible to extract using existing open source tools.

The other component to the game, sounds, are stored in AudioKinetic Wwise sound bank files inside IDCL containers. There's the universal sb_wolfstone.bnk inside sound.pack, and then a locale speific sb_vo_wolfstone.bnk inside the language pack (i.e. "english(us).pack"), which also appears in some of the patches to this file.

The format of these sound banks is well known, and contains Vorbis data which just needs to be transformed into a standard Ogg Vorbis container.

## References

* [Ooz (Kraken)](https://github.com/powzix/ooz)
* [Wwise sound bank extractor](https://github.com/eXpl0it3r/bnkextr)
* [Wwise WEM to Ogg](https://github.com/hcs64/ww2ogg)
* [ZenHAX thread on IDCL format](https://zenhax.com/viewtopic.php?t=5148)
