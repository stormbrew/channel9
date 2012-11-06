#include <iostream>
#include <fstream>
#include <stdlib.h>

#include "json/reader.h"
#include "channel9.hpp"
#include "istream.hpp"
#include "environment.hpp"
#include "context.hpp"
#include "instruction.hpp"

class NoReturnChannel : public Channel9::CallableContext
{
public:
	NoReturnChannel(){}
	~NoReturnChannel(){}

	void send(Channel9::Environment *env, const Channel9::Value &val, const Channel9::Value &ret)
	{
		std::cout << "Unexpected return to no return channel.\n";
		exit(1);
	}
	std::string inspect() const
	{
		return "No Return Channel";
	}
};
NoReturnChannel *no_return_channel = new NoReturnChannel;

class ExitChannel : public Channel9::CallableContext
{
public:
	ExitChannel()
	{}
	~ExitChannel()
	{}

	void send(Channel9::Environment *env, const Channel9::Value &val, const Channel9::Value &ret)
	{
		if (is_number(val))
			exit(val.machine_num);
		else
			exit(0);
	}
	std::string inspect() const
	{
		return "Clean Exit Channel";
	}
};
ExitChannel *exit_channel = new ExitChannel;

class StdoutChannel : public Channel9::CallableContext
{
public:
	void send(Channel9::Environment *env, const Channel9::Value &val, const Channel9::Value &ret)
	{
		switch (type(val))
		{
		case Channel9::BFALSE:
			std::cout << "false";
			break;
		case Channel9::BTRUE:
			std::cout << "true";
			break;
		case Channel9::UNDEF:
			std::cout << "undef";
			break;
		case Channel9::NIL:
			std::cout << "nil";
			break;
		case Channel9::FLOAT_NUM:
			std::cout << float_num(val);
			break;
		case Channel9::POSITIVE_NUMBER:
		case Channel9::NEGATIVE_NUMBER:
			std::cout << val.machine_num;
			break;
		case Channel9::STRING:
			std::cout << val.str_p;
			break;
		default:
			break;
		}
		channel_send(env, ret, val, Channel9::value(no_return_channel));
	}
	std::string inspect() const
	{
		return "Stdout Channel";
	}
};
StdoutChannel *stdout_channel = new StdoutChannel;

Channel9::Value convert_json(const Json::Value &val);

Channel9::Value convert_json_array(const Json::Value &arr)
{
	std::vector<Channel9::Value> vals(arr.size());
	std::vector<Channel9::Value>::iterator oiter = vals.begin();
	Json::Value::const_iterator iiter = arr.begin();
	while (iiter != arr.end())
		*oiter++ = convert_json(*iiter++);
	return Channel9::value(Channel9::new_tuple(vals.begin(), vals.end()));
}

Channel9::Value convert_json_complex(const Json::Value &obj)
{
	if (!obj["undef"].isNull())
	{
		return Channel9::Undef;
	} else if (obj["message_id"].isString()) {
		return Channel9::value((int64_t)Channel9::make_message_id(obj["message_id"].asString()));
	} else if (obj["protocol_id"].isString()) {
		return Channel9::value((int64_t)Channel9::make_protocol_id(obj["protocol_id"].asString()));
	}
	std::cout << "Unknown complex object " << obj.toStyledString() << "\n";
	exit(1);
}

Channel9::Value convert_json(const Json::Value &val)
{
	using namespace Json;
	switch (val.type())
	{
	case nullValue:
		return Channel9::Nil;
	case intValue:
	case uintValue:
		return Channel9::value(val.asInt64());
	case realValue:
		return Channel9::value(val.asDouble());
	case stringValue:
		return Channel9::value(val.asString());
	case booleanValue:
		return Channel9::bvalue(val.asBool());
	case arrayValue:
		return convert_json_array(val);
	case objectValue:
		return convert_json_complex(val);
	default:
		std::cout << "Invalid value encountered while parsing json\n";
		exit(1);
	}
}

int run_program(const Json::Value &code, bool debug, bool detail)
{
	using namespace Channel9;
	GCRef<IStream*> istream = new IStream;

	int num = 0;
	for (Json::Value::const_iterator it = code.begin(); it != code.end(); it++, num++)
	{
		const Json::Value &line = *it;
		if (!line.isArray() || line.size() < 1)
		{
			std::cout << "Malformed line " << num << ": not an array or not enough elements\n";
			exit(1);
		}

		const Json::Value &ival = line[0];
		if (!ival.isString())
		{
			std::cout << "Instruction on line " << num << " was not a string (" << ival.toStyledString() << ")\n";
			exit(1);
		}

		const std::string &instruction = ival.asString();
		if (instruction == "line")
		{
			if (line.size() > 1)
			{
				std::string file = "(unknown)", extra;
				uint64_t lineno = 0, linepos = 0;
				file = line[1].asString();
				if (line.size() > 2)
					lineno = line[2].asUInt64();
				if (line.size() > 3)
					linepos = line[3].asUInt64();
				if (line.size() > 4)
					extra = line[4].asString();
				(*istream)->set_source_pos(SourcePos(file, lineno, linepos, extra));
			}
		} else if (instruction == "set_label") {
			if (line.size() != 2 || !line[1].isString())
			{
				std::cout << "Invalid set_label line at " << num << "\n";
				exit(1);
			}
			(*istream)->set_label(line[1].asString());
		} else {
			INUM insnum = inum(instruction);
			if (insnum == INUM_INVALID)
			{
				std::cout << "Invalid instruction " << instruction << " at " << num << "\n";
				exit(1);
			}
			Instruction ins = {insnum, {0}, {0}, {0}};
			InstructionInfo info = iinfo(ins);
			if (line.size() - 1 < info.argc)
			{
				std::cout << "Instruction " << instruction << " takes "
					<< info.argc << "arguments, but was given " << line.size() - 1
					<< " at line " << num << "\n";
				exit(1);
			}

			if (info.argc > 0)
				ins.arg1 = convert_json(line[1]);
			if (info.argc > 1)
				ins.arg2 = convert_json(line[2]);
			if (info.argc > 2)
				ins.arg3 = convert_json(line[3]);

			(*istream)->add(ins);
		}
	}

	Channel9::Environment *env = new Channel9::Environment();
	env->set_special_channel("exit", Channel9::value(exit_channel));
	env->set_special_channel("stdout", Channel9::value(stdout_channel));

	(*istream)->normalize();
	GCRef<Channel9::VariableFrame*> frame = Channel9::new_variable_frame(*istream);
	GCRef<Channel9::RunnableContext*> ctx = Channel9::new_context(*istream, *frame);
	Channel9::channel_send(env, value(*ctx), Channel9::Nil, Channel9::value(exit_channel));

	return 0;
/*

  environment = Channel9::Environment.new(debug)
  context = Channel9::Context.new(environment, stream)
  stime = Time.now
  begin
    context.channel_send(environment, nil, Channel9::CleanExitChannel)
  ensure
    etime = Time.now
    puts("Ran in #{etime - stime}s")
  end
end
*/
}

int main(int argc, const char **argv)
{
	bool debug = false;
	bool detail = false;
	const char *filename = NULL;
	const char *program = *argv++; argc--; // chop off program name.
	int i = 0;

	for (i = 0; i < argc && argv[i][0] == '-'; i++)
	{
		if (strcmp("-d", argv[i]) == 0)
			debug = true;
		else if (strcmp("-dd", argv[i]) == 0)
			detail = true;
	}
	// first non-flag argument is the file to parse
	if (i < argc) {
		filename = *argv;
	} else {
		std::cout << program << ": No file specified to run.\n";
		exit(1);
	}

	std::ifstream file(filename);
	if (file.is_open())
	{
		Json::Reader reader;
		Json::Value body;
		if (reader.parse(file, body, false))
		{
			Json::Value code = body["code"];
			if (!code.isArray())
			{
				std::cout << program << ": No code block in " << filename << "\n";
				exit(1);
			}

			return run_program(code, debug, detail);
		} else {
			std::cout << program << ": Failed to parse json in " << filename << ":\n"
				<< reader.getFormattedErrorMessages();
			exit(1);
		}
	} else {
		std::cout << program << ": Could not open file " << filename << ".\n";
		exit(1);
	}
}
