#-------------------------------------------------
#
#   Dominant colors from image with openCV
#               in 3D color spaces
#
#    by AbsurdePhoton - www.absurdephoton.fr
#
#                v0.1 - 2019/10/24
#
#-------------------------------------------------

QT += core gui opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = dominant-colors-3d-color-space
TEMPLATE = app

INCLUDEPATH += /usr/local/include/opencv2

LIBS += -L/usr/local/lib -lopencv_core -lopencv_imgcodecs -lopencv_highgui

SOURCES += main.cpp\
        mainwindow.cpp \
        mat-image-tools.cpp \
        dominant-colors.cpp \
        openglwidget.cpp \
        opengl-draw.cpp

HEADERS  += mainwindow.h \
            mat-image-tools.h \
            dominant-colors.h \
            openglwidget.h \
            opengl-draw.h

FORMS    += mainwindow.ui

# we add the package opencv to pkg-config
CONFIG += link_pkgconfig
PKGCONFIG += opencv4

QMAKE_CXXFLAGS += -std=c++11

# icons
RESOURCES += resources.qrc
