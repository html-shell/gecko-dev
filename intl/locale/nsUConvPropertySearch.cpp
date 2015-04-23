/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsUConvPropertySearch.h"
#include "nsCRT.h"
#include "mozilla/BinarySearch.h"

namespace {

struct PropertyComparator
{
  const nsCString& mKey;
  explicit PropertyComparator(const nsCString& aKey) : mKey(aKey) {}
  int operator()(const char* (&aProperty)[3]) const {
    return mKey.Compare(aProperty[0]);
  }
};

}

// static
nsresult
nsUConvPropertySearch::SearchPropertyValue(const char* aProperties[][3],
                                           int32_t aNumberOfProperties,
                                           const nsACString& aKey,
                                           nsACString& aValue)
{
  using mozilla::BinarySearchIf;

  const nsCString& flat = PromiseFlatCString(aKey);
  size_t index;
  if (BinarySearchIf(aProperties, 0, aNumberOfProperties,
                     PropertyComparator(flat), &index)) {
    nsDependentCString val(aProperties[index][1],
                           NS_PTR_TO_UINT32(aProperties[index][2]));
    aValue.Assign(val);
    return NS_OK;
  }

  aValue.Truncate();
  return NS_ERROR_FAILURE;
}

// static
nsresult
nsUConvPropertySearch::SearchPropertyValue(const char* aProperties[][3],
                                           int32_t aNumberOfProperties,
                                           const char* aKey,
                                           char** aValue)
{
  nsresult rv = NS_ERROR_NULL_POINTER;
  if (aKey && aValue) {
    rv = NS_OK;
  }
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoCString value;
  rv = SearchPropertyValue(aProperties, aNumberOfProperties, nsCString(aKey), value);
  NS_ENSURE_SUCCESS(rv, rv);

  uint32_t length = value.Length();
  *aValue = (char *)NS_Alloc(length + 1);
  if (!(*aValue)) rv = NS_ERROR_OUT_OF_MEMORY;
  NS_ENSURE_SUCCESS(rv, rv);

  if (length > 0)
    memcpy(*aValue, value.get(), length);
  (*aValue)[length] = '\0';

  return NS_OK;
}