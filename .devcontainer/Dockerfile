FROM ubuntu:22.04

ADD requirements.txt  .
ADD package.list .

RUN --mount=type=cache,target=/var/cache/apt \
    apt update && apt upgrade -y && \
    xargs apt install -y < ./package.list  && \
    pip config set global.index-url https://pypi.tuna.tsinghua.edu.cn/simple &&\
    pip install -r requirements.txt && \
    rm ./package.list ./requirements.txt
