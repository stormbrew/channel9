#pragma once

#include <string>

#include "channel9.hpp"
#include "value.hpp"

namespace Channel9
{
	enum INUM
	{
		NOP = 0,
		DEBUGGER,

		POP,
		PUSH,
		SWAP,
		DUP_TOP,

		IS,
		IS_EQ,
		IS_NOT,
		IS_NOT_EQ,
		JMP,
		JMP_IF,
		JMP_IF_NOT,

		LEXICAL_CLEAN_SCOPE,
		LEXICAL_LINKED_SCOPE,
		FRAME_GET,
		FRAME_SET,
		LOCAL_GET,
		LOCAL_SET,
		LEXICAL_GET,
		LEXICAL_SET,

		CHANNEL_NEW,
		CHANNEL_SPECIAL,
		CHANNEL_SEND,
		CHANNEL_CALL,
		CHANNEL_RET,

		MESSAGE_NEW,
		MESSAGE_SPLAT,
		MESSAGE_ADD,
		MESSAGE_COUNT,
		MESSAGE_IS,
		MESSAGE_IS_PROTO,
		MESSAGE_ID,
		MESSAGE_SPLIT_ID,
		MESSAGE_CHECK,
		MESSAGE_FORWARD,
		MESSAGE_SYS_PREFIX,
		MESSAGE_SYS_UNPACK,
		MESSAGE_UNPACK,

		STRING_NEW,
		STRING_COERCE,

		TUPLE_NEW,
		TUPLE_SPLAT,
		TUPLE_UNPACK,

		INUM_COUNT,
		INUM_INVALID = -1,
	};

	struct InstructionInfo
	{
		INUM inum;

		// number of stack args popped and pushed
		size_t in, out;
		// number of arguments the instruction takes
		size_t argc;
		// whether the instruction is terminal.
		bool terminal;
	};

	INUM inum(std::string iname);
	std::string iname(INUM);

	struct Instruction
	{
		INUM instruction;
		Value arg1;
		Value arg2;
		Value arg3;
		Value cache;
	};
	Instruction instruction(INUM instruction, const Value &arg1 = Nil, const Value &arg2 = Nil, const Value &arg3 = Nil);
	inline Instruction instruction(const std::string &ins, const Value &arg1 = Nil, const Value &arg2 = Nil, const Value &arg3 = Nil)
	{
		return instruction(inum(ins), arg1, arg2);
	}

	InstructionInfo iinfo(const Instruction &ins);

	std::string inspect(const Instruction &ins);

	inline void gc_scan(Instruction *ins)
	{
		gc_scan(ins->arg1);
		gc_scan(ins->arg2);
		gc_scan(ins->arg3);
	}
}

