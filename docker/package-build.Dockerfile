# SPDX-License-Identifier: GPL-3.0-or-later
ARG BASE_IMAGE=lofibox-zero/dev-container:trixie
FROM ${BASE_IMAGE}

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get -o Acquire::Retries=5 update \
 && apt-get -o Acquire::Retries=5 install -y --no-install-recommends \
    appstream \
    aptly \
    autopkgtest \
    build-essential \
    debhelper \
    desktop-file-utils \
    dpkg-dev \
    gnupg \
    lintian \
    libmpg123-dev \
    libsoxr-dev

WORKDIR /workspace/LoFiBox-Zero

CMD ["/bin/bash"]
