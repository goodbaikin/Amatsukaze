FROM ubuntu:22.04
RUN apt-get update &&\
	apt-get install -y \
		gcc \
		g++ \
		git \
		make \
		cmake \
		nasm \
		ninja-build \
		libssl-dev \
		zlib1g-dev

RUN git clone --depth 1 -b v3.7.3 https://github.com/AviSynth/AviSynthPlus &&\
	cd AviSynthPlus &&\
	mkdir build &&\
	cd build &&\
	cmake .. -GNinja &&\
	ninja &&\
	ninja install

RUN git clone --depth 1 -b n4.4.2 https://github.com/FFmpeg/FFmpeg &&\
	cd FFmpeg &&\
	./configure --enable-shared --enable-pic &&\
	make -j$(nproc) &&\
	make install

RUN git clone https://github.com/tobitti0/join_logo_scp &&\
	cd join_logo_scp/src &&\
	make -j$(nproc) &&\
	cp join_logo_scp /usr/local/bin/

RUN git clone https://github.com/tobitti0/chapter_exe &&\
	cd chapter_exe/src &&\
	make -j$(nproc) &&\
	cp chapter_exe /usr/local/bin/

RUN git clone https://github.com/l-smash/l-smash.git &&\
	cd l-smash &&\
	./configure --enable-shared &&\
	make &&\
	make install

RUN git clone --depth 1 https://github.com/gpac/gpac &&\
	cd gpac &&\
	./configure &&\
	make -j$(nproc) &&\
	make install

RUN git clone --depth 1 https://code.videolan.org/videolan/x264.git &&\
	cd x264 &&\
	./configure &&\
	make -j$(nproc) &&\
	make install

RUN git clone --depth 1 https://github.com/tobitti0/delogo-AviSynthPlus-Linux &&\
	cd delogo-AviSynthPlus-Linux/src &&\
	make &&\
	mkdir /usr/local/lib/plugins64 &&\
	cp libdelogo.so /usr/local/lib/plugins64/

ARG VERSION
ENV VERSION=$VERSION	
RUN git clone --depth 1 -b ${VERSION} https://github.com/goodbaikin/Amatsukaze

RUN cd Amatsukaze/libfaad &&\
make -j$(nproc) &&\
make install

RUN cd Amatsukaze/TVCaptionModLinux &&\
	make -j$(nproc) &&\
	make install

RUN cd Amatsukaze/Amatsukaze &&\
	./Version.sh &&\
	make -j$(nproc) &&\
	make install

FROM ubuntu:22.04
RUN apt-get update &&\
	apt-get install -y \
		wget \
		libavformat58 \
		libavutil56 \
		libswresample3 \
		libavfilter7 \
		libavdevice58 \
		libass9 \
		libmfx-gen1.2 \
		libigfxcmrt7 \
		gpg-agent &&\
	cd /tmp &&\
	wget -qO - https://repositories.intel.com/graphics/intel-graphics.key | gpg --dearmor --output /usr/share/keyrings/intel-graphics.gpg &&\
	echo 'deb [arch=amd64,i386 signed-by=/usr/share/keyrings/intel-graphics.gpg] https://repositories.intel.com/graphics/ubuntu jammy arc' | tee /etc/apt/sources.list.d/intel.gpu.jammy.list &&\
	apt-get update &&\
	apt-get install -y intel-media-va-driver-non-free intel-opencl-icd &&\
	\
	wget https://github.com/rigaya/QSVEnc/releases/download/7.62/qsvencc_7.62_Ubuntu22.04_amd64.deb &&\
	dpkg -i qsvencc_7.62_Ubuntu22.04_amd64.deb &&\
	\
	apt-get purge --autoremove -y wget gpg-agent &&\
	rm -rf /var/lib/apt/lists &&\
	rm -rf /var/cache/* &&\
	rm -rf /tmp/*
COPY --from=0 /usr/local /usr/local
RUN ldconfig