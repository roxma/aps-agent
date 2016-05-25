
all:
	mkdir -p bin
	g++ aps_agent.cpp -lpthread -O3 -o bin/aps_agent
	g++ sender.cpp -o bin/sender

.PHONY:rpm
rpm:
	rpmbuild -bb --define="_my_source_code_dir $$(pwd)" ./rpm/build.spec

clean:
	rm bin/aps_agent
	rm bin/sender

