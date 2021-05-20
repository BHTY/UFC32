import sys

text = """ADD r0 1
LD IDX LBL1
CALL IDX
LD IDX 0
JMP IDX
LBL1: LD IDX LBL2
CALL IDX
RET
LBL2: RET"""

text = """LD r0 data
LD r1 0
LD r2 6
LD IDX quicksort
CALL IDX
infloop: LD IDX infloop
JMP IDX

quicksort: CMP r2 r1
LD IDX escape
JLT IDX
PUSH r0
PUSH r1
PUSH r2
LD IDX partition
CALL IDX
LD r4 r0
POP r2
POP r1
POP r0

PUSH r0
PUSH r1
PUSH r2
PUSH r4
LD r2 r4
LD IDX quicksort
CALL IDX
POP r4
POP r2
POP r1
POP r0

LD r4 r1
ADD r1 1
LD IDX quicksort
CALL IDX
RET

partition: PUSH r1
ADD r1 r2
RS r1
LD IDX r0
ADD IDX r1
LD r3 [IDX]
POP r1
SUB r1 1
ADD r2 1

loop: LD r1 r1

innerloop1: ADD r1 1
LD IDX r0
ADD IDX r1
LD r4 [IDX]
CMP r3 r4
LD IDX innerloop1
JLT IDX

innerloop2: SUB r2 1
LD IDX r0
ADD IDX r2
LD r4 [IDX]
CMP r4 r3
LD IDX innerloop2
JLT IDX

CMP r2 r1
LD IDX escape2
JLT IDX
JEQ IDX

LD IDX r0
ADD IDX r1
LD r7 [IDX]
LD IDX r0
ADD IDX r2
LD r6 [IDX]
LD [IDX] r7
LD IDX r0
ADD IDX r1
LD [IDX] r6

LD IDX loop
JMP IDX

escape2: LD r0 r2
escape: RET

data: DB 0
DB 4
DB 1
DB 3
DB 5
DB 2"""

consts = {"r0": 0, "r1": 1, "r2": 2, "r3": 3, "r4": 4, "r5": 5, "r6": 6,
          "r7": 7, "IDX": 8, "SP": 10, "[IDX]": 14}
instructions = {"LD": 0x1000, "ADD": 0x100, "SUB": 0x200, "MUL": 0x300,
                "DIV": 0x400, "RS": 0x500, "LS": 0x600, "CMP": 0x700,
                "NOT": 0x800, "RET": 0x900, "CALL": 0xa00, "JMP": 0xb00,
                "JNE": 0xc00, "JLT": 0xf00, "PUSH": 0x1100, "POP": 0x1200,
                "JEQ": 0xd00, "JGT": 0xe00, "AND": 0x1300, "OR": 0x1400,
                "XOR": 0x1500}

inv_map = {v: k for k, v in instructions.items()}
d_sorted = {key:value for key, value in sorted(inv_map.items(), key=lambda item: int(item[0]))}

string = str

def disasm(bytes, labels={}):
    labels = {v: k for k, v in labels.items()}
    pc = 0
    increment = 1
    while True:
        inst = ""
        arg0 = ""
        arg1 = ""

        try:
            label = labels[pc]
        except: label = ""

        if (bytes[pc] & 240) == 240: #op0=imm
            arg0 = string(bytes[pc+1])
            increment += 1
        elif (bytes[pc] & 224) == 224: #op0=[idx]
            arg0 = "[IDX]"
        elif (bytes[pc] & 144) == 144: #op0=[imm]
            arg0 = "[" + string(bytes[pc+1]) + "]"
            increment += 1
        elif (bytes[pc] & 128) == 128: #op0=idx
            arg0 = "IDX"
        else:
            arg0 = "r{}".format(int((bytes[pc] & 240)/16))

        if (bytes[pc] & 15) == 15: #op0=imm
            arg1 = string(bytes[pc+1])
            increment += 1
        elif (bytes[pc] & 14) == 14: #op0=[idx]
            arg1 = "[IDX]"
        elif (bytes[pc] & 9) == 9: #op0=[imm]
            arg1 = "[" + string(bytes[pc+1]) + "]"
            increment += 1
        elif (bytes[pc] & 8) == 8: #op0=idx
            arg1 = "IDX"
        else:
            arg1 = "r{}".format(int(bytes[pc] & 15))

        #if (bytes[pc] & 4608)
        ist = list(d_sorted.items())
        i = len(ist)

        while True:
            i -= 1
            if i == -1: break

            instr = ist[i][0]
            
            if (bytes[pc]&instr)==instr:
                inst = d_sorted[instr]
                break
            

        print("{}: {} {} {} {}".format(pc, inst, arg0, arg1, label))
        pc += increment
        increment = 1
        if pc == len(bytes): break

def assemble(string): #todo - DEFINE macros and CONSTANTS and DB statements
    string = string.split("\n")
    labels = {}
    pc = 0
    program = []

    #first pass to detect labels
    for i in string:
        instsize = 1
        temp = i.split(" ")
        if "DB" in temp:
            pass
        elif len(temp) > 2:
            if temp[0][-1] != ":":
                if temp[2] not in consts:
                    instsize += 1
            else:
                if len(temp) == 4:
                    if temp[3] not in consts:
                        instsize += 1
                if len(temp) == 3:
                    if temp[2] not in consts:
                        instsize += 1
        if temp[0] != "":
            if temp[0][-1] == ":":
                labels[temp[0][0:-1]] = pc
        elif len(temp) == 2:
            if temp[1] not in consts:
                instsize += 1
        else: instsize = 0
        print(i, " instruction size ", instsize)
        pc += instsize

    pc = 0
    #return labels

    #second pass to assemble code
    for i in string:
        #print("{}: {}".format(pc, i))
        instsize = 1
        temp = i.split(" ")

        if len(temp) == 4: #inst with two ops and a label
            if temp[2] in consts:
                if temp[3] in consts:
                    program.append((instructions[temp[1]] + (consts[temp[2]] << 4) + consts[temp[3]]))
                elif temp[3][0] != "[": #op1=imm
                    program.append((instructions[temp[1]] + (consts[temp[2]] << 4) + 15))
                    try: program.append(int(temp[3]))
                    except: program.append(labels[temp[3]])
                else: #op1=[imm]
                    program.append((instructions[temp[1]] + (consts[temp[2]] << 4) + 9))
                    program.append(int(temp[3][1:-1]))
                    
            elif temp[2][0] != "[": #op0=imm
                program.append(instructions[temp[1]] + consts[temp[3]] + (15 << 4))
                program.append(int(temp[2]))
                
            else: #op0=[imm]
                program.append(instructions[temp[1]] + consts[temp[3]] + (9 << 4))
                program.append(int(temp[2][1:-1]))

        
        if len(temp) == 3:
            #detect if it's a 2-length inst with a label
            if temp[0][-1] == ":":
                if temp[2] in consts: #non-immediate value
                    program.append((instructions[temp[1]] + (consts[temp[2]] << 4)))
                elif temp[1] == "DB": #define dword
                    if temp[2].isdigit():
                        program.append(int(temp[2]))
                    if temp[2] in labels:
                        program.append(labels[temp[2]])
                elif temp[2][0] != "[": #imm
                    program.append((instructions[temp[1]] + (15 << 4)))
                    try: program.append(int(temp[2]))
                    except: program.append(labels[temp[2]])
                else: #[imm]
                    program.append((instructions[temp[1]] + (9 << 4)))
                    program.append(int(temp[2][1:-1]))
            
            #instruction with two operands
            else:
                if temp[1] in consts:
                    if temp[2] in consts:
                        program.append((instructions[temp[0]] + (consts[temp[1]] << 4) + consts[temp[2]]))
                    elif temp[2][0] != "[": #op1=imm
                        program.append((instructions[temp[0]] + (consts[temp[1]] << 4) + 15))
                        try: program.append(int(temp[2]))
                        except: program.append(labels[temp[2]])
                    else: #op1=[imm]
                        program.append((instructions[temp[0]] + (consts[temp[1]] << 4) + 9))
                        program.append(int(temp[2][1:-1]))
                    
                elif temp[1][0] != "[": #op0=imm
                    program.append(instructions[temp[0]] + consts[temp[2]] + (15 << 4))
                    program.append(int(temp[1]))
                
                else: #op0=[imm]
                    program.append(instructions[temp[0]] + consts[temp[2]] + (9 << 4))
                    program.append(int(temp[1][1:-1]))

        
        if len(temp) == 2:
            #detect if it's a 1-length inst with a label
            if temp[0][-1] == ":":
                program.append(instructions[temp[1]])

            elif temp[0] == "DB": #define dword
                if temp[1].isdigit():
                    program.append(int(temp[1]))
                if temp[1] in labels:
                    program.append(labels[temp[1]])
                
            else: #instruction with just op0
                if temp[1] in consts: #non-immediate value
                    program.append((instructions[temp[0]] + (consts[temp[1]] << 4)))
                elif temp[1][0] != "[": #imm
                    program.append((instructions[temp[0]] + (15 << 4)))
                    try: program.append(int(temp[1]))
                    except: program.append(labels[temp[1]])
                else: #[imm]
                    program.append((instructions[temp[0]] + (9 << 4)))
                    program.append(int(temp[1][1:-1]))

                    
        if len(temp) == 1 and temp[0] != "":
            program.append(instructions[temp[0]])
        pc+=1

    return program, labels

file = open(sys.argv[1], "r+")
text = file.read()
file.close()
binary, labels = assemble(text)
print(disasm(binary, labels))
newbin = []

#convert binary to 8-bit
for i in binary:
    lowest = i & 255 #least signifigant bits
    lower = (i & 65280) >> 8 #second-least
    low = (i & 16711680) >> 16 #second highest
    highest = (i & 4278190080) >> 24 #highest
    #print(i)

    #newbin.append(highest)
    #newbin.append(low)
    #newbin.append(lower)
    #newbin.append(lowest)
    newbin.append(lowest)
    newbin.append(lower)
    newbin.append(low)
    newbin.append(highest)

file = open(sys.argv[2], "wb")
file.write(bytes(newbin))
file.close()

i = 0

thing = bytes(newbin)
extra = []

while i < len(thing):
    #bin = thing[i] + thing[i+1]*256 + thing[i+2]*65536 + thing[i+3]*(2**24)
    bin = thing[i]*(2**24) + thing[i+1]*65536 + thing[i+2]*256 + thing[i+3]
    extra.append(bin)
    i += 4
#print(extra==binary)

for i in range(len(binary)):
    #print("{} | {}".format(binary[i], extra[i]))
    pass

"""file = open(sys.argv[2], "rb")
info = file.read()

for i in info:
    print(i)

file.close()"""
