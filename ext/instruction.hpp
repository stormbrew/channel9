#pragma once

#include <string>

#include "channel9.hpp"
#include "value.hpp"

namespace Channel9
{
	enum INUM
	{
		NOP = 0,
		DEBUG,

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

		LOCAL_CLEAN_SCOPE,
		LOCAL_LINKED_SCOPE,
		FRAME_GET,
		FRAME_SET,
		LOCAL_GET,
		LOCAL_SET,

		CHANNEL_NEW,
		CHANNEL_SPECIAL,
		CHANNEL_SEND,
		CHANNEL_CALL,
		CHANNEL_RET,

		MESSAGE_NEW,
		MESSAGE_SPLAT,
		MESSAGE_ADD,
		MESSAGE_COUNT,
		MESSAGE_NAME,
		MESSAGE_SYS_UNPACK,
		MESSAGE_UNPACK,

		STRING_NEW,

		TUPLE_NEW,
		TUPLE_SPLAT,
		TUPLE_UNPACK,
	};

	INUM inum(std::string iname);
	std::string iname(INUM);

	struct Instruction
	{
		INUM instruction;
		Value arg1;
		Value arg2;
		Value arg3;
	};
	Instruction instruction(INUM instruction, const Value &arg1 = Value::Nil, const Value &arg2 = Value::Nil, const Value &arg3 = Value::Nil);
	inline Instruction instruction(const std::string &ins, const Value &arg1 = Value::Nil, const Value &arg2 = Value::Nil, const Value &arg3 = Value::Nil)
	{ 
		return instruction(inum(ins), arg1, arg2); 
	}
}