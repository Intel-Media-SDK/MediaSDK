// Copyright (c) 2017-2018 Intel Corporation
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#if !defined(__CM_VM_H__) && !defined(CM_VM_H)
#define __CM_VM_H__
#define CM_VM_H

#include <assert.h>
#include <limits>
#include <limits.h>

#if defined(__clang__)
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wnon-virtual-dtor"
#elif defined(__GNUC__)
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#endif

typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;

template <typename T, uint R, uint C> class matrix;
template <typename T, uint R, uint C> class matrix_ref;
template <typename T, uint SZ> class vector;
template <typename T, uint SZ> class vector_ref;

template <typename T> struct cmtype;
template <> struct cmtype<char> {
    static const bool value = true;
};

template <> struct cmtype<signed char> {
    static const bool value = true;
};

template <> struct cmtype<unsigned char> {
    static const bool value = true;
};

template <> struct cmtype<short> {
    static const bool value = true;
};

template <> struct cmtype<unsigned short> {
    static const bool value = true;
};
template <> struct cmtype<int> {
    static const bool value = true;
};

template <> struct cmtype<unsigned int> {
    static const bool value = true;
};

template <> struct cmtype<unsigned long> {
    static const bool value = true;
};

template <> struct cmtype<float> {
    static const bool value = true;
};

template <> struct cmtype<double> {
    static const bool value = true;
};

template <> struct cmtype<long long> {
    static const bool value = true;
};

template <> struct cmtype<unsigned long long> {
    static const bool value = true;
};

template <> struct cmtype<SurfaceIndex> {
    static const bool value = true;
};

#define SIMDCF_WRAPPER(X, SZ, i) X
#define SIMDCF_ELEMENT_SKIP(i)
#define OFFSET uint

namespace cm {
    template<typename ty>
    struct pointer_traits
    {
        enum { dim = 0 };
        typedef ty tail_pointee_type;
    };

    template<typename ty, int n>
    struct pointer_traits<ty[n]>
    {
        enum { dim = pointer_traits<ty>::dim + 1 };
        typedef typename pointer_traits<ty>::tail_pointee_type tail_pointee_type;
    };

    template<typename ty>
    struct pointer_traits<ty *>
    {
        enum { dim = pointer_traits<ty>::dim + 1 };
        typedef typename pointer_traits<ty>::tail_pointee_type tail_pointee_type;
    };

#ifdef CM_EMU
    template <typename dataTy, unsigned int size>
    class array_1d
    {
    public:
        typedef dataTy(*datasTy)[size];
    public:
        array_1d() : datas_(NULL), size_(size)
        {
        }
        array_1d(void * datas) : datas_((datasTy)datas), size_(size)
        {
        }
    public:
        datasTy      datas_;
        unsigned int size_;
    };

    template <typename dataTy, unsigned int rows, unsigned int cols>
    class array_2d
    {
    public:
        typedef dataTy(*datasTy)[cols];
    public:
        array_2d(void * datas)
            : rows_(rows), cols_(cols)
        {
            for (unsigned int row = 0; row < rows_; ++row) {
                array_1ds_[row].datas_ = (datasTy)datas + row;
            }
        }
    public:
        array_1d<dataTy, cols> array_1ds_[rows];
        unsigned int           rows_;
        unsigned int           cols_;
    };
#endif
};

template <typename T> struct is_inttype {
    static const bool value = false;
};
template <> struct is_inttype<char> {
    static const bool value = true;
};
template <> struct is_inttype<unsigned char> {
    static const bool value = true;
};
template <> struct is_inttype<short> {
    static const bool value = true;
};
template <> struct is_inttype<unsigned short> {
    static const bool value = true;
};
template <> struct is_inttype<int> {
    static const bool value = true;
};
template <> struct is_inttype<unsigned int> {
    static const bool value = true;
};
template <> struct is_inttype<long long> {
    static const bool value = true;
};
template <> struct is_inttype<unsigned long long> {
    static const bool value = true;
};


#define SAT 1

namespace CmEmulSys
{
    template<typename RT>
    struct satur {
        template<typename T> static RT
            saturate(const T val, const int flags) {
            if ((flags & SAT) == 0) {
                return (RT)val;
            }

#ifdef max
#undef max
#undef min
#endif
            const RT t_max = std::numeric_limits<RT>::max();
            const RT t_min = std::numeric_limits<RT>::min();


            if (val > t_max) {
                return t_max;
            }
            else if ((val >= 0) && (t_min < 0)) {
                // RT is "signed" if t_min < 0
                // when comparing a signed and a unsigned variable, the signed one cast to unsigned first.
                return (RT)val;
            }
            else if (val < t_min) {
                return t_min;
            }
            else {
                return (RT)val;
            }
        }
    };

    template<>
    struct satur<float> {
        template<typename T> static float
            saturate(const T val, const int flags) {
            if ((flags & SAT) == 0) {
                return (float)val;
            }

            if (val < 0.) {
                return 0;
            }
            else if (val > 1.) {
                return 1.;
            }
            else {
                return (float)val;
            }
        }
    };

    template<>
    struct satur<double> {
        template<typename T> static double
            saturate(const T val, const int flags) {
            if ((flags & SAT) == 0) {
                return (double)val;
            }

            if (val < 0.) {
                return 0;
            }
            else if (val > 1.) {
                return 1.;
            }
            else {
                return (double)val;
            }
        }
    };

    template<typename T1, bool B> struct _SetSatur {
        static uint SetSatur() {
            return 0;
        }
    };

    template <> struct _SetSatur<float, true> {
        static uint SetSatur() {
            return SAT;
        }
    };

    template <> struct _SetSatur<double, true> {
        static uint SetSatur() {
            return SAT;
        }
    };

} /* ::CmEmulSys */

/* Basic stream. Non template class. */
class basic_stream {
public:
    virtual int extract_data(void *buf, uint size = 0xffffffff) = 0;
    virtual void* get_addr(uint i) = 0;
    virtual void* get_addr_data() = 0;
    virtual void* get_addr_obj() = 0;
    virtual uint get_size_of_element() const = 0;
    virtual uint get_number_of_elements() const = 0;
    virtual uint get_size_data() const = 0;
    virtual uint get_size_object() const = 0;
};

// stream
template <typename T, uint SZ>
class stream : public basic_stream {
    static_assert(cmtype<T>::value, "invalid type");
public:
    typedef  T _Type;

    CM_INLINE int n_elems() const { return SZ; }

    virtual T get(uint i) const = 0;
    virtual T& getref(uint i) = 0;
    virtual void* get_addr(uint i) = 0;
    virtual void* get_addr_data() = 0;
    virtual void* get_addr_obj() = 0;
    int extract_data(void *buf, uint size = 0xffffffff);
    virtual uint get_size_of_element() const { return sizeof(T); };
    virtual uint get_number_of_elements() const { return SZ; };
    virtual uint get_size_data() const = 0;
    virtual uint get_size_object() const = 0;

    /* merge */
    CM_NOINLINE void merge(const T x, const uint c);
    template <typename T1>  void CM_NOINLINE merge(const stream<T1, SZ> &x, const uint c);
    template <typename T1, typename T2> CM_NOINLINE void merge(const stream<T1, SZ> &x, const stream<T2, SZ> &c);
    template <typename T1> CM_NOINLINE void merge(const T x, const stream<T1, SZ> &c);
    template <typename T1, typename T2> CM_NOINLINE void merge(const stream<T1, SZ>& x, const stream<T2, SZ>& y, const uint c);
    template <typename T1> CM_NOINLINE void merge(const T x, const stream<T1, SZ>& y, const uint c);
    template <typename T1> CM_NOINLINE  void merge(const stream<T1, SZ>& x, const T y, const uint c);
    CM_NOINLINE void merge(const T x, const T y, const uint c);
    template <typename T1, typename T2, typename T3> CM_NOINLINE void merge(const stream<T1, SZ>& x, const stream<T2, SZ>& y, const stream<T3, SZ>& c);
    template <typename T1, typename T2>  CM_NOINLINE void merge(const T x, const stream<T1, SZ>& y, const stream<T2, SZ>& c);
    template <typename T1, typename T2> CM_NOINLINE void merge(const stream<T1, SZ>& x, const T y, const stream<T2, SZ>& c);
    template <typename T1> CM_NOINLINE void merge(const T x, const T y, const stream<T1, SZ>& c);

    // any,all
    CM_NOINLINE ushort any(void) const;
    CM_NOINLINE ushort all(void) const;
};


// matrix
template <typename T, uint R, uint C>
class matrix : public stream<T, R*C> {
    template <typename T1, uint R1, uint C1> friend class matrix_ref;
    template <typename T1, uint SZ1> friend class vector_ref;
public:
    template <typename T1, uint R1, uint C1> friend class matrix;
    template <typename T1, uint SZ1> friend class vector;

    enum { SZ = R*C };
    enum { ROWS = R, COLS = C, ELEMS = R*C };

    CM_INLINE int n_rows() const { return R; }
    CM_INLINE int n_cols() const { return C; }

    template <uint REP> CM_INLINE
        const vector<T, R*C*REP> replicate(OFFSET ioff = 0, OFFSET joff = 0)
    {
        return genx_select<REP, 0, R*C, 1>(ioff, joff);
    };
    template <uint REP, uint W> CM_INLINE
        const vector<T, W*REP> replicate(OFFSET ioff = 0, OFFSET joff = 0)
    {
        return genx_select<REP, 0, W, 1>(ioff, joff);
    };
    template <uint REP, uint VS, uint W> CM_INLINE
        const vector<T, W*REP> replicate(OFFSET ioff = 0, OFFSET joff = 0)
    {
        return genx_select<REP, VS, W, 1>(ioff, joff);
    };
    template <uint REP, uint VS, uint W, uint HS> CM_INLINE
        const vector<T, W*REP> replicate(OFFSET ioff = 0, OFFSET joff = 0)
    {
        return genx_select<REP, VS, W, HS>(ioff, joff);
    };

    virtual T get(uint i) const { return data[i]; }
    virtual T& getref(uint i) { return data[i]; }
    virtual void* get_addr(uint i) { return &data[i]; }
    virtual void* get_addr_data() {
#ifdef CMRT_EMU
        return this;
#elif CMRT_SIM
        return &data[0];
#else
        return &data[0];
#endif
    }
    virtual void* get_addr_obj() { return this; }
    virtual uint get_size_data() const {
#ifdef CMRT_EMU
        return sizeof(*this);
#elif CMRT_SIM
        return sizeof(data);
#else
        return sizeof(data);
#endif
    }
    virtual uint get_size_object() const { return sizeof(*this); }

    CM_NOINLINE T operator () (OFFSET i, OFFSET j) const {
        assert(i < R && j < C);
        return get(i*C + j);
    }

    CM_NOINLINE T& operator () (OFFSET i, OFFSET j) {
        assert(i < R && j < C);
        return data[i*C + j];
    }

    template <typename T1, uint R1, uint C1>
    class inner_hack {
        matrix* m_t;
        OFFSET _i;
    public:
        inner_hack(matrix* m, OFFSET i) :m_t(m) { _i = i; }
        CM_NOINLINE const T1 operator[](OFFSET j) const { return (*m_t)(_i, j); }
        CM_NOINLINE T1& operator[](OFFSET j) { return (*m_t)(_i, j); }

    };

    CM_NOINLINE inner_hack<T, R, C> operator [] (OFFSET i) const {
        return inner_hack<T, R, C>(this, i);
    }

    CM_NOINLINE inner_hack<T, R, C> operator [] (OFFSET i) {
        return inner_hack<T, R, C>(this, i);
    }

    CM_NOINLINE matrix();
    template <typename T2> CM_NOINLINE matrix(const T2 initArray[]);
    CM_NOINLINE matrix(const matrix<T, R, C>& src);
    template <typename T2> CM_NOINLINE matrix(const T2& src);
    template <typename T2, uint R2, uint C2> CM_NOINLINE matrix(const matrix<T2, R2, C2>& src, const uint sat = 0);
    template <typename T2, uint R2, uint C2> CM_NOINLINE matrix(const matrix_ref<T2, R2, C2>& src, const uint sat = 0);
    template <typename T2> CM_NOINLINE matrix(const vector<T2, R*C>& src)
#ifdef CM_EMU
        : array_2d_(data)
#endif
    {
        new (this) matrix<T, R, C>((matrix<T2, 1, R*C>&)src);
    }

    template <typename T2> CM_NOINLINE matrix(const vector_ref<T2, R*C>& src)
#ifdef CM_EMU
        : array_2d_(data)
#endif
    {
        new (this) matrix<T, R, C>((matrix_ref<T2, 1, R*C>&)src);
    }

    //operator =
    CM_NOINLINE matrix<T, R, C>& operator = (const matrix<T, R, C>& src);
    template <typename T2> CM_NOINLINE matrix<T, R, C>& operator = (const T2 src);
    template <typename T2, uint R2, uint C2> CM_NOINLINE matrix<T, R, C>& operator = (const matrix<T2, R2, C2>& src);
    template <typename T2, uint R2, uint C2> CM_NOINLINE matrix<T, R, C>& operator = (const matrix_ref<T2, R2, C2>& src);
    template <typename T2> CM_NOINLINE matrix<T, R, C>& operator = (const vector<T2, R*C>& src) { return this->operator=((const matrix<T2, 1, R*C>&)src); };
    template <typename T2> CM_NOINLINE matrix<T, R, C>& operator = (const vector_ref<T2, R*C>& src) { return this->operator=((const matrix_ref<T2, 1, R*C>&)src); };

    //selects
    template <typename T2> CM_NOINLINE vector_ref<T2, R*C * sizeof(T) / sizeof(T2)> format();
    template <typename T2, uint R2, uint C2> CM_NOINLINE matrix_ref<T2, R2, C2> format();
    template <typename T2> CM_NOINLINE const vector_ref<T2, R*C * sizeof(T) / sizeof(T2)> format() const;
    template <typename T2, uint R2, uint C2> CM_NOINLINE const matrix_ref<T2, R2, C2> format() const;
    vector_ref<T, C> CM_NOINLINE row(OFFSET i);
    matrix_ref<T, R, 1> CM_NOINLINE column(OFFSET i);
    template <uint R2, uint RS, uint C2, uint CS> CM_NOINLINE matrix_ref<T, R2, C2> select(OFFSET ioff = 0, OFFSET joff = 0);
    template <uint R2, uint RS, uint C2, uint CS> CM_NOINLINE const matrix_ref<T, R2, C2> select(OFFSET ioff = 0, OFFSET joff = 0) const;

    //1D iselect
    template <typename T2, uint WD> CM_NOINLINE vector<T, WD> iselect(const vector<T2, WD>& index);
    template <typename T2, uint WD> CM_NOINLINE vector<T, WD> iselect(const vector_ref<T2, WD>& index);

    //2D iselect
    template <typename T2, uint WD> CM_NOINLINE vector<T, WD> iselect(const vector<T2, WD>& index_x, const vector<T2, WD>& index_y);

    template <typename T2, uint WD> CM_NOINLINE vector<T, WD> iselect(const vector<T2, WD>& index_x, const vector_ref<T2, WD>& index_y);

    template <typename T2, uint WD> CM_NOINLINE vector<T, WD> iselect(const vector_ref<T2, WD>& index_x, const vector<T2, WD>& index_y);

    template <typename T2, uint WD> CM_NOINLINE vector<T, WD> iselect(const vector_ref<T2, WD>& index_x, const vector_ref<T2, WD>& index_y);
    //end of iselect

    template <uint R2, uint VS, uint WD, uint HS> CM_NOINLINE const vector<T, R2*WD> genx_select(OFFSET ioff = 0, OFFSET joff = 0);

    matrix_ref<T, R, C> CM_NOINLINE select_all();
    const matrix_ref<T, R, C> CM_NOINLINE select_all() const;

    // operators +=, -=, ...
#define declare_operation(OP) \
        template <typename T2> CM_NOINLINE matrix<T,R,C>& operator OP (const T2 x);\
        template <typename T2, uint R2, uint C2> CM_NOINLINE matrix<T,R,C>& operator OP (const matrix<T2,R2,C2>& x);\
        template <typename T2, uint R2, uint C2> CM_NOINLINE matrix<T,R,C>& operator OP (const matrix_ref<T2,R2,C2>& x);\
        template <typename T2> CM_NOINLINE matrix<T,R,C>& operator OP (const vector<T2,SZ>& x);\
        template <typename T2> CM_NOINLINE matrix<T,R,C>& operator OP (const vector_ref<T2,SZ>& x);\

    declare_operation(+= )     // +=
        declare_operation(-= )     // -=
        declare_operation(*= )     // *=
        declare_operation(/= )     // /=
        declare_operation(%= )     // %=
        declare_operation(&= )     // &=
        declare_operation(|= )     // |=
        declare_operation(^= )     // ^=
        declare_operation(>>= )     // >>=
        declare_operation(<<= )     // <<=
#undef declare_operation

                                    // for debug
        uint id() const { return number; }

#ifdef CM_DEBUG
    virtual std::string type_name() const { std::stringstream ss; ss << "M<" << typeid(T).name() << "," << R << "," << C << ">"; return ss.str(); }
    virtual std::string obj_name() const { std::stringstream ss; ss << typeid(T).name() << "[" << /*id()*/SZ << "]"; return ss.str(); }
#endif /* CM_DEBUG */

private:

    T data[SZ];
#ifdef CM_EMU
    cm::array_2d<T, R, C> array_2d_;
#endif
    CM_NOINLINE T operator () (uint i) const {
        assert(i < SZ);
        return get(i);
    }

    CM_NOINLINE T& operator () (uint i) {
        assert(i < SZ);
        return data[i];
    }
    uint number;
}; //matrix

   // vector
template <typename T, uint SZ>
class vector : public matrix<T, 1, SZ> {
    void assign(const stream<T, SZ> &src);
    template <typename T1, uint R1, uint C1> friend class matrix_ref;
    template <typename T1, uint SZ1> friend class vector_ref;
public:
    template <typename T1, uint R1, uint C1> friend class matrix;
    template <typename T1, uint SZ1> friend class stream;

    template <uint REP> CM_INLINE
        vector<T, SZ*REP> replicate(OFFSET joff = 0)
    {
        return ((matrix<T, 1, SZ> *)this)->template replicate<REP>(0, joff);
    };
    template <uint REP, uint W> CM_INLINE
        vector<T, W*REP> replicate(OFFSET joff = 0)
    {
        return ((matrix<T, 1, SZ> *)this)->template replicate<REP, W>(0, joff);
    };
    template <uint REP, uint VS, uint W> CM_INLINE
        vector<T, W*REP> replicate(OFFSET joff = 0)
    {
        return ((matrix<T, 1, SZ> *)this)->template replicate<REP, VS, W>(0, joff);
    };
    template <uint REP, uint VS, uint W, uint HS> CM_INLINE
        vector<T, W*REP> replicate(OFFSET joff = 0)
    {
        return ((matrix<T, 1, SZ> *)this)->template replicate<REP, VS, W, HS>(0, joff);
    };

    CM_NOINLINE vector() : ::matrix<T, 1, SZ>() {}
    template <typename T2> CM_NOINLINE vector(const T2 initArray[]) : ::matrix<T, 1, SZ>(initArray) {
        static_assert(!std::is_floating_point<T2>::value, "floating point array initialization values are not supported");
    }
    CM_NOINLINE vector(const vector<T, SZ>& src) : ::matrix<T, 1, SZ>((const matrix<T, 1, SZ>&)src) {}
    template <typename T2> CM_NOINLINE vector(const T2& src) : ::matrix<T, 1, SZ>(src) {}
    template <typename T2, uint R2, uint C2> CM_NOINLINE vector(const matrix<T2, R2, C2>& src, uint sat = 0) : ::matrix<T, 1, SZ>(src, sat) {}
    template <typename T2, uint R2, uint C2> CM_NOINLINE vector(const matrix_ref<T2, R2, C2>& src, uint sat = 0) : ::matrix<T, 1, SZ>(src, sat) {}
    template <typename T2> CM_NOINLINE vector(const vector<T2, SZ>& src) : ::matrix<T, 1, SZ>(src) {}
    template <typename T2> CM_NOINLINE vector(const vector_ref<T2, SZ>& src) : ::matrix<T, 1, SZ>(src) {}


    CM_NOINLINE T operator () (OFFSET i) const {
        assert(i < SZ);
        return matrix<T, 1, SZ>::get(i);
    }

    CM_NOINLINE T& operator () (OFFSET i) {
        assert(i < SZ);
        return matrix<T, 1, SZ>::data[i];
    }

    CM_NOINLINE T operator [] (OFFSET i) const {
        assert(i < SZ);
        return matrix<T, 1, SZ>::get(i);
    }

    CM_NOINLINE T& operator [] (OFFSET i) {
        assert(i < SZ);
        return matrix<T, 1, SZ>::data[i];
    }

    template <typename T2, uint WD> CM_NOINLINE vector<T, WD> operator () (const vector<T2, WD>& index) {
        return matrix<T, 1, SZ>::template iselect<T2, WD>(index);
    }

    template <typename T2, uint WD> CM_NOINLINE vector<T, WD> operator () (const vector_ref<T2, WD>& index) {
        return matrix<T, 1, SZ>::template iselect<T2, WD>(index);
    }

    template <typename T2, uint WD> CM_NOINLINE vector<T, WD> operator [] (const vector<T2, WD>& index) {
        return matrix<T, 1, SZ>::template iselect<T2, WD>(index);
    }

    template <typename T2, uint WD> CM_NOINLINE vector<T, WD> operator [] (const vector_ref<T2, WD>& index) {
        return matrix<T, 1, SZ>::template iselect<T2, WD>(index);
    }

    //1D iselect only
    template <typename T2, uint WD> CM_NOINLINE vector<T, WD> iselect(const vector<T2, WD>& index) {
        static_assert((std::is_unsigned<T2>::value), "iselect index vector element type must be unsigned");
        return matrix<T, 1, SZ>::template iselect<T2, WD>(index);
    }

    template <typename T2, uint WD> CM_NOINLINE vector<T, WD> iselect(const vector_ref<T2, WD>& index) {
        static_assert((std::is_unsigned<T2>::value), "iselect index vector element type must be unsigned");
        return matrix<T, 1, SZ>::template iselect<T2, WD>(index);
    }
    //end of iselect

    //operator =: call base versions of operator =
    CM_NOINLINE vector<T, SZ>& operator = (const vector<T, SZ>& src) { ((matrix<T, 1, SZ>*)this)->operator=(src); return *this; } // assignment operator
    template <typename T2> CM_NOINLINE vector<T, SZ>& operator = (const T2 src) {
        ((matrix<T, 1, SZ>*)this)->operator=(src); return *this;
    }
    template <typename T2, uint R2, uint C2> CM_NOINLINE vector<T, SZ>& operator = (const matrix<T2, R2, C2>& src) { ((matrix<T, 1, SZ>*)this)->operator=(src); return *this; }
    template <typename T2, uint R2, uint C2> CM_NOINLINE vector<T, SZ>& operator = (const matrix_ref<T2, R2, C2>& src) { ((matrix<T, 1, SZ>*)this)->operator=(src); return *this; }
    template <typename T2> CM_NOINLINE vector<T, SZ>& operator = (const vector<T2, SZ>& src) { ((matrix<T, 1, SZ>*)this)->operator=(src); return *this; }
    template <typename T2> CM_NOINLINE vector<T, SZ>& operator = (const vector_ref<T2, SZ>& src) { ((matrix<T, 1, SZ>*)this)->operator=(src); return *this; }

    //vector select
    template <uint C, uint CS> CM_NOINLINE const vector_ref<T, C> select(OFFSET joff = 0) const {
        static_assert(((SZ) >= (C)), "select size is greater than source vector size");
        static_assert(((SZ) >= ((C - 1)*(CS)) + 1) || (CS == 1), "select range exceeds source vector bounds");
        return ((matrix<T, 1, SZ>*)this)->template select<1, 1, C, CS>(0, joff);
    }
    template <uint C, uint CS> CM_NOINLINE vector_ref<T, C> select(OFFSET joff = 0) {
        static_assert(((SZ) >= (C)), "select size is greater than source vector size");
        static_assert(((SZ) >= ((C - 1)*(CS)) + 1) || (CS == 1), "select range exceeds source vector bounds");
        return ((matrix<T, 1, SZ>*)this)->template select<1, 1, C, CS>(0, joff);
    }

    //vector genx_select
    template <uint R, uint VS, uint WD, uint HS> CM_NOINLINE const vector<T, R*WD> genx_select(OFFSET joff = 0) {
        static_assert((!std::is_same<T, double>::value), "genx_select is not supported for vectors with element type 'double'");
        static_assert(((SZ) >= (WD)), "genx_select width is greater than source vector size");
        return ((matrix<T, 1, SZ>*)this)->template genx_select<R, VS, WD, HS>(0, joff);
    }
}; //vector


/*
    STREAM
*/

template <typename T, uint SZ>
int stream<T, SZ>::extract_data(void *buf, uint size)
{
    uint i;

    assert(SZ * sizeof(T) <= size);

    (void)size;

    for (i = 0; i< SZ; i++) {
        ((T*)buf)[i] = get(i);
    }

    return SZ * sizeof(T);
}

/*
*  merge
*/
template <typename T, uint SZ>
void stream<T, SZ>::merge(const T x, const uint c)
{
    T* p;
    for (uint i = 0; i<SZ; i++) {
        SIMDCF_ELEMENT_SKIP(i);
        if (((c >> i) & 1) != 0) {
            p = (T*)this->get_addr(i);
            *p = x;
        }
    }
}

template <typename T, uint SZ>
template <typename T1>
void stream<T, SZ>::merge(const stream<T1, SZ> &x, const uint c)
{
    vector<T1, SZ> in_x; in_x.assign(x);
    T* p;
    for (uint i = 0; i<SZ; i++) {
        SIMDCF_ELEMENT_SKIP(i);
        if (((c >> i) & 1) != 0) {
            p = (T*)this->get_addr(i);
            *p = in_x.get(i);
        }
    }
}


template <typename T, uint SZ>
template <typename T1, typename T2>
void stream<T, SZ>::merge(const stream<T1, SZ> &x, const stream<T2, SZ> &c)
{
    vector<T1, SZ> in_x; in_x.assign(x);
    vector<T2, SZ> in_c; in_c.assign(c);
    T* p;
    for (uint i = 0; i<SZ; i++) {
        SIMDCF_ELEMENT_SKIP(i);
        if ((in_c.get(i) & 1) != 0) {
            p = (T*)this->get_addr(i);
            *p = (T)in_x.get(i);
        }
    }
}

template <typename T, uint SZ>
template <typename T1>
void stream<T, SZ>::merge(const T x, const stream<T1, SZ> &c)
{
    vector<T1, SZ> in_c; in_c.assign(c);
    T* p;
    for (uint i = 0; i<SZ; i++) {
        SIMDCF_ELEMENT_SKIP(i);
        if ((in_c.get(i) & 1) != 0) {
            p = (T*)this->get_addr(i);
            *p = x;
        }
    }
}

template <typename T, uint SZ>
template <typename T1, typename T2>
void stream<T, SZ>::merge(const stream<T1, SZ>& x, const stream<T2, SZ>& y,
    const uint c)
{
    vector<T1, SZ> in_x; in_x.assign(x);
    vector<T2, SZ> in_y; in_y.assign(y);
    T* p;
    for (uint i = 0; i<SZ; i++) {
        SIMDCF_ELEMENT_SKIP(i);
        p = (T*)this->get_addr(i);
        if (((c >> i) & 1) != 0) {
            *p = in_x.get(i);
        }
        else {
            *p = in_y.get(i);
        }
    }
}

template <typename T, uint SZ>
template <typename T1>
void stream<T, SZ>::merge(const T x, const stream<T1, SZ>& y, const uint c)
{
    vector<T1, SZ> in_y; in_y.assign(y);
    T* p;
    for (uint i = 0; i<SZ; i++) {
        SIMDCF_ELEMENT_SKIP(i);
        p = (T*)this->get_addr(i);
        if (((c >> i) & 1) != 0) {
            *p = x;
        }
        else {
            *p = in_y.get(i);
        }
    }
}

template <typename T, uint SZ>
template <typename T1>
void stream<T, SZ>::merge(const stream<T1, SZ>& x, const T y, const uint c)
{
    vector<T1, SZ> in_x; in_x.assign(x);
    T* p;
    for (uint i = 0; i<SZ; i++) {
        SIMDCF_ELEMENT_SKIP(i);
        p = (T*)this->get_addr(i);
        if (((c >> i) & 1) != 0) {
            *p = in_x.get(i);
        }
        else {
            *p = y;
        }
    }
}

template <typename T, uint SZ>
void stream<T, SZ>::merge(const T x, const T y, const uint c)
{
    T* p;
    for (uint i = 0; i<SZ; i++) {
        SIMDCF_ELEMENT_SKIP(i);
        p = (T*)this->get_addr(i);
        if (((c >> i) & 1) != 0) {
            *p = x;
        }
        else {
            *p = y;
        }
    }
}

template <typename T, uint SZ>
template <typename T1, typename T2, typename T3>
void stream<T, SZ>::merge(const stream<T1, SZ>& x, const stream<T2, SZ>& y, const stream<T3, SZ>& c)
{
    vector<T1, SZ> in_x; in_x.assign(x);
    vector<T2, SZ> in_y; in_y.assign(y);
    vector<T3, SZ> in_c; in_c.assign(c);

    T* p;
    for (uint i = 0; i<SZ; i++) {
        SIMDCF_ELEMENT_SKIP(i);
        p = (T*)this->get_addr(i);
        if ((in_c.get(i) & 1) != 0) {
            *p = in_x.get(i);
        }
        else
        {
            *p = in_y.get(i);
        }
    }
}

template <typename T, uint SZ>
template <typename T1, typename T2>
void stream<T, SZ>::merge(const T x, const stream<T1, SZ>& y, const stream<T2, SZ>& c)
{
    vector<T1, SZ> in_y; in_y.assign(y);
    vector<T2, SZ> in_c; in_c.assign(c);
    T* p;
    for (uint i = 0; i<SZ; i++) {
        SIMDCF_ELEMENT_SKIP(i);
        p = (T*)this->get_addr(i);
        if ((in_c.get(i) & 1) != 0) {
            *p = x;
        }
        else
        {
            *p = in_y.get(i);
        }
    }
}

template <typename T, uint SZ>
template <typename T1, typename T2>
void stream<T, SZ>::merge(const stream<T1, SZ>& x, const T y,
    const stream<T2, SZ>& c)
{
    vector<T1, SZ> in_x; in_x.assign(x);
    vector<T2, SZ> in_c; in_c.assign(c);
    T* p;
    for (uint i = 0; i<SZ; i++) {
        SIMDCF_ELEMENT_SKIP(i);
        p = (T*)this->get_addr(i);
        if ((in_c.get(i) & 1) != 0) {
            *p = in_x.get(i);
        }
        else
        {
            *p = y;
        }
    }
}

template <typename T, uint SZ>
template <typename T1>
void stream<T, SZ>::merge(const T x, const T y, const stream<T1, SZ>& c)
{
    vector<T1, SZ> in_c; in_c.assign(c);
    T* p;
    for (uint i = 0; i<SZ; i++) {
        SIMDCF_ELEMENT_SKIP(i);
        p = (T*)this->get_addr(i);
        if ((in_c.get(i) & 1) != 0) {
            *p = x;
        }
        else
        {
            *p = y;
        }
    }
}


/*
    MATRIX
*/

template <typename T, uint R, uint C>
template <typename T2>
matrix<T, R, C>::matrix(const T2 initArray[])
#ifdef CM_EMU
    : array_2d_(data)
#endif
{
    static_assert(!std::is_floating_point<T2>::value, "floating point array initialization values are not supported");
    typedef typename cm::pointer_traits<T2>::tail_pointee_type tail_pointee_type;
    for (int i = 0; i < SZ; i++) {
        SIMDCF_WRAPPER(data[i] = (T)((tail_pointee_type *)initArray)[i], SZ, i);
    }
}

template <typename T, uint R, uint C>
matrix<T, R, C>::matrix() :
#ifdef CM_EMU
    array_2d_(data),
#endif
    number(0)
{

}

//copy constructor
template <typename T, uint R, uint C>
matrix<T, R, C>::matrix(const matrix<T, R, C>& src)
#ifdef CM_EMU
    : array_2d_(data)
#endif
{
    (*this) = src;
}
template <typename T, uint R, uint C>
template <typename T2>
matrix<T, R, C>::matrix(const T2& src)
#ifdef CM_EMU
    : array_2d_(data)
#endif
{
    (*this) = src;
}
template <typename T, uint R, uint C>
template <typename T2, uint R2, uint C2>
matrix<T, R, C>::matrix(const matrix<T2, R2, C2>& src, const uint sat)
#ifdef CM_EMU
    : array_2d_(data)
#endif
{
    static_assert(R*C == R2*C2, "matrices have different dimensions");

    uint sat1 = 0;
    vector<T2, SZ> in_src; in_src.assign(src);

    for (uint i = 0; i < SZ; i++) {
        SIMDCF_WRAPPER((*this)(i) = CmEmulSys::satur<T>::saturate(in_src(i), sat | sat1), SZ, i);
    }
}
template <typename T, uint R, uint C>
template <typename T2, uint R2, uint C2>
matrix<T, R, C>::matrix(const matrix_ref<T2, R2, C2>& src, const uint sat)
#ifdef CM_EMU
    : array_2d_(data)
#endif
{
    static_assert(R*C == R2*C2, "matrices have different dimensions");

    vector<T2, SZ> in_src; in_src.assign(src);
    for (uint i = 0; i < SZ; i++) {
        SIMDCF_WRAPPER((*this)(i) = CmEmulSys::satur<T>::saturate(in_src(i), sat), SZ, i);
    }
}
//
// matrix operator =
//
template <typename T, uint R, uint C>
matrix<T, R, C>& matrix<T, R, C>::operator = (const matrix<T, R, C>& src)
{
    vector<T, SZ> in_src; in_src.assign(src);
    for (uint i = 0; i<SZ; ++i) {
        SIMDCF_WRAPPER(this->getref(i) = in_src(i), SZ, i);
    }
    return *this;
}

template <typename T, uint R, uint C>
template <typename T2>
matrix<T, R, C>& matrix<T, R, C>::operator = (const T2 src)
{
    uint sat1 = 0;

    for (uint i = 0; i < SZ; i++) {
        SIMDCF_WRAPPER(this->getref(i) = CmEmulSys::satur<T>::saturate(src, sat1), SZ, i);
    }

    return *this;
}
template <typename T, uint R, uint C>
template <typename T2, uint R2, uint C2>
matrix<T, R, C>& matrix<T, R, C>::operator = (const matrix<T2, R2, C2>& src)
{
    static_assert(R*C == R2*C2, "matrices have different dimensions"); \

    uint sat1 = 0;
    vector<T2, SZ> in_src; in_src.assign(src);

    for (uint i = 0; i < SZ; i++) {
        SIMDCF_WRAPPER(this->getref(i) = CmEmulSys::satur<T>::saturate(in_src(i), sat1), SZ, i);
    }

    return *this;
}
template <typename T, uint R, uint C>
template <typename T2, uint R2, uint C2>
matrix<T, R, C>& matrix<T, R, C>::operator = (const matrix_ref<T2, R2, C2>& src)
{
    static_assert(R*C == R2*C2, "matrices have different dimensions"); \

    uint sat1 = 0;
    vector<T2, SZ> in_src; in_src.assign(src);

    for (uint i = 0; i < SZ; i++) {
        SIMDCF_WRAPPER(this->getref(i) = CmEmulSys::satur<T>::saturate(in_src(i), sat1), SZ, i);
    }

    return *this;
}


#define matrix_operation(OP) \
\
template <typename T, uint R, uint C> \
template <typename T2> \
matrix<T,R,C>& matrix<T,R,C>::operator OP##= (const T2 x) \
{ \
        static_assert(cmtype<T2>::value, "invalid type");\
        uint sat1 = 0; \
        for (uint i=0; i < SZ; i++) { \
            SIMDCF_WRAPPER(this->getref(i) = CmEmulSys::satur<T>::saturate((*this).get(i) OP x, sat1), SZ, i); \
        } \
        return *this; \
} \
template <typename T, uint R, uint C> \
template <typename T2, uint R2, uint C2> \
matrix<T,R,C>& matrix<T,R,C>::operator OP##= (const matrix<T2,R2,C2>& x) \
{ \
        static_assert(R*C == R2*C2, "matrices have different dimensions"); \
        uint sat1 = 0; \
        vector<T2, /*SZ*/R*C> in_x; in_x.assign(x); \
        for (uint i=0; i < SZ; i++) { \
            SIMDCF_WRAPPER(this->getref(i) = CmEmulSys::satur<T>::saturate((*this).get(i) OP in_x(i), sat1), SZ, i);\
        } \
        return *this; \
} \
template <typename T, uint R, uint C> \
template <typename T2, uint R2, uint C2> \
matrix<T,R,C>& matrix<T,R,C>::operator OP##= (const matrix_ref<T2,R2,C2>& x) \
{ \
        static_assert(R*C == R2*C2, "matrices have different dimensions"); \
        uint sat1 = 0; \
        vector<T2, /*SZ*/R*C> in_x; in_x.assign(x); \
        for (uint i=0; i < SZ; i++) { \
            SIMDCF_WRAPPER(this->getref(i) = CmEmulSys::satur<T>::saturate((*this).get(i) OP in_x(i), sat1), SZ, i);\
        } \
        return *this; \
} \
template <typename T, uint R, uint C> \
template <typename T2> \
matrix<T,R,C>& matrix<T,R,C>::operator OP##= (const vector<T2,SZ>& x) \
{ \
        uint sat1 = 0; \
        vector<T2, SZ> in_x; in_x.assign(x); \
        for (uint i=0; i < SZ; i++) { \
            SIMDCF_WRAPPER(this->getref(i) = CmEmulSys::satur<T>::saturate((*this).get(i) OP in_x(i), sat1), SZ, i);\
        } \
        return *this; \
} \
template <typename T, uint R, uint C> \
template <typename T2> \
matrix<T,R,C>& matrix<T,R,C>::operator OP##= (const vector_ref<T2,SZ>& x) \
{ \
        uint sat1 = 0; \
        vector<T2, SZ> in_x; in_x.assign(x); \
        for (uint i=0; i < SZ; i++) { \
            SIMDCF_WRAPPER(this->getref(i) = CmEmulSys::satur<T>::saturate((*this).get(i) OP in_x(i), sat1), SZ, i);\
        } \
        return *this; \
} \


matrix_operation(+)     // +=
matrix_operation(-)     // -=
matrix_operation(*)     // *=
matrix_operation(/ )     // /=
matrix_operation(%)     // %=
matrix_operation(&)     // &=
matrix_operation(| )     // |=
matrix_operation(^)     // ^=
matrix_operation(>> )     // >>=
matrix_operation(<< )     // <<=
#undef matrix_operation


/*
    VECTOR
*/

template <typename T, uint R, uint C>
template <uint R2, uint VS, uint WD, uint HS>
const vector<T, R2*WD> matrix<T, R, C>::genx_select(OFFSET ioff, OFFSET joff)
{
    static_assert((!std::is_same<T, double>::value), "genx_select is not supported for matrices with element type of 'double'");
    static_assert(R2 > 0, "invalid dimensions");
    static_assert(WD > 0, "invalid dimensions");
    assert(ioff < R);
    assert(joff < C);

    vector<T, R2*WD> ret(id());
    for (uint i = 0; i < R2*WD; i++) {
        SIMDCF_WRAPPER(ret(i) = data[C*ioff + joff + (i / WD)*VS + (i%WD)*HS], R2*WD, i);
    }
    return ret;
}

//1D iselect for matrix
template <typename T, uint R, uint C>
template <typename T2, uint WD>
vector<T, WD> matrix<T, R, C>::iselect(const vector<T2, WD>& index)
{
    static_assert(is_inttype<T2>::value, "invalid type");
    static_assert(WD > 0, "invalid dimensions");

    for (uint i = 0; i < WD; i++) {
        SIMDCF_WRAPPER(assert(index.get(i) < SZ), WD, i);
    }

    vector<T, WD> ret(id());
    for (uint i = 0; i < WD; i++) {
        SIMDCF_WRAPPER(ret(i) = data[index.get(i)], WD, i);
    }
    return ret;
}

template <typename T, uint R, uint C>
template <typename T2, uint WD>
vector<T, WD> matrix<T, R, C>::iselect(const vector_ref<T2, WD>& index)
{
    static_assert(is_inttype<T2>::value, "invalid type");
    static_assert(WD > 0, "invalid dimensions");

    for (uint i = 0; i < WD; i++) {
        SIMDCF_WRAPPER(assert(index.get(i) < SZ), WD, i);
    }

    vector<T, WD> ret(id());
    for (uint i = 0; i < WD; i++) {
        SIMDCF_WRAPPER(ret(i) = data[index.get(i)], WD, i);
    }
    return ret;
}

//below are 2D iselect for matrix
template <typename T, uint R, uint C>
template <typename T2, uint WD>
vector<T, WD> matrix<T, R, C>::iselect(const vector<T2, WD>& index_x, const vector<T2, WD>& index_y)
{
    static_assert((std::is_unsigned<T2>::value), "iselect index vector element type must be unsigned");
    static_assert(WD > 0, "invalid dimensions");

    for (uint i = 0; i < WD; i++) {
        SIMDCF_WRAPPER(assert(index_x.get(i) < R), WD, i);
        SIMDCF_WRAPPER(assert(index_y.get(i) < C), WD, i);
    }

    vector<T, WD> ret(id());
    for (uint i = 0; i < WD; i++) {
        SIMDCF_WRAPPER(ret(i) = data[index_x.get(i)*C + index_y.get(i)], WD, i);
    }
    return ret;
}

template <typename T, uint R, uint C>
template <typename T2, uint WD>
vector<T, WD> matrix<T, R, C>::iselect(const vector_ref<T2, WD>& index_x, const vector<T2, WD>& index_y)
{
    static_assert((std::is_unsigned<T2>::value), "iselect index vector element type must be unsigned");
    static_assert(WD > 0, "invalid dimensions");

    for (uint i = 0; i < WD; i++) {
        SIMDCF_WRAPPER(assert(index_x.get(i) < R), WD, i);
        SIMDCF_WRAPPER(assert(index_y.get(i) < C), WD, i);
    }

    vector<T, WD> ret(id());
    for (uint i = 0; i < WD; i++) {
        SIMDCF_WRAPPER(ret(i) = data[index_x.get(i)*C + index_y.get(i)], WD, i);
    }
    return ret;
}


template <typename T, uint R, uint C>
template <typename T2, uint WD>
vector<T, WD> matrix<T, R, C>::iselect(const vector<T2, WD>& index_x, const vector_ref<T2, WD>& index_y)
{
    static_assert((std::is_unsigned<T2>::value), "iselect index vector element type must be unsigned");
    static_assert(WD > 0, "invalid dimensions");

    for (uint i = 0; i < WD; i++) {
        SIMDCF_WRAPPER(assert(index_x.get(i) < R), WD, i);
        SIMDCF_WRAPPER(assert(index_y.get(i) < C), WD, i);
    }

    vector<T, WD> ret(id());
    for (uint i = 0; i < WD; i++) {
        SIMDCF_WRAPPER(ret(i) = data[index_x.get(i)*C + index_y.get(i)], WD, i);
    }
    return ret;
}

template <typename T, uint R, uint C>
template <typename T2, uint WD>
vector<T, WD> matrix<T, R, C>::iselect(const vector_ref<T2, WD>& index_x, const vector_ref<T2, WD>& index_y)
{
    static_assert((std::is_unsigned<T2>::value), "iselect index vector element type must be unsigned");
    static_assert(WD > 0, "invalid dimensions");

    for (uint i = 0; i < WD; i++) {
        SIMDCF_WRAPPER(assert(index_x.get(i) < R), WD, i);
        SIMDCF_WRAPPER(assert(index_y.get(i) < C), WD, i);
    }

    vector<T, WD> ret(id());
    for (uint i = 0; i < WD; i++) {
        SIMDCF_WRAPPER(ret(i) = data[index_x.get(i)*C + index_y.get(i)], WD, i);
    }
    return ret;
}
// end of iselect for 2D matrix

#if defined(__clang__)
  #pragma clang diagnostic pop
#elif defined(__GNUC__)
  #pragma GCC diagnostic pop
#endif

#endif
