/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifdef MOZILLA_INTERNAL_API
#undef MOZILLA_INTERNAL_API
#define nsSupportsCStringImpl nsSupportsCStringAPIImpl
#define nsSupportsStringImpl nsSupportsStringAPIImpl
#include "nsStringEnumerator.h"
#include "nsISimpleEnumerator.h"
#include "nsSupportsPrimitives.h"
#include "mozilla/Attributes.h"
#include "nsTArray.h"

#include "nsMemory.h"
#include "prprf.h"

using mozilla::fallible_t;

/*****************************************************************************
 * nsSupportsCStringImpl
 *****************************************************************************/

NS_IMPL_ISUPPORTS(nsSupportsCStringImpl, nsISupportsCString,
                  nsISupportsPrimitive)

NS_IMETHODIMP nsSupportsCStringImpl::GetType(uint16_t *aType)
{
    NS_ASSERTION(aType, "Bad pointer");

    *aType = TYPE_CSTRING;
    return NS_OK;
}

NS_IMETHODIMP nsSupportsCStringImpl::GetData(nsACString& aData)
{
    aData = mData;
    return NS_OK;
}

NS_IMETHODIMP nsSupportsCStringImpl::ToString(char **_retval)
{
    *_retval = ToNewCString(mData);

    if (!*_retval)
        return NS_ERROR_OUT_OF_MEMORY;
    
    return NS_OK;
}

NS_IMETHODIMP nsSupportsCStringImpl::SetData(const nsACString& aData)
{
    const char* data;
    PRUint32 len = NS_CStringGetData(aData, &data);
    mData.Assign(data, len);
    return NS_OK;
}



/*****************************************************************************
 * nsSupportsStringImpl
 *****************************************************************************/

NS_IMPL_ISUPPORTS(nsSupportsStringImpl, nsISupportsString,
                  nsISupportsPrimitive)

NS_IMETHODIMP nsSupportsStringImpl::GetType(uint16_t *aType)
{
    NS_ASSERTION(aType, "Bad pointer");

    *aType = TYPE_STRING;
    return NS_OK;
}

NS_IMETHODIMP nsSupportsStringImpl::GetData(nsAString& aData)
{
    aData = mData;
    return NS_OK;
}

NS_IMETHODIMP nsSupportsStringImpl::ToString(char16_t **_retval)
{
    *_retval = ToNewUnicode(mData);
    
    if (!*_retval)
        return NS_ERROR_OUT_OF_MEMORY;
    
    return NS_OK;
}

NS_IMETHODIMP nsSupportsStringImpl::SetData(const nsAString& aData)
{
    const char16_t* data;
    PRUint32 len = NS_StringGetData(aData, &data);
    mData.Assign(data, len);
    return NS_OK;
}


//
// nsStringEnumeratorExport
//

class nsStringEnumeratorExport MOZ_FINAL : public nsIStringEnumerator,
                                     public nsIUTF8StringEnumerator,
                                     public nsISimpleEnumerator
{
public:
    nsStringEnumeratorExport(const nsTArray<nsString>* aArray, bool aOwnsArray) :
        mArray(aArray), mIndex(0), mOwnsArray(aOwnsArray), mIsUnicode(true)
    {}
    
    nsStringEnumeratorExport(const nsTArray<nsCString>* aArray, bool aOwnsArray) :
        mCArray(aArray), mIndex(0), mOwnsArray(aOwnsArray), mIsUnicode(false)
    {}

    nsStringEnumeratorExport(const nsTArray<nsString>* aArray, nsISupports* aOwner) :
        mArray(aArray), mIndex(0), mOwner(aOwner), mOwnsArray(false), mIsUnicode(true)
    {}
    
    nsStringEnumeratorExport(const nsTArray<nsCString>* aArray, nsISupports* aOwner) :
        mCArray(aArray), mIndex(0), mOwner(aOwner), mOwnsArray(false), mIsUnicode(false)
    {}

    NS_DECL_ISUPPORTS
    NS_DECL_NSIUTF8STRINGENUMERATOR

    // have to declare nsIStringEnumerator manually, because of
    // overlapping method names
    NS_IMETHOD GetNext(nsAString& aResult);
    NS_DECL_NSISIMPLEENUMERATOR

private:
    ~nsStringEnumeratorExport() {
        if (mOwnsArray) {
            // const-casting is safe here, because the NS_New*
            // constructors make sure mOwnsArray is consistent with
            // the constness of the objects
            if (mIsUnicode)
                delete const_cast<nsTArray<nsString>*>(mArray);
            else
                delete const_cast<nsTArray<nsCString>*>(mCArray);
        }
    }

    union {
        const nsTArray<nsString>* mArray;
        const nsTArray<nsCString>* mCArray;
    };

    inline uint32_t Count() {
        return mIsUnicode ? mArray->Length() : mCArray->Length();
    }
    
    uint32_t mIndex;

    // the owner allows us to hold a strong reference to the object
    // that owns the array. Having a non-null value in mOwner implies
    // that mOwnsArray is false, because we rely on the real owner
    // to release the array
    nsCOMPtr<nsISupports> mOwner;
    bool mOwnsArray;
    bool mIsUnicode;
};

NS_IMPL_ISUPPORTS(nsStringEnumeratorExport,
                  nsIStringEnumerator,
                  nsIUTF8StringEnumerator,
                  nsISimpleEnumerator)

NS_IMETHODIMP
nsStringEnumeratorExport::HasMore(bool* aResult)
{
    *aResult = mIndex < Count();
    return NS_OK;
}

NS_IMETHODIMP
nsStringEnumeratorExport::HasMoreElements(bool* aResult)
{
    return HasMore(aResult);
}

NS_IMETHODIMP
nsStringEnumeratorExport::GetNext(nsISupports** aResult)
{
    if (mIsUnicode) {
        nsSupportsStringImpl* stringImpl = new nsSupportsStringImpl();
        if (!stringImpl) return NS_ERROR_OUT_OF_MEMORY;
        
        stringImpl->SetData(mArray->ElementAt(mIndex++));
        *aResult = stringImpl;
    }
    else {
        nsSupportsCStringImpl* cstringImpl = new nsSupportsCStringImpl();
        if (!cstringImpl) return NS_ERROR_OUT_OF_MEMORY;

        cstringImpl->SetData(mCArray->ElementAt(mIndex++));
        *aResult = cstringImpl;
    }
    NS_ADDREF(*aResult);
    return NS_OK;
}

NS_IMETHODIMP
nsStringEnumeratorExport::GetNext(nsAString& aResult)
{
    if (NS_WARN_IF(mIndex >= Count()))
        return NS_ERROR_UNEXPECTED;

    if (mIsUnicode)
        aResult = mArray->ElementAt(mIndex++);
    else
        CopyUTF8toUTF16(mCArray->ElementAt(mIndex++), aResult);
    
    return NS_OK;
}

NS_IMETHODIMP
nsStringEnumeratorExport::GetNext(nsACString& aResult)
{
    if (NS_WARN_IF(mIndex >= Count()))
        return NS_ERROR_UNEXPECTED;
    
    if (mIsUnicode)
        CopyUTF16toUTF8(mArray->ElementAt(mIndex++), aResult);
    else
        aResult = mCArray->ElementAt(mIndex++);
    
    return NS_OK;
}

template<class T>
static inline nsresult
StringEnumeratorTail(T** aResult)
{
    if (!*aResult)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(*aResult);
    return NS_OK;
}

//
// constructors
//

NS_EXPORT_(nsresult)
NS_NewStringEnumerator(nsIStringEnumerator** aResult,
                       const nsTArray<nsString>* aArray, nsISupports* aOwner)
{
    if (NS_WARN_IF(!aResult) || NS_WARN_IF(!aArray))
        return NS_ERROR_INVALID_ARG;
    
    *aResult = new nsStringEnumeratorExport(aArray, aOwner);
    return StringEnumeratorTail(aResult);
}


NS_EXPORT_(nsresult)
NS_NewUTF8StringEnumerator(nsIUTF8StringEnumerator** aResult,
                           const nsTArray<nsCString>* aArray, nsISupports* aOwner)
{
    if (NS_WARN_IF(!aResult) || NS_WARN_IF(!aArray))
        return NS_ERROR_INVALID_ARG;
    
    *aResult = new nsStringEnumeratorExport(aArray, aOwner);
    return StringEnumeratorTail(aResult);
}

NS_EXPORT_(nsresult)
NS_NewAdoptingStringEnumerator(nsIStringEnumerator** aResult,
                               nsTArray<nsString>* aArray)
{
    if (NS_WARN_IF(!aResult) || NS_WARN_IF(!aArray))
        return NS_ERROR_INVALID_ARG;
    
    *aResult = new nsStringEnumeratorExport(aArray, true);
    return StringEnumeratorTail(aResult);
}

NS_EXPORT_(nsresult)
NS_NewAdoptingUTF8StringEnumerator(nsIUTF8StringEnumerator** aResult,
                                   nsTArray<nsCString>* aArray)
{
    if (NS_WARN_IF(!aResult) || NS_WARN_IF(!aArray))
        return NS_ERROR_INVALID_ARG;
    
    *aResult = new nsStringEnumeratorExport(aArray, true);
    return StringEnumeratorTail(aResult);
}

// const ones internally just forward to the non-const equivalents
NS_EXPORT_(nsresult)
NS_NewStringEnumerator(nsIStringEnumerator** aResult,
                       const nsTArray<nsString>* aArray)
{
    if (NS_WARN_IF(!aResult) || NS_WARN_IF(!aArray))
        return NS_ERROR_INVALID_ARG;
    
    *aResult = new nsStringEnumeratorExport(aArray, false);
    return StringEnumeratorTail(aResult);
}

NS_EXPORT_(nsresult)
NS_NewUTF8StringEnumerator(nsIUTF8StringEnumerator** aResult,
                           const nsTArray<nsCString>* aArray)
{
    if (NS_WARN_IF(!aResult) || NS_WARN_IF(!aArray))
        return NS_ERROR_INVALID_ARG;
    
    *aResult = new nsStringEnumeratorExport(aArray, false);
    return StringEnumeratorTail(aResult);
}

#define MOZILLA_INTERNAL_API
#endif /* MOZILLA_INTERNAL_API */
