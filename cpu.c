#define _CRT_SECURE_NO_WARNINGS
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <SDL_thread.h>
#include <math.h>

const int SCREEN_WIDTH = 512;
const int SCREEN_HEIGHT = 224;
char pixels[512 * 224 * 4]; //reduce res or color depth?
short realPixels[512 * 224]; //512x224x15 is a bit high - optimizing blitting routines in asm will only do somuch

int loop = 1;
int add = 0;
int quit = 0;

unsigned int program[4096 * 1024];
unsigned int RAM[512 * 1024];
unsigned int PEX2RAM[168 * 1024];
unsigned int joypress = 0;
unsigned int r0 = 0;
unsigned int r1 = 0;
unsigned int r2 = 0;
unsigned int r3 = 0;
unsigned int r4 = 0;
unsigned int r5 = 0;
unsigned int r6 = 0;
unsigned int r7 = 0;
unsigned int sp = 4718591;
unsigned int pc = 0;
unsigned int IDX = 0;
unsigned int flags = 0;
unsigned int currentInstruction;
unsigned int increment = 1;
unsigned int arg1;
unsigned int arg0; //long on x64
unsigned char nameArg0[20] = "";
unsigned char nameArg1[20] = "";
int logging = 0;
int opsCPU1 = 0;
int PEX2RUNNING = 0;

unsigned int r0P2 = 0;
unsigned int r1P2 = 0;
unsigned int r2P2 = 0;
unsigned int r3P2 = 0;
unsigned int r4P2 = 0;
unsigned int r5P2 = 0;
unsigned int r6P2 = 0;
unsigned int r7P2 = 0;
unsigned int spP2 = 114687;
unsigned int pcP2 = 0;
unsigned int IDXP2 = 0;
unsigned int flagsP2 = 0;
unsigned int currentInstructionP2;
unsigned int incrementP2 = 1;
unsigned int arg1P2;
unsigned int arg0P2; //long on x64
unsigned char nameArg0P2[20] = "";
unsigned char nameArg1P2[20] = "";
int opsCPU2 = 0;

void cvtNum(int num, char* string) {
	for (int i = 0; i < 12; i++) {
		int thing = (int)pow(2, 11 - i);
		if (num & thing) {
			string[i] = 49;
		}
		else {
			string[i] = 48;
		}
	}
}

unsigned int MMUgetPtrP1(int addr) {
	if (addr < 4194304) { return (unsigned int)&program[addr]; }
	if ((addr > 4194303) && (addr < 4718592)) { return (unsigned int)&RAM[addr - 4194304]; }
	if ((addr > 4718591) && (addr < 4816896)) { return (unsigned int)&PEX2RAM[addr - 4718592]; }
	if (addr == 4816896) { return (unsigned int)&joypress; }
	//sound registers here
}

int MMUgetValP1(int addr) {
	if (addr < 4194304) { return program[addr]; }
	if ((addr > 4194303) && (addr < 4718592)) { return RAM[addr - 4194304]; }
	if ((addr > 4718591) && (addr < 4816896)) { return PEX2RAM[addr - 4718592]; }
	if (addr == 4816896) { return joypress; }
	//sound registers here
}

unsigned int MMUgetPtrP2(int addr) {
	return (unsigned int)&PEX2RAM[addr];
}

int MMUgetValP2(int addr) {
	return PEX2RAM[addr];
}

void cpuStep();

void cpuStepP2();

int counterFunc(void* data) {
	while (1) {
		//Sleep(1000);
		SDL_Delay(1000);
		printf("PEX-1: %d MIPS\n", opsCPU1/(1000*1000));
		opsCPU1 = 0;
	}
	return 0;
}

int threadFunction(void* data) {
	while (1) {
		if (logging) { printf("PC=%u r0=%u r1=%u r2=%u r3=%u r4=%u SP=%u IDX=%u Flags=%u  ", pc, r0, r1, r2, r3, r4, sp, IDX, flags); }
		cpuStep();
		if (PEX2RUNNING) { cpuStepP2(); }
	}
}

void cpuStep() { //when do you clear/reset flags??????????????? - add INT to start PEX-2 execution - PIXEL PLOTTING on PEX-2?
	currentInstruction = MMUgetValP1(pc);
	arg0 = 0;
	arg1 = 0;

	//fetch arg0
	if ((currentInstruction & 240) == 240) { //op0=imm
		//arg0 = (unsigned int)&program[pc + 1];
		arg0 = MMUgetPtrP1(pc + 1);
		//itoa(*(int *)arg0, (char *)nameArg0, 10);
		if (logging) { sprintf((char*)nameArg0, "%d", *(int*)arg0); }
		increment += 1;
	}
	else if ((currentInstruction & 224) == 224) {//op0=[idx]
		//arg0 = (unsigned int)&program[IDX];
		arg0 = MMUgetPtrP1(IDX);
		if (logging) { sprintf((char*)nameArg0, "[IDX](%d:%d)", IDX, *(int*)arg0); }
	}
	else if ((currentInstruction & 176) == 176) { //op0=flags
		arg0 = (unsigned int)&flags;
		if (logging) { sprintf((char*)nameArg0, "FLAGS(%d)", *(int*)arg0); }
	}
	else if ((currentInstruction & 160) == 160) { //op0=sp
		arg0 = (unsigned int)&sp;
		if (logging) { sprintf((char*)nameArg0, "SP(%d)", *(int*)arg0); }
	}
	else if ((currentInstruction & 144) == 144) {//op0=[imm]
		//arg0 = (unsigned int)&program[program[pc + 1]];
		arg0 = MMUgetPtrP1(MMUgetValP1(pc+1));
		increment += 1;
		//if (logging) { sprintf((char*)nameArg0, "[%d](%d)", program[pc + 1], *(int*)arg0); }
		if (logging) { sprintf((char*)nameArg0, "[%d](%d)", MMUgetValP1(pc + 1), *(int*)arg0); }
	}
	else if ((currentInstruction & 128) == 128) { //op0=idx
		arg0 = (unsigned int)&IDX;
		//printf("op0 is IDX when PC=%d and current instruction is %d, don't forget %d\n",pc, currentInstruction, currentInstruction&128);
		if (logging) { sprintf((char*)nameArg0, "IDX(%d)", *(int*)arg0); }
	}
	else if ((currentInstruction & 112) == 112) { //op0=r7
		arg0 = (unsigned int)&r7;
		if (logging) { sprintf((char*)nameArg0, "r7(%d)", *(int*)arg0); }
	}
	else if ((currentInstruction & 96) == 96) { //op0=r6
		arg0 = (unsigned int)&r6;
		if (logging) { sprintf((char*)nameArg0, "r6(%d)", *(int*)arg0); }
	}
	else if ((currentInstruction & 80) == 80) { //op0=r5
		arg0 = (unsigned int)&r5;
		if (logging) {
			sprintf((char*)nameArg0, "r5(%d)", *(int*)arg0);
		}
	}
	else if ((currentInstruction & 64) == 64) { //op0=r4
		arg0 = (unsigned int)&r4;
		if (logging) {
			sprintf((char*)nameArg0, "r4(%d)", *(int*)arg0);
		}
	}
	else if ((currentInstruction & 48) == 48) { //op0=r3
		arg0 = (unsigned int)&r3;
		if (logging) {
			sprintf((char*)nameArg0, "r3(%d)", *(int*)arg0);
		}
	}
	else if ((currentInstruction & 32) == 32) { //op0=r2
		arg0 = (unsigned int)&r2;
		if (logging) {
			sprintf((char*)nameArg0, "r2(%d)", *(int*)arg0);
		}
	}
	else if ((currentInstruction & 16) == 16) { //op0=r1
		arg0 = (unsigned int)&r1;
		if (logging) {
			sprintf((char*)nameArg0, "r1(%d)", *(int*)arg0);
		}
	}
	else if ((currentInstruction & 0) == 0) { //op0=r0
		arg0 = (unsigned int)&r0;
		if (logging) {
			sprintf((char*)nameArg0, "r0(%d)", *(int*)arg0);
		}
	}
	//printf("%d", arg0);

	//fetch arg1
	if ((currentInstruction & 15) == 15) { //op1=imm
		//arg1 = program[pc + 1];
		arg1 = MMUgetValP1(pc + 1);
		//itoa(arg1, (char*)nameArg1, 10);
		if (logging) {
			sprintf((char*)nameArg1, "%d", arg1);
		}
		increment += 1;
	}
	else if ((currentInstruction & 14) == 14) { //op1=[idx]
		//arg1 = program[IDX];
		arg1 = MMUgetPtrP1(IDX);
		if (logging) {
			sprintf((char*)nameArg1, "[IDX](%d:%d)", IDX, arg1);
		}
	}
	else if ((currentInstruction & 11) == 11) { //op1=flags
		arg1 = flags;
		if (logging) {
			sprintf((char*)nameArg1, "FLAGS(%d)", arg1);
		}
	}
	else if ((currentInstruction & 10) == 10) { //op1=sp
		arg1 = sp;
		if (logging) {
			sprintf((char*)nameArg1, "SP(%d)", arg1);
		}
	}
	else if ((currentInstruction & 9) == 9) { //op1=[imm]
		//arg1 = program[program[pc + 1]];
		arg1 = MMUgetValP1(MMUgetValP1(pc + 1));
		increment += 1;
		if (logging) {
			//sprintf((char*)nameArg1, "[%d](%d)", program[pc + 1], arg1);
			sprintf((char*)nameArg1, "[%d](%d)", MMUgetValP1(pc+1), arg1);
		}
	}
	else if ((currentInstruction & 8) == 8) { //op1=idx
		arg1 = IDX;
		if (logging) {
			sprintf((char*)nameArg1, "IDX(%d)", arg1);
		}
	}
	else if ((currentInstruction & 7) == 7) { //op1=r7
		arg1 = r7;
		if (logging) {
			sprintf((char*)nameArg1, "r7(%d)", arg1);
		}
	}
	else if ((currentInstruction & 6) == 6) { //op1=r6
		arg1 = r6;
		if (logging) {
			sprintf((char*)nameArg1, "r6(%d)", arg1);
		}
	}
	else if ((currentInstruction & 5) == 5) { //op1=r5
		arg1 = r5;
		if (logging) {
			sprintf((char*)nameArg1, "r5(%d)", arg1);
		}
	}
	else if ((currentInstruction & 4) == 4) { //op1=r4
		arg1 = r4;
		if (logging) {
			sprintf((char*)nameArg1, "r4(%d)", arg1);
		}
	}
	else if ((currentInstruction & 3) == 3) { //op1=r3
		arg1 = r3;
		if (logging) {
			sprintf((char*)nameArg1, "r3(%d)", arg1);
		}
	}
	else if ((currentInstruction & 2) == 2) { //op1=r2
		arg1 = r2;
		if (logging) {
			sprintf((char*)nameArg1, "r2(%d)", arg1);
		}
	}
	else if ((currentInstruction & 1) == 1) { //op1=r1
		arg1 = r1;
		if (logging) {
			sprintf((char*)nameArg1, "r1(%d)", arg1);
		}
	}
	else if ((currentInstruction & 0) == 0) { //op1=r0
		arg1 = r0;
		if (logging) {
			sprintf((char*)nameArg1, "r0(%d)", arg1);
		}
	}

	//detect and execute instruction - switch case would be better
	if ((currentInstruction & 6144) == 6144) { //INT
		if (logging) {
			printf("INT\n");
		}
		PEX2RUNNING = ~(PEX2RUNNING);
	}
	else if ((currentInstruction & 5888) == 5888) { //cmps
		if (logging) {
			printf("CMPS %s %s\n", nameArg0, nameArg1);
		}
		//printf("CMP %d %d\n", *(unsigned int*)arg0, arg1);
		flags = 0;
		if (*(int*)arg0 < arg1) {
			flags = flags | 4;
		}
		if (*(int*)arg0 == arg1) {
			flags = flags | 1;
		}
	}
	else if ((currentInstruction & 5632) == 5632) { //CLF
		if (logging) {
			printf("CLF\n");
		}
		flags = 0;
	}
	else if ((currentInstruction & 5376) == 5376) { //xor
		if (logging) {
			printf("XOR %s %s\n", nameArg0, nameArg1);
		}
		*(unsigned int*)arg0 = *(unsigned int*)arg0 ^ arg1;
		//flags?
	}
	else if ((currentInstruction & 5120) == 5120) { //or
		if (logging) {
			printf("OR %s %s\n", nameArg0, nameArg1);
		}
		*(unsigned int*)arg0 = *(unsigned int*)arg0 | arg1;
		//flags?
	}
	else if ((currentInstruction & 4684) == 4684) { //and
		printf("AND %s %s\n", nameArg0, nameArg1);
		*(unsigned int*)arg0 = *(unsigned int*)arg0 & arg1;
		//flags?
	}
	else if ((currentInstruction & 4608) == 4608) { //pop
		//printf("POP %d\n", *(unsigned int*)arg0);
		if (logging) {
			printf("POP %s\n", nameArg0);
		}
		//*(unsigned int*)arg0 = program[sp];
		*(unsigned int*)arg0 = MMUgetValP1(sp);
		sp++;
	}
	else if ((currentInstruction & 4352) == 4352) { //push
		if (logging) {
			printf("PUSH %s\n", nameArg0);
		}
		//printf("PUSH %d\n", *(unsigned int*)arg0);
		sp--;
		//program[sp] = *(unsigned int*)arg0;
		*(unsigned int*)MMUgetPtrP1(sp) = *(unsigned int*)arg0;
	}
	else if ((currentInstruction & 4096) == 4096) { //ld
		if (logging) {
			printf("LD %s %s\n", nameArg0, nameArg1);
		}
		//printf("LD %d %d\n", *(unsigned int*)arg0, arg1);
		*(unsigned int*)arg0 = arg1;
	}
	else if ((currentInstruction & 3840) == 3840) { //jlt
		if (logging) {
			printf("JLT %s\n", nameArg0);
		}
		//printf("JLT %d\n", *(unsigned int*)arg0);
		if ((flags & 4) == 4) {
			pc = *(unsigned int*)arg0;
			increment = 0;
		}
	}
	else if ((currentInstruction & 3584) == 3584) { //jgt - fixed
		if (logging) {
			printf("JGT %s\n", nameArg0);
		}
		//printf("JEQ %d\n", *(unsigned int*)arg0);
		if (!((flags & 1) == 1) && !((flags & 4) == 4)) {
			pc = *(unsigned int*)arg0;
			increment = 0;
		}
	}
	else if ((currentInstruction & 3328) == 3328) { //jeq
		if (logging) {
			printf("JEQ %s\n", nameArg0);
		}
		//printf("JEQ %d\n", *(unsigned int*)arg0);
		if ((flags & 1) == 1) {
			pc = *(unsigned int*)arg0;
			increment = 0;
		}
	}
	else if ((currentInstruction & 3072) == 3072) { //jne
		if (logging) {
			printf("JNE %s\n", nameArg0);
		}
		//printf("JNE %d\n", *(unsigned int*)arg0);
		if ((flags & 1) != 1) {
			pc = *(unsigned int*)arg0;
			increment = 0;
		}
	}
	else if ((currentInstruction & 2816) == 2816) { //jmp
		//printf("JMP %d\n", arg0);
		//printf("JMP %d\n", *(unsigned int*)arg0);
		if (logging) {
			printf("JMP %s\n", nameArg0);
		}
		pc = *(unsigned int*)arg0;
		increment = 0;
	}
	else if ((currentInstruction & 2560) == 2560) { //call
		if (logging) {
			printf("CALL %s\n", nameArg0);
		}
		//printf("CALL %d\n", *(unsigned int*)arg0);
		sp--;
		//program[sp] = pc + 1;
		*(unsigned int*)MMUgetPtrP1(sp) = pc + increment;
		pc = *(unsigned int*)arg0;
		increment = 0;
	}
	else if ((currentInstruction & 2304) == 2304) { //ret
		if (logging) {
			printf("RET\n");
		}
		//pc = program[sp];
		pc = MMUgetValP1(sp);
		sp++;
		increment = 0;
	}
	else if ((currentInstruction & 2048) == 2048) { //not
		if (logging) {
			printf("NOT %s\n", nameArg0);
		}
		*(unsigned int*)arg0 = ~(*(unsigned int*)arg0);
	}
	else if ((currentInstruction & 1792) == 1792) { //cmp
		if (logging) {
			printf("CMP %s %s\n", nameArg0, nameArg1);
		}
		//printf("CMP %d %d\n", *(unsigned int*)arg0, arg1);
		flags = 0;
		if (*(unsigned int*)arg0 < arg1) {
			flags = flags | 4;
		}
		if (*(unsigned int*)arg0 == arg1) {
			flags = flags | 1;
		}
	}
	else if ((currentInstruction & 1536) == 1536) { //ls - multishift
		if (logging) {
			printf("LS %s\n", nameArg0);
		}
		//printf("RS %d\n", *(unsigned int*)arg0);
		*(unsigned int*)arg0 = *(unsigned int*)arg0 << 1;
	}
	else if ((currentInstruction & 1280) == 1280) { //rs - multishift
		if (logging) {
			printf("RS %s\n", nameArg0);
		}
		//printf("RS %d\n", *(unsigned int*)arg0);
		*(unsigned int*)arg0 = *(unsigned int*)arg0 >> 1;
	}
	else if ((currentInstruction & 1024) == 1024) { //div
		if (1) {
			printf("DIV %s %s\n", nameArg0, nameArg1);
		}
		*(unsigned int*)arg0 = *(unsigned int*)arg0 / arg1;
	}	//flags?
	
	else if ((currentInstruction & 768) == 768) { //mul
		if (logging) {
			printf("MUL %s %s\n", nameArg0, nameArg1);
		}
		*(unsigned int*)arg0 = *(unsigned int*)arg0 * arg1;
		//flags?
	}
	else if ((currentInstruction & 512) == 512) { //sub
		if (logging) {
			printf("SUB %s %s\n", nameArg0, nameArg1);
		}
		int oldArg0 = *(unsigned int*)arg0;
		//printf("Partially-implemented SUB %d %d\n", *(unsigned int*)arg0, arg1);
		*(unsigned int*)arg0 = *(unsigned int*)arg0 - arg1;
		flags = 0;
		if ((*(unsigned int*)arg0) > oldArg0) { //set borrow flag
			flags = flags | 4;
		}
		if (*(unsigned int*)arg0 == 0) { //set zero flag
			flags = flags | 1;
		}
	}
	else if ((currentInstruction & 256) == 256) { //add
		//printf("%d ", &r0);
		if (logging) {
			printf("ADD %s %s\n", nameArg0, nameArg1);
		}
		int oldArg0 = *(unsigned int*)arg0;
		//printf("ADD %d %d\n", *(unsigned int*)arg0, arg1);
		*(unsigned int*)arg0 = *(unsigned int*)arg0 + arg1;
		flags = 0;
		//implement processor flag setting
		if ((*(unsigned int*)arg0) < oldArg0) { //set overflow flag
			flags = flags | 2;
		}
	}
	else {
		if (logging) {
			printf("NOP\n");
		}
	}

	/**printf("   ");
	for(int i = 0; i<6; i++){
		printf("%d ", program[i+95]);
	}
	printf("\n");**/

	pc += increment;
	increment = 1;
	/**if (pc > sizeof(program) / 4) {
		pc = 0;
	}**/
	opsCPU1++;
}

void cpuStepP2() { //when do you clear/reset flagsP2??????????????? - add INT to start PEX-2 execution - PIXEL PLOTTING on PEX-2?
	currentInstructionP2 = MMUgetValP2(pcP2);
	arg0P2 = 0;
	arg1P2 = 0;

	//fetch arg0
	if ((currentInstructionP2 & 240) == 240) { //op0=imm
		//arg0 = (unsigned int)&program[pcP2 + 1];
		arg0P2 = MMUgetPtrP2(pcP2 + 1);
		//itoa(*(int *)arg0P2, (char *)nameArg0P2, 10);
		if (logging) { sprintf((char*)nameArg0P2, "%d", *(int*)arg0P2); }
		incrementP2 += 1;
	}
	else if ((currentInstructionP2 & 224) == 224) {//op0=[idx]
		//arg0P2P2 = (unsigned int)&program[IDXP2];
		arg0P2 = MMUgetPtrP2(IDXP2);
		if (logging) { sprintf((char*)nameArg0P2, "[IDXP2](%d:%d)", IDXP2, *(int*)arg0P2); }
	}
	else if ((currentInstructionP2 & 176) == 176) { //op0=flagsP2
		arg0P2 = (unsigned int)&flagsP2;
		if (logging) { sprintf((char*)nameArg0P2, "FLAGS(%d)", *(int*)arg0P2); }
	}
	else if ((currentInstructionP2 & 160) == 160) { //op0=sp
		arg0P2 = (unsigned int)&spP2;
		if (logging) { sprintf((char*)nameArg0P2, "SP(%d)", *(int*)arg0P2); }
	}
	else if ((currentInstructionP2 & 144) == 144) {//op0=[imm]
		//arg0P2 = (unsigned int)&program[program[pcP2 + 1]];
		arg0P2 = MMUgetPtrP2(MMUgetValP2(pcP2 + 1));
		incrementP2 += 1;
		//if (logging) { sprintf((char*)nameArg0P2, "[%d](%d)", program[pcP2 + 1], *(int*)arg0P2); }
		if (logging) { sprintf((char*)nameArg0P2, "[%d](%d)", MMUgetValP2(pcP2 + 1), *(int*)arg0P2); }
	}
	else if ((currentInstructionP2 & 128) == 128) { //op0=idx
		arg0P2 = (unsigned int)&IDXP2;
		//printf("op0 is IDXP2 when PC=%d and current instruction is %d, don't forget %d\n",pcP2, currentInstructionP2, currentInstructionP2&128);
		if (logging) { sprintf((char*)nameArg0P2, "IDXP2(%d)", *(int*)arg0P2); }
	}
	else if ((currentInstructionP2 & 112) == 112) { //op0=r7P2
		arg0P2 = (unsigned int)&r7P2;
		if (logging) { sprintf((char*)nameArg0P2, "r7P2(%d)", *(int*)arg0P2); }
	}
	else if ((currentInstructionP2 & 96) == 96) { //op0=r6P2
		arg0P2 = (unsigned int)&r6P2;
		if (logging) { sprintf((char*)nameArg0P2, "r6P2(%d)", *(int*)arg0P2); }
	}
	else if ((currentInstructionP2 & 80) == 80) { //op0=r5P2
		arg0P2 = (unsigned int)&r5P2;
		if (logging) {
			sprintf((char*)nameArg0P2, "r5P2(%d)", *(int*)arg0P2);
		}
	}
	else if ((currentInstructionP2 & 64) == 64) { //op0=r4P2
		arg0P2 = (unsigned int)&r4P2;
		if (logging) {
			sprintf((char*)nameArg0P2, "r4P2(%d)", *(int*)arg0P2);
		}
	}
	else if ((currentInstructionP2 & 48) == 48) { //op0=r3P2
		arg0P2 = (unsigned int)&r3P2;
		if (logging) {
			sprintf((char*)nameArg0P2, "r3P2(%d)", *(int*)arg0P2);
		}
	}
	else if ((currentInstructionP2 & 32) == 32) { //op0=r2P2
		arg0P2 = (unsigned int)&r2P2;
		if (logging) {
			sprintf((char*)nameArg0P2, "r2P2(%d)", *(int*)arg0P2);
		}
	}
	else if ((currentInstructionP2 & 16) == 16) { //op0=r1P2
		arg0P2 = (unsigned int)&r1P2;
		if (logging) {
			sprintf((char*)nameArg0P2, "r1P2(%d)", *(int*)arg0P2);
		}
	}
	else if ((currentInstructionP2 & 0) == 0) { //op0=r0P2
		arg0P2 = (unsigned int)&r0P2;
		if (logging) {
			sprintf((char*)nameArg0P2, "r0P2(%d)", *(int*)arg0P2);
		}
	}
	//printf("%d", arg0P2);

	//fetch arg1P2
	if ((currentInstructionP2 & 15) == 15) { //op1=imm
		//arg1P2 = program[pcP2 + 1];
		arg1P2 = MMUgetValP2(pcP2 + 1);
		//itoa(arg1P2, (char*)nameArg1P2, 10);
		if (logging) {
			sprintf((char*)nameArg1P2, "%d", arg1P2);
		}
		incrementP2 += 1;
	}
	else if ((currentInstructionP2 & 14) == 14) { //op1=[idx]
		//arg1P2 = program[IDXP2];
		arg1P2 = MMUgetPtrP2(IDXP2);
		if (logging) {
			sprintf((char*)nameArg1P2, "[IDXP2](%d:%d)", IDXP2, arg1P2);
		}
	}
	else if ((currentInstructionP2 & 11) == 11) { //op1=flagsP2
		arg1P2 = flagsP2;
		if (logging) {
			sprintf((char*)nameArg1P2, "FLAGS(%d)", arg1P2);
		}
	}
	else if ((currentInstructionP2 & 10) == 10) { //op1=sp
		arg1P2 = spP2;
		if (logging) {
			sprintf((char*)nameArg1P2, "SP(%d)", arg1P2);
		}
	}
	else if ((currentInstructionP2 & 9) == 9) { //op1=[imm]
		//arg1P2 = program[program[pcP2 + 1]];
		arg1P2 = MMUgetValP2(MMUgetValP2(pcP2 + 1));
		incrementP2 += 1;
		if (logging) {
			//sprintf((char*)nameArg1P2, "[%d](%d)", program[pcP2 + 1], arg1P2);
			sprintf((char*)nameArg1P2, "[%d](%d)", MMUgetValP2(pcP2 + 1), arg1P2);
		}
	}
	else if ((currentInstructionP2 & 8) == 8) { //op1=idx
		arg1P2 = IDXP2;
		if (logging) {
			sprintf((char*)nameArg1P2, "IDXP2(%d)", arg1P2);
		}
	}
	else if ((currentInstructionP2 & 7) == 7) { //op1=r7P2
		arg1P2 = r7P2;
		if (logging) {
			sprintf((char*)nameArg1P2, "r7P2(%d)", arg1P2);
		}
	}
	else if ((currentInstructionP2 & 6) == 6) { //op1=r6P2
		arg1P2 = r6P2;
		if (logging) {
			sprintf((char*)nameArg1P2, "r6P2(%d)", arg1P2);
		}
	}
	else if ((currentInstructionP2 & 5) == 5) { //op1=r5P2
		arg1P2 = r5P2;
		if (logging) {
			sprintf((char*)nameArg1P2, "r5P2(%d)", arg1P2);
		}
	}
	else if ((currentInstructionP2 & 4) == 4) { //op1=r4P2
		arg1P2 = r4P2;
		if (logging) {
			sprintf((char*)nameArg1P2, "r4P2(%d)", arg1P2);
		}
	}
	else if ((currentInstructionP2 & 3) == 3) { //op1=r3P2
		arg1P2 = r3P2;
		if (logging) {
			sprintf((char*)nameArg1P2, "r3P2(%d)", arg1P2);
		}
	}
	else if ((currentInstructionP2 & 2) == 2) { //op1=r2P2
		arg1P2 = r2P2;
		if (logging) {
			sprintf((char*)nameArg1P2, "r2P2(%d)", arg1P2);
		}
	}
	else if ((currentInstructionP2 & 1) == 1) { //op1=r1P2
		arg1P2 = r1P2;
		if (logging) {
			sprintf((char*)nameArg1P2, "r1P2(%d)", arg1P2);
		}
	}
	else if ((currentInstructionP2 & 0) == 0) { //op1=r0P2
		arg1P2 = r0P2;
		if (logging) {
			sprintf((char*)nameArg1P2, "r0P2(%d)", arg1P2);
		}
	}

	//detect and execute instruction - switch case would be better
	if ((currentInstructionP2 & 6144) == 6144) { //INT
		if (logging) {
			printf("INT\n");
		}
		PEX2RUNNING = ~(PEX2RUNNING);
	}
	else if ((currentInstructionP2 & 5888) == 5888) { //cmps
		if (logging) {
			printf("CMPS %s %s\n", nameArg0P2, nameArg1P2);
		}
		//printf("CMP %d %d\n", *(unsigned int*)arg0P2, arg1P2);
		flagsP2 = 0;
		if (*(int*)arg0P2 < arg1P2) {
			flagsP2 = flagsP2 | 4;
		}
		if (*(int*)arg0P2 == arg1P2) {
			flagsP2 = flagsP2 | 1;
		}
	}
	else if ((currentInstructionP2 & 5632) == 5632) { //CLF
		if (logging) {
			printf("CLF\n");
		}
		flagsP2 = 0;
	}
	else if ((currentInstructionP2 & 5376) == 5376) { //xor
		if (logging) {
			printf("XOR %s %s\n", nameArg0P2, nameArg1P2);
		}
		*(unsigned int*)arg0P2 = *(unsigned int*)arg0P2 ^ arg1P2;
		//flagsP2?
	}
	else if ((currentInstructionP2 & 5120) == 5120) { //or
		if (logging) {
			printf("OR %s %s\n", nameArg0P2, nameArg1P2);
		}
		*(unsigned int*)arg0P2 = *(unsigned int*)arg0P2 | arg1P2;
		//flagsP2?
	}
	else if ((currentInstructionP2 & 4684) == 4684) { //and
		printf("AND %s %s\n", nameArg0P2, nameArg1P2);
		*(unsigned int*)arg0P2 = *(unsigned int*)arg0P2 & arg1P2;
		//flagsP2?
	}
	else if ((currentInstructionP2 & 4608) == 4608) { //pop
		//printf("POP %d\n", *(unsigned int*)arg0P2);
		if (logging) {
			printf("POP %s\n", nameArg0P2);
		}
		//*(unsigned int*)arg0P2 = program[sp];
		*(unsigned int*)arg0P2 = MMUgetValP2(sp);
		spP2++;
	}
	else if ((currentInstructionP2 & 4352) == 4352) { //push
		if (logging) {
			printf("PUSH %s\n", nameArg0P2);
		}
		//printf("PUSH %d\n", *(unsigned int*)arg0P2);
		spP2--;
		//program[sp] = *(unsigned int*)arg0P2;
		*(unsigned int*)MMUgetPtrP2(sp) = *(unsigned int*)arg0P2;
	}
	else if ((currentInstructionP2 & 4096) == 4096) { //ld
		if (logging) {
			printf("LD %s %s\n", nameArg0P2, nameArg1P2);
		}
		//printf("LD %d %d\n", *(unsigned int*)arg0P2, arg1P2);
		*(unsigned int*)arg0P2 = arg1P2;
	}
	else if ((currentInstructionP2 & 3840) == 3840) { //jlt
		if (logging) {
			printf("JLT %s\n", nameArg0P2);
		}
		//printf("JLT %d\n", *(unsigned int*)arg0P2);
		if ((flagsP2 & 4) == 4) {
			pcP2 = *(unsigned int*)arg0P2;
			incrementP2 = 0;
		}
	}
	else if ((currentInstructionP2 & 3584) == 3584) { //jgt - fixed
		if (logging) {
			printf("JGT %s\n", nameArg0P2);
		}
		//printf("JEQ %d\n", *(unsigned int*)arg0P2);
		if (!((flagsP2 & 1) == 1) && !((flagsP2 & 4) == 4)) {
			pcP2 = *(unsigned int*)arg0P2;
			incrementP2 = 0;
		}
	}
	else if ((currentInstructionP2 & 3328) == 3328) { //jeq
		if (logging) {
			printf("JEQ %s\n", nameArg0P2);
		}
		//printf("JEQ %d\n", *(unsigned int*)arg0P2);
		if ((flagsP2 & 1) == 1) {
			pcP2 = *(unsigned int*)arg0P2;
			incrementP2 = 0;
		}
	}
	else if ((currentInstructionP2 & 3072) == 3072) { //jne
		if (logging) {
			printf("JNE %s\n", nameArg0P2);
		}
		//printf("JNE %d\n", *(unsigned int*)arg0P2);
		if ((flagsP2 & 1) != 1) {
			pcP2 = *(unsigned int*)arg0P2;
			incrementP2 = 0;
		}
	}
	else if ((currentInstructionP2 & 2816) == 2816) { //jmp
		//printf("JMP %d\n", arg0P2);
		//printf("JMP %d\n", *(unsigned int*)arg0P2);
		if (logging) {
			printf("JMP %s\n", nameArg0P2);
		}
		pcP2 = *(unsigned int*)arg0P2;
		incrementP2 = 0;
	}
	else if ((currentInstructionP2 & 2560) == 2560) { //call
		if (logging) {
			printf("CALL %s\n", nameArg0P2);
		}
		//printf("CALL %d\n", *(unsigned int*)arg0P2);
		spP2--;
		//program[sp] = pcP2 + 1;
		*(unsigned int*)MMUgetPtrP2(sp) = pcP2 + incrementP2;
		pcP2 = *(unsigned int*)arg0P2;
		incrementP2 = 0;
	}
	else if ((currentInstructionP2 & 2304) == 2304) { //ret
		if (logging) {
			printf("RET\n");
		}
		//pcP2 = program[sp];
		pcP2 = MMUgetValP2(sp);
		spP2++;
		incrementP2 = 0;
	}
	else if ((currentInstructionP2 & 2048) == 2048) { //not
		if (logging) {
			printf("NOT %s\n", nameArg0P2);
		}
		*(unsigned int*)arg0P2 = ~(*(unsigned int*)arg0P2);
	}
	else if ((currentInstructionP2 & 1792) == 1792) { //cmp
		if (logging) {
			printf("CMP %s %s\n", nameArg0P2, nameArg1P2);
		}
		//printf("CMP %d %d\n", *(unsigned int*)arg0P2, arg1P2);
		flagsP2 = 0;
		if (*(unsigned int*)arg0P2 < arg1P2) {
			flagsP2 = flagsP2 | 4;
		}
		if (*(unsigned int*)arg0P2 == arg1P2) {
			flagsP2 = flagsP2 | 1;
		}
	}
	else if ((currentInstructionP2 & 1536) == 1536) { //ls - multishift
		if (logging) {
			printf("LS %s\n", nameArg0P2);
		}
		//printf("RS %d\n", *(unsigned int*)arg0P2);
		*(unsigned int*)arg0P2 = *(unsigned int*)arg0P2 << 1;
	}
	else if ((currentInstructionP2 & 1280) == 1280) { //rs - multishift
		if (logging) {
			printf("RS %s\n", nameArg0P2);
		}
		//printf("RS %d\n", *(unsigned int*)arg0P2);
		*(unsigned int*)arg0P2 = *(unsigned int*)arg0P2 >> 1;
	}
	else if ((currentInstructionP2 & 1024) == 1024) { //div
		if (1) {
			printf("DIV %s %s\n", nameArg0P2, nameArg1P2);
		}
		*(unsigned int*)arg0P2 = *(unsigned int*)arg0P2 / arg1P2;
	}	//flagsP2?

	else if ((currentInstructionP2 & 768) == 768) { //mul
		if (logging) {
			printf("MUL %s %s\n", nameArg0P2, nameArg1P2);
		}
		*(unsigned int*)arg0P2 = *(unsigned int*)arg0P2 * arg1P2;
		//flagsP2?
	}
	else if ((currentInstructionP2 & 512) == 512) { //sub
		if (logging) {
			printf("SUB %s %s\n", nameArg0P2, nameArg1P2);
		}
		int oldArg0 = *(unsigned int*)arg0P2;
		//printf("Partially-implemented SUB %d %d\n", *(unsigned int*)arg0P2, arg1P2);
		*(unsigned int*)arg0P2 = *(unsigned int*)arg0P2 - arg1P2;
		flagsP2 = 0;
		if ((*(unsigned int*)arg0P2) > oldArg0) { //set borrow flag
			flagsP2 = flagsP2 | 4;
		}
		if (*(unsigned int*)arg0P2 == 0) { //set zero flag
			flagsP2 = flagsP2 | 1;
		}
	}
	else if ((currentInstructionP2 & 256) == 256) { //add
		//printf("%d ", &r0P2);
		if (logging) {
			printf("ADD %s %s\n", nameArg0P2, nameArg1P2);
		}
		int oldArg0 = *(unsigned int*)arg0P2;
		//printf("ADD %d %d\n", *(unsigned int*)arg0P2, arg1P2);
		*(unsigned int*)arg0P2 = *(unsigned int*)arg0P2 + arg1P2;
		flagsP2 = 0;
		//implement processor flag setting
		if ((*(unsigned int*)arg0P2) < oldArg0) { //set overflow flag
			flagsP2 = flagsP2 | 2;
		}
	}
	else {
		if (logging) {
			printf("NOP\n");
		}
	}

	/**printf("   ");
	for(int i = 0; i<6; i++){
		printf("%d ", program[i+95]);
	}
	printf("\n");**/

	pcP2 += incrementP2;
	incrementP2 = 1;
	/**if (pcP2 > sizeof(program) / 4) {
		pcP2 = 0;
	}**/
	opsCPU2++;
}


void convert(short* original, char* RGBA) {
	for (int i = 0; i < (512*224); i++) {
		RGBA[i * 4] = (original[i] & 31) << 3;
		RGBA[i * 4 + 1] = (original[i] & 992) >> 2;
		RGBA[i * 4 + 2] = (original[i] & 31744) >> 7;
		RGBA[i * 4 + 3] = 255;
	}
	//int i = (512 * 224) - 1;
	//printf("B:%d G:%d R:%d\n", original[i] & 31, original[i] & 992, original[i] & 31744);
}

int main(int argc, char* args[]) //implement sound DAC
{
	char fileName[100] = "C:\\Users\\Will\\Documents\\UFC32-main-1\\UFC32-main\\compiler\\out.bin";
	printf("Loading %s\n", fileName);
	//printf("Loading %s\n", args[1]);

	FILE *fp = fopen(fileName, "rb");
	//FILE* fp = fopen(args[1], "rb");
	fseek(fp, 0L, SEEK_END);
	int sz = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
	fread(program, 4, sz/4, fp);

	SDL_Window* window = NULL;
	SDL_Surface* screenSurface = NULL;
	SDL_Init(SDL_INIT_VIDEO);
	window = SDL_CreateWindow("UFC32 Emulator v0.5", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
	SDL_Event e;
	SDL_Thread* thread = SDL_CreateThread(threadFunction, "PEX1", (void*)0);
	SDL_Thread* counter = SDL_CreateThread(counterFunc, "Count", (void*)0);
	short* pixelData = (short*)&PEX2RAM[114688];
	while (!quit) {
		screenSurface = SDL_GetWindowSurface(window);
		SDL_LockSurface(screenSurface);
		convert(pixelData, pixels);
		SDL_memcpy(screenSurface->pixels, pixels, screenSurface->h * screenSurface->pitch);
		SDL_UnlockSurface(screenSurface);
		SDL_UpdateWindowSurface(window);
		//add++;

		while (SDL_PollEvent(&e) != 0) {
			if (e.type == SDL_QUIT)
			{
				quit = true;
			}
			else if (e.type == SDL_KEYDOWN) {
				switch (e.key.keysym.sym) {
					case 32:
						//printf("Space pressed!\n");
						joypress = joypress | 1;
						break;
					case 13:
						joypress = joypress | 2;
						break;
					case 101:
						joypress = joypress | 4;
						break;
					case 113:
						joypress = joypress | 8;
						break;
					case 118:
						joypress = joypress | 16;
						break;
					case 99:
						joypress = joypress | 32;
						break;
					case 120:
						joypress = joypress | 64;
						break;
					case 122:
						joypress = joypress | 128;
						break;
					case 100:
						joypress = joypress | 256;
						break;
					case 79:
						joypress = joypress | 256;
						break;
					case 80:
						joypress = joypress | 512;
						break;
					case 97:
						joypress = joypress | 512;
						break;
					case 81:
						joypress = joypress | 1024;
						break;
					case 115:
						joypress = joypress | 1024;
						break;
					case 82:
						joypress = joypress | 2048;
						break;
					case 119:
						joypress = joypress | 2048;
						break;
				}
			}
			else if (e.type == SDL_KEYUP) {
				switch (e.key.keysym.sym) {
				case 32:
					//printf("Space pressed!\n");
					joypress = joypress & 4094;
					break;
				case 13:
					joypress = joypress & 4093;
					break;
				case 101:
					joypress = joypress & 4091;
					break;
				case 113:
					joypress = joypress & 4087;
					break;
				case 118:
					joypress = joypress & 4079;
					break;
				case 99:
					joypress = joypress & 4063;
					break;
				case 120:
					joypress = joypress & 4031;
					break;
				case 122:
					joypress = joypress & 3967;
					break;
				case 100:
					joypress = joypress & 3839;
					break;
				case 79:
					joypress = joypress & 3839;
					break;
				case 80:
					joypress = joypress & 3583;
					break;
				case 97:
					joypress = joypress & 3583;
					break;
				case 81:
					joypress = joypress & 3071;
					break;
				case 115:
					joypress = joypress & 3071;
					break;
				case 82:
					joypress = joypress & 2047;
					break;
				case 119:
					joypress = joypress & 2047;
					break;
				}
			}
		}
	}

	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
