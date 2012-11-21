#include "c9/channel9.hpp"
#include "c9/message.hpp"
#include "c9/string.hpp"
#include "c9/value.hpp"
#include "c9/context.hpp"
#include "rb_glob.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fnmatch.h>
#include <unistd.h>

#include <list>
#include <string>

using namespace Channel9;

typedef std::vector<Value> result_set;

void match(result_set &res, const std::string &up, const std::string &down);
void match_alldir(result_set &res, const std::string &up, const std::string &down)
{
	size_t slash_pos = down.find('/');

	std::string matcher = down.substr(0, slash_pos);
	std::string rest = "";
	if (slash_pos != std::string::npos)
		rest = down.substr(slash_pos + 1);

	DIR *in;
	if (!up.size())
		in = opendir(".");
	else
		in = opendir(up.c_str());
	struct dirent entry = {0};
	struct dirent *entry_p = NULL;

	// search this directory first.
	match(res, up, down);

	// now recurse through all subdirectories and resume matches there.
	rewinddir(in);

	while (readdir_r(in, &entry, &entry_p) == 0 && entry_p != NULL)
	{
		if (entry.d_type == DT_DIR && strcmp(".", entry.d_name) != 0 &&
			strcmp("..", entry.d_name) != 0)
			match_alldir(res, up + entry.d_name + "/", rest);
	}
}
void match(result_set &res, const std::string &up, const std::string &down)
{
	size_t slash_pos = down.find('/');

	std::string matcher = down.substr(0, slash_pos);
	std::string rest = "";
	if (slash_pos != std::string::npos)
		rest = down.substr(slash_pos + 1);

	DIR *in;
	if (!up.size())
		in = opendir(".");
	else
		in = opendir(up.c_str());
	struct dirent entry = {0};
	struct dirent *entry_p = NULL;

	if (matcher == "**")
	{
		// search this directory first.
		match(res, up, rest);

		// now recurse through all subdirectories and resume matches there.
		rewinddir(in);

		while (readdir_r(in, &entry, &entry_p) == 0 && entry_p != NULL)
		{
			if (entry.d_type == DT_DIR && strcmp(".", entry.d_name) != 0 &&
				strcmp("..", entry.d_name) != 0)
				match_alldir(res, up + entry.d_name + "/", rest);
		}
	} else {
		// look for anything that's a fnmatch. If it's a directory,
		// recurse. If rest is empty we're at the leaf of the match
		// pattern and we should add it to the matches.
		while (readdir_r(in, &entry, &entry_p) == 0 && entry_p != NULL)
		{
			if (fnmatch(matcher.c_str(), entry.d_name, 0) == 0 &&
				strcmp(".", entry.d_name) != 0 && strcmp("..", entry.d_name) != 0)
			{
				if (!rest.size()) // empty, done
					res.push_back(value(new_string(up + entry.d_name)));
				else if (entry.d_type == DT_DIR)
					match(res, up + entry.d_name + "/", rest);
			}
		}
	}
	closedir(in);
}

Tuple *match(const std::string &pattern)
{
	result_set res;
	std::string up = "";
	std::string down = pattern;

	if (pattern[0] == '/')
	{
		up = "/";
		down = pattern.substr(1);
	}
	match(res, up, down);

	return new_tuple(res.begin(), res.end());
}

void GlobChannel::send(Environment *env, const Value &val, const Value &ret)
{
	if (is(val, MESSAGE))
	{
		Message *msg = ptr<Message>(val);
		if (msg->arg_count() == 1)
		{
			if (is(msg->args()[0], STRING))
			{
				String *pattern = ptr<String>(msg->args()[0]);
				Tuple *matches = match(pattern->str());
				channel_send(env, ret, value(matches), value(&no_return_ctx));
				return;
			}
		}
	}
	channel_send(env, ret, Nil, value(&no_return_ctx));
}
