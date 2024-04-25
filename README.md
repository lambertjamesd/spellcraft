# Spellcraft

A new original N64 game

## Building

Use [libdragon](https://github.com/DragonMinded/libdragon) to build the project. You should use the preview branch to get opengl (sha bcfddb30fc08784d3e537da1274cbe95fdf1e4b0)

You need to specify the location of blender 4.0.2 using en enviroment variable

```
export BLENDER_4=/home/james/Blender/blender-4.0.2-linux-x64/blender
```
replace the location of blender where it is installed on your machine.

You will also need python installed. I have version 3.10.12 installed.

After that it should only take running `make` in the root directory of this project

## TODO

- build out puzzle elements
-  texture scrolling
-  transition to using t3d
-  build out spell system some more
-  switch
-  collectables
- build out enemy
- flesh out spell casting
-  camera and controls during remote control spell
- spells
-  fire push modifier