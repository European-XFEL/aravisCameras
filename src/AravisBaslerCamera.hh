/*
 * Author: <parenti>
 *
 * Created on October 20, 2020,  4:34 PM
 *
 * Copyright (c) European XFEL GmbH Schenefeld. All rights reserved.
 */

#ifndef KARABO_ARAVISBASLERCAMERA_HH
#define KARABO_ARAVISBASLERCAMERA_HH

#include <karabo/karabo.hpp>

#include "AravisBaslerBase.hh"
#include "version.hh" // provides ARAVISCAMERAS_PACKAGE_VERSION


/**
 * The main Karabo namespace
 */
namespace karabo {

    class AravisBaslerCamera final : public AravisBaslerBase {
       public:
        KARABO_CLASSINFO(AravisBaslerCamera, "AravisBaslerCamera", ARAVISCAMERAS_PACKAGE_VERSION)

        static void expectedParameters(karabo::util::Schema& expected);

        explicit AravisBaslerCamera(const karabo::util::Hash& config);

        virtual ~AravisBaslerCamera() = default;

        bool synchronize_timestamp() override;

        bool configure_timestamp_chunk() override;

        bool get_timestamp(ArvBuffer* buffer, karabo::util::Timestamp& ts) override;

       private:
        karabo::util::Epochstamp m_last_clock_reset;
    };

} // namespace karabo

#endif // KARABO_ARAVISBASLERCAMERA_HH
