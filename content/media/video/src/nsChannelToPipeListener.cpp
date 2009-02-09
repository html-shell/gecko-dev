/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla code.
 *
 * The Initial Developer of the Original Code is the Mozilla Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2007
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Chris Double <chris.double@double.co.nz>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
#include "nsAString.h"
#include "nsNetUtil.h"
#include "nsMediaDecoder.h"
#include "nsIScriptSecurityManager.h"
#include "nsChannelToPipeListener.h"
#include "nsICachingChannel.h"
#include "nsDOMError.h"
#include "nsHTMLMediaElement.h"

#define HTTP_OK_CODE 200
#define HTTP_PARTIAL_RESPONSE_CODE 206

nsChannelToPipeListener::nsChannelToPipeListener(
    nsMediaDecoder* aDecoder,
    PRBool aSeeking,
    PRInt64 aOffset) :
  mDecoder(aDecoder),
  mIntervalStart(0),
  mIntervalEnd(0),
  mOffset(aOffset),
  mTotalBytes(0),
  mSeeking(aSeeking)
{
}

nsresult nsChannelToPipeListener::Init() 
{
  nsresult rv = NS_NewPipe(getter_AddRefs(mInput), 
                           getter_AddRefs(mOutput),
                           0, 
                           PR_UINT32_MAX);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

void nsChannelToPipeListener::Stop()
{
  mDecoder = nsnull;
  mInput = nsnull;
  mOutput = nsnull;
}

void nsChannelToPipeListener::Cancel()
{
  if (mOutput)
    mOutput->Close();

  if (mInput)
    mInput->Close();
}

double nsChannelToPipeListener::BytesPerSecond() const
{
  return mOutput ? mTotalBytes / ((PR_IntervalToMilliseconds(mIntervalEnd-mIntervalStart)) / 1000.0) : NS_MEDIA_UNKNOWN_RATE;
}

nsresult nsChannelToPipeListener::GetInputStream(nsIInputStream** aStream)
{
  NS_IF_ADDREF(*aStream = mInput);
  return NS_OK;
}

nsresult nsChannelToPipeListener::OnStartRequest(nsIRequest* aRequest, nsISupports* aContext)
{
  nsHTMLMediaElement* element = mDecoder->GetMediaElement();
  NS_ENSURE_TRUE(element, NS_ERROR_FAILURE);
  if (element->ShouldCheckAllowOrigin()) {
    // If the request was cancelled by nsCrossSiteListenerProxy due to failing
    // the Access Control check, send an error through to the media element.
    nsresult status;
    nsresult rv = aRequest->GetStatus(&status);
    if (NS_FAILED(rv) || status == NS_ERROR_DOM_BAD_URI) {
      mDecoder->NetworkError();
      return NS_ERROR_DOM_BAD_URI;
    }
  }

  mIntervalStart = PR_IntervalNow();
  mIntervalEnd = mIntervalStart;
  mTotalBytes = 0;
  mDecoder->UpdateBytesDownloaded(mOffset);
  nsCOMPtr<nsIHttpChannel> hc = do_QueryInterface(aRequest);
  if (hc) {
    nsCAutoString ranges;
    nsresult rv = hc->GetResponseHeader(NS_LITERAL_CSTRING("Accept-Ranges"),
                                        ranges);
    PRBool acceptsRanges = ranges.EqualsLiteral("bytes"); 

    if (!mSeeking) {
      // Look for duration headers from known Ogg content systems. In the case
      // of multiple options for obtaining the duration the order of precedence is;
      // 1) The Media resource metadata if possible (done by the decoder itself).
      // 2) X-Content-Duration.
      // 3) x-amz-meta-content-duration.
      // 4) Perform a seek in the decoder to find the value.
      nsCAutoString durationText;
      PRInt32 ec = 0;
      nsresult rv = hc->GetResponseHeader(NS_LITERAL_CSTRING("X-Content-Duration"), durationText);
      if (NS_FAILED(rv)) {
        rv = hc->GetResponseHeader(NS_LITERAL_CSTRING("X-AMZ-Meta-Content-Duration"), durationText);
      }

      if (NS_SUCCEEDED(rv)) {
        float duration = durationText.ToFloat(&ec);
        if (ec == NS_OK && duration >= 0) {
          mDecoder->SetDuration(PRInt64(NS_round(duration*1000)));
        }
      }
    }
 
    PRUint32 responseStatus = 0; 
    hc->GetResponseStatus(&responseStatus);
    if (mSeeking && responseStatus == HTTP_OK_CODE) {
      // If we get an OK response but we were seeking, and therefore
      // expecting a partial response of HTTP_PARTIAL_RESPONSE_CODE,
      // seeking should still be possible if the server is sending
      // Accept-Ranges:bytes.
      mDecoder->SetSeekable(acceptsRanges);
    }
    else if (!mSeeking && 
             (responseStatus == HTTP_OK_CODE ||
              responseStatus == HTTP_PARTIAL_RESPONSE_CODE)) {
      // We weren't seeking and got a valid response status,
      // set the length of the content.
      PRInt32 cl = 0;
      hc->GetContentLength(&cl);
      mDecoder->SetTotalBytes(cl);

      // If we get an HTTP_OK_CODE response to our byte range request,
      // and the server isn't sending Accept-Ranges:bytes then we don't
      // support seeking.
      mDecoder->SetSeekable(responseStatus == HTTP_PARTIAL_RESPONSE_CODE ||
                            acceptsRanges);
    }
  }

  nsCOMPtr<nsICachingChannel> cc = do_QueryInterface(aRequest);
  if (cc) {
    PRBool fromCache = PR_FALSE;
    nsresult rv = cc->IsFromCache(&fromCache);
    if (NS_SUCCEEDED(rv) && !fromCache) {
      cc->SetCacheAsFile(PR_TRUE);
    }
  }

  /* Get our principal */
  nsCOMPtr<nsIChannel> chan(do_QueryInterface(aRequest));
  if (chan) {
    nsCOMPtr<nsIScriptSecurityManager> secMan =
      do_GetService("@mozilla.org/scriptsecuritymanager;1");
    if (secMan) {
      nsresult rv = secMan->GetChannelPrincipal(chan,
                                                getter_AddRefs(mPrincipal));
      if (NS_FAILED(rv)) {
        return rv;
      }
    }
  }

  // Fires an initial progress event and sets up the stall counter so stall events
  // fire if no download occurs within the required time frame.
  mDecoder->Progress(PR_FALSE);

  return NS_OK;
}

nsresult nsChannelToPipeListener::OnStopRequest(nsIRequest* aRequest, nsISupports* aContext, nsresult aStatus) 
{
  mOutput = nsnull;
  if (aStatus != NS_BINDING_ABORTED && mDecoder) {
    if (NS_SUCCEEDED(aStatus)) {
      mDecoder->ResourceLoaded();
    } else {
      mDecoder->NetworkError();
    }
  }
  return NS_OK;
}

nsresult nsChannelToPipeListener::OnDataAvailable(nsIRequest* aRequest, 
                                                nsISupports* aContext, 
                                                nsIInputStream* aStream,
                                                PRUint32 aOffset,
                                                PRUint32 aCount)
{
  if (!mOutput)
    return NS_ERROR_FAILURE;

  PRUint32 bytes = 0;
  
  do {
    nsresult rv = mOutput->WriteFrom(aStream, aCount, &bytes);
    NS_ENSURE_SUCCESS(rv, rv);
    
    aCount -= bytes;
    mTotalBytes += bytes;
    mDecoder->UpdateBytesDownloaded(mOffset + aOffset + bytes);
  } while (aCount) ;
  
  nsresult rv = mOutput->Flush();
  NS_ENSURE_SUCCESS(rv, rv);

  // Fire a progress events according to the time and byte constraints outlined
  // in the spec.
  mDecoder->Progress(PR_FALSE);

  mIntervalEnd = PR_IntervalNow();
  return NS_OK;
}

nsIPrincipal*
nsChannelToPipeListener::GetCurrentPrincipal()
{
  return mPrincipal;
}

NS_IMPL_THREADSAFE_ISUPPORTS2(nsChannelToPipeListener, nsIRequestObserver, nsIStreamListener)

