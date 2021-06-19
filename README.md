# Building the library only
`make lib/hchess-lib.o`

# Building the example cli
`make out/cli-example`

The Makefile assumes you have clang installed, change the variable COMPILER to e.g. `gcc` otherwise.
# Running the example cli
`out/cli-example`
## Commands
- `connect <IP>` assumes chess server listens on port 8089
- `join <Room Id>`
- `create room` prints a room id on success
- `move <position> to <position>` where position is e.g. a1 to h8 (must be lower case)
- `quit` quits the program
## Example
```
out/cli-example     
> connect 127.0.0.1
Connected
> join 374272000
> 
8  04 03 02 06 05 02 03 04 

7  01 01 01 01 01 01 01 01 

6  00 00 00 00 00 00 00 00 

5  00 00 00 00 00 00 00 00 

4  00 00 00 00 00 00 00 00 

3  00 00 00 00 00 00 00 00 

2  81 81 81 81 81 81 81 81 

1  84 83 82 86 85 82 83 84 

   a  b  c  d  e  f  g  h  

on_turn_change: It's the opponents turn.
```
