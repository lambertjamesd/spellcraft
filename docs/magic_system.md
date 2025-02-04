
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

This symbol will spit out fire. When used as a modifier with ice, it creates lightning. When used as a modifier with other runes, it changes the behavior to be short and bursty

## Ice

This symbol will spit out ice. When used as a modifier to fire, it creates lightning. When used as a modifier with wind, it will make things slippery while puhing. Not sure what it should do with other elements

## Earth

Earth will act a sheild. When combined with air, it acts as a projectile. Sheilds can chain into other spells to create a parry. Projectiles can trigger other spells upon hitting its target.

When used as a modifier, it will push elemental effects in all directions.

## Air

Air will act a dash for the player or can be used to push other objects. When used as modifier it reduces the angle but increases the range of elemental attacks.

## Life

Life will heal the player. When used as a modifier to elemental runes it will create a living version of the element that will seek out targets automatically.

## Effect Permutations

This table documents what effect each combination has

| Fire | Ice | Air | Life | Primary    | Effect |
|------|-----|-----|------|------------|--------|
|      |     |     |      | Fire       | Flame Fan |
|      |     |     |      | Earth      | Shield    |
|      |     |     |      | Ice        | Ice Fan   |
|      |     |     |      | Air        | Dash      |
|      |     |     |      | Life       | Heal      |
| X    |     |     |      | Fire       | Flame Circle |
| X    |     |     |      | Earth      | Fire Sheild  |
| X    |     |     |      | Ice        | Lightning   |
| X    |     |     |      | Air        | Fire Dash   |
| X    |     |     |      | Life       | Burst Heal  |
|      | X   |     |      | Fire       | Lightning |
|      | X   |     |      | Earth      | Snowman    |
|      | X   |     |      | Ice        | Ice Circle   |
|      | X   |     |      | Air        | Ice Dash      |
|      | X   |     |      | Life       | Slow Heal     |
| X    | X   |     |      | Fire       | Lightning Circle |
| X    | X   |     |      | Earth      | Lightning Sheild    |
| X    | X   |     |      | Ice        | Lightning Circle   |
| X    | X   |     |      | Air        | Lightning Dash      |
| X    | X   |     |      | Life       | Over Heal     |
|      |     | X   |      | Fire       | Fire Cone |
|      |     | X   |      | Earth      | Projectile    |
|      |     | X   |      | Ice        | Ice Cone   |
|      |     | X   |      | Air        | Push      |
|      |     | X   |      | Life       | ?  |
| X    |     | X   |      | Fire       | ? |
| X    |     | X   |      | Earth      | Rocket Projectile    |
| X    |     | X   |      | Ice        | Cone Lightning   |
| X    |     | X   |      | Air        | ?      |
| X    |     | X   |      | Life       | ?  |
|      | X   | X   |      | Fire       | Cone Lightning |
|      | X   | X   |      | Earth      | Snowball    |
|      | X   | X   |      | Ice        | ?   |
|      | X   | X   |      | Air        | ?      |
|      | X   | X   |      | Life       | ?  |
| X    | X   | X   |      | Fire       | ? |
| X    | X   | X   |      | Earth      | Lightning Projectile    |
| X    | X   | X   |      | Ice        | ?   |
| X    | X   | X   |      | Air        | ?  |
| X    | X   | X   |      | Life       | ? Zombie? |
|      |     |     | X    | Fire       | Fire Sprite |
|      |     |     | X    | Earth      | Land Mine    |
|      |     |     | X    | Ice        | Ice Sprite   |
|      |     |     | X    | Air        | Wind Sprite?  |
|      |     |     | X    | Life       | Heal target  |
| X    |     |     | X    | Fire       | Large Fire Sprite |
| X    |     |     | X    | Earth      | Fire Land Mine    |
| X    |     |     | X    | Ice        | Lightning Sprite   |
| X    |     |     | X    | Air        | ?  |
| X    |     |     | X    | Life       | Heal target  |
|      | X   |     | X    | Fire       | Lightning Sprite |
|      | X   |     | X    | Earth      | Frosty the snowman    |
|      | X   |     | X    | Ice        | Large Ice Sprite  |
|      | X   |     | X    | Air        | ?  |
|      | X   |     | X    | Life       | Slow heal target  |
| X    | X   |     | X    | Fire       | Large Lightning Sprite |
| X    | X   |     | X    | Earth      | Lightning mine  |
| X    | X   |     | X    | Ice        | Large Lightning Sprite   |
| X    | X   |     | X    | Air        | ?  |
| X    | X   |     | X    | Life       | Over heal target  |
|      |     | X   | X    | Fire       | Caster Fire Sprite |
|      |     | X   | X    | Earth      | Tracking projectile  |
|      |     | X   | X    | Ice        | Caster Ice Sprite   |
|      |     | X   | X    | Air        | Tracking push |
|      |     | X   | X    | Life       | ?  |
| X    |     | X   | X    | Fire       | Large Caster Fire Sprite |
| X    |     | X   | X    | Earth      | Tracking rocket projectile  |
| X    |     | X   | X    | Ice        | Caster Lightning Sprite   |
| X    |     | X   | X    | Air        | Tracking fire push? |
| X    |     | X   | X    | Life       | ?  |
|      | X   | X   | X    | Fire       | Caster Lightning Sprite |
|      | X   | X   | X    | Earth      | Tracking snowball  |
|      | X   | X   | X    | Ice        | Large Caster Ice Sprite    |
|      | X   | X   | X    | Air        | ? |
|      | X   | X   | X    | Life       | ?  |
| X    | X   | X   | X    | Fire       | Large Caster Lightning Sprite |
| X    | X   | X   | X    | Earth      | Tracking lightning ball  |
| X    | X   | X   | X    | Ice        | Large Caster Lightning Sprite |
| X    | X   | X   | X    | Air        | ? |
| X    | X   | X   | X    | Life       | ?  |

To figure out
* What does life -> air do?
* What does air -> life do? Life steal instead? Is that overpowered?
* What does having an element on a push do?
* What does adding push to a circle shape do?