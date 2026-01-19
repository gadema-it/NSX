#include "application.h"
#include "mainwindow.h"
#include "material.h"

#include <QSurfaceFormat>
#include <QStyleFactory>
#include <QOpenGLFunctions>
#include <QOffscreenSurface>

int main(int argc, char *argv[])
{   
    QSurfaceFormat format;
    //format.setSwapInterval(0); // TODO https://stackoverflow.com/questions/5829881/avoid-waiting-on-swapbuffers
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    //format.setVersion(4, 2);
    format.setProfile(QSurfaceFormat::CompatibilityProfile);

    QSurfaceFormat::setDefaultFormat(format);

    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    Application &application = Application::instance();
    application.setStyle(QStyleFactory::create("windows"));

//    QOpenGLContext openGLContext;
//    openGLContext.setFormat(format);
//    openGLContext.create();

//    QOffscreenSurface *offscreen = new QOffscreenSurface();
//    offscreen->setFormat(format);
//    offscreen->create();
//    qDebug() << offscreen->isValid();
//    qDebug() << offscreen->format();
//    openGLContext.makeCurrent(offscreen);

//    Material *m = new Material();

    MainWindow w;
    application.mainWindow = &w;
    application.init();
    w.show();

    return application.exec();
}
