# 源码来源

StateBit 的图处理和枚举框架包含来自 **Efficient Enumeration of Large Maximal k-Plexes** 对应的 Maximal-kPlex 项目的实现。

StateBit 的实现包括局部位表示、跨状态 SIMD 执行、位宽转换、固定顶点折叠与约束表示，以及统一的顺序/并行搜索核心。

引用相关工作时，请同时注明 StateBit 论文和上述基础枚举工作。代码中的来源与版权声明应予保留。
