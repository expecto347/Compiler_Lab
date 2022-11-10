int test(int x[]){
    int y;
    y = x[1];
    return 0;
}

int main(void){
    int x[10];
    x[1] = 114514;
    test(x);
    return 1;
}