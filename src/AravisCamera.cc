/*
 * Author: <parenti>
 *
 * Created on September, 2019, 10:46 AM
 *
 * Copyright (c) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <boost/algorithm/string/trim.hpp>

#include "AravisCamera.hh"

using namespace std;

USING_KARABO_NAMESPACES;

#define GET_PATH(hash, path, type) hash.has(path) ? hash.get<type>(path) : this->get<type>(path);

namespace karabo {


    KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<>, ImageSource, CameraImageSource, AravisCamera)

    void AravisCamera::expectedParameters(Schema& expected) {
        OVERWRITE_ELEMENT(expected).key("state")
                .setNewOptions(State::UNKNOWN, State::ERROR, State::ON, State::ACQUIRING)
                .commit();

        STRING_ELEMENT(expected).key("idType")
                .displayedName("ID Type")
                .description("The type of identifier to be used, to connect to the camera."
                "Available options are 'IP' (IP address), 'HOST' (IP name), SN (Serial Number), MAC (MAC address).")
                .assignmentOptional().defaultValue("IP")
                .options("IP,HOST,SN,MAC")
                .init()
                .commit();

        STRING_ELEMENT(expected).key("cameraId")
                .displayedName("Camera ID")
                .description("The 'identifier' of the network camera. It can be an IP address (e.g. 192.168.1.153), "
                "an IP name (e.g. exflqr1234), a serial number or a MAC address (e.g. 00:30:53:25:ab:b7). "
                "The type must be specified in the 'idType' property.")
                .assignmentMandatory()
                .init() // cannot be changed after device instantiation
                .commit();

        INT64_ELEMENT(expected).key("packetDelay")
                .displayedName("Packet Delay")
                .description("Configure the inter packet delay to insert between each packet for the current stream. "
                "This can be used as a crude flow-control mechanism if the application or the network "
                "infrastructure cannot keep up with the packets coming from the device.")
                .assignmentOptional().noDefaultValue()
                .minInc(0)
                .unit(Unit::SECOND).metricPrefix(MetricPrefix::NANO)
                .reconfigurable()
                .allowedStates(State::UNKNOWN, State::ON)
                .commit();

        BOOL_ELEMENT(expected).key("autoPacketSize")
                .displayedName("Auto Packet Size")
                .description("Automatically determine the biggest packet size that can be used for data streaming, "
                "and set its value accordingly. If this functionality is not available, the packet size will be set "
                "to a default value (1500 bytes).")
                .assignmentOptional().defaultValue(true)
                .reconfigurable()
                .allowedStates(State::UNKNOWN, State::ON)
                .commit();

        INT32_ELEMENT(expected).key("packetSize")
                .displayedName("Packet Size")
                .description("Specifies the packet size to be used by the camera for data streaming. "
                "This does not include data leader and data trailer and the last data packet which might be "
                "of smaller size.")
                .assignmentOptional().noDefaultValue()
                .minExc(0)
                .unit(Unit::BYTE)
                .reconfigurable()
                .allowedStates(State::UNKNOWN, State::ON)
                .commit();

        SLOT_ELEMENT(expected).key("acquire")
                .displayedName("Acquire")
                .allowedStates(State::ON)
                .commit();

        SLOT_ELEMENT(expected).key("stop")
                .displayedName("Stop")
                .allowedStates(State::ACQUIRING)
                .commit();

        SLOT_ELEMENT(expected).key("trigger")
                .displayedName("Software Trigger")
                .allowedStates(State::ACQUIRING)
                .commit();

        SLOT_ELEMENT(expected).key("refresh")
                .displayedName("Refresh")
                .description("Refresh hardware parameters and options.")
                .allowedStates(State::ON)
                .commit();

        SLOT_ELEMENT(expected).key("reset")
                .displayedName("Reset")
                .description("Reset error state.")
                .allowedStates(State::ERROR)
                .commit();

        NODE_ELEMENT(expected).key("frameRate")
                .displayedName("Frame Rate")
                .commit();

        BOOL_ELEMENT(expected).key("frameRate.enable")
                .displayedName("Frame Rate Enable")
                .description("Enable the frame rate control.")
                .assignmentOptional().defaultValue(false)
                .reconfigurable()
                .allowedStates(State::UNKNOWN, State::ON)
                .commit();

        FLOAT_ELEMENT(expected).key("frameRate.target")
                .displayedName("Target Frame Rate")
                .description("Sets the 'absolute' value of the acquisition frame rate on the camera. "
                "The 'absolute' value is a float value that sets the acquisition frame rate in frames per second.")
                .assignmentOptional().defaultValue(10.)
                .minInc(0.)
                .unit(Unit::HERTZ)
                .reconfigurable()
                .allowedStates(State::UNKNOWN, State::ON)
                .commit();

        FLOAT_ELEMENT(expected).key("frameRate.actual")
                .displayedName("Actual Frame Rate")
                .description("The measured frame rate.")
                .unit(Unit::HERTZ)
                .readOnly()
                .initialValue(0.)
                .commit();

        INT32_ELEMENT(expected).key("pollingInterval")
                .displayedName("Polling Interval")
                .description("The interval for polling the camera for read-out values.")
                .assignmentOptional().defaultValue(20)
                .unit(Unit::SECOND)
                .minInc(5).maxInc(60)
                .reconfigurable()
                .commit();

        STRING_ELEMENT(expected).key("camId")
                .displayedName("Camera ID")
                .readOnly().initialValue("")
                .commit();

        STRING_ELEMENT(expected).key("vendor")
                .displayedName("Vendor Name")
                .readOnly().initialValue("")
                .commit();

        STRING_ELEMENT(expected).key("model")
                .displayedName("Model Name")
                .readOnly().initialValue("")
                .commit();

        INT32_ELEMENT(expected).key("width")
                .displayedName("Sensor Width")
                .readOnly().initialValue(0)
                .commit();

        INT32_ELEMENT(expected).key("height")
                .displayedName("Sensor Height")
                .readOnly().initialValue(0)
                .commit();

        NODE_ELEMENT(expected).key("roi")
                .displayedName("Image ROI")
                .commit();

        INT32_ELEMENT(expected).key("roi.x")
                .displayedName("X Offset")
                .unit(Unit::PIXEL)
                .assignmentOptional().defaultValue(0)
                .minInc(0)
                .reconfigurable()
                .allowedStates(State::UNKNOWN, State::ON)
                .commit();

        INT32_ELEMENT(expected).key("roi.y")
                .displayedName("Y Offset")
                .unit(Unit::PIXEL)
                .assignmentOptional().defaultValue(0)
                .minInc(0)
                .reconfigurable()
                .allowedStates(State::UNKNOWN, State::ON)
                .commit();

        INT32_ELEMENT(expected).key("roi.width")
                .displayedName("Width")
                .description("The ROI width. Use '0' for the whole sensor width.")
                .unit(Unit::PIXEL)
                .assignmentOptional().defaultValue(0)
                .minInc(0)
                .reconfigurable()
                .allowedStates(State::UNKNOWN, State::ON)
                .commit();

        INT32_ELEMENT(expected).key("roi.height")
                .displayedName("Height")
                .description("The ROI height. Use '0' for the whole sensor height.")
                .unit(Unit::PIXEL)
                .assignmentOptional().defaultValue(0)
                .minInc(0)
                .reconfigurable()
                .allowedStates(State::UNKNOWN, State::ON)
                .commit();

        NODE_ELEMENT(expected).key("bin")
                .displayedName("Image Binning")
                .commit();

        INT32_ELEMENT(expected).key("bin.x")
                .displayedName("X Binning")
                .unit(Unit::PIXEL)
                .assignmentOptional().defaultValue(1)
                .minInc(1)
                .reconfigurable()
                .allowedStates(State::UNKNOWN, State::ON)
                .commit();

        INT32_ELEMENT(expected).key("bin.y")
                .displayedName("Y Binning")
                .unit(Unit::PIXEL)
                .assignmentOptional().defaultValue(1)
                .minInc(1)
                .reconfigurable()
                .allowedStates(State::UNKNOWN, State::ON)
                .commit();

        STRING_ELEMENT(expected).key("pixelFormat")
                .displayedName("Pixel Format")
                .description("This enumeration sets the format of the pixel data transmitted for acquired images. "
                "For example Mono8 means monochromatic, 8 bits-per-pixel.")
                .assignmentOptional().defaultValue("Mono12Packed")
                // Fill-up with some commonly available options. They will be updated on connection.
                .options("Mono8,Mono12,Mono12Packed")
                .reconfigurable()
                .allowedStates(State::UNKNOWN, State::ON)
                .commit();

        UINT16_ELEMENT(expected).key("bpp")
                .displayedName("Bits-per-pixel")
                .readOnly().initialValue(0)
                .commit();

        DOUBLE_ELEMENT(expected).key("exposureTime")
                .displayedName("Exposure Time")
                .description("This float value sets the camera's exposure time. "
                "It can only be a multiple of the minimum exposure time.")
                .unit(Unit::SECOND).metricPrefix(MetricPrefix::MICRO)
                .assignmentOptional().defaultValue(10.)
                .minExc(0.)
                .reconfigurable()
                .allowedStates(State::UNKNOWN, State::ON)
                .commit();

        STRING_ELEMENT(expected).key("triggerSelector")
                .displayedName("Trigger Selector")
                .description("This enumeration selects the trigger type to configure. "
                "Once a trigger type has been selected, all changes to the trigger settings will be applied to "
                "the selected trigger.")
                .assignmentOptional().defaultValue("FrameStart")
                // options will be injected on connection
                .reconfigurable()
                .allowedStates(State::UNKNOWN, State::ON)
                .commit();

        STRING_ELEMENT(expected).key("triggerMode")
                .displayedName("Trigger Mode")
                .description("This enumeration enables or disables the selected trigger.")
                .assignmentOptional().defaultValue("Off")
                .options("On,Off")
                .reconfigurable()
                .allowedStates(State::UNKNOWN, State::ON)
                .commit();

        STRING_ELEMENT(expected).key("triggerSource")
                .displayedName("Trigger Source")
                .description("This enumeration sets the signal source for the selected trigger.")
                .assignmentOptional().noDefaultValue()
                // options will be injected on connection
                .reconfigurable()
                .allowedStates(State::UNKNOWN, State::ON)
                .commit();

        STRING_ELEMENT(expected).key("triggerActivation")
                .displayedName("Trigger Activation")
                .description("This enumeration sets the signal transition needed to activate the selected trigger.")
                .assignmentOptional().defaultValue("RisingEdge")
                .options("RisingEdge,FallingEdge")
                .reconfigurable()
                .allowedStates(State::UNKNOWN, State::ON)
                .commit();

        STRING_ELEMENT(expected).key("autoGain")
                .displayedName("Auto Gain")
                .description("Configures automatic gain feature.")
                .assignmentOptional().defaultValue("Off")
                .options("Off,Once,Continuous")
                .reconfigurable()
                .allowedStates(State::UNKNOWN, State::ON)
                .commit();

        DOUBLE_ELEMENT(expected).key("gain")
                .displayedName("Gain")
                .description("Sets the gain of the ADC converter.")
                .assignmentOptional().noDefaultValue()
                .reconfigurable()
                .allowedStates(State::UNKNOWN, State::ON)
                .commit();

        STRING_ELEMENT(expected).key("acquisitionMode")
                .displayedName("Acquisition Mode")
                .description("This property sets the image acquisition mode.")
                .assignmentOptional().defaultValue("Continuous")
                .options("Continuous,SingleFrame,MultiFrame")
                .reconfigurable()
                .allowedStates(State::UNKNOWN, State::ON)
                .commit();

        INT64_ELEMENT(expected).key("frameCount")
                .displayedName("Frame Count")
                .description("This value sets the number of frames acquired in the 'Multiframe' acquisition mode.")
                .assignmentOptional().noDefaultValue()
                .minInc(1)
                .reconfigurable()
                .allowedStates(State::UNKNOWN, State::ON)
                .commit();

    }


    AravisCamera::AravisCamera(const karabo::util::Hash& config) : CameraImageSource(config),
            m_connect(true), m_reconnect_timer(EventLoop::getIOService()), m_failed_connections(0u),
            m_poll_timer(EventLoop::getIOService()),
            m_camera(nullptr), m_stream(nullptr),
            m_arv_camera_trigger(true), m_is_binning_available(false), m_is_exposure_time_available(false),
            m_is_frame_rate_available(false), m_is_gain_available(false), m_is_gain_auto_available(false) {
        KARABO_SLOT(acquire);
        KARABO_SLOT(stop);
        KARABO_SLOT(trigger);
        KARABO_SLOT(refresh);
        KARABO_SLOT(reset);

        KARABO_INITIAL_FUNCTION(initialize);
    }


    AravisCamera::~AravisCamera() {
        this->clear_stream();
        this->clear_camera();

        m_connect = false;
        m_reconnect_timer.cancel();
        m_poll_timer.cancel();
    }


    void AravisCamera::preReconfigure(karabo::util::Hash& incomingReconfiguration) {
        this->configure(incomingReconfiguration);
    }


    void AravisCamera::postReconfigure() {
        const bool success = this->updateOutputSchema();
        if (!success) {
            this->updateState(State::ERROR);
        }
    }


    void AravisCamera::getPathsByTag(std::vector<std::string>& paths, const std::string& tags) {
        // N.B. Device::getCurrentConfiguration(tags)) cannot be used, as it
        // does not return parameters with no value set

        const Schema schema = this->getFullSchema();
        const Hash& parameters = schema.getParameterHash();
        const Hash filteredParameters = this->filterByTags(parameters, tags);

        filteredParameters.getPaths(paths);
    }


    bool AravisCamera::isFeatureAvailable(const std::string& feature) {
        if (m_device != nullptr) {
            ArvGcNode* node = arv_device_get_feature(m_device, feature.c_str());
            if (node != nullptr) {
                return true;
            }
        }

        return false;
    }


    void AravisCamera::disableElement(const std::string& key, const std::string& feature, karabo::util::Schema& schemaUpdate) {
        STRING_ELEMENT(schemaUpdate).key(key)
            .displayedName(feature)
            .description("Not available for this camera.")
            .readOnly()
            .commit();
    }


    bool AravisCamera::getBoolFeature(const std::string& feature, bool& value) {
        GError* error = nullptr;
        value = arv_device_get_boolean_feature_value(m_device, feature.c_str(), &error);

        if (error != nullptr) {
            KARABO_LOG_FRAMEWORK_ERROR << "arv_device_get_boolean_feature_value failed: " << error->message;
            g_clear_error(&error);
            return false;
        }

        return true;
    }


    bool AravisCamera::getStringFeature(const std::string& feature, std::string& value) {
        GError* error = nullptr;
        value = arv_device_get_string_feature_value(m_device, feature.c_str(), &error);

        if (error != nullptr) {
            KARABO_LOG_FRAMEWORK_ERROR << "arv_device_get_string_feature_value failed: " << error->message;
            g_clear_error(&error);
            return false;
        }

        return true;
    }


    bool AravisCamera::getIntFeature(const std::string& feature, long long& value) {
        GError* error = nullptr;
        value = arv_device_get_integer_feature_value(m_device, feature.c_str(), &error);

        if (error != nullptr) {
            KARABO_LOG_FRAMEWORK_ERROR << "arv_device_get_integer_feature_value failed: " << error->message;
            g_clear_error(&error);
            return false;
        }

        return true;
    }


    bool AravisCamera::getFloatFeature(const std::string& feature, double& value) {
        GError* error = nullptr;
        value = arv_device_get_float_feature_value(m_device, feature.c_str(), &error);

        if (error != nullptr) {
            KARABO_LOG_FRAMEWORK_ERROR << "arv_device_get_float_feature_value failed: " << error->message;
            g_clear_error(&error);
            return false;
        }

        return true;
    }


    bool AravisCamera::setBoolFeature(const std::string& feature, bool& value) {
        GError* error = nullptr;
        arv_device_set_boolean_feature_value(m_device, feature.c_str(), value, &error);
        if (error != nullptr) {
            KARABO_LOG_FRAMEWORK_ERROR << "arv_device_set_boolean_feature_value failed: " << error->message;
            g_clear_error(&error);
        } else {
            return true; // success
        }

        // read back value
        const bool rvalue = arv_device_get_boolean_feature_value(m_device, feature.c_str(), &error);
        if (error != nullptr) { // Could not read back value
            KARABO_LOG_FRAMEWORK_ERROR << "arv_device_get_boolean_feature_value failed: " << error->message;
            g_clear_error(&error);
            return false;
        } else if (rvalue != value) { // The value was not set
            value = rvalue;
            return false;
        } else { // The value was set
            return true;
        }
    }


    bool AravisCamera::setStringFeature(const std::string& feature, std::string& value) {
        GError* error = nullptr;
        arv_device_set_string_feature_value(m_device, feature.c_str(), value.c_str(), &error);
        if (error != nullptr) {
            KARABO_LOG_FRAMEWORK_ERROR << "arv_device_set_string_feature_value failed: " << error->message;
            g_clear_error(&error);
        } else {
            return true; // success
        }

        // read back value
        const std::string rvalue = arv_device_get_string_feature_value(m_device, feature.c_str(), &error);
        if (error != nullptr) { // Could not read back value
            KARABO_LOG_FRAMEWORK_ERROR << "arv_device_get_string_feature_value failed: " << error->message;
            g_clear_error(&error);
            return false;
        } else if (rvalue != value) { // The value was not set
            value = rvalue;
            return false;
        } else { // The value was set
            return true;
        }
    }


    bool AravisCamera::setIntFeature(const std::string& feature, long long& value) {
        GError* error = nullptr;
        arv_device_set_integer_feature_value(m_device, feature.c_str(), value, &error);
        if (error != nullptr) {
            KARABO_LOG_FRAMEWORK_ERROR << "arv_device_set_integer_feature_value failed: " << error->message;
            g_clear_error(&error);
        } else {
            return true; // success
        }

        // read back value
        const long long rvalue = arv_device_get_integer_feature_value(m_device, feature.c_str(), &error);
        if (error != nullptr) { // Could not read back value
            KARABO_LOG_FRAMEWORK_ERROR << "arv_device_get_integer_feature_value failed: " << error->message;
            g_clear_error(&error);
            return false;
        } else if (rvalue != value) { // The value was not set
            value = rvalue;
            return false;
        } else { // The value was set
            return true;
        }
    }


    bool AravisCamera::setFloatFeature(const std::string& feature, double& value) {
        GError* error = nullptr;
        arv_device_set_float_feature_value(m_device, feature.c_str(), value, &error);
        if (error != nullptr) {
            KARABO_LOG_FRAMEWORK_ERROR << "arv_device_set_float_feature_value failed: " << error->message;
            g_clear_error(&error);
        } {
            return true; // success
        }

        // read back value
        const double rvalue = arv_device_get_float_feature_value(m_device, feature.c_str(), &error);
        if (error != nullptr) { // The value was not set
            KARABO_LOG_FRAMEWORK_ERROR << "arv_device_get_float_feature_value failed: " << error->message;
            g_clear_error(&error);
            return false;
        } else if (rvalue != value) { // The value was not set
            value = rvalue;
            return false;
        } else { // The value was set
            return true;
        }
    }


    void AravisCamera::initialize() {
        m_reconnect_timer.expires_from_now(boost::posix_time::milliseconds(1));
        m_reconnect_timer.async_wait(karabo::util::bind_weak(&AravisCamera::connect, this, boost::asio::placeholders::error));

        m_poll_timer.expires_from_now(boost::posix_time::seconds(1l));
        m_poll_timer.async_wait(karabo::util::bind_weak(&AravisCamera::pollCamera, this, boost::asio::placeholders::error));
    }


    void AravisCamera::connect(const boost::system::error_code & ec) {
        if (ec == boost::asio::error::operation_aborted) return;
        if (!m_connect) return;

        if (m_camera) {
            // Already connected
            m_reconnect_timer.expires_from_now(boost::posix_time::seconds(5l));
            m_reconnect_timer.async_wait(karabo::util::bind_weak(&AravisCamera::connect, this, boost::asio::placeholders::error));
            return;
        }

        const std::string& idType = this->get<std::string>("idType");
        const std::string& cameraId = this->get<std::string>("cameraId");
        std::string cameraIp;

        if (idType == "IP") { // IP address
            cameraIp = cameraId;

        } else if (idType == "HOST") { // IP name
            std::string message;
            const bool success = this->resolveHostname(cameraId, cameraIp, message);
            if (!success) {
                this->connection_failed_helper(message);
                return;
            } else if (m_failed_connections < 1) {
                KARABO_LOG_INFO << message;
            }

        } else if (idType == "SN") { // Serial number
            // Update the internal list of available devices
            arv_update_device_list();

            for (unsigned int idx=0; idx < arv_get_n_devices(); ++idx) {
                // Look for a matching serial number
                if (cameraId == arv_get_device_serial_nbr(idx)) {
                    cameraIp = arv_get_device_address(idx);
                    if (m_failed_connections < 1) {
                        KARABO_LOG_INFO << "Serial number resolved: " << cameraId << " -> " <<  cameraIp;
                    }
                    break;
                }
            }

            if (cameraIp.size() == 0) {
                const std::string message("Could not discover any camera with serial: " + cameraId);
                this->connection_failed_helper(message);
                return;
            }

        } else if (idType == "MAC") { // MAC address
            // Update the internal list of available devices
            arv_update_device_list();

            for (unsigned int idx=0; idx < arv_get_n_devices(); ++idx) {
                // Look for a matching MAC address
                if (cameraId == arv_get_device_physical_id(idx)) {
                    cameraIp = arv_get_device_address(idx);
                    if (m_failed_connections < 1) {
                        KARABO_LOG_INFO << "MAC address resolved: " << cameraId << " -> " <<  cameraIp;
                    }
                    break;
                }
            }

            if (cameraIp.size() == 0) {
                const std::string message("Could not discover any camera with MAC: " + cameraId);
                this->connection_failed_helper(message);
                return;
            }

        }

        this->clear_stream();
        this->clear_camera();
        GError* error = nullptr;
        m_camera = arv_camera_new(cameraIp.c_str(), &error);

        if (error != nullptr) {
            std::stringstream ss;
            ss << "arv_camera_new failed: " << error->message; // detailed message
            this->connection_failed_helper("Cannot connect to " + cameraIp, ss.str());
            g_clear_error(&error);
            return;
        }

        // ArvDevice gives more complete access to camera features
        m_device = arv_camera_get_device(m_camera);

        // The following is a workaround due to the fact that ARAVIS 0.6 does
        // not decode the AccessStatus from the discovery pong.
        // Therefore we send a "TriggerSoftware" command, which is listed as
        // "recommended" in the GenICam standard, and check for status.
        // If it is not "SUCCESS" we assume it's because of another application
        // controlling the camera.
        arv_device_execute_command(m_device, "TriggerSoftware", &error);
        if (error != nullptr) {
            const std::string message("Cannot connect to " + cameraIp
                + ". Another application might be controlling it.");
            std::stringstream ss;
            ss << "arv_device_execute_command failed: " << error->message; // detailed message
            this->connection_failed_helper(message, ss.str());
            g_clear_error(&error);
            return;
        }

        // Successfully connected!
        KARABO_LOG_INFO << "Connected to " << cameraIp;

        // Connect the control-lost signal
        g_signal_connect(arv_camera_get_device(m_camera), "control-lost", G_CALLBACK(AravisCamera::control_lost_cb), static_cast<void*>(this));

        Hash h;

        // Read immutable properties
        if (error == nullptr) h.set("camId", std::string(arv_camera_get_device_id(m_camera, &error)));
        if (error == nullptr) h.set("vendor", std::string(arv_camera_get_vendor_name(m_camera, &error)));
        if (error == nullptr) h.set("model", std::string(arv_camera_get_model_name(m_camera, &error)));

        if (error == nullptr) {
            gint width, height;
            arv_camera_get_sensor_size(m_camera, &width, &height, &error);
            h.set("width", width);
            h.set("height", height);
        }

        if (error == nullptr) m_is_binning_available = arv_camera_is_binning_available(m_camera, &error);
        if (error == nullptr) m_is_exposure_time_available = arv_camera_is_exposure_time_available(m_camera, &error);
        if (error == nullptr) m_is_frame_rate_available = arv_camera_is_frame_rate_available(m_camera, &error);
        if (error == nullptr) m_is_gain_available = arv_camera_is_gain_available(m_camera, &error);
        if (error == nullptr) m_is_gain_auto_available = arv_camera_is_gain_auto_available(m_camera, &error);

        if (error != nullptr) {
            const std::string detailed_msg(error->message);
            this->connection_failed_helper("An error occurred whilst connecting to " + cameraIp, detailed_msg);
            g_clear_error(&error);
            return;
        }

        this->set(h);
        
        // Apply initial configuration
        Hash initialConfiguration = this->getCurrentConfiguration();
        this->configure(initialConfiguration);

        const bool success = this->updateOutputSchema();
        if (!success) {
            this->connection_failed_helper("Could not update output schema");
            return;
        }

        this->updateState(State::ON);
        m_failed_connections = 0;
        m_reconnect_timer.expires_from_now(boost::posix_time::seconds(5l));
        m_reconnect_timer.async_wait(karabo::util::bind_weak(&AravisCamera::connect, this, boost::asio::placeholders::error));
    }


    void AravisCamera::connection_failed_helper(const std::string& message, const std::string& detailed_msg) {
        if (m_failed_connections < 1) {
            // Only log first error message
            KARABO_LOG_ERROR << message;
            if (!detailed_msg.empty()) {
                KARABO_LOG_FRAMEWORK_ERROR << detailed_msg;
            }
        } else {
            KARABO_LOG_DEBUG << message;
            if (!detailed_msg.empty()) {
                KARABO_LOG_FRAMEWORK_DEBUG << detailed_msg;
            }
        }

        // Increase counter, and reset objects
        ++m_failed_connections;
        m_camera = nullptr;
        m_device = nullptr;

        // Try reconnecting after some time
        m_reconnect_timer.expires_from_now(boost::posix_time::seconds(5l));
        m_reconnect_timer.async_wait(karabo::util::bind_weak(&AravisCamera::connect, this, boost::asio::placeholders::error));
    }


    bool AravisCamera::set_auto_packet_size() {
        GError* error = nullptr;

        const guint packetSize = arv_camera_gv_auto_packet_size(m_camera, &error);
        if (error != nullptr) {
            KARABO_LOG_FRAMEWORK_ERROR << "arv_camera_gv_auto_packet_size failed: " << error->message;
            g_clear_error(&error);
            return false; // failure
        }

        arv_camera_gv_set_packet_size(m_camera, packetSize, &error);
        if (error != nullptr) {
            KARABO_LOG_FRAMEWORK_ERROR << "arv_camera_gv_set_packet_size failed: " << error->message;
            g_clear_error(&error);
            return false; // failure
        }

        return true; // success
    }


    bool AravisCamera::set_region(int x, int y, int width, int height) {
        GError* error = nullptr;

        // Get bounds
        gint xmin, xmax, ymin, ymax, wmin, wmax, hmin, hmax;
        arv_camera_get_x_offset_bounds(m_camera, &xmin, &xmax, &error);
        if (error == nullptr) {
            arv_camera_get_y_offset_bounds(m_camera, &ymin, &ymax, &error);
        }
        if (error == nullptr) {
            arv_camera_get_width_bounds(m_camera, &wmin, &wmax, &error);
        }
        if (error == nullptr) {
            arv_camera_get_height_bounds(m_camera, &hmin, &hmax, &error);
        }

        if (error != nullptr) {
            KARABO_LOG_FRAMEWORK_ERROR << "Could not get ROI bounds: " << error->message;
            g_clear_error(&error);
            return false; // failure
        }

        // Apply bounds
        x = max(x, xmin);
        x = min(x, xmax);
        y = max(y, ymin);
        y = min(y, ymax);
        if (width == 0) {
            // Whole sensor width
            width = wmax;
        } else {
            width = max(width, wmin);
            width = min(width, wmax);
        }
        if (height == 0) {
            // Whole sensor height
            height = hmax;
        } else {
            height = max(height, hmin);
            height = min(height, hmax);
        }

        arv_camera_set_region(m_camera, x, y, width, height, &error);
        if (error != nullptr) {
            KARABO_LOG_FRAMEWORK_ERROR << "arv_camera_set_region failed: " << error->message;
            g_clear_error(&error);
            return false; // failure
        }

        return true; // success
    }


    bool AravisCamera::set_binning(int bin_x, int bin_y) {
        GError* error = nullptr;

        // Get bounds
        gint xmin, xmax, ymin, ymax;
        arv_camera_get_x_binning_bounds(m_camera, &xmin, &xmax, &error);
        if (error == nullptr) {
            arv_camera_get_y_binning_bounds(m_camera, &ymin, &ymax, &error);
        }

        if (error != nullptr) {
            KARABO_LOG_FRAMEWORK_ERROR << "Could not get binning bounds: " << error->message;
            g_clear_error(&error);
            return false; // failure
        }

        // Apply bounds
        bin_x = max(bin_x, xmin);
        bin_x = min(bin_x, xmax);
        bin_y = max(bin_y, ymin);
        bin_y = min(bin_y, ymax);

        arv_camera_set_binning(m_camera, bin_x, bin_y, &error);
        if (error != nullptr) {
            KARABO_LOG_FRAMEWORK_ERROR << "arv_camera_set_binning failed: " << error->message;
            g_clear_error(&error);
            return false; // failure
        }

        return true; // success
    }


    bool AravisCamera::set_exposure_time(double exposure_time) {
        GError* error = nullptr;

        // Get bounds
        double tmin, tmax;
        arv_camera_get_exposure_time_bounds(m_camera, &tmin, &tmax, &error);

        if (error != nullptr) {
            KARABO_LOG_FRAMEWORK_ERROR << "arv_camera_get_exposure_time_bounds failed: " << error->message;
            g_clear_error(&error);
            return false; // failure
        }

        if (tmin > 0.) {
            // exposure time must be multiple of tmin
            exposure_time = tmin * floor(exposure_time / tmin);
        }

        // Apply bounds
        exposure_time = max(exposure_time, tmin);
        exposure_time = min(exposure_time, tmax);

        arv_camera_set_exposure_time(m_camera, exposure_time, &error);
        if (error != nullptr) {
            KARABO_LOG_FRAMEWORK_ERROR << "arv_camera_set_exposure_time failed: " << error->message;
            g_clear_error(&error);
            return false; // failure
        }

        return true; // success
    }


    bool AravisCamera::set_frame_rate(bool enable, double frame_rate) {
        GError* error = nullptr;

        if (enable) {
            // set frame rate

            if (frame_rate <= 0.) {
                // If no valid rate is provided, the one on the camera is re-applied
                frame_rate = arv_camera_get_frame_rate(m_camera, &error);
                if (error != nullptr) {
                    KARABO_LOG_FRAMEWORK_ERROR << "arv_camera_get_frame_rate failed: " << error->message;
                    g_clear_error(&error);
                    return false; // failure
                }
            }

            // read the current trigger selector
            const std::string& triggerSelector = arv_device_get_string_feature_value(m_device, "TriggerSelector", &error);
            if (error != nullptr) {
                KARABO_LOG_FRAMEWORK_ERROR << "arv_device_get_string_feature_value failed: " << error->message;
                g_clear_error(&error);
                return false; // failure
            }

            // read which triggers are "On" in order to restore them later
            guint n_triggers;
            const char** triggerSelectorOptions = arv_camera_dup_available_triggers(m_camera, &n_triggers, &error);
            if (error != nullptr) {
                KARABO_LOG_FRAMEWORK_ERROR << "arv_camera_dup_available_triggers failed: " << error->message;
                g_clear_error(&error);
                return false; // failure
            }

            std::vector<std::string> triggerOn;
            for (unsigned short i = 0; i < n_triggers; ++i) {
                arv_device_set_string_feature_value(m_device, "TriggerSelector", triggerSelectorOptions[i], &error);
                if (error != nullptr) break;

                const std::string mode(arv_device_get_string_feature_value(m_device, "TriggerMode", &error));
                if (error != nullptr) break;

                if (mode == "On") {
                    triggerOn.push_back(triggerSelectorOptions[i]);
                }
            }

            if (error != nullptr) {
                KARABO_LOG_FRAMEWORK_ERROR << "Could not get TriggerModes: " << error->message;
                g_clear_error(&error);
                return false; // failure
            }

            // N.B. this function will set triggerMode to "Off" on all the selectors
            arv_camera_set_frame_rate(m_camera, frame_rate, &error);
            if (error != nullptr) {
                KARABO_LOG_FRAMEWORK_ERROR << "arv_camera_set_frame_rate failed: " << error->message;
                g_clear_error(&error);
                return false; // failure
            }

            // restore trigger modes
            for (const std::string& selector : triggerOn) {
                arv_device_set_string_feature_value(m_device, "TriggerSelector", selector.c_str(), &error);
                if (error != nullptr) break;

                arv_device_set_string_feature_value(m_device, "TriggerMode", "On", &error);
                if (error != nullptr) break;
            }

            if (error != nullptr) {
                KARABO_LOG_FRAMEWORK_ERROR << "Could not restore TriggerModes: " << error->message;
                g_clear_error(&error);
                return false; // failure
            }

            // restore trigger selector
            arv_device_set_string_feature_value(m_device, "TriggerSelector", triggerSelector.c_str(), &error);
            if (error != nullptr) {
                KARABO_LOG_FRAMEWORK_ERROR << "Could not restore TriggerSelector: " << error->message;
                g_clear_error(&error);
                return false; // failure
            }

        } else { // enable == false
            arv_device_set_boolean_feature_value(m_device, "AcquisitionFrameRateEnable", false, &error);
            if (error != nullptr) {
                KARABO_LOG_FRAMEWORK_ERROR << "Could not set AcquisitionFrameRateEnable: " << error->message;
                g_clear_error(&error);
                return false; // failure
            }
        }

        return true; // success
    }


    bool AravisCamera::set_gain(double gain) {
        GError* error = nullptr;

        // Get bounds
        double gmin, gmax;
        arv_camera_get_gain_bounds(m_camera, &gmin, &gmax, &error);
        if (error != nullptr) {
            KARABO_LOG_FRAMEWORK_ERROR << "arv_camera_get_gain_bounds failed: " << error->message;
            g_clear_error(&error);
            return false; // failure
        }

        // Apply bounds
        gain = max(gain, gmin);
        gain = min(gain, gmax);

        arv_camera_set_gain(m_camera, gain, &error);
        if (error != nullptr) {
            KARABO_LOG_FRAMEWORK_ERROR << "arv_camera_set_gain failed: " << error->message;
            g_clear_error(&error);
            return false; // failure
        }

        return true; // success
    }


    bool AravisCamera::set_frame_count(gint64 frame_count) {
        GError* error = nullptr;

        // Get bounds
        gint64 fmin, fmax;
        arv_camera_get_frame_count_bounds(m_camera, &fmin, &fmax, &error);
        if (error != nullptr) {
            KARABO_LOG_FRAMEWORK_ERROR << "arv_camera_get_frame_count_bounds failed: " << error->message;
            g_clear_error(&error);
            return false; // failure
        }

        // Apply bounds
        frame_count = max(frame_count, fmin);
        frame_count = min(frame_count, fmax);

        arv_camera_set_frame_count(m_camera, frame_count, &error);
        if (error != nullptr) {
            KARABO_LOG_FRAMEWORK_ERROR << "arv_camera_set_frame_count failed: " << error->message;
            g_clear_error(&error);
            return false; // failure
        }

        return true; // success
    }


    void AravisCamera::configure(karabo::util::Hash& configuration) {
        if (m_camera == nullptr) {
            // cannot configure camera, as we are not connected
            return;
        }

        GError* error = nullptr;

        if (configuration.has("packetDelay")) {
            arv_camera_gv_set_packet_delay(m_camera, configuration.get<long long>("packetDelay"), &error);
            if (error != nullptr) {
                KARABO_LOG_FRAMEWORK_ERROR << "arv_camera_gv_set_packet_delay failed: " << error->message;
                configuration.erase("packetDelay");
                g_clear_error(&error);
            }
        }

        const bool autoPacketSize = GET_PATH(configuration, "autoPacketSize", bool);
        if (autoPacketSize) {
            const bool success = this->set_auto_packet_size();
            if (!success && configuration.has("autoPacketSize")) {
                configuration.erase("autoPacketSize");
            }
        } else {
            try {
                const guint packetSize = GET_PATH(configuration, "packetSize", int);
                arv_camera_gv_set_packet_size(m_camera, packetSize, &error);
                if (error != nullptr) {
                    KARABO_LOG_FRAMEWORK_ERROR << "arv_camera_gv_set_packet_size failed: " << error->message;
                    if (configuration.has("packetSize")) configuration.erase("packetSize");
                    g_clear_error(&error);
                }
            } catch (const karabo::util::ParameterException& e) {
                // key neither in configuration nor on device
            }
        }

        if (configuration.has("pixelFormat")) {
            const char* pixelFormat = configuration.get<std::string>("pixelFormat").c_str();
            arv_camera_set_pixel_format_from_string(m_camera, pixelFormat, &error);
            if (error != nullptr) {
                KARABO_LOG_FRAMEWORK_ERROR << "arv_camera_set_pixel_format_from_string failed: " << error->message;
                configuration.erase("pixelFormat");
                g_clear_error(&error);
            }
        }

        if (configuration.has("roi")) {
            const int x = GET_PATH(configuration, "roi.x", int);
            const int y = GET_PATH(configuration, "roi.y", int);
            const int width = GET_PATH(configuration, "roi.width", int);
            const int height = GET_PATH(configuration, "roi.height", int);

            const bool success = this->set_region(x, y, width, height);
            if (!success) {
                configuration.erase("roi");
            }
        }

        if (configuration.has("bin") && m_is_binning_available) {
            const int bin_x = GET_PATH(configuration, "bin.x", int);
            const int bin_y = GET_PATH(configuration, "bin.y", int);

            const bool success = this->set_binning(bin_x, bin_y);
            if (!success) {
                configuration.erase("bin");
            }
        }

        if (configuration.has("exposureTime") && m_is_exposure_time_available) {
            const double exposureTime = configuration.get<double>("exposureTime");

            const bool success = this->set_exposure_time(exposureTime);
            if (!success) {
                configuration.erase("exposureTime");
            }
        }

        if (configuration.has("frameRate") && m_is_frame_rate_available) {
            const bool enable = GET_PATH(configuration, "frameRate.enable", bool);
            double frameRate;
            try {
                frameRate = GET_PATH(configuration, "frameRate.target", float);
            } catch (const karabo::util::ParameterException& e) {
                // key neither in configuration nor on device
                frameRate = -1; // i.e. read from camera
            }

            bool success = this->set_frame_rate(enable, frameRate);
            if (!success) {
                configuration.erase("frameRate");
            }
        }

        if (m_arv_camera_trigger) {
            // trigger properties can be accessed with the arv_camera interface
            bool success;

            if (configuration.has("triggerSelector")) {
                std::string triggerSelector = configuration.get<std::string>("triggerSelector");
                success = this->setStringFeature("TriggerSelector", triggerSelector);
                if (!success) {
                    configuration.erase("triggerSelector");
                }
            }

            if (configuration.has("triggerMode")) {
                std::string triggerMode = configuration.get<std::string>("triggerMode");
                success = this->setStringFeature("TriggerMode", triggerMode);
                if (!success) {
                    configuration.erase("triggerMode");
                }
            }

            if (configuration.has("triggerSource")) {
                std::string triggerSource = configuration.get<std::string>("triggerSource");
                success = this->setStringFeature("TriggerSource", triggerSource);
                if (!success) {
                    configuration.erase("triggerSource");
                }
            }

            if (configuration.has("triggerActivation")) {
                std::string triggerActivation = configuration.get<std::string>("triggerActivation");
                success = this->setStringFeature("TriggerActivation", triggerActivation);
                if (!success) {
                    configuration.erase("triggerActivation");
                }
            }
        }

        if (configuration.has("autoGain") && m_is_gain_auto_available) {
            const std::string& autoGainStr = configuration.get<std::string>("autoGain");
            const ArvAuto autoGain = arv_auto_from_string(autoGainStr.c_str());

            arv_camera_set_gain_auto(m_camera, autoGain, &error);
            if (error != nullptr) {
                KARABO_LOG_FRAMEWORK_ERROR << "arv_camera_set_gain_auto failed: " << error->message;
                configuration.erase("autoGain");
                g_clear_error(&error);
            }
        }

        if (configuration.has("gain") && m_is_gain_available) {
            double gain = configuration.get<double>("gain");

            const bool success = this->set_gain(gain);
            if (!success) {
                configuration.erase("gain");
            }
        }

        if (configuration.has("acquisitionMode")) {
            const std::string& acquisitionMode = configuration.get<std::string>("acquisitionMode");
            arv_camera_set_acquisition_mode(m_camera, arv_acquisition_mode_from_string(acquisitionMode.c_str()), &error);
            if (error != nullptr) {
                KARABO_LOG_FRAMEWORK_ERROR << "arv_camera_set_acquisition_mode failed: " << error->message;
                configuration.erase("acquisitionMode");
                g_clear_error(&error);
            }
        }

        if (configuration.has("frameCount")) {
            gint64 frameCount = configuration.get<long long>("frameCount");

            const bool success = this->set_frame_count(frameCount);
            if (!success) {
                configuration.erase("frameCount");
            }
        }

        // Filter configuration by tag "genicam" and loop over it
        const Hash filtered = this->filterByTags(configuration, "genicam");
        const Schema schema = this->getFullSchema();
        std::vector<std::string> paths;
        filtered.getPaths(paths);
        for (const auto& key : paths) {
            bool success = false;
            const auto feature = this->getAliasFromKey<std::string>(key);
            const auto valueType = this->getValueType(key);
            const auto accessMode = schema.getAccessMode(key);
            bool boolValue;
            long long intValue;
            double floatValue;
            std::string stringValue;

            if (accessMode == AccessType::READ) {
                // Read-Only parameter
                continue;
            }

            switch(valueType) {
                case Types::BOOL:
                    boolValue = configuration.get<bool>(key);
                    success = this->setBoolFeature(feature, boolValue);
                    configuration.set<bool>(key, boolValue); // set read value
                    break;
                case Types::STRING:
                    stringValue = configuration.get<std::string>(key);
                    success = this->setStringFeature(feature, stringValue);
                    configuration.set<std::string>(key, stringValue); // set read value
                    break;
                case Types::INT32:
                    intValue = configuration.get<int>(key);
                    success = this->setIntFeature(feature, intValue);
                    configuration.set<int>(key, intValue); // set read value
                    break;
                case Types::INT64:
                    intValue = configuration.get<long long>(key);
                    success = this->setIntFeature(feature, intValue);
                    configuration.set<long long>(key, intValue); // set read value
                    break;
                case Types::FLOAT:
                    floatValue = configuration.get<float>(key);
                    success = this->setFloatFeature(feature, floatValue);
                    configuration.set<float>(key, floatValue); // set read value
                    break;
                case Types::DOUBLE:
                    floatValue = configuration.get<double>(key);
                    success = this->setFloatFeature(feature, floatValue);
                    configuration.set<double>(key, floatValue); // set read value
                    break;
                default:
                    throw KARABO_NOT_IMPLEMENTED_EXCEPTION(key + " datatype not available in GenICam");
            }

            if (!success) {
                KARABO_LOG_WARN << "Setting value for " << key << " may not have been successful. Value on device updated according to camera.";
            }
        }

    }


    void AravisCamera::acquire() {
        const std::string error_msg("Could not start acquisition");
        GError* error = nullptr;

        m_timer.now();
        m_counter = 0;
        m_stream = arv_camera_create_stream(m_camera, AravisCamera::stream_cb, static_cast<void*>(this), &error);

        if (error != nullptr) {
            std::stringstream ss;
            ss << "arv_camera_create_stream failed: " << error->message;
            this->acquire_failed_helper(ss.str());
            g_clear_error(&error);
            return;
        }

        // Enable emission of signals (it's disabled by default for performance reason)
        arv_stream_set_emit_signals(m_stream, TRUE);

        // Create and push buffers to the stream
	const gint payload = arv_camera_get_payload(m_camera, &error);
        if (error != nullptr) {
            std::stringstream ss;
            ss << "arv_camera_get_payload failed: " << error->message;
            this->acquire_failed_helper(ss.str());
            g_clear_error(&error);
            return;
        }
	for (size_t i = 0; i < 10; i++)
            arv_stream_push_buffer(m_stream, arv_buffer_new(payload, NULL));

        arv_camera_start_acquisition(m_camera, &error);
        if (error != nullptr) {
            std::stringstream ss;
            ss << "arv_camera_start_acquisition failed: " << error->message;
            this->acquire_failed_helper(ss.str());
            g_clear_error(&error);
            return;
        }

        // Connect the 'new-buffer' signal
        g_signal_connect(m_stream, "new-buffer", G_CALLBACK(AravisCamera::new_buffer_cb), static_cast<void*>(this));

        this->updateState(State::ACQUIRING);
    }


    void AravisCamera::acquire_failed_helper(const std::string& detailed_msg) {
        const std::string message("Could not start acquisition");

        KARABO_LOG_ERROR << message;
        KARABO_LOG_FRAMEWORK_ERROR << detailed_msg;
        this->updateState(State::ERROR);
    }


    void AravisCamera::stop() {
        this->clear_stream();

        GError* error = nullptr;
        arv_camera_stop_acquisition(m_camera, &error);

        if (error != nullptr) {
            KARABO_LOG_ERROR << "Could not stop acquisition";
            KARABO_LOG_FRAMEWORK_ERROR << "arv_camera_stop_acquisition failed: " << error->message;
            g_clear_error(&error);
            this->set("frameRate.actual", 0.);
            this->updateState(State::ERROR);
            return;
        }

        this->signalEOS(); // End-of-Stream signal
        this->set("frameRate.actual", 0.);
        this->updateState(State::ON);
    }


    void AravisCamera::trigger() {
        if (!m_arv_camera_trigger) {
            return;
        }

        GError* error = nullptr;

        const std::string& triggerMode = this->get<std::string>("triggerMode");
        if (triggerMode == "On") {
            const std::string& triggerSource = this->get<std::string>("triggerSource");
            if (triggerSource == "Software") {
                arv_camera_software_trigger(m_camera, &error);
                if (error != nullptr) {
                    KARABO_LOG_FRAMEWORK_ERROR << "arv_camera_software_trigger failed: " << error->message;
                    g_clear_error(&error);
                }
            }
        }
    }


    void AravisCamera::refresh() {
        // Poll parameters and update options
        const bool success = this->updateOutputSchema();
        if (!success) {
            this->updateState(State::ERROR);
        }
    }


    void AravisCamera::reset() {
        // Poll parameters and update options
        const bool success = this->updateOutputSchema();
        if (success) {
            this->updateState(State::ON);
        }
    }


    void AravisCamera::clear_camera() {
        if (m_camera != nullptr) {
            g_clear_object(&m_camera);
        }
    }


    void AravisCamera::clear_stream() {
        if (m_stream != nullptr) {
            // TODO possibly disconnect signal, see https://developer.gnome.org/gobject/stable/gobject-Signals.html#g-signal-handler-disconnect

            // Disable emission of signals and free resource
            arv_stream_set_emit_signals(m_stream, FALSE);
            g_clear_object(&m_stream);
        }
    }


    void AravisCamera::stream_cb(void *context, ArvStreamCallbackType type, ArvBuffer *buffer) {
        Self* self = static_cast<Self*>(context);

        if (type == ARV_STREAM_CALLBACK_TYPE_INIT) {
            KARABO_LOG_FRAMEWORK_DEBUG << "Init stream";
                if (!arv_make_thread_realtime(10) && !arv_make_thread_high_priority(-10)) {
                    KARABO_LOG_FRAMEWORK_WARN << "Failed to make stream thread high priority";
                }
        }
    }


    void AravisCamera::new_buffer_cb(ArvStream* stream, void* context) {
        Self* self = static_cast<Self*>(context);

        ArvBuffer* arv_buffer = arv_stream_pop_buffer(stream);
	if (arv_buffer == nullptr) {
            return;
        }

        if (arv_buffer_get_status(arv_buffer) == ARV_BUFFER_STATUS_SUCCESS) {
            gint x, y, width, height;
            size_t buffer_size;

            const void* buffer_data = arv_buffer_get_data(arv_buffer, &buffer_size);
            arv_buffer_get_image_region(arv_buffer, &x, &y, &width, &height);
            const ArvPixelFormat pixel_format = arv_buffer_get_image_pixel_format(arv_buffer); // e.g. ARV_PIXEL_FORMAT_MONO_8
            const guint32 frame_id = arv_buffer_get_frame_id(arv_buffer);
            //KARABO_LOG_FRAMEWORK_DEBUG << "Got frame " << frame_id;

            switch(pixel_format) {
                case ARV_PIXEL_FORMAT_MONO_8:
                    self->writeOutputChannels<unsigned char>(buffer_data, width, height);
                    break;
                case ARV_PIXEL_FORMAT_MONO_10:
                case ARV_PIXEL_FORMAT_MONO_12:
                case ARV_PIXEL_FORMAT_MONO_14:
                case ARV_PIXEL_FORMAT_MONO_16:
                    self->writeOutputChannels<unsigned short>(buffer_data, width, height);
                    break;
                case ARV_PIXEL_FORMAT_MONO_10_PACKED:
                case ARV_PIXEL_FORMAT_MONO_12_PACKED:
                {
                    const uint8_t* data = reinterpret_cast<const uint8_t*>(buffer_data);
                    uint16_t* unpackedData = new uint16_t[width * height];
                    unpackMono12Packed(data, width, height, unpackedData);
                    self->writeOutputChannels<unsigned short>(unpackedData, width, height);
                    delete[] unpackedData;
                }
                    break;
                // TODO RGB, YUV...
                default:
                    if (self->m_pixelFormatOptions.find(pixel_format) != self->m_pixelFormatOptions.end()) {
                        KARABO_LOG_FRAMEWORK_ERROR << "Format " << self->m_pixelFormatOptions[pixel_format]
                            << " is not yet supported";
                    } else {
                        KARABO_LOG_FRAMEWORK_ERROR << "Format " << pixel_format << " is not yet supported";
                    }

                    if (self->getState() == State::ACQUIRING) {
                        self->execute("stop");
                    }
            }
        }

        // Push back the buffer to the stream
        arv_stream_push_buffer(stream, arv_buffer);
    }


    void AravisCamera::control_lost_cb(ArvGvDevice* gv_device, void* context) {
        // Control of the device is lost

        Self* self = static_cast<Self*>(context);

        // TODO what happens with multiple cameras on server?
        // Possibly use arv_gv_device_get_device_address (gv_device) to verify IP address

        KARABO_LOG_FRAMEWORK_WARN << "Control of the camera " << self->get<std::string>("cameraId") << " is lost";
        // TODO possibly release resources
        // NOTE 'self->clear_camera();' will seg fault
        self->m_camera = nullptr;
        self->m_device = nullptr;

        self->updateState(State::UNKNOWN);
    }


    void AravisCamera::pollOnce(karabo::util::Hash& h) {
        GError* error = nullptr;

        const long long packetDelay = arv_camera_gv_get_packet_delay(m_camera, &error);
        if (error == nullptr) {
            h.set("packetDelay", packetDelay);
        } else {
            KARABO_LOG_FRAMEWORK_WARN << "arv_camera_gv_get_packet_delay failed: " << error->message;
            g_clear_error(&error);
        }

        const guint packetSize = arv_camera_gv_get_packet_size(m_camera, &error);
        if (error == nullptr) {
            h.set("packetSize", packetSize);
        } else {
            KARABO_LOG_FRAMEWORK_WARN << "arv_camera_gv_get_packet_size failed: " << error->message;
            g_clear_error(&error);
        }

        gint x, y, width, height;
        arv_camera_get_region(m_camera, &x, &y, &width, &height, &error);
        if (error == nullptr) {
            h.set("roi.x", x);
            h.set("roi.y", y);
            h.set("roi.width", width);
            h.set("roi.height", height);
        } else {
            KARABO_LOG_FRAMEWORK_WARN << "arv_camera_get_region failed: " << error->message;
            g_clear_error(&error);
        }

        if (m_is_binning_available) {
            gint dx, dy;
            arv_camera_get_binning(m_camera, &dx, &dy, &error);
            if (error == nullptr) {
                h.set("bin.x", dx);
                h.set("bin.y", dy);
            } else {
                KARABO_LOG_FRAMEWORK_WARN << "arv_camera_get_binning failed: " << error->message;
                g_clear_error(&error);
            }
        }

        const std::string pixelFormat = arv_camera_get_pixel_format_as_string(m_camera, &error);
        if (error == nullptr) {
            h.set("pixelFormat", pixelFormat);
        } else {
            KARABO_LOG_FRAMEWORK_WARN << "arv_camera_get_pixel_format_as_string failed: " << error->message;
            g_clear_error(&error);
        }

        if (m_is_exposure_time_available) {
            const double exposureTime = arv_camera_get_exposure_time(m_camera, &error);
            if (error == nullptr) {
                h.set("exposureTime", exposureTime);
            } else {
                KARABO_LOG_FRAMEWORK_WARN << "arv_camera_get_exposure_time failed: " << error->message;
                g_clear_error(&error);
            }
        }

        if (m_arv_camera_trigger) {
            const std::string triggerSelector = arv_device_get_string_feature_value(m_device, "TriggerSelector", &error);
            if (error == nullptr) {
                h.set("triggerSelector", triggerSelector);
            } else {
                KARABO_LOG_FRAMEWORK_WARN << "Could not get TriggerSelector: " << error->message;
                g_clear_error(&error);
            }

            const std::string triggerMode = arv_device_get_string_feature_value(m_device, "TriggerMode", &error);
            if (error == nullptr) {
                h.set("triggerMode", triggerMode);
            } else {
                KARABO_LOG_FRAMEWORK_WARN << "Could not get TriggerMode: " << error->message;
                g_clear_error(&error);
            }

            // Under certain circumstances, nullptr is returned for "TriggerSource"
            const char* triggerSourcePtr = arv_device_get_string_feature_value(m_device, "TriggerSource", &error);
            if (error != nullptr) {
                KARABO_LOG_FRAMEWORK_WARN << "Could not get TriggerSource: " << error->message;
                g_clear_error(&error);
            } else if (triggerSourcePtr != nullptr) {
                h.set("triggerSource", triggerSourcePtr);
            } else {
                KARABO_LOG_FRAMEWORK_WARN << this->get<std::string>("deviceId")
                    << ": cannot get trigger sources from the camera";

                // Ensure that Karabo device and camera are in sync
                const std::string& triggerSource = this->get<std::string>("triggerSource");
                arv_device_set_string_feature_value(m_device, "TriggerSource", triggerSource.c_str(), &error);
                if (error != nullptr) {
                    KARABO_LOG_FRAMEWORK_WARN << "Could not set TriggerSource: " << error->message;
                    g_clear_error(&error);
                }
            }

            const std::string triggerActivation = arv_device_get_string_feature_value(m_device, "TriggerActivation", &error);
            if (error == nullptr) {
                h.set("triggerActivation", triggerActivation);
            } else {
                KARABO_LOG_FRAMEWORK_WARN << "Could not get TriggerActivation: " << error->message;
                g_clear_error(&error);
            }
        }

        if (m_is_frame_rate_available) {
            const double frameRate = arv_camera_get_frame_rate(m_camera, &error);
            if (error == nullptr) {
                h.set("frameRate.target", frameRate);
            } else {
                KARABO_LOG_FRAMEWORK_WARN << "arv_camera_get_frame_rate failed: " << error->message;
                g_clear_error(&error);
            }
        }

        if (m_is_gain_auto_available) {
            const ArvAuto autoGain = arv_camera_get_gain_auto(m_camera, &error);
            if (error == nullptr) {
                const std::string autoGainStr(arv_auto_to_string(autoGain));
                h.set("autoGain", autoGainStr);
            } else {
                KARABO_LOG_FRAMEWORK_WARN << "arv_camera_get_gain_auto failed: " << error->message;
                g_clear_error(&error);
            }
        }

        if (m_is_gain_available) {
            const double gain = arv_camera_get_gain(m_camera, &error);
            if (error == nullptr) {
                h.set("gain", gain);
            } else {
                KARABO_LOG_FRAMEWORK_WARN << "arv_camera_get_gain failed: " << error->message;
                g_clear_error(&error);
            }
        }

        const ArvAcquisitionMode acquisitionMode = arv_camera_get_acquisition_mode(m_camera, &error);
        if (error == nullptr) {
            const std::string acquisitionModeStr(arv_acquisition_mode_to_string(acquisitionMode));
            h.set("acquisitionMode", acquisitionModeStr);
        } else {
            KARABO_LOG_FRAMEWORK_WARN << "arv_camera_get_acquisition_mode failed: " << error->message;
            g_clear_error(&error);
        }

        const long long frameCount = arv_camera_get_frame_count(m_camera, &error);
        if (error == nullptr) {
            h.set("frameCount", frameCount);
        } else {
            KARABO_LOG_FRAMEWORK_WARN << "arv_camera_get_frame_count failed: " << error->message;
            g_clear_error(&error);
        }

        // Filter paths by tag "genicam" and poll features
        std::vector<std::string> paths;
        this->getPathsByTag(paths, "genicam");
        this->pollGenicamFeatures(paths, h);

    }


    void AravisCamera::pollCamera(const boost::system::error_code & ec) {
        if (ec == boost::asio::error::operation_aborted) return;

        if (!m_camera) {
            // Not connected
            m_poll_timer.expires_from_now(boost::posix_time::seconds(5l));
            m_poll_timer.async_wait(karabo::util::bind_weak(&AravisCamera::pollCamera, this, boost::asio::placeholders::error));
            return;
        }

        // Filter paths by tag "poll" and poll features
        std::vector<std::string> paths;
        this->getPathsByTag(paths, "poll");
        Hash h;
        this->pollGenicamFeatures(paths, h);

        this->set(h);

        const int pollingInterval = this->get<int>("pollingInterval");
        m_poll_timer.expires_from_now(boost::posix_time::seconds(pollingInterval));
        m_poll_timer.async_wait(karabo::util::bind_weak(&AravisCamera::pollCamera, this, boost::asio::placeholders::error));
    }


    void AravisCamera::pollGenicamFeatures(const std::vector<std::string>& paths, karabo::util::Hash& h) {
        for (const auto& key : paths) {
            const auto feature = this->getAliasFromKey<std::string>(key);
            const auto valueType = this->getValueType(key);
            bool boolValue;
            long long intValue;
            double doubleValue;
            std::string stringValue;
            switch(valueType) {
                case Types::BOOL:
                    if (this->getBoolFeature(feature, boolValue)) {
                        h.set(key, boolValue);
                    }
                    break;
                case Types::STRING:
                    if (this->getStringFeature(feature, stringValue)) {
                        h.set(key, stringValue);
                    }
                    break;
                case Types::INT32:
                case Types::INT64:
                    if (this->getIntFeature(feature, intValue)) {
                        h.set(key, intValue);
                    }
                    break;
                case Types::FLOAT:
                case Types::DOUBLE:
                    if (this->getFloatFeature(feature, doubleValue)) {
                        h.set(key, doubleValue);
                    }
                    break;
                default:
                    throw KARABO_NOT_IMPLEMENTED_EXCEPTION(key + " datatype not available in GenICam");
            }
        }
    }

    bool AravisCamera::updateOutputSchema() {
        if (m_camera == nullptr) {
            // cannot query camera, as we are not connected
            return true;
        }

        Hash h;
        this->pollOnce(h);

        const unsigned long long height = h.get<int>("roi.height");
        const unsigned long long width = h.get<int>("roi.width");
        const std::vector<unsigned long long> shape = {height, width};

        GError* error = nullptr;

        const ArvPixelFormat pixelFormat = arv_camera_get_pixel_format(m_camera, &error);
        if (error != nullptr) {
            KARABO_LOG_ERROR << "Could not update output schema";
            KARABO_LOG_FRAMEWORK_ERROR << "arv_camera_get_pixel_format failed: " << error->message;
            g_clear_error(&error);
            return false; // failure
        }

        Types::ReferenceType kType;
        switch(pixelFormat) {
            case ARV_PIXEL_FORMAT_MONO_8:
                m_encoding = Encoding::GRAY;
                kType = Types::UINT8;
                break;
            case ARV_PIXEL_FORMAT_MONO_10:
            case ARV_PIXEL_FORMAT_MONO_10_PACKED:
                m_encoding = Encoding::GRAY;
                kType = Types::UINT16;
                break;
            case ARV_PIXEL_FORMAT_MONO_12:
            case ARV_PIXEL_FORMAT_MONO_12_PACKED:
                m_encoding = Encoding::GRAY;
                kType = Types::UINT16;
                break;
            case ARV_PIXEL_FORMAT_MONO_14:
                m_encoding = Encoding::GRAY;
                kType = Types::UINT16;
                break;
            case ARV_PIXEL_FORMAT_MONO_16:
                m_encoding = Encoding::GRAY;
                kType = Types::UINT16;
                break;
            case ARV_PIXEL_FORMAT_RGB_8_PACKED:
            case ARV_PIXEL_FORMAT_RGB_8_PLANAR:
                m_encoding = Encoding::RGB;
                kType = Types::UINT8;
                break;
            case ARV_PIXEL_FORMAT_RGB_10_PACKED:
            case ARV_PIXEL_FORMAT_RGB_10_PLANAR:
                m_encoding = Encoding::RGB;
                kType = Types::UINT16;
                break;
            case ARV_PIXEL_FORMAT_RGB_12_PACKED:
            case ARV_PIXEL_FORMAT_RGB_12_PLANAR:
                m_encoding = Encoding::RGB;
                kType = Types::UINT16;
                break;
            case ARV_PIXEL_FORMAT_RGB_16_PLANAR:
                m_encoding = Encoding::RGB;
                kType = Types::UINT16;
                break;
            // TODO: YUV
            default:
                m_encoding = Encoding::GRAY;
                kType = Types::UNKNOWN;
                break;
        }

        const unsigned short bpp = ARV_PIXEL_FORMAT_BIT_PER_PIXEL(pixelFormat);
        h.set("bpp", bpp);

        this->set(h);
        CameraImageSource::updateOutputSchema(shape, m_encoding, kType);

        guint n_int_values, n_str_values;
        gint64* int_options;
        const char** str_options;

        // get available pixel formats
        int_options = arv_camera_dup_available_pixel_formats(m_camera, &n_int_values, &error);
        if (error != nullptr) {
            KARABO_LOG_ERROR << "Could not update output schema";
            KARABO_LOG_FRAMEWORK_ERROR << "arv_camera_dup_available_pixel_formats failed: " << error->message;
            g_clear_error(&error);
            return false; // failure
        }

        str_options = arv_camera_dup_available_pixel_formats_as_strings(m_camera, &n_str_values, &error);
        if (error != nullptr) {
            KARABO_LOG_ERROR << "Could not update output schema";
            KARABO_LOG_FRAMEWORK_ERROR << "arv_camera_dup_available_pixel_formats_as_strings failed: " << error->message;
            g_clear_error(&error);
            return false; // failure
        }

        if (n_int_values == n_str_values) {
            // fill-up the pixel_format_options map
            for (unsigned short i = 0; i < n_int_values; ++i) {
                m_pixelFormatOptions[int_options[i]] = str_options[i];
            }
        } else {
            KARABO_LOG_WARN << "Could not fill-up pixel_format_options map: different number of "
                << "int and string options.";
        }

        std::string pixelFormatOptions;
        for (unsigned short i = 0; i < n_str_values; ++i) {
            if (i > 0) pixelFormatOptions.append(",");
            pixelFormatOptions.append(str_options[i]);
        }
        g_free(int_options);
        g_free(str_options);

        Schema schemaUpdate;
        STRING_ELEMENT(schemaUpdate).key("pixelFormat")
                .displayedName("Pixel Format")
                .assignmentOptional().noDefaultValue()
                .options(pixelFormatOptions)
                .reconfigurable()
                .allowedStates(State::UNKNOWN, State::ON)
                .commit();

        // Disable setting not available properties
        const std::string notAvailable("Not available for this camera.");

        if (m_arv_camera_trigger) {
            // get available trigger selectors
            str_options = arv_camera_dup_available_triggers(m_camera, &n_str_values, &error);
            if (error != nullptr) {
                KARABO_LOG_ERROR << "Could not update output schema";
                KARABO_LOG_FRAMEWORK_ERROR << "arv_camera_dup_available_triggers failed: " << error->message;
                g_clear_error(&error);
                return false; // failure
            }

            std::string triggerSelectorOptions;
            for (unsigned short i = 0; i < n_str_values; ++i) {
                if (i > 0) triggerSelectorOptions.append(",");
                triggerSelectorOptions.append(str_options[i]);
            }
            g_free(str_options);

            STRING_ELEMENT(schemaUpdate).key("triggerSelector")
                    .displayedName("Trigger Selector")
                    .description("This enumeration selects the trigger type to configure. "
                    "Once a trigger type has been selected, all changes to the trigger settings will be applied to "
                    "the selected trigger.")
                    .assignmentOptional().defaultValue("FrameStart")
                    .options(triggerSelectorOptions)
                    .reconfigurable()
                    .allowedStates(State::UNKNOWN, State::ON)
                    .commit();

            // get available trigger sources
            str_options = arv_camera_dup_available_trigger_sources(m_camera, &n_str_values, &error);
            if (error != nullptr) {
                KARABO_LOG_ERROR << "Could not update output schema";
                KARABO_LOG_FRAMEWORK_ERROR << "arv_camera_dup_available_trigger_sources failed: " << error->message;
                g_clear_error(&error);
                return false; // failure
            }

            std::string triggerSourceOptions;
            for (unsigned short i = 0; i < n_str_values; ++i) {
                if (i > 0) triggerSourceOptions.append(",");
                triggerSourceOptions.append(str_options[i]);
            }
            g_free(str_options);

            if (n_str_values == 0) {
                KARABO_LOG_FRAMEWORK_WARN << this->get<std::string>("deviceId")
                    << ": could not get available trigger sources from camera. "
                    << "Using defaults.";
                triggerSourceOptions = "Software,Line1";
            }

            STRING_ELEMENT(schemaUpdate).key("triggerSource")
                    .displayedName("Trigger Source")
                    .description("This enumeration sets the signal source for the selected trigger.")
                    .assignmentOptional().noDefaultValue()
                    .options(triggerSourceOptions)
                    .reconfigurable()
                    .allowedStates(State::UNKNOWN, State::ON)
                    .commit();
        }

        if (!m_is_binning_available) {
            NODE_ELEMENT(schemaUpdate).key("bin")
                    .displayedName("Image Binning")
                    .commit();

            INT32_ELEMENT(schemaUpdate).key("bin.x")
                    .displayedName("X Binning")
                    .description(notAvailable)
                    .readOnly()
                    .commit();

            INT32_ELEMENT(schemaUpdate).key("bin.y")
                    .displayedName("Y Binning")
                    .description(notAvailable)
                    .readOnly()
                    .commit();
        }

        if (!m_is_exposure_time_available) {
            DOUBLE_ELEMENT(schemaUpdate).key("exposureTime")
                    .displayedName("Exposure Time")
                    .description(notAvailable)
                    .readOnly()
                    .commit();
        }

        NODE_ELEMENT(schemaUpdate).key("frameRate")
                .displayedName("Frame Rate")
                .commit();

        if (!m_is_frame_rate_available) {
            FLOAT_ELEMENT(schemaUpdate).key("frameRate.target")
                    .displayedName("Target Frame Rate")
                    .description(notAvailable)
                    .readOnly()
                    .commit();

            FLOAT_ELEMENT(schemaUpdate).key("frameRate.actual")
                    .displayedName("Actual Frame Rate")
                    .description("The measured frame rate.")
                    .unit(Unit::HERTZ)
                    .readOnly()
                    .initialValue(0.)
                    .commit();
        } else {
            FLOAT_ELEMENT(schemaUpdate).key("frameRate.target")
                    .displayedName("Target Frame Rate")
                    .description("Sets the 'target' value of the acquisition frame rate on the camera. "
                    "Please be aware that if you enable this feature in combination with external trigger, "
                    "the resulting 'actual' frame rate will most likely be smaller.")
                    .assignmentOptional().defaultValue(10.)
                    .minExc(0.)
                    .unit(Unit::HERTZ)
                    .reconfigurable()
                    .allowedStates(State::UNKNOWN, State::ON)
                    .commit();

            FLOAT_ELEMENT(schemaUpdate).key("frameRate.actual")
                    .displayedName("Actual Frame Rate")
                    .description("The measured frame rate.")
                    .unit(Unit::HERTZ)
                    .readOnly()
                    .initialValue(0.)
                    .commit();
        }

        const std::string vendor = arv_camera_get_vendor_name(m_camera, &error);
        if (error != nullptr) {
            KARABO_LOG_FRAMEWORK_WARN << "arv_camera_get_vendor_name failed: " << error->message;
            g_clear_error(&error);
        }
        if (vendor == "Basler") {
            BOOL_ELEMENT(schemaUpdate).key("frameRate.enable")
                    .displayedName("Frame Rate Enable")
                    .description("Enable frame rate control when camera is in trigger mode.")
                    .assignmentOptional().defaultValue(false)
                    .reconfigurable()
                    .allowedStates(State::UNKNOWN, State::ON)
                    .commit();
        } else {
            BOOL_ELEMENT(schemaUpdate).key("frameRate.enable")
                    .displayedName("Frame Rate Enable")
                    .description(notAvailable)
                    .readOnly()
                    .commit();
        }

        if (!m_is_gain_auto_available) {
            STRING_ELEMENT(schemaUpdate).key("autoGain")
                    .displayedName("Auto Gain")
                    .description(notAvailable)
                    .readOnly()
                    .commit();
        }

        if (!m_is_gain_available) {
            DOUBLE_ELEMENT(schemaUpdate).key("gain")
                    .displayedName("Gain")
                    .description(notAvailable)
                    .readOnly()
                    .commit();
        }

        // Disable features which are unavailable on the camera
        std::vector<std::string> paths;
        this->getPathsByTag(paths, "genicam,poll");
        for (const auto& key : paths) {
            const std::string feature = this->getAliasFromKey<std::string>(key);
            if (!this->isFeatureAvailable(feature)) {
                // This feature is not available on the camera
                this->disableElement(key, feature, schemaUpdate);
            }
        }

        this->appendSchema(schemaUpdate);
        return true; // success
    }


    template <class T>
    void AravisCamera::writeOutputChannels(const void* data, gint width, gint height) {
        const Dims shape(height, width);

        // Non-copy NDArray constructor
        const karabo::util::NDArray imgArray((T*) data, width*height, karabo::util::NDArray::NullDeleter(), shape);

        const unsigned short bpp = this->get<unsigned short>("bpp");
        const Dims binning(this->get<int>("bin.y"), this->get<int>("bin.x"));
        const Dims roiOffsets(this->get<int>("roi.y"), this->get<int>("roi.x"));
        const Timestamp ts = this->getActualTimestamp();
        const Hash header;

        // Send image and metadata to output channel
        this->writeChannels(imgArray, binning, bpp, m_encoding, roiOffsets, ts, header);

        m_counter += 1;
        if (m_timer.elapsed() >= 1.) {
            const float frameRate = m_counter / m_timer.elapsed();
            m_counter = 0;
            m_timer.now();
            this->set("frameRate.actual", frameRate);
        }
    }


    bool AravisCamera::resolveHostname(const std::string& hostname, std::string&ip_address, std::string& message) {
        bool success = false;
        boost::asio::io_service io_service;
        boost::asio::ip::tcp::resolver resolver(io_service);
        const boost::asio::ip::tcp::resolver::query query(hostname, "");
        const boost::asio::ip::tcp::resolver::iterator end;
        boost::system::error_code ec;
        auto it = resolver.resolve(query, ec);
        if (ec != boost::system::errc::success) {
            ip_address = "";
            message = "Boost error in resolveHostname: " + ec.message();
        } else if (it != end) {
            const boost::asio::ip::tcp::endpoint endpoint = it->endpoint();
            success = true;
            ip_address = endpoint.address().to_string();
            message = "IP name resolved: " + hostname + " -> " + ip_address;
        } else {
            ip_address = "";
            message = "Cannot resolve hostname: " + hostname;
        }

        return success;
    }

}
