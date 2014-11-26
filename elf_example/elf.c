
typedef void (*func_type)();

int dynload_max(int a, int b)
{
    if (a >= b) {
        return a;
    } else {
        return b;
    }
}

void dynload_elf()
{
    int a, b, max;
    void *val;
    func_type ptr;

    a = 0x60;
    b = 0x1;
    max = dynload_max(a, b);
    val = (void *)max;
    ptr = (func_type)(0x001bd800 + val);

    ptr("hehe%s<%d>\r\n", __func__, __LINE__);
    ptr("xixi%s<%d>\r\n", __func__, __LINE__);

    return;
}
