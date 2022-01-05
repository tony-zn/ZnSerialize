#include "zn_serialize.hpp"

// 普通序列化
struct Normal
{
    int a;
    double b;
    float c;
    std::string d;
    std::string e;
    // 未在宏类指定的成员不会序列化
    ZN_SERIALIZE(a,b,c,d);
};

void test1()
{
    Normal n1;
    n1.a = 11;
    n1.b = 22.22;
    n1.c = 111.111f;
    n1.d = "test1";
    n1.e = "with out";
    ZnSerializeBuffer buf;
    n1.serialize(buf);
    Normal n2;
    n2.deserialize(buf);
}

// 嵌套使用的类型需要用ZN_STRUCT宏定义
// 其原理就是继承一个特殊的基类
ZN_STRUCT(Sub)
{
    int a;
    double b;
    float c;
    std::string d;
    ZN_SERIALIZE(a,b,c,d);
};

ZN_STRUCT(Item)
{
    Sub a;
    Sub b;
    ZN_SERIALIZE(a,b);
    // 容器需要的排序运行符
    bool operator <(const Item& o) const {return a.a < o.a.a;}
};

ZN_STRUCT(All)
{
    std::vector<Item> a;
    std::deque<Item> b;
    std::list<Item> c;
    std::set<Item> d;
    std::multiset<Item> e;
    std::map<std::string, Item> f;
    std::multimap<Item, int> g;
    ZN_SERIALIZE(a,b,c,d,e,f,g);
};

void test2()
{
    Item i;
    i.a.a = 10;
    i.a.b = 900.99;
    i.a.c = 12.12f;
    i.a.d = "test serialize 1";
    i.b.a = 20;
    i.b.b = 200.22;
    i.b.c = 9.9f;
    i.b.d = "test serialize 2";
    ZnSerializeBuffer buf;
    i.serialize(buf);
    Item i2;
    i2.deserialize(buf);
    All a;
    a.a.push_back(i);
    a.b.push_back(i);
    a.c.push_back(i);
    a.d.insert(i);
    a.e.insert(i);
    a.f["test"] = i;
    a.g.insert(std::make_pair(i, 100));
    buf.clear();
    a.serialize(buf);
    All a2;
    a2.deserialize(buf);
}

// 测试数据结构的继承
ZN_STRUCT(Parent)
{
    int aaa;
    ZN_SERIALIZE(aaa);
};

ZN_STRUCT_CHILD(Child, Parent, All)
{
    std::string name;
    ZN_SERIALIZE_CHILD(name);
};


void test3()
{
    Child a;
    a.aaa = 20;
    a.name = "test3";

    Item i;
    i.a.a = 10;
    i.a.b = 900.99;
    i.a.c = 12.12f;
    i.a.d = "test serialize 1";
    i.b.a = 20;
    i.b.b = 200.22;
    i.b.c = 9.9f;
    i.b.d = "test serialize 2";

    a.a.push_back(i);
    a.b.push_back(i);
    a.c.push_back(i);
    a.d.insert(i);
    a.e.insert(i);
    a.f["test"] = i;
    a.g.insert(std::make_pair(i, 100));
    ZnSerializeBuffer buf;
    a.serialize(buf);
    Child a2;
    a2.deserialize(buf);
}

int main()
{
    test1();
    test2();
    test3();
    return 0;
}
