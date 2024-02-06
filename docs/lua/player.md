# Player

Represents a player.

## Attributes

| Name | Type | Writable | Description |
| --- | --- | --- | --- |
| mo | Mobj | N | The player's corresponding Mobj |

## Methods

Call these like so: `playerVariable:method(...)`.

`nil call(Pspr pspr, string cptr, int/nil arg1, int/nil arg2, ...)`: Call a DeHackEd codepointer for weapons (`Pspr`), optionally passing MBF21 arguments.

`boolean getCheat(cheat_t cheat)`: Checks if a given cheat is enabled.

`int getPower(powertype_t power)`: Returns the given powerup's tic counter. Usually, a value of 0 means the player doesn't have the powerup, and a positive value indicates the time remaining in tics (1/35ths of a second), but this varies between powerups.
