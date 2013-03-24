#include "c9/callable_context.hpp"
#include "c9/message.hpp"
#include "c9/context.hpp"
#include "c9/string.hpp"

#include "rb_regexp.hpp"

#include <oniguruma.h>

namespace Channel9
{
	class RegexpMatcherChannel : public CallableContext
	{
	private:
		std::string pattern;
		regex_t *regex;

	public:
		RegexpMatcherChannel(const std::string &pattern, regex_t *regex)
		 : pattern(pattern), regex(regex)
		{}
		~RegexpMatcherChannel(){
			onig_free(regex);
		}

		void send(Environment *env, const Value &val, const Value &ret)
		{
			if (is(val, MESSAGE))
			{
				Message *msg = ptr<Message>(val);
				if (msg->arg_count() >= 1)
				{
					if (is(msg->args()[0], STRING))
					{
						// returns a tuple of (errcode, errstring, matchlist)
						// where errstring is nil on success and matchlist is nil on failure.
						// errstring and matchlist will both be nil if there were no matches.
						// matchlist is a tuple of tuples, each item containing
						// a pair of indexes to the string for the match. If the subgroup
						// did not match, it will be the pair -1,-1
						String *str = ptr<String>(msg->args()[0]);
						size_t off = 0;

						if (msg->arg_count() >= 2 && is_number(msg->args()[1]))
							off = (size_t)msg->args()[1].machine_num;

						if (off >= str->length())
						{
							Value rval[] = { value(int64_t(0)), Nil, Nil };
							channel_send(env, ret, value(new_tuple(3, rval)), value(&no_return_ctx));
							return;
						}

						OnigRegion *region = onig_region_new();
						int err = onig_search(regex, (OnigUChar*)str->begin(), (OnigUChar*)str->end(),
							(OnigUChar*)str->begin()+off, (OnigUChar*)str->end(), region, ONIG_OPTION_NONE);
						Value errval = value((int64_t)err);

						if (err >= 0) // why is this not ONIG_NORMAL like onig_new? No idea.
						{
							Tuple *matches = new_tuple(region->num_regs);
							Tuple::iterator it = matches->begin();
							for (ssize_t i = 0; i < region->num_regs; i++)
							{
								Value pos[] = { value((int64_t)region->beg[i]), value((int64_t)region->end[i]) };
								*it++ = value(new_tuple(2, pos));
							}

							Value rval[] = { errval, Nil, value(matches) };
							channel_send(env, ret, value(new_tuple(3, rval)), value(&no_return_ctx));
						} else if (err == ONIG_MISMATCH) {
							Value rval[] = { errval, Nil, Nil };
							channel_send(env, ret, value(new_tuple(3, rval)), value(&no_return_ctx));
						} else {
						    char s[ONIG_MAX_ERROR_MESSAGE_LEN];
						    onig_error_code_to_str((OnigUChar*)s, err);
						    Value rval[] = { errval, value(new_string(s)), Nil };
						    channel_send(env, ret, value(new_tuple(3, rval)), value(&no_return_ctx));
						}
						onig_region_free(region, 1);
						return;
					}
				}
			}
			channel_send(env, ret, Nil, value(&no_return_ctx));
		}
		std::string inspect() const
		{
			return "Regexp Matcher Channel";
		}
	};

	void RegexpChannel::send(Environment *env, const Value &val, const Value &ret)
	{
		if (is(val, MESSAGE))
		{
			Message *msg = ptr<Message>(val);
			if (msg->arg_count() == 1)
			{
				if (is(msg->args()[0], STRING))
				{
					// returns a tuple of (errcode, errstring, matcher)
					// where errstring is nil on success and matcher is nil on failure.
					String *pattern = ptr<String>(msg->args()[0]);

					regex_t *regex;
					OnigErrorInfo einfo;
					int err = onig_new(&regex, (OnigUChar*)pattern->begin(), (OnigUChar*)pattern->end(), ONIG_OPTION_DEFAULT, ONIG_ENCODING_ASCII, ONIG_SYNTAX_RUBY, &einfo);
					Value errval = value((int64_t)err);

					if (err == ONIG_NORMAL)
					{
						RegexpMatcherChannel *matcher = new RegexpMatcherChannel(pattern->str(), regex);
						Value rval[] = { errval, Nil, value(matcher) };
						channel_send(env, ret, value(new_tuple(3, rval)), value(&no_return_ctx));
					} else {
					    char s[ONIG_MAX_ERROR_MESSAGE_LEN];
					    onig_error_code_to_str((OnigUChar*)s, err, &einfo);
					    Value rval[] = { errval, value(new_string(s)), Nil };
					    channel_send(env, ret, value(new_tuple(3, rval)), value(&no_return_ctx));
					}
					return;
				}
			}
		}
		channel_send(env, ret, Nil, value(&no_return_ctx));
	}
}
