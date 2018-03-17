FROM alpine:3.7
RUN apk add --no-cache build-base
COPY snail snail
RUN cd snail && make
