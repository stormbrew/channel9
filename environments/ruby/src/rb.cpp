#include <stdexcept>

#include "c9/channel9.hpp"
#include "c9/environment.hpp"
#include "c9/context.hpp"
#include "c9/string.hpp"
#include "c9/value.hpp"
#include "c9/message.hpp"
#include "c9/loader.hpp"
#include "c9/ffi.hpp"

#include "rb_glob.hpp"
#include "rb_loader.hpp"
#include "rb_regexp.hpp"

using namespace Channel9;

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
		channel_send(env, ret, val, value(&no_return_ctx));
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

				FFICall *sprintf_call = new FFICall("asprintf", ffi_fn(asprintf), &ffi_type_sint, types);
				sprintf_call->send(env, value(msg), ret);
				return;
			}
		}
		channel_send(env, ret, val, value(&no_return_ctx));
	}
	std::string inspect() const
	{
		return "Sprintf primitive channel";
	}
};

void setup_basic_ffi_functions(Environment *env)
{
	FFIDefinition *write_args = new FFIDefinition("write args");
	write_args->add(ffi_type_map<int>());
	write_args->add_pointer();
	write_args->add(ffi_type_map<size_t>());
	FFICall *write_call = new FFICall("write", ffi_fn(write), ffi_type_map<ssize_t>(), write_args);

	env->set_special_channel("ffi_write", Channel9::value(write_call));

	FFIDefinition *fopen_args = new FFIDefinition("fopen args");
	fopen_args->add_pointer(); // filename
	fopen_args->add_pointer(); // mode string
	FFICall *fopen_call = new FFICall("fopen", ffi_fn(fopen), ffi_type_map<intptr_t>(), fopen_args);

	FFIDefinition *fwrite_args = new FFIDefinition("fwrite args");
	fwrite_args->add_pointer(); // string
	fwrite_args->add(ffi_type_map<size_t>()); // size
	fwrite_args->add(ffi_type_map<size_t>()); // members
	fwrite_args->add(ffi_type_map<intptr_t>()); // FILE*
	FFICall *fwrite_call = new FFICall("fwrite", ffi_fn(fwrite), ffi_type_map<size_t>(), fwrite_args);

	// TODO: This needs to be made to pass an actual buffer object of some sort rather than a String
	// object that it then writes to. This is going to cause all sorts of problems down the road, but
	// isn't relevant right now. Sorry future me, remember that this project is mostly for fun.
	FFIDefinition *fgets_args = new FFIDefinition("fgets args");
	fgets_args->add_pointer(); // string buffer
	fgets_args->add(ffi_type_map<int>());
	fgets_args->add(ffi_type_map<intptr_t>()); // FILE*
	FFICall *fgets_call = new FFICall("fgets", ffi_fn(fgets), ffi_type_map<intptr_t>(), fgets_args);

	FFIDefinition *fclose_args = new FFIDefinition("fclose args");
	fclose_args->add(ffi_type_map<intptr_t>()); // FILE*
	FFICall *fclose_call = new FFICall("fclose", ffi_fn(fclose), ffi_type_map<int>(), fclose_args);

	env->set_special_channel("ffi_fopen", Channel9::value(fopen_call));
	env->set_special_channel("ffi_fgets", Channel9::value(fgets_call));
	env->set_special_channel("ffi_fwrite", Channel9::value(fwrite_call));
	env->set_special_channel("ffi_fclose", Channel9::value(fclose_call));

	FFIDefinition *time_t_holder = new FFIDefinition("time_t holder");
	time_t_holder->add("time", ffi_type_map<time_t>());
	FFIDefinition *time_now_args = new FFIDefinition("time_now_args");
	time_now_args->add_pointer(time_t_holder);
	FFICall *time_now_call = new FFICall("time", ffi_fn(time), ffi_type_map<time_t>(), time_now_args);

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
	struct_stat->add(ffi_type_map<long>(), 3);

	FFIDefinition *stat_args = new FFIDefinition("stat args");
	stat_args->add_pointer(); // string path
	stat_args->add_pointer(struct_stat);
	FFICall *stat_call = new FFICall("stat", ffi_fn(stat), &ffi_type_sint, stat_args);

	env->set_special_channel("ffi_stat", Channel9::value(stat_call));
	env->set_special_channel("ffi_struct_timespec", Channel9::value(struct_timespec));
	env->set_special_channel("ffi_struct_stat", Channel9::value(struct_stat));

	FFIDefinition *string_holder = new FFIDefinition("string holder");
	string_holder->add_pointer();
	env->set_special_channel("ffi_sprintf_buf", Channel9::value(string_holder));
	env->set_special_channel("ffi_sprintf", Channel9::value(new SprintfChannel(string_holder)));

	FFIDefinition *chdir_args = new FFIDefinition("chdir args");
	chdir_args->add_pointer();
	FFICall *chdir_call = new FFICall("chdir", ffi_fn(chdir), &ffi_type_sint, chdir_args);

	FFIDefinition *mkdir_args = new FFIDefinition("mkdir args");
	mkdir_args->add_pointer();
	mkdir_args->add(ffi_type_map<mode_t>());
	FFICall *mkdir_call = new FFICall("mkdir", ffi_fn(mkdir), &ffi_type_sint, mkdir_args);

	env->set_special_channel("ffi_chdir", Channel9::value(chdir_call));
	env->set_special_channel("ffi_mkdir", Channel9::value(mkdir_call));

	env->set_special_channel("glob", Channel9::value(new GlobChannel));
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
	env->set_special_channel("regexp", Channel9::value(new RegexpChannel()));

	setup_basic_ffi_functions(env);

	return 0;
}
