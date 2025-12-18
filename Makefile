CC = gcc
CFLAGS = -Wall -Wextra -O2

# Répertoires
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin

# Fichiers sources
COMMON_SRC = $(SRC_DIR)/common.c $(SRC_DIR)/Statistique.c

MANAGER_SRC = $(SRC_DIR)/manager.c
PLAYER_HUMAN_SRC = $(SRC_DIR)/player_human.c
PLAYER_BOT_SRC = $(SRC_DIR)/player_bot.c

# Fichiers objets
COMMON_OBJ = $(BUILD_DIR)/common.o $(BUILD_DIR)/Statistique.o
MANAGER_OBJ = $(BUILD_DIR)/manager.o
PLAYER_HUMAN_OBJ = $(BUILD_DIR)/player_human.o
PLAYER_BOT_OBJ = $(BUILD_DIR)/player_bot.o

# Exécutables
MANAGER = $(BIN_DIR)/manager
PLAYER_HUMAN = $(BIN_DIR)/player_human
PLAYER_BOT = $(BIN_DIR)/player_bot

TARGETS = $(MANAGER) $(PLAYER_HUMAN) $(PLAYER_BOT)

all: $(TARGETS)

# Création des dossiers si besoin
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Compilation des fichiers objets
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Édition des liens
$(MANAGER): $(MANAGER_OBJ) $(COMMON_OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

$(PLAYER_HUMAN): $(PLAYER_HUMAN_OBJ) $(COMMON_OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

$(PLAYER_BOT): $(PLAYER_BOT_OBJ) $(COMMON_OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -rf $(BUILD_DIR)/*.o $(BIN_DIR)/*

.PHONY: all clean