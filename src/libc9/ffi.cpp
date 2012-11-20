#include <sstream>
#include <stdexcept>

#include "c9/channel9.hpp"
#include "c9/context.hpp"
#include "c9/ffi.hpp"

using namespace Channel9;

void ffi_map(ffi_type *type, FFIDefinition *ptr_type, const Value &val, char *buffer)
{
	switch (type->type)
	{
	case FFI_TYPE_SINT8:
		if (is_number(val))
		{
			*(int8_t*)buffer = (int8_t)val.machine_num;
			return;
		} else {
			throw std::runtime_error("Passed a non-number to an int8 argument.");
		}
	case FFI_TYPE_SINT16:
		if (is_number(val))
		{
			*(int16_t*)buffer = (int16_t)val.machine_num;
			return;
		} else {
			throw std::runtime_error("Passed a non-number to an int16 argument.");
		}
	case FFI_TYPE_SINT32:
		if (is_number(val))
		{
			*(int32_t*)buffer = (int32_t)val.machine_num;
			return;
		} else {
			throw std::runtime_error("Passed a non-number to an int32 argument.");
		}
	case FFI_TYPE_SINT64:
		if (is_number(val))
		{
			*(int64_t*)buffer = (int64_t)val.machine_num;
			return;
		} else {
			throw std::runtime_error("Passed a non-number to an int64 argument.");
		}
	case FFI_TYPE_UINT8:
		if (is_number(val))
		{
			*(uint8_t*)buffer = (uint8_t)val.machine_num;
			return;
		} else {
			throw std::runtime_error("Passed a non-number to an uint8 argument.");
		}
	case FFI_TYPE_UINT16:
		if (is_number(val))
		{
			*(uint16_t*)buffer = (uint16_t)val.machine_num;
			return;
		} else {
			throw std::runtime_error("Passed a non-number to an uint16 argument.");
		}
	case FFI_TYPE_UINT32:
		if (is_number(val))
		{
			*(uint32_t*)buffer = (uint32_t)val.machine_num;
			return;
		} else {
			throw std::runtime_error("Passed a non-number to an uint32 argument.");
		}
	case FFI_TYPE_UINT64:
		if (is_number(val))
		{
			*(uint64_t*)buffer = (uint64_t)val.machine_num;
			return;
		} else {
			throw std::runtime_error("Passed a non-number to an uint64 argument.");
		}

	case FFI_TYPE_FLOAT:
		if (Channel9::type(val) == FLOAT_NUM)
		{
			*(float*)buffer = (float)float_num(val);
			return;
		} else {
			throw std::runtime_error("Passed a non-float to a float argument.");
		}

	case FFI_TYPE_DOUBLE:
		if (Channel9::type(val) == FLOAT_NUM)
		{
			*(double*)buffer = (double)float_num(val);
			return;
		} else {
			throw std::runtime_error("Passed a non-float to a double argument.");
		}

	case FFI_TYPE_POINTER:
		if (!ptr_type) // is a string
		{
			char const **buff_ptr = reinterpret_cast<char const **>(buffer);
			*buff_ptr = ptr<String>(val)->c_str();
			return;
		} else if (is(val, CALLABLE_CONTEXT)) {
			CallableContext *context = ptr<CallableContext>(val);
			FFIObject *obj = dynamic_cast<FFIObject*>(context);
			if (obj && obj->definition() == ptr_type)
			{
				char const **buff_ptr = reinterpret_cast<char const **>(buffer);
				*buff_ptr = obj->blob();
				return;
			}
		} else if (is(val, NIL)) {
			void const **buff_ptr = reinterpret_cast<void const **>(buffer);
			*buff_ptr = NULL;
			return;
		} else {
			throw std::runtime_error("Unknown pointer type.");
		}
	default: break;
	}
	throw std::runtime_error("Unknown FFI type.");
}

Value ffi_unmap(ffi_type *type, FFIDefinition *ptr_type, FFIObject *outer, char *buffer)
{
	switch (type->type)
	{
	case FFI_TYPE_SINT8:
		return value(int64_t(*(int8_t*)buffer));
	case FFI_TYPE_SINT16:
		return value(int64_t(*(int16_t*)buffer));
	case FFI_TYPE_SINT32:
		return value(int64_t(*(int32_t*)buffer));
	case FFI_TYPE_SINT64:
		return value(int64_t(*(int64_t*)buffer));
	case FFI_TYPE_UINT8:
		return value(int64_t(*(uint8_t*)buffer));
	case FFI_TYPE_UINT16:
		return value(int64_t(*(uint16_t*)buffer));
	case FFI_TYPE_UINT32:
		return value(int64_t(*(uint32_t*)buffer));
	case FFI_TYPE_UINT64:
		return value(int64_t(*(uint64_t*)buffer));

	case FFI_TYPE_FLOAT:
		return value(double(*(float*)buffer));
	case FFI_TYPE_DOUBLE:
		return value(double(*(double*)buffer));

	case FFI_TYPE_STRUCT:
		return value(new FFIObject(ptr_type, outer, buffer));

	case FFI_TYPE_POINTER:
		if (!ptr_type)
			return value(new_string(*(char**)buffer));
		// TODO: else it's a structure, deal with it.

	default: break;
	}
	throw std::runtime_error("Could not unmap FFI type");
}

void FFIDefinition::add(const std::string &name, ffi_type *type)
{
	m_name_map[name] = count();
	add(type);
}
void FFIDefinition::add(const std::string &name, FFIDefinition *type)
{
	m_name_map[name] = count();
	add(type);
}

void FFIDefinition::add(ffi_type *type, size_t count)
{
	for (size_t i = 0; i < count; i++)
	{
		m_elements.insert(m_elements.end()-1, type);
		m_ptr_types.push_back(NULL);
	}
	m_type.elements = &m_elements[0];
}
void FFIDefinition::add(FFIDefinition *type, size_t count)
{
	m_dependent_definitions.push_back(type);
	for (size_t i = 0; i < count; i++)
	{
		m_elements.insert(m_elements.end()-1, &type->m_type);
		m_ptr_types.push_back(type);
	}
	m_type.elements = &m_elements[0];
}
void FFIDefinition::add_pointer(FFIDefinition *type, size_t count)
{
	m_dependent_definitions.push_back(type);
	for (size_t i = 0; i < count; i++)
	{
		m_elements.insert(m_elements.end()-1, &ffi_type_pointer);
		m_ptr_types.push_back(type);
	}
	m_type.elements = &m_elements[0];
}
void FFIDefinition::add_pointer(size_t count)
{
	// this is for adding a 'string' blob, so
	// deal with it that way.
	add(&ffi_type_pointer, count);
}
void FFIDefinition::add_pointer(const std::string &name, FFIDefinition *type)
{
	m_name_map[name] = count();
	add_pointer(type);
}
void FFIDefinition::add_pointer(const std::string &name)
{
	m_name_map[name] = count();
	add_pointer();
}

size_t FFIDefinition::name_idx(const std::string &name) const
{
	std::map<std::string, size_t>::const_iterator it = m_name_map.find(name);
	if (it == m_name_map.end())
		return -1;
	else
		return it->second;
}

void FFIDefinition::normalize()
{
	if (m_type.size == 0)
	{
		ffi_cif cif;
		ffi_type *list = &m_type;
		if (FFI_OK != ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 1, &ffi_type_void, &list) || m_type.size == 0)
			throw std::runtime_error("Error with ffi_prep_cif"); // TODO: Make this blow up in a better way.
	}
}

ffi_type *FFIDefinition::get_type(size_t idx) const
{
	return m_elements[idx];
}
FFIDefinition *FFIDefinition::get_ptr_type(size_t idx) const
{
	return m_ptr_types[idx];
}
size_t FFIDefinition::get_offset(size_t idx) const
{
	// break out early for the first element.
	if (idx == 0)
		return 0;

	std::vector<ffi_type*>::const_iterator it = m_elements.begin();
	size_t i = 0;
	size_t offset = 0;
	do
	{
		// move up the size of the element
		offset += (*it)->size;
		it++;

		// move up to correct for alignment.
		size_t alignment = (*it)->alignment;
		size_t unalign = offset % alignment;
		if (unalign != 0) offset += alignment - unalign;

		i++;
	} while (i < idx);
	return offset;
}

void FFIDefinition::send(Environment *env, const Value &val, const Value &ret)
{
	// allocate a new object and return it.
	FFIObject *obj = new FFIObject(this);
	channel_send(env, ret, value(obj), Nil);
}
void FFIDefinition::scan()
{
	for (std::vector<FFIDefinition*>::const_iterator it = m_dependent_definitions.begin(); it != m_dependent_definitions.end(); it++)
	{
		gc_scan(*it);
	}
	CallableContext::scan();
}

void FFIObject::send(Environment *env, const Value &val, const Value &ret)
{
	if (is(val, MESSAGE))
	{
		Message *msg = ptr<Message>(val);
		Value idx;
		size_t inum = -1;
		Value val;

		if (msg->arg_count() > 0)
		{
			idx = msg->args()[0];
			if (is_number(idx))
				inum = (size_t)idx.machine_num;
			else if (is(idx, STRING)) {
				String *s = ptr<String>(idx);
				inum = m_definition->name_idx(s->str());
			}
			if (inum == -1 || inum >= m_definition->count())
				throw std::runtime_error("Invalid index.");
			switch (msg->arg_count())
			{
			case 1:
				channel_send(env, ret, get(inum), Nil);
				return;
			case 2:
				val = msg->args()[1];
				set(inum, val);
				channel_send(env, ret, val, Nil);
				return;
			}
		}
	}
	throw std::runtime_error("Unknown or invalid message send to FFIObject");
}
void FFIObject::scan()
{
	gc_scan(m_definition);
	if (m_outer)
		gc_scan(m_outer);
	CallableContext::scan();
}
std::string FFIObject::inspect() const
{
	std::stringstream stream;
	stream << "FFIObject<" << m_definition->inspect() << ">(0x" << this << ")";
	return stream.str();
}

void FFIObject::raw_set(size_t idx, const char *val)
{
	size_t off = m_definition->get_offset(idx);
	ffi_type *type = m_definition->get_type(idx);
	memcpy(m_blob + off, val, type->size);
}
std::pair<const char*, ffi_type*> FFIObject::raw_get(size_t idx)
{
	size_t off = m_definition->get_offset(idx);
	ffi_type *type = m_definition->get_type(idx);
	return std::make_pair(m_blob + off, type);
}

void FFIObject::set(size_t idx, const Value &val)
{
	size_t off = m_definition->get_offset(idx);
	ffi_type *type = m_definition->get_type(idx);
	FFIDefinition *ptr_type = m_definition->get_ptr_type(idx);
	ffi_map(type, ptr_type, val, m_blob + off);
}
Value FFIObject::get(size_t idx) const
{
	size_t off = m_definition->get_offset(idx);
	ffi_type *type = m_definition->get_type(idx);
	FFIDefinition *ptr_type = m_definition->get_ptr_type(idx);
	return ffi_unmap(type, ptr_type, const_cast<FFIObject*>(this), m_blob + off);
}

void FFICall::build_cif()
{
	if (FFI_OK != ffi_prep_cif(&m_cif, FFI_DEFAULT_ABI, m_args->count(), m_return, m_args->get_type()->elements))
		throw std::runtime_error("Error with ffi_prep_cif"); // TODO: Make this blow up in a better way.
}

void FFICall::send(Environment *env, const Value &val, const Value &ret)
{
	if (is(val, MESSAGE))
	{
		Message *msg = ptr<Message>(val);
		if (msg->arg_count() != m_args->count())
			throw std::runtime_error("Incorrect number of arguments passed to FFICall");
		void **args = (void**)alloca(m_args->count());
		Message::const_iterator it = msg->args();
		size_t idx = 0;
		for (it = msg->args(); it != msg->args_end(); it++, idx++)
		{
			ffi_type *type = m_args->get_type(idx);
			FFIDefinition *ptr_type = m_args->get_ptr_type(idx);
			char *block = (char*)alloca(type->size);

			ffi_map(type, ptr_type, *it, block);
			args[idx] = block;
		}

		char *ret_buf = (char*)alloca(m_return->size);
		ffi_call(&m_cif, m_func, ret_buf, args);

		Value ret_val = ffi_unmap(m_return, NULL, NULL, ret_buf);
		channel_send(env, ret, ret_val, Nil);
	} else {
		throw std::runtime_error("Unknown message type for FFICall.");
	}
}
void FFICall::scan()
{
	gc_scan(m_args);
	if (m_return_def)
		gc_scan(m_return_def);
	CallableContext::scan();
}

std::string FFICall::inspect() const
{
	std::stringstream stream;
	stream << "FFICall(" << m_name << "@0x" << this << ")";
	return stream.str();
}
