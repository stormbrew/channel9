extern "C" {
#	include "ruby.h"
//#	include "intern.h"
}
#include "environment.hpp"
#include "context.hpp"
#include "value.hpp"
#include "istream.hpp"
#include "message.hpp"

#include <map>
#include <exception>
#include <stdint.h>

using namespace Channel9;

const char *STR2CSTR(VALUE val)
{
	return StringValuePtr(val);
}

typedef VALUE (*ruby_method)(ANYARGS);
typedef VALUE (*ruby_protected)(VALUE);
typedef void (*mark_method)();
typedef void (*free_method)();

VALUE rb_mChannel9;
VALUE rb_mPrimitive;

VALUE rb_cEnvironment;
VALUE rb_cStream;
VALUE rb_cContext;
VALUE rb_cCallableContext;
VALUE rb_cRunningContext;
VALUE rb_cRunnableContext;
VALUE rb_cMessage;
VALUE rb_cUndef;
VALUE rb_Undef;

static VALUE c9_to_rb(const Value &val);
static GCRef<Value> rb_to_c9(VALUE val);
static VALUE rb_Environment_new(Environment *env);
static VALUE rb_Message_new(GCRef<Message*> msg);
static VALUE rb_CallableContext_new(GCRef<CallableContext*> ctx);
static VALUE rb_Context_new(GCRef<RunnableContext*> ctx);
static VALUE rb_RunningContext_new(GCRef<RunningContext*> ctx);

class RubyUnwind : public std::exception
{
private:
	int m_jump_tag;

public:
	RubyUnwind(int jtag) : m_jump_tag(jtag) {}

	void ruby_jump() const
	{
		rb_jump_tag(m_jump_tag);
	}
};

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

	static VALUE protected_send(VALUE *stuff)
	{
		return rb_funcall(stuff[0], rb_intern("channel_send"), 3, stuff[1], stuff[2], stuff[3]);
	}

	void send(Environment *env, const Value &val, const Value &ret)
	{
//		TRACE_PRINTF(TRACE_GENERAL, TRACE_INFO, "Oh hi %s\n", STR2CSTR(rb_funcall(rb_class_of(m_val), rb_intern("to_s"), 0)));
		int error = 0;
		VALUE vals[] = {m_val, rb_Environment_new(env), c9_to_rb(val), c9_to_rb(ret)};
		rb_protect(ruby_protected(&protected_send), (VALUE)vals, &error);
		if (error)
			throw RubyUnwind(error);
	}

	void scan()
	{}
};


template <typename tVal>
GCRef<tVal> *get_gc_ref_p(VALUE obj)
{
	GCRef<tVal> *rptr;
	assert(TYPE(obj) == T_DATA);
	Data_Get_Struct(obj, GCRef<tVal>, rptr);
	return rptr;
}

template <typename tVal>
GCRef<tVal> &get_gc_ref(VALUE obj)
{
	return *get_gc_ref_p<tVal>(obj);
}

template <typename tVal>
tVal &get_gc_val(VALUE obj)
{
	return **get_gc_ref_p<tVal>(obj);
}

template <typename tVal>
struct stupid_shim_for_old_gcc {
	static void free_gc_ref(GCRef<tVal> *obj)
	{
		delete obj;
	}
};

template <typename tVal>
VALUE wrap_gc_ref(VALUE klass, const GCRef<tVal> &ref)
{
	GCRef<tVal> *rptr = new GCRef<tVal>(ref);
	return Data_Wrap_Struct(klass, 0, free_method(&stupid_shim_for_old_gcc<tVal>::free_gc_ref), rptr);
}

static GCRef<Value> rb_to_c9(VALUE val)
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
		return value(int64_t(-1)); // TODO: Make this not stupid.
	case T_FIXNUM:
		return value(NUM2LL(val));
	case T_FLOAT:
		return value(NUM2DBL(val));
	case T_ARRAY: {
		size_t len = RARRAY_LEN(val);
		GCRef<Tuple*> tuple = new_tuple(len);

		// this is as ugly as it is because it needs to
		// not accidentally push data to the old tuple reference
		// if one of the rb_to_c9 expressions causes a garbage
		// collections.
		bzero(tuple->m_data, sizeof(Value)*len);
		for (size_t i = 0; i < len; ++i)
		{
			GCRef<Value> c9_val = rb_to_c9(rb_ary_entry(val, i));
			(*tuple)->begin()[i] = *c9_val;
		}
		return value(*tuple);
		}
	case T_MODULE:
	case T_CLASS:
	case T_OBJECT:
		if (rb_respond_to(val, rb_intern("channel_send")))
		{
			Value c9val = value(new RubyChannel(val));
			TRACE_PRINTF(TRACE_GENERAL, TRACE_INFO, "Converting object %s to RubyChannel: %s\n",
				STR2CSTR(rb_any_to_s(val)), inspect(c9val).c_str());
			return c9val;
		} else if (val == rb_Undef) {
			return Undef;
		}
		break;
	case T_DATA: {
		VALUE klass = rb_class_of(val);
		if (klass == rb_cCallableContext) {
			CallableContext *ctx = get_gc_val<CallableContext*>(val);
			return value(ctx);
		} else if (klass == rb_cContext) {
			RunnableContext *ctx = get_gc_val<RunnableContext*>(val);
			return value(ctx);
		} else if (klass == rb_cRunningContext) {
			RunningContext *ctx = get_gc_val<RunningContext*>(val);
			return value(ctx);
		} else if (klass == rb_cMessage) {
			Message *msg = get_gc_val<Message*>(val);
			return value(msg);
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
	case FLOAT_NUM:
		return rb_float_new(float_num(val));
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
		return rb_Message_new(gc_ref(ptr<Message>(val)));
	case CALLABLE_CONTEXT:
		return rb_CallableContext_new(gc_ref(ptr<CallableContext>(val)));
	case RUNNABLE_CONTEXT:
		return rb_Context_new(gc_ref(ptr<RunnableContext>(val)));
	case RUNNING_CONTEXT:
		return rb_RunningContext_new(gc_ref(ptr<RunningContext>(val)));
	default:
		printf("Unknown value type %i\n", type(val));
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

static VALUE Environment_new(VALUE self, VALUE debug) try
{
	Environment *env = new Environment();
	VALUE obj = Data_Wrap_Struct(rb_cEnvironment, 0, 0, env);
	rb_obj_call_init(obj, 1, &debug);

	c9_env_map[env] = obj;

	return obj;
} catch (const RubyUnwind &unwind) { unwind.ruby_jump(); return Qnil; }

static VALUE Environment_special_channel(VALUE self, VALUE name) try
{
	Environment *env;
	assert(TYPE(self) == T_DATA);
	Data_Get_Struct(self, Environment, env);
	switch (TYPE(name))
	{
	case T_SYMBOL: return c9_to_rb(env->special_channel(rb_id2name(SYM2ID(name)))); break;
	case T_STRING: return c9_to_rb(env->special_channel(STR2CSTR(name))); break;
	}
	return Qnil;
} catch (const RubyUnwind &unwind) { unwind.ruby_jump(); return Qnil; }
static VALUE Environment_set_special_channel(VALUE self, VALUE name, VALUE channel) try
{
	Environment *env;
	assert(TYPE(self) == T_DATA);
	Data_Get_Struct(self, Environment, env);
	switch (TYPE(name))
	{
	case T_SYMBOL: env->set_special_channel(rb_id2name(SYM2ID(name)), *rb_to_c9(channel)); break;
	case T_STRING: env->set_special_channel(STR2CSTR(name), *rb_to_c9(channel)); break;
	}
	return Qnil;
} catch (const RubyUnwind &unwind) { unwind.ruby_jump(); return Qnil; }

static VALUE Environment_current_context(VALUE self) try
{
	Environment *env;
	assert(TYPE(self) == T_DATA);
	Data_Get_Struct(self, Environment, env);

	RunningContext *ctx = env->context();
	if (ctx)
		return c9_to_rb(value(ctx));
	else
		return Qnil;
} catch (const RubyUnwind &unwind) { unwind.ruby_jump(); return Qnil; }

static VALUE Environment_set_current_context(VALUE self, VALUE rb_ctx) try
{
	Environment *env;
	RunningContext *ctx = NULL;
	assert(TYPE(self) == T_DATA);
	Data_Get_Struct(self, Environment, env);
	if (rb_ctx != Qnil)
	{
		ctx = get_gc_val<RunningContext*>(rb_ctx);
	}

	env->run(ctx);

	return Qnil;
} catch (const RubyUnwind &unwind) { unwind.ruby_jump(); return Qnil; }

static void Init_Channel9_Environment()
{
	rb_cEnvironment = rb_define_class_under(rb_mChannel9, "Environment", rb_cObject);
	rb_define_singleton_method(rb_cEnvironment, "new", ruby_method(Environment_new), 1);
	rb_define_method(rb_cEnvironment, "special_channel", ruby_method(Environment_special_channel), 1);
	rb_define_method(rb_cEnvironment, "set_special_channel", ruby_method(Environment_set_special_channel), 2);
	rb_define_method(rb_cEnvironment, "current_context", ruby_method(Environment_current_context), 0);
	rb_define_method(rb_cEnvironment, "set_current_context", ruby_method(Environment_set_current_context), 1);
}

static VALUE Stream_new(VALUE self) try
{
	IStream *stream = new IStream();
	VALUE obj = wrap_gc_ref(rb_cStream, gc_ref(stream));
	rb_obj_call_init(obj, 0, 0);
	return obj;
} catch (const RubyUnwind &unwind) { unwind.ruby_jump(); return Qnil; }

static VALUE Stream_add_instruction(VALUE self, VALUE name, VALUE args) try
{
	Instruction instruction = {NOP, {0}, {0}, {0}};

	assert(TYPE(self) == T_DATA);
	GCRef<IStream*> stream = get_gc_ref<IStream*>(self);

	instruction.instruction = inum(STR2CSTR(name));

	size_t argc = RARRAY_LEN(args);
	if (argc > 0)
		instruction.arg1 = *rb_to_c9(rb_ary_entry(args, 0));

	if (argc > 1)
		instruction.arg2 = *rb_to_c9(rb_ary_entry(args, 1));

	if (argc > 2)
		instruction.arg3 = *rb_to_c9(rb_ary_entry(args, 2));

	(*stream)->add(instruction);

	return self;
} catch (const RubyUnwind &unwind) { unwind.ruby_jump(); return Qnil; }

static VALUE Stream_add_label(VALUE self, VALUE name) try
{
	assert(TYPE(self) == T_DATA);
	GCRef<IStream*> stream = get_gc_ref<IStream*>(self);

	(*stream)->set_label(STR2CSTR(name));

	return self;
} catch (const RubyUnwind &unwind) { unwind.ruby_jump(); return Qnil; }

static VALUE Stream_add_line_info(VALUE self, VALUE file, VALUE line, VALUE pos, VALUE extra) try
{
	assert(TYPE(self) == T_DATA);
	GCRef<IStream*> stream = get_gc_ref<IStream*>(self);

	(*stream)->set_source_pos(SourcePos(STR2CSTR(file), FIX2INT(line), FIX2INT(pos), STR2CSTR(extra)));

	return self;
} catch (const RubyUnwind &unwind) { unwind.ruby_jump(); return Qnil; }

static void Init_Channel9_Stream()
{
	rb_cStream = rb_define_class_under(rb_mChannel9, "Stream", rb_cObject);
	rb_define_singleton_method(rb_cStream, "new", ruby_method(Stream_new), 0);
	rb_define_method(rb_cStream, "add_instruction", ruby_method(Stream_add_instruction), 2);
	rb_define_method(rb_cStream, "add_label", ruby_method(Stream_add_label), 1);
	rb_define_method(rb_cStream, "add_line_info", ruby_method(Stream_add_line_info), 4);
}

static VALUE rb_Context_new(GCRef<RunnableContext*> ctx)
{
	VALUE obj = wrap_gc_ref(rb_cContext, ctx);
	return obj;
}

static VALUE rb_RunningContext_new(GCRef<RunningContext*> ctx)
{
	VALUE obj = wrap_gc_ref(rb_cRunningContext, ctx);
	return obj;
}

static VALUE Context_new(VALUE self, VALUE rb_env, VALUE rb_stream) try
{
	Environment *env;

	assert(TYPE(rb_env) == T_DATA);
	Data_Get_Struct(rb_env, Environment, env);
	assert(TYPE(rb_stream) == T_DATA);
	GCRef<IStream*> stream = get_gc_ref<IStream*>(rb_stream);

	(*stream)->normalize();
	GCRef<RunnableContext*> ctx = new_context(*stream);
	(*ctx)->new_scope(new_variable_frame(*stream));
	VALUE obj = wrap_gc_ref(rb_cContext, ctx);
	VALUE argv[2] = {rb_env, rb_stream};
	rb_obj_call_init(obj, 2, argv);
	return obj;
} catch (const RubyUnwind &unwind) { unwind.ruby_jump(); return Qnil; }

static VALUE Context_channel_send(VALUE self, VALUE rb_cenv, VALUE rb_val, VALUE rb_ret) try
{
	Environment *cenv;
	assert(TYPE(rb_cenv) == T_DATA);
	Data_Get_Struct(rb_cenv, Environment, cenv);

	GCRef<Value> ctx = value(get_gc_val<RunnableContext*>(self));
	GCRef<Value> val = rb_to_c9(rb_val);
	GCRef<Value> ret = rb_to_c9(rb_ret);

	channel_send(cenv, *ctx, *val, *ret);

	return Qnil;
} catch (const RubyUnwind &unwind) { unwind.ruby_jump(); return Qnil; }

static void Init_Channel9_Context()
{
	rb_cContext = rb_define_class_under(rb_mChannel9, "Context", rb_cObject);
	rb_define_singleton_method(rb_cContext, "new", ruby_method(Context_new), 2);
	rb_define_method(rb_cContext, "channel_send", ruby_method(Context_channel_send), 3);
}

static VALUE RunningContext_channel_send(VALUE self, VALUE rb_cenv, VALUE rb_val, VALUE rb_ret) try
{
	Environment *cenv;
	assert(TYPE(rb_cenv) == T_DATA);
	Data_Get_Struct(rb_cenv, Environment, cenv);

	GCRef<Value> ctx = value(get_gc_val<RunningContext*>(self));
	GCRef<Value> val = rb_to_c9(rb_val);
	GCRef<Value> ret = rb_to_c9(rb_ret);

	channel_send(cenv, *ctx, *val, *ret);

	return Qnil;
} catch (const RubyUnwind &unwind) { unwind.ruby_jump(); return Qnil; }

static VALUE RunningContext_line_info(VALUE self)
{
	GCRef<RunningContext*> &ctx = get_gc_ref<RunningContext*>(self);
	SourcePos pos = (*ctx)->m_instructions->source_pos(ctx->m_pos);

	VALUE linfo = rb_ary_new();
	rb_ary_push(linfo, rb_str_new2(pos.file.c_str()));
	rb_ary_push(linfo, INT2NUM(pos.line_num));
	rb_ary_push(linfo, INT2NUM(pos.column));
	rb_ary_push(linfo, rb_str_new2(pos.annotation.c_str()));

	return linfo;
}

static VALUE RunningContext_caller(VALUE self)
{
	GCRef<RunningContext*> &ctx = get_gc_ref<RunningContext*>(self);

	if (ctx->m_caller)
		return rb_RunningContext_new((*ctx)->m_caller);
	else
		return Qnil;
}

static void Init_Channel9_RunningContext()
{
	rb_cRunningContext = rb_define_class_under(rb_mChannel9, "RunningContext", rb_cObject);
	rb_define_method(rb_cRunningContext, "channel_send", ruby_method(RunningContext_channel_send), 3);
	rb_define_method(rb_cRunningContext, "line_info", ruby_method(RunningContext_line_info), 0);
	rb_define_method(rb_cRunningContext, "caller", ruby_method(RunningContext_caller), 0);
}

static VALUE rb_CallableContext_new(GCRef<CallableContext*> ctx_p)
{
	RubyChannel *robj = dynamic_cast<RubyChannel*>(*ctx_p);
	if (robj)
	{
		return robj->data();
	} else {
		VALUE obj = wrap_gc_ref(rb_cCallableContext, ctx_p);
		rb_obj_call_init(obj, 0, 0);
		return obj;
	}
}

static VALUE CallableContext_channel_send(VALUE self, VALUE rb_cenv, VALUE rb_val, VALUE rb_ret) try
{
	Environment *cenv;
	assert(TYPE(rb_cenv) == T_DATA);
	Data_Get_Struct(rb_cenv, Environment, cenv);

	assert(TYPE(self) == T_DATA);
	CallableContext *ctx = get_gc_val<CallableContext*>(self);

	ctx->send(cenv, *rb_to_c9(rb_val), *rb_to_c9(rb_ret));

	return Qnil;
} catch (const RubyUnwind &unwind) { unwind.ruby_jump(); return Qnil; }

static void Init_Channel9_CallableContext()
{
	rb_cCallableContext = rb_define_class_under(rb_mChannel9, "CallableContext", rb_cObject);
	rb_define_method(rb_cCallableContext, "channel_send", ruby_method(CallableContext_channel_send), 3);
}
static VALUE rb_Message_new(GCRef<Message*> msg_p)
{
	Message *msg = *msg_p;
	VALUE obj = wrap_gc_ref(rb_cMessage, msg_p);
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

	VALUE argv[3] = {ID2SYM(rb_intern((*msg_p)->name()->c_str())), sysargs, args};
	rb_obj_call_init(obj, 3, argv);
	return obj;
}

static VALUE Message_new(VALUE self, VALUE name, VALUE sysargs, VALUE args) try
{
	size_t sysarg_count = RARRAY_LEN(sysargs), arg_count = RARRAY_LEN(args);
	GCRef<Value> c9_name = rb_to_c9(name);
	assert(is(*c9_name, STRING));

	GCRef<Message*> msg = new_message(*c9_name, sysarg_count, arg_count);
	bzero(msg->m_data, sizeof(Value)*(sysarg_count + arg_count));

	size_t i;
	for (i = 0; i < sysarg_count; i++)
	{
		GCRef<Value> val = rb_to_c9(rb_ary_entry(sysargs, i));
		(*msg)->sysargs()[i] = *val;
	}
	for (i = 0; i < arg_count; i++)
	{
		GCRef<Value> val = rb_to_c9(rb_ary_entry(args, i));
		(*msg)->args()[i] = *val;
	}

	VALUE obj = wrap_gc_ref(rb_cMessage, msg);
	VALUE argv[3] = {name, sysargs, args};
	rb_obj_call_init(obj, 3, argv);
	return obj;
} catch (const RubyUnwind &unwind) { unwind.ruby_jump(); return Qnil; }

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

static VALUE Primitive_channel_send(VALUE self, VALUE cenv, VALUE val, VALUE ret) try
{
	Environment *c9_cenv;
	GCRef<Value> c9_channel = rb_to_c9(self);
	GCRef<Value> c9_val = rb_to_c9(val);
	GCRef<Value> c9_ret = rb_to_c9(ret);

	assert(TYPE(cenv) == T_DATA);
	Data_Get_Struct(cenv, Environment, c9_cenv);
	channel_send(c9_cenv, *c9_channel, *c9_val, *c9_ret);
	return Qnil;
} catch (const RubyUnwind &unwind) { unwind.ruby_jump(); return Qnil; }

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
	Init_Channel9_RunningContext();
	Init_Channel9_CallableContext();

	Init_Channel9_Primitives();
}

