bin_PROGRAMS = searchclient
searchclient_SOURCES = main.cc util.cc
searchclient_LDFLAGS = -ldbus-c++-1

BUILT_SOURCES = glue/sd_proxy-glue.h

glue/sd_proxy-glue.h: ${XMLDIR}/sd.xml
	mkdir -p glue
	cpp -P \
		-DINC_SearchResultInd \
		-DINC_CloseSearchInd \
		-DINC_SearchReq \
		-DINC_CancelSearchReq \
		$< > /tmp/sd_cpp_searchclient.xml
	dbusxx-xml2cpp /tmp/sd_cpp_searchclient.xml --proxy=$@
	rm -f /tmp/sd_cpp_searchclient.xml

clean-local:
	rm -rf glue

