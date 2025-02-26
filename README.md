# Spellcraft

A new original N64 game

## Building

Use [libdragon](https://github.com/DragonMinded/libdragon) to build the project. You should use the preview branch to get opengl (sha 1559f4a02828508bf31d6c63252f20d887f32be8)

You need to specify the location of blender 4.0.2 using en enviroment variable

```
export BLENDER_4=/home/james/Blender/blender-4.0.2-linux-x64/blender
```
replace the location of blender where it is installed on your machine.

You will also need python installed. 

```
sudo apt install python3
```

I have version 3.12.3 installed.

After that it should only take running `make` in the root directory of this project
### JQ
Building requires jq
```
sudo apt install jq
```

## Docker
You can optionally build the project with docker. Build the container
```
docker build -t spellcraft-dev-env .
```
Run the container
```
docker run -v ${pwd}:/spellcraft -it spellcraft-dev-env
```
Run Make
```
make
```

## TODO

- open world rendering
-  subdivide mesh into pieces
- figure out if player model could be rendered more effeciently
- fix globals.dat build issues
- camera logic
-  do a better job with camera tracking logic
-  implement automatic camera framing when talking to npc
- build out puzzle elements
-  build out spell system some more
-  switch
- build out enemy
- spells
-  spell shape variants
-  shield
-  sprite
-  tracking projectile