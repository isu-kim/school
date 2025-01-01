FROM ubuntu:24.04
RUN apt-get update
RUN apt-get install sudo python3 python3-pip -y

RUN mkdir app
WORKDIR app
COPY requirements.txt .
COPY api.py .
RUN pip3 install -r requirements.txt --break-system-packages

RUN apt-get install net-tools iproute2 netsniff-ng tcpreplay -y
EXPOSE 8000
CMD ["python3", "api.py"]
