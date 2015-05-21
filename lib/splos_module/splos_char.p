#ifndef __SPLOS_CHAR_P
#define __SPLOS_CHAR_P
/*
 * Name:
 *		sp_chartolower,sp_chartoupper -- transfer c lowercase or uppercase
 * */
char sp_chartoupper(char c)
{
	int distance = 'a' - 'A';

	if(c >= 'a' && c <= 'z')
		return c-distance;
}
char sp_chartolower(char c)
{
	int distance = 'a' - 'A';

	if(c >= 'A' && c <= 'Z')
		return c+distance;
}
/*
 *	Name:
 *		sp_charcasecmp - compare two chars ignoring case or not
 *
 *	Descrition:
 *		cmp decide whether to ignore case
 *		if cmp is 0, ignoreing case 
 *		if cmp is 1, not ignoring case
 * */
int sp_charcasecmp(char c1, char c2, int cmp)
{
	if(cmp == 0) {
		c1 = sp_chartolower(c1);
		c2 = sp_chartolower(c2);
	} 

	return c1 - c2;
}

#endif
