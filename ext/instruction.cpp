#include "instruction.hpp"
#include <map>
#include <algorithm>

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
			INUM_DEF(DEBUG);
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
			INAME_DEF(DEBUG);
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
			INAME_DEF(SWAP);
			INAME_DEF(TUPLE_NEW);
			INAME_DEF(TUPLE_SPLAT);
			INAME_DEF(TUPLE_UNPACK);
		}
		return "<unknown>";
	}

	Instruction instruction(INUM ins, const Value &arg1, const Value &arg2, const Value &arg3)
	{
		Instruction instruction = {ins, arg1, arg2, arg3};
		return instruction;
	}
}