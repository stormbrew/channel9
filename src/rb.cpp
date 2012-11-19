#include <stdexcept>

#include "c9/channel9.hpp"
#include "c9/environment.hpp"
#include "c9/context.hpp"
#include "c9/string.hpp"
#include "c9/value.hpp"
#include "c9/message.hpp"
#include "c9/loader.hpp"
#include "c9/ffi.hpp"

#include "ruby.h"

using namespace Channel9;

static VALUE rb_ruby_mod;
static ID rb_compile;
static ID rb_compile_string;
static ID rb_to_json;

#ifndef STR2CSTR
const char *STR2CSTR(VALUE val)
{
    return StringValuePtr(val);
}
#endif

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

class SprintfChannel : public CallableContext
{
	FFIDefinition *m_string_holder;

public:
	SprintfChannel(FFIDefinition *string_holder) : m_string_holder(string_holder) {}
	~SprintfChannel(){}

	void send(Environment *env, const Value &val, const Value &ret)
	{
		if (is(val, MESSAGE))
		{
			Message *msg = ptr<Message>(val);
			if (msg->arg_count() >= 2)
			{
				FFIDefinition *types = new FFIDefinition("sprintf args");
				types->add_pointer(m_string_holder); // output pointer
				types->add_pointer(); // format string

				Message::const_iterator it;
				for (it = msg->args()+2; it != msg->args_end(); it++)
				{
					switch (type(*it))
					{
					case POSITIVE_NUMBER:
					case NEGATIVE_NUMBER:
						types->add(&ffi_type_sint);
						break;
					case FLOAT_NUM:
						types->add(&ffi_type_double);
						break;
					case NIL:
					case UNDEF:
					case STRING:
						types->add_pointer();
						break;
					case BFALSE:
					case BTRUE:
						types->add(&ffi_type_uint);
						break;
					default:
						throw std::runtime_error("Invalid input given to sprintf");
					}
				}

				FFICall *sprintf_call = new FFICall(ffi_fn(asprintf), &ffi_type_sint, types);
				sprintf_call->send(env, value(msg), ret);
				return;
			}
		}
		channel_send(env, ret, val, value(no_return_channel));
	}
	std::string inspect() const
	{
		return "Sprintf primitive channel";
	}
};

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
				std::vector<Value> bt;
				RunningContext *ctx;
				if (is(ret, RUNNING_CONTEXT))
					ctx = ptr<RunningContext>(ret);
				else
					ctx = env->context();
				while (ctx)
				{
					SourcePos pos = ctx->m_instructions->source_pos(ctx->m_pos);
					std::stringstream posinfo;
					posinfo << pos.file << ":" << pos.line_num << ":" << pos.column;
					if (!pos.annotation.empty())
						posinfo << " (" << pos.annotation << ")";
					bt.push_back(value(new_string(posinfo.str())));
					ctx = ctx->m_caller;
				}
				channel_send(env, ret, value(new_tuple(bt.begin(), bt.end())), Channel9::value(no_return_channel));
				return;
			} else if (msg->m_message_id == mid_load && msg->arg_count() == 1 && is(msg->args()[0], STRING)) {
				// try to load an alongside bytecode object directly first.
				std::string path = ptr<String>(*msg->args())->str();
				try {
					GCRef<RunnableContext*> ctx = load_bytecode(env, path + ".c9b");
					channel_send(env, value(*ctx), Nil, ret);
					return;
				} catch (loader_error &err) {
					// now try to compile it.
					VALUE res = rb_funcall(rb_ruby_mod, rb_compile, 1, rb_str_new2(path.c_str()));
					if (!NIL_P(res)) {
						// make it a json string.
						res = rb_funcall(res, rb_to_json, 0);
						GCRef<RunnableContext*> ctx = load_bytecode(env, path, STR2CSTR(res));
						channel_send(env, value(*ctx), Nil, ret);
						return;
					} else {
						channel_send(env, ret, False, Channel9::value(no_return_channel));
						return;
					}
				}
			} else if (msg->m_message_id == mid_compile && msg->arg_count() == 4 &&
				is(msg->args()[0], STRING) && is(msg->args()[1], STRING) &&
				is(msg->args()[2], STRING) && is_number(msg->args()[3])) {
				std::string type = ptr<String>(msg->args()[0])->str();
				std::string path = ptr<String>(msg->args()[1])->str();
				std::string code = ptr<String>(msg->args()[2])->str();
				int64_t line_num = msg->args()[3].machine_num;

				VALUE res = rb_funcall(rb_ruby_mod, rb_compile_string, 4,
					ID2SYM(rb_intern(type.c_str())), rb_str_new2(path.c_str()),
					rb_str_new2(code.c_str()), INT2NUM(line_num));
				if (!NIL_P(res)) {
					// make it a json string.
					res = rb_funcall(res, rb_to_json, 0);
					channel_send(env, ret, value(*load_bytecode(env, path, STR2CSTR(res))), value(no_return_channel));
					return;
				} else {
					channel_send(env, ret, False, value(no_return_channel));
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

void setup_basic_ffi_functions(Environment *env)
{
	FFIDefinition *write_args = new FFIDefinition("write args");
	write_args->add(ffi_type_map<int>());
	write_args->add_pointer();
	write_args->add(ffi_type_map<size_t>());
	FFICall *write_call = new FFICall(ffi_fn(write), ffi_type_map<ssize_t>(), write_args);

	env->set_special_channel("ffi_write", Channel9::value(write_call));

	FFIDefinition *time_t_holder = new FFIDefinition("time_t holder");
	time_t_holder->add("time", ffi_type_map<time_t>());
	FFIDefinition *time_now_args = new FFIDefinition("time_now_args");
	time_now_args->add_pointer(time_t_holder);
	FFICall *time_now_call = new FFICall(ffi_fn(time), ffi_type_map<time_t>(), time_now_args);

	env->set_special_channel("ffi_time_now", Channel9::value(time_now_call));

	FFIDefinition *struct_timespec = new FFIDefinition("struct timespec");
	struct_timespec->add("tv_sec", ffi_type_map<time_t>());
	struct_timespec->add("tv_nsec", ffi_type_map<long>());
	FFIDefinition *struct_stat = new FFIDefinition("struct stat");
	struct_stat->add("st_dev", ffi_type_map<dev_t>());
	struct_stat->add("st_ino", ffi_type_map<ino_t>());
	struct_stat->add("st_nlink", ffi_type_map<nlink_t>());
	struct_stat->add("st_mode", ffi_type_map<mode_t>());
	struct_stat->add("st_uid", ffi_type_map<uid_t>());
	struct_stat->add("st_gid", ffi_type_map<gid_t>());
	struct_stat->add(ffi_type_map<int>());
	struct_stat->add("st_rdev", ffi_type_map<dev_t>());
	struct_stat->add("st_size", ffi_type_map<off_t>());
	struct_stat->add("st_blksize", ffi_type_map<blksize_t>());
	struct_stat->add("st_blocks", ffi_type_map<blkcnt_t>());
	struct_stat->add("st_atim", struct_timespec);
	struct_stat->add("st_mtim", struct_timespec);
	struct_stat->add("st_ctim", struct_timespec);
	struct_stat->add(ffi_type_map<int>(), 3);
	assert(struct_stat->size() == sizeof(struct stat));

	FFIDefinition *stat_args = new FFIDefinition("stat args");
	stat_args->add_pointer(); // string path
	stat_args->add_pointer(struct_stat);
	FFICall *stat_call = new FFICall(ffi_fn(stat), &ffi_type_sint, stat_args);

	env->set_special_channel("ffi_stat", Channel9::value(stat_call));
	env->set_special_channel("ffi_struct_timespec", Channel9::value(struct_timespec));
	env->set_special_channel("ffi_struct_stat", Channel9::value(struct_stat));

	FFIDefinition *string_holder = new FFIDefinition("string holder");
	string_holder->add_pointer();
	env->set_special_channel("ffi_sprintf_buf", Channel9::value(string_holder));
	env->set_special_channel("ffi_sprintf", Channel9::value(new SprintfChannel(string_holder)));
}

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

	env->set_special_channel("set_special_channel", Channel9::value(new SetSpecialChannel));
	env->set_special_channel("initial_load_path", Channel9::value(new_tuple(initial_paths, initial_paths + 3)));
	env->set_special_channel("loader", Channel9::value(new LoaderChannel(path)));

	setup_basic_ffi_functions(env);

	ruby_init();
	ruby_init_loadpath();
	VALUE rb_load_path = rb_gv_get("LOAD_PATH");
	rb_ary_push(rb_load_path, rb_str_new2(C9_RUBY_LIB));
	rb_ary_push(rb_load_path, rb_str_new2(C9RB_RUBY_LIB));
	ruby_script("channel9.rb");

	rb_ruby_mod = rb_define_module_under(rb_define_module("Channel9"), "Ruby");
	rb_compile_string = rb_intern("compile_string");
	rb_compile = rb_intern("compile");
	rb_to_json = rb_intern("to_json");

	rb_require("rubygems");
	rb_require("channel9/ruby");

	return 0;
}
