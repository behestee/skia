/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkColorSpace_Base_DEFINED
#define SkColorSpace_Base_DEFINED

#include "SkColorLookUpTable.h"
#include "SkColorSpace.h"
#include "SkData.h"
#include "SkOnce.h"
#include "SkTemplates.h"

struct SkGammas : SkRefCnt {

    // There are four possible representations for gamma curves.  kNone_Type is used
    // as a placeholder until the struct is initialized.  It is not a valid value.
    enum class Type : uint8_t {
        kNone_Type,
        kNamed_Type,
        kValue_Type,
        kTable_Type,
        kParam_Type,
    };

    // Contains information for a gamma table.
    struct Table {
        size_t fOffset;
        int    fSize;

        const float* table(const SkGammas* base) const {
            return SkTAddOffset<const float>(base, sizeof(SkGammas) + fOffset);
        }
    };

    // Contains the actual gamma curve information.  Should be interpreted
    // based on the type of the gamma curve.
    union Data {
        Data()
            : fTable{ 0, 0 }
        {}

        inline bool operator==(const Data& that) const {
            return this->fTable.fOffset == that.fTable.fOffset &&
                   this->fTable.fSize == that.fTable.fSize;
        }

        inline bool operator!=(const Data& that) const {
            return !(*this == that);
        }

        SkGammaNamed             fNamed;
        float                    fValue;
        Table                    fTable;
        size_t                   fParamOffset;

        const SkColorSpaceTransferFn& params(const SkGammas* base) const {
            return *SkTAddOffset<const SkColorSpaceTransferFn>(
                    base, sizeof(SkGammas) + fParamOffset);
        }
    };

    bool isNamed(int i) const {
        return Type::kNamed_Type == this->type(i);
    }

    bool isValue(int i) const {
        return Type::kValue_Type == this->type(i);
    }

    bool isTable(int i) const {
        return Type::kTable_Type == this->type(i);
    }

    bool isParametric(int i) const {
        return Type::kParam_Type == this->type(i);
    }

    const Data& data(int i) const {
        SkASSERT(i >= 0 && i < fChannels);
        return fData[i];
    }

    const float* table(int i) const {
        SkASSERT(isTable(i));
        return this->data(i).fTable.table(this);
    }

    int tableSize(int i) const {
        SkASSERT(isTable(i));
        return this->data(i).fTable.fSize;
    }

    const SkColorSpaceTransferFn& params(int i) const {
        SkASSERT(isParametric(i));
        return this->data(i).params(this);
    }

    Type type(int i) const {
        SkASSERT(i >= 0 && i < fChannels);
        return fType[i];
    }

    uint8_t channels() const { return fChannels; }

    SkGammas(uint8_t channels)
        : fChannels(channels) {
        SkASSERT(channels <= kMaxColorChannels);
        for (uint8_t i = 0; i < kMaxColorChannels; ++i) {
            fType[i] = Type::kNone_Type;
        }
    }

    // These fields should only be modified when initializing the struct.
    uint8_t fChannels;
    Data    fData[kMaxColorChannels];
    Type    fType[kMaxColorChannels];

    // Objects of this type are sometimes created in a custom fashion using
    // sk_malloc_throw and therefore must be sk_freed.  We overload new to
    // also call sk_malloc_throw so that memory can be unconditionally released
    // using sk_free in an overloaded delete. Overloading regular new means we
    // must also overload placement new.
    void* operator new(size_t size) { return sk_malloc_throw(size); }
    void* operator new(size_t, void* p) { return p; }
    void operator delete(void* p) { sk_free(p); }
};

class SkColorSpace_Base : public SkColorSpace {
public:

    enum class Type : uint8_t {
        kXYZ,
        kA2B
    };

    virtual Type type() const = 0;

    static sk_sp<SkColorSpace> MakeRGB(SkGammaNamed gammaNamed, const SkMatrix44& toXYZD50);

    enum Named : uint8_t {
        kSRGB_Named,
        kAdobeRGB_Named,
        kSRGBLinear_Named,
        kSRGB_NonLinearBlending_Named,
    };

    static sk_sp<SkColorSpace> MakeNamed(Named);

protected:
    SkColorSpace_Base(sk_sp<SkData> profileData);

private:
    sk_sp<SkData> fProfileData;

    friend class SkColorSpace;
    friend class SkColorSpace_XYZ;
    friend class ColorSpaceXformTest;
    friend class ColorSpaceTest;
    typedef SkColorSpace INHERITED;
};

static inline SkColorSpace_Base* as_CSB(SkColorSpace* colorSpace) {
    return static_cast<SkColorSpace_Base*>(colorSpace);
}

static inline const SkColorSpace_Base* as_CSB(const SkColorSpace* colorSpace) {
    return static_cast<const SkColorSpace_Base*>(colorSpace);
}

static inline SkColorSpace_Base* as_CSB(const sk_sp<SkColorSpace>& colorSpace) {
    return static_cast<SkColorSpace_Base*>(colorSpace.get());
}

#endif
