#include "rb_loader.hpp"

#include "c9/message.hpp"
#include "c9/context.hpp"
#include "c9/loader.hpp"

#include <ruby.h>

#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

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

LoaderChannel::LoaderChannel(const std::string &environment_path)
 : environment_path(environment_path),
   mid_load_c9(make_message_id("load_c9")),
   mid_load(make_message_id("load")),
   mid_compile(make_message_id("compile")),
   mid_backtrace(make_message_id("backtrace"))
{
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
}

void LoaderChannel::store_bytecode(const std::string &path, const char *bytecode)
{
	FILE *bytecode_file = fopen((path + ".c9b").c_str(), "w");
	if (bytecode_file)
	{
		fwrite(bytecode, strlen(bytecode), 1, bytecode_file);
		fclose(bytecode_file);
	}
}

void LoaderChannel::compile_and_run_ruby(Environment *env, const Value &ret, const std::string &path, bool compile_only)
{
	struct stat rb_file;
	struct stat c9_file;

	bool rb_file_found = stat(path.c_str(), &rb_file) == 0;
	bool c9_file_found = stat((path + ".c9b").c_str(), &c9_file) == 0;

	if (c9_file_found && S_ISREG(c9_file.st_mode))
	{
		if (!rb_file_found || c9_file.st_ctim.tv_sec >= rb_file.st_ctim.tv_sec)
		{
			try {
				GCRef<RunnableContext*> ctx = load_bytecode(env, path + ".c9b");
				if (!compile_only) {
					channel_send(env, value(*ctx), Nil, ret);
				} else {
					channel_send(env, ret, True, Channel9::value(&no_return_ctx));
				}
				return;

			} catch (loader_error &err) {
				// ignore.
			}
		}
	}

	if (rb_file_found && S_ISREG(rb_file.st_mode))
	{
		// now try to compile it.
		VALUE res = rb_funcall(rb_ruby_mod, rb_compile, 1, rb_str_new2(path.c_str()));
		if (!NIL_P(res)) {
			// make it a json string.
			res = rb_funcall(res, rb_to_json, 0);
			GCRef<RunnableContext*> ctx = load_bytecode(env, path, STR2CSTR(res));
			store_bytecode(path, STR2CSTR(res));
			if (!compile_only) {
				channel_send(env, value(*ctx), Nil, ret);
			} else {
				channel_send(env, ret, True, Channel9::value(&no_return_ctx));
			}
			return;
		}
	}

	channel_send(env, ret, False, Channel9::value(&no_return_ctx));
	return;
}

void LoaderChannel::send(Channel9::Environment *env, const Channel9::Value &val, const Channel9::Value &ret)
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
				channel_send(env, ret, False, Channel9::value(&no_return_ctx));
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
			channel_send(env, ret, value(new_tuple(bt.begin(), bt.end())), Channel9::value(&no_return_ctx));
			return;
		} else if (msg->m_message_id == mid_load && msg->arg_count() >= 1 && is(msg->args()[0], STRING)) {
			// try to load an alongside bytecode object directly first.
			std::string path = ptr<String>(*msg->args())->str();
			compile_and_run_ruby(env, ret, path, msg->arg_count() > 1 && is_truthy(msg->args()[1]));
			return;
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
				channel_send(env, ret, value(*load_bytecode(env, path, STR2CSTR(res))), value(&no_return_ctx));
				return;
			} else {
				channel_send(env, ret, False, value(&no_return_ctx));
				return;
			}
		} else {
			std::cout << "Trap: Unknown message to loader: " << message_names[msg->m_message_id] << "\n";
			exit(1);
		}
	}

	channel_send(env, ret, val, Channel9::value(&no_return_ctx));
}
