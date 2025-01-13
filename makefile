CC = gcc

CFLAGS = -g -I ./teacher/ -I ./student/ -I ./main/ -I ./tutor/

SRCS = ./main/main.c teacher/teacher.c student/student.c tutor/tutor.c

OBJS = $(SRCS:.c=.o)

TARGET = my_program

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: clean
