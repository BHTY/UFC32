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
mixin(grammar(`PEXC:
	Program < (GlobalDecl / GlobalArrayDecl / FunctionDecl)+
	GlobalDecl < "u32" identifier ("=" IntLiteral)? ";"
	IntLiteral < ~("-"?[0-9]+)
	GlobalArrayDecl < "u32[" IntLiteral ? "]" ^identifier ("=" (ArrayLiteral / StringLiteral))? ";"
	ArrayLiteral < "{" IntLiteral ("," IntLiteral)* "}"
	StringLiteral < '\"' ~(((!'\"' .) | :'\\' '\"')*) !'\\' '\"'
	FunctionDecl < ("u32" / "void") ^identifier ArgumentList Statement
	ArgumentList < "(" (Argument ("," Argument)+)? ")"
	Argument < "u32" ^identifier
	CallArgumentList < "(" (RValue ("," RValue)+)? ")"
	Statement < BlockStatement / LocalDecl / ExpressionStatement / ReturnStatement
	LocalDecl < "u32" identifier ("=" IntLiteral)? ";"
	BlockStatement < "{" Statement* "}"
	ExpressionStatement < (LValue ^("=" / "+=" / "-=" / "*=" / "/=" / "&=" / "|=" / "<<=" / ">>="))? RValue ";"
	LValue < ^identifier / "*" ExprAtom
	RValue < ExprLogOr ("?" RValue ":" RValue)?
	ExprLogOr < ExprLogAnd ("||" ExprLogOr)?
	ExprLogAnd < ExprBitOr ("&&" ExprLogAnd)?
	ExprBitOr < ExprBitXor ("|" ExprBitOr)?
	ExprBitXor < ExprBitAnd ("^" ExprBitXor)?
	ExprBitAnd < ExprEq ("&" ExprBitAnd)?
	ExprEq < ExprCmp (("==" / "!=") ExprEq)?
	ExprCmp < ExprShift (("<" / ">" / "<=" / ">=") ExprCmp)?
	ExprShift < ExprAdd (("<<" / ">>") ExprShift)?
	ExprAdd < ExprMul (("+" / "-") ExprAdd)?
	ExprMul < ExprPrefixes (("*" / "/") ExprMul)?
	ExprPrefixes < ("~" / "!" / "+" / "-"/ "*")* ExprAtom
	ExprAtom < IntLiteral / "(" RValue ")" / ^identifier CallArgumentList / ("++" / "--" / "&")? ^identifier / ^identifier ("++" / "--")?
	ReturnStatement < "return" RValue ";"`));

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
	foreach(line; assembly) {
		outfile.writeln(line);
	}
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
			} else if (parse[0].name == "PEXC.IntLiteral") {
				arrayLength = parse[0].matches[0].to!int;
			} else if (parse[$-1].name == "PEXC.ArrayLiteral") {
				arrayLength = parse[0].children.length;
			} else {
				arrayLength = parse[0].matches[1].length;
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
			assembly ~= parse[1].matches[0] ~ ":";
			foreach (i; 0..arrayLength) {
				assembly ~= format!"DB %d"(i < arrayPrefix.length ? arrayPrefix[i] : 0);
			}
			globalSymbols ~= parse[1].matches[0];
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
					assembly ~= format!"LD [IDX] r%d"(whichReg++);
					assembly ~= format!"SUB IDX 1";
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
				assembly ~= format!"PUSH %s"(parse[0].matches[0]);
			} else {
				assembly ~= "PUSH 0";
			}
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
							assembly ~= format!"LD r%d SP"(pseudoStackSize);
							if (spShift - localVariable.spOffset != 1) {
								assembly ~= format!"ADD r%d %d"(pseudoStackSize++, spShift - localVariable.spOffset -1);
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
					default:
						throw new CompilationException("sorry that compound assignment operator's not supported yet!");
				}
			} else {
				pseudoStackSize--;
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
		default:
			foreach(child; parse) {
				bfp(child);
			}
	}
}
