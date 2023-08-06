#pragma once

#include "utility.h"
#include <string>
#include <vector>

namespace dark::IR {


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
        if(dimension == 1 && !type->is_trivial())
            warning("Stupid Mx does not support dereference for class type!");
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
    bool is_trivial()  const override { return false; }
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
        return string_join('[',std::to_string(length)," x ",type_name,']');
    }

    size_t size() const override { return length; }
    ~cstring_type() override = default;
};


/* Builtin string type. (Can build up from a cstring type.) */
struct string_type : typeinfo {
    string_type() noexcept = default;

    bool is_trivial()  const override { return false; }
    std::string name() const override { throw error("String should never be refered."); }
    size_t size()      const override { return MxPTRSIZE; }
    ~string_type() override = default;
};


/* This is a special type marker. */
struct class_type : typeinfo {
    std::string          unique_name;   /* Class name (with '%'). */
    std::vector <  wrapper  > layout;   /* Variable typeinfo. */
    std::vector <std::string> member;   /* Member variables. */

    class_type(std::string __name) noexcept : unique_name(std::move(__name)) {}

    size_t index(std::string_view __view) const {
        for(size_t i = 0 ; i != member.size() ; ++i)
            if(member[i] == __view) return i;
        throw dark::error("This should never happen! No such Member!");
    }
    size_t size() const override {
        size_t __n = 0;
        for(auto &&__p : layout) __n += __p.size();
        return __n;
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

}