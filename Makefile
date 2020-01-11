TARGET=db2qtl
CC=g++
PCH_HEADER=stdafx.h
PCH=stdafx.h.gch
OBJ=db2qtl.o mysqlconfig.o qtlconfig.o sqliteconfig.o
GENMOD=qtlgen
CFLAGS=-g -DNDEBUG -O3 -I ./third_party/nlohmann/include -I ./third_party/inja/include -I ./third_party/qtl/include -I ./third_party/leech/include -I ./third_party/fmt/include  -I/usr/include -I/usr/local/include -I /usr/include/mysql
CXXFLAGS= -std=c++14
LDFLAGS= -L/usr/lib -L/usr/local/lib

all : $(GENMOD) $(TARGET)

-include $(OBJS:.o=.d) 

$(PCH) : $(PCH_HEADER)
	$(CC) $(CFLAGS) $(CXXFLAGS) -x c++-header -o $@ $<
	
%.d: %.cpp  
	@set -e; rm -f $@; $(CC) -MM $< $(CFLAGS) $(CXXFLAGS) > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$
	
$(GENMOD).o : $(GENMOD).cpp $(PCH)
	$(CC) -c $(CFLAGS) $(CXXFLAGS) -fPIC -fvisibility=hidden -o $@ $< 

$(GENMOD) : $(GENMOD).o
	$(CC) $(LDFLAGS) -shared -o lib$@.so $^ -lboost_filesystem -lboost_system -ldl

%.o : %.cpp $(PCH)
	$(CC) -c $(CFLAGS) $(CXXFLAGS) -o $@ $< 

$(TARGET) : $(OBJ)
	libtool --tag=CXX --mode=link $(CC) $(LDFLAGS) -o $@ $^ -L. -l$(GENMOD) -lyaml-cpp -lmysqlclient -lsqlite3 -lfmt -lboost_program_options -lboost_filesystem -lboost_system -ldl

clean:
	rm $(TARGET) $(OBJ) $(GENMOD).o lib$(GENMOD).so -f
