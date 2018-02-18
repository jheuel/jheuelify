FROM ubuntu:devel
#FROM ubuntu:rolling
#FROM ubuntu:latest
ARG DEBIAN_FRONTEND=noninteractive

ENV NUM_CORES 4

RUN apt-get update -q
RUN apt-get install -qy --fix-missing \
            wget \
            cmake \
            git \
            python \
            gfortran \
            libcurl4-openssl-dev \
            build-essential \
            libboost-all-dev \
            doxygen \
            libssl-dev \
            libcurlpp-dev \
            libeigen3-dev \
            unzip \
            yasm \
            pkg-config \
            libswscale-dev \
            libtbb2 \
            libtbb-dev \
            libjpeg-dev \
            libpng-dev \
            libtiff-dev \
            libavformat-dev \
            libpq-dev

WORKDIR /tmp/opencv
ENV OPENCV_VERSION="3.4.0"
RUN wget https://github.com/opencv/opencv/archive/${OPENCV_VERSION}.zip -q
RUN unzip ${OPENCV_VERSION}.zip
WORKDIR /tmp/opencv/opencv-${OPENCV_VERSION}/build
RUN cmake -DBUILD_TIFF=ON \
     -DBUILD_opencv_java=OFF \
     -DWITH_CUDA=OFF \
     -DENABLE_AVX=ON \
     -DWITH_OPENGL=ON \
     -DWITH_OPENCL=ON \
     -DWITH_IPP=ON \
     -DWITH_TBB=ON \
     -DWITH_EIGEN=ON \
     -DWITH_V4L=ON \
     -DBUILD_TESTS=OFF \
     -DBUILD_PERF_TESTS=OFF \
     -DCMAKE_BUILD_TYPE=RELEASE \
     ..
RUN make -j${NUM_CORES}
RUN make install
RUN rm -r /tmp/opencv

# install OpenBLAS
WORKDIR /tmp/blas
RUN git clone -q --branch=master git://github.com/xianyi/OpenBLAS.git
WORKDIR /tmp/blas/OpenBLAS
RUN make DYNAMIC_ARCH=1 NO_AFFINITY=1 NUM_THREADS=4 -j${NUM_CORES}
RUN make install
RUN ldconfig
RUN rm -rf /tmp/blas

# install dlib
WORKDIR /tmp/dlib/build
RUN wget http://dlib.net/files/dlib-19.9.tar.bz2 -O /tmp/dlib.tar.bz2 -q
RUN tar xf /tmp/dlib.tar.bz2 -C /tmp/dlib
RUN cmake ../dlib-*
RUN cmake --build . --config Release -- -j${NUM_CORES}
RUN make install
RUN ldconfig
RUN rm -rf /tmp/dlib

# install tgbot lib
WORKDIR /tmp
RUN git clone https://github.com/reo7sp/tgbot-cpp
WORKDIR /tmp/tgbot-cpp/build
RUN cmake ..
RUN make -j${NUM_CORES}
RUN make install
RUN rm -rf /tmp/tgbot-cpp


# create user without root privileges
ARG username=physicist
RUN useradd --create-home --home-dir /home/${username} --shell /bin/bash ${username}
ENV HOME /home/${username}
WORKDIR /home/${username}

# make output directory writeable
RUN mkdir /output
RUN chown ${username}:${username} -R /output

# get dlib model
RUN wget -q http://dlib.net/files/shape_predictor_68_face_landmarks.dat.bz2
RUN bzip2 -d shape_predictor_68_face_landmarks.dat.bz2


# add faces and source
ADD jheuelify jheuelify
ADD faces faces
RUN chown -R ${username}:${username} .

USER ${username}

# build project
RUN mkdir /home/${username}/build
WORKDIR /home/${username}/build
RUN cmake ../jheuelify
RUN make -j${NUM_CORES}

WORKDIR /home/${username}

# Define default command.
CMD ./build/bin/jheuelify                                     \
    --dlib_model_path shape_predictor_68_face_landmarks.dat   \
    --faces_path faces                                        \
    --output_path /output                                     \
    --api_key ${TELEGRAM_API_KEY}
