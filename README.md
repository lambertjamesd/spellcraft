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

## Enemy ideas

* Stone golem
* Something that flies
* Something fast
* Big biter
* Something underground
* Magic user
* Fighter (weakness is parry)
* Enemies that are individually weak but come in groups
* Single difficult fight
* Something immune to life steal (zombie maybe)
* A support enemy that strengthens allys

### Enemy attack ideas

* Attack that spread on the ground, jumping can dodge
* Punches, sword swings (can parry)
* Ranged attacks (Parry could fire projectiles straight back)

### Spell ideas

Create a decoy
Have earth + life be recast

### minigame ideas

"basketball"

## TODO

- z targeting
- direct camera control
- particle rendering
- figure out if player model could be rendered more effeciently
- camera logic
-  do a better job with camera tracking logic
-  implement automatic camera framing when talking to npc