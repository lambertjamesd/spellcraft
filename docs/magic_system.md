
# Magic System

The following symbols will be usable by the player 

* fire
* projectile
* push
* recast
* shield
* reverse
* target
* up
* time dialaiton

## Fire

This symbol will spit out fire. Reversse + Up + Fire will cast fire around the player. Use reverse fire to emit ice. Combine ice and fire to create lightning.

## Projectile

This will throw a projectile forward. The projectile can be imbuied with an element or chained into other spell effects by putting symbols after the projectil

You can also trigger an alternate effect by chaining symbols below and to the right of the projectile symbol

```
O
 X
```

## Push

This acts as a dash when applied to the player but can also be used to move other objects. If you apply fire to the dash it will do a quick bursty dash. Applying ice will allow the player to move over water. Apply lighting to teleport a short distance.

Pushing up will cause the target to float in place. Doing an up fire push will cause the target to jump.

Reversing a push will pull instead.

## Recast

Allows spell effects to be delayed waiting for an additional player input. During that time the player can mash A to charge up the power of the next spell.

Reverse recast will continue casting a spell even if the player releases the spell button until they press the button a second time.

## Shield

Blocks damage of an enemy attack (or fall damage). If you time the shield to block right before taking damage you can chain an additional effect to trigger with power proportional to the damage that would have been taken effectively allowing the player to parry.

Some ideas for shield variants based on adding elements.

* Fire - Doesn't block damage unless the player parries but increases the power of the parry.
* Ice - Breaks on any hit but always parries. The parry has a fixed power instead of being proportional to the attack damage
* Lighting - Can block all damage but can't parry

A reverse sheild will put up a shield in the opposite direction.

## Reverse

Putting reverse before most other symbols will change what that sybmol does. A reverse reverse will delay for a small period of time

## Target

Modifies the direction of the next spell to be towards the nearest enemy. Reverse + Target will go in the direction of the player.

## Up

Modifies the direction of the next spell to point up. A reverse up will emit the spell around in all directions

## Time Dialation

This will speed up the player or can be used to speed up another target. A reverse time dialation will slow down the target.

## Effect Permutations

This table documents what effect each combination has

| Fire | Ice | Air | Life | Terminates | Effect |
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