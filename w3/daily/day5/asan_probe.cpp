#include<iostream>
using namespace std;
int get()
{
    int* t=new int[10];
    t[3]=5;
    t[4]=t[3]+1080;
    return t[3]*t[5];
}
int main()
{
    cout<<get()<<"\n";
}