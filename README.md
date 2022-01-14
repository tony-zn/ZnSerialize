# ZnSerialize

做这个工具的目的就是为了解决我项目上网络传输时的数据序列化与反序列化的问题

用过google的protobuf，用着太麻烦了，而且我的简单需求根本配不上那么大的一个库。

今天突然有个思路用可变参模板实现一个通用的好像很简单，就卷起袖子撸了一下，发现200多行就实现了基本功能。

整个代码简单，因为都是用的基本语法与标准库，所以也没有平台限制, 我的测试环境为(VS2013, g++9.3)

# 支持的类型

  系统类型, 数组, 数据结构嵌套, 数据结构多继承

  std::string, std::wstring, std::vector, std::deque, std::list, std::set, std::multiset, std::map, std::multimap, std::tuple

# 使用方法
```c++
#include "zn_serialize.hpp"

// 定义数据结构Normal
ZN_STRUCT(Normal)
{
    int a;
    double b;
    float c;
    std::string d;
    std::string e;
    // 未在宏类指定的成员不会序列化
    ZN_SERIALIZE(a,b,c,d);
};

int main()
{
    // 像正常的结构体一样使用
    Normal n1;
    n1.a = 11;
    n1.b = 22.22;
    n1.c = 111.111f;
    n1.d = "test1";
    n1.e = "with out";
    ZnSerializeBuffer buf;
    // 序列化
    n1.serialize(buf);
    // 反序列化
    Normal n2;
    n2.deserialize(buf);
    return 0;
}
```
   **更多使用方法详见:test.cpp** 

# 类型扩展

如:有一个Custom的类型需要扩展，只需要特化:

```c++
template<>
void zn_serialize::serialize(ZnSerializeBuffer& out, const Custom& v)
{
  ......
}

template<>
const uint8_t* zn_serialize::deserialize(const uint8_t* begin, const uint8_t* end, Custom& v)
{
  ......
}

```


