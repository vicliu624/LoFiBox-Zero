ARG BASE_IMAGE=lofibox-zero/dev-container:trixie
FROM ${BASE_IMAGE}

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get -o Acquire::Retries=5 update \
 && apt-get -o Acquire::Retries=5 install -y --no-install-recommends \
    appstream \
    autopkgtest \
    debhelper \
    desktop-file-utils \
    lintian \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace/LoFiBox-Zero

CMD ["/bin/bash"]
