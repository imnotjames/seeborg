# Build

FROM ubuntu:jammy AS build

RUN apt-get update

RUN DEBIAN_FRONTEND=noninteractive apt-get install -qy \
  build-essential \
  clang \
  libc++-dev \
  libc++abi-dev \
  libircclient1 \
  libircclient-dev

WORKDIR /seeborg

COPY COPYING .
COPY *.cpp ./
COPY *.h ./
COPY Makefile .

RUN make

# Runtime
FROM ubuntu:jammy AS runtime

ENV SEEBORG_SERVER_HOST ""
ENV SEEBORG_SERVER_PORT 6667

ENV SEEBORG_NICK "SeeBorg"
ENV SEEBORG_USER "SeeBorg"
ENV SEEBORG_REAL "I am SeeBorg v0.51 beta"
ENV SEEBORG_QUIT "Byebye..."

ENV SEEBORG_REPLY_RATE 1.00
ENV SEEBORG_REPLY_NICK 33.00
ENV SEEBORG_REPLY_MAGIC 33.00

ENV SEEBORG_SPEAKING 1
ENV SEEBORG_LEARNING 1
ENV SEEBORG_STEALTH 0
ENV SEEBORG_JOIN_INVITES 1

ENV SEEBORG_CHANNELS ""
ENV SEEBORG_MAGIC_WORDS ""
ENV SEEBORG_OWNERS ""

ENV SEEBORG_DICTIONARY_FILE "/data/lines.txt"
ENV SEEBORG_DICTIONARY_SAVE_INTERVAL 600


RUN apt-get update \
  && DEBIAN_FRONTEND=noninteractive \
  apt-get install -qy --no-install-recommends \
  libc++1 \
  libc++abi1 \
  libircclient1 \
  && apt-get clean \
  && rm -rf /var/lib/apt/lists/*

RUN addgroup --system seeborg
RUN useradd --system -g seeborg seeborg

USER seeborg

COPY --chown=seeborg:seeborg --from=build \
    ./seeborg/seeborg-irc \
    ./app/

VOLUME "/data"

CMD "/app/seeborg-irc"