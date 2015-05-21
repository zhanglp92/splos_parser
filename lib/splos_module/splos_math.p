
#ifndef __SPLOS_MATH_P
#define __SPLOS_MATH_P
//  returns the value of x raised to the power of y
int sp_pow(int x, int y)
{
	int ret = 1;

	while(y > 0) {
		ret = ret * x;
		y = y - 1;
	}

	return ret;
}

int sp_abs(int x)
{
    if(x < 0)
      x = 0 - x;
	return x;
}
#endif
