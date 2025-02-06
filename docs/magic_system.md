
# Magic System

The following symbols will be usable by the player 

* fire
* ice
* earth
* air
* life

A spell is composed of rune groups. A rune group start with a single primary rune followed by other runes as modifiers. A rune can only show up once in a rune group.

Some spells can trigger other rune groups on a given event like when a rock hits its target. New groups can be created by tapping 'R'. An example of a fireball spell could be `earth+wind | fire` with

## Fire

This symbol will spit out fire. 

Effects of modifiers when life isn't a modifier

Fire can trigger another effect if it kills a target

* Ice - Turns the fire into lightning
* Earth - Reshapes the attack to be all around the player
* Air - Does a short bursty dash
* Air + Earth - Player spins in place swinging a large fire sword

When Life is a modifier

Sprites can trigger another effect upon hitting a target

* Ice - Turns into a lightning sprite
* Earth - Sprite attack because an AOE attack
* Air - Launches the sprite

## Ice

This symbol will spit out ice.

Effects of modifiers when life isn't a modifier

Fire can trigger another effect if it kills a target

* Fire - Turns the ice into lightning (I may end up having this be water instead)
* Earth - Reshapes the attack to be all around the player
* Air - Ice dash. Can be used until all mana is used up
* Earth + Air - Spin in a circle 

When Life is a modifier

Sprites can trigger another effect upon hitting a target

* Fire - Turns into a lightning sprite
* Earth - Sprite attack becomes an AOE
* Air - Launches the sprite

## Earth

Earth will act a sheild. Putting up a shield at the last moment can chain effects, creating a parry

* Fire - An indestrutable sheild. Cannot parry
* Ice - A fragile shield that always parrys, but does not tranfer attack damage to magic power
* Fire + Ice - A lightning shield that doesn't block damage but damages enemies on contact. Always parries

* Life - Create a stationary mine. Triggers chain effects when touched

When air is present, earth acts as a projectile instead. When the projectile hits a target it can trigger another effect

* Fire - Turns the projectile into a rocket, moving in a straight line
* Ice - A snowball. It doesn't do any damage but can be chained with other effects
* Fire + Ice - A fast moving sniper shot

Adding life makes the projectile automatically target the nearest enemy

## Air

Air creates a column of air to push away objects

* Earth - Pushes all enemies away from the player outward in a circle
* Fire - Wind push also lights things on fire
* Ice - Freezes and turns off friction for objects being pushed
* Fire + Ice - Teleports a short distance. Can teleport through some obstacles

When life is included

Creates a living wind sprite that triggers the wind spell on contact

## Life

Life will heal the player. When used as a modifier to elemental runes it will create a living version of the element that will seek out targets automatically.

When combined with

* Fire - Instant, but less effecient, heal
* Ice - Reverse heal that turns HP to mana
* Fire + Ice - Reverse instance life steal

* Earth - Over heals the player to make a temporary larger health bar
* Earth + Ice - Over heals the 

* Wind - Apply effect to others instead of the player

## Effect Permutations

This table documents what effect each combination has

Modifier shorthand

* F = Fire
* I = ice
* A = Air
* E = Earth
* L = Life

| Primary | Modifiers | Effect                           |
|---------|-----------|----------------------------------|
| Fire    |           | Shoot flames                     |
| Fire    | I         | Shoot lightning                  |
| Fire    | I A       | Lightning dash                   |
| Fire    | I A E     | Spin lightning sword             |
| Fire    | I A E L   | Shoot AEO lightning sprite       |
| Fire    | I A L     | Shoot lightning sprite           |
| Fire    | I E       | Lightning explosion              |
| Fire    | I E L     | AEO lightning sprite             |
| Fire    | I L       | Lightning sprite                 |
| Fire    | A         | Fire dash                        |
| Fire    | A E       | Spin fire sword or aeo fire dash |
| Fire    | A E L     | Shoot AEO fire sprite            |
| Fire    | A L       | Shoots a fire sprite             |
| Fire    | E         | AEO explosion                    |
| Fire    | E L       | AEO fire sprite                  |
| Fire    | L         | Fire sprite                      |
| Ice     |           | Shoot ice                        |
| Ice     | F         | Shoot lightning (maybe water)    |
| Ice     | F A       | Lightning dash                   |
| Ice     | F A E     | Spin lightning sword             |
| Ice     | F A E L   | Shoot AEO lightning sprite       |
| Ice     | F A L     | Shoot lightning sprite           |
| Ice     | F E       | Lightning explosion              |
| Ice     | F E L     | AEO lightning sprite             |
| Ice     | F L       | Lightning sprite                 |
| Ice     | A         | Ice dash                         |
| Ice     | A E       | Spin ice sword or aeo ice dash   |
| Ice     | A E L     | Shoot AEO ice sprite             |
| Ice     | A L       | Shoots a ice sprite              |
| Ice     | E         | AEO explosion                    |
| Ice     | E L       | AEO ice sprite                   |
| Ice     | L         | Ice sprite                       |
| Earth   |           | Shield                           |
| Earth   | F         | Fire sheild                      |
| Earth   | F I       | Lightning sheild                 |
| Earth   | F I A     | Lightning sniper rock            |
| Earth   | F I A L   | Homing lightning sniper rock     |
| Earth   | F I L     | Lightning ground mine            |
| Earth   | F A       | Rocket projectile                |
| Earth   | F A L     | Homing rocket projectile         |
| Earth   | F L       | Fire mine                        |
| Earth   | I         | Ice shield                       |
| Earth   | I A       | Snowball                         |
| Earth   | I A L     | Homing snowball                  |
| Earth   | I L       | Frosty the snowman               |
| Earth   | A         | Projectile                       |
| Earth   | A L       | Homing projectile                |
| Earth   | L         | Ground mine                      |
| Air     |           | Push                             |
| Air     | F         | Burning push                     |
| Air     | F I       | Lightning push (teleports)       |
| Air     | F I E     | circular push (teleports)        |
| Air     | F I E L   | sprite that telepushes outward   |
| Air     | F I L     | sprite that telepushes forward   |
| Air     | F E       | burning push outward             |
| Air     | F E L     | sprite that burn pushes outward  |
| Air     | F L       | sprite that burn pushes forward  |
| Air     | I         | icy push                         |
| Air     | I E       | icy push outward                 |
| Air     | I E L     | sprite that icy pushes outward   |
| Air     | I L       | sprite that icy pushes forward   |
| Air     | E         | push outward                     |
| Air     | E L       | sprite that pushes outward       |
| Air     | L         | sprite that pushes forward       |
| Life    |           | healing                          |
| Life    | F         | instant aeo healing              |
| Life    | F I       | instant convert health to mana   |
| Life    | F I A     | instant life steal sprite        |
| Life    | F I A E   | instant aeo life steal sprite    |
| Life    | F I E     | aeo life steal                   |
| Life    | F A       | instant healing sprite           |
| Life    | F A E     | instant healing aeo sprite       |
| Life    | F E       | instant aeo healing              |
| Life    | I         | convert health to mana           |
| Life    | I A       | create mana steal sprite         |
| Life    | I A E     | create aeo mana steal sprite     |
| Life    | I E       | aeo mana steal                   |
| Life    | A         | healing sprite                   |
| Life    | A E       | healing aeo sprite               |
| Life    | E         | aeo heal                         |
