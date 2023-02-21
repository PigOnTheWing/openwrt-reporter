FROM openwrtorg/rootfs:x86_64

ARG VERSION
ARG RELEASE
ARG ARCH

COPY reporter_${VERSION}-${RELEASE}_${ARCH}.ipk "tmp/"
COPY setup.sh "/"

RUN ["bin/ash", "-c", "chmod +x setup.sh"]
