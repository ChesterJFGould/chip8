/*
A Chip8 interpreter written in c99 suckless.

zlib License

(C) 2020 Chester J.F. Gould 

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <raylib.h>


/*
Type Declarations
*/

typedef struct {
	unsigned short ProgramCounter;
	unsigned short Stack[32];
	int StackCounter;
	union {
		unsigned long long Low[32];
		unsigned long long High[2][64];
	} DisplayBuffer;
	int DisplayIsHigh;
	int UsingCompatibility;
	unsigned char Time;
	unsigned char Tone;
	unsigned short I;
	unsigned short Keys;
	unsigned short KeyMask;
	int WaitingForKeyPress;
	unsigned char V[16];
	unsigned char Memory[0x1000];
} State;

/*
Function Declarations
*/

/*
Chip8 Declarations
*/

/*
0x00E0
clear
Clears the screen.
*/
void
clearScreen(void);

/*
0x00EE
ret
Returns from subroutine.
*/
void
subroutineReturn(void);

/*
DEPRECATED
0x00FA
compatability
Only for backwards compatability.
Causes "save" and "restore" opcodes to leave I register unchanged.
*/
void
compatability(void);

/*
0x0NNN
jump addr
Jumps to address NNN
NNN must be even.
NNN must be in range 0x200 to 0xFFE.
*/
void
jump(unsigned short address);

/*
0xBNNN
jump addr, v0
Jumps to address NNN + v0
NNN must be even.
v0 must be even.
NNN + v0 must be even.
NNN + v0 must be in range 0x200 to 0xFFE.
*/
void
jumpv0(unsigned short address);

/*
0x1NNN
call addr
Calls subroutine at address NNN.
NNN must be even.
NNN must be in range 0x200 to 0xFFE.
*/
void
call(unsigned short address);


/*
0x3XYY
skip.eq vX, value
Skips the next instruction if vX is equal to value.
*/
void
skipEqvXValue(unsigned char registerX, unsigned char value);

/*
0x3XY0
skip.eq vX, Vy
Skips the next instruction if vX is equal to vY.
*/
void
skipEqvXvY(unsigned char registerX, unsigned char registerY);

/*
0xEX9E
skip.eq vX, key
Skips the next instruction if the key with the value of the lower 4 bits of vX is being pressed.
*/
void
skipvXKey(unsigned char registerX);

/*
0x4XKK
skip.ne vX, value
Skips the next instruction if vX is not equal to value.
*/
void
skipNevXValue(unsigned char registerX, unsigned char value);

/*
0x9XY0
skip.ne vY, vY
Skips the next instruction if vY is not equal to vY.
*/
void
skipNevXvY(unsigned char registerX, unsigned char registerY);

/*
0xEXA1
skip.ne vX, key
Skips the next instruction if the key with the value of the lower 4 bits of vX is not being pressed.
*/
void
skipNevXKey(unsigned char registerX);

/*
0x6XKK
load vX, value
Loads register vX with value.
*/
void
loadvXValue(unsigned char registerX, unsigned char value);

/*
0xFX0A
load vX, key
If no key is currently pressed wait until one is, then load vX with the lowest key currently being pressed.
Loaded key will not be registered as pressed again until it is let go and re-pressed.
*/
void
loadvXKey(unsigned char registerX);

/*
0x8XY0
load vX, vY
Loads register vX with value of vY.
*/
void
loadvXvY(unsigned char registerX, unsigned char registerY);

/*
0xFX07
load vX, time
Loads register vX with the value of the time register.
*/
void
loadvXTime(unsigned char registerX);

/*
0xFX15
load time, vX
Loads the time register with the value of register vX.
*/
void
loadTimevX(unsigned char registerX);

/*
0xFX18
load tone, vX
Loads the tone register with the value of register vX.
*/
void
loadTonevX(unsigned char registerX);

/*
0xANNN
load i, addr
Loads the I register with NNN.
NNN must be in the range 0x200 to 0xFFF
*/
void
loadI(unsigned short address);

/*
0x7XKK
add vX, value
Adds KK to register vX.
*/
void
addvXValue(unsigned char registerX, unsigned char value);

/*
0x8XY4
add vX, vY
Adds register vY to register vX.
Register v15 is set to 1 if result overflows.
Else register v15 is set to 0.
*/
void
addvXvY(unsigned char registerX, unsigned char registerY);

/*
0xFX1E
add i, vX
Adds the value of register vX to the I register.
*/
void
addIvX(unsigned char registerX);

/*
0x8XY1
or vX, vY
Bitwise ORs the value of register vY into register vX.
*/
void
orvXvY(unsigned char registerX, unsigned char registerY);

/*
0x8XY2
and vX, vY
Bitwise ANDs the value of register vY into register vX.
*/
void
andvXvY(unsigned char registerX, unsigned char registerY);

/*
0x8XY3
xor vX, vY
Bitwise XORs the value of register vY into register vX.
*/
void
xorvXvY(unsigned char registerX, unsigned char registerY);

/*
0x8XY5
sub vX, vY
Subtracts the value of register vY from register vX.
Register v15 is set to 1 if the result underflows.
Else register v15 is set to 0.
*/
void
subvXvY(unsigned char registerX, unsigned char registerY);

/*
0x8X06
shr vX
Shifts the value of register vX right one bit.
Register v15 is set to 1 if register vX was odd before the operation.
Else register v15 is set to 0.
*/
void
shrvX(unsigned char registerX);

/*
0x8XY7
dif vX, vY
Sets register vX to the value of register vY minus register vX.
Register v15 is set to 1 if the result would be less than 0.
Else register v15 is set to 0.
*/
void
difvXvY(unsigned char registerX, unsigned char registerY);

/*
0x8X0E
shl vX
Shifts the value of register vX left one bit.
Register v15 is set to 1 if the high bit of register vX was set before the operation.
Else register v15 is set to 0.
*/
void
shlvX(unsigned char registerX);

/*
0xCXKK
rnd vX, mask
Sets register vX to the bitwise AND of a random number and KK.
*/
void
rndvXMask(unsigned char registerX, unsigned char mask);

/*
0xDXYN
draw xY, vY, rows
Draws an image to the display buffer.
The image is 8 pixels wide and N pixels long.
The image is pointed to by the I register.
The top left corner of the image will be at (register vX, register vY).
Pixels are blitted to the display buffer using XOR.
If this causes one or more pixels to be erased register v15 is set to 1.
Else register v15 is set to 0.
N must be in the range 1 to 15.
*/
void
drawvXvYRows(unsigned char registerX, unsigned char registerY, unsigned char rows);

/*
0xFX29
hex vX
Points the I register to an image of a hex character representing the low 4 bits of register vX.
The image is 4 pixels wide and 5 pixels long.
*/
void
hexvX(unsigned char registerX);

/*
0xFX33
bcd vX
Stores a BCD(Binary Coded Decimal) representation of the value of register vX into memory starting at the byte pointed to by the I register. 
Most significant digit is loaded first.
*/
void
bcdvX(unsigned char registerX);

/*
0xFX55
save vX
Stores the values of registers v0 to vX in memory starting at the byte pointed to by the I register.
*/
void
savevX(unsigned char registerX);

/*
0xFX65
restore vX
Loads the values in memory starting at the byte pointed to by the I register into registers v0 to vX.
*/
void
restorevX(unsigned char registerX);

/*
DEBUG OPCODE
0x001X
exit value
Causes the program to exit with the value of X.
X must be in the range 0 to 1.
*/
int
programExitValue(unsigned char value);

/*
Super Chip8 Declarations
*/

/*
0x00Cn
scdown n
Scrolls the display buffer down by n pixels.
*/
void
scrollDownN(unsigned char n);

/*
0x00FB
scright
Scrolls the display buffer right 4 pixels.
*/
void
scrollRight(void);

/*
0x00FC
scleft
Scrolls the display buffer left 4 pixels.
*/
void
scrollLeft(void);

/*
0x00FE
low
Sets the display buffer to the low resolution (64 x 32).
This is the default state.
*/
void
displayBufferLow(void);

/*
0x00FF
high
Sets the display buffer to the high resolution (128 x 64).
*/
void
displayBufferHigh(void);

/*
0xDXY0
xdraw vX, vY
Draws an image to the display buffer.
The image is 16 pixels wide and 16 pixels long.
The image is pointed to by the I register.
The top left corner of the image will be at (register vX, register vY).
Pixels are blitted to the display buffer using XOR.
If this causes one or more pixels to be erased register v15 is set to 1.
Else register v15 is set to 0.
*/
void
drawvXvY(unsigned char registerX, unsigned char registerY);

/*
0x00FD
exit
Causes the program to exit with a successful exit status.
*/
int
programExit(void);

/*
Global Variables
*/

static State MachineState;

/*
Function Definitions
*/

/*
Chip8 Definitions
*/

/*
0x00E0
clear
Clears the screen.
*/
void
clearScreen(void)
{
	memset(MachineState.DisplayBuffer.High, 0, sizeof(MachineState.DisplayBuffer.High));
}

/*
0x00EE
ret
Returns from subroutine.
*/
void
subroutineReturn(void)
{
	MachineState.ProgramCounter = MachineState.Stack[MachineState.StackCounter--];
}

/*
DEPRECATED
0x00FA
compatability
Only for backwards compatability.
Causes "save" and "restore" opcodes to leave I register unchanged.
*/
void
compatability(void)
{
	MachineState.UsingCompatibility = 1;
}

/*
0x0NNN
jump addr
Jumps to address NNN
NNN must be even.
NNN must be in range 0x200 to 0xFFE.
*/
void
jump(unsigned short address)
{
	MachineState.ProgramCounter = address - 2;
}

/*
0xBNNN
jump addr, v0
Jumps to address NNN + v0
NNN must be even.
v0 must be even.
NNN + v0 must be even.
NNN + v0 must be in range 0x200 to 0xFFE.
*/
void
jumpv0(unsigned short address)
{
	MachineState.ProgramCounter = address + MachineState.V[0] - 2;
}

/*
0x1NNN
call addr
Calls subroutine at address NNN.
NNN must be even.
NNN must be in range 0x200 to 0xFFE.
*/
void
call(unsigned short address)
{
	MachineState.Stack[++MachineState.StackCounter] = MachineState.ProgramCounter;
	MachineState.ProgramCounter = address - 2;
}


/*
0x3XYY
skip.eq vX, value
Skips the next instruction if vX is equal to value.
*/
void
skipEqvXValue(unsigned char registerX, unsigned char value)
{
	if (MachineState.V[registerX] == value) {
		MachineState.ProgramCounter += 2;
	}
}

/*
0x3XY0
skip.eq vX, Vy
Skips the next instruction if vX is equal to vY.
*/
void
skipEqvXvY(unsigned char registerX, unsigned char registerY)
{
	if (MachineState.V[registerX] == MachineState.V[registerY]) {
		MachineState.ProgramCounter += 2;
	}
}

/*
0xEX9E
skip.eq vX, key
Skips the next instruction if the key with the value of the lower 4 bits of vX is being pressed.
*/
void
skipvXKey(unsigned char registerX)
{
	if ((1 << (MachineState.V[registerX] & 0xF)) & MachineState.Keys) {
		MachineState.ProgramCounter += 2;
	}
}

/*
0x4XKK
skip.ne vX, value
Skips the next instruction if vX is not equal to value.
*/
void
skipNevXValue(unsigned char registerX, unsigned char value)
{
	if (MachineState.V[registerX] != value) {
		MachineState.ProgramCounter += 2;	
	}	
}

/*
0x9XY0
skip.ne vY, vY
Skips the next instruction if vY is not equal to vY.
*/
void
skipNevXvY(unsigned char registerX, unsigned char registerY)
{
	if (MachineState.V[registerX] != MachineState.V[registerY]) {
		MachineState.ProgramCounter += 2;
	}
}

/*
0xEXA1
skip.ne vX, key
Skips the next instruction if the key with the value of the lower 4 bits of vX is not being pressed.
*/
void
skipNevXKey(unsigned char registerX)
{
	if (!((1 << (MachineState.V[registerX] & 0xF)) & MachineState.Keys)) {
		MachineState.ProgramCounter += 2;
	}
}

/*
0x6XKK
load vX, value
Loads register vX with value.
*/
void
loadvXValue(unsigned char registerX, unsigned char value)
{
	MachineState.V[registerX] = value;
}

/*
0xFX0A
load vX, key
If no key is currently pressed wait until one is, then load vX with the lowest key currently being pressed.
Loaded key will not be registered as pressed again until it is let go and re-pressed.
*/
void
loadvXKey(unsigned char registerX)
{
	if ((MachineState.Keys & MachineState.KeyMask)== 0) {
		MachineState.WaitingForKeyPress = 1;
	} else {
		MachineState.WaitingForKeyPress = 0;
		unsigned char lowestKey = 0;
		while ((MachineState.Keys & MachineState.KeyMask) & (1 << lowestKey)) {
			++lowestKey;
		}		
		MachineState.V[registerX] = lowestKey;
		MachineState.KeyMask &= ~(1 << lowestKey);	
	}
}

/*
0x8XY0
load vX, vY
Loads register vX with value of vY.
*/
void
loadvXvY(unsigned char registerX, unsigned char registerY)
{
	MachineState.V[registerX] = MachineState.V[registerY];
}

/*
0xFX07
load vX, time
Loads register vX with the value of the time register.
*/
void
loadvXTime(unsigned char registerX)
{
	MachineState.V[registerX] = MachineState.Time;
}

/*
0xFX15
load time, vX
Loads the time register with the value of register vX.
*/
void
loadTimevX(unsigned char registerX)
{
	MachineState.Time = MachineState.V[registerX];
}

/*
0xFX18
load tone, vX
Loads the tone register with the value of register vX.
*/
void
loadTonevX(unsigned char registerX)
{
	MachineState.Tone = MachineState.V[registerX];
}

/*
0xANNN
load i, addr
Loads the I register with NNN.
NNN must be in the range 0x200 to 0xFFF
*/
void
loadI(unsigned short address)
{
	MachineState.I = address;
}

/*
0x7XKK
add vX, value
Adds KK to register vX.
*/
void
addvXValue(unsigned char registerX, unsigned char value)
{
	MachineState.V[registerX] += value;
}

/*
0x8XY4
add vX, vY
Adds register vY to register vX.
Register v15 is set to 1 if result overflows.
Else register v15 is set to 0.
*/
void
addvXvY(unsigned char registerX, unsigned char registerY)
{
	MachineState.V[registerX] += MachineState.V[registerY];
}

/*
0xFX1E
add i, vX
Adds the value of register vX to the I register.
*/
void
addIvX(unsigned char registerX)
{
	MachineState.I += MachineState.V[registerX];	
}

/*
0x8XY1
or vX, vY
Bitwise ORs the value of register vY into register vX.
*/
void
orvXvY(unsigned char registerX, unsigned char registerY)
{
	MachineState.V[registerX] |= MachineState.V[registerY];
}

/*
0x8XY2
and vX, vY
Bitwise ANDs the value of register vY into register vX.
*/
void
andvXvY(unsigned char registerX, unsigned char registerY)
{
	MachineState.V[registerX] &= MachineState.V[registerY];
}

/*
0x8XY3
xor vX, vY
Bitwise XORs the value of register vY into register vX.
*/
void
xorvXvY(unsigned char registerX, unsigned char registerY)
{
	MachineState.V[registerX] ^= MachineState.V[registerY];
}

/*
0x8XY5
sub vX, vY
Subtracts the value of register vY from register vX.
Register v15 is set to 1 if the result underflows.
Else register v15 is set to 0.
*/
void
subvXvY(unsigned char registerX, unsigned char registerY)
{
	MachineState.V[15] = MachineState.V[registerX] < MachineState.V[registerY];
	
	MachineState.V[registerX] -= MachineState.V[registerY];
}

/*
0x8X06
shr vX
Shifts the value of register vX right one bit.
Register v15 is set to 1 if register vX was odd before the operation.
Else register v15 is set to 0.
*/
void
shrvX(unsigned char registerX)
{
	MachineState.V[15] = MachineState.V[registerX] & 1;
	
	MachineState.V[registerX] >>= 1;
}

/*
0x8XY7
dif vX, vY
Sets register vX to the value of register vY minus register vX.
Register v15 is set to 1 if the result would be less than 0.
Else register v15 is set to 0.
*/
void
difvXvY(unsigned char registerX, unsigned char registerY)
{
	MachineState.V[15] = MachineState.V[registerX] > MachineState.V[registerY];
	
	MachineState.V[registerX] = MachineState.V[registerY] - MachineState.V[registerX];
}

/*
0x8X0E
shl vX
Shifts the value of register vX left one bit.
Register v15 is set to 1 if the high bit of register vX was set before the operation.
Else register v15 is set to 0.
*/
void
shlvX(unsigned char registerX)
{
	MachineState.V[15] = MachineState.V[registerX] & 0x80;

	MachineState.V[registerX] <<= 1;
}

/*
0xCXKK
rnd vX, mask
Sets register vX to the bitwise AND of a random number and KK.
*/
void
rndvXMask(unsigned char registerX, unsigned char mask)
{
	MachineState.V[registerX] = (unsigned char)rand() & mask;
}

/*
0XDXYN
draw xY, vY, rows
Draws an image to the display buffer.
The image is 8 pixels wide and N pixels long.
The image is pointed to by the I register.
The top left corner of the image will be at (register vX, register vY).
Pixels are blitted to the display buffer using XOR.
If this causes one or more pixels to be erased register v15 is set to 1.
Else register v15 is set to 0.
N must be in the range 1 to 15.
*/
void
drawvXvYRows(unsigned char registerX, unsigned char registerY, unsigned char rows)
{
	int x = MachineState.V[registerX];
	int y = MachineState.V[registerY];

	if (MachineState.DisplayIsHigh) {
		for (int i = 0; i < y; i++) {
			unsigned long long firstHalf = ((unsigned long long)MachineState.Memory[MachineState.I + i] << 56) >> x; 
			unsigned long long secondHalf = (unsigned long long)MachineState.Memory[MachineState.I + i] << (120 - x);

			if ((MachineState.DisplayBuffer.High[0][y + i] & firstHalf) || (MachineState.DisplayBuffer.High[1][i] & secondHalf)) {
				MachineState.V[15] = 1;						
			} else {
				MachineState.V[15] = 0;	
			}
			
			MachineState.DisplayBuffer.High[0][y + i] ^= firstHalf;
			MachineState.DisplayBuffer.High[0][y + i] ^= secondHalf;			
		}
	} else {
		for (int i = 0; i < rows; i++) {
			unsigned long long line = (unsigned long long)MachineState.Memory[MachineState.I + i] << (56 - x);
			if (MachineState.DisplayBuffer.Low[y + i] & line) {
				MachineState.V[15] = 1;
			} else {
				MachineState.V[15] = 0;
			}
			
			MachineState.DisplayBuffer.Low[y + i] ^= line;
		}
	}
}

/*
0xFX29
hex vX
Points the I register to an image of a hex character representing the low 4 bits of register vX.
The image is 4 pixels wide and 5 pixels long.
*/
void
hexvX(unsigned char registerX)
{
	MachineState.I = (MachineState.V[registerX] & 0xF) << 4;
}

/*
0xFX33
bcd vX
Stores a BCD(Binary Coded Decimal) representation of the value of register vX into memory starting at the byte pointed to by the I register. 
Most significant digit is loaded first.
*/
void
bcdvX(unsigned char registerX)
{
	MachineState.Memory[MachineState.I] = MachineState.V[registerX] / 100;
	MachineState.Memory[MachineState.I+1] = (MachineState.V[registerX] % 100) / 10;
	MachineState.Memory[MachineState.I+2] = (MachineState.V[registerX] % 10);
}

/*
0xFX55
save vX
Stores the values of registers v0 to vX in memory starting at the byte pointed to by the I register.
*/
void
savevX(unsigned char registerX)
{
	for (int i = 0; i <= registerX; i++) {
		MachineState.Memory[MachineState.I + i] = MachineState.V[i];
	}

	if (!MachineState.UsingCompatibility) {
		MachineState.I += registerX + 1;
	}
}

/*
0xFX65
restore vX
Loads the values in memory starting at the byte pointed to by the I register into registers v0 to vX.
*/
void
restorevX(unsigned char registerX)
{
	for (int i = 0; i <= registerX; i++) {
		MachineState.V[i] = MachineState.Memory[MachineState.I + i];
	}

	if (!MachineState.UsingCompatibility) {
		MachineState.I += registerX + 1;
	}
}

/*
DEBUG OPCODE
0x001X
exit value
Causes the program to exit with the value of X.
X must be in the range 0 to 1.
*/
int
programExitValue(unsigned char value)
{
	return value;
}

/*
Super Chip8 Definitions
*/
// TODO
/*
0x00Cn
scdown n
Scrolls the display buffer down by n pixels.
*/
void
scrollDownN(unsigned char n)
{
/*
	Possibly delay execution of this opcode until drawing in Low mode?
*/
	if (MachineState.DisplayIsHigh) {
		/*
			Copy DisplayBuffer lines that will be kept to their new spots.
		*/
		for (int i = 0; i < (64 - n); i++) {
			MachineState.DisplayBuffer.High[0][i+n] = MachineState.DisplayBuffer.High[0][i];
			MachineState.DisplayBuffer.High[1][i+n] = MachineState.DisplayBuffer.High[1][i];
		}
		/*
			Zero copied lines.
			This should work but if stuff breaks check here first.
		*/
		memset(MachineState.DisplayBuffer.High, 0, n * 2 * sizeof(unsigned long long));
	} else {
		/*
			Copy DisplayBuffer lines that will be kept to their new spots.
		*/
		for (int i = 0; i < (32 - n); i++) {
			MachineState.DisplayBuffer.Low[i+n] = MachineState.DisplayBuffer.Low[i];
		}
		/*
			Zero copied lines.
			This should work but if stuff breaks check here first.
		*/
		memset(MachineState.DisplayBuffer.Low, 0, n * sizeof(unsigned long long));
	}	
}

/*
0x00FB
scright
Scrolls the display buffer right 4 pixels.
*/
void
scrollRight(void)
{
/*
	Possibly delay execution of this opcode until drawing in Low mode?
*/
	if (MachineState.DisplayIsHigh) {
		for (int i = 0; i < 64; i++) {
			/*
				Shift the second line over
			*/
			MachineState.DisplayBuffer.High[1][i] >>= 4;
			/*
				Copy last 4 bits of first half into first 4 bits of second half
			*/
			MachineState.DisplayBuffer.High[1][i] |= (MachineState.DisplayBuffer.High[0][i] & 0xf) << 60;
			/*
				Shift the first line over.
			*/
			MachineState.DisplayBuffer.High[0][i] >>= 4;	
		}
	} else {
		for (int i = 0; i < 32; i++) {
			MachineState.DisplayBuffer.Low[i] >>= 4;
		}
	}
}

/*
0x00FC
scleft
Scrolls the display buffer left 4 pixels.
*/
void
scrollLeft(void)
{
/*
	Possibly delay execution of this opcode until drawing in Low mode?
*/
	if (MachineState.DisplayIsHigh) {
		for (int i = 0; i < 64; i++) {
			/*
				Shift the first line over
			*/
			MachineState.DisplayBuffer.High[0][i] <<= 4;
			/*
				Copy first 4 bits of second half into last 4 bits of first half
			*/
			MachineState.DisplayBuffer.High[0][i] |= (MachineState.DisplayBuffer.High[0][i] & ((unsigned long long)0xf << 60)) >> 60;
			/*
				Shift the second line over.
			*/
			MachineState.DisplayBuffer.High[1][i] <<= 4;	
		}
	} else {
		for (int i = 0; i < 32; i++) {
			MachineState.DisplayBuffer.Low[i] <<= 4;
		}
	}
}

/*
0x00FE
low
Sets the display buffer to the low resolution (64 x 32).
This is the default state.
*/
void
displayBufferLow(void)
{
	MachineState.DisplayIsHigh = 0;	
}

/*
0x00FF
high
Sets the display buffer to the high resolution (128 x 64).
*/
void
displayBufferHigh(void)
{
	MachineState.DisplayIsHigh = 1;
}

/*
0xDXY0
xdraw vX, vY
Draws an image to the display buffer.
The image is 16 pixels wide and 16 pixels long.
The image is pointed to by the I register.
The top left corner of the image will be at (register vX, register vY).
Pixels are blitted to the display buffer using XOR.
If this causes one or more pixels to be erased register v15 is set to 1.
Else register v15 is set to 0.
*/
void
drawvXvY(unsigned char registerX, unsigned char registerY)
{
	int x = MachineState.V[registerX];
	int y = MachineState.V[registerY];

	if (MachineState.DisplayIsHigh) {
		for (int i = 0; i < 16; i++) {
			unsigned long long firstHalf = ((((unsigned long long)MachineState.Memory[MachineState.I + 2 * i] << 8) | ((unsigned long long)MachineState.Memory[MachineState.I + 2 * i + 1])) << 48) >> x;
			unsigned long long secondHalf = ((MachineState.Memory[MachineState.I + 2 * i] << 8) | (MachineState.Memory[MachineState.I + 2 * i + 1])) << (111 - x);

			if ((MachineState.DisplayBuffer.High[0][y + i] & firstHalf) || (MachineState.DisplayBuffer.High[1][y + i] & secondHalf)) {
				MachineState.V[15] = 1;
			} else {
				MachineState.V[15] = 0;
			}
			
			MachineState.DisplayBuffer.High[0][y + i] ^= firstHalf;
			MachineState.DisplayBuffer.High[1][y + i] ^= secondHalf;
		}				
	} else {
		for (int i = 0; i < 16; i++) {
			unsigned long long line = ((unsigned long long)MachineState.Memory[MachineState.I + 2 * i] << 8) | ((unsigned long long)MachineState.Memory[MachineState.I + 2 * i + 1]) << (56 - x);
			
			if (MachineState.DisplayBuffer.Low[y + i] & line) {
				MachineState.V[15] = 1;
			} else {
				MachineState.V[15] = 0;
			}

			MachineState.DisplayBuffer.Low[y + i] ^= line;
		}
	}	
}

/*
0x00FD
exit
Causes the program to exit with a successful exit status.
*/
int
programExit(void) {
	return 0;
}


int
main(int argc, char *argv[])
{
	if (argc != 2) {
		printf("Please specify one program file\n");	
		return 0;
	}

	/*
	Init MachineState
	*/
	MachineState.ProgramCounter = 0x200;
	MachineState.StackCounter = -1;
	memset(MachineState.DisplayBuffer.High, 0, sizeof(MachineState.DisplayBuffer.High));	
	MachineState.DisplayIsHigh = 0;
	MachineState.UsingCompatibility = 0;
	MachineState.Time = 0;
	MachineState.Tone = 0;
	MachineState.I = 0;
	MachineState.Keys = 0;
	MachineState.KeyMask = 0xFFFF;
	MachineState.WaitingForKeyPress = 0;
	memset(MachineState.V, 0, sizeof(MachineState.V));
	memset(MachineState.Stack, 0, sizeof(MachineState.Stack));
	memset(MachineState.Memory, 0, sizeof(MachineState.Memory));
	
	/*
	Init Memory with hex characters at 0xN0
	*/
	
	/*
	####
	#  #
	#  #
	####
	*/
	MachineState.Memory[0x00] = 0xf0;
	MachineState.Memory[0x00 + 1] = 0x90;
	MachineState.Memory[0x00 + 2] = 0x90;
	MachineState.Memory[0x00 + 3] = 0x90;
	MachineState.Memory[0x00 + 4] = 0xf0;

	/*
	  #
	 ##
	  #
	 ###	
	*/
	MachineState.Memory[0x10] = 0x20;
	MachineState.Memory[0x10 + 1] = 0x60;
	MachineState.Memory[0x10 + 2] = 0x20;
	MachineState.Memory[0x10 + 3] = 0x20;
	MachineState.Memory[0x10 + 4] = 0x70;

	/*
	####
	   #
	####
	#
	####
	*/
	MachineState.Memory[0x20] = 0xf0;
	MachineState.Memory[0x20 + 1] = 0x10;
	MachineState.Memory[0x20 + 2] = 0xf0;
	MachineState.Memory[0x20 + 3] = 0x80;
	MachineState.Memory[0x20 + 4] = 0xf0;

	/*
	####
	   #
	####
	   #
	####	
	*/
	MachineState.Memory[0x30] = 0xf0;
	MachineState.Memory[0x30 + 1] = 0x10;
	MachineState.Memory[0x30 + 2] = 0xf0;
	MachineState.Memory[0x30 + 3] = 0x10;
	MachineState.Memory[0x30 + 4] = 0xf0;

	/*
	#  #
	#  #
	####
	   #
	   #
	*/
	MachineState.Memory[0x40] = 0x90;
	MachineState.Memory[0x40 + 1] = 0x90;
	MachineState.Memory[0x40 + 2] = 0xf0;
	MachineState.Memory[0x40 + 3] = 0x10;
	MachineState.Memory[0x40 + 4] = 0x10;

	/*
	####
	#
	####
	   #
	####
	*/
	MachineState.Memory[0x50] = 0xf0;
	MachineState.Memory[0x50 + 1] = 0x80;
	MachineState.Memory[0x50 + 2] = 0xf0;
	MachineState.Memory[0x50 + 3] = 0x10;
	MachineState.Memory[0x50 + 4] = 0xf0;

	/*
	####
	#
	####
	#  #
	####	
	*/
	MachineState.Memory[0x60] = 0xf0;
	MachineState.Memory[0x60 + 1] = 0x80;
	MachineState.Memory[0x60 + 2] = 0xf0;
	MachineState.Memory[0x60 + 3] = 0x90;
	MachineState.Memory[0x60 + 4] = 0xf0;
	
	/*
	####
	   #
	  #
	 #
	 #
	*/
	MachineState.Memory[0x70] = 0xf0;
	MachineState.Memory[0x70 + 1] = 0x10;
	MachineState.Memory[0x70 + 2] = 0x20;
	MachineState.Memory[0x70 + 3] = 0x40;
	MachineState.Memory[0x70 + 4] = 0x40;

	/*
	####
	#  #
	####
	#  #
	####
	*/
	MachineState.Memory[0x80] = 0xf0;
	MachineState.Memory[0x80 + 1] = 0x90;
	MachineState.Memory[0x80 + 2] = 0xf0;
	MachineState.Memory[0x80 + 3] = 0x90;
	MachineState.Memory[0x80 + 4] = 0xf0;

	/*
	####
	#  #
	####
	   #
	####
	*/
	MachineState.Memory[0x90] = 0xf0;
	MachineState.Memory[0x90 + 1] = 0x90;
	MachineState.Memory[0x90 + 2] = 0xf0;
	MachineState.Memory[0x90 + 3] = 0x10;
	MachineState.Memory[0x90 + 4] = 0xf0;

	/*
	####
	#  #
	####
	#  #
	#  #
	*/
	MachineState.Memory[0xa0] = 0xf0;
	MachineState.Memory[0xa0 + 1] = 0x90;
	MachineState.Memory[0xa0 + 2] = 0xa0;
	MachineState.Memory[0xa0 + 3] = 0x90;
	MachineState.Memory[0xa0 + 4] = 0x90;

	/*
	###
	#  #
	###
	#  #
	###
	*/
	MachineState.Memory[0xb0] = 0xe0;
	MachineState.Memory[0xb0 + 1] = 0x90;
	MachineState.Memory[0xb0 + 2] = 0xe0;
	MachineState.Memory[0xb0 + 3] = 0x90;
	MachineState.Memory[0xb0 + 4] = 0xe0;

	/*
	####
	#
	#
	#
	####
	*/
	MachineState.Memory[0xc0] = 0xf0;
	MachineState.Memory[0xc0 + 1] = 0x80;
	MachineState.Memory[0xc0 + 2] = 0x80;
	MachineState.Memory[0xc0 + 3] = 0x80;
	MachineState.Memory[0xc0 + 4] = 0xf0;
	
	/*
	###
	#  #
	#  #
	#  #
	###
	*/
	MachineState.Memory[0xd0] = 0xe0;
	MachineState.Memory[0xd0 + 1] = 0x90;
	MachineState.Memory[0xd0 + 2] = 0x90;
	MachineState.Memory[0xd0 + 3] = 0x90;
	MachineState.Memory[0xd0 + 4] = 0xe0;

	/*
	####
	#
	####
	#
	####
	*/
	MachineState.Memory[0xe0] = 0xf0;
	MachineState.Memory[0xe0 + 1] = 0x80;
	MachineState.Memory[0xe0 + 2] = 0xf0;
	MachineState.Memory[0xe0 + 3] = 0x80;
	MachineState.Memory[0xe0 + 4] = 0xf0;

	/*
	####
	#
	####
	#
	#
	*/
	MachineState.Memory[0xf0] = 0xf0;
	MachineState.Memory[0xf0 + 1] = 0x80;
	MachineState.Memory[0xf0 + 2] = 0xf0;
	MachineState.Memory[0xf0 + 3] = 0x80;
	MachineState.Memory[0xf0 + 4] = 0x80;

	/*
	Load program into memory
	*/

	FILE *programFile = fopen(argv[1], "rb");

	fread(MachineState.Memory + 0x200, sizeof(unsigned char), 0x1000 - 0x200, programFile);

	fclose(programFile);

	int screenWidth = 1920;
	int screenHeight = 1080;

	InitWindow(screenWidth, screenHeight, "Chip8 Emulator");

	SetTargetFPS(60);

	while (!IsWindowReady()) {
		
	}

	while (!WindowShouldClose()) {
		if (IsWindowResized()) {
			screenWidth = GetScreenWidth();
			screenHeight = GetScreenHeight();
		}
		/*
		Handle Input
		*/
		
		/*
		Clear Keys
		*/
		MachineState.Keys = 0;
		
		/*
		Key 0
		*/
		MachineState.Keys |= (unsigned short)IsKeyDown(KEY_ONE) << 0;

		/*
		Key 1
		*/
		MachineState.Keys |= (unsigned short)IsKeyDown(KEY_TWO) << 1;

		/*
		Key 2
		*/
		MachineState.Keys |= (unsigned short)IsKeyDown(KEY_THREE) << 2;

		/*
		Key 3
		*/
		MachineState.Keys |= (unsigned short)IsKeyDown(KEY_FOUR) << 3;

		/*
		Key 4
		*/
		MachineState.Keys |= (unsigned short)IsKeyDown(KEY_Q) << 4;

		/*
		Key 5
		*/
		MachineState.Keys |= (unsigned short)IsKeyDown(KEY_W) << 5;

		/*
		Key 6
		*/
		MachineState.Keys |= (unsigned short)IsKeyDown(KEY_E) << 6;
		
		/*
		Key 7
		*/
		MachineState.Keys |= (unsigned short)IsKeyDown(KEY_R) << 7;

		/*
		Key 8
		*/
		MachineState.Keys |= (unsigned short)IsKeyDown(KEY_A) << 8;
	
		/*
		Key 9
		*/
		MachineState.Keys |= (unsigned short)IsKeyDown(KEY_S) << 9;

		/*
		Key a
		*/
		MachineState.Keys |= (unsigned short)IsKeyDown(KEY_D) << 10;

		/*
		Key b
		*/
		MachineState.Keys |= (unsigned short)IsKeyDown(KEY_F) << 11;

		/*
		Key c
		*/
		MachineState.Keys |= (unsigned short)IsKeyDown(KEY_Z) << 12;

		/*
		Key d
		*/
		MachineState.Keys |= (unsigned short)IsKeyDown(KEY_X) << 13;

		/*
		Key e
		*/
		MachineState.Keys |= (unsigned short)IsKeyDown(KEY_C) << 14;

		/*
		Key f
		*/
		MachineState.Keys |= (unsigned short)IsKeyDown(KEY_V) << 15;

		/*
		Set KeyMask for released keys
		*/
		MachineState.KeyMask |= ~(MachineState.Keys);	

		/*
		Do N ticks
		*/
		int ticksPerFrame = 10;

		for (int i = 0; i < ticksPerFrame; i++) {
			unsigned short opcode = (MachineState.Memory[MachineState.ProgramCounter] << 8) | MachineState.Memory[MachineState.ProgramCounter + 1];		
			#ifdef DEBUG
			printf("Opcode: %X\n", opcode);
			for (int j = 0; j < 15; j++) {
				printf("v%i: %X\n", j, MachineState.V[j]);
			}	
			printf("Keys: %X\n", MachineState.Keys);
			printf("DisplayBuffer:\n");
			if (MachineState.DisplayIsHigh) {
				for (int i = 0; i < 64; i++) {
					printf("%llX %llX\n", MachineState.DisplayBuffer.High[0][i], MachineState.DisplayBuffer.High[1][i]);
				}
			} else {
				for (int i = 0; i < 32; i++) {
					printf("%llX\n", MachineState.DisplayBuffer.Low[i]);
				}
			}
			getchar();
			getchar();
			#endif
			/*
			Match opcode to function
			This could be made faster with a prefix tree
			*/
			switch (opcode & 0xF000) {
			case 0x0000:
				switch (opcode & 0xF00) {
				case 0x000:
					switch (opcode & 0xF0) {
					case 0x10:
						/*
						0x001X
						*/
						return programExitValue(opcode & 0xF);
						break;
					case 0xC0:
						/*
						0x00C0
						*/
						scrollDownN(opcode & 0xF);
						break;
					case 0xE0:
						switch (opcode & 0xF) {
						case 0x0:
							/*
							0x00E0
							*/
							clearScreen();
							break;
						case 0xE:
							/*
							0x00EE
							*/
							subroutineReturn();
							break;
						}
						break;
					case 0xF0:
						switch (opcode & 0xF) {
						case 0xA:
							/*
							0x00FA
							*/
							compatability();
							break;
						case 0xB:
							/*
							0x00FB
							*/
							scrollRight();
							break;
						case 0xC:
							/*
							0x00FC
							*/
							scrollLeft();
							break;
						case 0xD:
							/*
							0x00FD
							*/
							return programExit();
						case 0xE:
							/*
							0x00FE
							*/
							displayBufferLow();
							break;
						case 0xF:
							/*
							0x00FF
							*/
							displayBufferHigh();
							break;
						}
						break;
					}
					break;
				default:
					jump(opcode & 0xFFF);	
					break;
				}
				break;
			case 0x1000:
				/*
				0x1NNN
				*/
				call(opcode & 0xFFF);
				break;
			case 0x3000:
				switch (opcode & 0xF) {
				case 0x0:
					/*
					0x3XY0
					*/
					skipEqvXvY((opcode & 0xF00) >> 8, (opcode & 0xF0) >> 4);
					break;
				default:
					/*
					0x3XYY
					*/
					skipEqvXValue((opcode & 0xF00) >> 8, opcode & 0xFF);
					break;
				}
				break;
			case 0x4000:
				/*
				0x4XKK
				*/
				skipNevXValue((opcode & 0xF00) >> 8, opcode & 0xFF);
				break;
			case 0x6000:
				/*
				0x6XKK
				*/
				loadvXValue((opcode & 0xF00) >> 8, opcode & 0xFF);
				break;
			case 0x7000:
				/*
				0x7XKK
				*/
				addvXValue((opcode & 0xF00) >> 8, opcode & 0xFF);
				break;
			case 0x8000:
				switch (opcode & 0xF) {
				case 0x0:
					/*
					0x8XY0
					*/
					loadvXvY((opcode & 0xF00) >> 8, (opcode & 0xF0) >> 4);
					break;
				case 0x1:
					/*
					0x8XY1
					*/
					orvXvY((opcode & 0xF00) >> 8, (opcode & 0xF0) >> 4);
					break;
				case 0x2:
					/*
					0x8XY2
					*/
					andvXvY((opcode & 0xF00) >> 8, (opcode & 0xF0) >> 4);
					break;
				case 0x3:
					/*
					0x8XY3
					*/
					xorvXvY((opcode & 0xF00) >> 8, (opcode & 0xF0) >> 4);
					break;
				case 0x4:
					/*
					0x8XY4
					*/
					addvXvY((opcode & 0xF00) >> 8, (opcode & 0xF0) >> 4);
					break;
				case 0x5:
					/*
					0x8XY5
					*/
					subvXvY((opcode & 0xF00) >> 8, (opcode & 0xF0) >> 4);
					break;
				case 0x6:
					/*
					0x8X06
					*/
					shrvX((opcode & 0xF00) >> 8);
					break;
				case 0x7:
					/*
					0x8XY7
					*/
					difvXvY((opcode & 0xF00) >> 8, (opcode & 0xF0) >> 4);
					break;
				case 0xE:
					/*
					0x8X0E
					*/
					shlvX((opcode & 0xF00) >> 8);
					break;
				}
				break;
			case 0x9000:
				/*
				0x9XY0
				*/
				skipNevXvY((opcode & 0xF00) >> 8, (opcode & 0xF0) >> 4);
				break;
			case 0xA000:
				/*
				0xANNN
				*/
				loadI(opcode & 0xFFF);
				break;
			case 0xB000:
				/*
				0xBNNN
				*/
				jumpv0(opcode & 0xFFF);
				break;
			case 0xC000:
				/*
				0xCXKK
				*/
				rndvXMask((opcode & 0xF00) >> 8, opcode & 0xFF);
				break;
			case 0xD000:
				switch (opcode & 0xF) {

				case 0x0:
					/*
					0xDXY0
					*/
					#ifdef DEBUG
					printf("draw %X, %X\n", (opcode & 0xF00) >> 8, (opcode & 0xF0) >> 4);
					#endif

					drawvXvY((opcode & 0xF00) >> 8, (opcode & 0xF0) >> 4);
					break;
				default:
					/*
					0xDXYN
					*/
					drawvXvYRows((opcode & 0xF00) >> 8, (opcode & 0xF0) >> 4, opcode & 0xF);
					break;
				}
			case 0xE000:
				switch (opcode & 0xFF) {
				case 0x9E:
					/*
					0xEX9E
					*/
					skipvXKey((opcode & 0xF00) >> 8);
					break;
				case 0xA1:
					/*
					0xEXA1
					*/
					skipNevXKey((opcode & 0xF00) >> 8);
					break;
				}
				break;
			case 0xF000:
				switch (opcode & 0xFF) {
				case 0x07:
					/*
					0xFX07
					*/
					loadvXTime((opcode & 0xF00) >> 8);
					break;
				case 0x0A:
					/*
					0xFX0A
					*/
					loadvXKey((opcode & 0xF00) >> 8);
					break;
				case 0x15:
					/*
					0xFX15
					*/
					loadTimevX((opcode & 0xF00) >> 8);
					break;
				case 0x18:
					/*
					0xFX18
					*/
					loadTonevX((opcode & 0xF00) >> 8);
					break;
				case 0x1E:
					/*
					0xFX1E
					*/
					addIvX((opcode & 0xF00) >> 8);
					break;
				case 0x29:
					/*
					0xFX29
					*/
					hexvX((opcode & 0xF00) >> 8);
					break;
				case 0x33:
					/*
					0xFX33
					*/
					bcdvX((opcode & 0xF00) >> 8);
					break;
				case 0x55:
					/*
					0xFX55
					*/
					savevX((opcode & 0xF00) >> 8);
					break;
				case 0x65:
					/*
					0xFX65
					*/
					restorevX((opcode & 0xF00) >> 8);
					break;
				}
				break;
			}
			
			if (!MachineState.WaitingForKeyPress) {
				MachineState.ProgramCounter += 2;
			}
		}		
		
		/*
		Handle Time and Tone registers
		*/
		if (!MachineState.WaitingForKeyPress) {
			if (MachineState.Time > 0) {
				MachineState.Time -= 1;
			}

			if (MachineState.Tone > 0) {
				/*
				TODO
				Play Tone
				*/
				MachineState.Tone -= 1;
			}
		}

		BeginDrawing();

		ClearBackground(BLACK);
		
		/*
		Draw DisplayBuffer
		*/	
		if (MachineState.DisplayIsHigh) {
			int pixelWidth = screenWidth / 128;
			int pixelHeight = screenWidth / 64;

			for (int y = 0; y < 64; y++) {
				for (int x = 0; x < 64; x++) {
					if (MachineState.DisplayBuffer.High[0][y] & ((unsigned long long)1 << (63 - x))) {
						DrawRectangle(x * pixelWidth, y * pixelHeight, pixelWidth, pixelHeight, WHITE);	
					}

					if (MachineState.DisplayBuffer.High[1][y] & ((unsigned long long)1 << (63 - x))) {
						DrawRectangle(x * pixelWidth + pixelWidth * 64, y * pixelHeight, pixelWidth, pixelHeight, WHITE);	
					}
				}
			}
		} else {
			int pixelWidth = screenWidth / 64;
			int pixelHeight = screenHeight/ 32;
			
			for (int y = 0; y < 32; y++) {
				for (int x = 0; x < 64; x++) {
					if (MachineState.DisplayBuffer.Low[y] & ((unsigned long long)1 << (63 - x))) {
						DrawRectangle(x * pixelWidth, y * pixelHeight, pixelWidth, pixelHeight, WHITE);
					}
				}
			}
		}

		EndDrawing();
	}
	return 0;
}
