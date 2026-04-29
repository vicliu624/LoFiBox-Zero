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
    lintian

WORKDIR /workspace/LoFiBox-Zero

CMD ["/bin/bash"]
