u32 add(u32 x, u32 y){
    return x + y;
}

void main() {
    u32 a = 511;
    *(a-1) = add(1, 2);
}
