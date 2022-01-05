# ZnSerialize

做这个工具的目的就是为了解决我项目上网络传输时的数据序列化与反序列化的问题
用过google的protobuf，用着太麻烦了，而且我的简单需求根本配不上那么大的一个库。
今天突然有个思路用可变参模板实现一个通用的好像很简单，就卷起袖子撸了一下，发现200多行就实现了功能。
整个代码简单，因为都是用的基本语法与标准库，所以也没有平台限制
现支持：基本类型，数组，stl中的string和各种容器（stack,queue,priority_queue因无数据操作的接口无法支持）
如需要扩展自己的类型只需要对函数模板进行特化，
如:有一个Custom的类型需要扩展，只需要特化:
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

#############################################################################################

2022-01-05  增加对数据结构继承的支持
            将size_t修改为uint32_t避免32位与64位的长度不统一的问题
            增加数组的支持

使用方法详见: test.cpp
