BINPATH=./bin
OBJPATH=./obj

all:
	g++ CondVar.cpp -lpthread -fpic  -c -o $(OBJPATH)/CondVar.o
	g++ Mutex.cpp -lpthread -fpic -c -o $(OBJPATH)/Mutex.o
	g++ Task.cpp -lpthread -fpic -c -o $(OBJPATH)/Task.o
	g++ ThreadPool.cpp -lpthread -fpic -c -o $(OBJPATH)/ThreadPool.o
	g++ -fPIC -shared -lpthread $(OBJPATH)/CondVar.o $(OBJPATH)/Mutex.o $(OBJPATH)/Task.o $(OBJPATH)/ThreadPool.o -o $(BINPATH)/libthreadpool.so
#	g++ threadpool_test.cpp -L$(BINPATH) -lthreadpool -lpthread -fpic -o $(BINPATH)/threadpool_test

	g++ $(OBJPATH)/CondVar.o $(OBJPATH)/Mutex.o $(OBJPATH)/Task.o $(OBJPATH)/ThreadPool.o threadpool_test.cpp -lpthread -o $(BINPATH)/threadpool_test

#all:
#	g++ threadpool.cpp -lpthread -fpic -c -o bin/obj/threadpool.o
#	g++ -L./bin bin/obj/threadpool.o -lpthread threadpool_test.cpp -o bin/example/threadpool_test

#threadpool:
#	g++ threadpool.cpp -lpthread -fpic -c -o bin/obj/threadpool.o
#	g++ -shared -fPIC bin/obj/threadpool.o -o bin/lib/libthreadpool.so
#example:
#	g++ -L./bin/lib -lthreadpool threadpool_test.cpp -o threadpool_test
