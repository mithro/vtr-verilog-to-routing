#!/usr/bin/env bash

set -e

sudo apt-get install --yes apt-cacher-ng
sudo service apt-cacher-ng stop
echo "PassThroughPattern: .*" | sudo tee -a /etc/apt-cacher-ng/acng.conf
echo "VerboseLog: 2" | sudo tee -a /etc/apt-cacher-ng/acng.conf
#sudo sed -i -e's/CacheDir:.*/CacheDir: /home/travis/.debs/' /etc/apt-cacher-ng/acng.conf
cat /etc/apt-cacher-ng/acng.conf
sudo service apt-cacher-ng start

echo "-----------------------"
ps -ef | grep apt-cacher-ng
netstat -an | grep "LISTEN "
echo "-----------------------"

echo 'Acquire::http::Proxy "http://127.0.0.1:3142";' | sudo tee /etc/apt/apt.conf.d/00proxy
sudo sed -i -e's@https://@http://HTTPS///@' /etc/apt/sources.list /etc/apt/sources.list.d/*
#sudo sed -i -e's@https://@http://localhost:3142/HTTPS///@' /etc/apt/sources.list /etc/apt/sources.list.d/*

set +x
for i in /etc/apt/sources.list /etc/apt/sources.list.d/*; do
	 echo
	 echo $i
	 echo "-----------------------"
	 cat $i
	 echo "-----------------------"
done
sudo -E apt-get --quiet --yes update; export RETCODE=$?

APT_NG=/var/cache/apt-cacher-ng/
ls -l $APT_NG
find $APT_NG

echo
echo "apt-cacher.err"
echo "-----------------------"
tail -n 100 $APT_NG/apt-cacher.log || true
echo "-----------------------"

echo
echo "apt-cacher.err"
echo "-----------------------"
tail -n 100 $APT_NG/apt-cacher.err || true
echo "-----------------------"

if [ "$RETCODE" -ne 0 ]; then
	exit $RETCODE
fi
