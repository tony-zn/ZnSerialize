#include "zn_serialize.hpp"

// 普通序列化
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

void test1()
{
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
}

// 结构体嵌套使用
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

ZN_STRUCT(Child, Parent, All)
{
    std::string name;
    ZN_SERIALIZE(name);
};

ZN_STRUCT(Grandson, Child)
{
    std::string son[2];
    std::wstring wstr;
    std::tuple<int ,double ,float> tu;
    ZN_SERIALIZE(son, wstr, tu);
};

void test3()
{
    Grandson a;
    a.son[0] = "son1";
    a.son[1] = "son2";
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
    a.wstr = L"wstring还原的会话内容wstring";
    std::get<0>(a.tu) = 1;
    std::get<1>(a.tu) = 2.2;
    std::get<2>(a.tu) = 3.3f;
    ZnSerializeBuffer buf;
    a.serialize(buf);
    Grandson a2;
    a2.deserialize(buf);
}

// 当结构体为模板时不能直接用宏，只能手动继承基类
template<typename t>
struct Template : public zn_serialize::AutoAdaptBase<Template<t>, Normal>
{
    t value;
    ZN_SERIALIZE(value);
};

ZN_STRUCT(CTemp)
{
    std::vector<Template<int>> v1;
    std::vector<Template<double>> v2;
    ZN_SERIALIZE(v1, v2);
};

void test4()
{
    Template<int> t1; 
    t1.value = 5; 
    t1.a = 100;
    Template<double> t2; 
    t2.value = 5.5;
    t2.b = 100.001;
    CTemp ct; 
    ct.v1.push_back(t1); 
    ct.v2.push_back(t2);
    ZnSerializeBuffer buf;
    ct.serialize(buf);
    CTemp ct2;
    ct2.deserialize(buf);
}

#include<fstream>
// 将数据结构字节流保存为文件
void test5()
{
    Grandson a;
    a.son[0] = "son1";
    a.son[1] = "son2";
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
    a.wstr = L"wstring还原的会话内容wstring";
    std::get<0>(a.tu) = 1;
    std::get<1>(a.tu) = 2.2;
    std::get<2>(a.tu) = 3.3f;
    ZnSerializeBuffer buf;
    a.serialize(buf);
    std::ofstream ofs("data", std::fstream::binary);
    ofs.write(reinterpret_cast<const char*>(buf.data()), buf.size());
}
// 从文件读取字节流反序列化测试完整性
void test6()
{
    std::ifstream ifs("data", std::fstream::binary);
    if (!ifs.is_open())
        return;
    ifs.seekg(0, std::fstream::end);
    size_t size = static_cast<size_t>(ifs.tellg());
    ifs.seekg(0, std::fstream::beg);
    ZnSerializeBuffer buf(size, 0);
    ifs.read(reinterpret_cast<char*>(buf.data()), buf.size());
    Grandson a;
    a.deserialize(buf);
}

int main()
{
    test1();
    test2();
    test3();
    test4();
    test5();
    test6();
    return 0;
}
