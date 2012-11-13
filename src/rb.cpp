#include "c9/channel9.hpp"
#include "c9/environment.hpp"
#include "c9/context.hpp"
#include "c9/string.hpp"
#include "c9/value.hpp"
#include "c9/loader.hpp"

using namespace Channel9;

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

class SetSpecialChannel : public CallableContext
{
public:
	SetSpecialChannel(){}
	~SetSpecialChannel(){}

	void send(Environment *env, const Value &val, const Value &ret)
	{
		if (is(val, MESSAGE))
		{
			Message *msg = ptr<Message>(val);
			if (msg->arg_count() == 2)
			{
				if (is(msg->args()[0], STRING))
				{
					env->set_special_channel(ptr<String>(msg->args()[0])->str(), msg->args()[1]);
				}
			}
		}
		channel_send(env, ret, val, value(no_return_channel));
	}
	std::string inspect() const
	{
		return "Set Special Channel";
	}
};
SetSpecialChannel *set_special_channel = new SetSpecialChannel;

class PrintChannel : public Channel9::CallableContext
{
public:
	void send(Channel9::Environment *env, const Channel9::Value &val, const Channel9::Value &ret)
	{
		if (is(val, MESSAGE))
		{
			Message *msg = ptr<Message>(val);
			for (Message::iterator it = msg->args(); it != msg->args_end(); it++)
			{
				if (is(*it, STRING))
					std::cout << *ptr<String>(*it);
			}
		}

		channel_send(env, ret, val, Channel9::value(no_return_channel));
	}
	std::string inspect() const
	{
		return "Raw Print Channel";
	}
};
PrintChannel *print_channel = new PrintChannel;

class LoaderChannel : public CallableContext
{
private:
	std::string environment_path;
	uint64_t mid_load_c9;
	uint64_t mid_load;
	uint64_t mid_compile;
	uint64_t mid_backtrace;

public:
	LoaderChannel(const std::string &environment_path)
	 : environment_path(environment_path),
	   mid_load_c9(make_message_id("load_c9")),
	   mid_load(make_message_id("load")),
	   mid_compile(make_message_id("compile")),
	   mid_backtrace(make_message_id("backtrace"))
	{}

	void send(Channel9::Environment *env, const Channel9::Value &val, const Channel9::Value &ret)
	{
		if (is(val, MESSAGE))
		{
			Message *msg = ptr<Message>(val);
			if (msg->m_message_id == mid_load_c9 && msg->arg_count() == 1 && is(msg->args()[0], STRING))
			{
				try {
					std::string path = environment_path + "/";
					path += ptr<String>(*msg->args())->str();
					GCRef<RunnableContext*> ctx = load_bytecode(env, path);
					channel_send(env, value(*ctx), Nil, ret);
					return;
				} catch (loader_error &err) {
					channel_send(env, ret, False, Channel9::value(no_return_channel));
					return;
				}
			} else if (msg->m_message_id == mid_backtrace) {
				Value empty[1];
				channel_send(env, ret, value(new_tuple(empty,empty)), Channel9::value(no_return_channel));
				return;
			} else if (msg->m_message_id == mid_load && msg->arg_count() == 1 && is(msg->args()[0], STRING)) {
				// try to load an alongside bytecode object directly first.
				try {
					std::string path = ptr<String>(*msg->args())->str() + ".c9b";
					GCRef<RunnableContext*> ctx = load_bytecode(env, path);
					channel_send(env, value(*ctx), Nil, ret);
					return;
				} catch (loader_error &err) {
					// for now just fail outright if there isn't one.
					channel_send(env, ret, False, Channel9::value(no_return_channel));
					return;
				}
			} else {
				std::cout << "Trap: Unknown message to loader: " << message_names[msg->m_message_id] << "\n";
				exit(1);
			}
		}

		channel_send(env, ret, val, Channel9::value(no_return_channel));
	}
	std::string inspect() const
	{
		return "Ruby Loader Channel";
	}
};

extern "C" int Channel9_environment_initialize(Channel9::Environment *env, const std::string &filename)
{
	std::string path = "";
	size_t last_slash = filename.rfind('/');
	if (last_slash != std::string::npos)
		path = filename.substr(0, last_slash+1);
	Value initial_paths[3] = {
		value(new_string(path + "lib")),
		value(new_string(path + "site-lib")),
		value(new_string(".")),
	};

	env->set_special_channel("set_special_channel", Channel9::value(set_special_channel));
	env->set_special_channel("initial_load_path", Channel9::value(new_tuple(initial_paths, initial_paths + 3)));
	env->set_special_channel("print", Channel9::value(print_channel));
	env->set_special_channel("loader", Channel9::value(new LoaderChannel(path)));
	return 0;
}
