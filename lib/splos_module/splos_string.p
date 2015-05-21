#ifndef __SPLOS_STRING_H
#define __SPLOS_STRING_H

#import {
	./splos_math.p;
	./splos_char.p;
}

/***************string lib**********************************/
/*
	Name: get the length of string

	Return Value:
		return the length of string	
 */
int sp_strlen(char str[]) {

	int i=0;

	if( !str ) {
		return -1;
	}
	
	while(str[i]) {
		i++;
	}

	return i;
}

/*
	Name: 
		sp_strchr - get the location of the first occurrence of the specified letter

	Return Value:
		if c exist in str, so return it's first location in str.
		or return  -1

 */
int sp_strchr(char str[], char c, int cmp) {
	
	int i, len;

	len = sp_strlen(str);
	i = 0;

	while(i < len) {
		if(sp_charcasecmp(str[i], c, cmp) == 0) 
		  return i;
		i = i + 1;
	}

	return -1;
}

/*
	Name: 
		sp_rindex - get the location of the last occurrence of the specified letter

	Return Value:
		if c exist in str, so return it's lastt location in str.
		or return  -1

*/

int sp_strrchr(char str[], char c, int cmp) {

	int i, len;

	len = sp_strlen(str);
	i = len-1;

	while(i > -1) {
		if(sp_charcasecmp(str[i], c, cmp) == 0) 
		  return i;
		i = i - 1;
	}

	return -1;
}

/*
	Name: 
		sp_strcpy - copy a string

	Description:
		if str1's length > str2's length , the whole str2 into str1 
		or copy  str1's length content  

	Return Value: 
		return a  copy of length
*/

int sp_strcpy(char str1[], char str2[])
{
	int i, len1, len2;

	i = 0;
	len1 = sp_strlen(str1);
	len2 = sp_strlen(str2);

	if(len1 > len2)	
	  len1 = len2;

	while(i < len1) {
		str1[i] = str2[i];
		i = i + 1;
	}

	str1[i] = '\0';

	return len1;
}
/*
 *	Name:
 *		sp_toupper, tolower - convert letter to upper or lower case
 *	
 * */
void sp_toupper(char str[])
{
	char anum, znum, Anum;
	int i,upper;
	
	anum = 'a';
	znum = 'z';
	Anum = 'A';
	
	//calculate the distance of a to A in AscII
	upper = anum - Anum;

	i = 0;
	while(str[i] != '\0') {
		
		//if the letter is lowercase
		if(str[i] >= anum && str[i] <= znum) {
			str[i] = str[i] - upper;
		}

		i++;
	}
}


void sp_tolower(char str[])
{
	char Anum, Znum, anum;
	int i,upper;
	
	Anum = 'A';
	Znum = 'Z';
	anum = 'a';
	
	//calculate the distance of a to A in AscII
	upper = anum - Anum;

	i = 0;
	while(str[i] != '\0') {
		
		//if the letter is uppercase
		if(str[i] >= Anum && str[i] <= Znum) {
			str[i] = str[i] + upper;
		}

		i++;
	}
}
/* 
 *	compare the first n bytes between  str1 and str2
 *	n less than or equal to str1's length  and str2's length
 * */
int sp_strcmpN(char str1[], char str2[], int cmp, int n)
{
	int i, ret;

	i = 0;
	while(i < n) {
		ret = sp_charcasecmp(str1[i], str2[i], cmp);

		if( ret != 0 )
		  return ret;

		i++;
	}

	return 0;
}

/*
 * 比较字符串str1 和 str2 是否相同
 * cmp 决定是否忽略大小写
 *	cmp = 0, 忽略大小写
 *	cmp = 1, 不忽略大小写
 * 返回值：
 *	假如str1 与 str2 完全相同，则返回 0
 *	假如str1 与 str2 最后比较的一个字母c1,c2不同，则返回 c1 - c2  
 *	如果在 ascii 码表中，c1 在 c2 之前，返回负数；否则返回正数  
  * */
int sp_strcmp(char str1[], char str2[], int cmp)
{
	int i, len1, len2, len;
	int ret;

	len1 = sp_strlen(str1);
	len2 = sp_strlen(str2);

	i = 0;
	ret = 0;

	len = len1;
	if(len1 > len2)
	  len = len2;
	
	//compare the first n bytes
	ret = sp_strcmpN(str1, str2, cmp, len);
	if(ret != 0)
		return ret;

	if(len1 == len2)
	  return ret;
	else if(len1 > len2)
	  return str1[i];
	else
	  return str2[i];
}

/*
 * 比较字符串str1和str2的前n个字符是否相同  
 * cmp 决定是否忽略大小写
 * cmp = 0, 忽略大小写
 * cmp = 1, 不忽略大小写
 * */
int sp_strncmp(char str1[], char str2[],int cmp, int n)
{
	int i, len1, len2;
	
	if(n < 0)
	  n = 0 - n;

	len1 = sp_strlen(str1);
	len2 = sp_strlen(str2);
	
	if(n >= len1 || n >= len2) {
		return sp_strcmp(str1, str2, cmp);
	} else
		return sp_strcmpN(str1, str2, cmp, n);
}

/*
 * 找到字符串str中的子串substr返回其位置  
 * cmp决定是否忽略大小写  
 * cmp = 0, 忽略大小写
 * cmp = 1,不忽略大小写
 * */
int sp_strstr(char str[], char substr[], int cmp)
{
	int len1,len2;
	int start, end;
	int i;
	int judge;

	start = sp_strchr(str, substr[0], cmp);
	if(start < 0)
	  return -1;
	end = sp_strrchr(str, substr[0], cmp);


	len1 = sp_strlen(str);
	len2 = sp_strlen(substr);

	if((len1-start) < len2)
	  return -2;

	if((end-start+1) < len2)
	  return -3;

	if((len1-end) < len2)
	  len1 = end - len2 + 1;
	
	while(start < len1) {
	
		i = 0;
		judge = 1;
		while(i < len2) {
			if( sp_charcasecmp(str[start+i], substr[i], cmp) ) {
				judge = 0;
				break;
			}
			i = i + 1;
		}

		if(judge) 
		  return start;
		start = start + 1;
	}

	return -4;
}

/*
 * 把字符串中的紧邻的数字部分转化成数字  
 * success 表示是否有数字被转化成功，success[0] 为0表示没有, 1表示有
 * 返回值为转化的数字  
 * */
int sp_strtoi(char str[], char success[])
{
	int startIndex;
	int i, j, k;
	int len;
	char numStr[20];
	int num;

	startIndex = -1;	//表示没有任何数字
	len = sp_strlen(str);
	
	i = 0;
	success[0] = 0;
	while(i < len) {
		if(str[i] >= 48 && str[i] <= 57) {
			startIndex = i;
			success[0] = 1;
			j = i;
			k = 0;
			while(j < len && (str[j] >= 48 && str[j] <= 57)) {
				numStr[k] = str[j];

				k = k + 1;
				j = j + 1;
			}
			break;
		}
		i = i + 1;
	}
	
	//假如存在数字
	if(success[0] == 1) {
		i = 0;
		num = 0;		
		while(k > 0) {
			num = num + numStr[i]*sp_pow(10, k);
			k = k - 1;
			i = i + 1;
		}
		if(startIndex - 1 >= 0 && str[startIndex-1] == '-')
			return 0 - num;
		else
			return num;
	} else {
		success[0] = 0;
		return -1;
	}

}

#endif
