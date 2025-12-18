CC = gcc
CFLAGS = -Wall -Wextra -O2

# Fichiers communs à tous les programmes
COMMON = common.c Statistique.c

# Cibles finales
TARGETS = manager player_human player_bot

all: $(TARGETS)

manager: manager.c $(COMMON)
	$(CC) $(CFLAGS) -o manager manager.c $(COMMON)

player_human: player_human.c $(COMMON)
	$(CC) $(CFLAGS) -o player_human player_human.c $(COMMON)

player_bot: player_bot.c $(COMMON)
	$(CC) $(CFLAGS) -o player_bot player_bot.c $(COMMON)

clean:
	rm -f $(TARGETS) *.o

.PHONY: all clean
