FROM openwrtorg/rootfs:x86_64

COPY reporter_1.0-1_x86_64.ipk "tmp/"
COPY setup.sh "/"

RUN ["bin/ash", "-c", "chmod +x setup.sh"]

# CMD ["bin/ash", "-c", "./setup.sh reporter_1.0-1_x86_64.ipk"]
