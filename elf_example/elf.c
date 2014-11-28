typedef void (*func_type)();

void *g_http_download_task = -1;

int dynload_max(int a, int b)
{
    if (a >= b) {
        return a;
    } else {
        return b;
    }
}
#if 0
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

    ptr("g_http_download_task <0x%8X>, value = 0x%8X\r\n", &g_http_download_task, g_http_download_task);
    if (g_http_download_task != -1) {
        ptr("Task already running\r\n");
    } else {
        ptr("Task generated...\r\n");
    }

    return;
}
#endif
void dynload_entry()
{
    func_type ptr;

    ptr = (func_type)(0x001bd860);

    ptr("g_http_download_task <0x%8X>, value = 0x%8X\r\n", &g_http_download_task, g_http_download_task);

    return;
}

void dynload_exit()
{
    func_type ptr;

    ptr = (func_type)(0x001bd860);

    ptr("Exit...\r\n");

    return;
}
