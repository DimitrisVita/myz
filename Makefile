CC = gcc
CFLAGS = -Wall -Wextra -std=c99
TARGET = myz
SRCDIR = src
INCDIR = include
OBJS = $(SRCDIR)/main.o $(SRCDIR)/myz.o $(SRCDIR)/utils.o $(SRCDIR)/ADTList.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

$(SRCDIR)/main.o: $(SRCDIR)/main.c $(INCDIR)/common.h $(INCDIR)/utils.h $(INCDIR)/myz.h
	$(CC) $(CFLAGS) -I$(INCDIR) -c $(SRCDIR)/main.c -o $(SRCDIR)/main.o

$(SRCDIR)/myz.o: $(SRCDIR)/myz.c $(INCDIR)/common.h $(INCDIR)/ADTList.h $(INCDIR)/myz.h
	$(CC) $(CFLAGS) -I$(INCDIR) -c $(SRCDIR)/myz.c -o $(SRCDIR)/myz.o

$(SRCDIR)/utils.o: $(SRCDIR)/utils.c $(INCDIR)/common.h $(INCDIR)/utils.h
	$(CC) $(CFLAGS) -I$(INCDIR) -c $(SRCDIR)/utils.c -o $(SRCDIR)/utils.o

$(SRCDIR)/ADTList.o: $(SRCDIR)/ADTList.c $(INCDIR)/common.h $(INCDIR)/ADTList.h
	$(CC) $(CFLAGS) -I$(INCDIR) -c $(SRCDIR)/ADTList.c -o $(SRCDIR)/ADTList.o

clean:
	rm -f $(TARGET) $(OBJS)
