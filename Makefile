TARGET=db2qtl
CC=g++
PCH_HEADER=stdafx.h
PCH=stdafx.h.gch
OBJ=db2qtl.o mysqlconfig.o qtlconfig.o sqliteconfig.o postgresconfig.o qtlgen.o
CFLAGS=-g -DNDEBUG -O3 -I ./third_party/nlohmann/include -I ./third_party/inja/include -I ./third_party/qtl/include -I ./third_party/leech/include -I ./third_party/fmt/include -I$(BOOST_HOME) -I/usr/include -I/usr/local/include $(shell mysql_config --cflags) -I$(shell pg_config --includedir) -I$(shell pg_config --includedir-server )
CXXFLAGS= -std=c++14
LDFLAGS= -L/usr/lib -L/usr/local/lib -L$(BOOST_HOME)/stage/lib

all : $(TARGET)

-include $(OBJS:.o=.d) 

$(PCH) : $(PCH_HEADER)
	$(CC) $(CFLAGS) $(CXXFLAGS) -x c++-header -o $@ $<
	
%.d: %.cpp  
	@set -e; rm -f $@; $(CC) -MM $< $(CFLAGS) $(CXXFLAGS) > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$
	
%.o : %.cpp $(PCH)
	$(CC) -c $(CFLAGS) $(CXXFLAGS) -o $@ $< 

$(TARGET) : $(OBJ)
	libtool --tag=CXX --mode=link $(CC) $(LDFLAGS) -o $@ $^ -L. -L$(shell pg_config --libdir)  -lyaml-cpp  $(shell mysql_config --libs)  -lpq -lpgtypes -lsqlite3 -lfmt -lboost_program_options -lboost_filesystem -lboost_system -ldl

clean:
	rm $(TARGET) $(OBJ) -f
