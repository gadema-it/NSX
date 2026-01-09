QT += core gui widgets

CONFIG += c++11
#CONFIG+=WITH_OPENSUBDIV

DEFINES += QT_DEPRECATED_WARNINGS

INCLUDEPATH += $$PWD/src

HEADERS += \
    src/application.h \
    src/camera.h \
    src/commands/deletecommand.h \
    src/geometry.h \
    src/locator.h \
    src/mainwindow.h \
    src/material.h \
    src/memorypool.h \
    src/mesh.h \
    src/tools/edgetool.h \
    src/tools/navigationtool.h \
    src/object3d.h \
    src/scene.h \
    src/scenetreemodel.h \
    src/selection.h \
    src/tools/polygontool.h \
    src/transform3d.h \
    src/uveditor.h \
    src/uvviewport.h \
    src/viewport.h \
    src/viewportaxis.h \
    src/viewportgrid.h

SOURCES += \
    src/application.cpp \
    src/camera.cpp \
    src/commands/deletecommand.cpp \
    src/geometry.cpp \
    src/locator.cpp \
    src/main.cpp \
    src/mainwindow.cpp \
    src/material.cpp \
    src/mesh.cpp \
    src/object3d.cpp \
    src/scene.cpp \
    src/scenetreemodel.cpp \
    src/selection.cpp \
    src/tools/edgetool.cpp \
    src/tools/polygontool.cpp \
    src/uveditor.cpp \
    src/uvviewport.cpp \
    src/viewport.cpp \
    src/viewportaxis.cpp \
    src/viewportgrid.cpp \
    src/transform3d.cpp \


# ---------------------------------------- tools

HEADERS += \
    src/tools/selectiontool.h \
    src/tools/tool.h \
    src/tools/transformtool.h \

SOURCES += \
    src/tools/tool.cpp \
    src/tools/transformtool.cpp \
    src/tools/navigationtool.cpp \
    src/tools/selectiontool.cpp \


# ---------------------------------------- commands

HEADERS += \
    src/commands/command.h \
    src/commands/setpropertycommand.h \
    src/commands/splitedgecommand.h \
    src/commands/translatecommand.h \

SOURCES += \
    src/commands/command.cpp \
    src/commands/setpropertycommand.cpp \
    src/commands/splitedgecommand.cpp \
    src/commands/translatecommand.cpp \


# ---------------------------------------- operators

# ---


FORMS += \
    src/mainwindow.ui
	

RESOURCES += \
    shaders.qrc


win32 {

    LIBS += opengl32.lib

    WITH_OPENSUBDIV  {
        INCLUDEPATH += $$PWD/3rdparty/OpenSubdiv/opensubdiv
        DEFINES += WITH_OPENSUBDIV
        CONFIG( debug, debug|release ) {
            LIBS += $$PWD/3rdparty/OpenSubdiv/build/opensubdiv/sdc/sdc_obj.dir/Debug/sdc_obj.lib
            LIBS += $$PWD/3rdparty/OpenSubdiv/build/opensubdiv/vtr/vtr_obj.dir/Debug/vtr_obj.lib
            LIBS += $$PWD/3rdparty/OpenSubdiv/build/opensubdiv/far/far_obj.dir/Debug/far_obj.lib
        } else {
            LIBS += $$PWD/3rdparty/OpenSubdiv/build/opensubdiv/sdc/sdc_obj.dir/Release/sdc_obj.lib
            LIBS += $$PWD/3rdparty/OpenSubdiv/build/opensubdiv/vtr/vtr_obj.dir/Release/vtr_obj.lib
            LIBS += $$PWD/3rdparty/OpenSubdiv/build/opensubdiv/far/far_obj.dir/Release/far_obj.lib
        }
    }
}


#INSTALLS += target

# Default rules for deployment.
#qnx: target.path = /tmp/$${TARGET}/bin
#else: unix:!android: target.path = /opt/$${TARGET}/bin
#!isEmpty(target.path): INSTALLS += target

#include(src/mesh/mesh.pri)


##win32:CONFIG(release, debug|release): LIBS += -L$$PWD/lib/OpenSubdiv/bin/opensubdiv/far/far_obj.dir/release/ -lfar_obj
#win32:CONFIG(debug, debug|release): LIBS += $$PWD/lib/OpenSubdiv/bin/opensubdiv/far/far_obj.dir/debug/far_obj.lib
##else:unix: LIBS += -L$$PWD/lib/OpenSubdiv/bin/opensubdiv/far/far_obj.dir/ -lfar_obj

#INCLUDEPATH += $$PWD/lib/OpenSubdiv/opensubdiv
##DEPENDPATH += $$PWD/lib/OpenSubdiv/opensubdiv

#win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/lib/OpenSubdiv/bin/opensubdiv/far/far_obj.dir/release/libfar_obj.a
#else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/lib/OpenSubdiv/bin/opensubdiv/far/far_obj.dir/debug/libfar_obj.a
#else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/lib/OpenSubdiv/bin/opensubdiv/far/far_obj.dir/release/far_obj.lib
#else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/lib/OpenSubdiv/bin/opensubdiv/far/far_obj.dir/debug/far_obj.lib
#else:unix: PRE_TARGETDEPS += $$PWD/lib/OpenSubdiv/bin/opensubdiv/far/far_obj.dir/libfar_obj.a

#win32:CONFIG(release, debug|release): LIBS += -L$$PWD/lib/OpenSubdiv/bin/opensubdiv/far/far_obj.dir/release/ -lfar_obj
#else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/lib/OpenSubdiv/bin/opensubdiv/far/far_obj.dir/debug/ -lfar_obj
#else:unix: LIBS += -L$$PWD/lib/OpenSubdiv/bin/opensubdiv/far/far_obj.dir/ -lfar_obj

#INCLUDEPATH += $$PWD/lib/OpenSubdiv/bin/opensubdiv/far/far_obj.dir/Debug
#DEPENDPATH += $$PWD/lib/OpenSubdiv/bin/opensubdiv/far/far_obj.dir/Debug
