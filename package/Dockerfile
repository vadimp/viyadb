from ubuntu:zesty

MAINTAINER Michael Spector <michael@viyadb.com>
LABEL Description="ViyaDB is in-memory analytical data store"

RUN apt-get update && apt-get install -y --no-install-recommends \
    libssl1.0.0 \
    software-properties-common \
  && add-apt-repository ppa:ubuntu-toolchain-r/test \
  && apt-get update && apt-get install -y \
    g++-7 \
  && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-7 60 --slave /usr/bin/g++ g++ /usr/bin/g++-7 \
  && apt-get autoremove -y gcc-5-base gcc-6-base manpages-dev python3.5-minimal \
  && apt-get clean \
  && strip $(gcc -print-prog-name=cc1plus) \
  && strip $(gcc -print-prog-name=lto1) \
  && rm -rf \
    /var/lib/apt/lists/* \
    /var/tmp/* \
    /tmp/* \
    /usr/share/doc \
    /usr/share/man \
    /usr/share/locale \
    /usr/share/dh-python \
    $(gcc -print-prog-name=cc1)

ARG tarball
ADD $tarball /opt/
RUN mv /opt/viyadb* /opt/viyadb

RUN mkdir /opt/viyadb/conf
ADD store.json /opt/viyadb/conf/store.json

EXPOSE 5000

WORKDIR "/opt/viyadb"
CMD ["/opt/viyadb/bin/viyad", "/opt/viyadb/conf/store.json"]

