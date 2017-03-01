#!/usr/bin/env bash


function clean() {
	echo "killing all"
	sudo killall -9 preload_server
	sudo killall -9 client_udp

	rm /dev/shm/tu*
}

function run() {
	./preload_server &
	LD_PRELOAD="$PWD/libpreload.so" ./server_udp 1500 | tee server.txt &
	LD_PRELOAD="$PWD/libpreload.so" ./client_udp 1500 &
}

main() {
	run
	echo "sleep 20"
	sleep 20
	clean
}
main
