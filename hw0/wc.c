#include<stdio.h>
int main(int argc, char *argv[]) {
    if(argc == 1){
	printf("No file");
	return 0;
    }
    else{
	while (*++argv) {
	    countFile(*argv);
	}
    }
    return 0;
}

int countFile(char* file) {
    printf("Begin of countFile :%s\n",file);
    int line_cnt = 0;
    int char_cnt = 0;
    int word_cnt = 0;
    FILE * fp;
    fp = fopen(file,"r");
    char c;
    while ((c = fgetc(fp)) != EOF){
	printf("%c",c);
	switch (c) {
	    case '\n':line_cnt++;
	    case ' ':word_cnt++;break;
	    default:char_cnt++;printf("%c",c);
	}
    }
    fclose(fp);
    printf("Line Count:%d\n",line_cnt);
    printf("Word Count:%d\n",word_cnt);
    printf("Char Count:%d\n",char_cnt);
    printf("End of countFile!");
    return 0;
}

