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

#pragma once

#include "mfx_config.h"
#include "mfx_trace.h"

#if defined(MFX_TRACE_ENABLE_REFLECT)

#include <memory>
#include <string>
#include <string.h> //for memcmp on Linux
#include <list>
#include <vector>
#include <map>
#include <stdexcept>

#include <typeindex>

namespace mfx_reflect
{
    class ReflectedField;
    class ReflectedType;
    class ReflectedTypesCollection;

    class ReflectedField
    {
    public:
        typedef std::shared_ptr<ReflectedField> SP;
        typedef std::vector<ReflectedField::SP> FieldsCollection;
        typedef FieldsCollection::const_iterator FieldsCollectionCI;


        ReflectedType*  FieldType;
        ReflectedType*  AggregatingType;
        const std::string &             FieldTypeName; //single C++ type may have several typedef aliases. This is original name that was used for this field declaration.
        size_t                          Offset;
        const std::string               FieldName;
        size_t                          Count;
        ReflectedTypesCollection *      m_pCollection;

        void * GetAddress(void *pBase)
        {
            return ((unsigned char*)pBase) + Offset;
        }

    protected:
        ReflectedField(ReflectedTypesCollection *pCollection, ReflectedType* aggregatingType, ReflectedType* fieldType, const std::string& fieldTypeName, size_t offset, const std::string& fieldName, size_t count)
            : FieldType(fieldType)
            , AggregatingType(aggregatingType)
            , FieldTypeName(fieldTypeName)
            , Offset(offset)
            , FieldName(fieldName)
            , Count(count)
            , m_pCollection(pCollection)
        {
        }

        friend class ReflectedType;
    };

    class ReflectedType
    {
    public:
        std::type_index         m_TypeIndex;
        std::list< std::string >  TypeNames;
        size_t          Size;
        ReflectedTypesCollection *m_pCollection;
        bool m_bIsPointer;
        unsigned int ExtBufferId;

        ReflectedType(ReflectedTypesCollection *pCollection, std::type_index typeIndex, const std::string& typeName, size_t size, bool isPointer, unsigned int extBufferId);

        ReflectedField::SP AddField(std::type_index typeIndex, const std::string &typeName, size_t typeSize, bool isPointer, size_t offset, const std::string &fieldName, size_t count, unsigned int extBufferId);

        ReflectedField::FieldsCollectionCI FindField(const std::string& fieldName) const;

        ReflectedField::FieldsCollection m_Fields;
        typedef std::shared_ptr<ReflectedType> SP;
        typedef std::list< std::string> StringList;

    };

    class ReflectedTypesCollection
    {
    public:
        typedef std::map<std::type_index, ReflectedType::SP> Container;

        Container m_KnownTypes;

        template <class T> ReflectedType::SP FindExistingType()
        {
            return FindExistingType(std::type_index(typeid(T)));
        }

        ReflectedType::SP FindExistingByTypeInfoName(const char* name);

        ReflectedType::SP FindExistingType(std::type_index typeIndex);
        ReflectedType::SP FindExtBufferTypeById(unsigned int ExtBufferId);

        ReflectedType::SP DeclareType(std::type_index typeIndex, const std::string& typeName, size_t typeSize, bool isPointer, unsigned int extBufferId);

        ReflectedType::SP FindOrDeclareType(std::type_index typeIndex, const std::string& typeName, size_t typeSize, bool isPointer, unsigned int extBufferId);

        void DeclareMsdkStructs();
    };

    template <class T>
    class AccessorBase
    {
    public:
        void *      m_P;
        const T*    m_pReflection;
        AccessorBase(void *p, const T &reflection)
            : m_P(p)
            , m_pReflection(&reflection)
        {}
        typedef std::shared_ptr<T> P;

        template <class ORIGINAL_TYPE>
        const ORIGINAL_TYPE& Get() const
        {
            return *((const ORIGINAL_TYPE*)m_P);
        }
    };

    template <class T, class I>
    class IterableAccessor : public AccessorBase < T >
    {
    public:
        I m_Iterator;

        IterableAccessor(void *p, I iterator)
            : AccessorBase<T>(p, *(*iterator))
            , m_Iterator(iterator)
        {}
    };

    class AccessorType;

    class AccessorField : public IterableAccessor < ReflectedField, ReflectedField::FieldsCollectionCI >
    {
    public:
        AccessorField(const AccessorType &baseStruct, ReflectedField::FieldsCollectionCI iterator)
            : IterableAccessor(NULL, iterator)
            , m_BaseStruct(baseStruct)
        {
            m_IndexElement = 0;
            SetFieldAddress();
        }

        size_t GetIndexElement() const
        {
            return m_IndexElement;
        }

        void SetIndexElement(size_t index)
        {
            if (index >= m_pReflection->Count)
            {
                throw std::invalid_argument(std::string("Index is not valid"));
            }

            m_IndexElement = index;
            SetFieldAddress();
        }

        void Move(std::ptrdiff_t delta)
        {
            this->m_P = (char*)this->m_P + delta;
        }

        AccessorField& operator++();

        bool Equal(const AccessorField& field) const
        {
            if (field.m_pReflection != m_pReflection)
            {
                throw std::invalid_argument(std::string("Types mismatch"));
            }

            if (field.m_IndexElement != m_IndexElement)
            {
                throw std::invalid_argument(std::string("Indices mismatch"));
            }

            size_t size = m_pReflection->FieldType->Size;
            return (0 == memcmp(m_P, field.m_P, size));
        }

        AccessorType AccessSubtype() const;

        bool IsValid() const;

    protected:
        void SetFieldAddress();
        const AccessorType &m_BaseStruct;
        size_t m_IndexElement;

        void SetIterator(ReflectedField::FieldsCollectionCI iterator)
        {
            m_Iterator = iterator;
            m_pReflection = (*m_Iterator).get();
            SetFieldAddress();
        }
    };

    class AccessorType : public AccessorBase < ReflectedType >
    {
    public:
        AccessorType(void *p, const ReflectedType &reflection) : AccessorBase(p, reflection) {}

        AccessorField AccessField(ReflectedField::FieldsCollectionCI iter) const;
        AccessorField AccessField(const std::string& fieldName) const;
        AccessorField AccessFirstField() const;
        AccessorType AccessSubtype(const std::string& fieldName) const;
    };

    class AccessibleTypesCollection : public ReflectedTypesCollection
    {
    public:
        AccessibleTypesCollection()
            : m_bIsInitialized(false)
        {}
        bool m_bIsInitialized;
        template <class T>
        AccessorType Access(T *p)
        {
            ReflectedType::SP pType = FindExistingType(std::type_index(typeid(T)));
            if (pType == NULL)
            {
                throw std::invalid_argument(std::string("Unknown type"));
            }
            else
            {
                return AccessorType(p, *pType);
            }
        }

        static void Initialize();
    };

    class TypeComparisonResult;
    typedef std::shared_ptr<TypeComparisonResult> TypeComparisonResultP;

    struct FieldComparisonResult
    {
        AccessorField accessorField1;
        AccessorField accessorField2;
        TypeComparisonResultP subtypeComparisonResultP;
    };

    class TypeComparisonResult : public std::list < FieldComparisonResult >
    {
    public:
        list<unsigned int> extBufferIdList;
    };

    typedef std::shared_ptr<AccessorType> AccessorTypeP;

    TypeComparisonResultP CompareTwoStructs(AccessorType data1, AccessorType data2);

    std::string CompareStructsToString(AccessorType data1, AccessorType data2);
    void PrintStuctsComparisonResult(std::ostream& comparisonResult, const std::string& prefix, const TypeComparisonResultP& result);

    template <class T> bool PrintFieldIfTypeMatches(std::ostream& stream, AccessorField field);
    void PrintFieldValue(std::ostream &stream, AccessorField field);
    std::ostream& operator<< (std::ostream& stream, AccessorField field);
    std::ostream& operator<< (std::ostream& stream, AccessorType data);

    template <class T> ReflectedType::SP DeclareTypeT(ReflectedTypesCollection& collection, const std::string typeName);

    mfx_reflect::AccessibleTypesCollection GetReflection();
} // namespace mfx_reflect

#endif // #if defined(MFX_TRACE_ENABLE_REFLECT)
