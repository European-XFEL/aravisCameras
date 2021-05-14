/*
 * Author: <parenti>
 *
 * Created on September, 2019, 10:46 AM
 *
 * Copyright (c) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_ARAVISCAMERA_HH
#define KARABO_ARAVISCAMERA_HH

#include <unordered_map>

extern "C" {
  #include <arv.h>
}

#include <karabo/karabo.hpp>

#include <image_source/CameraImageSource.hh>

#include "version.hh"  // provides ARAVISCAMERAS_PACKAGE_VERSION

/**
 * The main Karabo namespace
 */
namespace karabo {

    class AravisCamera : public CameraImageSource {

    public:

        // Add reflection information and Karabo framework compatibility to this class
        KARABO_CLASSINFO(AravisCamera, "AravisCamera", ARAVISCAMERAS_PACKAGE_VERSION)

        /**
         * Necessary method as part of the factory/configuration system
         * @param expected Will contain a description of expected parameters for this device
         */
        static void expectedParameters(karabo::util::Schema& expected);

        /**
         * Constructor providing the initial configuration in form of a Hash object.
         * If this class is constructed using the configuration system the Hash object will
         * already be validated using the information of the expectedParameters function.
         * The configuration is provided in a key/value fashion.
         */
        AravisCamera(const karabo::util::Hash& config);

        /**
         * The destructor will be called in case the device gets killed
         */
        virtual ~AravisCamera();

        /**
         * This function acts as a hook and is called after an reconfiguration request was received,
         * but BEFORE this reconfiguration request is actually merged into this device's state.
         *
         * The reconfiguration information is contained in the Hash object provided as an argument.
         * You have a chance to change the content of this Hash before it is merged into the device's current state.
         *
         * NOTE: (a) The incomingReconfiguration was validated before
         *       (b) If you do not need to handle the reconfigured data, there is no need to implement this function.
         *           The reconfiguration will automatically be applied to the current state.
         * @param incomingReconfiguration The reconfiguration information as was triggered externally
         */
        virtual void preReconfigure(karabo::util::Hash& incomingReconfiguration);


        /**
         * This function acts as a hook and is called after an reconfiguration request was received,
         * and AFTER this reconfiguration request got merged into this device's current state.
         * You may access any (updated or not) parameters using the usual getters and setters.
         * @code
         * int i = get<int>("myParam");
         * @endcode
         */
        virtual void postReconfigure();

    protected:
        bool m_arv_camera_trigger; // Use arv_camera to access trigger

        ArvCamera* m_camera;
        ArvChunkParser* m_parser;

        void configure(karabo::util::Hash& configuration);

        virtual bool synchronize_timestamp();
        virtual bool configure_timestamp_chunk();
        bool m_chunk_mode;
        karabo::util::Timestamp m_reference_karabo_time;

        virtual bool get_shape_and_format(ArvBuffer* buffer, gint& width, gint& height, ArvPixelFormat& format) const;
        virtual bool get_timestamp(ArvBuffer* buffer, karabo::util::Timestamp& ts) const;

    private:
        void initialize();

        bool m_connect;
        boost::asio::deadline_timer m_reconnect_timer;
        unsigned short m_failed_connections;
        void connect(const boost::system::error_code& ec);
        void connection_failed_helper(const std::string& message, const std::string& detailed_msg="");

        bool set_auto_packet_size();
        bool set_region(int x, int y, int width, int height);
        bool set_binning(int bin_x, int bin_y);
        bool set_exposure_time(double exposure_time);
        bool set_frame_rate(bool enable, double frame_rate=0.);
        bool set_gain(double gain);
        bool set_frame_count(gint64 frame_count);

        boost::asio::deadline_timer m_poll_timer;

        void acquire();
        void acquire_failed_helper(const std::string& detailed_msg);
        void stop();
        virtual void trigger();
        void refresh();
        void reset();

        void getPathsByTag(std::vector<std::string >& paths, const std::string& tags);

        bool isFeatureAvailable(const std::string& feature);
        void disableElement(const std::string& key, const std::string& feature, karabo::util::Schema& schemaUpdate);

        bool getBoolFeature(const std::string& feature, bool& value);
        bool getStringFeature(const std::string& feature, std::string& value);
        bool getIntFeature(const std::string& feature, long long& value);
        bool getFloatFeature(const std::string& feature, double& value);

        bool setBoolFeature(const std::string& feature, bool& value);
        bool setStringFeature(const std::string& feature, std::string& value);
        bool setIntFeature(const std::string& feature, long long& value);
        bool setFloatFeature(const std::string& feature, double& value);

        void clear_camera();
        void clear_stream();

        static void stream_cb(void* context, ArvStreamCallbackType type, ArvBuffer* buffer);
        static void new_buffer_cb(ArvStream* stream, void* context);
        static void control_lost_cb(ArvGvDevice* gv_device, void* context);

        void pollOnce(karabo::util::Hash& h);
        void pollCamera(const boost::system::error_code & ec);
        void pollGenicamFeatures(const std::vector<std::string>& paths, karabo::util::Hash& h);
        bool updateOutputSchema();
        template <class T>
        void writeOutputChannels(const void* data, gint width, gint height, const karabo::util::Timestamp& timestamp);

        bool resolveHostname(const std::string& hostname, std::string& ip_address, std::string& message);

        ArvDevice* m_device;
        ArvStream* m_stream;

        bool m_is_binning_available;
        bool m_is_exposure_time_available;
        bool m_is_frame_rate_available;
        bool m_is_gain_available;
        bool m_is_gain_auto_available;

        std::unordered_map<ArvPixelFormat, std::string> m_pixelFormatOptions;

        karabo::util::Epochstamp m_timer;
        unsigned long m_counter;

        karabo::xms::EncodingType m_encoding;
    };
}

#endif
