/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifdef MOZILLA_INTERNAL_API
#undef MOZILLA_INTERNAL_API

#define nsSupportsIDImpl nsSupportsIDImplExport
#define nsSupportsCStringImpl nsSupportsCStringImplExport
#define nsSupportsStringImpl nsSupportsStringImplExport
#define nsSupportsPRBoolImpl nsSupportsPRBoolImplExport
#define nsSupportsPRUint8Impl nsSupportsPRUint8ImplExport
#define nsSupportsPRUint16Impl nsSupportsPRUint16ImplExport
#define nsSupportsPRUint32Impl nsSupportsPRUint32ImplExport
#define nsSupportsPRUint64Impl nsSupportsPRUint64ImplExport
#define nsSupportsPRTimeImpl nsSupportsPRTimeImplExport
#define nsSupportsCharImpl nsSupportsCharImplExport
#define nsSupportsPRInt16Impl nsSupportsPRInt16ImplExport
#define nsSupportsPRInt32Impl nsSupportsPRInt32ImplExport
#define nsSupportsPRInt64Impl nsSupportsPRInt64ImplExport
#define nsSupportsFloatImpl nsSupportsFloatImplExport
#define nsSupportsDoubleImpl nsSupportsDoubleImplExport
#define nsSupportsVoidImpl nsSupportsVoidImplExport
#define nsSupportsInterfacePointerImpl nsSupportsInterfacePointerImplExport
#define nsSupportsDependentCString nsSupportsDependentCStringExport
#include "nsSupportsPrimitives.cpp"

#define nsStringEnumerator nsStringEnumeratorExport
#include "nsStringEnumerator.cpp"

#define MOZILLA_INTERNAL_API
#endif /* MOZILLA_INTERNAL_API */
