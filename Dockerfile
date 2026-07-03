FROM ubuntu:latest

ARG TOOLCHAIN_URL="https://github.com/DragonMinded/libdragon/releases/download/toolchain-continuous-prerelease/gcc-toolchain-mips64-x86_64.deb"
ARG TINY3D_REPO_URL="https://github.com/HailToDodongo/tiny3d.git"

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    libpng-dev \
    wget \
    git \
    xz-utils \
    jq

# Install libdragon toolchain
RUN wget $TOOLCHAIN_URL -O toolchain.deb &&\
    dpkg -i toolchain.deb &&\
    rm toolchain.deb

# Install libdragon
ENV N64_INST=/opt/libdragon
COPY libdragon /libdragon
RUN cd /libdragon &&\
    make install \
    clobber \
    tools \
    tools-install \
    tools-clean \
    clobber &&\
    cd / &&\
    rm -r /libdragon

# Install tiny3d
ENV T3D_INST=/opt/tiny3d
RUN cd /opt &&\
    git clone $TINY3D_REPO_URL &&\
    cd tiny3d &&\
    make &&\
    cd /

# Install Blender
ENV BLENDER_5=/opt/blender/blender
RUN wget https://download.blender.org/release/Blender5.1/blender-5.1.2-linux-x64.tar.xz
RUN apt-get install xz-utils -y &&\
    tar -xvf blender-5.1.2-linux-x64.tar.xz &&\
    mv blender-5.1.2-linux-x64 /opt/blender &&\
    rm blender-5.1.2-linux-x64.tar.xz

# Install Blender Dependencies
RUN apt-get install -y \
    libx11-6 \
    libxi6 \
    libxrender1 \
    libgl1 \
    libgl1-mesa-dri \
    libglu1-mesa \
    libglib2.0-0 \
    libsm6 \
    libxext6 \
    libxfixes3 \
    libxcursor1 \
    libxinerama1 \
    libxxf86vm1 \
    libxrandr2 \
    libxkbcommon0 \
    python3 \
    && apt-get clean

# Install Fast64
RUN wget https://github.com/Fast-64/fast64/archive/refs/heads/main.zip
USER ubuntu
RUN /opt/blender/blender --command extension install-file -r user_default --enable main.zip
RUN make tools/mesh_export.zip
RUN /opt/blender/blender --command extension install-file -r user_default --enable tools/mesh_export.zip

WORKDIR /spellcraft
