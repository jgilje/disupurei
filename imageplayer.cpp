#include "imageplayer.h"

#include <QDateTime>

#define PROGRAM_VERTEX_ATTRIBUTE 0
#define PROGRAM_TEXCOORD_ATTRIBUTE 1

ImagePlayer::ImagePlayer(QWidget *parent) : QOpenGLWidget(parent),
    _texture(QOpenGLTexture::Target2D) {
    _texture.setAutoMipMapGenerationEnabled(false);

    _timer.setSingleShot(true);
    connect(&_timer, &QTimer::timeout, this, &ImagePlayer::_timeout);
    connect(&_faderTimer, &QTimer::timeout, this, &ImagePlayer::_fade);
}

ImagePlayer::~ImagePlayer() {
    makeCurrent();

    _texture.destroy();

    doneCurrent();
}

void ImagePlayer::open(const QString &filename, int duration) {
    makeCurrent();
    _duration = duration;

    _texture.destroy();
    _texture.setData(QImage(filename));

    _fader = 1.0f;
    _fade_dir = false;
    _fade_start = QDateTime::currentMSecsSinceEpoch();

    _faderTimer.start(1000 / 60);
    doneCurrent();
}

void ImagePlayer::stop() {

}

void ImagePlayer::initializeGL() {
    initializeOpenGLFunctions();
    _makeObject();

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
            "uniform mediump float fader;\n"
            "varying mediump vec4 texc;\n"
            "void main(void)\n"
            "{\n"
            "    gl_FragColor = mix(texture2D(texture, texc.st), vec4(1.0, 1.0, 1.0, 1.0), fader);\n"
            "}\n";
    fshader->compileSourceCode(fsrc);

    _program = std::unique_ptr<QOpenGLShaderProgram>(new QOpenGLShaderProgram);
    _program->addShader(vshader);
    _program->addShader(fshader);
    _program->bindAttributeLocation("vertex", PROGRAM_VERTEX_ATTRIBUTE);
    _program->bindAttributeLocation("texCoord", PROGRAM_TEXCOORD_ATTRIBUTE);
    _program->link();

    _program->bind();
    _program->setUniformValue("texture", 0);

    _texture.setData(QImage());
}

void ImagePlayer::paintGL() {
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    QMatrix4x4 m;
    _program->setUniformValue("matrix", m);
    _program->enableAttributeArray(PROGRAM_VERTEX_ATTRIBUTE);
    _program->enableAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE);
    _program->setAttributeBuffer(PROGRAM_VERTEX_ATTRIBUTE, GL_FLOAT, 0, 3, 5 * sizeof(GLfloat));
    _program->setAttributeBuffer(PROGRAM_TEXCOORD_ATTRIBUTE, GL_FLOAT, 3 * sizeof(GLfloat), 2, 5 * sizeof(GLfloat));
    _program->setUniformValue("fader", _fader);

    _texture.bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void ImagePlayer::resizeGL(int width, int height) {
    glViewport(0, 0, width, height);
    _makeObject();
}

void ImagePlayer::_makeObject() {
    static const GLfloat coords[6][3] =
        { { -1.0f, -1.0f,  0.0f },
          {  1.0f, -1.0f,  0.0f },
          { -1.0f,  1.0f,  0.0f },
          { -1.0f,  1.0f,  0.0f },
          {  1.0f,  1.0f,  0.0f },
          {  1.0f, -1.0f,  0.0f } };

    QVector<GLfloat> vertData;
    for (int i = 0; i < 6; i++) {
        // vertex position
        vertData.append(coords[i][0]);
        vertData.append(coords[i][1]);
        vertData.append(coords[i][2]);
        // texture coordinate
        vertData.append(coords[i][0] > 0 ? 1 : 0);
        vertData.append(coords[i][1] > 0 ? 0 : 1);
    }

    if (! _vbo.isCreated()) {
        _vbo.create();
    }

    _vbo.bind();
    _vbo.allocate(vertData.constData(), vertData.count() * sizeof(GLfloat));
}

void ImagePlayer::_fade() {
    float diff = QDateTime::currentMSecsSinceEpoch() - _fade_start;
    if (_fade_dir) {
        if (diff > _fade_time) {
            _fader = 1.0f;
            _faderTimer.stop();
            emit timeout();
        } else {
            _fader = (diff / _fade_time);
        }
    } else {
        if (diff > _fade_time) {
            _fader = 0.0f;
            _faderTimer.stop();
            _timer.start(_duration);
        } else {
            _fader = 1.0f - (diff / _fade_time);
        }
    }
    update();
}

void ImagePlayer::_timeout() {
    _fader = 0.0f;
    _fade_dir = true;
    _fade_start = QDateTime::currentMSecsSinceEpoch();

    _faderTimer.start(1000 / 60);
}
