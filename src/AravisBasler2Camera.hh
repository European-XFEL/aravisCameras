/*
 * Author: <parenti>
 *
 * Created on April 27, 2021,  6:00 PM
 *
 * Copyright (c) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_ARAVISBASLER2CAMERA_HH
#define KARABO_ARAVISBASLER2CAMERA_HH

#include <karabo/karabo.hpp>

#include "AravisBaslerBase.hh"
#include "version.hh"  // provides ARAVISCAMERAS_PACKAGE_VERSION

/**
 * The main Karabo namespace
 */
namespace karabo {

    class AravisBasler2Camera final : public AravisBaslerBase {
    public:

        KARABO_CLASSINFO(AravisBasler2Camera, "AravisBasler2Camera", ARAVISCAMERAS_PACKAGE_VERSION)

        static void expectedParameters(karabo::util::Schema& expected);

        explicit AravisBasler2Camera(const karabo::util::Hash& config);

        virtual ~AravisBasler2Camera() = default;

        bool get_shape_and_format(ArvBuffer* buffer, gint& width, gint& height, ArvPixelFormat& format) const override;

        bool synchronize_timestamp() override;

        bool configure_timestamp_chunk() override;

        bool get_timestamp(ArvBuffer* buffer, karabo::util::Timestamp& ts) override;

    private:
        void resetCamera() override;
        bool set_exposure_time(double exposure_time) override;
    };

} // namespace karabo

#endif // KARABO_ARAVISBASLER2CAMERA_HH
