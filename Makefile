CC=g++
RM=rm -f
CFLAGS=-c -Wall -std=c++11
LDFLAGS=
LIBS=-lcurl
SOURCES=pssender.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=pssender

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@ $(LIBS)

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	$(RM) $(OBJECTS) $(EXECUTABLE)
