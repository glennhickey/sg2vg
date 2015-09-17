FROM ubuntu:15.04

MAINTAINER Glenn Hickey <glenn.hickey@gmail.com>

# Install dependencies and clear the package index
RUN \
    apt-get update && \
    apt-get install -y \
        build-essential \
        libcurl4-openssl-dev \
        zlib1g-dev && \
    rm -rf /var/lib/apt/lists/*
    
# Move in all the files
COPY . /app

WORKDIR /app
    
# Build and test
RUN make -j 8 test

ENTRYPOINT ["/app/sg2vg"]
CMD ["--help"]

