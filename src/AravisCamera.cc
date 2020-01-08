/*
 * Author: <parenti>
 *
 * Created on September, 2019, 10:46 AM
 *
 * Copyright (c) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "AravisCamera.hh"

using namespace std;

USING_KARABO_NAMESPACES;

namespace karabo {


    KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<>, ImageSource, AravisCamera)

    void AravisCamera::expectedParameters(Schema& expected) {
        STRING_ELEMENT(expected).key("cameraIp")
                .displayedName("Camera IP")
                .description("The IP address of the network camera (eg 131.169.140.193).")  // TODO: hostname
                .assignmentMandatory()
                .init() // cannot be changed after device instantiation
                .commit();

        SLOT_ELEMENT(expected).key("connect")
                .displayedName("Connect")
                .allowedStates(State::UNKNOWN)
                .commit();

        SLOT_ELEMENT(expected).key("acquire")
                .displayedName("Acquire")
                .allowedStates(State::ON)
                .commit();

        SLOT_ELEMENT(expected).key("stop")
                .displayedName("Stop")
                .allowedStates(State::ACQUIRING)
                .commit();

        FLOAT_ELEMENT(expected).key("frameRate")
                .displayedName("Frame Rate")
                .description("The actual frame rate.")
                .unit(Unit::HERTZ)
                .readOnly()
                .initialValue(0.)
                .commit();

        STRING_ELEMENT(expected).key("cameraId")
                .displayedName("Camera ID")
                .readOnly()
                .commit();

        STRING_ELEMENT(expected).key("pixelFormat")
                .displayedName("Pixel Format")
                .assignmentOptional().defaultValue("Mono8")
                .options("Mono8 Mono12 Mono12Packed YUV422Packed YUV422_YUYV_Packed")
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        // TODO more properties
    }


    AravisCamera::AravisCamera(const karabo::util::Hash& config) : ImageSource(config), m_camera(NULL), m_stream(NULL) {
        KARABO_SLOT(connect);
        KARABO_SLOT(acquire);
        KARABO_SLOT(stop);
    }


    AravisCamera::~AravisCamera() {
        this->clear_stream();
        this->clear_camera();
    }


    void AravisCamera::preReconfigure(karabo::util::Hash& incomingReconfiguration) {
        if (incomingReconfiguration.has("pixelFormat")) {
            const char* pixelFormat = incomingReconfiguration.get<std::string>("pixelFormat").c_str();
            arv_camera_set_pixel_format_from_string(m_camera, pixelFormat);
        }
    }


    void AravisCamera::postReconfigure() {
    }


    void AravisCamera::connect() {
        // TODO connect worker

        this->clear_stream();
        this->clear_camera();

        const std::string cameraIp = this->get<std::string>("cameraIp");
        m_camera = arv_camera_new(cameraIp.c_str());

        if (!ARV_IS_CAMERA(m_camera)) {
            KARABO_LOG_ERROR << "Cannot connect to " << cameraIp;
            m_camera = NULL;
            return;
        }
        
        KARABO_LOG_INFO << "Connected to " << cameraIp;

        // Connect the control-lost signal
        g_signal_connect(arv_camera_get_device(m_camera), "control-lost", G_CALLBACK(AravisCamera::control_lost_cb), static_cast<void*>(this));

        Hash h;
        h.set("cameraId", std::string(arv_camera_get_device_id(m_camera)));
        // TODO more properties...
        this->set(h);

        // TODO: this->updateOutputSchema(shape, encoding, kType)

        this->updateState(State::ON);
    }

    
    void AravisCamera::acquire() {
        m_timer.now();
        m_counter = 0;
        m_stream = arv_camera_create_stream(m_camera, AravisCamera::stream_cb, static_cast<void*>(this));

        if (!ARV_IS_STREAM(m_stream)) {
            KARABO_LOG_ERROR << "Stream could not be created";
            return;
        }

        // Enable emission of signals (it's disabled by default for performance reason)
        arv_stream_set_emit_signals(m_stream, TRUE);

        // Create and push buffers to the stream
	const gint payload = arv_camera_get_payload(m_camera);
	for (size_t i = 0; i < 10; i++)
            arv_stream_push_buffer(m_stream, arv_buffer_new(payload, NULL));

        arv_camera_start_acquisition(m_camera);

        // Connect the 'new-buffer' signal
        g_signal_connect(m_stream, "new-buffer", G_CALLBACK(AravisCamera::new_buffer_cb), static_cast<void*>(this));

        this->updateState(State::ACQUIRING);
    }

    
    void AravisCamera::stop() {
        this->clear_stream();

        arv_camera_stop_acquisition(m_camera);

        this->set("frameRate", 0.);
        this->updateState(State::ON);
    }


    void AravisCamera::clear_camera() {
        if (ARV_IS_CAMERA(m_camera)) {
            g_clear_object(&m_camera);
        }
    }


    void AravisCamera::clear_stream() {
        if (ARV_IS_STREAM(m_stream)) {
            // TODO possibly disconnect signal, see https://developer.gnome.org/gobject/stable/gobject-Signals.html#g-signal-handler-disconnect

            // Disable emission of signals and free resource
            arv_stream_set_emit_signals(m_stream, FALSE);
            g_clear_object(&m_stream);
        }
    }


    void AravisCamera::stream_cb(void *context, ArvStreamCallbackType type, ArvBuffer *buffer) {
        Self* self = static_cast<Self*>(context);

        // TODO proper logging
        if (type == ARV_STREAM_CALLBACK_TYPE_INIT) {
            std::cout << "Init stream" << std::endl;
                if (!arv_make_thread_realtime(10) && !arv_make_thread_high_priority(-10)) {
                    std::cout << "Failed to make stream thread high priority" << std::endl;
                }
        }
    }


    void AravisCamera::new_buffer_cb(ArvStream* stream, void* context) {
        Self* self = static_cast<Self*>(context);

        // TODO proper logging

        ArvBuffer* arv_buffer = arv_stream_pop_buffer(stream);
	if (!ARV_IS_BUFFER(arv_buffer))
		return;

        if (arv_buffer_get_status(arv_buffer) == ARV_BUFFER_STATUS_SUCCESS) {
            gint x, y, width, height;
            size_t buffer_size;
            
            const void* buffer_data = arv_buffer_get_data(arv_buffer, &buffer_size);
            arv_buffer_get_image_region(arv_buffer, &x, &y, &width, &height);
            const ArvPixelFormat pixel_format = arv_buffer_get_image_pixel_format(arv_buffer); // e.g. ARV_PIXEL_FORMAT_MONO_8
            const guint32 frame_id = arv_buffer_get_frame_id(arv_buffer);
            //std::cout << "Got frame " << frame_id << std::endl;

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
                // TODO PACKED formats
                default:
                    std::cout << "Format " << pixel_format << " is not supported"  << std::endl;
                    self->execute("stop");
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

        // TODO proper logging
        std::cout << "Control of the camera is lost" << std::endl;
        self->updateState(State::UNKNOWN);
    }


    template <class T>
    void AravisCamera::writeOutputChannels(const void* data, gint width, gint height) {
        const Dims shape(height, width);

        // Non-copy NDArray constructor
        karabo::util::NDArray imgArray((T*) data, width*height, karabo::util::NDArray::NullDeleter(), shape);

        // TODO bpp, binning, ROI and encoding from camera configuration
        const unsigned short bpp = 8 * sizeof(T);
        const karabo::util::Dims binning(0,0), roiOffsets(0, 0);
        const Timestamp ts;
        const Hash header;

        // Send image and metadata to output channel
        this->writeChannels(imgArray, binning, bpp, Encoding::GRAY, roiOffsets, ts, header);

        m_counter += 1;
        if (m_timer.elapsed() >= 1.) {
            const float frameRate = m_counter / m_timer.elapsed();
            m_counter = 0;
            m_timer.now();
            this->set("frameRate", frameRate);
        }
    }

}
