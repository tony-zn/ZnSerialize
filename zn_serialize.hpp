/*
 * 基于C++11的字节序列化工具
 * gitee地址: https://gitee.com/tony_zn/zn-serialize
 * github地址: https://github.com/tony-zn/ZnSerialize
 * 作者: tony.zn
*/
#pragma once
#include <stdint.h>
#include <string>
#include <vector>
#include <deque>
#include <list>
#include <map>
#include <set>
#include <stack>
#include <queue>
#include <tuple>

typedef std::vector<uint8_t> ZnSerializeBuffer;

namespace zn_serialize
{
    struct Exception
    {
    public:
        explicit Exception(const char* message)
            : message_(message)
        {}
        const char* what() const { return message_.c_str(); }
    private:
        std::string message_;
    };
    struct Struct
    {
        typedef Struct ZnSerialize;
        virtual void serialize(ZnSerializeBuffer& buffer) = 0;
        virtual void deserialize(const ZnSerializeBuffer& buffer) = 0;
        virtual const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end) = 0;
    };

    template<typename child_t, typename parent_t, typename...args>
    struct Parent : public parent_t, public Parent<Parent<child_t, parent_t, args...>, args...>
    {
        typedef Parent<child_t, parent_t, args...>   parent_pack_t;
    protected:
        void parent_serialize(child_t* child, ZnSerializeBuffer& buffer)
        {
            child->parent_t::serialize(buffer);
            Parent<Parent<child_t, parent_t, args...>, args...>::parent_serialize(this, buffer);
        }
        const uint8_t* parent_deserialize(child_t* child, const uint8_t* begin, const uint8_t* end)
        {
            auto p = child->parent_t::deserialize(begin, end);
            return Parent<Parent<child_t, parent_t, args...>, args...>::parent_deserialize(this, p, end);
        }
    };

    template<typename child_t, typename parent_t>
    struct Parent<child_t, parent_t> : public parent_t
    {
        typedef Parent<child_t, parent_t>   parent_pack_t;
    protected:
        void parent_serialize(child_t* child, ZnSerializeBuffer& buffer)
        {
            child->parent_t::serialize(buffer);
        }
        const uint8_t* parent_deserialize(child_t* child, const uint8_t* begin, const uint8_t* end)
        {
            return child->parent_t::deserialize(begin, end);
        }
    };

    template<typename t> t* get_zn_struct(double);
    template<typename t> typename t::ZnSerialize* get_zn_struct(int);
    template<typename t> struct GetZnStructPtr { typedef decltype(get_zn_struct<t>(0)) Ptr;};

    template<typename t>
    void default_serialize(ZnSerializeBuffer& out, const t& v, t* p)
    {
        out.insert(out.end(), reinterpret_cast<const uint8_t*>(&v), reinterpret_cast<const uint8_t*>(&v) + sizeof(v));
    }

    template<typename t>
    void default_serialize(ZnSerializeBuffer& out, const t& v, Struct* p)
    {
        p->serialize(out);
    }

    template<typename t>
    const uint8_t* default_deserialize(const uint8_t* begin, const uint8_t* end, t& v, t* p)
    {
        if (begin + sizeof(v) > end)
            throw Exception("deserialize value failed, out of memery");
        v = *reinterpret_cast<const t*>(begin);
        return begin + sizeof(v);
    }

    template<typename t>
    const uint8_t* default_deserialize(const uint8_t* begin, const uint8_t* end, t& v, Struct* p)
    {
        return p->deserialize(begin, end);
    }

    template<typename t>
    void serialize(ZnSerializeBuffer& out, const t& v)
    {
        default_serialize(out, v, static_cast<typename GetZnStructPtr<t>::Ptr>(const_cast<t*>(&v)));
    }

    template<typename t>
    const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end, t& v)
    {
        return default_deserialize(begin, end, v, static_cast<typename GetZnStructPtr<t>::Ptr>(const_cast<t*>(&v)));
    }

    template<>
    void serialize(ZnSerializeBuffer& out, const std::string& v)
    {
        uint32_t size = static_cast<uint32_t>(v.size());
        out.insert(out.end(), reinterpret_cast<const uint8_t*>(&size), reinterpret_cast<const uint8_t*>(&size) + sizeof(size));
        out.insert(out.end(), reinterpret_cast<const uint8_t*>(v.data()), reinterpret_cast<const uint8_t*>(v.data()) + size);
    }

    template<>
    const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end, std::string& v)
    {
        if (begin + sizeof(uint32_t) > end)
            throw Exception("deserialize string failed, out of memery");
        auto p = begin;
        uint32_t size = *reinterpret_cast<const uint32_t*>(p);
        p += sizeof(uint32_t);
        if (p + size > end)
            throw Exception("deserialize string failed, out of memery");
        v.assign(p, p + size);
        return p + size;
    }

    template<>
    void serialize(ZnSerializeBuffer& out, const std::wstring& v)
    {
        uint32_t size = static_cast<uint32_t>(v.size() * sizeof(wchar_t));
        out.insert(out.end(), reinterpret_cast<const uint8_t*>(&size), reinterpret_cast<const uint8_t*>(&size) + sizeof(size));
        out.insert(out.end(), reinterpret_cast<const uint8_t*>(v.data()), reinterpret_cast<const uint8_t*>(v.data()) + size);
    }

    template<>
    const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end, std::wstring& v)
    {
        if (begin + sizeof(uint32_t) > end)
            throw Exception("deserialize string failed, out of memery");
        auto p = begin;
        uint32_t size = *reinterpret_cast<const uint32_t*>(p);
        p += sizeof(uint32_t);
        if (p + size > end)
            throw Exception("deserialize string failed, out of memery");
        v.assign(reinterpret_cast<const wchar_t*>(p), reinterpret_cast<const wchar_t*>(p + size));
        return p + size;
    }

    template<size_t i, typename...t>
    struct ForeachTuple
    {
        void serialize(ZnSerializeBuffer& out, const std::tuple<t...>& tuple)
        {
            ForeachTuple<i-1, t...>().serialize(out, tuple);
            zn_serialize::serialize(out, std::get<i>(tuple));
        }
        const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end, std::tuple<t...>& tuple)
        {
            auto p = ForeachTuple<i-1, t...>().deserialize(begin, end, tuple);
            return zn_serialize::deserialize(p, end, std::get<i>(tuple));
        }
    };

    template<typename...t>
    struct ForeachTuple<0, t...>
    {
        void serialize(ZnSerializeBuffer& out, const std::tuple<t...>& tuple)
        {
            zn_serialize::serialize(out, std::get<0>(tuple));
        }
        const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end, std::tuple<t...>& tuple)
        {
            return zn_serialize::deserialize(begin, end, std::get<0>(tuple));
        }
    };

    template<typename...t>
    void serialize(ZnSerializeBuffer& out, const std::tuple<t...>& v)
    {
        ForeachTuple<sizeof...(t)-1, t...>().serialize(out, v);
    }

    template<typename...t>
    const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end, std::tuple<t...>& v)
    {
        return ForeachTuple<sizeof...(t)-1, t...>().deserialize(begin, end, v);
    }

    template<typename t, uint32_t s>
    void serialize(ZnSerializeBuffer& out, const t(&v)[s])
    {
        uint32_t size = s;
        out.insert(out.end(), reinterpret_cast<const uint8_t*>(&size), reinterpret_cast<const uint8_t*>(&size) + sizeof(size));
        for (uint32_t i = 0; i < size; ++i)
            serialize(out, v[i]);
    }

    template<typename t, uint32_t s>
    const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end, t(&v)[s])
    {
        if (begin + sizeof(uint32_t) > end)
            throw Exception("deserialize string failed, out of memery");
        auto p = begin;
        uint32_t size = *reinterpret_cast<const uint32_t*>(p);
        if (size != s)
            throw Exception("deserialize array failed, size mismatch");
        p += sizeof(uint32_t);
        for (uint32_t i = 0; i < size; ++i)
            p = deserialize(p, end, v[i]);
        return p;
    }


    template<>
    void serialize(ZnSerializeBuffer& out, const Struct& v)
    {
        const_cast<Struct&>(v).serialize(out);
    }

    template<>
    const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end, Struct& v)
    {
        return v.deserialize(begin, end);
    }

    template<typename t>
    void serialize_container(ZnSerializeBuffer& out, const t& v)
    {
        uint32_t size = static_cast<uint32_t>(v.size());
        out.insert(out.end(), reinterpret_cast<const uint8_t*>(&size), reinterpret_cast<const uint8_t*>(&size) + sizeof(size));
        for (const auto& i : v)
            serialize(out, i);
    }

    template<typename t>
    void serialize_map(ZnSerializeBuffer& out, const t& v)
    {
        uint32_t size = static_cast<uint32_t>(v.size());
        out.insert(out.end(), reinterpret_cast<const uint8_t*>(&size), reinterpret_cast<const uint8_t*>(&size) + sizeof(size));
        for (const auto& i : v)
        {
            serialize(out, i.first);
            serialize(out, i.second);
        }
    }

    template<typename t>
    void serialize(ZnSerializeBuffer& out, const std::vector<t>& v) { serialize_container(out, v); }
    template<typename t>
    void serialize(ZnSerializeBuffer& out, const std::deque<t>& v) { serialize_container(out, v); }
    template<typename t>
    void serialize(ZnSerializeBuffer& out, const std::list<t>& v) { serialize_container(out, v); }
    template<typename t>
    void serialize(ZnSerializeBuffer& out, const std::set<t>& v) { serialize_container(out, v); }
    template<typename t>
    void serialize(ZnSerializeBuffer& out, const std::multiset<t>& v) { serialize_container(out, v); }
    template<typename t>
    void serialize(ZnSerializeBuffer& out, const std::stack<t>& v) { throw Exception("serialize failed, not allowed on statck"); }
    template<typename t>
    void serialize(ZnSerializeBuffer& out, const std::queue<t>& v) { throw Exception("serialize failed, not allowed on queue"); }
    template<typename t>
    void serialize(ZnSerializeBuffer& out, const std::priority_queue<t>& v) { throw Exception("serialize failed, not allowed on priority_queue"); }
    template<typename k, typename t>
    void serialize(ZnSerializeBuffer& out, const std::map<k, t>& v) { serialize_map(out, v); }
    template<typename k, typename t>
    void serialize(ZnSerializeBuffer& out, const std::multimap<k, t>& v) { serialize_map(out, v); }


    template<typename t>
    const uint8_t* deserialize_container(const uint8_t* begin, const uint8_t* end, t& v)
    {
        if (begin + sizeof(uint32_t) > end)
            throw Exception("deserialize container failed, out of memery");
        auto p = begin;
        uint32_t size = *reinterpret_cast<const uint32_t*>(p);
        p += sizeof(uint32_t);
        for (uint32_t i = 0; i < size; ++i)
        {
            typename t::value_type item;
            p = deserialize(p, end, item);
            v.push_back(item);
        }
        return p;
    }

    template<typename t>
    const uint8_t* deserialize_set(const uint8_t* begin, const uint8_t* end, t& v)
    {
        if (begin + sizeof(uint32_t) > end)
            throw Exception("deserialize set failed, out of memery");
        auto p = begin;
        uint32_t size = *reinterpret_cast<const uint32_t*>(p);
        p += sizeof(uint32_t);
        for (uint32_t i = 0; i < size; ++i)
        {
            typename t::value_type item;
            p = deserialize(p, end, item);
            v.insert(item);
        }
        return p;
    }

    template<typename t>
    const uint8_t* deserialize_map(const uint8_t* begin, const uint8_t* end, t& v)
    {
        if (begin + sizeof(uint32_t) > end)
            throw Exception("deserialize map failed, out of memery");
        auto p = begin;
        uint32_t size = *reinterpret_cast<const uint32_t*>(p);
        p += sizeof(uint32_t);
        for (uint32_t i = 0; i < size; ++i)
        {
            typename t::key_type key;
            typename t::value_type::second_type item;
            p = deserialize(p, end, key);
            p = deserialize(p, end, item);
            v.insert(std::make_pair(key, item));
        }
        return p;
    }

    template<typename t>
    const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end, std::vector<t>& v) { return deserialize_container(begin, end, v); }
    template<typename t>
    const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end, std::deque<t>& v) { return deserialize_container(begin, end, v); }
    template<typename t>
    const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end, std::list<t>& v) { return deserialize_container(begin, end, v); }
    template<typename t>
    const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end, std::set<t>& v) { return deserialize_set(begin, end, v); }
    template<typename t>
    const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end, std::multiset<t>& v) { return deserialize_set(begin, end, v); }
    template<typename t>
    const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end, std::stack<t>& v) { throw Exception("deserialize failed, not allowed on statck"); }
    template<typename t>
    const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end, std::queue<t>& v) { throw Exception("deserialize failed, not allowed on queue"); }
    template<typename t>
    const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end, std::priority_queue<t>& v) { throw Exception("deserialize failed, not allowed on priority_queue"); }
    template<typename k, typename t>
    const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end, std::map<k, t>& v) { return deserialize_map(begin, end, v); }
    template<typename k, typename t>
    const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end, std::multimap<k, t>& v) { return deserialize_map(begin, end, v); }

    template<typename t, typename...args_t>
    void serialize(ZnSerializeBuffer& out, const t& v, const args_t&...args)
    {
        serialize(out, v);
        serialize(out, args...);
    }

    template<typename t, typename...args_t>
    const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end, t& v, args_t&...args)
    {
        return deserialize(deserialize(begin, end, v), end, args...);
    }

    template<typename t, typename...args_t>
    const uint8_t* deserialize(const ZnSerializeBuffer& in, t& v, args_t&...args)
    {
        auto end = in.data() + in.size();
        return deserialize(deserialize(in.data(), end, v), end, args...);
    }

    template<typename t, typename...parents_t>
    struct AutoAdaptBase : public Parent<t, parents_t...>
    {
    protected:
        template<typename ...args_t>
        void auto_adapt_serialize(t* child, ZnSerializeBuffer& buffer, const args_t&...args)
        {
            Parent<t, parents_t...>::parent_pack_t::parent_serialize(child, buffer);
            zn_serialize::serialize(buffer, args...);
        }
        void auto_adapt_serialize(t* child, ZnSerializeBuffer& buffer)
        {
            Parent<t, parents_t...>::parent_pack_t::parent_serialize(child, buffer);
        }
        template<typename ...args_t>
        const uint8_t* auto_adapt_deserialize(t* child, const uint8_t* begin, const uint8_t* end, args_t&...args)
        {
            auto p = Parent<t, parents_t...>::parent_pack_t::parent_deserialize(child, begin, end);
            return zn_serialize::deserialize(p, end, args...);
        }
        const uint8_t* auto_adapt_deserialize(t* child, const uint8_t* begin, const uint8_t* end)
        {
            return Parent<t, parents_t...>::parent_pack_t::parent_deserialize(child, begin, end);
        }
    };

    template<typename t>
    struct AutoAdaptBase<t> : public Struct
    {
    protected:
        template<typename...args_t>
        void auto_adapt_serialize(t* child, ZnSerializeBuffer& buffer, const args_t&...args)
        {
            zn_serialize::serialize(buffer, args...);
        }
        template<typename...args_t>
        const uint8_t* auto_adapt_deserialize(t* child, const uint8_t* begin, const uint8_t* end, args_t&...args)
        {
            return zn_serialize::deserialize(begin, end, args...);
        }
    };
};

#define ZN_STRUCT(name,...) struct name : public zn_serialize::AutoAdaptBase<name, ##__VA_ARGS__>

#define ZN_SERIALIZE(...)   void serialize(ZnSerializeBuffer& buffer){ this->auto_adapt_serialize(this, buffer, ##__VA_ARGS__); }\
                            void deserialize(const ZnSerializeBuffer& buffer){ deserialize(buffer.data(), buffer.data() + buffer.size()); }\
                            const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end){ return this->auto_adapt_deserialize(this, begin, end, ##__VA_ARGS__); }
