#include "instruction.hpp"
#include <map>
#include <algorithm>
#include <sstream>

namespace Channel9
{
	namespace
	{
		typedef std::map<std::string, INUM> inum_table_t;
		inum_table_t make_inum_table()
		{
			inum_table_t table;
#			define INUM_DEF(name) table[#name] = name

			INUM_DEF(NOP);
			INUM_DEF(CHANNEL_NEW);
			INUM_DEF(CHANNEL_SPECIAL);
			INUM_DEF(CHANNEL_SEND);
			INUM_DEF(CHANNEL_CALL);
			INUM_DEF(CHANNEL_RET);
			INUM_DEF(DEBUGGER);
			INUM_DEF(DUP_TOP);
			INUM_DEF(FRAME_GET);
			INUM_DEF(FRAME_SET);
			INUM_DEF(IS);
			INUM_DEF(IS_EQ);
			INUM_DEF(IS_NOT);
			INUM_DEF(IS_NOT_EQ);
			INUM_DEF(JMP);
			INUM_DEF(JMP_IF);
			INUM_DEF(JMP_IF_NOT);
			INUM_DEF(LOCAL_CLEAN_SCOPE);
			INUM_DEF(LOCAL_LINKED_SCOPE);
			INUM_DEF(LOCAL_GET);
			INUM_DEF(LOCAL_SET);
			INUM_DEF(MESSAGE_NEW);
			INUM_DEF(MESSAGE_SPLAT);
			INUM_DEF(MESSAGE_ADD);
			INUM_DEF(MESSAGE_COUNT);
			INUM_DEF(MESSAGE_NAME);
			INUM_DEF(MESSAGE_SYS_UNPACK);
			INUM_DEF(MESSAGE_UNPACK);
			INUM_DEF(POP);
			INUM_DEF(PUSH);
			INUM_DEF(STRING_NEW);
			INUM_DEF(STRING_COERCE);
			INUM_DEF(SWAP);
			INUM_DEF(TUPLE_NEW);
			INUM_DEF(TUPLE_SPLAT);
			INUM_DEF(TUPLE_UNPACK);
			return table;
		}
		inum_table_t inum_table = make_inum_table();
	}

	INUM inum(std::string iname)
	{
		std::transform(iname.begin(), iname.end(), iname.begin(), toupper);
		return inum_table[iname];
	}
	std::string iname(INUM inum)
	{
#		define INAME_DEF(op) case op: return #op
		switch (inum)
		{
			INAME_DEF(NOP);
			INAME_DEF(CHANNEL_NEW);
			INAME_DEF(CHANNEL_SPECIAL);
			INAME_DEF(CHANNEL_SEND);
			INAME_DEF(CHANNEL_CALL);
			INAME_DEF(CHANNEL_RET);
			INAME_DEF(DEBUGGER);
			INAME_DEF(DUP_TOP);
			INAME_DEF(FRAME_GET);
			INAME_DEF(FRAME_SET);
			INAME_DEF(IS);
			INAME_DEF(IS_EQ);
			INAME_DEF(IS_NOT);
			INAME_DEF(IS_NOT_EQ);
			INAME_DEF(JMP);
			INAME_DEF(JMP_IF);
			INAME_DEF(JMP_IF_NOT);
			INAME_DEF(LOCAL_CLEAN_SCOPE);
			INAME_DEF(LOCAL_LINKED_SCOPE);
			INAME_DEF(LOCAL_GET);
			INAME_DEF(LOCAL_SET);
			INAME_DEF(MESSAGE_NEW);
			INAME_DEF(MESSAGE_SPLAT);
			INAME_DEF(MESSAGE_ADD);
			INAME_DEF(MESSAGE_COUNT);
			INAME_DEF(MESSAGE_NAME);
			INAME_DEF(MESSAGE_SYS_UNPACK);
			INAME_DEF(MESSAGE_UNPACK);
			INAME_DEF(POP);
			INAME_DEF(PUSH);
			INAME_DEF(STRING_NEW);
			INAME_DEF(STRING_COERCE);
			INAME_DEF(SWAP);
			INAME_DEF(TUPLE_NEW);
			INAME_DEF(TUPLE_SPLAT);
			INAME_DEF(TUPLE_UNPACK);
			default: break;
		}
		return "<unknown>";
	}

	InstructionInfo iinfo(const Instruction &ins)
	{
#		define IINFO(name, in, out, args, terminal) \
			case name: { InstructionInfo ret = { name, (in), (out), (args), (terminal) }; return ret; }

		switch (ins.instruction)
		{
		IINFO(NOP, 0, 0, 0, false);
		IINFO(DEBUGGER, 0, 0, 1, false);

		IINFO(POP, 1, 0, 0, false);
		IINFO(PUSH, 0, 1, 1, false);
		IINFO(SWAP, 2, 2, 0, false);
		IINFO(DUP_TOP, 1, 2, 0, false);

		IINFO(IS, 1, 1, 1, false);
		IINFO(IS_EQ, 2, 1, 0, false);
		IINFO(IS_NOT, 1, 1, 1, false);
		IINFO(IS_NOT_EQ, 2, 1, 0, false);
		IINFO(JMP, 0, 0, 1, false);
		IINFO(JMP_IF, 1, 0, 1, false);
		IINFO(JMP_IF_NOT, 1, 0, 1, false);

		IINFO(LOCAL_CLEAN_SCOPE, 0, 0, 0, false);
		IINFO(LOCAL_LINKED_SCOPE, 0, 0, 0, false);
		IINFO(FRAME_GET, 0, 1, 1, false);
		IINFO(FRAME_SET, 1, 0, 1, false);
		IINFO(LOCAL_GET, 0, 1, 2, false);
		IINFO(LOCAL_SET, 1, 0, 2, false);

		IINFO(CHANNEL_NEW, 0, 1, 1, false);
		IINFO(CHANNEL_SPECIAL, 0, 1, 1, false);
		IINFO(CHANNEL_SEND, 3, 2, 0, true);
		IINFO(CHANNEL_CALL, 2, 2, 0, true);
		IINFO(CHANNEL_RET, 2, 2, 0, true);

		IINFO(MESSAGE_NEW,
			(size_t)(ins.arg2.machine_num + ins.arg3.machine_num), 1,
			3, false
			);
		IINFO(MESSAGE_SPLAT, 2, 1, 0, false);
		IINFO(MESSAGE_ADD,
			(size_t)(1 + ins.arg1.machine_num), 1,
			1, false
			);
		IINFO(MESSAGE_COUNT, 1, 2, 0, false);
		IINFO(MESSAGE_NAME, 1, 2, 0, false);
		IINFO(MESSAGE_SYS_UNPACK,
			1, (size_t)(1 + ins.arg1.machine_num),
			1, false
			);
		IINFO(MESSAGE_UNPACK,
			1, (size_t)(1 + ins.arg1.machine_num + (ins.arg2.machine_num != 0?1:0) + ins.arg3.machine_num),
			3, false
			);

		IINFO(STRING_NEW, (size_t)ins.arg1.machine_num, 1, 1, false);
		IINFO(STRING_COERCE, 1, 2, 1, false);

		IINFO(TUPLE_NEW,
			(size_t)ins.arg1.machine_num, 1,
			1, false
			);
		IINFO(TUPLE_SPLAT, 2, 1, 0, false);
		IINFO(TUPLE_UNPACK,
			1, 1 + (size_t)(ins.arg1.machine_num + (ins.arg2.machine_num != 0?1:0) + ins.arg3.machine_num),
			3, false
			);
		default: break;
		}
		InstructionInfo ret = {NOP};
		return ret;
	}

	Instruction instruction(INUM ins, const Value &arg1, const Value &arg2, const Value &arg3)
	{
		Instruction instruction = {ins, arg1, arg2, arg3};
		return instruction;
	}

	std::string inspect(const Instruction &ins)
	{
		std::stringstream str;
		InstructionInfo info = iinfo(ins);
		str << iname(ins.instruction) << "(";
		if (info.argc > 0)
			str << inspect(ins.arg1);
		if (info.argc > 1)
			str << ", " << inspect(ins.arg2);
		if (info.argc > 2)
			str << ", " << inspect(ins.arg3);
		switch (ins.instruction)
		{
		case CHANNEL_NEW:
		case JMP:
		case JMP_IF:
		case JMP_IF_NOT:
		case LOCAL_GET:
		case LOCAL_SET:
		case FRAME_GET:
		case FRAME_SET:
			str << ", c:" << inspect(ins.arg3);
		default: break;
		}
		str << ")";
		return str.str();
	}
}
