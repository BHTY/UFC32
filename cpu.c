#include <stdio.h>
#include <conio.h>
#include <stdlib.h>

unsigned int program[16384];
unsigned int r0 = 0;
unsigned int r1 = 0;
unsigned int r2 = 0;
unsigned int r3 = 0;
unsigned int r4 = 0;
unsigned int r5 = 0;
unsigned int r6 = 0;
unsigned int r7 = 0;
unsigned int sp = 1023;
unsigned int pc = 0;
unsigned int IDX = 0;
unsigned int flags = 0;
unsigned int currentInstruction;
unsigned int increment = 1;
unsigned int arg1;
unsigned int arg0; //long on x64
unsigned char nameArg0[20] = "";
unsigned char nameArg1[20] = "";

void cpuStep() { //when do you clear/reset flags???????????????
	currentInstruction = program[pc];
	arg0 = 0;
	arg1 = 0;

	//fetch arg0
	if ((currentInstruction & 240) == 240) { //op0=imm
		arg0 = (unsigned int)&program[pc + 1];
		//itoa(*(int *)arg0, (char *)nameArg0, 10);
		sprintf((char *)nameArg0, "%d", *(int *)arg0);
		increment += 1;
	}
	else if ((currentInstruction & 224) == 224) {//op0=[idx]
		arg0 = (unsigned int)&program[IDX];
		sprintf((char *)nameArg0, "[IDX](%d:%d)",IDX, *(int*)arg0);
	}
	else if ((currentInstruction & 176) == 176) { //op0=flags
		arg0 = (unsigned int)&flags;
		sprintf((char*)nameArg0, "FLAGS(%d)", *(int*)arg0);
	}
	else if ((currentInstruction & 160) == 160) { //op0=sp
		arg0 = (unsigned int)&sp;
		sprintf((char*)nameArg0, "SP(%d)", *(int*)arg0);
	}
	else if ((currentInstruction & 144) == 144) {//op0=[imm]
		arg0 = (unsigned int)&program[program[pc + 1]];
		increment += 1;
		sprintf((char*)nameArg0, "[%d](%d)", program[pc+1], *(int*)arg0);
	}
	else if ((currentInstruction & 128) == 128) { //op0=idx
		arg0 = (unsigned int)&IDX;
		//printf("op0 is IDX when PC=%d and current instruction is %d, don't forget %d\n",pc, currentInstruction, currentInstruction&128);
		sprintf((char*)nameArg0, "IDX(%d)", *(int*)arg0);
	}
	else if ((currentInstruction & 112) == 112) { //op0=r7
		arg0 = (unsigned int)&r7;
		sprintf((char*)nameArg0, "r7(%d)", *(int*)arg0);
	}
	else if ((currentInstruction & 96) == 96) { //op0=r6
		arg0 = (unsigned int)&r6;
		sprintf((char*)nameArg0, "r6(%d)", *(int*)arg0);
	}
	else if ((currentInstruction & 80) == 80) { //op0=r5
		arg0 = (unsigned int)&r5;
		sprintf((char*)nameArg0, "r5(%d)", *(int*)arg0);
	}
	else if ((currentInstruction & 64) == 64) { //op0=r4
		arg0 = (unsigned int)&r4;
		sprintf((char*)nameArg0, "r4(%d)", *(int*)arg0);
	}
	else if ((currentInstruction & 48) == 48) { //op0=r3
		arg0 = (unsigned int)&r3;
		sprintf((char*)nameArg0, "r3(%d)", *(int*)arg0);
	}
	else if ((currentInstruction & 32) == 32) { //op0=r2
		arg0 = (unsigned int)&r2;
		sprintf((char*)nameArg0, "r2(%d)", *(int*)arg0);
	}
	else if ((currentInstruction & 16) == 16) { //op0=r1
		arg0 = (unsigned int)&r1;
		sprintf((char*)nameArg0, "r1(%d)", *(int*)arg0);
	}
	else if ((currentInstruction & 0) == 0) { //op0=r0
		arg0 = (unsigned int)&r0;
		sprintf((char*)nameArg0, "r0(%d)", *(int*)arg0);
	}
	//printf("%d", arg0);

	//fetch arg1
	if ((currentInstruction & 15) == 15) { //op1=imm
		arg1 = program[pc + 1];
		//itoa(arg1, (char*)nameArg1, 10);
		sprintf((char*)nameArg1, "%d", arg1);
		increment += 1;
	}
	else if ((currentInstruction & 14) == 14) { //op1=[idx]
		arg1 = program[IDX];
		sprintf((char*)nameArg1, "[IDX](%d:%d)", IDX, arg1);
	}
	else if ((currentInstruction & 11) == 11) { //op1=flags
		arg1 = flags;
		sprintf((char*)nameArg1, "FLAGS(%d)", arg1);
	}
	else if ((currentInstruction & 10) == 10) { //op1=sp
		arg1 = sp;
		sprintf((char*)nameArg1, "SP(%d)", arg1);
	}
	else if ((currentInstruction & 9) == 9) { //op1=[imm]
		arg1 = program[program[pc + 1]];
		increment += 1;
		sprintf((char*)nameArg1, "[%d](%d)", program[pc + 1], arg1);
	}
	else if ((currentInstruction & 8) == 8) { //op1=idx
		arg1 = IDX;
		sprintf((char*)nameArg1, "IDX(%d)", arg1);
	}
	else if ((currentInstruction & 7) == 7) { //op1=r7
		arg1 = r7;
		sprintf((char*)nameArg1, "r7(%d)", arg1);
	}
	else if ((currentInstruction & 6) == 6) { //op1=r6
		arg1 = r6;
		sprintf((char*)nameArg1, "r6(%d)", arg1);
	}
	else if ((currentInstruction & 5) == 5) { //op1=r5
		arg1 = r5;
		sprintf((char*)nameArg1, "r5(%d)", arg1);
	}
	else if ((currentInstruction & 4) == 4) { //op1=r4
		arg1 = r4;
		sprintf((char*)nameArg1, "r4(%d)", arg1);
	}
	else if ((currentInstruction & 3) == 3) { //op1=r3
		arg1 = r3;
		sprintf((char*)nameArg1, "r3(%d)", arg1);
	}
	else if ((currentInstruction & 2) == 2) { //op1=r2
		arg1 = r2;
		sprintf((char*)nameArg1, "r2(%d)", arg1);
	}
	else if ((currentInstruction & 1) == 1) { //op1=r1
		arg1 = r1;
		sprintf((char*)nameArg1, "r1(%d)", arg1);
	}
	else if ((currentInstruction & 0) == 0) { //op1=r0
		arg1 = r0;
		sprintf((char*)nameArg1, "r0(%d)", arg1);
	}

	//detect and execute instruction - switch case would be better
	if ((currentInstruction & 5632) == 5632) { //CLF
		printf("CLF\n");
		flags = 0;
	}
	else if ((currentInstruction & 5376) == 5376) { //xor
		printf("XOR %s %s\n", nameArg0, nameArg1);
		*(unsigned int*)arg0 = *(unsigned int*)arg0 ^ arg1;
		//flags?
	}
	else if ((currentInstruction & 5120) == 5120) { //or
		printf("OR %s %s\n", nameArg0, nameArg1);
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
		printf("POP %s\n", nameArg0);
		*(unsigned int*)arg0 = program[sp];
		sp++;
	}
	else if ((currentInstruction & 4352) == 4352) { //push
		printf("PUSH %s\n", nameArg0);
		//printf("PUSH %d\n", *(unsigned int*)arg0);
		sp--;
		program[sp] = *(unsigned int*)arg0;
	}
	else if ((currentInstruction & 4096) == 4096) { //ld
		printf("LD %s %s\n", nameArg0, nameArg1);
		//printf("LD %d %d\n", *(unsigned int*)arg0, arg1);
		*(unsigned int*)arg0 = arg1;
	}
	else if ((currentInstruction & 3840) == 3840) { //jlt
		printf("JLT %s\n", nameArg0);
		//printf("JLT %d\n", *(unsigned int*)arg0);
		if ((flags & 4) == 4) {
			pc = *(unsigned int*)arg0;
			increment = 0;
		}
	}
	else if ((currentInstruction & 3584) == 3584) { //jgt - fix this
		printf("JGT %s\n", nameArg0);
		//printf("JEQ %d\n", *(unsigned int*)arg0);
		if (!((flags&1)==1) && !((flags&4)==4)) {
			pc = *(unsigned int*)arg0;
			increment = 0;
		}
	}
	else if ((currentInstruction & 3328) == 3328) { //jeq
		printf("JEQ %s\n", nameArg0);
		//printf("JEQ %d\n", *(unsigned int*)arg0);
		if ((flags & 1) == 1) {
			pc = *(unsigned int*)arg0;
			increment = 0;
		}
	}
	else if ((currentInstruction & 3072) == 3072) { //jne
		printf("JNE %s\n", nameArg0);
		//printf("JNE %d\n", *(unsigned int*)arg0);
		if ((flags & 1) != 1) {
			pc = *(unsigned int*)arg0;
			increment = 0;
		}
	}
	else if ((currentInstruction & 2816) == 2816) { //jmp
		//printf("JMP %d\n", arg0);
		//printf("JMP %d\n", *(unsigned int*)arg0);
		printf("JMP %s\n", nameArg0);
		pc = *(unsigned int*)arg0;
		increment = 0;
	}
	else if ((currentInstruction & 2560) == 2560) { //call
		printf("CALL %s\n", nameArg0);
		//printf("CALL %d\n", *(unsigned int*)arg0);
		sp--;
		program[sp] = pc + 1;
		pc = *(unsigned int*)arg0;
		increment = 0;
	}
	else if ((currentInstruction & 2304) == 2304) { //ret
		printf("RET\n");
		pc = program[sp];
		sp++;
		increment = 0;
	}
	else if ((currentInstruction & 2048) == 2048) { //not
		printf("NOT %s\n",nameArg0);
		*(unsigned int*)arg0 = ~(*(unsigned int*)arg0);
	}
	else if ((currentInstruction & 1792) == 1792) { //cmp
		printf("CMP %s %s\n", nameArg0, nameArg1);
		//printf("CMP %d %d\n", *(unsigned int*)arg0, arg1);
		flags = 0;
		if (*(unsigned int*)arg0 < arg1) {
			flags = flags | 4;
		}
		if (*(unsigned int*)arg0 == arg1) {
			flags = flags | 1;
		}
	}
	else if ((currentInstruction & 1536) == 1536) { //ls
		printf("LS %s\n", nameArg0);
		//printf("RS %d\n", *(unsigned int*)arg0);
		*(unsigned int*)arg0 = *(unsigned int*)arg0 << 1;
	}
	else if ((currentInstruction & 1280) == 1280) { //rs
		printf("RS %s\n", nameArg0);
		//printf("RS %d\n", *(unsigned int*)arg0);
		*(unsigned int*)arg0 = *(unsigned int*)arg0 >> 1;
	}
	else if ((currentInstruction & 1024) == 1024) { //div
		printf("DIV %s %s\n", nameArg0, nameArg1);
		*(unsigned int*)arg0 = *(unsigned int*)arg0 / arg1;
		//flags?
	}
	else if ((currentInstruction & 768) == 768) { //mul
		printf("MUL %s %s\n", nameArg0, nameArg1);
		*(unsigned int*)arg0 = *(unsigned int*)arg0 * arg1;
		//flags?
	}
	else if ((currentInstruction & 512) == 512) { //sub
		printf("SUB %s %s\n", nameArg0, nameArg1);
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
		printf("ADD %s %s\n", nameArg0, nameArg1);
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
		printf("NOP\n");
	}

	/**printf("   ");
	for(int i = 0; i<6; i++){
		printf("%d ", program[i+95]);
	}
	printf("\n");**/

	pc += increment;
	increment = 1;
}

int main(int argc, char** argv) {
	int t = 0;
	int inst = 1;
	//printf(argv[1]);
	FILE *fp = fopen(argv[1], "rb");
	fseek(fp, 0L, SEEK_END);
	int sz = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
	fread(program, 4, sz/4, fp);
	int c;
	while (1) {
		printf("PC=%u r0=%u r1=%u r2=%u r3=%u r4=%u SP=%u IDX=%u Flags=%u Value=%u  ", pc, r0, r1, r2, r3, r4, sp, IDX, flags, program[510]);
		//printf("%d: ", r0);
		cpuStep();
		if (t % inst == 0) {
			c = getch();
			if (c == 32) { exit(0); }
		}
		t++;

		/**if(pc==512){
			for(int i=0; i<30; i++){
				printf("%d\n", program[i+512]);
			}

			while(1){
			}
		}**/

		//printf("%d\n", r0);
	}
}
