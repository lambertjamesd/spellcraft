# Spellcraft

A new original N64 game

## Building

Use [libdragon](https://github.com/DragonMinded/libdragon) to build the project.
```sh
git clone --recurse-submodules https://github.com/lambertjamesd/spellcraft
# or to init the submodule (after default clone)
git submodule update --init --recursive
# or to update the content of the submodules to the pinned libdragon sha
git submodule update --remote --recursive
```


You need to specify the location of blender 4.5.8 using en enviroment variable

```
export BLENDER_4=/home/james/Blender/blender-4.5.8-linux-x64/blender
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
* Ranged attacker
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

- implement crushing check
- prevent z lock on held object
- refactor how the animator works
- include material/image dependencies in asset dependencies
- create fixed point camera regions
- direct camera control
- load on background thread
- particle rendering in mesh
- camera logic
- do a better job with camera tracking logic
- assert(found_index != -1); inside of collision_scene.c was triggered
- crash debug_collider_disable - callback_list_remove

## Scripting TODO

- function imports

## Fire trails todo

* Make it look better

## Short ideas

* Crush check logic
* Sloped surface check

## Setup the blender plugin

```
cmd /c mklink /D "C:\Users\YourUsername\.config\nvim" "\\wsl$\Ubuntu\home\username\.config\nvim"
```
