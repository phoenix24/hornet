#ifndef HORNET_VM_HH
#define HORNET_VM_HH

#include <hornet/gc.hh>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <mutex>

namespace hornet {

class constant_pool;
struct method;
struct field;
struct klass;
class loader;

class jvm {
public:
    void register_klass(std::shared_ptr<klass> klass);
    void invoke(method* method);

private:
    std::mutex _mutex;
    std::vector<std::shared_ptr<klass>> _klasses;
};

extern jvm *_jvm;

typedef uint64_t value_t;

struct object {
    struct klass *klass;

    object(struct klass* _klass) : klass(_klass) {}
};

using method_list_type = std::vector<std::shared_ptr<method>>;
using field_list_type = std::vector<std::shared_ptr<field>>;

struct klass {
    struct object object;
    std::string   name;
    klass*        super;
    uint16_t      access_flags;

    klass(loader* loader, std::shared_ptr<constant_pool> const_pool);
    ~klass();

    void add(std::shared_ptr<method> method);
    void add(std::shared_ptr<field> field);
    bool verify();

    std::shared_ptr<constant_pool> const_pool() const {
        return _const_pool;
    }

    bool is_subclass_of(klass* klass) {
        auto* super = this;
        while (super != nullptr) {
            if (super == klass) {
                return true;
            }
            super = super->super;
        }
        return false;
    }

    std::shared_ptr<method> lookup_method(std::string name, std::string desciptor);
    std::shared_ptr<field> lookup_field(std::string name, std::string desciptor);

    std::shared_ptr<klass>  resolve_class (uint16_t idx);
    std::shared_ptr<field>  resolve_field (uint16_t idx);
    std::shared_ptr<method> resolve_method(uint16_t idx);

private:
    std::shared_ptr<constant_pool> _const_pool;
    method_list_type _methods;
    field_list_type _fields;
    loader* _loader;
};

struct field {
    value_t     value;
    std::string name;
    std::string descriptor;

    field();
    ~field();

    bool matches(std::string name, std::string descriptor);
};

struct method {
    // Method lifecycle is tied to the class it belongs to. Use a pointer to
    // klass instead of a smart pointer to break the cyclic dependency during
    // destruction.
    struct klass* klass;
    uint16_t    access_flags;
    std::string name;
    std::string descriptor;
    struct klass* return_type;
    uint16_t    args_count;
    uint16_t    max_locals;
    char*       code;
    uint32_t    code_length;

    method();
    ~method();

    bool is_init() const {
        return name[0] == '<';
    }

    bool matches(std::string name, std::string descriptor);
};

struct array {
    struct object object;
    uint32_t length;

    array(klass* klass, uint32_t length)
        : object(klass)
        , length(length) {}
};

class thread {
public:
    thread();
    ~thread();

    object *exception;

    static thread *current() {
        static thread thread;

        return &thread;
    }

    template<typename T>
    T* alloc() { return alloc<T>(0); }

    template<typename T>
    T* alloc(size_t extra) {
        auto size = sizeof(T) + extra;
        if (!_alloc_buffer->is_enough_space(size)) {
            _alloc_buffer = memory_block::swap(_alloc_buffer);
        }
        if (!_alloc_buffer) {
            return nullptr;
        }
        auto p = _alloc_buffer->alloc(size);
        return reinterpret_cast<T*>(p);
    }

private:
    memory_block* _alloc_buffer;

};

inline void throw_exception(struct object *exception)
{
    thread *current = thread::current();

    current->exception = exception;
}

#define java_lang_NoClassDefFoundError reinterpret_cast<hornet::object *>(0xdeabeef)
#define java_lang_NoSuchMethodError reinterpret_cast<hornet::object *>(0xdeabeef)
#define java_lang_VerifyError reinterpret_cast<hornet::object *>(0xdeabeef)

object* gc_new_object(klass* klass);
array* gc_new_object_array(klass* klass, size_t length);

}

#endif
