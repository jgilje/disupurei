#ifndef GSTPIPELINE_H
#define GSTPIPELINE_H

#include <QThread>
#include <QOpenGLWidget>

#define GST_USE_UNSTABLE_API
#include <gst/gl/gstglcontext.h>
#include <gst/gl/gstgldisplay.h>

#include <asyncqueue.h>

class GstreamerPipeline : public QObject
{
    Q_OBJECT
public:
    GstreamerPipeline();
    ~GstreamerPipeline();

    void initialize(QOpenGLContext *context);
    void open(const QString& filename) { emit openFileRequested(filename); }
    void notifyNewFrame() {emit newFrameReady();}
    void stop();

    AsyncQueue<GstBuffer*> queue_input_buf;
    AsyncQueue<GstBuffer*> queue_output_buf;
signals:
    void finished();
    void newFrameReady();
    void videoSize(int width, int height);

    void openFileRequested(const QString& filename);
public slots:
private:
    enum class PipelineState {
        STOPPED,
        PAUSED,
        PLAYING
    };

    PipelineState _state = PipelineState::STOPPED;
    GstBus* m_bus;

    GstPipeline* _pipeline = nullptr;
    GstElement* _filesrc = nullptr;
    GstElement* _glupload = nullptr;

    GstGLDisplay* _display;
    GstGLContext* _context;

    static void on_gst_buffer(GstElement * element, GstBuffer * buf, GstPad * pad, GstreamerPipeline* p);
    static gboolean bus_call (GstBus *bus, GstMessage *msg, GstreamerPipeline* p);
    static gboolean sync_bus_call (GstBus *bus, GstMessage *msg, GstreamerPipeline* p);

    void _signalFinished();
    void _startPipeline();
    void _pausePipeline();
    void _stopPipeline();
private slots:
    void _open(const QString& filename);
};

#endif // GSTPIPELINE_H
