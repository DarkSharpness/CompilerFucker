#pragma once

#include "utility.h"
#include <string>
#include <set>

namespace dark::IR {

struct function;

inline static constexpr size_t MxPTRSIZE = 4;

struct typeinfo {
    virtual bool is_trivial()  const = 0;
    virtual std::string name() const = 0;
    virtual size_t size()      const = 0;
    virtual ~typeinfo() = default;
};


/* Type information wrapper. */
struct wrapper {
    const typeinfo *type; /* Pointer to the real class. */
    ssize_t    dimension; /* Dimension of the pointers. */

    /* Return the type name. */
    std::string name() const noexcept {
        if(dimension) return "ptr";
        else return type->name();
    }

    size_t size() const noexcept { return dimension ? MxPTRSIZE : type->size(); }

    /**
     * @brief Return this wrapper pointed to.
     * This operator will not modify the data within!
     * 
     * @throw dark::error if the wrapper cannot dereference.
    */
    wrapper operator --(void) const {
        if(dimension <= 0) throw error("No dereference for non-pointer types!");
        // if(dimension == 1 && !type->is_trivial())
            // warning("Stupid Mx does not support dereference for class type!");
        return wrapper {type , dimension - 1};
    }

    /**
     * @brief Return this wrapper's pointer.
     * This operator will not modify the data within!
     * 
    */
    wrapper operator ++(void) const noexcept {
        return wrapper {type , dimension + 1};
    }

    wrapper operator + (ssize_t __n) const {
        if(dimension + __n < 0) throw error("No dereference for non-pointer types!");
        return wrapper {type , dimension + __n};
    }

    wrapper operator - (ssize_t __n) const {
        if(dimension - __n < 0) throw error("No dereference for non-pointer types!");
        return wrapper {type , dimension - __n};
    }

    inline friend bool operator == (const wrapper &,const wrapper &);
};


/* Builtin integer array type. */
template <size_t __n>
struct integer_type : typeinfo {
    inline static const std::string type_name = 'i' + std::to_string(__n);
    integer_type() noexcept = default;
    bool is_trivial()  const override { return true; }
    std::string name() const override { return type_name; }
    size_t size()      const override { return ((__n - 1) >> 3) + 1; }
    ~integer_type() override = default;
};

struct void_type : typeinfo {
    void_type() noexcept = default;
    bool is_trivial()  const override { return true;  }
    std::string name() const override { return "void"; }
    size_t size()      const override { throw error("Can evaluate size for void type."); }
    ~void_type() override = default;
};


/* Special pointer type. */
struct null_type : typeinfo {
    null_type() noexcept = default;
    bool is_trivial()  const override { return true;  }
    std::string name() const override { return "ptr"; }
    size_t size()      const override { return MxPTRSIZE; }
    ~null_type() override = default;
};


/* Builtin const char array type. */
struct cstring_type : integer_type <8> {
    size_t length; /* Length of the array. */

    cstring_type(size_t __len) noexcept : length(__len) {}
    
    std::string name() const override {
        return string_join('[',std::to_string(length + 1)," x ",type_name,']');
    }

    size_t size() const override { return length; }
    ~cstring_type() override = default;
};


/* Builtin string type. (Can build up from a cstring type.) */
struct string_type : typeinfo {
    string_type() noexcept = default;

    bool is_trivial()  const override { return false; }
    std::string name() const override { return "ERROR!"; }
    size_t size()      const override { return MxPTRSIZE; }
    ~string_type() override = default;
};


/* This is a special type marker. */
struct class_type : typeinfo {
    function * constructor = nullptr;   /* Constructor function. */
    std::string          unique_name;   /* Class name (with '%'). */
    std::vector <  wrapper  > layout;   /* Variable typeinfo. */
    std::vector <std::string> member;   /* Member variables. */

    class_type(std::string __name) noexcept : unique_name(std::move(__name)) {}

    size_t index(std::string_view __view) const {
        for(size_t i = 0 ; i != member.size() ; ++i)
            if(member[i] == __view) return i;
        throw dark::error("This should never happen! No such Member!");
    }

    /* Now it is 4-byte aligned. */
    size_t size() const override {
        // size_t __n = 0;
        // for(auto &&__p : layout) __n += __p.size();
        return layout.size() * MxPTRSIZE;
    }

    /* Specially arranged. */
    size_t get_bias(size_t __n) const { return __n * MxPTRSIZE; }

    std::string data() const noexcept {
        std::vector <std::string> __view;
        __view.reserve(member.size() << 1 | 1);
        __view.push_back(unique_name + " = type { ");
        for(auto __p : layout) {
            __view.push_back(__p.name());
            __view.push_back(", ");
        }
        if(layout.size()) __view.back() = " }";
        else              __view.push_back("}");
        return string_join_array(__view.begin(),__view.end());
    }

    bool is_trivial()  const override { return false; }
    std::string name() const override { return unique_name;  }
    ~class_type() override = default;
};

inline void_type __void_class__{};
inline null_type __null_class__{};

inline integer_type <1>  __boolean_class__ {};
inline integer_type <32> __integer_class__ {};

/* Library function! */
inline bool __is_null_type(const wrapper &__w) {
    return !__w.dimension && dynamic_cast <const null_type *> (__w.type);
}

/* Return whether 2 wrapper types are the same. */
inline bool operator == (const wrapper &lhs,const wrapper &rhs) {
    if(lhs.dimension == rhs.dimension) { /* Non null pointer case! */
        return lhs.type->name() == rhs.type->name();
    } else { /* At least one of them must be nullptr! */
        return __is_null_type(lhs) || __is_null_type(rhs);
    }
}


/* A definition can be variable / literal / temporary. */
struct definition {
    virtual wrapper get_value_type() const = 0;
    virtual std::string       data() const = 0;
    virtual ~definition() = default;
    wrapper get_point_type() const { return --get_value_type(); };
};

/* Non literal type. (Variable / Temporary) */
struct non_literal : definition {
    std::string name; /* Unique name.  */
    wrapper     type; /* Type wrapper. */
    wrapper get_value_type() const override final { return type; }
    std::string       data() const override final { return name; }
};

/* Literal constants. */
struct literal : definition {
    /* Special function used in global variable initialization. */
    virtual std::string type_data() const = 0;
};

/* Variables are pointers to value. */
struct variable : non_literal {
    ~variable() override = default;
};

struct function_argument : variable {
    ~function_argument() override = default;
};

/* Temporaries! */
struct temporary : non_literal {
    ~temporary() override = default;
};


struct string_constant : literal {
    std::string context;
    cstring_type   type;
    explicit string_constant(const std::string &__ctx) : context(__ctx),type({__ctx.length()}) {}
    wrapper get_value_type() const override { return wrapper {&type,0}; }
    std::string  type_data() const override {
        return string_join("private unnamed_addr constant ",type.name()); 
    }
    std::string data() const override {
        std::string __ans = "c\"";
        for(char __p : context) {
            switch(__p) {
                case '\n': __ans += "\\0A"; break;
                case '\"': __ans += "\\22"; break;
                case '\\': __ans += "\\5C"; break;
                default: __ans.push_back(__p);
            }
        }
        return __ans += "\\00\"";
    }

    std::string ASMdata() const {
        std::string __ans = "\"";
        for(char __p : context) {
            switch(__p) {
                case '\n': __ans += "\\n"; break;
                case '\"': __ans += "\\\""; break;
                case '\\': __ans += "\\\\"; break;
                default: __ans.push_back(__p);
            }
        }
        return __ans += "\"";
    }

    ~string_constant() override = default;
};


struct pointer_constant : literal {
    variable *var;
    explicit pointer_constant(variable *__ptr) : var(__ptr) {}
    std::string  type_data() const override { return "global ptr"; }
    wrapper get_value_type() const override { return var ? ++var->type : wrapper {&__null_class__,0}; }
    std::string  data()      const override { return var ? var->name : "null"; }
    ~pointer_constant() override = default;
};


struct integer_constant : literal {
    const int value;
    /* Must initialize with a value. */
    explicit integer_constant(int __v) : value(__v) {}
    std::string  type_data() const override { return "global i32"; }
    wrapper get_value_type() const override { return {&__integer_class__,0}; }
    std::string  data()      const override { return std::to_string(value); }
    ~integer_constant() override = default;
};



struct boolean_constant : literal {
    const bool value;
    explicit boolean_constant(bool __v) : value(__v) {}
    std::string  type_data() const override { return "global i1"; }
    wrapper get_value_type() const override { return {&__boolean_class__,0}; }
    std::string  data()      const override { return value ? "true" : "false"; }
    ~boolean_constant() override = default;
};

inline bool operator < (const integer_constant &lhs,
                        const integer_constant &rhs) {
    return lhs.value < rhs.value;
}


inline integer_constant *create_integer(int __n) {
    static std::set <integer_constant> __pool;
    return const_cast <integer_constant *> (&*__pool.emplace(__n).first);
}

inline boolean_constant *create_boolean(bool __n) {
    static boolean_constant __pool[2] = {boolean_constant {false},boolean_constant {true}};
    return __pool + __n;
}


}