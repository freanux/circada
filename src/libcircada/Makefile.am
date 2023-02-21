if BUILD_LIBRARY
lib_LTLIBRARIES = libcircada.la
else
noinst_LTLIBRARIES = libcircada.la
endif
libcircada_la_SOURCES = Circada.cpp Configuration.cpp DCC.cpp DCCManager.cpp Environment.cpp Exception.cpp Flags.cpp GlobalSettings.cpp IOSync.cpp IrcClientSide.cpp IrcServerSide.cpp LineFetcher.cpp Message.cpp Mutex.cpp Nick.cpp ParserCommands.cpp Parser.cpp Recoder.cpp Session.cpp SessionOptions.cpp SessionProtocol.cpp Socket.cpp Thread.cpp Utils.cpp Window.cpp WindowManager.cpp
libcircada_la_CXXFLAGS = -I./include -Wno-unused-result
libcircada_la_LDFLAGS = -release 0.0.1
libcircada_la_LIBADD = -lpthread -lgnutls -lgnutlsxx
