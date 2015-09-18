
# tools
MKDIR=mkdir
CP=cp
RM=rm -f
RMDIR=rm -rf

RELEASE_DIR=release


# compiler and linker
CC=g++
CFLAGS=-c -DDEBUG -Wall -O2
LDFLAGS=-static -Xlinker --strip-all


HR13SRC=hrust13/hrust13.c
HR13OBJ=hrust13/hrust13.o
HR13BIN=hrust13/hrust13.exe

DHR13SRC=dehrust13/dehrust13.cpp
DHR13OBJ=dehrust13/dehrust13.o
DHR13BIN=dehrust13/dehrust13.exe

HM35SRC=hrum35/hrum35.cpp
HM35OBJ=hrum35/hrum35.o
HM35BIN=hrum35/hrum35.exe

DHM35SRC=dehrum35/dehrum35.cpp
DHM35OBJ=dehrum35/dehrum35.o
DHM35BIN=dehrum35/dehrum35.exe


all: $(HR13BIN) $(DHR13BIN) $(HM35BIN) $(DHM35BIN)

	$(RMDIR) $(RELEASE_DIR)
	$(MKDIR) $(RELEASE_DIR)
	$(MKDIR) $(RELEASE_DIR)/hrust13
	$(MKDIR) $(RELEASE_DIR)/hrum35

	$(CP) $(HR13BIN) $(RELEASE_DIR)/hrust13/
	$(CP) hrust13/hrust13.txt $(RELEASE_DIR)/hrust13/
	$(CP) hrust13/example.bat $(RELEASE_DIR)/hrust13/

	$(CP) $(DHR13BIN) $(RELEASE_DIR)/hrust13/
	$(CP) dehrust13/dehrust13.txt $(RELEASE_DIR)/hrust13/
	$(CP) dehrust13/dehrust1.bin $(RELEASE_DIR)/hrust13/

	$(CP) $(HM35BIN) $(RELEASE_DIR)/hrum35/
	$(CP) hrum35/hrum35.txt $(RELEASE_DIR)/hrum35/
	$(CP) hrum35/example.bat $(RELEASE_DIR)/hrum35/

	$(CP) $(DHM35BIN) $(RELEASE_DIR)/hrum35/
	$(CP) dehrum35/dehrum35.txt $(RELEASE_DIR)/hrum35/

	$(CP) README.md $(RELEASE_DIR)/

	@echo OK.



$(HR13OBJ): $(HR13SRC)
	$(CC) $(CFLAGS) $(HR13SRC) -o $(HR13OBJ)

$(DHR13OBJ): $(DHR13SRC)
	$(CC) $(CFLAGS) $(DHR13SRC) -o $(DHR13OBJ)

$(HR13BIN): $(HR13OBJ)
	$(CC) $(LDFLAGS) $(HR13OBJ) -o $(HR13BIN)

$(DHR13BIN): $(DHR13OBJ)
	$(CC) $(LDFLAGS) $(DHR13OBJ) -o $(DHR13BIN)


$(HM35OBJ): $(HM35SRC)
	$(CC) $(CFLAGS) $(HM35SRC) -o $(HM35OBJ)

$(DHM35OBJ): $(DHM35SRC)
	$(CC) $(CFLAGS) $(DHM35SRC) -o $(DHM35OBJ)

$(HM35BIN): $(HM35OBJ)
	$(CC) $(LDFLAGS) $(HM35OBJ) -o $(HM35BIN)

$(DHM35BIN): $(DHM35OBJ)
	$(CC) $(LDFLAGS) $(DHM35OBJ) -o $(DHM35BIN)


clean:
	$(RMDIR) $(HR13OBJ)
	$(RMDIR) $(HR13BIN)
	$(RMDIR) $(DHR13OBJ)
	$(RMDIR) $(DHR13BIN)
	$(RMDIR) $(HM35OBJ)
	$(RMDIR) $(HM35BIN)
	$(RMDIR) $(DHM35OBJ)
	$(RMDIR) $(DHM35BIN)
	$(RMDIR) $(RELEASE_DIR)

