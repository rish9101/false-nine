#include <malloc.h>

struct C
{
    int f1;
};

struct B
{
    int f1;
    struct C *f2;
};

struct A
{
    int f1;
    char f2;
    struct B *f3;
};
struct A *global_ptr;



int *foo(int x) {

    int *a = malloc(0x4);
    *a = x;
    global_ptr =  malloc(sizeof(struct A));
    return a;

}

int main(int x)
{


    struct A *a = malloc(sizeof(struct A));
    a->f3 = malloc(sizeof(struct B));
    a->f3->f2 = malloc(sizeof(struct C));

    int *y = foo(a);


    return a->f3->f1 + (*y);
}