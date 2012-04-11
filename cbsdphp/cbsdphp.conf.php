server {
listen      127.0.0.1:8989;
server_name cbsd;
error_log   /var/log/httpd/cbsdphp.err info;
access_log /var/log/httpd/cbsdphp.acc main;
root /usr/local/cbsd/cbsdphp;
set $php_root $document_root;

location ~ \.php$ {
    include php-core.conf;
}

location / {
    index index.php;
    add_header Cache-Control "public";
    expires 15m;
}

}
