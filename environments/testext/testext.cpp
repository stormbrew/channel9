#include "c9/channel9.hpp"
#include "c9/environment.hpp"
#include "c9/context.hpp"
#include "c9/string.hpp"
#include "c9/value.hpp"

class TestChannel : public Channel9::CallableContext
{
public:
	TestChannel(){}
	~TestChannel(){}

	void send(Channel9::Environment *env, const Channel9::Value &val, const Channel9::Value &ret)
	{
		if (Channel9::is(val, Channel9::STRING))
			std::cout << "Hello, " << Channel9::ptr<Channel9::String>(val)->c_str() << ", and welcome to the test module.\n";
		else
			std::cout << "Hello from the test module! I don't understand your name.\n";
		channel_send(env, ret, val, Channel9::Nil);
	}
	std::string inspect() const
	{
		return "Test Channel";
	}
};
TestChannel *test_channel = new TestChannel;

extern "C" int Channel9_environment_initialize(Channel9::Environment *env)
{
	env->set_special_channel("hello_test_channel", Channel9::value(test_channel));
	return 0;
}
