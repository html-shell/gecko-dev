/* -*- Mode: C++; tab-width: 20; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef GFX_PLATFORM_MAC_H
#define GFX_PLATFORM_MAC_H

#include "nsTArrayForwardDeclare.h"
#include "gfxPlatform.h"

#define MAC_OS_X_VERSION_10_6_HEX 0x00001060
#define MAC_OS_X_VERSION_10_7_HEX 0x00001070

#define MAC_OS_X_MAJOR_VERSION_MASK 0xFFFFFFF0U

namespace mozilla { namespace gfx { class DrawTarget; }}

class gfxPlatformMac : public gfxPlatform {
public:
    gfxPlatformMac();
    virtual ~gfxPlatformMac();

    static gfxPlatformMac *GetPlatform() {
        return (gfxPlatformMac*) gfxPlatform::GetPlatform();
    }

    virtual already_AddRefed<gfxASurface>
      CreateOffscreenSurface(const IntSize& size,
                             gfxContentType contentType) MOZ_OVERRIDE;

    mozilla::TemporaryRef<mozilla::gfx::ScaledFont>
      GetScaledFontForFont(mozilla::gfx::DrawTarget* aTarget, gfxFont *aFont);

    nsresult GetStandardFamilyName(const nsAString& aFontName, nsAString& aFamilyName);

    gfxFontGroup*
    CreateFontGroup(const mozilla::FontFamilyList& aFontFamilyList,
                    const gfxFontStyle *aStyle,
                    gfxUserFontSet *aUserFontSet);

    virtual gfxFontEntry* LookupLocalFont(const nsAString& aFontName,
                                          uint16_t aWeight,
                                          int16_t aStretch,
                                          bool aItalic);

    virtual gfxPlatformFontList* CreatePlatformFontList();

    virtual gfxFontEntry* MakePlatformFont(const nsAString& aFontName,
                                           uint16_t aWeight,
                                           int16_t aStretch,
                                           bool aItalic,
                                           const uint8_t* aFontData,
                                           uint32_t aLength);

    bool IsFontFormatSupported(nsIURI *aFontURI, uint32_t aFormatFlags);

    nsresult GetFontList(nsIAtom *aLangGroup,
                         const nsACString& aGenericFamily,
                         nsTArray<nsString>& aListOfFonts);
    nsresult UpdateFontList();

    virtual void GetCommonFallbackFonts(uint32_t aCh, uint32_t aNextCh,
                                        int32_t aRunScript,
                                        nsTArray<const char*>& aFontList);

    virtual bool CanRenderContentToDataSurface() const MOZ_OVERRIDE {
      return true;
    }

    bool UseAcceleratedCanvas();

    virtual bool UseTiling() MOZ_OVERRIDE;
    virtual bool UseProgressivePaint() MOZ_OVERRIDE;
    virtual void InitHardwareVsync() MOZ_OVERRIDE;

    // lower threshold on font anti-aliasing
    uint32_t GetAntiAliasingThreshold() { return mFontAntiAliasingThreshold; }

private:
    virtual void GetPlatformCMSOutputProfile(void* &mem, size_t &size);

    // read in the pref value for the lower threshold on font anti-aliasing
    static uint32_t ReadAntiAliasingThreshold();

    uint32_t mFontAntiAliasingThreshold;
};

#endif /* GFX_PLATFORM_MAC_H */
