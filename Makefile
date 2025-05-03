CC=gcc
CCFLAGS=-Wall
TARGETS=treasure_manager monitor treasure_hub
all: $(TARGETS)
treasure_manager: treasure_manager.c 
	$(CC) $(CCFLAGS) treasure_manager.c -o treasure_manager

monitor: monitor.c
	$(CC) $(CCFLAGS) monitor.c -o monitor

treasure_hub: treasure_hub.c
	$(CC) $(CCFLAGS) treasure_hub.c -o treasure_hub

clean:
	rm -f $(TARGETS)