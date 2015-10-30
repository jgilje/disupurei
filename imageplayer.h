#ifndef IMAGEPLAYER_H
#define IMAGEPLAYER_H

#include <memory>

#include <QTimer>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QOpenGLShaderProgram>

class ImagePlayer : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT
public:
    explicit ImagePlayer(QWidget *parent = 0);
    ~ImagePlayer();

    void open(const QString& filename, int duration);
    void stop();
signals:
    void timeout();

public slots:

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;

private:
    QTimer _timer;
    QTimer _faderTimer;

    QOpenGLBuffer _vbo;
    QOpenGLTexture _texture;
    std::unique_ptr<QOpenGLShaderProgram> _program;

    int _duration = 0;
    float _fader = 1.0f;
    float _fade_time = 2500.0f;
    qint64 _fade_start = 0;
    bool _fade_dir = false;
    void _makeObject();
private slots:
    void _fade();
    void _timeout();
};

#endif // IMAGEPLAYER_H
