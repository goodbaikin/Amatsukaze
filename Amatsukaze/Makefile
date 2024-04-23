CFLAGS = -g3 -I../common -I../include -I../include_gpl -I./linux \
	-mavx -mavx2 -mfma\
	-Wno-multichar -Wno-deprecated-declarations
CXXFLAGS = $(CFLAGS)
OBJS = AdtsParser.o \
	AMTLogo.o \
	AMTSource.o \
	AudioEncoder.o \
	CaptionData.o \
	CaptionFormatter.o \
	CMAnalyze.o \
	ComputeKernel.o \
	Encoder.o \
	EncoderOptionParser.o \
	FileUtils.o \
	FilteredSource.o \
	H264VideoParser.o \
	InterProcessComm.o \
	LogoScan.o \
	Mpeg2TsParser.o \
	Mpeg2VideoParser.o \
	Muxer.o \
	NicoJK.o \
	linux/OSUtil.o \
	PacketCache.o \
	PerformanceUtil.o \
	linux/ProcessThread.o \
	ReaderWriterFFmpeg.o \
	StreamReform.o \
	StreamUtils.o \
	StringUtils.o \
	TranscodeManager.o \
	TranscodeSetting.o \
	TsInfo.o \
	TsSplitter.o \
	WaveWriter.o \
	../include/TextOut.o

BIN = lib/libamatsukaze.so bin/amatsukaze

%.o: %.cpp
	g++ -c -fPIC $(CXXFLAGS) $^ -o $@
%.o: %.c
	gcc -c -fPIC $(CFLAGS) $^ -o $@

all: $(BIN)

../libfaad/libfaad.so:
	make -C ../libfaad

lib/libamatsukaze.so: Amatsukaze.o $(OBJS)
	-@mkdir lib
	g++ -shared -o $@ $^ -lavformat -lfaad -lcaptionmod2 -lcrypto

bin/amatsukaze: main.cpp $(OBJS)
	-@mkdir bin
	g++ $(CXXFLAGS) main.cpp -I../include -o $@ $(OBJS) -lavcodec -lavformat -lavutil -lcrypto -lfaad -lcaptionmod2
install: $(BIN)
	cp lib/libamatsukaze.so /usr/local/lib/
	cp bin/amatsukaze /usr/local/bin/
clean:
	rm *.o
	linux/*.o
	rm $(BIN)
