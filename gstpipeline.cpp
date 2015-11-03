#include <QDebug>
#include <QOpenGLContext>

#include "gstpipeline.h"

#define GST_USE_UNSTABLE_API
#include <gst/gl/gstglconfig.h>
#include <gst/gl/gstglcontext.h>
#include <gst/gl/gstgldisplay.h>

#if GST_GL_HAVE_PLATFORM_EGL
	#include <QtPlatformHeaders/QEGLNativeContext>
	#include <gst/gl/egl/gstgldisplay_egl.h>
#endif

#if GST_GL_HAVE_PLATFORM_GLX
	#include <QtX11Extras/QX11Info>
	#include <QtPlatformHeaders/QGLXNativeContext>
	#include <gst/gl/x11/gstgldisplay_x11.h>
#endif

#if GST_GL_HAVE_PLATFORM_WGL
	#include <QtPlatformHeaders/QWGLNativeContext>
#endif

GstreamerPipeline::GstreamerPipeline() {
    setObjectName("GstreamerPipeline");

    connect(this, &GstreamerPipeline::openFileRequested, this, &GstreamerPipeline::_open);
}

GstreamerPipeline::~GstreamerPipeline() {
    stop();
}

void GstreamerPipeline::initialize(QOpenGLContext *context) {
    GstGLDisplay *gstDisplay = nullptr;
    GstGLContext *gstContext = nullptr;

    QVariant handle = context->nativeHandle();
#if GST_GL_HAVE_PLATFORM_GLX
    gstDisplay = (GstGLDisplay *) gst_gl_display_x11_new_with_display (QX11Info::display());
    if (handle.canConvert<QGLXNativeContext>()) {
        QGLXNativeContext glxContext = handle.value<QGLXNativeContext>();
        gstContext = gst_gl_context_new_wrapped(gstDisplay, (guintptr) glxContext.context(), GST_GL_PLATFORM_GLX, GST_GL_API_OPENGL);

        // display = (GstGLDisplay *) gst_gl_display_x11_new_with_display(glxContext.display());
    }
#endif
#if GST_GL_HAVE_PLATFORM_EGL
    if (handle.canConvert<QEGLNativeContext>()) {
        QEGLNativeContext eglContext = handle.value<QEGLNativeContext>();
        gstDisplay = (GstGLDisplay *) gst_gl_display_egl_new_with_egl_display(eglContext.display());
        gstContext = gst_gl_context_new_wrapped(gstDisplay, (guintptr) eglContext.context(), GST_GL_PLATFORM_EGL, GST_GL_API_GLES2);
    }
#endif
#if GST_GL_HAVE_PLATFORM_WGL
    if (handle.canConvert<QWGLNativeContext>()) {
        QWGLNativeContext wglContext = handle.value<QWGLNativeContext>();
        gstDisplay = (GstGLDisplay *) gst_gl_display_new();
        gstContext = gst_gl_context_new_wrapped(gstDisplay, (guintptr) wglContext.context(), GST_GL_PLATFORM_WGL, GST_GL_API_OPENGL);
    }
#endif
    if (gstDisplay == nullptr) {
        qWarning() << Q_FUNC_INFO << "Couldn't find display";
        return;
    }

    if (gstContext == nullptr) {
        qWarning() << Q_FUNC_INFO << "Couldn't find context";
        return;
    }

    _display = gstDisplay;
    _context = gstContext;
}

void GstreamerPipeline::stop() {
    if (_state == PipelineState::PLAYING || _state == PipelineState::PAUSED) {
        _stopPipeline();
    }
}

void GstreamerPipeline::_open(const QString &filename) {
    _pipeline = GST_PIPELINE (gst_parse_launch
      ("filesrc name=filesrc ! "
       "decodebin ! "
       "glupload name=glupload ! "
       "glcolorconvert ! "
       "video/x-raw(memory:GLMemory), format=(string)RGBA ! "
       "fakesink name=fakesink sync=1", NULL));

    _filesrc = gst_bin_get_by_name(GST_BIN(_pipeline), "filesrc");
    g_assert(_filesrc != nullptr);

    _glupload = gst_bin_get_by_name(GST_BIN(_pipeline), "glupload");
    g_assert(_glupload != nullptr);

    m_bus = gst_pipeline_get_bus (GST_PIPELINE (_pipeline));
    gst_bus_add_watch (m_bus, (GstBusFunc) bus_call, this);
    gst_bus_enable_sync_message_emission (m_bus);
    g_signal_connect (m_bus, "sync-message", G_CALLBACK (sync_bus_call), this);
    gst_object_unref (m_bus);

    // set a callback to retrieve the gst gl textures
    GstElement *fakesink = gst_bin_get_by_name (GST_BIN (_pipeline), "fakesink");
    g_object_set (G_OBJECT (fakesink), "signal-handoffs", TRUE, NULL);
    g_signal_connect (fakesink, "handoff", G_CALLBACK (on_gst_buffer), this);
    gst_object_unref (fakesink);

    QByteArray bytes = filename.toUtf8();
    g_object_set(_filesrc, "location", bytes.constData(), NULL);

    _pausePipeline();
    _startPipeline();
}

void GstreamerPipeline::_pausePipeline() {
    gst_element_set_state (GST_ELEMENT (_pipeline), GST_STATE_PAUSED);
    GstState state = GST_STATE_PAUSED;
    GstStateChangeReturn stateChange = gst_element_get_state(GST_ELEMENT (_pipeline), &state, NULL, GST_CLOCK_TIME_NONE);
    if (stateChange == GST_STATE_CHANGE_FAILURE && state != GST_STATE_READY) {
        qDebug() << "failed to pause pipeline" << state;
        return;
    }

    GstPad* pad = gst_element_get_static_pad(_glupload, "src");
    GstCaps* caps = gst_pad_get_current_caps(pad);
    for (guint i = 0; i < gst_caps_get_size(caps); i++) {
        GstStructure *structure = gst_caps_get_structure(caps, i);

        auto width = gst_structure_get_value(structure, "width");
        auto height = gst_structure_get_value(structure, "height");

        emit videoSize(g_value_get_int(width), g_value_get_int(height));
    }

    _state = PipelineState::PAUSED;
}

void GstreamerPipeline::_startPipeline() {
    GstStateChangeReturn ret = gst_element_set_state (GST_ELEMENT (_pipeline), GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        qDebug ("Failed to start up pipeline!");

        /* check if there is an error message with details on the bus */
        GstMessage *msg = gst_bus_poll (this->m_bus, GST_MESSAGE_ERROR, 0);
        if (msg) {
            GError *err = NULL;
            gst_message_parse_error (msg, &err, NULL);
            qDebug ("ERROR: %s", err->message);
            g_error_free (err);
            gst_message_unref (msg);
        }
        return;
    }

    _state = PipelineState::PLAYING;
}

void GstreamerPipeline::_stopPipeline() {
    gst_element_set_state (GST_ELEMENT (_pipeline), GST_STATE_NULL);
    GstState state = GST_STATE_NULL;
    if (gst_element_get_state (GST_ELEMENT (_pipeline), &state, NULL, GST_CLOCK_TIME_NONE) != GST_STATE_CHANGE_SUCCESS) {
        qDebug ("failed to stop pipeline");
        return;
    }

    m_bus = gst_pipeline_get_bus (GST_PIPELINE (_pipeline));
    gst_bus_remove_watch(m_bus);
    gst_object_unref (m_bus);

    if (_glupload != nullptr) {
        gst_object_unref(_glupload);
    }
    if (_filesrc != nullptr) {
        gst_object_unref(_filesrc);
    }
    if (_pipeline != nullptr) {
        gst_object_unref(_pipeline);
    }

    _state = PipelineState::STOPPED;
}

/* fakesink handoff callback */
void GstreamerPipeline::on_gst_buffer (GstElement * element, GstBuffer * buf, GstPad * pad, GstreamerPipeline * p) {
    Q_UNUSED (pad)
    Q_UNUSED (element)

    /* ref then push buffer to use it in qt */
    gst_buffer_ref (buf);
    p->queue_input_buf.put (buf);

    if (p->queue_input_buf.size () > 3)
        p->notifyNewFrame ();

    /* pop then unref buffer we have finished to use in qt */
    if (p->queue_output_buf.size () > 3) {
        GstBuffer *buf_old = (p->queue_output_buf.get ());
        if (buf_old)
            gst_buffer_unref (buf_old);
    }
}

gboolean GstreamerPipeline::bus_call (GstBus * bus, GstMessage * msg, GstreamerPipeline * p) {
    Q_UNUSED (bus)

    switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_EOS:
        qDebug ("End-of-stream received. Stopping.");
        p->_stopPipeline();
        p->_signalFinished();
        break;

    case GST_MESSAGE_ERROR:
    {
        gchar *debug = NULL;
        GError *err = NULL;
        gst_message_parse_error (msg, &err, &debug);
        qDebug ("Error: %s", err->message);
        g_error_free (err);
        if (debug) {
            qDebug ("Debug deails: %s", debug);
            g_free (debug);
        }

        p->_stopPipeline();
        break;
    }

    default:
        break;
    }

    return TRUE;
}

gboolean GstreamerPipeline::sync_bus_call (GstBus * bus, GstMessage * msg, GstreamerPipeline * p) {
    Q_UNUSED(bus)

    switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_NEED_CONTEXT: {
        const gchar *context_type;

        gst_message_parse_context_type (msg, &context_type);
        // g_print ("got need context %s\n", context_type);

        if (g_strcmp0 (context_type, GST_GL_DISPLAY_CONTEXT_TYPE) == 0) {
            GstContext *display_context = gst_context_new (GST_GL_DISPLAY_CONTEXT_TYPE, TRUE);
            gst_context_set_gl_display (display_context, p->_display);
            gst_element_set_context (GST_ELEMENT (msg->src), display_context);
        } else if (g_strcmp0 (context_type, "gst.gl.app_context") == 0) {
            GstContext *app_context = gst_context_new ("gst.gl.app_context", TRUE);
            GstStructure *s = gst_context_writable_structure (app_context);
            gst_structure_set (s, "context", GST_GL_TYPE_CONTEXT, p->_context, NULL);
            gst_element_set_context (GST_ELEMENT (msg->src), app_context);
        }
        break;
    }

    default:
        break;
    }
    return FALSE;
}

void GstreamerPipeline::_signalFinished() {
    emit finished();
}
