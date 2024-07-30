#FROM nginx:latest 访问dockerhub官方镜像存在网络问题，使用quay.mirrors.ustc.edu.cn加速
FROM quay.mirrors.ustc.edu.cn/jitesoft/nginx

ENV SERVER_ROOT=/usr/share/nginx/html

COPY nginx.conf /etc/nginx/conf.d/default.conf

ADD html.tar.gz /usr/share/nginx/html

WORKDIR /usr/share/nginx/html

CMD ["nginx", "-g", "daemon off;"]
