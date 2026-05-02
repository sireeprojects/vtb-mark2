/grid/dpdk/dpdk23/installed/bin/dpdk-testpmd \
	-l 6,9,11 -n 4 \
	--vdev 'net_vhost0,iface=/tmp/vhost-user.sock,client=0' \
	-- \
	--rxq=1 --txq=1 \
	--nb-cores=2 \
	--i --forward-mode=io
