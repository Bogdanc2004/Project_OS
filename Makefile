CC=gcc
CCFLAGS=-Wall
TARGET=treasure_manager
all: $(TARGET)
$(TARGET): treasure_manager.c 
		$(CC) $(CCFLAGS) treasure_manager.c -o $(TARGET)

clean:
	rm -f $(TARGET)