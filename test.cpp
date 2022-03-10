#include "zn_serialize.hpp"

// 普通序列化
ZN_STRUCT(Normal)
{
    int a;
    double b;
    float c;
    std::string d;
    std::wstring e;
    std::tuple<int ,double ,float> f;
    std::string g;
    // 未在宏类指定的成员g不会序列化
    // 同时znset接口也不会对它赋值
    ZN_SERIALIZE(a,b,c,d,e,f);
};

void test1(Normal& normal)
{
    // 增加了一个类似初始化列表的赋值接口
    normal.znset(11, 22.22, 111.111f, "string", L"wstring", std::make_tuple(111,222.222, 333.333f), "with out");
    ZnSerializeBuffer buf;
    // 序列化
    normal.serialize(buf);
    // 反序列化为新的对象
    Normal new_normal;
    new_normal.deserialize(buf);
}

// 结构体嵌套使用
ZN_STRUCT(Used)
{
    Normal n1;
    Normal n2;
    ZN_SERIALIZE(n1, n2);
    // 容器需要的排序运行符
    bool operator <(const Used& o) const {return n1.a < o.n1.a;}
};

// 对标准容器的支持
ZN_STRUCT(Container)
{
    std::vector<std::shared_ptr<Used>> vector;
    std::deque<Used> deque;
    std::list<Used> list;
    std::set<Used> std_set;
    std::multiset<Used> multiset;
    std::map<std::string, Used> map;
    std::multimap<Used, int> multimap;
    ZN_SERIALIZE(vector,deque,list,std_set,multiset,map,multimap);
};

void test2(Container& container, Normal& normal)
{
    std::shared_ptr<Used> used = std::make_shared<Used>();
    used->znset(normal, normal);
    ZnSerializeBuffer buf;
    used->serialize(buf);
    Used new_used;
    new_used.deserialize(buf);
    container.vector.push_back(used);
    container.vector.push_back(used);
    container.deque.push_back(*used);
    container.deque.push_back(new_used);
    container.list.push_back(*used);
    container.list.push_back(new_used);
    container.std_set.insert(*used);
    container.std_set.insert(new_used);
    container.multiset.insert(*used);
    container.multiset.insert(new_used);
    container.map["used"] = *used;
    container.map["new_used"] = new_used;
    container.multimap.insert(std::make_pair(*used, 1));
    container.multimap.insert(std::make_pair(new_used, 2));
    buf.clear();
    container.serialize(buf);
    Container new_container;
    new_container.deserialize(buf);
}

// 测试数据结构的多继承
ZN_STRUCT(Child, Container, Normal)
{
    std::string name;
    ZN_SERIALIZE(name);
};

void test3(Child& child)
{
    child.name = "child";
    ZnSerializeBuffer buf;
    child.serialize(buf);
    Child new_child;
    new_child.deserialize(buf);
}

// 当结构体为模板时不能直接用宏，只能手动继承基类
template<typename t>
struct Template : public zn_serialize::AutoAdaptBase<Template<t>, Normal>
{
    t value;
    ZN_SERIALIZE(value);
};

void test4()
{
    Template<int> t1; 
    t1.value = 5; 
    // 基类成员的设置要指明基类
    t1.Normal::znset(1, 2.2, 3.3f, "string1", L"wstring1", std::make_tuple(111,222.222, 333.333f), "with out");
    Template<double> t2; 
    t2.value = 5.5;
    t2.Normal::znset(100, 200.2, 300.3f, "string2", L"wstring", std::make_tuple(100,200.222, 300.333f), "with out");
    ZnSerializeBuffer buf;
    t1.serialize(buf);
    Template<int> new_t1;
    new_t1.deserialize(buf);
    buf.clear();
    t2.serialize(buf);
    Template<double> new_t2;
    new_t2.deserialize(buf);
}

#include<fstream>
// 将数据结构字节流保存为文件
void test5(Child& child)
{
    ZnSerializeBuffer buf;
    child.serialize(buf);
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
    Child child;
    child.deserialize(buf);
}

ZN_STRUCT(Empty, Used, Normal)
{
    ZN_SERIALIZE();
};

int main()
{
    Child child;
    test1(child);
    test2(child, child);
    test3(child);
    test4();
    test5(child);
    test6();

    Empty emp;
    emp.Used::znset(child, child);
    emp.Normal::znset(child.a, child.b, child.c, child.d, child.e, child.f, child.g);
#if ZN_VA_OPT_SUPPORTED == 1
    // 当编译器不支持__VA_OPT__时，调用空结构体的znset接口会报定义错误
    emp.znset();
#endif
    return 0;
}
