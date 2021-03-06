#include "hornet/java.hh"
#include "hornet/vm.hh"

#include <classfile_constants.h>
#include <functional>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <string>

namespace hornet {

class_file::class_file(void *data, size_t size)
    : _offset(0)
    , _size(size)
    , _data(reinterpret_cast<char *>(data))
{
}

class_file::~class_file()
{
}

std::shared_ptr<klass> class_file::parse()
{
    if (!_size)
        assert(0);

    auto magic = read_u4();

    if (magic != 0xcafebabe)
        assert(0);

    /*auto minor_version = */read_u2();

    auto major_version = read_u2();

    if (major_version > JVM_CLASSFILE_MAJOR_VERSION)
        assert(0);

    auto const_pool = read_constant_pool();

    auto access_flags = read_u2();

    /*auto this_class = */read_u2();

    auto super_class = read_u2();

    auto* klass = new hornet::klass(hornet::system_loader(), const_pool);

    auto interfaces_count = read_u2();

    for (auto i = 0; i < interfaces_count; i++)
        read_u2();

    auto fields_count = read_u2();

    for (auto i = 0; i < fields_count; i++) {
        auto field = read_field_info(*const_pool);

        klass->add(field);
    }

    auto methods_count = read_u2();

    for (auto i = 0; i < methods_count; i++) {
        auto method = read_method_info(klass, *const_pool);

        klass->add(method);
    }

    auto attr_count = read_u2();

    for (auto i = 0; i < attr_count; i++) {
        read_attr_info(*const_pool);
    }

    klass->access_flags = access_flags;

    if (super_class) {
        auto super = klass->resolve_class(super_class);
        klass->super = super.get();
    } else {
        klass->super = nullptr;
    }

    return std::shared_ptr<hornet::klass>(klass);
}

std::shared_ptr<constant_pool> class_file::read_constant_pool()
{
    auto constant_pool_count = read_u2();

    assert(constant_pool_count > 0);

    auto const_pool = std::make_shared<constant_pool>(constant_pool_count);

    for (auto idx = 0; idx < constant_pool_count-1; idx++) {
        auto tag = read_u1();
        std::shared_ptr<cp_info> cp_info;
        switch (tag) {
        case JVM_CONSTANT_Class:
            cp_info = read_const_class();
            break;
        case JVM_CONSTANT_Fieldref:
            cp_info = read_const_fieldref();
            break;
        case JVM_CONSTANT_Methodref:
            cp_info = read_const_methodref();
            break;
        case JVM_CONSTANT_InterfaceMethodref:
            cp_info = read_const_interface_methodref();
            break;
        case JVM_CONSTANT_String:
            cp_info = read_const_string();
            break;
        case JVM_CONSTANT_Integer:
            cp_info = read_const_integer();
            break;
        case JVM_CONSTANT_Float:
            read_const_float();
            break;
        case JVM_CONSTANT_Long:
            cp_info = read_const_long();
            break;
        case JVM_CONSTANT_Double:
            read_const_double();
            break;
        case JVM_CONSTANT_NameAndType:
            cp_info = read_const_name_and_type();
            break;
        case JVM_CONSTANT_Utf8:
            cp_info = read_const_utf8();
            break;
        case JVM_CONSTANT_MethodHandle:
            read_const_method_handle();
            break;
        case JVM_CONSTANT_MethodType:
            read_const_method_type();
            break;
        case JVM_CONSTANT_InvokeDynamic:
            read_const_invoke_dynamic();
            break;
        default:
            fprintf(stderr, "error: tag %u not supported.\n", tag);
            assert(0);
        }
        const_pool->set(idx, cp_info);
        if (tag == JVM_CONSTANT_Long || tag == JVM_CONSTANT_Double) {
            // 8-byte constants take up two entries in the constant pool.
            idx++;
        }
    }

    return const_pool;
}

std::shared_ptr<cp_info> class_file::read_const_class()
{
    auto name_index = read_u2();

    return cp_info::const_class(name_index);
}

std::shared_ptr<cp_info> class_file::read_const_fieldref()
{
    auto class_index = read_u2();
    auto name_and_type_index = read_u2();

    return cp_info::const_fieldref(class_index, name_and_type_index);
}

std::shared_ptr<cp_info> class_file::read_const_methodref()
{
    auto class_index = read_u2();
    auto name_and_type_index = read_u2();

    return cp_info::const_methodref(class_index, name_and_type_index);
}

std::shared_ptr<cp_info> class_file::read_const_interface_methodref()
{
    auto class_index = read_u2();
    auto name_and_type_index = read_u2();

    return cp_info::const_interface_methodref(class_index, name_and_type_index);
}

std::shared_ptr<cp_info> class_file::read_const_string()
{
    auto string_index = read_u2();

    return cp_info::const_string(string_index);
}

std::shared_ptr<cp_info> class_file::read_const_integer()
{
    auto value = read_u4();
    
    return cp_info::const_integer(value);
}

void class_file::read_const_float()
{
    /*auto bytes = */read_u4();
}

std::shared_ptr<cp_info> class_file::read_const_long()
{
    auto high_bytes = read_u4();
    auto low_bytes = read_u4();

    return cp_info::const_long((uint64_t)high_bytes << 32 || (uint64_t)low_bytes);
}

void class_file::read_const_double()
{
    /*auto high_bytes = */read_u4();
    /*auto low_bytes = */read_u4();
}

std::shared_ptr<cp_info> class_file::read_const_name_and_type()
{
    auto name_index = read_u2();
    auto descriptor_index = read_u2();

    return cp_info::const_name_and_type(name_index, descriptor_index);
}

std::shared_ptr<cp_info> class_file::read_const_utf8()
{
    auto length = read_u2();

    auto ret = new const_utf8_info();

    ret->bytes = new char[length + 1];

    for (auto i = 0; i < length; i++)
        ret->bytes[i] = read_u1();

    ret->bytes[length] = '\0';

    return std::shared_ptr<const_utf8_info>(ret);
}

void class_file::read_const_method_handle()
{
    /*auto reference_kind = */read_u1();
    /*auto reference_index = */read_u2();
}

void class_file::read_const_method_type()
{
    /*auto descriptor_index = */read_u2();
}

void class_file::read_const_invoke_dynamic()
{
    /*auto bootstrap_method_attr_index = */read_u2();
    /*auto name_and_type_index = */read_u2();
}

std::shared_ptr<field> class_file::read_field_info(constant_pool &constant_pool)
{
    /*auto access_flags = */read_u2();
    auto name_index = read_u2();

    auto *cp_name = constant_pool.get_utf8(name_index);

    auto descriptor_index = read_u2();

    auto *cp_descriptor = constant_pool.get_utf8(descriptor_index);

    auto f = std::make_shared<field>();

    f->name         = cp_name->bytes;
    f->descriptor   = cp_descriptor->bytes;

    auto attr_count = read_u2();

    for (auto i = 0; i < attr_count; i++) {
        auto attr = read_attr_info(constant_pool);
    }

    return f;
}

klass jvm_void_klass(nullptr, nullptr);

static klass* parse_type(std::string descriptor, int& pos)
{
    auto ch = descriptor[pos++];
    switch (ch) {
    case 'B':
    case 'C':
    case 'D':
    case 'F':
    case 'I':
    case 'J':
    case 'S':
    case 'Z':
        break;
    case 'L':
        while (descriptor[pos++] != ';')
            ;;
        break;
    case '[':
        parse_type(descriptor, pos);
        break;
    case 'V':
        return &jvm_void_klass;
    default:
        fprintf(stderr, "%s '%c'\n", descriptor.c_str(), ch);
        assert(0);
        break;
    }
    return nullptr;
}

static void parse_method_descriptor(std::shared_ptr<method> m)
{
    int pos = 0;

    m->args_count = 0;

    assert(m->descriptor[pos++] == '(');

    while (m->descriptor[pos] != ')') {
        parse_type(m->descriptor, pos);
        m->args_count++;
    }
    m->return_type = parse_type(m->descriptor, ++pos);
}

std::shared_ptr<method> class_file::read_method_info(klass* klass, constant_pool &constant_pool)
{
    auto access_flags = read_u2();

    auto name_index = read_u2();

    auto *cp_name = constant_pool.get_utf8(name_index);

    assert(cp_name != nullptr);

    auto descriptor_index = read_u2();

    auto *cp_descriptor = constant_pool.get_utf8(descriptor_index);

    assert(cp_descriptor != nullptr);

    auto attr_count = read_u2();

    auto m = std::make_shared<method>();

    m->klass        = klass;
    m->access_flags = access_flags;
    m->name         = cp_name->bytes;
    m->descriptor   = cp_descriptor->bytes;
    m->code         = nullptr;
    m->code_length  = 0;

    parse_method_descriptor(m);

    for (auto i = 0; i < attr_count; i++) {
        auto attr = read_attr_info(constant_pool);

        switch (attr->type) {
        case attr_type::code: {
            code_attr* c = static_cast<code_attr*>(attr.get());
            m->max_locals  = c->max_locals;
            m->code        = c->code;
            m->code_length = c->code_length;
            break;
        }
        default:
            /* ignore */
            break;
        }
    }

    return m;
}

std::unique_ptr<attr_info>
class_file::read_attr_info(constant_pool& constant_pool)
{
    auto attribute_name_index = read_u2();

    auto attribute_length = read_u4();

    auto cp_name = constant_pool.get_utf8(attribute_name_index);

    if (!strcmp(cp_name->bytes, "Code")) {
        return read_code_attribute(constant_pool);
    }

    for (uint32_t i = 0; i < attribute_length; i++)
        read_u1();

    return std::unique_ptr<attr_info>(new unknown_attr);
}

std::unique_ptr<code_attr>
class_file::read_code_attribute(constant_pool& constant_pool)
{
    auto* attr = new code_attr();
    /*auto max_stack = */read_u2();
    attr->max_locals = read_u2();
    attr->code_length = read_u4();
    attr->code = new char[attr->code_length];
    for (uint32_t i = 0; i < attr->code_length; i++)
        attr->code[i] = read_u1();
    auto exception_table_length = read_u2();
    for (uint16_t i = 0; i < exception_table_length; i++) {
        /*auto start_pc = */read_u2();
        /*auto end_pc = */read_u2();
        /*auto handler_pc = */read_u2();
        /*auto catch_type = */read_u2();
    }
    auto attr_count = read_u2();
    for (auto i = 0; i < attr_count; i++) {
        read_attr_info(constant_pool);
    }
    return std::unique_ptr<code_attr>(attr);
}

uint8_t class_file::read_u1()
{
    return _data[_offset++];
}

uint16_t class_file::read_u2()
{
    return static_cast<uint16_t>(read_u1()) << 8
         | static_cast<uint16_t>(read_u1());
}

uint32_t class_file::read_u4()
{
    return static_cast<uint32_t>(read_u1()) << 24
         | static_cast<uint32_t>(read_u1()) << 16
         | static_cast<uint32_t>(read_u1()) << 8
         | static_cast<uint32_t>(read_u1());
}

uint64_t class_file::read_u8()
{
    return static_cast<uint64_t>(read_u4()) << 32 | read_u4();
}

}
