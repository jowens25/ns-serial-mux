make

cp -f ns-socket-server /usr/bin/ns-socket-server

make clean

#rm /etc/systemd/system/ns-socket.service
cp -f ns-socket.service /etc/systemd/system/ns-socket.service

sudo systemctl daemon-reload
systemctl start ns-socket.service
systemctl status ns-socket.service