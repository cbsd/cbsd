#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char 
isIP(char *str)
{
	char           *c;
	int		i;
	int		j;

	c = str;
	i = 0;
	j = 0;
	while (*c) {
		if ((*c >= '0') && (*c <= '9'))
			i++;
		else if ((*c == '.') && i) {
			i = 0;
			j++;
		} else
			return 0;
		c++;
	}
	return (i && (j == 3));
}

unsigned long 
str2ip(char *str)
{
	char           *c;
	unsigned long	res;
	unsigned char	n;

	if (!isIP(str))
		return 0;
	res = 0;
	n = 0;
	c = str;
	while (*c) {
		if ((*c >= '0') && (*c <= '9'))
			n = n * 10 + (*c - '0');
		else {
			res = (res << 8) | n;
			n = 0;
		}
		c++;
	}
	res = (res << 8) | n;
	return res;
}

unsigned long 
str2mask(char *str)
{
	char           *c;
	unsigned long	res;
	unsigned int	n;
	unsigned int	i;

	if (isIP(str))
		return str2ip(str);
	n = 0;
	c = str;
	while (*c) {
		if ((*c >= '0') && (*c <= '9'))
			n = n * 10 + (*c - '0');
		else
			return 0;
		c++;
	}
	res = 0;
	for (i = 0; i < n; i++) {
		res = res | (1 << (31 - i));
	}
	return res;
}

int 
usage()
{
	printf("return the sign of entry into the subnet ip\n");
	printf("require: ip mask test\n");
	exit(0);
}

int 
main(int argc, char **args)
{
	unsigned long	ip;
	unsigned long	mask;
	unsigned long	test;

	if (!strcmp(args[1], "--help"))
		usage();

	if (argc != 4) {
		return 2;
	}
	ip = str2ip(args[1]);
	mask = str2mask(args[2]);
	test = str2ip(args[3]);

	if (ip && mask && test) {
		//printf("%08x\t%08x\t%08x\n", ip, mask, test);
		if ((ip & mask) == (test & mask)) {
			//printf("yes\n");
			return 1;
		}
		//printf("no\n");
		return 0;
	}
	return 2;
}
