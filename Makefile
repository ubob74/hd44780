hd44780-y += hd44780_display.o

obj-m += hd44780.o

all:
	make -C $(KSRC) M=$(PWD) modules
clean:
	make -C $(KSRC) M=$(PWD) clean
	rm -f *.o *~

