CPPFLAGS=-Wall -O3
LIBS=-lupnp -lixml -lthreadutil -lpthread -lid3tag -lavformat
OBJECTS=stream360.o xmlutils.o directory.o resource.o music.o video.o container.o

all: stream360

stream360: $(OBJECTS)
	g++ $(LIBS) $(OBJECTS) -o stream360

clean:
	rm -f *.o stream360
