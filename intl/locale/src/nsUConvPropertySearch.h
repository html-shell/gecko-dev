/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef nsUConvPropertySearch_h_
#define nsUConvPropertySearch_h_

#include <string>
#include "nsStringGlue.h"
using std::string;

class nsUConvPropertySearch
{
  public:
    /**
     * Looks up a property by value.
     *
     * @param aProperties
     *   the static property array
     * @param aKey
     *   the key to look up
     * @param aValue
     *   the return value (empty string if not found)
     * @return NS_OK if found or NS_ERROR_FAILURE if not found
     */
    static nsresult SearchPropertyValue(const char* aProperties[][3],
                                        int32_t aNumberOfProperties,
                                        const nsACString& aKey,
                                        nsACString& aValue);

    static nsresult SearchPropertyValue(const char* aProperties[][3],
                                        int32_t aNumberOfProperties,
                                        const string& aKey,
                                        string& aValue);
};

#endif /* nsUConvPropertySearch_h_ */
