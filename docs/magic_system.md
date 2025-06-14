
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

Progress indiciators

* F = Basic function
* M = Consumes mana
* T = Has trigger

| Primary | Modifiers | Effect                           | Trigger          | Progress  |
|---------|-----------|----------------------------------|------------------|-----------|
| Fire    |           | Shoot flames                     | On kill          | F         |
| Fire    | I         | Shoot lightning                  | On kill          | F         |
| Fire    | I A       | Lightning push (teleports)       | On dash through  | F         |
| Fire    | I A E     | Lightning push out (teleports)   | On impact        | F         |
| Fire    | I A E L   | Shoot AEO lightning sprite       | On sprite attack | F         |
| Fire    | I A L     | Shoot lightning sprite           | On sprite attack | F         |
| Fire    | I E       | Lightning explosion              | On kill          | F         |
| Fire    | I E L     | AEO lightning sprite             | On sprite attack | F         |
| Fire    | I L       | Lightning sprite                 | On sprite attack | F         |
| Fire    | A         | Fire push                        |                  | F M       |
| Fire    | A E       | Fire puh outward                 |                  | F         |
| Fire    | A E L     | Shoot AEO fire sprite            | On sprite attack | F         |
| Fire    | A L       | Shoots a fire sprite             | On sprite attack | F         |
| Fire    | E         | AEO explosion                    | On kill          | F         |
| Fire    | E L       | AEO fire sprite                  | On sprite attack | F         |
| Fire    | L         | Fire sprite                      | On sprite attack | F         |
| Ice     |           | Shoot ice                        | On ice shatter   | F         |
| Ice     | F         | Shoot water                      | On contact       | F         |
| Ice     | F A       | Water push                       |                  | F M       |
| Ice     | F A E     | Water puh outward                |                  | F         |
| Ice     | F A E L   | Shoot AEO water sprite           | On sprite attack | F         |
| Ice     | F A L     | Shoot water sprite               | On sprite attack | F         |
| Ice     | F E       | Water explosion                  | On contact       | F         |
| Ice     | F E L     | AEO water sprite                 | On sprite attack | F         |
| Ice     | F L       | Water sprite                     | On sprite attack | F         |
| Ice     | A         | Ice push                         | On impact        | F M       |
| Ice     | A E       | Ice push outward                 | On ice shatter   | F M       |
| Ice     | A E L     | Shoot AEO ice sprite             | On ice shatter   | F         |
| Ice     | A L       | Shoots a ice sprite              | On ice shatter   | F         |
| Ice     | E         | AEO explosion                    | On ice shatter   | F         |
| Ice     | E L       | AEO ice sprite                   | On ice shatter   | F         |
| Ice     | L         | Ice sprite                       | On ice shatter   | F         |
| Earth   |           | Shield                           | On parry         | F         |
| Earth   | F         | Fire sheild                      | On parry         | F         |
| Earth   | F I       | Lightning sheild                 | On taking damage |           |
| Earth   | F I A     | Lightning sniper rock            | On impact        | F         |
| Earth   | F I A L   | Homing lightning sniper rock     | On impact        |           |
| Earth   | F I L     | Lightning ground mine            | On touch         |           |
| Earth   | F A       | Rocket projectile                | On impact        | F         |
| Earth   | F A L     | Homing rocket projectile         | On impact        | F         |
| Earth   | F L       | Fire mine                        | On touch         |           |
| Earth   | I         | Ice shield                       | On parry         | F         |
| Earth   | I A       | Snowball                         | On impact        | F         |
| Earth   | I A L     | Homing snowball                  | On impact        | F         |
| Earth   | I L       | Frosty the snowman               | On touch         |           |
| Earth   | A         | Projectile                       | On impact        | F T       |
| Earth   | A L       | Homing projectile                | On impact        | F         |
| Earth   | L         | Ground mine                      | On touch         |           |
| Air     |           | Dash                             |                  | F         |
| Air     | F         | Fire dash                        | On dah end       | F         |
| Air     | F I       | Lightning dash (teleports)       | On teleport end  | F         |
| Air     | F I E     | Lightning jump (teleports)       | On teleport end  | F         |
| Air     | F I E L   | sprite that telepushes outward   |                  | F         |
| Air     | F I L     | sprite that telepushes forward   |                  | F         |
| Air     | F E       | Fire jump                        |                  | F         |
| Air     | F E L     | sprite that burn pushes outward  |                  | F         |
| Air     | F L       | sprite that burn pushes forward  |                  | F         |
| Air     | I         | ice dash                         |                  | F         |
| Air     | I E       | instant stop                     |                  | F         |
| Air     | I E L     | sprite that icy pushes outward   | On ice shatter   |           |
| Air     | I L       | sprite that icy pushes forward   | On ice shatter   |           |
| Air     | E         | jump                             |                  | F         |
| Air     | E L       | sprite that pushes outward       |                  | F         |
| Air     | L         | sprite that pushes forward       |                  | F         |
| Life    |           | healing                          |                  | F         |
| Life    | F         | instant                          |                  | F         |
| Life    | F I       | instant convert health to mana   |                  | F         |
| Life    | F I A     | shoot life steal sprite          |                  | F         |
| Life    | F I A E   | shoot aeo life steal sprite      |                  | F         |
| Life    | F I E     | aeo life steal                   |                  | F         |
| Life    | F A       | shoot healing sprite             |                  | F         |
| Life    | F A E     | shoot healing aeo sprite         |                  | F         |
| Life    | F E       | instant aeo healing              |                  | F         |
| Life    | I         | convert health to mana           |                  | F         |
| Life    | I A       | create mana steal sprite         | On mana steal    | F         |
| Life    | I A E     | create aeo mana steal sprite     | On mana steal    | F         |
| Life    | I E       | aeo mana steal                   | On mana steal    | F         |
| Life    | A         | healing sprite                   |                  | F         |
| Life    | A E       | healing aeo sprite               |                  | F         |
| Life    | E         | aeo heal                         |                  | F         |
