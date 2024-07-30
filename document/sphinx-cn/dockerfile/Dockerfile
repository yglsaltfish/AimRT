#FROM nginx:latest 访问dockerhub官方镜像存在网络问题，改用quay.mirrors.ustc.edu.cn镜像
FROM quay.mirrors.ustc.edu.cn/kubernetes-ingress-controller/nginx-ingress-controller:0.30.0

COPY nginx.conf /etc/nginx/conf.d/default.conf

ADD html.tar.gz /usr/share/nginx/html

WORKDIR /usr/share/nginx/html

CMD ["nginx", "-g", "daemon off;"]
