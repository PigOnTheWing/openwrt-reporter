FROM openwrtorg/rootfs:x86_64

ENV REPORTER=reporter_1.0-1_x86_64.ipk

COPY ${REPORTER} "tmp/"
COPY setup.sh "/"

RUN ["bin/ash", "-c", "chmod +x setup.sh"]
