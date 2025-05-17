CC=gcc
CCFLAGS=-Wall
TARGETS=treasure_manager monitor treasure_hub calculate_score
all: $(TARGETS)
treasure_manager: treasure_manager.c 
	$(CC) $(CCFLAGS) treasure_manager.c -o treasure_manager

monitor: monitor.c
	$(CC) $(CCFLAGS) monitor.c -o monitor

treasure_hub: treasure_hub.c
	$(CC) $(CCFLAGS) treasure_hub.c -o treasure_hub

calculate_score: calculate_score.c
	$(CC) $(CCFLAGS) calculate_score.c -o calculate_score

clean:
	rm -f $(TARGETS)