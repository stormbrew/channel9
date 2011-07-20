extern "C" {
#	include "ruby.h"
#	include "intern.h"
}
#include "environment.hpp"
#include "context.hpp"
#include "value.hpp"
#include "istream.hpp"
#include "message.hpp"

#include <map>

using namespace Channel9;

typedef VALUE (*ruby_method)(ANYARGS);
typedef void (*mark_method)();

VALUE rb_mChannel9;
VALUE rb_mPrimitive;

VALUE rb_cEnvironment;
VALUE rb_cStream;
VALUE rb_cContext;
VALUE rb_cCallableContext;
VALUE rb_cRunnableContext;
VALUE rb_cMessage;
VALUE rb_cUndef;
VALUE rb_Undef;

static VALUE c9_to_rb(const Value &val);
static Value rb_to_c9(VALUE val);
static VALUE rb_Environment_new(Environment *env);
static VALUE rb_Message_new(const Message *msg);
static VALUE rb_CallableContext_new(CallableContext *ctx);
static VALUE rb_Context_new(RunnableContext *ctx);

class RubyChannel : public CallableContext
{
private:
	VALUE m_val;

public:
	RubyChannel(VALUE val) : m_val(val) 
	{
		rb_gc_register_address(&m_val);
	}
	~RubyChannel()
	{
		rb_gc_unregister_address(&m_val);
	}

	VALUE data() { return m_val; }

	void send(Environment *env, const Value &val, const Value &ret)
	{
		DO_TRACE printf("Oh hi %s\n", STR2CSTR(rb_funcall(rb_class_of(m_val), rb_intern("to_s"), 0)));
		rb_funcall(m_val, rb_intern("channel_send"), 3,
			rb_Environment_new(env), c9_to_rb(val), c9_to_rb(ret));
	}

	void scan()
	{}
};

static Value rb_to_c9(VALUE val)
{
	int type = TYPE(val);
	switch (type)
	{
	case T_NIL:
		return Nil;
	case T_FALSE:
		return False;
	case T_TRUE:
		return True;
	case T_SYMBOL:
		return value(rb_id2name(SYM2ID(val)));
	case T_STRING:
		return value(STR2CSTR(val));
	case T_BIGNUM:
	case T_FIXNUM:
		return value(NUM2LL(val));
	case T_ARRAY: {
		size_t len = RARRAY_LEN(val);
		Tuple *tuple = new_tuple(len);
		Tuple::iterator out = tuple->begin();
		for (size_t i = 0; i < len; ++i)
		{
			*out++ = rb_to_c9(rb_ary_entry(val, i));
		}
		return value(tuple);
		}
	case T_MODULE:
	case T_CLASS:
	case T_OBJECT:
		if (rb_respond_to(val, rb_intern("channel_send")))
		{
			Value c9val = value(new RubyChannel(val));
			DO_TRACE printf("Converting object %s to RubyChannel: %s\n", 
				STR2CSTR(rb_any_to_s(val)), inspect(c9val).c_str());
			return c9val;
		} else if (val == rb_Undef) {
			return Undef;
		}
		break;
	case T_DATA: {
		VALUE klass = rb_class_of(val);
		if (klass == rb_cCallableContext) {
			CallableContext *ctx;
			Data_Get_Struct(val, CallableContext, ctx);
			return value(ctx);
		} else if (klass == rb_cContext) {
			RunnableContext *ctx;
			Data_Get_Struct(val, RunnableContext, ctx);
			return value(ctx);
		} else if (klass == rb_cMessage) {
			Message *msg;
			Data_Get_Struct(val, Message, msg);
			return value(*msg);
		}
		}
		break;
	}
	rb_raise(rb_eRuntimeError, "Could not convert object %s (%d) to c9 object.", 
		STR2CSTR(rb_any_to_s(val)), type);
	return Nil;
}

static VALUE c9_to_rb(const Value &val)
{
	switch (type(val))
	{
	case NIL:
		return Qnil;
	case UNDEF:
		return rb_Undef;
	case BFALSE:
		return Qfalse;
	case BTRUE:
		return Qtrue;
	case POSITIVE_NUMBER:
	case NEGATIVE_NUMBER:
		return LL2NUM(val.machine_num);
//	case FLOAT_NUM:
//		return rb_float_new(val.float_num);
	case STRING:
		return rb_str_new2(ptr<String>(val)->c_str());
	case TUPLE: {
		VALUE ary = rb_ary_new();
		Tuple *tuple = ptr<Tuple>(val);
		for (Tuple::const_iterator it = tuple->begin(); it != tuple->end(); ++it)
		{
			rb_ary_push(ary, c9_to_rb(*it));
		}
		return ary;
		}
	case MESSAGE:
		return rb_Message_new(ptr<Message>(val));
	case CALLABLE_CONTEXT:
		return rb_CallableContext_new(ptr<CallableContext>(val));
	case RUNNABLE_CONTEXT:
		return rb_Context_new(ptr<RunnableContext>(val));
	default:
		printf("Unknown value type %llu\n", type(val));
		exit(1);
	}
	return Qnil;
}

std::map<Environment*, VALUE> c9_env_map;

static VALUE rb_Environment_new(Environment *env)
{
	if (c9_env_map.find(env) != c9_env_map.end())
		return c9_env_map[env];
	
	VALUE obj = Data_Wrap_Struct(rb_cEnvironment, 0, 0, env);
	VALUE debug = Qfalse;
	rb_obj_call_init(obj, 1, &debug);

	c9_env_map[env] = obj;

	return obj;
}

static VALUE Environment_new(VALUE self, VALUE debug)
{
	Environment *env = new Environment();
	VALUE obj = Data_Wrap_Struct(rb_cEnvironment, 0, 0, env);
	rb_obj_call_init(obj, 1, &debug);

	c9_env_map[env] = obj;

	return obj;
}

static VALUE Environment_special_channel(VALUE self, VALUE name)
{
	Environment *env;
	Data_Get_Struct(self, Environment, env);
	switch (TYPE(name))
	{
	case T_SYMBOL: return c9_to_rb(env->special_channel(rb_id2name(SYM2ID(name)))); break;
	case T_STRING: return c9_to_rb(env->special_channel(STR2CSTR(name))); break;
	}
	return Qnil;
}
static VALUE Environment_set_special_channel(VALUE self, VALUE name, VALUE channel)
{
	Environment *env;
	Data_Get_Struct(self, Environment, env);
	switch (TYPE(name))
	{
	case T_SYMBOL: env->set_special_channel(rb_id2name(SYM2ID(name)), rb_to_c9(channel)); break;
	case T_STRING: env->set_special_channel(STR2CSTR(name), rb_to_c9(channel)); break;
	}
	return Qnil;
}

static VALUE Environment_current_context(VALUE self)
{
	Environment *env;
	Data_Get_Struct(self, Environment, env);

	RunnableContext *ctx = env->context();
	if (ctx)
		return c9_to_rb(value(ctx));
	else
		return Qnil;
}

static VALUE Environment_set_current_context(VALUE self, VALUE rb_ctx)
{
	Environment *env;
	RunnableContext *ctx = NULL;
	Data_Get_Struct(self, Environment, env);
	if (rb_ctx != Qnil)
	{
		Data_Get_Struct(rb_ctx, RunnableContext, ctx);
	}

	env->run(ctx);

	return Qnil;
}

static void Init_Channel9_Environment()
{
	rb_cEnvironment = rb_define_class_under(rb_mChannel9, "Environment", rb_cObject);
	rb_define_singleton_method(rb_cEnvironment, "new", ruby_method(Environment_new), 1);
	rb_define_method(rb_cEnvironment, "special_channel", ruby_method(Environment_special_channel), 1);
	rb_define_method(rb_cEnvironment, "set_special_channel", ruby_method(Environment_set_special_channel), 2);
	rb_define_method(rb_cEnvironment, "current_context", ruby_method(Environment_current_context), 0);
	rb_define_method(rb_cEnvironment, "set_current_context", ruby_method(Environment_set_current_context), 1);
}

static VALUE Stream_new(VALUE self)
{
	IStream *stream = new IStream();
	VALUE obj = Data_Wrap_Struct(rb_cStream, 0, 0, stream);
	rb_obj_call_init(obj, 0, 0);
	return obj;
}

static VALUE Stream_add_instruction(VALUE self, VALUE name, VALUE args)
{
	Instruction instruction = {NOP, {0}, {0}, {0}};

	IStream *stream;
	Data_Get_Struct(self, IStream, stream);

	instruction.instruction = inum(STR2CSTR(name));

	size_t argc = RARRAY_LEN(args);
	if (argc > 0)
		instruction.arg1 = rb_to_c9(rb_ary_entry(args, 0));
	
	if (argc > 1)
		instruction.arg2 = rb_to_c9(rb_ary_entry(args, 1));

	if (argc > 2)
		instruction.arg3 = rb_to_c9(rb_ary_entry(args, 2));

	stream->add(instruction);

	return self;
}

static VALUE Stream_add_label(VALUE self, VALUE name)
{
	IStream *stream;
	Data_Get_Struct(self, IStream, stream);

	stream->set_label(STR2CSTR(name));

	return self;
}

static VALUE Stream_add_line_info(VALUE self, VALUE file, VALUE line, VALUE pos, VALUE extra)
{
	IStream *stream;
	Data_Get_Struct(self, IStream, stream);

	stream->set_source_pos(SourcePos(STR2CSTR(file), FIX2INT(line), FIX2INT(pos), STR2CSTR(extra)));

	return self;
}

static void Init_Channel9_Stream()
{
	rb_cStream = rb_define_class_under(rb_mChannel9, "Stream", rb_cObject);
	rb_define_singleton_method(rb_cStream, "new", ruby_method(Stream_new), 0);
	rb_define_method(rb_cStream, "add_instruction", ruby_method(Stream_add_instruction), 2);
	rb_define_method(rb_cStream, "add_label", ruby_method(Stream_add_label), 1);
	rb_define_method(rb_cStream, "add_line_info", ruby_method(Stream_add_line_info), 4);
}

static VALUE rb_Context_new(RunnableContext *ctx)
{
	VALUE obj = Data_Wrap_Struct(rb_cContext, 0, 0, ctx);
	return obj;
}

static VALUE Context_new(VALUE self, VALUE rb_env, VALUE rb_stream)
{
	Environment *env;
	IStream *stream;

	Data_Get_Struct(rb_env, Environment, env);
	Data_Get_Struct(rb_stream, IStream, stream);

	stream->normalize();
	RunnableContext *ctx = new_context(stream);
	VALUE obj = Data_Wrap_Struct(rb_cContext, 0, 0, ctx);
	VALUE argv[2] = {rb_env, rb_stream};
	rb_obj_call_init(obj, 2, argv);
	return obj;
}

static VALUE Context_channel_send(VALUE self, VALUE rb_cenv, VALUE rb_val, VALUE rb_ret)
{
	Environment *cenv;
	RunnableContext *ctx;

	Data_Get_Struct(rb_cenv, Environment, cenv);
	Data_Get_Struct(self, RunnableContext, ctx);

	ctx->send(cenv, rb_to_c9(rb_val), rb_to_c9(rb_ret));

	return Qnil;
}

static void Init_Channel9_Context()
{
	rb_cContext = rb_define_class_under(rb_mChannel9, "Context", rb_cObject);
	rb_define_singleton_method(rb_cContext, "new", ruby_method(Context_new), 2);
	rb_define_method(rb_cContext, "channel_send", ruby_method(Context_channel_send), 3);
}

static VALUE rb_CallableContext_new(CallableContext *ctx)
{
	RubyChannel *robj = dynamic_cast<RubyChannel*>(ctx);
	if (robj)
	{
		return robj->data();
	} else {
		VALUE obj = Data_Wrap_Struct(rb_cCallableContext, 0, 0, ctx);
		rb_obj_call_init(obj, 0, 0);
		return obj;
	}
}

static VALUE CallableContext_channel_send(VALUE self, VALUE rb_cenv, VALUE rb_val, VALUE rb_ret)
{
	Environment *cenv;
	CallableContext *ctx;

	Data_Get_Struct(rb_cenv, Environment, cenv);
	Data_Get_Struct(self, CallableContext, ctx);

	ctx->send(cenv, rb_to_c9(rb_val), rb_to_c9(rb_ret));

	return Qnil;
}

static void Init_Channel9_CallableContext()
{
	rb_cCallableContext = rb_define_class_under(rb_mChannel9, "CallableContext", rb_cObject);
	rb_define_method(rb_cCallableContext, "channel_send", ruby_method(CallableContext_channel_send), 3);
}
static VALUE rb_Message_new(const Message *msg)
{
	VALUE obj = Data_Wrap_Struct(rb_cMessage, 0, 0, (void*)msg);
	VALUE sysargs = rb_ary_new();
	VALUE args = rb_ary_new();

	Message::const_iterator it;
	for (it = msg->sysargs(); it != msg->sysargs_end(); it++)
	{
		rb_ary_push(sysargs, c9_to_rb(*it));
	}
	for (it = msg->args(); it != msg->args_end(); it++)
	{
		rb_ary_push(args, c9_to_rb(*it));
	}

	VALUE argv[3] = {ID2SYM(rb_intern(msg->name()->c_str())), sysargs, args};
	rb_obj_call_init(obj, 3, argv);	
	return obj;
}

static VALUE Message_new(VALUE self, VALUE name, VALUE sysargs, VALUE args)
{
	size_t sysarg_count = RARRAY_LEN(sysargs), arg_count = RARRAY_LEN(args);
	Value c9_name = rb_to_c9(name);
	assert(is(c9_name, STRING));

	Message *msg = new_message(ptr<String>(c9_name), sysarg_count, arg_count);

	Message::iterator out;
	size_t i;
	for (out = msg->sysargs(), i = 0; i < sysarg_count; i++)
	{
		*out++ = rb_to_c9(rb_ary_entry(sysargs, i));
	}
	for (out = msg->args(), i = 0; i < arg_count; i++)
	{
		*out++ = rb_to_c9(rb_ary_entry(args, i));
	}

	VALUE obj = Data_Wrap_Struct(rb_cMessage, 0, 0, msg);
	VALUE argv[3] = {name, sysargs, args};
	rb_obj_call_init(obj, 3, argv);
	return obj;
}

static void Init_Channel9_Message()
{
	rb_cMessage = rb_define_class_under(rb_mPrimitive, "Message", rb_cObject);
	rb_define_singleton_method(rb_cMessage, "new", ruby_method(Message_new), 3);
}

static void Init_Channel9_Undef()
{
	rb_cUndef = rb_define_class_under(rb_mPrimitive, "UndefC", rb_cObject);
	rb_Undef = rb_class_new_instance(0, 0, rb_cUndef);
	rb_define_const(rb_mPrimitive, "Undef", rb_Undef);
}

static VALUE Primitive_channel_send(VALUE self, VALUE cenv, VALUE val, VALUE ret)
{
	Environment *c9_cenv;
	Value c9_channel = rb_to_c9(self);
	Value c9_val = rb_to_c9(val);
	Value c9_ret = rb_to_c9(ret);

	Data_Get_Struct(cenv, Environment, c9_cenv);
	channel_send(c9_cenv, c9_channel, c9_val, c9_ret);
	return Qnil;
}

static void Init_Channel9_Primitives()
{
	rb_define_method(rb_cTrueClass, "channel_send", ruby_method(Primitive_channel_send), 3);
	rb_define_method(rb_cFalseClass, "channel_send", ruby_method(Primitive_channel_send), 3);
	rb_define_method(rb_cNilClass, "channel_send", ruby_method(Primitive_channel_send), 3);
	rb_define_method(rb_cFixnum, "channel_send", ruby_method(Primitive_channel_send), 3);
	rb_define_method(rb_cArray, "channel_send", ruby_method(Primitive_channel_send), 3);
	rb_define_method(rb_cFloat, "channel_send", ruby_method(Primitive_channel_send), 3);
	rb_define_method(rb_cString, "channel_send", ruby_method(Primitive_channel_send), 3);
	rb_define_method(rb_cSymbol, "channel_send", ruby_method(Primitive_channel_send), 3);
}

extern "C" void Init_channel9ext()
{
	rb_mChannel9 = rb_define_module("Channel9");
	rb_mPrimitive = rb_define_module_under(rb_mChannel9, "Primitive");
	Init_Channel9_Environment();
	Init_Channel9_Stream();
	Init_Channel9_Message();
	Init_Channel9_Undef();
	Init_Channel9_Context();
	Init_Channel9_CallableContext();

	Init_Channel9_Primitives();
}