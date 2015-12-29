all: hw4.cgi httpd socks-server
hw4.cgi: cgi/cgi.cpp cgi/fdstream.hpp cgi/lib.hpp cgi/pipe.hpp cgi/nonblock_client.hpp
	@echo using `g++ --version | grep ^g++ `
	@GCC_MAJOR=`g++ --version | grep ^g++ | sed 's/^.* //g' | awk -F. '{print $$1}'` ;\
	GCC_MINOR=`g++ --version | grep ^g++ | sed 's/^.* //g' | awk -F. '{print $$2}'` ;\
	GCCVERSION=`expr $$GCC_MAJOR \* 10000 + $$GCC_MINOR ` ;\
	if [ $$GCCVERSION -ge 50000 ];\
	then \
	  echo "compiling as executable: hw4.cgi" ;\
	  g++ --std=c++11 cgi/cgi.cpp -o hw4.cgi;\
	  cp hw4.cgi ~/public_html/np;\
	else \
	  rm -f hw4.cgi ;\
	  echo "Please use g++ 5.0.0+ version" ;\
	fi
httpd: http/httpd.cpp http/fdstream.hpp http/lib.hpp http/pipe.hpp
	@echo using `g++ --version | grep ^g++ `
	@GCC_MAJOR=`g++ --version | grep ^g++ | sed 's/^.* //g' | awk -F. '{print $$1}'` ;\
	GCC_MINOR=`g++ --version | grep ^g++ | sed 's/^.* //g' | awk -F. '{print $$2}'` ;\
	GCCVERSION=`expr $$GCC_MAJOR \* 10000 + $$GCC_MINOR ` ;\
	if [ $$GCCVERSION -ge 50000 ];\
	then \
	  echo "compiling as executable: httpd" ;\
	  g++ --std=c++11 http/httpd.cpp -o httpd;\
	else \
	  rm -f httpd ;\
	  echo "Please use g++ 5.0.0+ version" ;\
	fi
socks-server: socks/server.cpp socks/socks.hpp
	@echo using `g++ --version | grep ^g++ `
	@GCC_MAJOR=`g++ --version | grep ^g++ | sed 's/^.* //g' | awk -F. '{print $$1}'` ;\
	GCC_MINOR=`g++ --version | grep ^g++ | sed 's/^.* //g' | awk -F. '{print $$2}'` ;\
	GCCVERSION=`expr $$GCC_MAJOR \* 10000 + $$GCC_MINOR ` ;\
	if [ $$GCCVERSION -ge 50000 ];\
	then \
	  echo "compiling as executable: httpd" ;\
	  g++ --std=c++11 socks/server.cpp -o socks-server;\
	else \
	  rm -f socks-server ;\
	  echo "Please use g++ 5.0.0+ version" ;\
	fi

clean:
	rm -f hw4.cgi
	rm -f httpd
	rm -f socks-server
