TEMPLATE = app
TARGET = updater-qt

CONFIG += thread -w
QT += network

greaterThan(QT_MAJOR_VERSION,4): QT += widgets

OBJECTS_DIR = build
MOC_DIR = build
UI_DIR = build

HEADERS = src/updater.h
SOURCES = src/updater.cpp
RESOURCES = src/updater.qrc
FORMS = src/updaterForm.ui

# platform specific defaults, if not overridden on command line
isEmpty(BOOST_LIB_SUFFIX){
  macx:BOOST_LIB_SUFFIX =
  windows:BOOST_LIB_SUFFIX = -mgw92-mt-s-1_65
}

isEmpty(BOOST_THREAD_LIB_SUFFIX) {
    BOOST_THREAD_LIB_SUFFIX = $$BOOST_LIB_SUFFIX
}

isEmpty(BOOST_LIB_PATH){
  macx:BOOST_LIB_PATH = /opt/local/lib
}

isEmpty(BOOST_INCLUDE_PATH){
  macx:BOOST_INCLUDE_PATH = /opt/local/include
}

windows:DEFINES += WIN32
windows:RC_FILE = src/updater.rc

windows:!contains(MINGW_THREAD_BUGFIX, 0) {
    # At least qmake's win32-g++-cross profile is missing the -lmingwthrd
    # thread-safety flag. GCC has -mthreads to enable this, but it doesn't
    # work with static linking. -lmingwthrd must come BEFORE -lmingw, so
    # it is prepended to QMAKE_LIBS_QT_ENTRY.
    # It can be turned off with MINGW_THREAD_BUGFIX=0, just in case it causes
    # any problems on some untested qmake profile now or in the future.
    DEFINES += _MT
    QMAKE_LIBS_QT_ENTRY = -lmingwthrd $$QMAKE_LIBS_QT_ENTRY
}

!windows:!mac{
  DEFINES += LINUX
  LIBS += -lrt
}
QMAKE_CXXFLAGS += -msse2 -w
QMAKE_CFLAGS += -msse2

QMAKE_CXXFLAGS_WARN_ON = -fdiagnostics-show-option -Wall -Wextra -Wformat -Wformat-security -Wno-unused-parameter -Wstack-protector

macx:HEADERS += src/macdockiconhandler.h
macx:OBJECTIVE_SOURCES += src/macdockiconhandler.mm
macx:LIBS += -framework Foundation -framework ApplicationServices -framework AppKit
macx:DEFINES += MAC_OSX MSG_NOSIGNAL=0
macx:ICON = src/images/lcp.icns
macx:TARGET = "updater-qt"

# Set libraries and includes at end, to use platform-defined defaults if not overridden
INCLUDEPATH += $$BOOST_INCLUDE_PATH $$OPENSSL_INCLUDE_PATH
LIBS += $$join(BOOST_LIB_PATH,,-L,) $$join(OPENSSL_LIB_PATH,,-L,)
LIBS += -lssl -lcrypto
windows:LIBS += -lws2_32 -lshlwapi -lmswsock -lole32 -loleaut32 -luuid -lgdi32
LIBS += -lboost_system$$BOOST_LIB_SUFFIX -lboost_filesystem$$BOOST_LIB_SUFFIX -lboost_program_options$$BOOST_LIB_SUFFIX -lboost_thread$$BOOST_THREAD_LIB_SUFFIX
windows:LIBS += -lboost_chrono$$BOOST_LIB_SUFFIX -static-libgcc -static-libstdc++ -Wl,-Bstatic -lstdc++ -lpthread -Wl,--stack,16777216

contains(RELEASE, 1) {
    !windows:!macx {
        # Linux: turn dynamic linking back on for c/c++ runtime libraries
        LIBS += -Wl,-Bdynamic
    }
}

