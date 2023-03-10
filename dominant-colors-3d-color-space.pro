#-------------------------------------------------
#
#   Dominant colors from image with openCV
#               in 3D color spaces
#
#    by AbsurdePhoton - www.absurdephoton.fr
#
#                v2.3 - 2023/03/10
#
#-------------------------------------------------

QT += core gui openglwidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = dominant-colors-3d-color-space
TEMPLATE = app

#INCLUDEPATH += /usr/local/include/opencv4/opencv2

LIBS += -fopenmp -ltbb -ffast-math -fno-math-errno

SOURCES +=  main.cpp\
            mainwindow.cpp \
            openglwidget.cpp \
            opengl-draw.cpp \
            widgets/file-dialog.cpp \
            lib/dominant-colors.cpp \
            lib/color-spaces.cpp \
            lib/angles.cpp \
            lib/image-transform.cpp \
            lib/image-color.cpp \
            lib/image-lut.cpp \
            lib/image-utils.cpp

HEADERS  += mainwindow.h \
            openglwidget.h \
            opengl-draw.h \
            palette.h \
            widgets/file-dialog.h \
            lib/dominant-colors.h \
            lib/color-spaces.h \
            lib/angles.h \
            lib/image-transform.h \
            lib/image-color.h \
            lib/image-utils.h \
            lib/image-lut.h \
            lib/randomizer.h

FORMS    += mainwindow.ui

# we add the package opencv to pkg-config
CONFIG += link_pkgconfig
PKGCONFIG += opencv4

# icons
RESOURCES += resources.qrc

CONFIG += c++17

# openMP
QMAKE_LFLAGS += -fopenmp
QMAKE_CXXFLAGS += -fopenmp

# optimizations
QMAKE_CXXFLAGS += -ffast-math -march=native -mtune=intel -msse4.2 -ftree-vectorize -mavx

# optimization level
#QMAKE_CXXFLAGS_RELEASE -= -O2
#QMAKE_CXXFLAGS_RELEASE += -O3
