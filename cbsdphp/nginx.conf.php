user  www www;
worker_processes  1;

error_log /var/log/httpd/nginx.err error;
pid /var/run/nginx.pid;


events {
    worker_connections   40000;
    kqueue_changes  1024;
    use kqueue;
}


http {
    geoip_country   /usr/local/share/GeoIP/GeoIP.dat;
    server_tokens off;
    include       mime.types;
    default_type  application/octet-stream;

    log_format  main  '$remote_addr - $remote_user [$time_local] "$request" '
                      '$status $body_bytes_sent "$http_referer" '
                      '"$http_user_agent" "$http_x_forwarded_for" "$request_time"';

    access_log  /var/log/httpd/nginx.acc main buffer=32k;

    sendfile        on;
    tcp_nopush     on;
    tcp_nodelay      on;
    aio             on;
    directio 512;
    reset_timedout_connection  on;
    send_lowat       12000;
    #keepalive_timeout  0;
    keepalive_timeout  65;
    log_not_found off;
    output_buffers 10 2m;
    gzip              on;
    gzip_buffers      16 8k;
#   gzip_comp_level   9;
    gzip_http_version 1.1;
    gzip_min_length   10;
    gzip_types        text/plain text/css application/x-javascript text/xml application/xml application/xml+rss text/javascript;
    gzip_vary         on;
    gzip_static       on;
    gzip_proxied      any;
    gzip_disable      "MSIE [1-6]\.";


server {
    listen *:80 default rcvbuf=8192 sndbuf=16384 backlog=32000 accept_filter=httpready;
}

include  vhosts/*.conf;
}

