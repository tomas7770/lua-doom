# LUAHACK

LUAHACK is the lump at the heart of LuaDoom's functionality. It's where all Lua code is written.

Make sure to read the [Don't](#dont) section for things you shouldn't do.

## Defining new DeHackEd codepointers

Using LUAHACK, it's possible to define new codepointers as Lua functions, for use in DeHackEd lumps. See the [Global functions](#global-functions) section for more info.

## Global functions

`nil registerCodepointer(string name, function f)`: Registers a Lua function for use as a DeHackEd codepointer.

 - `string name`: Name to use in the DEHACKED lump.

 - `function f`: Lua function to be called as codepointer. For actor functions, takes 1 argument of type `Mobj`. For weapons functions, takes a `Player` followed by a `Pspr`. Any other arguments have their value set to MBF21 arguments defined in the DEHACKED.

`nil registerMobjMethod(string name, function f)`: Registers a Lua function for use as a `Mobj` method. This is so that you can call the function with the syntax `mobjVariable:name()` instead of `name(mobjVariable)`.

`nil registerPlayerMethod(string name, function f)`: Same as `registerMobjMethod`, but for `Player`.

`boolean isMobj(any value)`: Checks if a given value is a `Mobj`.

`int tofixed(number n)`: Converts the given number into 16.16 bit fixed-point format, i.e. a normal 32-bit integer, but with a different "scale" (65536 represents 1.0). Some MBF21 codepointers expect arguments in this format.

`number tan(number n)`
`number sin(number n)`
`number cos(number n)`: Trigonometric functions that use Doom's finite tables. Angle in degrees.

`number pointToAngle(number x1, number y1, number x2, number y2)`: Returns an angle formed by the line starting at (x1, y1) and ending at (x2, y2).

`int random()`: Random function that uses Doom's finite PRNG table.

`Mobj spawnMobj(dmobjtype_t type, number x, number y, number z)`: Spawns a Mobj (thing) and returns it.

- `dmobjtype_t type`: DeHackEd thing number of the thing to spawn.

- `number x, y, z`: Coordinates to spawn the thing at.

`skill_t getGameSkill()`: Returns the current game skill level.

`number aproxDistance(int dx, int dy)`: Returns an approximate length of a vector (dx, dy), as done by Doom's original functions.

`iterator getMobjs()`: Returns an iterator for getting all Mobjs currently present in the game world. The most common use of this is with a `for` loop:

```lua
for mobj in getMobjs() do
    -- Do something
end
```

## Types

Some new types are provided to Lua to manipulate the game environment.

[Mobj](mobj.md)

[Player](player.md)

Pspr

## Pseudo-types

These aren't really new types, just a different interpretation of existing ones. As a result, for functions that take those as arguments, their corresponding real type is valid as well. They may be replaced by real types at some point, though.

`skill_t (int)`: Game skill level. The following **global constants** exist for this:

- `SK_BABY`: I'm too young to die.

- `SK_EASY`: Hey, not too rough.

- `SK_MEDIUM`: Hurt me plenty.

- `SK_HARD`: Ultra-Violence.

- `SK_NIGHTMARE`: Nightmare!

`cheat_t (int)`: Cheat code identifier. The following **global constants** exist for this:

- `CHEATS_NOCLIP`

- `CHEATS_GOD`

- `CHEATS_NOMOMENTUM`

- `CHEATS_BUDDHA`

- `CHEATS_NOTARGET`

`powertype_t (int)`: Powerup identifier. The following **global constants** exist for this:

- `POWERS_INVUL`: Invulnerability

- `POWERS_BERSERK`: Berserk

- `POWERS_INVIS`: Partial invisibility

- `POWERS_RAD`: Radiation shielding suit

- `POWERS_MAP`: Computer area map

- `POWERS_LIGHT`: Light amplification visor

`mobjflag_t (int)`: Actor flag. You can find a list [here](/src/p_mobj.h#L108). **Global constants** exist in the same format as the list:

- `MF_SPECIAL`

- `MF_SOLID`

- etc.

`mobjflag2_t (int)`: MBF21 actor flag. You can find a list [here](/src/p_mobj.h#L202). **Global constants** exist in the same format as the list.

`dmobjtype_t (int)`: DeHackEd thing number. You can find a list [here](/src/info.h#L1294). **Global constants** exist in the same format as the list.

## Don't...

- Use global variables for game logic. These persist after starting a new game or loading a save file. Best case scenario, you break demo/multiplayer support. Worst case scenario, the game bugs out unpredictably or crashes.

- Use `math.random`. It's incompatible with demos/multiplayer. Use the global `random()` function instead.

- Use `math.sin`, `math.cos`, `math.tan`, or other trigonometric functions from the Lua `math` library. Compatibility with demos/multiplayer isn't guaranteed, and you'd need to convert the angle to radians anyway. Use the global `sin`, `cos`, and `tan` functions instead.

- Modify pre-existing global functions or constants.
