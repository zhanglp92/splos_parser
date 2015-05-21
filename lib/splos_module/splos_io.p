#ifndef __SPLOS_IO_H

#define __SPLOS_IO_H

void printf_char(char c) {
	printf(c);

}

void printf_chars(char s[], int n) {
	int i = 0;
	
	while(i < n) {
		printf('\'');
		printf(s[i]);
		printf('\'');
		printf(' ');
	}
	printf('\n');
}

void printf_nums(int s[], int n) {
	int i = 0;
	
	while(i < n) {
		printf(s[i]);
		printf(' ');
	}
	printf('\n');
}

void printf_num(int num) {
	printf(num);
}

#endif
