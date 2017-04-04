// Copyright (c) 2017 Intel Corporation
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

#ifndef CM_INTERNAL_EMU_H
#define CM_INTERNAL_EMU_H


#define MAX_MASK_WIDTH 16

#ifdef __GNUC__
#include <typeinfo>   //for "const type_info *t",
using std::type_info;  //otherwise "ISO C++ forbids declaration of 'type_info' with no type_info"
#endif

namespace __CMInternal__ {

    class Dlink {
    protected:
        virtual void vtable_stub() const {}

    public:
        Dlink() {
            _next = _prev = this;
        }
        virtual ~Dlink() {}

        void unlink()
            {
                _prev->_next=_next;
                _next->_prev=_prev;
                _next=_prev=this;
            }

        // insert "this" after "prev_link"
        void insertAfter(Dlink *prev_link) {
            assert(_prev==this);
            assert(_next==this);
            _prev = prev_link;
            _next = prev_link->_next;
            prev_link->_next = this;
            _next->_prev = this;
        }

        void insertBefore(Dlink *next_link) {
            assert(_prev==this);
            assert(_next==this);
            _next = next_link;
            _prev = next_link->_prev;
            next_link->_prev = this;
            _prev->_next = this;
        }

        Dlink *getNext()
            {
                return _next;
            }

        Dlink *getPrev()
            {
                return _prev;
            }

    protected:
        Dlink *_next, *_prev;

    };

    class stackImpl
    {
    public:

        stackImpl()
            {
                depth = 0;
            }

        void* pop() {
            if (isEmpty()) return NULL;
            stackElem* e = workList.next();
            void *elem = e->elem;
            free(e);
            depth --;
            return elem;
        }

        void  push(void *elem) {
            stackElem* se = new stackElem();
            se->elem = elem;
            se->insertAfter(&workList);
            depth ++;
        };

        void* top()
            {
                return (void *)(workList.next()->elem);
            }

        int getDepth()
            {
                return depth;
            }

        bool  isEmpty()
            {
                return workList.next() == &workList;
            }

    private:

        class stackElem : public Dlink
        {
        public:
            void*  elem;

            stackElem() : Dlink() {}

            stackElem* next()
                {
                    return (stackElem*)_next;
                }

            stackElem* prev()
                {
                    return (stackElem*)_prev;
                }
        };

        stackElem workList;
        int depth;

        void  free(stackElem* n) {
            n->unlink();
            delete n;
        }
    };


    class Stack : public stackImpl {
    public:
        Stack() : stackImpl() {}

        void* pop()
            {
                return (void*)stackImpl::pop();
            }

        void  push(void* elem)
            {
                stackImpl::push(elem);
            }

        void* top()
            {
                return (void*)stackImpl::top();
            }

        int getDepth()
            {
                return stackImpl::getDepth();
            }
    };

    class maskItem {
    public:
        maskItem(ushort m, const type_info *t, int w)
            {
                assert(w <= MAX_MASK_WIDTH);
                width = w;
                mask = m;
                type = t;
                executed_mask = m;
            }

        ushort getMask()
            {
                return mask;
            }

        ushort getExecutedMask()
            {
                return executed_mask;
            }

        void setMask(ushort m)
            {
                mask = m;
            }

        void setExecutedMask(ushort m)
            {
                executed_mask |= m;
            }

        int getWidth()
            {
                return width;
            }

        void setWidth(int w)
            {
                width = w;
            }

        const type_info* getType()
            {
                return type;
            }

    private:
        int width;
        ushort mask;
        ushort executed_mask;
        const type_info* type;
    };

    class breakMaskItem : public maskItem
    {
    public:
        breakMaskItem(ushort m, const type_info *t, int w) : maskItem(m, t, w) {workingDepth = 0;}

        int getWorkingDepth()
            {
                return workingDepth;
            }

        void setWorkingDepth(int depth)
            {
                workingDepth = depth;
            }

    private:
        int workingDepth;

    };

    CM_API extern Stack* getWorkingStack();
    CM_API extern void setWorkingStack(Stack *s);
    CM_API extern Stack* getBreakStack();
    CM_API extern void setBreakStack(Stack *s);
    CM_API extern ushort getSIMDMarker();
    CM_API extern void setSIMDMarker(ushort marker);


    CM_API extern ushort __cm_internal_simd();
    CM_API extern ushort __cm_internal_simd_then_end();
    CM_API extern ushort __cm_internal_simd_else_begin();
    CM_API extern ushort __cm_internal_simd_if_end();
    CM_API extern ushort __cm_internal_simd_if_join();
    CM_API extern void __cm_internal_simd_do_while_before();
    CM_API extern ushort __cm_internal_simd_do_while_begin();
    CM_API extern ushort __cm_internal_before_do_while_end();
    CM_API extern ushort __cm_internal_simd_after_do_while_end();
    CM_API extern ushort __cm_internal_simd_break();
    CM_API extern ushort __cm_internal_simd_continue();

    CM_API extern ushort __cm_internal_simd_do_while_end(bool cond);
    CM_API extern ushort __cm_internal_simd_do_while_end(int cond);
    CM_API extern ushort __cm_internal_simd_do_while_end(unsigned int cond);
    CM_API extern ushort __cm_internal_simd_do_while_end(char cond);
    CM_API extern ushort __cm_internal_simd_do_while_end(unsigned char cond);
    CM_API extern ushort __cm_internal_simd_do_while_end(short cond);
    CM_API extern ushort __cm_internal_simd_do_while_end(unsigned short cond);

    template <typename T, uint R, uint C>
    CM_API extern ushort
    __cm_internal_simd(const matrix<T,R,C> &cond)
    {
        return cond.any();
    }

    template <typename T, uint R, uint C>
    CM_API extern ushort
    __cm_internal_simd(const matrix_ref<T,R,C> cond)
    {
        return cond.any();
    }

    template <typename T, uint S>
    CM_API extern ushort
    __cm_internal_simd(const vector<T, S> &cond)
    {
        return cond.any();
    }

    template <typename T, uint S>
    CM_API extern ushort
    __cm_internal_simd(const vector_ref<T, S> cond)
    {
        return cond.any();
    }

    template <typename T>
    CM_API extern ushort
    __cm_internal_simd(T cond)
    {
        return ushort(cond);
    }

    // ------------------------------------------------------------------------
    // For SIMD IF/THEN/ELSE/END
    // ------------------------------------------------------------------------
#define simd_if_begin_common(T,W,cond)                                  \
    {                                                                   \
        ushort simd_mask = 0;                                           \
                                                                        \
        assert(W <= MAX_MASK_WIDTH);                                    \
        for (int i = 1; i <= W; i++) {                                  \
            T e = cond.get(i - 1);                                      \
            if (e) {                                                    \
                simd_mask |= 1 << (MAX_MASK_WIDTH - i); }               \
        }                                                               \
        if (!getWorkingStack())                                         \
            setWorkingStack(new Stack());                               \
        if (!getWorkingStack()->isEmpty())                              \
            simd_mask &= ((maskItem *)getWorkingStack()->top())->getMask(); \
                                                                        \
        maskItem *mi = new maskItem(simd_mask, &typeid(T), W);          \
        getWorkingStack()->push(mi);                                    \
                                                                        \
        return simd_mask;                                               \
    }

    //Matrix
    template <typename T, uint R, uint C>
    CM_API extern ushort
    __cm_internal_simd_if_begin(const matrix<T,R,C> &cond)
    {
        int width = R * C;
        simd_if_begin_common(T,width,cond);
    }

    template <typename T, uint R, uint C>
    CM_API extern ushort
    __cm_internal_simd_if_begin(const matrix_ref<T,R,C> cond)
    {
        int width = R * C;
        simd_if_begin_common(T,width,cond);
    }

    //Vector
    template <typename T, uint SZ>
    CM_API extern ushort
    __cm_internal_simd_if_begin(const vector<T,SZ> &cond)
    {
        int width = SZ;
        simd_if_begin_common(T,width,cond);
    }

    template <typename T, uint SZ>
    CM_API extern ushort
    __cm_internal_simd_if_begin(const vector_ref<T,SZ> cond)
    {
        int width = SZ;
        simd_if_begin_common(T,width,cond);
    }

    template <typename T>
    CM_API extern ushort
    __cm_internal_simd_if_begin(const T cond)
    {
        int width = MAX_MASK_WIDTH;
        vector<unsigned int, MAX_MASK_WIDTH> v = (unsigned int) cond;
        simd_if_begin_common(unsigned int,MAX_MASK_WIDTH,v);
    }

#define simd_elseif_begin_common(T,W,cond)                              \
    {                                                                   \
        ushort simd_mask = 0;                                           \
                                                                        \
        assert(W <= MAX_MASK_WIDTH);                                    \
        for (int i = 1; i <= W; i++) {                                  \
            T e = cond.get(i - 1);                                      \
            if (e) {                                                    \
                simd_mask |= 1 << (MAX_MASK_WIDTH - i);                 \
            }                                                           \
        }                                                               \
        assert(getWorkingStack());                                      \
        assert(!getWorkingStack()->isEmpty());                          \
        simd_mask &= ((maskItem *)getWorkingStack()->top())->getMask(); \
        ((maskItem *)getWorkingStack()->top())->setMask(simd_mask);     \
        ((maskItem *)getWorkingStack()->top())->setExecutedMask(simd_mask);\
        return simd_mask;                                               \
    }

    //Matrix
    template <typename T, uint R, uint C>
    CM_API extern ushort
    __cm_internal_simd_elseif_begin(const matrix<T,R,C> &cond)
    {
        int width = R * C;
        simd_elseif_begin_common(T,width,cond);
    }

    template <typename T, uint R, uint C>
    CM_API extern ushort
    __cm_internal_simd_elseif_begin(const matrix_ref<T,R,C> cond)
    {
        int width = R * C;
        simd_elseif_begin_common(T,width,cond);
    }

    //Vector
    template <typename T, uint SZ>
    CM_API extern ushort
    __cm_internal_simd_elseif_begin(const vector<T,SZ> &cond)
    {
        int width = SZ;
        simd_elseif_begin_common(T,width,cond);
    }

    template <typename T, uint SZ>
    CM_API extern ushort
    __cm_internal_simd_elseif_begin(const vector_ref<T,SZ> cond)
    {
        int width = SZ;
        simd_elseif_begin_common(T,width,cond);
    }

    template <typename T>
    CM_API extern ushort
    __cm_internal_simd_elseif_begin(T cond)
    {
        int width = MAX_MASK_WIDTH;
        vector<unsigned int, MAX_MASK_WIDTH> v = (unsigned int) cond;
        simd_elseif_begin_common(unsigned int,MAX_MASK_WIDTH,v);
    }

#define simd_do_while_end_common(T,W,cond)                              \
    {                                                                   \
        unsigned short simd_mask = 0;                                   \
                                                                        \
        assert(W <= MAX_MASK_WIDTH);                                    \
        for (int i = 1; i <= W; i++) {                                  \
            T e = cond.get(i - 1);                                      \
            if (e)                                                      \
                simd_mask |= 1 << (MAX_MASK_WIDTH - i);                 \
        }                                                               \
        assert(!getBreakStack()->isEmpty());                            \
                                                                        \
        simd_mask &= ((maskItem *)(getBreakStack()->top()))->getMask(); \
        ((maskItem *)(getBreakStack()->top()))->setMask(simd_mask);     \
        ((maskItem *)(getWorkingStack()->top()))->setMask(simd_mask);   \
        return simd_mask;                                               \
    }

    template <typename T, uint R, uint C>
    CM_API extern ushort
    __cm_internal_simd_do_while_end(const matrix<T,R,C> &cond)
    {
        int width = R * C;
        simd_do_while_end_common(T,width,cond);
    }

    template <typename T, uint R, uint C>
    CM_API extern ushort
    __cm_internal_simd_do_while_end(const matrix_ref<T,R,C> cond)
    {
        int width = R * C;
        simd_do_while_end_common(T,width,cond);
    }

    template <typename T, uint SZ>
    CM_API extern ushort
    __cm_internal_simd_do_while_end(const vector<T,SZ> &cond)
    {
        int width = SZ;
        simd_do_while_end_common(T,width,cond);
    }

    template <typename T, uint SZ>
    CM_API extern ushort
    __cm_internal_simd_do_while_end(const vector_ref<T,SZ> cond)
    {
        int width = SZ;
        simd_do_while_end_common(T,width,cond);
    }

};

#endif /* CM_INTERNAL_EMU_H */
