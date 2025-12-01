FROM ubuntu:latest
RUN apt-get update && apt-get install -y util-linux
WORKDIR /app
COPY script2.sh /app/script2.sh
RUN chmod +x /app/script2.sh
CMD ["/app/script2.sh"]

