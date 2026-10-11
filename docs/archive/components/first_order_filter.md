# first_order_filter

独立单精度一阶滤波器。调用方提供实例，初始化时配置闭区间 alpha 和初始输出；update 不读取时钟。
非法输入失败且保持实例与输出不变。无堆、RTOS 或全局状态。
