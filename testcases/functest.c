u32 poke(u32 addr, u32 val){
    *addr = val;
}

void main() {
    poke(511, 10);
}
