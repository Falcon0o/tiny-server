TARGET = TinyServer

OBJECTS = StringSlice.o TinyServer.o Config.o Crc.o Cycle.o 	\
	Listening.o Process.o ConnectionPool.o Timer.o Epoller.o 	\
	Event.o Connection.o Buffer.o HttpRequest.o HttpHeadersIn.o \
	Pool.o HttpConnection.o BufferChain.o IOVector.o		 	\
	HttpPhaseEngine.o OpenFileInfo.o CachedOpenFile.o 			\
	HttpHeadersOut.o HttpHeaderHandler.o

DEBUG = -g

.PHONY : install

install : $(TARGET)

$(TARGET) : $(OBJECTS)
	g++ -o $(TARGET) $(OBJECTS) $(DEBUG)

TinyServer.o : Cycle.h
	g++ -c TinyServer.cc $(DEBUG)

StringSlice.o : StringSlice.cc StringSlice.h Crc.h
	g++ -c StringSlice.cc $(DEBUG)
StringSlice.h : Config.h

Config.o : Config.cc Config.h
	g++ -c Config.cc $(DEBUG)
config.h : LinuxConfig.h Types.h Setting.h GlobalVal.h 

Crc.o : Crc.h Crc.cc
	g++ -c Crc.cc $(DEBUG)
Crc.h : Config.h

Cycle.o : Cycle.cc Cycle.h Connection.h Epoller.h Timer.h Pool.h ConnectionPool.h
	g++ -c Cycle.cc $(DEBUG)
Cycle.h : Config.h Event.h Listening.h Process.h

Listening.o : Listening.cc Listening.h
	g++ -c Listening.cc $(DEBUG)
Listening.h : Config.h

Process.o : Process.h Process.cc
	g++ -c Process.cc $(DEBUG)
Process.h : Config.h

ConnectionPool.o : ConnectionPool.cc ConnectionPool.h Connection.h Event.h Timer.h
	g++ -c ConnectionPool.cc $(DEBUG)
ConnectionPool.h : Config.h

Timer.o : Timer.cc Timer.h
	g++ -c Timer.cc $(DEBUG)
Timer.h : Config.h StringSlice.h

Epoller.o : Epoller.cc Epoller.h Event.h Connection.h Process.h Timer.h
	g++ -c Epoller.cc $(DEBUG)
Epoller.h : Config.h

Event.o : Event.cc Event.h Buffer.h Connection.h Cycle.h Epoller.h \
			HttpConnection.h HttpRequest.h Listening.h Pool.h Timer.h \
			ConnectionPool.h
	g++ -c Event.cc $(DEBUG)
Event.h : Config.h

Connection.o : Connection.h Connection.cc Buffer.h BufferChain.h \
	ConnectionPool.h Cycle.h Epoller.h HttpConnection.h HttpRequest.h  \
	IOVector.h Timer.h Pool.h
	g++ -c Connection.cc $(DEBUG)
Connection.h : Event.h Config.h

Buffer.o : Buffer.cc Buffer.h Connection.h HttpRequest.h File.h \
	Timer.h Pool.h OpenFileInfo.h
	g++ -c Buffer.cc $(DEBUG)
Buffer.h : Config.h StringSlice.h

HttpRequest.o : HttpRequest.cc HttpRequest.h Connection.h HttpConnection.h \
	Buffer.h BufferChain.h Epoller.h Pool.h Event.h File.h OpenFileInfo.h \
	CachedOpenFile.h 
	g++ -c HttpRequest.cc $(DEBUG)
HttpRequest.h : HttpHeadersIn.h HttpHeadersOut.h HttpPhaseEngine.h Config.h

HttpHeadersIn.o : HttpHeadersIn.h HttpHeadersIn.cc HttpRequest.h
	g++ -c HttpHeadersIn.cc $(DEBUG)
HttpHeadersIn.h : Config.h StringSlice.h

Pool.o : Pool.h Pool.cc BufferChain.h
	g++ -c Pool.cc $(DEBUG)
Pool.h : Config.h

HttpConnection.o : HttpConnection.cc HttpConnection.h
	g++ -c HttpConnection.cc $(DEBUG)
HttpConnection.h : Config.h

BufferChain.o : BufferChain.cc BufferChain.h Buffer.h Cycle.h
	g++ -c BufferChain.cc $(DEBUG)
BufferChain.h : Config.h

IOVector.o : IOVector.cc IOVector.h Buffer.h BufferChain.h
	g++ -c IOVector.cc $(DEBUG)
IOVector.h : Config.h

HttpPhaseEngine.o : HttpPhaseEngine.cc HttpPhaseEngine.h HttpRequest.h Connection.h 
	g++ -c HttpPhaseEngine.cc $(DEBUG)
HttpPhaseEngine.h : Config.h

OpenFileInfo.o : OpenFileInfo.h OpenFileInfo.cc CachedOpenFile.h
	g++ -c OpenFileInfo.cc $(DEBUG)
OpenFileInfo.h : Config.h

CachedOpenFile.o : CachedOpenFile.cc CachedOpenFile.h  OpenFileInfo.h Timer.h
	g++ -c CachedOpenFile.cc $(DEBUG)
CachedOpenFile.h : StringSlice.h Config.h

HttpHeadersOut.o : HttpHeadersOut.h HttpHeadersOut.cc
	g++ -c HttpHeadersOut.cc $(DEBUG)
HttpHeadersOut.h : Config.h StringSlice.h

HttpHeaderHandler.o : HttpHeadersIn.h HttpRequest.h HttpHeaderHandler.cc
	g++ -c HttpHeaderHandler.cc $(DEBUG)

.PHONY : clean
clean :
	-rm $(TARGET) $(OBJECTS)
