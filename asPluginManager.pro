######################################################################
# QT project file for asPluginManager
# (c) Andreas Schrell, Wermelskirchen, DE
######################################################################

# what we build here (plugin name and version)
TARGET = asPluginManager
VERSION = 1.0.1

include ( ../PluginDefaults/PluginDefaults.pri )

# Include extra Qt libraries for the web views etc. if you need them
# QT += webkit
# QT += network

# our header files
HEADERS += \ 
    asPluginManager.h

# our source files
SOURCES += \ 
    asPluginManager.cpp

# our resource file with html pages and images
RESOURCES += 

# the other files we pack in the resources
OTHER_FILES += \ 
    asPluginManager.afpxml \
    xlate.pro \
    TODO.txt

# the user interface file
FORMS += \ 
    asPluginManager.ui

unix {
!mac {
# private extra targets here on my linux box
QMAKE_POST_LINK += "echo 'extras...'"

# we create the source documentation
QMAKE_POST_LINK += "; doxygen"

# locale files
QMAKE_POST_LINK += "; lrelease xlate.pro"

# strip the lib
QMAKE_POST_LINK += "; strip 'lib$${TARGET}.so.$${VERSION}'"

# we pack our plugin - I hate PZ
QMAKE_POST_LINK += "; ./afz '$$TARGET' '$$VERSION'"
}
}





