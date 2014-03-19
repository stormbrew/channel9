#pragma once

#include "c9/callable_context.hpp"
#include <ffi.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

namespace Channel9
{
	template <typename tType>
	ffi_type *ffi_type_map();
	template <>
	inline ffi_type *ffi_type_map<char>() { return &ffi_type_sint8; }
	template <>
	inline ffi_type *ffi_type_map<int8_t>() { return &ffi_type_sint8; }
	template <>
	inline ffi_type *ffi_type_map<uint8_t>() { return &ffi_type_uint8; }
	template <>
	inline ffi_type *ffi_type_map<int16_t>() { return &ffi_type_sint16; }
	template <>
	inline ffi_type *ffi_type_map<uint16_t>() { return &ffi_type_uint16; }
	template <>
	inline ffi_type *ffi_type_map<int32_t>() { return &ffi_type_sint32; }
	template <>
	inline ffi_type *ffi_type_map<uint32_t>() { return &ffi_type_uint32; }
	template <>
	inline ffi_type *ffi_type_map<int64_t>() { return &ffi_type_sint64; }
	template <>
	inline ffi_type *ffi_type_map<uint64_t>() { return &ffi_type_uint64; }

#if defined(__APPLE__)
#include "TargetConditionals.h"
# if TARGET_CPU_X86_64
	template <>
	inline ffi_type *ffi_type_map<long>() { return &ffi_type_sint64; }
	template <>
	inline ffi_type *ffi_type_map<unsigned long>() { return &ffi_type_uint64; }
# endif
#endif

	class FFIDefinition : public CallableContext
	{
		std::string m_name;
		ffi_type m_type;
		std::vector<ffi_type*> m_elements;
		std::vector<FFIDefinition*> m_ptr_types;
		std::vector<FFIDefinition*> m_dependent_definitions;
		std::map<std::string, size_t> m_name_map;

	public:
		FFIDefinition(const std::string &name) : m_name(name)
		{
			m_elements.push_back(NULL); // arguments are null terminated
			m_type.size = 0;
			m_type.alignment = 0;
			m_type.type = FFI_TYPE_STRUCT;
			m_type.elements = &m_elements[0];
		}
		FFIDefinition() : m_name("<unknown>")
		{
			m_elements.push_back(NULL); // arguments are null terminated.
			m_type.size = 0;
			m_type.alignment = 0;
			m_type.type = FFI_TYPE_STRUCT;
			m_type.elements = &m_elements[0];
		}

		size_t size()
		{
			normalize();
			return m_type.size;
		}
		size_t count() const { return m_elements.size() - 1; }

		ffi_type *get_type() { return &m_type; }
		ffi_type const *get_type() const { return &m_type; }
		ffi_type *get_type(size_t idx) const;
		FFIDefinition *get_ptr_type(size_t idx) const;
		size_t get_offset(size_t idx) const;

		void normalize();
		void add(const std::string &name, ffi_type *type);
		void add(const std::string &name, FFIDefinition *type);
		void add(ffi_type *type, size_t count = 1);
		void add(FFIDefinition *type, size_t count = 1);

		void add_pointer(const std::string &name, FFIDefinition *type);
		void add_pointer(const std::string &name);
		void add_pointer(FFIDefinition *type, size_t count = 1);
		void add_pointer(size_t count = 1); // adds string pointers.

		static const size_t not_present = -1;
		size_t name_idx(const std::string &name) const;

		void send(Environment *env, const Value &val, const Value &ret);
		void scan();
		std::string inspect() const { return std::string("FFIDefinition(") + m_name + ")"; }

		~FFIDefinition() {}
	};

	class FFIObject : public CallableContext
	{
		FFIDefinition *m_definition;
		FFIObject *m_outer; // if it's inside another object.
		char *m_blob;

	public:
		FFIObject(FFIDefinition *definition) : m_definition(definition), m_outer(NULL)
		{
			m_blob = (char*)malloc(definition->size());
		}
		FFIObject(FFIDefinition *definition, FFIObject *outer, char *blob)
		 : m_definition(definition), m_outer(outer), m_blob(blob)
		{}
		~FFIObject()
		{
			if (!m_outer)
				free(m_blob);
		}

		const FFIDefinition *definition() const { return m_definition; }

		void send(Environment *env, const Value &val, const Value &ret);
		void scan();
		std::string inspect() const;

		char *blob() const { return m_blob; }

		void set(size_t idx, const Value &val);
		Value get(size_t idx) const;

		void raw_set(size_t idx, const char *val);
		std::pair<const char*, ffi_type*> raw_get(size_t idx);
	};

	typedef void (*ffi_fn)(void);

	class FFICall : public CallableContext
	{
		std::string m_name;

		ffi_fn m_func;

		ffi_cif m_cif;

		ffi_type *m_return;
		FFIDefinition *m_return_def; // only if m_return is a structure.

		FFIDefinition *m_args;

		void build_cif();

	public:
		FFICall(const std::string &name, ffi_fn func, ffi_type *ret, FFIDefinition *args)
		 : m_name(name), m_func(func), m_return(ret), m_return_def(NULL), m_args(args)
		{ build_cif(); }
		FFICall(const std::string &name, ffi_fn func, FFIDefinition *ret, FFIDefinition *args)
		 : m_name(name), m_func(func), m_return(ret->get_type()), m_return_def(NULL), m_args(args)
		{ build_cif(); }
		~FFICall() {}

		void send(Environment *env, const Value &val, const Value &ret);
		void scan();
		std::string inspect() const;
	};
}
