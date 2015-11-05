#include "videoplayer.h"

#include <QGuiApplication>
#include <QTimer>

#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QMouseEvent>

#define PROGRAM_VERTEX_ATTRIBUTE 0
#define PROGRAM_TEXCOORD_ATTRIBUTE 1

VideoPlayer::VideoPlayer(QWidget *parent)
    : QOpenGLWidget(parent),
      clearColor(Qt::black)
{
}

VideoPlayer::~VideoPlayer() {
    makeCurrent();
    vbo.destroy();

    delete program;
    doneCurrent();
}

void VideoPlayer::open(const QString &filename) {
    _pipeline->open(filename);
}

void VideoPlayer::stop() {
    _pipeline->stop();
}

QSize VideoPlayer::minimumSizeHint() const
{
    return QSize(50, 50);
}

QSize VideoPlayer::sizeHint() const
{
    return QSize(200, 200);
}

void VideoPlayer::setClearColor(const QColor &color)
{
    clearColor = color;
    update();
}

void VideoPlayer::initializeGL()
{
    initializeOpenGLFunctions();
    makeObject();

    QOpenGLShader *vshader = new QOpenGLShader(QOpenGLShader::Vertex, this);
    const char *vsrc =
            "attribute highp vec4 vertex;\n"
            "attribute mediump vec4 texCoord;\n"
            "varying mediump vec4 texc;\n"
            "uniform mediump mat4 matrix;\n"
            "void main(void)\n"
            "{\n"
            "    gl_Position = matrix * vertex;\n"
            "    texc = texCoord;\n"
            "}\n";
    vshader->compileSourceCode(vsrc);

    QOpenGLShader *fshader = new QOpenGLShader(QOpenGLShader::Fragment, this);
    const char *fsrc =
            "uniform sampler2D texture;\n"
            "varying mediump vec4 texc;\n"
            "void main(void)\n"
            "{\n"
            "    gl_FragColor = texture2D(texture, texc.st);\n"
            "}\n";
    fshader->compileSourceCode(fsrc);

    program = new QOpenGLShaderProgram;
    program->addShader(vshader);
    program->addShader(fshader);
    program->bindAttributeLocation("vertex", PROGRAM_VERTEX_ATTRIBUTE);
    program->bindAttributeLocation("texCoord", PROGRAM_TEXCOORD_ATTRIBUTE);
    program->link();

    program->bind();
    program->setUniformValue("texture", 0);
}

void VideoPlayer::paintGL() {
    glClearColor(clearColor.redF(), clearColor.greenF(), clearColor.blueF(), clearColor.alphaF());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    QMatrix4x4 m;
    // m.perspective(M_PI / 2.0f, 16.0f / 9.0f, 0.1f, 1000.0f);
    // m.ortho(-0.5f, +0.5f, +0.5f, -0.5f, 4.0f, 15.0f);
    // m.translate(0.0f, 0.0f, -15.0f);

    program->setUniformValue("matrix", m);
    program->enableAttributeArray(PROGRAM_VERTEX_ATTRIBUTE);
    program->enableAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE);
    program->setAttributeBuffer(PROGRAM_VERTEX_ATTRIBUTE, GL_FLOAT, 0, 3, 5 * sizeof(GLfloat));
    program->setAttributeBuffer(PROGRAM_TEXCOORD_ATTRIBUTE, GL_FLOAT, 3 * sizeof(GLfloat), 2, 5 * sizeof(GLfloat));
    // (int location, GLenum type, int offset, int tupleSize, int stride = 0);

    if (frame) {
        guint tex_id;
        GstVideoInfo v_info;
        GstVideoFrame v_frame;
        GstVideoMeta *v_meta;

        v_meta = gst_buffer_get_video_meta (this->frame);
        gst_video_info_set_format (&v_info, v_meta->format, v_meta->width, v_meta->height);
        gst_video_frame_map (&v_frame, &v_info, this->frame, (GstMapFlags) (GST_MAP_READ | GST_MAP_GL));
        tex_id = *(guint *) v_frame.data[0];

        glBindTexture (GL_TEXTURE_2D, tex_id);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        gst_video_frame_unmap (&v_frame);
    }
}

void VideoPlayer::resizeGL(int width, int height) {
    glViewport(0, 0, width, height);

    makeObject();
}

void VideoPlayer::makeObject() {
    static const GLfloat coords[6][3] =
        { { -1.0f, -1.0f,  0.0f },
          {  1.0f, -1.0f,  0.0f },
          { -1.0f,  1.0f,  0.0f },
          { -1.0f,  1.0f,  0.0f },
          {  1.0f,  1.0f,  0.0f },
          {  1.0f, -1.0f,  0.0f } };

    QVector<GLfloat> vertData;

    double scaledWidth = (double) width() / (double) _videoWidth;
    double scaledHeight = (double) height() / (double) _videoHeight;

    if (scaledWidth < scaledHeight) {
        double modHeight = ((double) _videoHeight * (double) width()) / (double) _videoWidth;
        double totalHeight = (double) modHeight / (double) height();

        for (int i = 0; i < 6; i++) {
            // vertex position
            vertData.append(coords[i][0]);
            vertData.append(coords[i][1] * totalHeight);
            vertData.append(coords[i][2]);
            // texture coordinate
            vertData.append(coords[i][0] > 0 ? 1 : 0);
            vertData.append(coords[i][1] > 0 ? 0 : 1);
        }
    } else if (scaledWidth > scaledHeight) {
        double modWidth = ((double) _videoWidth * (double) height()) / (double) _videoHeight;
        double totalWidth = (double) modWidth / (double) width();

        for (int i = 0; i < 6; i++) {
            // vertex position
            vertData.append(coords[i][0] * totalWidth);
            vertData.append(coords[i][1]);
            vertData.append(coords[i][2]);
            // texture coordinate
            vertData.append(coords[i][0] > 0 ? 1 : 0);
            vertData.append(coords[i][1] > 0 ? 0 : 1);
        }
    } else {
        for (int i = 0; i < 6; i++) {
            // vertex position
            vertData.append(coords[i][0]);
            vertData.append(coords[i][1]);
            vertData.append(coords[i][2]);
            // texture coordinate
            vertData.append(coords[i][0] > 0 ? 1 : 0);
            vertData.append(coords[i][1] > 0 ? 0 : 1);
        }
    }

    // new
    if (! vbo.isCreated()) {
        vbo.create();
    }

    vbo.bind();
    vbo.allocate(vertData.constData(), vertData.count() * sizeof(GLfloat));
}

void VideoPlayer::videoSize(int width, int height) {
    _videoWidth = width;
    _videoHeight = height;

    makeObject();
}

void VideoPlayer::newFrame () {
    /* frame is initialized as null */
    if (this->frame)
        _pipeline->queue_output_buf.put(this->frame);

    this->frame = _pipeline->queue_input_buf.get ();

    /* direct call to paintGL (no queued) */
    update();
}

void VideoPlayer::initPipeline() {
    if (! _pipeline) {
        _pipeline = std::unique_ptr<GstreamerPipeline>(new GstreamerPipeline());
        _pipeline->initialize(context());
        connect(_pipeline.get(), &GstreamerPipeline::newFrameReady, this, &VideoPlayer::newFrame, Qt::QueuedConnection);
        connect(_pipeline.get(), &GstreamerPipeline::videoSize, this, &VideoPlayer::videoSize, Qt::QueuedConnection);
        connect(_pipeline.get(), &GstreamerPipeline::finished, this, &VideoPlayer::finished, Qt::QueuedConnection);
    }
}
