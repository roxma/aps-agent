
all:
	mkdir -p bin
	g++ main.cpp -lpthread -O3 -o bin/aps-agent
	g++ sender.cpp -o bin/sender

.PHONY:rpm
rpm:
	rpmbuild -bb --define="_my_source_code_dir $$(pwd)" ./build/rpm.spec

clean:
	rm bin/aps-agent
	rm bin/sender

