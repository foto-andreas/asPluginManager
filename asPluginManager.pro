######################################################################
# QT project file for asPluginManager
# (c) Andreas Schrell, Wermelskirchen, DE
######################################################################

# what we build here (plugin name and version)
TARGET = asPluginManager
VERSION = 1.0.7
IDENTIFIER = de.schrell.asPluginManager
AUTHOR = Andreas Schrell
SITE = http://fotos.schrell.de

include ( ../PluginDefaults/PluginDefaults.pri )

# Include extra Qt libraries for the web views etc. if you need them
# QT += webkit
QT += network

# Xml processing
QT += xml

# our header files
HEADERS += \ 
    asPluginManager.h \
    ConfigurationMapper.h \
    ../PluginTools/WebInfos.h \
    ../PluginTools/WebContents.h \
    ../PluginTools/ConfigFile.h \
    ../PluginTools/ToolData.h \
    ../PluginTools/TargetVersion.h

# our source files
SOURCES += \ 
    asPluginManager.cpp \
    ConfigurationMapper.cpp \
    ../PluginTools/WebInfos.cpp \
    ../PluginTools/WebContents.cpp \
    ../PluginTools/ConfigFile.cpp \
    ../PluginTools/ToolData.cpp

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

# translations
TRANSLATIONS += locale/asPluginManager_en.ts 
TRANSLATIONS += locale/asPluginManager_de.ts
TRANSLATIONS += locale/asPluginManager_nl.ts
TRANSLATIONS += locale/asPluginManager_fr.ts
TRANSLATIONS += locale/asPluginManager_it.ts
TRANSLATIONS += locale/asPluginManager_ja.ts

unix {
!mac {
# private extra targets here on my linux box
QMAKE_POST_LINK += "echo 'extras...'"

# we create the source documentation
QMAKE_POST_LINK += "; doxygen"

# locale files
QMAKE_POST_LINK += "; lrelease $${TARGET}.pro"

# strip the lib
QMAKE_POST_LINK += "; strip 'lib$${TARGET}.so.$${VERSION}'"

# we pack our plugin - I hate PZ
QMAKE_POST_LINK += "; ../PluginDefaults/afz '$$TARGET' '$$VERSION' '$$IDENTIFIER' '$$AUTHOR' '$$SITE' "
}
}

