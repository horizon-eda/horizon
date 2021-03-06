FROM debian:buster
COPY . /src
RUN apt-get update \
  && apt-get upgrade -y \
  && apt-get install -y \
    build-essential \
    libsqlite3-dev \
    util-linux \
    librsvg2-dev \
    libcairomm-1.0-dev \
    libepoxy-dev \
    libgtkmm-3.0-dev \
    uuid-dev \
    libboost-dev \
    libzmq5 \
    libzmq3-dev \
    libglm-dev \
    libgit2-dev \
    libcurl4-gnutls-dev \
    liboce-ocaf-dev \
    libpodofo-dev \
    python3-dev \
    libzip-dev \
    git \
    python3-cairo-dev \
    libosmesa6-dev

ENV NVIDIA_VISIBLE_DEVICES all
# Include "utility" to get nvidia-smi
ENV NVIDIA_DRIVER_CAPABILITIES graphics,display,utility,compute,video

WORKDIR /src
RUN make -j$(nproc)

VOLUME /src/horizon-eda
CMD /src/build/horizon-eda
