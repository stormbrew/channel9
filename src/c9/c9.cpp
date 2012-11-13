#include <iostream>

#include "c9/channel9.hpp"
#include "c9/istream.hpp"
#include "c9/environment.hpp"
#include "c9/context.hpp"
#include "c9/instruction.hpp"
#include "c9/string.hpp"
#include "c9/tuple.hpp"
#include "c9/loader.hpp"

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
			std::cout << Channel9::ptr<Channel9::String>(val)->c_str();
			break;
		default:
			break;
		}
		std::cout << "\n";
		channel_send(env, ret, val, Channel9::value(no_return_channel));
	}
	std::string inspect() const
	{
		return "Stdout Channel";
	}
};
StdoutChannel *stdout_channel = new StdoutChannel;

int main(int argc, const char **argv)
{
	bool debug = false;
	bool detail = false;
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
	if (i == argc)
	{
		std::cout << program << ": No file specified to run.\n";
		exit(1);
	}

	const char *filename = argv[i++];

	std::vector<Channel9::Value> args;
	for (; i < argc; i++)
	{
		args.push_back(Channel9::value(Channel9::new_string(argv[i])));
	}

	Channel9::Environment *env = new Channel9::Environment();
	env->set_special_channel("exit", Channel9::value(exit_channel));
	env->set_special_channel("stdout", Channel9::value(stdout_channel));
	env->set_special_channel("argv", Channel9::value(Channel9::new_tuple(args.begin(), args.end())));

	try {
		return Channel9::run_file(env, filename, false);
	} catch (const Channel9::loader_error &err) {
		std::cout << program << ": " << err.reason << "\n";
		return 1;
	}
}
