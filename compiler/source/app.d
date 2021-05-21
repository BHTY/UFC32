import pegged.grammar;
import std.stdio;
import std.file;
import std.conv;
import std.format;
import std.algorithm;
import std.array;
import std.exception;
import std.range;
string[] globalSymbols;
struct LocalVar {
	string name;
	int spOffset;
}
struct FunctionEntry {
	string name;
	int numArguments;
	bool returnsVoid;
}
int spShift;
LocalVar[][] scopeStack;
FunctionEntry[] functions;
int pseudoStackSize;
int labelNum;
mixin(grammar(`PEXC:
	Program < (FunctionDecl / GlobalDecl / GlobalArrayDecl)+
	GlobalDecl < "u32" identifier ("=" IntLiteral)? ";"
	IntLiteral < ~("-"?[0-9]+)
	GlobalArrayDecl < "u32" ^identifier "[" (IntLiteral) ? "]" ("=" (ArrayLiteral / StringLiteral))? ";"
	ArrayLiteral < "{" IntLiteral ("," IntLiteral)* "}"
	StringLiteral < '\"' ~(((!'\"' .) | :'\\' '\"')*) !'\\' '\"'
	FunctionDecl < ("u32" / "void") ^identifier ArgumentList Statement
	ArgumentList < "(" (Argument ("," Argument)*)? ")"
	Argument < "u32" ^identifier
	CallArgumentList < "(" (RValue ("," RValue)*)? ")"
	Statement < BlockStatement / LocalDecl / LocalArrayDecl / ExpressionStatement ";" / ReturnStatement / InlineAsmStatement / IfStatement / WhileStatement / DoStatement / ForStatement
	LocalDecl < "u32" identifier ("=" RValue)? ";"
	LocalArrayDecl < "u32" ^identifier "[" (IntLiteral) ? "]" ("=" (ArrayLiteral / StringLiteral))? ";"
	BlockStatement < "{" Statement* "}"
	ExpressionStatement < (LValue ^("=" / "+=" / "-=" / "*=" / "/=" / "&=" / "|=" / "<<=" / ">>="))? RValue
	LValue < "*" ExprAtom / ^identifier ("[" RValue "]")*
	RValue < ExprLogOr ("?" RValue ":" RValue)?
	ExprLogOr < ExprLogAnd ("||" ExprLogOr)?
	ExprLogAnd < ExprBitOr ("&&" ExprLogAnd)?
	ExprBitOr < ExprBitXor ("|" ExprBitOr)?
	ExprBitXor < ExprBitAnd ("^" ExprBitXor)?
	ExprBitAnd < ExprEq ("&" ExprBitAnd)?
	ExprEq < ExprCmp (^("==" / "!=") ExprEq)?
	ExprCmp < ExprShift (^("<=" / ">=" / "<" / ">") ExprCmp)?
	ExprShift < ExprAdd (^("<<" / ">>") ExprShift)?
	ExprAdd < ExprMul (^("+" / "-") ExprAdd)?
	ExprMul < ExprPrefixes (^("*" / "/") ExprMul)?
	ExprPrefixes < (^("~" / "!" / "+" / "-"/ "*"))* ExprIndexed
	ExprIndexed < ExprAtom ("[" RValue "]")*
	ExprAtom < IntLiteral / "(" RValue ")" / ^identifier CallArgumentList / ("++" / "--" / "&") ^identifier / ^identifier ("++" / "--")?
	ReturnStatement < "return" RValue ";"
	InlineAsmStatement < "asm" "{" InlineAsm "}" ";"
	InlineAsm <- ~((!'}' .)*)
	IfStatement < "if" "(" RValue ")" Statement ("else" Statement)?
	WhileStatement < "while" "(" RValue ")" Statement
	DoStatement < "do" Statement "while" "(" RValue ")" ";"
	ForStatement < "for" "(" ExpressionStatement ";" RValue ";" ExpressionStatement ")" Statement`));

string[] assembly;
void main(string[] argv) {
	if (argv.length < 3) {
		writeln("Usage: " ~ argv[0] ~ " <in.pexc> <out.pexs> [parsetree.txt]");
	}
	auto tree = PEXC(readText(argv[1]));
	if (argv.length >= 4) {
		auto outfile = File(argv[3], "w");
		outfile.write(tree.to!string);
		outfile.close();
	}
	if (!tree.successful || tree.end != tree.input.length) {
		writefln("Syntax error on line %d", count(tree.input[0..tree.end], "\n") + 1);
		return;
	}
	assembly ~= ["LD IDX main","CALL IDX"];
	bfp(tree);
	auto outfile = File(argv[2], "w");
	string outtxt;
	foreach(line; assembly) {
		outtxt ~= line ~ "\n";
	}
	outfile.write(outtxt);
	outfile.close();
	
}
class CompilationException : Exception {
	mixin basicExceptionCtors;
}
void bfp(ParseTree parse) {
	switch (parse.name) {
		case "PEXC.GlobalDecl":
			assembly ~= format!"%s: DB %s"(parse.matches[1], parse.children.length == 1 ? parse[0].matches[0] : "0");
			globalSymbols ~= parse.matches[1];
			break;
		case "PEXC.GlobalArrayDecl":
			ulong arrayLength;
			if (parse.children.length == 0) {
				arrayLength = 0;
			} else if (parse[1].name == "PEXC.IntLiteral") {
				arrayLength = parse[1].matches[0].to!int;
			} else if (parse[$-1].name == "PEXC.ArrayLiteral") {
				arrayLength = parse[1].children.length;
			} else {
				arrayLength = parse[1].matches[1].length;
			}
			int[] arrayPrefix;
			if (parse.children.length != 0 && parse[$-1].name == "PEXC.ArrayLiteral") {
				arrayPrefix = parse[$-1].children.map!(a => a.matches[0].to!int).array;
			} else if (parse.children.length != 0 && parse[$-1].name == "PEXC.StringLiteral") {
				arrayPrefix = parse[$-1].matches[1].map!(a => cast(int)(a)).array;
			}
			if (arrayPrefix.length > arrayLength) {
				throw new CompilationException(format!"Array not long enough error on line %d"(count(parse.input[0..parse.end], "\n") + 1));
			}
			assembly ~= "_" ~ parse[0].matches[0] ~ "_data:";
			foreach (i; 0..arrayLength) {
				assembly ~= format!"DB %d"(i < arrayPrefix.length ? arrayPrefix[i] : 0);
			}
			assembly ~= parse[0].matches[0] ~ ":";
			assembly ~= "DB _" ~ parse[0].matches[0] ~ "_data";
			globalSymbols ~= parse[0].matches[0];
			break;
		case "PEXC.FunctionDecl":
			scopeStack ~= new LocalVar[0];
			spShift = 0;
			functions ~= FunctionEntry(parse[0].matches[0], parse[1].children.length.to!int, parse.matches[0] == "void");
			assembly ~= parse[0].matches[0] ~ ":";
			int whichReg = 0;
			if (parse[1].children.length > 0) {
				assembly ~= "LD IDX SP";
				foreach(arg; parse[1].children) {
					scopeStack[$-1] ~= LocalVar(arg.matches[1], spShift);
					if (whichReg > 7) {
						throw new CompilationException(format!"Too many arguments for %s error on line %d"(parse[0].matches[0], count(parse.input[0..parse.end], "\n") + 1));
					}
					assembly ~= format!"SUB IDX 1";
					assembly ~= format!"LD [IDX] r%d"(whichReg++);
					spShift++;
				}
				assembly ~= "LD SP IDX";
			}
			bfp(parse[2]);
			if (spShift > 0) {
				assembly ~= format!"ADD SP %d"(spShift);
			}
			assembly ~= "RET";
			scopeStack = scopeStack[1..$];
			break;
		case "PEXC.LocalDecl":
			scopeStack[$-1] ~= LocalVar(parse.matches[1], spShift);
			if (parse.children.length == 1) {
				bfp(parse[0]);
				assembly ~= format!"PUSH r%d"(--pseudoStackSize);
			} else {
				assembly ~= "PUSH 0";
			}
			spShift++;
			break;
		case "PEXC.LocalArrayDecl":
			ulong arrayLength;
			if (parse.children.length == 0) {
				arrayLength = 0;
			} else if (parse[1].name == "PEXC.IntLiteral") {
				arrayLength = parse[1].matches[0].to!int;
			} else if (parse[$-1].name == "PEXC.ArrayLiteral") {
				arrayLength = parse[1].children.length;
			} else {
				arrayLength = parse[1].matches[1].length;
			}
			int[] arrayPrefix;
			if (parse.children.length != 0 && parse[$-1].name == "PEXC.ArrayLiteral") {
				arrayPrefix = parse[$-1].children.map!(a => a.matches[0].to!int).array;
			} else if (parse.children.length != 0 && parse[$-1].name == "PEXC.StringLiteral") {
				arrayPrefix = parse[$-1].matches[1].map!(a => cast(int)(a)).array;
			}
			if (arrayPrefix.length > arrayLength) {
				throw new CompilationException(format!"Array not long enough error on line %d"(count(parse.input[0..parse.end], "\n") + 1));
			}
			foreach (i; arrayLength.iota.retro) {
				assembly ~= format!"PUSH %d"(i < arrayPrefix.length ? arrayPrefix[i] : 0);
				spShift++;
			}
			assembly ~= "LD IDX SP";
			assembly ~= "PUSH IDX";
			scopeStack[$-1] ~= LocalVar(parse[0].matches[0], spShift);
			spShift++;
			break;
		case "PEXC.BlockStatement":
			scopeStack ~= new LocalVar[0];
			int oldSpShift = spShift;
			foreach(child; parse) {
				bfp(child);
			}
			if (spShift != oldSpShift) {
				assembly ~= format!"ADD SP %d"(spShift - oldSpShift);
				spShift = oldSpShift;
			}
			scopeStack = scopeStack[1..$];
			break;
		case "PEXC.ExprAtom":
			if (pseudoStackSize > 7) {
				throw new CompilationException(format!"Too deep expression nesting error on line %d"(count(parse.input[0..parse.end], "\n") + 1));
			}
			if (parse[0].name == "PEXC.IntLiteral") {
				assembly ~= format!"LD r%d %s"(pseudoStackSize++, parse[0].matches[0]);
			} else if (parse[0].name == "PEXC.RValue") {
				bfp(parse[0]);
			} else if (parse[$-1].name == "PEXC.CallArgumentList") {
				bool foundMatch = false;
				foreach(fx; functions) {
					if (fx.name == parse[0].matches[0]) {
						int[] backedUp;
						while (pseudoStackSize) {
							assembly ~= format!"PUSH r%d"(--pseudoStackSize);
							backedUp ~= pseudoStackSize;
						}
						foreach(i; 0..fx.numArguments) {
							bfp(parse[$-1].children[i]); 
						}
						pseudoStackSize = 1;
						assembly ~= format!"LD IDX %s"(fx.name);
						assembly ~= "CALL IDX";
						if (backedUp.length > 0) {
							assembly ~= format!"LD r%d r0"(backedUp.length);
						}
						foreach(reg; backedUp.retro) {
							assembly ~= format!"POP r%d"(reg);
							pseudoStackSize++;
						}
						foundMatch = true;
						break;
					}
				}
				if (!foundMatch) {
					throw new CompilationException(format!"Undeclared function error on line %d"(count(parse.input[0..parse.end], "\n") + 1));
				}
			} else {
				string varName = parse[0].matches[0];
				bool foundMatch = false;
				foreach(scopeLevel; scopeStack.retro) {
					foreach(localVariable; scopeLevel) {
						if (localVariable.name == varName) {
							assembly ~= "LD IDX SP";
							if (spShift - localVariable.spOffset != 1) {
								assembly ~= format!"ADD IDX %d"(spShift - localVariable.spOffset - 1);
							}
							foundMatch = true;
							break;
						}
					}
					if (foundMatch) break;
				}
				if (!foundMatch) {
					if (globalSymbols.canFind(varName)) {
						assembly ~= format!"LD IDX %s"(varName);
					} else {
						throw new CompilationException(format!"Variable not declared error on line %d"(count(parse.input[0..parse.end], "\n") + 1));
					}
				}
				if (parse.matches[0] == "++") {
					assembly ~= "ADD [IDX] 1";
					assembly ~= format!"LD r%d [IDX]"(pseudoStackSize++);
				} else if (parse.matches[0] == "--") {
					assembly ~= "SUB [IDX] 1";
					assembly ~= format!"LD r%d [IDX]"(pseudoStackSize++);
				} else if (parse.matches[0] == "&") {
					assembly ~= format!"LD r%d IDX"(pseudoStackSize++);
				} else if (parse.matches[$-1] == "++") {
					assembly ~= format!"LD r%d [IDX]"(pseudoStackSize++);
					assembly ~= "ADD [IDX] 1";
				} else if (parse.matches[$-1] == "--") {
					assembly ~= format!"LD r%d [IDX]"(pseudoStackSize++);
					assembly ~= "SUB [IDX] 1";
				} else {
					assembly ~= format!"LD r%d [IDX]"(pseudoStackSize++);
				}
			}
			break;
		case "PEXC.LValue":
			if (parse[0].name == "PEXC.ExprAtom") {
				bfp(parse[0]);
			} else {
				string varName = parse[0].matches[0];
				bool foundMatch = false;
				foreach(scopeLevel; scopeStack[].retro) {
					foreach(localVariable; scopeLevel) {
						if (localVariable.name == varName) {
							assembly ~= format!"LD r%d SP"(pseudoStackSize++);
							if (spShift - localVariable.spOffset != 1) {
								assembly ~= format!"ADD r%d %d"(pseudoStackSize-1, spShift - localVariable.spOffset-1);
							}
							foundMatch = true;
							break;
						}
					}
					if (foundMatch) break;
				}
				if (!foundMatch) {
					if (globalSymbols.canFind(varName)) {
						assembly ~= format!"LD r%d %s"(pseudoStackSize++, varName);
					} else {
						throw new CompilationException(format!"Variable not declared error on line %d"(count(parse.input[0..parse.end], "\n") + 1));
					}
				}
				foreach(idx; parse[1..$]) {
					bfp(idx);
					assembly ~= format!"LD IDX r%d"(pseudoStackSize-2);
					assembly ~= format!"LD r%d [IDX]"(pseudoStackSize-2);
					assembly ~= format!"ADD r%d r%d"(pseudoStackSize-2, --pseudoStackSize);
				}
			}
			break;
		case "PEXC.ExpressionStatement":
			bfp(parse[$-1]);
			if (parse.children.length == 3) {
				bfp(parse[0]);
				switch(parse[1].matches[0]) {
					case "=":
						assembly ~= format!"LD IDX r%d"(--pseudoStackSize);
						assembly ~= format!"LD [IDX] r%d"(--pseudoStackSize);
						break;
					case "+=":
						assembly ~= format!"LD IDX r%d"(--pseudoStackSize);
						assembly ~= format!"ADD [IDX] r%d"(--pseudoStackSize);
						break;
					case "-=":
						assembly ~= format!"LD IDX r%d"(--pseudoStackSize);
						assembly ~= format!"SUB [IDX] r%d"(--pseudoStackSize);
						break;
					case "*=":
						assembly ~= format!"LD IDX r%d"(--pseudoStackSize);
						assembly ~= format!"MUL [IDX] r%d"(--pseudoStackSize);
						break;
					case "/=":
						assembly ~= format!"LD IDX r%d"(--pseudoStackSize);
						assembly ~= format!"DIV [IDX] r%d"(--pseudoStackSize);
						break;
					case "<<=":
						assembly ~= format!"LD IDX r%d"(--pseudoStackSize);
						assembly ~= format!"LS [IDX] r%d"(--pseudoStackSize);
						break;
					case ">>=":
						assembly ~= format!"LD IDX r%d"(--pseudoStackSize);
						assembly ~= format!"RS [IDX] r%d"(--pseudoStackSize);
						break;
					case "&=":
						assembly ~= format!"LD IDX r%d"(--pseudoStackSize);
						assembly ~= format!"AND [IDX] r%d"(--pseudoStackSize);
						break;
					case "|=":
						assembly ~= format!"LD IDX r%d"(--pseudoStackSize);
						assembly ~= format!"OR [IDX] r%d"(--pseudoStackSize);
						break;
					case "^=":
						assembly ~= format!"LD IDX r%d"(--pseudoStackSize);
						assembly ~= format!"XOR [IDX] r%d"(--pseudoStackSize);
						break;
					default:
						throw new CompilationException("This text should never appear, but it'll be a pain if it does.");
				}
			} else {
				pseudoStackSize--;
			}
			break;
		case "PEXC.RValue":
			bfp(parse[0]);
			if (parse.children.length == 3) {
				assembly ~= format!"CMP r%d 0"(--pseudoStackSize);
				assembly ~= format!"JEQ l%d"(labelNum++);
				int firstLabel = labelNum-1;
				bfp(parse[1]);
				assembly ~= format!"JMP l%d"(labelNum++);
				assembly ~= format!"l%d:"(firstLabel);
				pseudoStackSize--;
				bfp(parse[1]);
				assembly ~= format!"l%d:"(labelNum-1);
			}
			break;
		case "PEXC.ExprLogOr":
			bfp(parse[0]);
			if (parse.children.length == 2) {
				assembly ~= format!"CMP r%d 0"(--pseudoStackSize);
				assembly ~= format!"JNE l%d"(labelNum++);
				int firstLabel = labelNum-1;
				bfp(parse[1]);
				assembly ~= format!"CMP r%d 0"(--pseudoStackSize);
				assembly ~= format!"JNE l%d"(firstLabel);
				assembly ~= format!"LD r%d 0"(pseudoStackSize);
				assembly ~= format!"JMP l%d"(labelNum++);
				assembly ~= format!"l%d:"(firstLabel);
				assembly ~= format!"LD r%d 1"(pseudoStackSize++);
				assembly ~= format!"l%d:"(labelNum-1);
			}
			break;
		case "PEXC.ExprLogAnd":
			bfp(parse[0]);
			if (parse.children.length == 2) {
				assembly ~= format!"CMP r%d 0"(--pseudoStackSize);
				assembly ~= format!"JEQ l%d"(labelNum++);
				int firstLabel = labelNum-1;
				bfp(parse[1]);
				assembly ~= format!"CMP r%d 0"(--pseudoStackSize);
				assembly ~= format!"JEQ l%d"(firstLabel);
				assembly ~= format!"LD r%d 1"(pseudoStackSize);
				assembly ~= format!"JMP l%d"(labelNum++);
				assembly ~= format!"l%d:"(firstLabel);
				assembly ~= format!"LD r%d 0"(pseudoStackSize++);
				assembly ~= format!"l%d:"(labelNum-1);
			}
			break;
		case "PEXC.ExprBitOr":
			bfp(parse[0]);
			if (parse.children.length == 2) {
				bfp(parse[1]);
				assembly ~= format!"OR r%d r%d"(pseudoStackSize-2, --pseudoStackSize);
			}
			break;
		case "PEXC.ExprBitXor":
			bfp(parse[0]);
			if (parse.children.length == 2) {
				bfp(parse[1]);
				assembly ~= format!"XOR r%d r%d"(pseudoStackSize-2, --pseudoStackSize);
			}
			break;
		case "PEXC.ExprBitAnd":
			bfp(parse[0]);
			if (parse.children.length == 2) {
				bfp(parse[1]);
				assembly ~= format!"AND r%d r%d"(pseudoStackSize-2, --pseudoStackSize);
			}
			break;
		case "PEXC.ExprEq":
			bfp(parse[0]);
			if (parse.children.length == 3) {
				string operation;
				switch(parse[1].matches[0]) {
					case "==":
						operation = "JEQ";
						break;
					case "!=":
						operation = "JNE";
						break;
					default:
						throw new CompilationException("This message should never appear. What will I do if it does?");
				}
				bfp(parse[2]);
				assembly ~= format!"CMP r%d r%d"(pseudoStackSize-2, --pseudoStackSize);
				assembly ~= format!"%s l%d"(operation, labelNum++);
				assembly ~= format!"LD r%d 0"(pseudoStackSize-1);
				assembly ~= format!"JMP l%d"(labelNum++);
				assembly ~= format!"l%d:"(labelNum-2);
				assembly ~= format!"LD r%d 1"(pseudoStackSize-1);
				assembly ~= format!"l%d:"(labelNum-1);
			}
			break;
		case "PEXC.ExprCmp":
			bfp(parse[0]);
			if (parse.children.length == 3) {
				string operation;
				bool invert;
				switch(parse[1].matches[0]) {
					case ">":
						operation = "JGT";
						break;
					case "<":
						operation = "JLT";
						break;
					case ">=":
						operation = "JLT";
						invert = true;
						break;
					case "<=":
						operation = "JGT";
						invert = true;
						break;
					default:
						throw new CompilationException("This message should never appear. What will I do if it does?");
				}
				bfp(parse[2]);
				assembly ~= format!"CMP r%d r%d"(pseudoStackSize-2, --pseudoStackSize);
				assembly ~= format!"%s l%d"(operation, labelNum++);
				assembly ~= format!"LD r%d %d"(pseudoStackSize-1, invert ? 1 : 0);
				assembly ~= format!"JMP l%d"(labelNum++);
				assembly ~= format!"l%d:"(labelNum-2);
				assembly ~= format!"LD r%d %d"(pseudoStackSize-1, invert ? 0 : 1);
				assembly ~= format!"l%d:"(labelNum-1);
			}
			break;
		case "PEXC.ExprShift":
			bfp(parse[0]);
			if (parse.children.length == 3) {
				bfp(parse[2]);
				if (parse[1].matches[0] == "<<") {
					assembly ~= format!"LS r%d r%d"(pseudoStackSize-2, --pseudoStackSize);
				} else {
					assembly ~= format!"RS r%d r%d"(pseudoStackSize-2, --pseudoStackSize);
				}
			}
			break;
		case "PEXC.ExprAdd":
			bfp(parse[0]);
			if (parse.children.length == 3) {
				bfp(parse[2]);
				if (parse[1].matches[0] == "+") {
					assembly ~= format!"ADD r%d r%d"(pseudoStackSize-2, --pseudoStackSize);
				} else {
					assembly ~= format!"SUB r%d r%d"(pseudoStackSize-2, --pseudoStackSize);
				}
			}
			break;
		case "PEXC.ExprMul":
			bfp(parse[0]);
			if (parse.children.length == 3) {
				bfp(parse[2]);
				if (parse[1].matches[0] == "*") {
					assembly ~= format!"MUL r%d r%d"(pseudoStackSize-2, --pseudoStackSize);
				} else {
					assembly ~= format!"DIV r%d r%d"(pseudoStackSize-2, --pseudoStackSize);
				}
			}
			break;
		case "PEXC.ExprPrefixes":
			bfp(parse[$-1]);
			foreach(op; parse[0..$-1]) {
				switch(op.matches[0]) {
					case "~":
						assembly ~= format!"NOT r%d"(pseudoStackSize-1);
						break;
					case "!":
						assembly ~= format!"CMP r%d 0"(pseudoStackSize-1);
						assembly ~= format!"JEQ l%d"(labelNum++);
						assembly ~= format!"LD r%d 1"(pseudoStackSize-1);
						assembly ~= format!"JMP l%d"(labelNum++);
						assembly ~= format!"l%d:"(labelNum-2);
						assembly ~= format!"LD r%d 0"(pseudoStackSize-1);
						assembly ~= format!"l%d:"(labelNum-1);
						break;
					case "+":
						break;
					case "-":
						assembly ~= format!"NOT r%d"(pseudoStackSize-1);
						assembly ~= format!"ADD r%d 1"(pseudoStackSize-1);
						break;
					case "*":
						assembly ~= format!"LD IDX r%d"(pseudoStackSize-1);
						assembly ~= format!"LD r%d [IDX]"(pseudoStackSize-1);
						break;
					default:
						throw new CompilationException("This message shouldn't appear, but if it does, it's a bug.");
				}
			}
			break;
		case "PEXC.ExprIndexed":
			bfp(parse[0]);
			foreach(idx; parse[1..$]) {
				bfp(idx);
				assembly ~= format!"ADD r%d r%d"(pseudoStackSize-2, --pseudoStackSize);
				assembly ~= format!"LD IDX r%d"(pseudoStackSize-1);
				assembly ~= format!"LD r%d [IDX]"(pseudoStackSize-1);
			}
			break;
		case "PEXC.ReturnStatement":
			bfp(parse[0]);
			pseudoStackSize--;
			assembly ~= format!"ADD SP %d"(spShift);
			assembly ~= "RET";
			break;
		case "PEXC.Statement":
			bfp(parse[0]);
			assert(pseudoStackSize == 0);
			break;
		case "PEXC.InlineAsmStatement":
			assembly ~= parse[0].matches[0];
			break;
		case "PEXC.IfStatement":
			bfp(parse[0]);
			assembly ~= format!"CMP r%d 0"(--pseudoStackSize);
			assembly ~= format!"JEQ l%d"(labelNum);
			int firstLabel = labelNum++;
			bfp(parse[1]);
			if (parse.children.length == 3) {
				assembly ~= format!"JMP l%d"(labelNum);
				assembly ~= format!"l%d:"(firstLabel);
				int secondLabel = labelNum++;
				bfp(parse[2]);
				assembly ~= format!"l%d:"(secondLabel);
			} else {
				assembly ~= format!"l%d:"(firstLabel);
			}
			break;
		case "PEXC.WhileStatement":
			int firstLabel = labelNum;
			assembly ~= format!"l%d:"(labelNum++);
			bfp(parse[0]);
			assembly ~= format!"CMP r%d 0"(--pseudoStackSize);
			assembly ~= format!"JEQ l%d"(labelNum);
			int secondLabel = labelNum++;
			bfp(parse[1]);
			assembly ~= format!"JMP l%d"(firstLabel);
			assembly ~= format!"l%d:"(secondLabel);
			break;
		case "PEXC.DoStatement":
			assembly ~= format!"l%d:"(labelNum);
			int firstLabel = labelNum++;
			bfp(parse[0]);
			bfp(parse[1]);
			assembly ~= format!"CMP r%d 0"(--pseudoStackSize);
			assembly ~= format!"JNE l%d"(firstLabel);
			break;
		case "PEXC.ForStatement":
			bfp(parse[0]);
			int firstLabel = labelNum++;
			assembly ~= format!"l%d:"(firstLabel);
			bfp(parse[1]);
			assembly ~= format!"CMP r%d 0"(--pseudoStackSize);
			int secondLabel = labelNum++;
			assembly ~= format!"JEQ l%d"(secondLabel);
			bfp(parse[3]);
			bfp(parse[2]);
			assembly ~= format!"JMP l%d"(firstLabel);
			assembly ~= format!"l%d:"(secondLabel);
			break;
		default:
			foreach(child; parse) {
				bfp(child);
			}
	}
}
