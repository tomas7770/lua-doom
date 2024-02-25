# Mobj

Represents a thing/actor that exists in the game world.

## Attributes

| Name | Type | Writable | Description |
| --- | --- | --- | --- |
| health | int | Y | Current health |
| spawnhealth | int | N | Initial health when Mobj is spawned |
| target | Mobj or nil | Y | For monsters, the Mobj they want to attack. For projectiles, the Mobj who shot them. |
| tracer | Mobj or nil | Y | Used by some projectiles, e.g. Revenant missile, to track a Mobj. |
| mass | int | N | Mass, affects physics. |
| meleerange | number | N | Melee attack range. |
| radius | number | N | Mobj radius. |
| height | number | N | Mobj height. |
| momx | number | Y | X component of velocity in global coordinates. |
| momy | number | Y | Y component of velocity in global coordinates. |
| momz | number | Y | Z component of velocity in global coordinates. |
| angle | number | Y | Orientation |
| x | number | Y (using `setPos`) | X position in global coordinates. |
| y | number | Y (using `setPos`) | Y position in global coordinates. |
| z | number | Y (using `setPos`) | Z position in global coordinates. |
| flags | int | Y | Actor flags as a bitmask. Read about `mobjflag_t` [here](luahack.md) for more info. |
| flags2 | int | Y | MBF21 actor flags as a bitmask. Read about `mobjflag2_t` [here](luahack.md) for more info. |
| type | dmobjtype_t | N | DeHackEd thing number of the Mobj. |
| movedir | int | Y |  |
| movecount | int | Y |  |
| strafecount | int | Y |  |
| reactiontime | int | Y |  |
| threshold | int | Y |  |
| pursuecount | int | Y |  |
| spawnstate | int | N |  |
| seestate | int | N |  |
| seesound | int | N |  |
| spawnreactiontime | int | N |  |
| attacksound | int | N |  |
| painstate | int | N |  |
| painchance | int | N |  |
| painsound | int | N |  |
| meleestate | int | N |  |
| missilestate | int | N |  |
| deathstate | int | N |  |
| xdeathstate | int | N |  |
| deathsound | int | N |  |
| speed | int | N |  |
| damage | int | N |  |
| activesound | int | N |  |
| raisestate | int | N |  |
| infighting_group | int | N |  |
| projectile_group | int | N |  |
| splash_group | int | N |  |
| projectile_group | int | N |  |
| ripsound | int | N |  |
| altspeed | int | N |  |
| droppeditem | dmobjtype_t | N |  |

## Methods

Call these like so: `mobjVariable:method(...)`.

`nil call(string cptr, int/nil arg1, int/nil arg2, ...)`: Call a DeHackEd codepointer, optionally passing MBF21 arguments.

`nil callMisc(string cptr, int/nil misc1, int/nil misc2)`: Call a DeHackEd codepointer, passing arguments through the Misc1/Misc2 fields of the state (as required by some MBF codepointers).

`boolean checkSight(Mobj dest)`: Checks whether `dest` is in line-of-sight of the calling Mobj.

`nil takeDamage(Mobj inflictor, Mobj source, int damage)`: Damages the Mobj. This is preferable to reducing the `health` attribute directly, as that doesn't check for death, pain states, etc. `inflictor` is the Mobj that hurt the calling one, e.g. a projectile. `source` is the Mobj responsible for the damage, and the one that'll be targeted by monsters.

`nil radiusAttack(Mobj source, int damage, int distance)`: Create an explosion at the calling actor's location.

`Mobj spawnMissile(Mobj dest, dmobjtype_t type)`: Spawns a projectile at the calling actor's location, aimed at `dest`, and returns it. `type` is the DeHackEd thing number of the projectile to spawn.

`nil setPos(number/nil x, number/nil y, number/nil z)`: Sets the Mobj's position. You can use `nil` for coordinates you don't want to change.

`number aimLineAttack(number angle, number/nil distance, boolean/nil skipFriends)`: Does vertical autoaim for a specified angle, maximum distance (default max distance if `nil`), and optionally ignoring friend Mobjs. Returns the resulting vertical slope.

`nil lineAttack(number angle, number/nil distance, number slope, int damage)`: Does an hitscan attack with a specified angle, maximum distance, vertical slope, and damage.

`boolean checkRange(number range)`: Checks if the Mobj's `target` is within a specified attack range.

`boolean checkMeleeRange()`: Check if the Mobj's `target` is within melee range.

`boolean getFlag(mobjflag_t flag)`: Gets the value of a non-MBF21 flag. More friendly alternative to reading the `flags` attribute directly.

`boolean getFlag2(mobjflag2_t flag)`: Gets the value of a MBF21 flag. More friendly alternative to reading the `flags2` attribute directly.

`nil setFlag(mobjflag_t flag, boolean value)`: Sets the value of a non-MBF21 flag. More friendly alternative to changing the `flags` attribute directly.

`nil setFlag2(mobjflag2_t flag, boolean value)`: Sets the value of a MBF21 flag. More friendly alternative to changing the `flags2` attribute directly.

`nil setState(int state)`
