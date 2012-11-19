#include "c9/channel9.hpp"
#include "c9/environment.hpp"
#include "c9/context.hpp"
#include "c9/string.hpp"
#include "c9/value.hpp"
#include "c9/ffi.hpp"

using namespace Channel9;

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

extern "C" int Channel9_environment_initialize(Channel9::Environment *env, const std::string &boot_path)
{
	FFIDefinition *args = new FFIDefinition("Puts Arguments");
	args->add_pointer();
	FFICall *puts_call = new FFICall(ffi_fn(puts), &ffi_type_sint, args);

	FFIDefinition *struct_timespec = new FFIDefinition("struct timespec");
	struct_timespec->add(ffi_type_map<time_t>()); // 0 time_t tv_sec
	struct_timespec->add(ffi_type_map<long>()); // 1 time_t tv_nsec
	FFIDefinition *struct_stat = new FFIDefinition("struct stat");
	struct_stat->add(ffi_type_map<dev_t>()); // 0 dev_t st_dev
	struct_stat->add(ffi_type_map<ino_t>()); // 1 ino_t st_ino
	struct_stat->add(ffi_type_map<nlink_t>()); // 2 nlink_t st_nlink
	struct_stat->add(ffi_type_map<mode_t>()); // 3 mode_t st_mode
	struct_stat->add(ffi_type_map<uid_t>()); // 4 uid_t st_uid
	struct_stat->add(ffi_type_map<gid_t>()); // 5 gid_t st_gid
	struct_stat->add(ffi_type_map<int>());
	struct_stat->add(ffi_type_map<dev_t>()); // 6 dev_t st_rdev
	struct_stat->add(ffi_type_map<off_t>()); // 8 off_t st_size
	struct_stat->add(ffi_type_map<blksize_t>()); // 9 blksize_t st_blksize
	struct_stat->add(ffi_type_map<blkcnt_t>()); // 10 blkcnt_t st_blocks
	struct_stat->add(struct_timespec, 3); // 11-13 time_t st_atim, st_mtime, st_ctim
/*	struct_stat->add(ffi_type_map<time_t>()); // 11 time_t st_atime
	struct_stat->add(ffi_type_map<unsigned long>()); // 12 unsigned long st_atime_msec
	struct_stat->add(ffi_type_map<time_t>()); // 13 time_t st_mtime
	struct_stat->add(ffi_type_map<unsigned long>()); // 14 unsigned long st_mtime_msec
	struct_stat->add(ffi_type_map<time_t>()); // 15 time_t st_ctime
	struct_stat->add(ffi_type_map<unsigned long>()); // 16 unsigned long st_ctime_msec */
	struct_stat->add(ffi_type_map<int>(), 3);
	assert(struct_stat->size() == sizeof(struct stat));

	FFIDefinition *stat_args = new FFIDefinition("stat args");
	stat_args->add_pointer(); // string path
	stat_args->add_pointer(struct_stat);
	FFICall *stat_call = new FFICall(ffi_fn(stat), &ffi_type_sint, stat_args);

	env->set_special_channel("ffi_puts", Channel9::value(puts_call));
	env->set_special_channel("ffi_stat", Channel9::value(stat_call));
	env->set_special_channel("ffi_struct_stat", Channel9::value(struct_stat));
	env->set_special_channel("hello_test_channel", Channel9::value(test_channel));
	return 0;
}
