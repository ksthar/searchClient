bin_PROGRAMS = searchclient
searchclient_SOURCES = main.cc util.cc
searchclient_LDFLAGS = -ldbus-c++-1

BUILT_SOURCES = glue/sd_proxy-glue.h glue/gatt_proxy-glue.h

glue/sd_proxy-glue.h: ./xml/sd.xml
	mkdir -p glue
	cpp -P \
		-DINC_SearchResultInd \
		-DINC_CloseSearchInd \
		-DINC_SearchReq \
		-DINC_CancelSearchReq \
		$< > ./xml/sd_proxy.xml
	dbusxx-xml2cpp ./xml/sd_proxy.xml --proxy=$@
	rm -f ./xml/sd_proxy.xml

glue/gatt_proxy-glue.h: ./xml/gatt.xml
	mkdir -p glue
	cpp -P \
		-DINC_RegisterReq \
		-DINC_RegisterCfm \
		-DINC_CentralReq \
		-DINC_CentralCfm \
		$< > ./xml/gatt_proxy.xml
	dbusxx-xml2cpp ./xml/gatt_proxy.xml --proxy=$@
	rm -f ./xml/gatt_proxy.xml

clean-local:
	rm -rf glue

