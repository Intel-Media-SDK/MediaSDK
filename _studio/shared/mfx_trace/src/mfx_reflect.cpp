// Copyright (c) 2018 Intel Corporation
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

#include "mfx_reflect.h"

#if defined(MFX_TRACE_ENABLE_REFLECT)

#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cstddef>

#include "mfxstructures.h"
#include "mfxvp8.h"
#include "mfxvp9.h"
#include "mfxplugin.h"
#include "mfxmvc.h"
#include "mfxcamera.h"
#include "mfxfei.h"
#include "mfxla.h"
#include "mfxsc.h"

#if (MFX_VERSION >= 1027)
#include "mfxfeihevc.h"
#endif

#include "ts_typedef.h"

#include <memory>

namespace mfx_reflect
{
    static AccessibleTypesCollection g_Reflection;

    AccessibleTypesCollection GetReflection()
    {
        return g_Reflection;
    }

    void AccessibleTypesCollection::Initialize()
    {
        if (!g_Reflection.m_bIsInitialized)
        {
            g_Reflection.DeclareMsdkStructs();
            g_Reflection.m_bIsInitialized = true;
        }
    }

    template<class T>
      struct mfx_ext_buffer_id {
          enum { id = 0 };
      };

#define EXTBUF(STRUCT, ID)                               \
        template<>struct mfx_ext_buffer_id<STRUCT> {     \
        enum { id = ID }; };

#include "ts_ext_buffers_decl.h"

    void AccessorField::SetFieldAddress()
    {
        m_P = (*m_Iterator)->GetAddress(m_BaseStruct.m_P);
        char *result = (char*)m_P;
        result += m_pReflection->FieldType->Size * m_IndexElement;
        m_P = (void*)result;
    }

    AccessorField& AccessorField::operator++()
    {
        m_IndexElement = 0;
        m_Iterator++;
        if (IsValid())
        {
            SetIterator(m_Iterator);
        }
        return *this;
    }

    bool AccessorField::IsValid() const
    {
        return m_Iterator != m_BaseStruct.m_pReflection->m_Fields.end();
    }

    AccessorType AccessorField::AccessSubtype() const
    {
        return AccessorType(m_P, *(m_pReflection->FieldType));
    }


    AccessorField AccessorType::AccessField(ReflectedField::FieldsCollectionCI iter) const
    {
        if (m_pReflection->m_Fields.end() != iter)
        {
            return AccessorField(*this, iter); // Address of base, not field.
        }
        else
        {
            throw std::invalid_argument(std::string("No such field"));
        }
    }

    AccessorField AccessorType::AccessField(const std::string& fieldName) const
    {
        ReflectedField::FieldsCollectionCI iter = m_pReflection->FindField(fieldName);
        return AccessField(iter);
    }

    AccessorField AccessorType::AccessFirstField() const
    {
        return AccessField(m_pReflection->m_Fields.begin());
    }

    AccessorType AccessorType::AccessSubtype(const std::string& fieldName) const
    {
        return AccessField(fieldName).AccessSubtype();
    }

    ReflectedType::ReflectedType(ReflectedTypesCollection *pCollection, std::type_index typeIndex, const std::string& typeName, size_t size, bool isPointer, mfxU32 extBufferId)
        : m_TypeIndex(typeIndex)
        , TypeNames(1, typeName)
        , Size(size)
        , m_pCollection(pCollection)
        , m_bIsPointer(isPointer)
        , ExtBufferId(extBufferId)
    {
        if (NULL == m_pCollection)
        {
            m_pCollection = NULL;
        }
    }

    ReflectedField::SP ReflectedType::AddField(std::type_index typeIndex, const std::string &typeName, size_t typeSize, bool isPointer, size_t offset, const std::string &fieldName, size_t count, mfxU32 extBufferId)
    {
        ReflectedField::SP pField;
        if (typeName.empty())
        {
            throw std::invalid_argument(std::string("Unexpected behavior - typeName is empty"));
        }
        if (NULL == m_pCollection)
        {
            return pField;
        }
        else
        {
            ReflectedType* pType = m_pCollection->FindOrDeclareType(typeIndex, typeName, typeSize, isPointer, extBufferId).get();
            if (pType != NULL)
            {
                StringList::iterator findIterator = std::find(pType->TypeNames.begin(), pType->TypeNames.end(), typeName);
                if (pType->TypeNames.end() == findIterator)
                {
                    throw std::invalid_argument(std::string("Unexpected behavior - fieldTypeName is NULL"));
                }
                m_Fields.push_back(ReflectedField::SP(new ReflectedField(m_pCollection, this, pType, *findIterator, offset, fieldName, count))); //std::make_shared cannot access protected constructor
                pField = m_Fields.back();
            }
            return pField;
        }

    }

    ReflectedField::FieldsCollectionCI ReflectedType::FindField(const std::string& fieldName) const
    {
        ReflectedField::FieldsCollectionCI iter = m_Fields.begin();
        for (; iter != m_Fields.end(); ++iter)
        {
            if ((*iter)->FieldName == fieldName)
            {
                break;
            }
        }
        return iter;
    }

    ReflectedType::SP ReflectedTypesCollection::FindExistingByTypeInfoName(const char* name) //currenly we are not using this
    {
        for (Container::iterator iter = m_KnownTypes.begin(); iter != m_KnownTypes.end(); ++iter)
        {
            if (0 == strcmp(iter->first.name(), name))
            {
                return iter->second;
            }
        }
        ReflectedType::SP pEmptyType;
        return pEmptyType;
    }

    ReflectedType::SP ReflectedTypesCollection::FindExistingType(std::type_index typeIndex)
    {
        Container::const_iterator it = m_KnownTypes.find(typeIndex);
        if (m_KnownTypes.end() != it)
        {
            return it->second;
        }
        ReflectedType::SP pEmptyType;
        return pEmptyType;
    }

    ReflectedType::SP ReflectedTypesCollection::FindExtBufferTypeById(mfxU32 ExtBufferId) //optimize with map (index -> type)
    {
        for (Container::iterator iter = m_KnownTypes.begin(); iter != m_KnownTypes.end(); ++iter)
        {
            if (ExtBufferId == iter->second->ExtBufferId)
            {
                return iter->second;
            }
        }
        ReflectedType::SP pEmptyExtBufferType;
        return pEmptyExtBufferType;
    }

    ReflectedType::SP ReflectedTypesCollection::DeclareType(std::type_index typeIndex, const std::string& typeName, size_t typeSize, bool isPointer, mfxU32 extBufferId)
    {
        if (m_KnownTypes.end() == m_KnownTypes.find(typeIndex))
        {
            ReflectedType::SP pType;
            pType = std::make_shared<ReflectedType>(this, typeIndex, typeName, typeSize, isPointer, extBufferId);
            m_KnownTypes.insert(std::make_pair(pType->m_TypeIndex, pType));
            return pType;
        }
        throw std::invalid_argument(std::string("Unexpected behavior - type is already declared"));
    }

    ReflectedType::SP ReflectedTypesCollection::FindOrDeclareType(std::type_index typeIndex, const std::string& typeName, size_t typeSize, bool isPointer, mfxU32 extBufferId)
    {
        ReflectedType::SP pType = FindExistingType(typeIndex);
        if (pType == NULL)
        {
            pType = DeclareType(typeIndex, typeName, typeSize, isPointer, extBufferId);
        }
        else
        {
            if (typeSize != pType->Size)
            {
                pType.reset();
            }
            else if (!typeName.empty())
            {
                ReflectedType::StringList::iterator findIterator = std::find(pType->TypeNames.begin(), pType->TypeNames.end(), typeName);
                if (pType->TypeNames.end() == findIterator)
                {
                    pType->TypeNames.push_back(typeName);
                }
            }
        }
        return pType;
    }

    template <class T> bool PrintFieldIfTypeMatches(std::ostream& stream, AccessorField field)
    {
        bool bResult = false;
        if (field.m_pReflection->FieldType->m_TypeIndex == field.m_pReflection->m_pCollection->FindExistingType<T>()->m_TypeIndex)
        {
            stream << field.Get<T>();
            bResult = true;
        }
        return bResult;
    }

    void PrintFieldValue(std::ostream &stream, AccessorField field)
    {
        if (field.m_pReflection->FieldType->m_bIsPointer)
        {
            stream << "0x" << field.Get<void*>();
        }
        else
        {
            bool bPrinted = false;

#define PRINT_IF_TYPE(T) if (!bPrinted) { bPrinted = PrintFieldIfTypeMatches<T>(stream, field); }
            PRINT_IF_TYPE(mfxU16);
            PRINT_IF_TYPE(mfxI16);
            PRINT_IF_TYPE(mfxU32);
            PRINT_IF_TYPE(mfxI32);
            if (!bPrinted)
            {
                size_t fieldSize = field.m_pReflection->FieldType->Size;
                {
                    stream << "<Unknown type \"" << field.m_pReflection->FieldType->m_TypeIndex.name() << "\" (size = " << fieldSize << ")>";
                }
            }
        }
    }

    void PrintFieldName(std::ostream &stream, AccessorField field)
    {
        stream << field.m_pReflection->FieldName;
        if (field.m_pReflection->Count > 1)
        {
            stream << "[" << field.GetIndexElement() << "]";
        }
    }

    std::ostream& operator<< (std::ostream& stream, AccessorField field)
    {
        PrintFieldName(stream, field);
        stream << " = ";
        PrintFieldValue(stream, field);
        return stream;
    }

    std::ostream& operator<< (std::ostream& stream, AccessorType data)
    {
        stream << data.m_pReflection->m_TypeIndex.name() << " = {";
        size_t nFields = data.m_pReflection->m_Fields.size();
        for (AccessorField field = data.AccessFirstField(); field.IsValid(); ++field)
        {
            if (nFields > 1)
            {
                stream << std::endl;
            }
            stream << "\t" << field;
        }
        stream << "}";
        return stream;
    }

    TypeComparisonResultP CompareExtBufferLists(mfxExtBuffer** pExtParam1, mfxU16 numExtParam1, mfxExtBuffer** pExtParam2, mfxU16 numExtParam2, ReflectedTypesCollection* collection);
    AccessorTypeP GetAccessorOfExtBufferOriginalType(mfxExtBuffer& pExtInnerParam, ReflectedTypesCollection& collection);

    TypeComparisonResultP CompareTwoStructs(AccessorType data1, AccessorType data2) // Always return not null result
    {
        TypeComparisonResultP result = std::make_shared<TypeComparisonResult>();
        if (data1.m_pReflection != data2.m_pReflection)
        {
            throw std::invalid_argument(std::string("Types mismatch"));
        }

        mfxExtBuffer** pExtParam1 = NULL;
        mfxExtBuffer** pExtParam2 = NULL;
        mfxU16 numExtParam = 0;

        for (AccessorField field1 = data1.AccessFirstField(); field1.IsValid(); ++field1)
        {
            AccessorField field2 = data2.AccessField(field1.m_Iterator);

            if (std::type_index(typeid(mfxExtBuffer**)) == field1.m_pReflection->FieldType->m_TypeIndex)
            {
                pExtParam1 = field1.Get<mfxExtBuffer**>();
                pExtParam2 = field2.Get<mfxExtBuffer**>();
            }
            else if ((field1.m_pReflection->FieldName == "NumExtParam") && (field1.m_pReflection->FieldType->m_TypeIndex == std::type_index(typeid(mfxU16))) && (field2.m_pReflection->FieldType->m_TypeIndex == std::type_index(typeid(mfxU16))))
            {
                if (field1.Get<mfxU16>() == field2.Get<mfxU16>())
                {
                    numExtParam = field1.Get<mfxU16>();
                }
                else
                {
                    throw std::invalid_argument(std::string("NumExtParam mismatch"));
                }
            }
            else
            {
                for (size_t i = 0; i < field1.m_pReflection->Count; ++i)
                {
                    field1.SetIndexElement(i);
                    field2.SetIndexElement(i);

                    AccessorType subtype1 = field1.AccessSubtype();
                    TypeComparisonResultP subtypeResult = NULL;

                    if (subtype1.m_pReflection->m_Fields.size() > 0)
                    {
                        AccessorType subtype2 = field2.AccessSubtype();
                        if (subtype1.m_pReflection != subtype2.m_pReflection)
                        {
                            throw std::invalid_argument(std::string("Subtypes mismatch - should never happen for same types"));
                        }
                        subtypeResult = CompareTwoStructs(subtype1, subtype2);
                    }

                    if (!field1.Equal(field2))
                    {
                        FieldComparisonResult fields = { field1 , field2, subtypeResult };
                        result->push_back(fields);
                    }
                }
            }
        }

        if ((pExtParam1 != NULL) && (pExtParam2 != NULL) && (numExtParam > 0))
        {
            TypeComparisonResultP extBufferCompareResult = CompareExtBufferLists(pExtParam1, numExtParam, pExtParam2, numExtParam, data1.AccessFirstField().m_pReflection->m_pCollection);
            if (extBufferCompareResult != NULL)
            {
                result->splice(result->end(), *extBufferCompareResult); //move elements of *extBufferCompareResult to the end of *result
                if (!extBufferCompareResult->extBufferIdList.empty())
                {
                    result->extBufferIdList.splice(result->extBufferIdList.end(), extBufferCompareResult->extBufferIdList);
                }
            }
            else
            {
                throw std::invalid_argument(std::string("Unexpected behavior - ExtBuffer comparison result is NULL"));
            }
        }
        return result;
    }

    AccessorTypeP GetAccessorOfExtBufferOriginalType(mfxExtBuffer& pExtBufferParam, ReflectedTypesCollection& collection)
    {
        AccessorTypeP pExtBuffer;
        mfxU32 id = pExtBufferParam.BufferId;
        if (0 != id)
        {
            ReflectedType::SP pTypeExtBuffer = collection.FindExtBufferTypeById(id); //find in KnownTypes this BufferId
            if (pTypeExtBuffer != NULL)
            {
                pExtBuffer = std::make_shared<AccessorType>(&pExtBufferParam, *pTypeExtBuffer);
            }
        }
        return pExtBuffer;
    }

    TypeComparisonResultP CompareExtBufferLists(mfxExtBuffer** pExtParam1, mfxU16 numExtParam1, mfxExtBuffer** pExtParam2, mfxU16 numExtParam2, ReflectedTypesCollection* collection) //always return not null result
    {
        TypeComparisonResultP result = std::make_shared<TypeComparisonResult>();
        if (NULL != pExtParam1 && NULL != pExtParam2 && NULL != collection)
        {
            for (int i = 0; i < numExtParam1 && i < numExtParam2; i++)
            {
                //supported only extBuffers with same Id order
                if (NULL != pExtParam1[i] && NULL != pExtParam2[i])
                {
                    AccessorTypeP extBufferOriginTypeAccessor1 = GetAccessorOfExtBufferOriginalType(*pExtParam1[i], *collection);
                    AccessorTypeP extBufferOriginTypeAccessor2 = GetAccessorOfExtBufferOriginalType(*pExtParam2[i], *collection);
                    if (NULL != extBufferOriginTypeAccessor1 && NULL != extBufferOriginTypeAccessor2)
                    {
                        TypeComparisonResultP tempResult = CompareTwoStructs(*extBufferOriginTypeAccessor1, *extBufferOriginTypeAccessor2);
                        if (NULL != tempResult)
                        {
                            result->splice(result->end(), *tempResult);
                        }
                        else
                        {
                            throw std::invalid_argument(std::string("Unexpected behavior - comparison result is NULL"));
                        }
                    }
                    else
                    {
                        result->extBufferIdList.push_back(pExtParam1[i]->BufferId);
                    }
                }
            }
        }
        return result;
    }

    void PrintStuctsComparisonResult(std::ostream& comparisonResult, const std::string& prefix, const TypeComparisonResultP& result)
    {
        for (std::list<FieldComparisonResult>::iterator i = result->begin(); i != result->end(); ++i)
        {
            TypeComparisonResultP subtypeResult = i->subtypeComparisonResultP;
            ReflectedType* aggregatingType = i->accessorField1.m_pReflection->AggregatingType;
            std::list< std::string >::const_iterator it = aggregatingType->TypeNames.begin();
            std::string strTypeName = ((it != aggregatingType->TypeNames.end()) ? *(it) : "unknown_type");

            if (NULL != subtypeResult)
            {
                std::stringstream fieldName;
                PrintFieldName(fieldName, i->accessorField1);

                std::string strFieldName = fieldName.str();
                std::string newprefix;

                newprefix = prefix.empty() ? (strTypeName + "." + strFieldName) : (prefix + "." + strFieldName);
                PrintStuctsComparisonResult(comparisonResult, newprefix, subtypeResult);
            }
            else
            {
                if (!prefix.empty()) { comparisonResult << prefix << "."; }
                else if (!strTypeName.empty()) { comparisonResult << strTypeName << "."; }
                comparisonResult << i->accessorField1 << " -> ";
                PrintFieldValue(comparisonResult, i->accessorField2);
                comparisonResult << std::endl;
            }
        }
        if (!result->extBufferIdList.empty())
        {
            comparisonResult << "Id of unparsed ExtBuffer types: " << std::endl;
            while (!result->extBufferIdList.empty())
            {
                unsigned char* FourCC = reinterpret_cast<unsigned char*>(&result->extBufferIdList.front());
                comparisonResult << "0x" << std::hex << std::setfill('0') << result->extBufferIdList.front() << " \"" << FourCC[0] << FourCC[1] << FourCC[2] << FourCC[3] << "\"";
                result->extBufferIdList.pop_front();
                if (!result->extBufferIdList.empty()) { comparisonResult << ", "; }
            }
        }
    }

    std::string CompareStructsToString(AccessorType data1, AccessorType data2)
    {
        std::ostringstream comparisonResult;
        if (data1.m_P == data2.m_P)
        {
            comparisonResult << "Comparing of VideoParams is unsupported: In and Out pointers are the same.";
        }
        else
        {
            comparisonResult << "Incompatible VideoParams were updated:" << std::endl;
            TypeComparisonResultP result = CompareTwoStructs(data1, data2);
            PrintStuctsComparisonResult(comparisonResult, "", result);
        }
        return comparisonResult.str();
    }

    template <class T>
    ReflectedField::SP AddFieldT(ReflectedType &type, const std::string typeName, size_t offset, const std::string fieldName, size_t count)
    {
        unsigned int extBufId = 0;
        extBufId = mfx_ext_buffer_id<T>::id;
        bool isPointer = false;
        isPointer = std::is_pointer<T>();
        return type.AddField(std::type_index(typeid(T)), typeName, sizeof(T), isPointer, offset, fieldName, count, extBufId);
    }

    template <class T>
    ReflectedType::SP DeclareTypeT(ReflectedTypesCollection& collection, const std::string typeName)
    {
        unsigned int extBufId = 0;
        extBufId = mfx_ext_buffer_id<T>::id;
        bool isPointer = false;
        isPointer = std::is_pointer<T>();
        return collection.DeclareType(std::type_index(typeid(T)), typeName, sizeof(T), isPointer, extBufId);
    }

    void ReflectedTypesCollection::DeclareMsdkStructs()
    {
#define STRUCT(TYPE, FIELDS) {                                  \
    typedef TYPE BaseType;                                      \
    ReflectedType::SP pType = DeclareTypeT<TYPE>(*this, #TYPE);  \
    FIELDS                                                      \
    }

#define FIELD_T(FIELD_TYPE, FIELD_NAME)                 \
    (void)AddFieldT<FIELD_TYPE>(                        \
        *pType,                                         \
        #FIELD_TYPE,                                    \
        offsetof(BaseType, FIELD_NAME),                 \
        #FIELD_NAME,                                    \
        (sizeof(((BaseType*)0)->FIELD_NAME)/sizeof(::FIELD_TYPE)) );

#define FIELD_S(FIELD_TYPE, FIELD_NAME) FIELD_T(FIELD_TYPE, FIELD_NAME)

#include "ts_struct_decl.h"
    }
}

#endif // #if defined(MFX_TRACE_ENABLE_REFLECT)
