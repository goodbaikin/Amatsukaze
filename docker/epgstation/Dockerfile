FROM l3tnun/epgstation:v2.10.0

FROM goodbaikin/amatsukaze:v0.15
RUN apt-get update &&\
    apt-get install -y curl xz-utils &&\
    rm -rf /var/lib/apt/lists
RUN cd /usr/local &&\ 
    curl -sL https://nodejs.org/dist/v18.20.3/node-v18.20.3-linux-x64.tar.xz | tar -xJ
ENV PATH $PATH:/usr/local/node-v18.20.3-linux-x64/bin

COPY --from=0 /app /app
WORKDIR /app

ENTRYPOINT ["npm"]
CMD ["start"]
