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
| meleerange | int | N | Melee attack range. |
| momx | fixed_t | Y | X component of velocity in global coordinates. |
| momy | fixed_t | Y | Y component of velocity in global coordinates. |
| momz | fixed_t | Y | Z component of velocity in global coordinates. |
| angle | angle_t | Y | Orientation |
| x | fixed_t | Y (using `setPos`) | X position in global coordinates. |
| y | fixed_t | Y (using `setPos`) | Y position in global coordinates. |
| z | fixed_t | Y (using `setPos`) | Z position in global coordinates. |

## Methods

Call these like so: `mobjVariable:method(...)`.

`nil call(string cptr, int/nil arg1, int/nil arg2, ...)`: Call a DeHackEd codepointer, optionally passing MBF21 arguments.

`nil callMisc(string cptr, int/nil misc1, int/nil misc2)`: Call a DeHackEd codepointer, passing arguments through the Misc1/Misc2 fields of the state (as required by some MBF codepointers).

`boolean checkSight(Mobj dest)`: Checks whether `dest` is in line-of-sight of the calling Mobj.

`nil takeDamage(Mobj inflictor, Mobj source, int damage)`: Damages the Mobj. This is preferable to reducing the `health` attribute directly, as that doesn't check for death, pain states, etc. `inflictor` is the Mobj that hurt the calling one, e.g. a projectile. `source` is the Mobj responsible for the damage, and the one that'll be targeted by monsters.

`nil radiusAttack(Mobj source, int damage, int distance)`: Create an explosion at the calling actor's location.

`Mobj spawnMissile(Mobj dest, int type)`: Spawns a projectile at the calling actor's location, aimed at `dest`, and returns it. `type` is the DeHackEd thing number of the projectile to spawn.

`nil setPos(fixed_t/nil x, fixed_t/nil y, fixed_t/nil z)`: Sets the Mobj's position. You can use `nil` for coordinates you don't want to change.

`fixed_t aimLineAttack(angle_t angle, fixed_t/nil distance, boolean/nil skipFriends)`: Does vertical autoaim for a specified angle, maximum distance (default max distance if `nil`), and optionally ignoring friend Mobjs. Returns the resulting vertical slope.

`nil lineAttack(angle_t angle, fixed_t/nil distance, fixed_t slope, int damage)`: Does an hitscan attack with a specified angle, maximum distance, vertical slope, and damage.
