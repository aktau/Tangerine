REPOSDIR=../..
include( $${REPOSDIR}/src/admin/app-common.pri )

#---------------

TARGET    = tangerine
CONFIG   += qt
CONFIG   -= app_bundle

# if you want graph support...
CONFIG += graphviz

QT += opengl xml sql

SOURCES     = *.cc
HEADERS     = *.h

RESOURCES = tangerine.qrc
RC_FILE = tangerine.rc

INCLUDEPATH += ../nicolash

DEPENDPATH += $${INCLUDEPATH}
DEPENDPATH += $${REPOSDIR}/lib/$${UNAME}.$${DBGNAME}

# UI_DIR    = ui
# FORMS     = ui/*.ui

DESTDIR=$${REPOSDIR}/bin/$${UNAME}.$${DBGNAME}

LIBS += $${THERAGUILIB}
LIBS += $${QTGUIAUXLIB}  # has to come after theraguilib
LIBS += $${THERACORELIB}
LIBS += $${THERATYPESLIB}
LIBS += $${TRIMESHLIB}   # has to come last

win32-g++: LIBS += -lglu32 -lopengl32

TRANSLATIONS = lcl_theraprocess_de.ts lcl_theraprocess_el.ts

win32-g++: CONFIG += console

graphviz {
	message(Tangerine: Graphviz support was added)

	DEFINES += WITH_GRAPH

	SOURCES += graph/*.cc
	HEADERS += graph/*.h
	
	QMAKE_LIBDIR += graph/lib
	#LIBPATH += graph/lib
	INCLUDEPATH += graph graph/include
	
	LIBS += -lgraph -lgvc
}
