#include <fstream>
#include <algorithm>

#include <stdlib.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "json/json.h"
#include "json/reader.h"
#include "c9/loader.hpp"
#include "c9/istream.hpp"
#include "c9/variable_frame.hpp"
#include "c9/context.hpp"

namespace Channel9
{
	class NothingChannel : public CallableContext
	{
	public:
		NothingChannel(){}
		~NothingChannel(){}

		void send(Environment *env, const Value &val, const Value &ret)
		{
		}
		std::string inspect() const
		{
			return "Nothing Channel";
		}
	};
	NothingChannel *nothing_channel = new NothingChannel;

	template <typename tRight>
	loader_error operator<<(const loader_error &left, const tRight &right)
	{
		std::stringstream stream;
		stream << left.reason;
		stream << right;
		return loader_error(stream.str());
	}

	Value convert_json(const Json::Value &val);

	Value convert_json_array(const Json::Value &arr)
	{
		std::vector<Value> vals(arr.size());
		std::vector<Value>::iterator oiter = vals.begin();
		Json::Value::const_iterator iiter = arr.begin();
		while (iiter != arr.end())
			*oiter++ = convert_json(*iiter++);
		return value(new_tuple(vals.begin(), vals.end()));
	}

	Value convert_json_complex(const Json::Value &obj)
	{
		if (!obj["undef"].isNull())
		{
			return Undef;
		} else if (obj["message_id"].isString()) {
			return value((int64_t)make_message_id(obj["message_id"].asString()));
		} else if (obj["protocol_id"].isString()) {
			return value((int64_t)make_protocol_id(obj["protocol_id"].asString()));
		}
		throw loader_error("Unknown complex object ") << obj.toStyledString();
	}

	Value convert_json(const Json::Value &val)
	{
		using namespace Json;
		switch (val.type())
		{
		case nullValue:
			return Nil;
		case intValue:
			return value(val.asInt64());
		case uintValue:
			return value((int64_t)val.asUInt64());
		case realValue:
			return value(val.asDouble());
		case stringValue:
			return value(val.asString());
		case booleanValue:
			return bvalue(val.asBool());
		case arrayValue:
			return convert_json_array(val);
		case objectValue:
			return convert_json_complex(val);
		default:
			throw loader_error("Invalid value encountered while parsing json");
		}
	}

	GCRef<RunnableContext*> load_program(Environment *env, const Json::Value &code)
	{
		using namespace Channel9;
		GCRef<IStream*> istream = new IStream;

		int num = 0;
		for (Json::Value::const_iterator it = code.begin(); it != code.end(); it++, num++)
		{
			const Json::Value &line = *it;
			if (!line.isArray() || line.size() < 1)
			{
				throw loader_error("Malformed line ") << num << ": not an array or not enough elements";
			}

			const Json::Value &ival = line[0];
			if (!ival.isString())
			{
				throw loader_error("Instruction on line ") << num << " was not a string (" << ival.toStyledString() << ")";
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
					throw loader_error("Invalid set_label line at ") << num;
				}
				(*istream)->set_label(line[1].asString());
			} else {
				INUM insnum = inum(instruction);
				if (insnum == INUM_INVALID)
				{
					throw loader_error("Invalid instruction ") << instruction << " at " << num;
				}
				Instruction ins = {insnum, {0}, {0}, {0}};
				InstructionInfo info = iinfo(ins);
				if (line.size() - 1 < info.argc)
				{
					throw loader_error("Instruction ") << instruction << " takes "
						<< info.argc << "arguments, but was given " << line.size() - 1
						<< " at line " << num;
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

		(*istream)->normalize();
		GCRef<VariableFrame*> frame = new_variable_frame(*istream);
		GCRef<RunnableContext*> ctx = new_context(*istream, *frame);

		return ctx;
	}

	GCRef<RunnableContext*> load_bytecode(Environment *env, const std::string &filename)
	{
		std::ifstream file(filename.c_str());
		if (file.is_open())
		{
			Json::Reader reader;
			Json::Value body;
			if (reader.parse(file, body, false))
			{
				Json::Value code = body["code"];
				if (!code.isArray())
				{
					throw loader_error("No code block in ") << filename;
				}

				return load_program(env, code);
			} else {
				throw loader_error("Failed to parse json in ") << filename << ":\n"
					<< reader.getFormattedErrorMessages();
			}
		} else {
			throw loader_error("Could not open file ") << filename;
		}
	}

	GCRef<RunnableContext*> load_bytecode(Environment *env, const std::string &filename, const std::string &str)
	{
		Json::Reader reader;
		Json::Value body;
		if (reader.parse(str, body, false))
		{
			Json::Value code = body["code"];
			if (!code.isArray())
			{
				throw loader_error("No code block in ") << filename;
			}

			return load_program(env, code);
		} else {
			throw loader_error("Failed to parse json in ") << filename << ":\n"
				<< reader.getFormattedErrorMessages();
		}
	}

	int run_bytecode(Environment *env, const std::string &filename)
	{
		GCRef<RunnableContext*> ctx = load_bytecode(env, filename);
		channel_send(env, value(*ctx), Nil, value(nothing_channel));
		return 0;
	}

	int run_list(Environment *env, const std::string &filename)
	{
		std::ifstream file(filename.c_str());
		if (file.is_open())
		{
			while (!file.eof())
			{
				std::string line;
				std::getline(file, line);
				if (line.size() > 0)
				{
					if (line[0] != '/')
					{
						// if the path is not absolute it's relative
						// to the c9l file.
						size_t last_slash_pos = filename.rfind('/');

						if (last_slash_pos == std::string::npos)
							run_file(env, line);
						else
							run_file(env, filename.substr(0, last_slash_pos+1) + line);
					}
				} else {
					break;
				}
			}
			return 0;
		} else {
			throw loader_error("Could not open file ") << filename << ".";
		}
	}

	typedef int (*entry_point)(Environment*, const std::string&);
	int run_shared_object(Environment *env, const std::string &filename)
	{
		void *shobj = dlopen(filename.c_str(), RTLD_LAZY | RTLD_LOCAL);
		if (!shobj)
		{
			throw loader_error("Could not load shared object ") << filename;
		}
		entry_point shobj_entry = (entry_point)dlsym(shobj, "Channel9_environment_initialize");
		if (!shobj_entry)
		{
			throw loader_error("Could not load entry point to shared object ") << filename;
		}
		return shobj_entry(env, filename);
	}

	void set_argv(Environment *env, int argc, const char **argv)
	{
		std::vector<Channel9::Value> args;
		for (int i = 0; i < argc; i++)
		{
			args.push_back(Channel9::value(Channel9::new_string(argv[i])));
		}
		env->set_special_channel("argv", Channel9::value(Channel9::new_tuple(args.begin(), args.end())));
	}

	int load_environment_and_run(Environment *env, std::string program, int argc, const char **argv, bool trace_loaded, bool compile)
	{
		// let the program invocation override the filename (ie. c9.rb always runs ruby)
		// but if the exe doesn't match the exact expectation, use the first argument's
		// extension to decide what to do.
		size_t slash_pos = program.rfind('/');
		if (slash_pos != std::string::npos)
			program = program.substr(slash_pos + 1, std::string::npos);
		if (program.length() < 3 || !std::equal(program.begin(), program.begin()+3, "c9."))
		{
			if (argc < 1)
				throw loader_error("No program file specified.");

			program = argv[0];
		}

		std::string ext = "";
		size_t ext_pos = program.rfind('.');
		if (ext_pos != std::string::npos)
			ext = program.substr(ext_pos);

		if (ext == "")
			throw loader_error("Can't discover environment for file with no extension.");

		if (ext == ".c9b" || ext == ".c9l" || ext == ".so")
		{
			// chop off the c9x file so it doesn't try to load itself.
			set_argv(env, argc-1, argv+1);
			return run_file(env, program);
		} else {
			// get the path of libc9.so. We expect c9 environments to be near it.
			// They'll be at dirname(libc9.so)/c9-env/ext/ext.c9l.
			Dl_info fninfo;
			dladdr((void*)load_environment_and_run, &fninfo);
			std::string search_path = std::string(fninfo.dli_fname);
			search_path = search_path.substr(0, search_path.find_last_of('/') + 1);
			search_path += "c9-env/";
			std::string extname = ext.substr(1, std::string::npos);
			search_path += extname + "/" + extname + ".c9l";

			// find a matching module
			struct stat match = {0};
			if (stat(search_path.c_str(), &match) != 0)
				throw loader_error("Could not find c9-env loader at ") << search_path;

			// include the program name argument so it knows what to load.
			set_argv(env, argc, argv);
			env->set_special_channel("trace_loaded", bvalue(trace_loaded));
			return run_file(env, search_path);
		}
		return 1;
	}

	int run_file(Environment *env, const std::string &filename)
	{
		// find the extension
		std::string ext = "";
		size_t ext_pos = filename.rfind('.');
		if (ext_pos != std::string::npos)
			ext = filename.substr(ext_pos);

		if (ext == ".c9b")
		{
			return run_bytecode(env, filename);
		} else if (ext == ".c9l") {
			return run_list(env, filename);
		} else if (ext == ".so") {
			return run_shared_object(env, filename);
		} else if (ext == "") {
			throw loader_error("Don't know what to do with no extension.");
		} else {
			throw loader_error("Don't know what to do with extension `") << ext << "`";
		}
	}
}
