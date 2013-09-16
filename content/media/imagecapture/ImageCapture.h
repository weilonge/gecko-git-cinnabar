/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef IMAGECAPTURE_H
#define IMAGECAPTURE_H

#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/dom/ImageCaptureBinding.h"
#include "prlog.h"

class nsIDOMBlob;

namespace mozilla {

#ifdef PR_LOGGING

#ifndef IC_LOG
PRLogModuleInfo* GetICLog();
#define IC_LOG(...) PR_LOG(GetICLog(), PR_LOG_DEBUG, (__VA_ARGS__))
#endif

#else

#ifndef IC_LOG
#define IC_LOG(...)
#endif

#endif // PR_LOGGING

namespace dom {

class VideoStreamTrack;

/**
 *  Implementation of https://dvcs.w3.org/hg/dap/raw-file/default/media-stream-
 *  capture/ImageCapture.html.
 *  The ImageCapture accepts a VideoStreamTrack as input source. The image will
 *  be sent back as a JPG format via Blob event.
 *
 *  All the functions in ImageCapture are run in main thread.
 */

class ImageCapture MOZ_FINAL : public DOMEventTargetHelper
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(ImageCapture, DOMEventTargetHelper)

  IMPL_EVENT_HANDLER(photo)
  IMPL_EVENT_HANDLER(error)

  // WebIDL members.
  void TakePhoto(ErrorResult& aResult);

  // The MediaStream passed into the constructor.
  VideoStreamTrack* GetVideoStreamTrack() const;

  // nsWrapperCache member
  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE
  {
    return ImageCaptureBinding::Wrap(aCx, this);
  }

  // ImageCapture class members
  nsPIDOMWindow* GetParentObject() { return GetOwner(); }

  static already_AddRefed<ImageCapture> Constructor(const GlobalObject& aGlobal,
                                                    VideoStreamTrack& aTrack,
                                                    ErrorResult& aRv);

  ImageCapture(VideoStreamTrack* aVideoStreamTrack, nsPIDOMWindow* aOwnerWindow);

  // Post a Blob event to script.
  nsresult PostBlobEvent(nsIDOMBlob* aBlob);

  // Post an error event to script.
  // aErrorCode should be one of error codes defined in ImageCaptureError.h.
  // aReason is the nsresult which maps to a error string in dom/base/domerr.msg.
  nsresult PostErrorEvent(uint16_t aErrorCode, nsresult aReason = NS_OK);

  bool CheckPrincipal();

protected:
  virtual ~ImageCapture();

  nsRefPtr<VideoStreamTrack> mVideoStreamTrack;
};

} // namespace dom
} // namespace mozilla

#endif // IMAGECAPTURE_H
