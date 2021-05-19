u32 add(u32 x, u32 y){
    return x + y;
}

u32 addPlusFive(u32 x){
    return add(x, 5);
}

void main() {
    u32 a = 511;
    *(a-1) = addPlusFive(add(1, 2));
}
