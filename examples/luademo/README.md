# [LuaDoom demo]

## What This Is

This is a project created by DoomMake. It's a showcase of the Lua scripting capabilities
of this source port.


## Generated by the Project

Some directories/files in this project are ignored by repository "ignore" files:

	/build               Build directory.
	/dist                Distributables directory.
	doommake.properties  Project property override file.


## Before You Build This Project

This project may need some local properties filled in by whoever clones this project.
Make a `doommake.properties` file and look at the properties in `PROPS.txt` for the
modifiable settings for this project. The `doommake.properties` file that you create should
NOT BE CHECKED IN TO THE REPOSITORY!


## To Build This Project


This project requires the [DoomTools](https://github.com/MTrop/DoomTools) toolchain for
building it. Clone this project to a new folder and type:

	doommake init


...In order to ensure everything has been prepped correctly (some directories or files
may need extracting, downloading, or copying).

To clean the build folder, type:

	doommake clean


To both initialize and build this project and its distributable archive:

	doommake


To build the DeHackEd patch from DECOHack source:

	doommake patch


To convert raw assets to Doom assets:

	doommake convert


To build the assets WAD:

	doommake assets


To build just the maps WAD:

	doommake maps


To extract only the textures used by maps to a separate WAD file (if any):

	doommake maptextures


To build all components:

	doommake all


To build the full release:

	doommake release

